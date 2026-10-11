.section ".text"
.option push    // save the assembler options, use pop to recover
.option norelax // prohibit linker to modify the instruction order

// declare Symbolic Constant, like a C #define
.equ TF_SIZE,    288
.equ TF_SEPC,    256
.equ TF_SSTATUS, 264
.equ TF_SCAUSE,  272
.equ TF_STVAL,   280

.balign 4
.global enter_user

// enter user mode
enter_user:
    // a0 -> user entry (code addr.)
    // a1 -> user stack top
    csrw sepc, a0
    csrw sscratch, sp // save kernel sp

    // clear SPP, SIE, clear SIE to avoid interrupt disturb
    li t0, (1 << 8) | (1 << 1)
    csrc sstatus, t0
    // set SPIE, to recover SIE when sret
    li t0, (1 << 5)
    csrs sstatus, t0

    mv sp, a1 // use user sp
    sret // Supervisor Return

.balign 4
.global trap_entry

// when trap happens
trap_entry:
    // sp -> user sp, sscratch -> kernel sp
    // ensure we use kernel stack, when sscratch is 0 means come from S-mode
    csrrw sp, sscratch, sp // swap sp sscratch
    bnez sp, .Lstack_ready // .L means local symbol, won't be in the symbol table
    csrrw sp, sscratch, sp

.Lstack_ready:
    addi sp, sp, -TF_SIZE // stack grows down

    // store x[0] ~ x[31], expect x[2] as know as sp
    .irp n,1,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
        // \n means parameter n, each reg is 8 bytes,  store x[n] to sp+(n*8)
        // sd is store double word (8 bytes)
        sd x\n, (\n * 8)(sp)
    .endr
    sd zero, 0(sp) // set x[0] to 0

    csrr t0, sstatus // read sstatus to t0
    sd t0, TF_SSTATUS(sp)
    andi t0, t0, (1 << 8) // SPP
    bnez t0, .Lkernel_sp

    // come frome U-mode
    csrr t0, sscratch // user sp is in sscratch
    j .Lsave_sp

.Lkernel_sp:
    // come from S-mode
    addi t0, sp, TF_SIZE // add the TF_SIZE back to get kernel sp

.Lsave_sp:
    sd t0, 16(sp) // store sp to x[2]
    csrw sscratch, zero // clear sscratch

    csrr t0, sepc
    sd t0, TF_SEPC(sp)

    csrr t0, scause
    sd t0, TF_SCAUSE(sp)

    csrr t0, stval
    sd t0, TF_STVAL(sp)

    // switch back to kernel gp, tp before we jump to C code
    la t0, trap_kernel_gp // load addr. 
    ld gp, 0(t0) // load double word for that addr.
    la t0, trap_kernel_tp
    ld tp, 0(t0)

    mv a0, sp // sp points to the trap frame
    call trap_handler

    ld t0, TF_SSTATUS(sp)
    andi t1, t0, (1 << 8) // SPP
    bnez t1, .Lreturn_kernel // back to S-mode

    addi t1, sp, TF_SIZE // back to U-mode
    csrw sscratch, t1 // sscratch store kernel sp
    j .Lrestore_csrs

.Lreturn_kernel:
    csrw sscratch, zero

.Lrestore_csrs:
    csrw sstatus, t0

    ld t0, TF_SEPC(sp)
    csrw sepc, t0

    .irp n,1,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
        ld x\n, (\n * 8)(sp)
    .endr

    ld sp, 16(sp)
    sret

.option pop


