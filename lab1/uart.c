#define UART_BASE 0xd4017000UL // orange pi
//#define UART_BASE 0x10000000UL // QEMU

#define UART_RBR  (volatile unsigned char*)(UART_BASE + 0x0)
#define UART_THR  (volatile unsigned char*)(UART_BASE + 0x0)
#define UART_LSR  (volatile unsigned char*)(UART_BASE + 0x14) // orange pi
//#define UART_LSR  (volatile unsigned char*)(UART_BASE + 0x5) // QEMU

#define LSR_DR    (1 << 0)
#define LSR_TDRQ  (1 << 5)

char uart_getc() {
    while(((*UART_LSR) & LSR_DR) == 0) {}
    
    char c = (char)(*UART_RBR);
    return c == '\r' ? '\n' : c;
}

void uart_putc(char c) {
    if(c == '\n') uart_putc('\r');
    
    while(((*UART_LSR) & LSR_TDRQ) == 0) {}
    
    *UART_THR = (char)c;
}

void uart_puts(const char* s) {
    while(*s){
        uart_putc(*s++);
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
