extern char uart_getc(void);
extern void uart_putc(char c);
extern void uart_puts(const char* s);
extern void uart_hex(unsigned long h);

#include<stdint.h>

#define BOOT_MAGIC 0x544F4F42 // BOOT in hex
#define LOAD_ADDR 0x00200000 // load kernel to this addr
//#define LOAD_ADDR 0x80200000 // for QEMU
#define KERNEL_MAX_SIZE (16UL * 1024UL * 1024UL) // 16MiB

typedef void (*kernel_entry_t)(unsigned long hartid, unsigned long fdt_addr); // define a function ptr kernel_entry_t that takes two args

static uint32_t uart_read_le32(){
    uint32_t res = 0;
    for(unsigned int shift=0; shift<32; shift+=8){
        // first convert to 8-bit than convert to 32-bit to prevent from sign extension
        // ex : char range -128 ~ 127, getc() -> -1 -> 0xFF -> 0x000000FF, prevent from 0xFFFFFFFF
        res |= ((uint32_t)(uint8_t)uart_getc()) << shift; 
    }
    return res;
}

void uart_load_kernel(unsigned long hartid, unsigned long fdt_addr){
    uint32_t magic, size;
    volatile uint8_t* dst;

    uart_puts("\r\n[Mybootloader] Waiting for BOOT hdr...\r\n");

    magic = uart_read_le32();
    if(magic != BOOT_MAGIC) {
        uart_puts("[Mybootloader] Error : bad magic.\r\n");
        while(1) {}
    }

    size = uart_read_le32();
    if(size == 0 || size > KERNEL_MAX_SIZE){
        uart_puts("[Mybootloader] Error : invalid image size.\r\n");
        while(1) {}
    }

    uart_puts("[Mybootloader] Receive ");
    uart_hex((unsigned long)size);
    uart_puts(" bytes.\r\n");

    dst = (volatile uint8_t *)LOAD_ADDR;
    for(uint32_t i=0; i<size; i++){
        dst[i] = (uint8_t)uart_getc(); // store one byte to the addr
    }

    uart_puts("[Mybootloader] Image received.\r\n");
    uart_puts("[Mybootloader] Jumping to ");
    uart_hex(LOAD_ADDR);
    uart_puts("...\r\n");

    __asm__ volatile(
        "fence.i" // RISCV instruction, ensure CPU fetch with new data, not the data in instuction cache
        :
        :
        : "memory"
    );

    ((kernel_entry_t)LOAD_ADDR)(hartid, fdt_addr); // convert load_addr to function ptr and pass the args
}
