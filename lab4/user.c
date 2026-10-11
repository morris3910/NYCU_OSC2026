#include "user.h"

void exec_user(const char* filename){
    const void* file_data;
    size_t file_size;

    if(initrd_find(filename, &file_data, &file_size) < 0 || !file_size){
        uart_puts("[Exec] File not found or empty.\n");
        return;
    }

    int order = size_to_buddy_order(file_size);
    if(order < 0){
        uart_puts("[Exec] File too large.\n");
        return;
    }
    
    void* code = buddy_allocate_pages((unsigned int)order);
    if(!code){
        uart_puts("[Exec] Can't allocate code.\n");
        return;
    }

    void* stack = buddy_allocate_pages(0);
    if(!stack){
        buddy_free_pages(code);
        uart_puts("[Exec] Can't allocate stack.\n");
        return;
    }

    for(size_t i=0; i<file_size; i++){
        ((uint8_t*)code)[i] = ((const uint8_t*)file_data)[i];
    }

    uintptr_t stack_top = (uintptr_t)stack + PAGE_SIZE;

    uart_puts("[Exec] Entry: ");
    uart_hex((unsigned long)code);
    uart_putc('\n');

    __asm__ volatile (
        "fence.i" 
        :
        :
        : "memory"
    );

    enter_user((uintptr_t)code, stack_top);
}