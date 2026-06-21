.section ".text.boot"
.global _start
_start:
    // TODO: Clear the .bss section
    // TODO: Set up the stack pointer
    la t0, __bss_start
    la t1, __bss_stop
    
1:
    bgeu t0, t1, 2f
    sb zero, 0(t0) // clear one byte
    addi t0, t0, 1 // next byte
    j 1b

2:
    la sp, _end // init stack ptr to stack top
    tail start_kernel // jump to C kernel entry
    
