#ifndef TRAP_H
#define TRAP_H

#include "uart.h"
#include "timer.h"
#include <stdint.h>

struct trap_frame{
    // those are 64-bit reg.
    uint64_t x[32];
    uint64_t sepc;
    uint64_t sstatus;
    uint64_t scause;
    uint64_t stval;
};

// check the total bytes of trap_frame is 288 bytes, 64bit * 36 / 8 = 288 bytes
_Static_assert(sizeof(struct trap_frame) == 288, "trap frame layout mismatch");

void trap_init();
void trap_entry();
void trap_handler(struct trap_frame* trap_frame);
void enter_user(uintptr_t entry, uintptr_t user_sp) __attribute__((noreturn));


#endif
