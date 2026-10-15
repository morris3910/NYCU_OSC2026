#include "trap.h"

uintptr_t trap_kernel_gp, trap_kernel_tp;

void trap_init(){
    // close interrupt
    asm volatile( // volatile tells compiler do not delete this instruction due to no C-visible output
        "csrci sstatus, 2\n" // csrci -> CSR Clear Immediate, it set bit 1 (2 is 0b10) to zero, means set SIE (Supervisor Interrupt Enable) to zero
        "csrw sie, zero\n" // csrw -> CSR Write, close individual interrupt, sstatus.sie is a global interrupt controller
        ::
        : "memory"
    );

    // save kernel global pointer to trap_kernel_gp
    asm volatile(
        "mv %0, gp" // move kernel global pointer to output reg.
        : "=r"(trap_kernel_gp) // = means output, r means a general-purpose reg.
    );

    // save kernel thread pointer to trap_kernel_tp
    asm volatile(
        "mv %0, tp"
        : "=r"(trap_kernel_tp)
    );

    // clear sscratch and set stvec to trap_entry
    // sscratch -> the space handler can use
    // stvec -> points to the trap handler entry
    asm volatile(
        "csrw sscratch, zero\n"
        "csrw stvec, %0\n"
        :
        : "r"((uintptr_t)trap_entry)
        : "memory"
    );
}

static void print_value(const char* name, uint64_t value){
    uart_puts(name);
    uart_hex((unsigned long)value);
    uart_putc('\n');
}

void trap_handler(struct trap_frame* trap_frame){
    // handle timer interrupt
    // RV64's timer interrupt sscause : 0x8000000000000005, MSB 1 means interrupt, 5 means timer interrupt
    uint64_t interrupt_bit = 1UL << 63;
    uint64_t cause = trap_frame->scause;

    if((cause & interrupt_bit) && ((cause & ~interrupt_bit) == 5)){
        timer_handle_interrupt();
        return;
    }

    // handle exception
    if(trap_frame->sstatus & (1UL << 8))
        uart_puts("[S-mode trap]\n");
    else
        uart_puts("[U-mode trap]\n");

    print_value("scause = ", trap_frame->scause);
    print_value("sepc = ", trap_frame->sepc);
    print_value("stval = ", trap_frame->stval);
    print_value("a[7] = ", trap_frame->x[17]);

    // scause == 8 means ecall is from U-mode, sstatus's bit 8 is SPP, SPP (Supervisor Previous Privilege) == 0 means before exec. trap's mode is U-mode 
    if(trap_frame->scause == 8 && !(trap_frame->sstatus & (1UL << 8))){
        trap_frame->sepc += 4; // sepc points to ecall, so we need to add 4, move to next instruction.
        return;
    }

    uart_puts("[Trap] Unhandled exception\n");
    while(1) {}
}