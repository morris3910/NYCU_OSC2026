#include "buddy.h"
#include "list.h"
#include "uart.h"
#include "fdt.h"
#include "initrd.h"
#include "dynamic_alloc.h"
#include "memory.h"
#include "trap.h"
#include "user.h"
#include <stdint.h>

extern char __image_start[];
extern char __image_end[];
extern void sbi_get_info(void);

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
    uart_puts("  exec  - run prog.bin in U-mode.\n");
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
    else if(strcmp(cmd, "exec")) exec_user("prog.bin");
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

void test_alloc_1() {
    uart_puts("Testing memory allocation...\n");
    char *ptr1 = (char *)allocate(4000);
    char *ptr2 = (char *)allocate(8000);
    char *ptr3 = (char *)allocate(4000);
    char *ptr4 = (char *)allocate(4000);

    free(ptr1);
    free(ptr2);
    free(ptr3);
    free(ptr4);

    /* Test kmalloc */
    uart_puts("Testing dynamic allocator...\n");
    char *kmem_ptr1 = (char *)allocate(16);
    char *kmem_ptr2 = (char *)allocate(32);
    char *kmem_ptr3 = (char *)allocate(64);
    char *kmem_ptr4 = (char *)allocate(128);

    free(kmem_ptr1);
    free(kmem_ptr2);
    free(kmem_ptr3);
    free(kmem_ptr4);

    char *kmem_ptr5 = (char *)allocate(16);
    char *kmem_ptr6 = (char *)allocate(32);

    free(kmem_ptr5);
    free(kmem_ptr6);

    // Test allocate new page if the cache is not enough
    void *kmem_ptr[102];
    for (int i=0; i<100; i++) {
        kmem_ptr[i] = (char *)allocate(128);
    }
    for (int i=0; i<100; i++) {
        free(kmem_ptr[i]);
    }

    // Test exceeding the maximum size
    char *kmem_ptr7 = (char *)allocate(MAX_ALLOC_SIZE + 1);
    if (kmem_ptr7 == NULL) {
        uart_puts("Allocation failed as expected for size > MAX_ALLOC_SIZE\n");
    }
    else {
        uart_puts("Unexpected allocation success for size > MAX_ALLOC_SIZE\n");
        free(kmem_ptr7);
    }
}

void start_kernel(unsigned long hartid, unsigned long fdt_addr)
{
    // set UART
    uintptr_t uart_base = 0;
    fdt_get_uart_base((const void *)fdt_addr, UART_PATH, &uart_base);
    uart_init(uart_base);
    
    uart_puts("\n");
    uart_puts("[Mybootloader] UART from Devicetree: ");
    uart_hex((unsigned long)uart_base);
    uart_puts("\n[Mybootloader] Execute image at ");
    uart_hex((unsigned long)__image_start);
    uart_puts(".\n");

    // trap init
    trap_init();
    
    // set initrd
    uint8_t *rd_start, *rd_end;
    if(fdt_get_initrd_range((const void *)fdt_addr, INITRD_PATH, &rd_start, &rd_end) != 0)
        uart_puts("[Mybootloader] initrd: fail to load.\n\n");
    if(!initrd_init(rd_start, rd_end)) uart_puts("[Mybootloader] initrd: loaded.\n\n");
    else uart_puts("[Mybootloader] initrd: fail to load.\n\n");

    // find memory range
    const struct memory_region *region = set_memory_region((const void *)fdt_addr);
    if(!region) {
        uart_puts("[Memory] Cannot find memory region\n");
    }
    if(buddy_configure_memory(region->start, region->end) < 0){
        uart_puts("[Memory] Cannot configure buddy memory\n");
    }

    // reserve memory
    memory_reservation_reset();
    if(fdt_reserved_memory((const void *)fdt_addr) < 0)
        uart_puts("[Reserve] Failed to reserve Devicetree reserved-memory\n");
    else
        uart_puts("[Reserve] Reserve Devicetree reserved-memory succeeded\n");
    if(memory_reserve((uintptr_t)fdt_addr, (size_t)get_fdt_size((const void *)fdt_addr)) < 0)
        uart_puts("[Reserve] Failed to reserve DTB\n");
    else
        uart_puts("[Reserve] Reserve DTB succeeded\n");
    if(memory_reserve((uintptr_t)__image_start, (size_t)((uintptr_t)__image_end - (uintptr_t)__image_start)))
        uart_puts("[Reserve] Failed to reserve kernel\n");
    else
        uart_puts("[Reserve] Reserve kernel image succeeded\n");
    if(memory_reserve((uintptr_t)rd_start, (size_t)((uintptr_t)rd_end - (uintptr_t)rd_start)))
        uart_puts("[Reserve] Failed to reserve initramfs\n");
    else
        uart_puts("[Reserve] Reserve initramfs succeeded\n\n");

    // startup allocator to allocate frame array
    if(startup_init(buddy_get_memory_base(), buddy_get_memory_end()) < 0)
        uart_puts("[Startup] Initialization failed\n");
    else
        uart_puts("[Startup] Initialization completed\n");

    // init memory allocator
    if(buddy_init() < 0)
        uart_puts("[Buddy] Initialization failed\n");
    else
        uart_puts("[Buddy] Initialization completed\n");
    
    startup_finish();
    dynamic_alloc_init();
    //test_alloc_1();
    
    shell();
}
