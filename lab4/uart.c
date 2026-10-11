#include "uart.h"

static volatile uint8_t *uart_base;

void uart_init(uintptr_t base){
    uart_base = (volatile uint8_t *)base;
}

static volatile uint8_t *uart_reg(unsigned long offset){
    return uart_base + offset;
}

char uart_getc() {
    while(((*(uart_reg(UART_LSR_OFFSET))) & LSR_DR) == 0) {}
    
    char c = (char)(*(uart_reg(UART_RBR_OFFSET)));
    return c;
    //return c == '\r' ? '\n' : c;
}

void uart_putc(char c) {
    if(c == '\n') uart_putc('\r');
    
    while(((*(uart_reg(UART_LSR_OFFSET))) & LSR_TDRQ) == 0) {}
    
    *(uart_reg(UART_THR_OFFSET)) = (char)c;
}

void uart_puts(const char* s) {
    while(*s){
        uart_putc(*s++);
    }
}

void uart_putd(int num){
    if(num == 0){
        uart_puts("0");
        return;
    }
    if(num < 0){
        uart_puts("-");
        num = -num;
    }
    
    char buf[20];
    int i = 0;
    while(num > 0){
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }
    
    for(int j=i-1; j>=0; j--){
        uart_putc(buf[j]);
    }
}


void uart_hex(unsigned long h) {
    uart_puts("0x");
    unsigned long n;
    // unsigned long has 64-bit, each time we take 4-bit and convert it to hex
    for (int c = 60; c >= 0; c -= 4) {
        n = (h >> c) & 0xf;
        // convert 0~9 to '0' ~ '9' and 10 ~ 15 to 'a' ~ 'f'
        n += n > 9 ? 0x57 : '0'; // 0x57 == 87, 10 + 87 = 97, ASCII 97 = 'a'
        uart_putc(n);
    }
}
