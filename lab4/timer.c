#include "timer.h"

static uint64_t timer_hz;
static uint64_t kernel_boot_ticks;

// use rdtime to read current timer counter value, return ticks.
uint64_t timer_read_time(){
    uint64_t ticks;
    __asm__ volatile(
        "rdtime %0"
        : "=r"(ticks)
    );
    return ticks;
}

// init and set first timer interrupt
int timer_init(uint32_t frequency, uint64_t boot_ticks){
    if(frequency == 0) return -1;

    timer_hz = frequency;
    kernel_boot_ticks = boot_ticks;

    uint64_t deadline = timer_read_time() + 2*timer_hz;

    if(sbi_set_timer(deadline) != 0) return -1;

    __asm__ volatile(
        "csrs sie, %0\n" // set sie.STIE, STIE means timer interrupt
        "csrsi sstatus, 2\n" // set sstatus.SIE
        :
        : "r"(1UL << 5)
        : "memory" 
    );

    return 0;
}

// set next timer interrupt and print time 
void timer_handle_interrupt(){
    uint64_t current = timer_read_time();

    if(sbi_set_timer(current + 2*timer_hz) != 0){
        __asm__ volatile(
            "csrc sie, %0"
            :
            : "r"(1UL << 5)
            : "memory"
        );
        uart_puts("[Timer] Failed to set timer\n");
        return;
    }
    uint64_t seconds = (current - kernel_boot_ticks) / timer_hz;

    uart_puts("[Timer] boot time : ");
    uart_putd((int)seconds);
    uart_putc('\n');
}