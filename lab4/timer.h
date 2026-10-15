#ifndef TIMER_H
#define TIMER_H

#include "uart.h"
#include "sbi.h"
#include <stdint.h>

uint64_t timer_read_time();
int timer_init(uint32_t frequency, uint64_t boot_ticks);
void timer_handle_interrupt();

#endif