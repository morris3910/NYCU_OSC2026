#include <stdint.h>

extern char __image_start[];
extern char uart_getc(void);
extern void uart_putc(char c);
extern void uart_puts(const char* s);
extern void uart_hex(unsigned long h);
extern void uart_init(uintptr_t base);
extern void sbi_get_info(void);
extern int fdt_get_uart_base(const void *fdt, const char *uart_path, uintptr_t *base);
extern int fdt_get_initrd_range(const void *fdt, const char *initrd_path, uint8_t **rd_start, uint8_t **rd_end);
extern int initrd_init(uint8_t *rd_start, uint8_t *rd_end);
extern void initrd_list();
extern void initrd_cat(const char* filename);

#define UART_PATH "/soc/serial"
#define INITRD_PATH "/chosen"
#define CMD_BUF_SIZE 64
static char cmd_buf[CMD_BUF_SIZE];

static int strcmp(const char *s1, const char *s2){
    while(*s1 && *s2){
        if(*s1++ != *s2++) return 0;
    }
    return *s1 == '\0' && *s2 == '\0';
}

static void print_prompt(){
    uart_puts("opi-rv2> ");
}

static void help(){
    uart_puts("Available commands:\n");
    uart_puts("  help  - show all commands.\n");
    uart_puts("  hello - print Hello world.\n");
    uart_puts("  info  - print system info.\n");
    uart_puts("  ls    - list the files.\n");
    uart_puts("  cat   - print the file.\n");
}

static void info(){
    sbi_get_info();
    return;
}

static void execute(const char *cmd){
    if(*cmd == '\0') return;
    
    if(strcmp(cmd, "help")) help();
    else if(strcmp(cmd, "hello")) uart_puts("Hello world.\n");
    else if(strcmp(cmd, "info")) info();
    else if(strcmp(cmd, "ls")) initrd_list();
    else if(cmd[0] == 'c' && cmd[1] == 'a' && cmd[2] == 't' && cmd[3] == ' ') initrd_cat(cmd+4);
    else {
        uart_puts("Unknown command: ");
        uart_puts(cmd);
        uart_puts("\nUse help to get commands.\n");
    }
    return;
}

static void shell(){
    int length = 0;
    int cr_flag = 0;
    print_prompt();
    
    while(1){
        char c = uart_getc(); // get a character from host
        // some terminal pass '\r' or '\n' or "\r\n"
        if (c == '\n' && cr_flag) {
            cr_flag = 0;
            continue;
        }
        
        if (c == '\r' || c == '\n') {
            cr_flag = (c == '\r');

            uart_putc('\n');

            cmd_buf[length] = '\0';
            execute(cmd_buf);

            length = 0;
            print_prompt();
            continue;
        }
        
        cr_flag = 0;
        
        // backspace or delete
        if(c == 0x08 || c == 0x7f) {
            if(length > 0) {
                length--;
                uart_puts("\b \b");
            }
            continue;
        }

        
        // only accept ASCII character
        if(c >= ' ' && c <= '~'){
            if(length < CMD_BUF_SIZE-1){
                uart_putc(c);
                cmd_buf[length++] = c;
            }
        }
    }
}

void start_kernel(unsigned long hartid, unsigned long fdt_addr)
{
    uintptr_t uart_base = 0;
    fdt_get_uart_base((const void *)fdt_addr, UART_PATH, &uart_base);
    uart_init(uart_base);
    
    uart_puts("\n");
    uart_puts("[Mybootloader] UART from Devicetree: ");
    uart_hex((unsigned long)uart_base);
    uart_puts("\n[Mybootloader] Execute image at ");
    uart_hex((unsigned long)__image_start);
    uart_puts(".\n");
    
    uint8_t *rd_start, *rd_end;
    if(fdt_get_initrd_range((const void *)fdt_addr, INITRD_PATH, &rd_start, &rd_end) != 0)
        uart_puts("[Mybootloader] initrd: fail to load.\n\n");
    if(!initrd_init(rd_start, rd_end)) uart_puts("[Mybootloader] initrd: loaded.\n\n");
    else uart_puts("[Mybootloader] initrd: fail to load.\n\n");
    
    shell();
}
