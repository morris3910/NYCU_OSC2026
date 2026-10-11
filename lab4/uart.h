#ifndef UART_H
#define UART_H

#include <stdint.h>

#define UART_RBR_OFFSET  0x0
#define UART_THR_OFFSET  0x0
//#define UART_LSR_OFFSET  0x14 // orange pi
#define UART_LSR_OFFSET  0x5 // QEMU

#define LSR_DR    (1 << 0)
#define LSR_TDRQ  (1 << 5)

void uart_init(uintptr_t base);
char uart_getc();
void uart_putc(char c);
void uart_puts(const char* s);
void uart_putd(int num);
void uart_hex(unsigned long h);

#endif
