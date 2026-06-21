#define UART_BASE 0x10000000UL
#define UART_RBR  (volatile unsigned char*)(UART_BASE + 0x0)
#define UART_THR  (volatile unsigned char*)(UART_BASE + 0x0)
#define UART_LSR  (volatile unsigned char*)(UART_BASE + 0x5)
#define LSR_DR    (1 << 0)
#define LSR_TDRQ  (1 << 5)

char uart_getc() {
    // TODO: Implement this function
    while(((*UART_LSR) & LSR_DR) == 0) {}
    
    return (char)(*UART_RBR);
}

void uart_putc(char c) {
    // TODO: Implement this function
    while(((*UART_LSR) & LSR_TDRQ) == 0) {}
    
    *UART_THR = (unsigned char)c;
}

void uart_puts(const char* s) {
    // TODO: Implement this function
    while(*s != '\0'){
        uart_putc(*s);
        s++;
    }
}

void start_kernel() {
    uart_puts("\nStarting kernel ...\n");
    while (1) {
        uart_putc(uart_getc());
    }
}
