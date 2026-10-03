#include "fdt.h"
#include <stdint.h>
#include <stddef.h>

#define MEMORY_PATH "/memory"
#define INITRD_PATH "/chosen"
//#define LOAD_ADDR 0x00200000 // Orange pi
#define LOAD_ADDR 0x80200000 // for QEMU
#define KERNEL_MAX_SIZE (16UL * 1024UL * 1024UL) // 16MiB
#define RELOC_GUARD_SIZE (64UL * 1024UL) // 64KiB, leave a space between sections.
#define PAGE_SIZE 0x1000UL // 16^3 = 4 KiB = 1 page size, used to align to a clear new page

extern void uboot_start_kernel(unsigned long hartid, unsigned long fdt_addr) __attribute__((noreturn)); // noreturn tells compiler that this function won't return.
extern char __image_start[];
extern char __image_end[];
extern char __bss_start[];
extern char __bss_stop[];
extern char __stack_top[];

static uintptr_t align_down(const uintptr_t ptr, const uintptr_t byte){
    return ptr & ~(byte - 1);
}

static int ranges_overlap(uintptr_t a_start,
                          uintptr_t a_end,
                          uintptr_t b_start,
                          uintptr_t b_end){
    return a_start < b_end && b_start < a_end;
}

static int is_bad_relocate_addr(uintptr_t dst,
                                uintptr_t dst_end,
                                uintptr_t fdt_start,
                                uintptr_t fdt_end,
                                uintptr_t initrd_start,
                                uintptr_t initrd_end){
    // don't put at standard entry region
    if(ranges_overlap(dst, dst_end, LOAD_ADDR, LOAD_ADDR + KERNEL_MAX_SIZE))
        return -1;
    
    // don't put at fdt region
    if(ranges_overlap(dst, dst_end, fdt_start, fdt_end))
        return -1;
    
    // don't put at initrd region
    if(ranges_overlap(dst, dst_end, initrd_start, initrd_end))
        return -1;

    return 0;
}

static uintptr_t choose_relocate_addr(const void *fdt, uintptr_t occupied_size){
    uintptr_t mem_start, mem_end, fdt_start, fdt_end, initrd_start, initrd_end;
    uint8_t *rd_start, *rd_end;
    fdt_get_memory_range(fdt, MEMORY_PATH, &mem_start, &mem_end);
    fdt_get_initrd_range(fdt, INITRD_PATH, &rd_start, &rd_end);
    fdt_start = (uintptr_t)fdt;
    fdt_end = fdt_start + get_fdt_size(fdt);
    initrd_start = (uintptr_t)rd_start;
    initrd_end = (uintptr_t)rd_end;

    uintptr_t candidate = align_down(mem_end - occupied_size - RELOC_GUARD_SIZE, PAGE_SIZE);

    while(candidate >= mem_start + PAGE_SIZE){
        uintptr_t dst = candidate, dst_end = candidate + occupied_size;

        if(dst_end <= mem_end && !is_bad_relocate_addr(dst, dst_end, fdt_start, fdt_end, initrd_start, initrd_end)){
            return dst;
        }

        candidate -= PAGE_SIZE; // check next page
    }
}

static void copy_image(uint8_t *src, uint8_t *dst, uintptr_t size){
    for(uintptr_t i=0; i<size; i++)
        dst[i] = src[i];
}

static void clear_bss(uint8_t *ptr, uintptr_t size){
    for(uintptr_t i=0; i<size; i++)
        ptr[i] = 0;
}

void relocate_and_enter(unsigned long hartid, unsigned long fdt_addr){
    uintptr_t image_size = (uintptr_t)__image_end - (uintptr_t)__image_start;
    uintptr_t bss_offset = (uintptr_t)__bss_start - (uintptr_t)__image_start;
    uintptr_t bss_size = (uintptr_t)__bss_stop - (uintptr_t)__bss_start;
    uintptr_t occupied_size = (uintptr_t)__stack_top - (uintptr_t)__image_start;
    uintptr_t entry_offset = (uintptr_t)uboot_start_kernel - (uintptr_t)__image_start;
    uintptr_t stack_offset = (uintptr_t)__stack_top - (uintptr_t)__image_start;
    
    uintptr_t new_base = choose_relocate_addr((const void *)fdt_addr, occupied_size);

    copy_image((uint8_t *)__image_start, (uint8_t *)new_base, image_size);
    clear_bss((uint8_t *)(new_base + bss_offset), bss_size);

    uintptr_t new_entry = new_base + entry_offset;
    uintptr_t new_stack = new_base + stack_offset;

    // RISCV instruction, ensure CPU fetch with new data, not the data in instuction cache
    // memory means telling compiler this instruction will affect memory, so don't re-arrange memory
    /*
    __asm__ volatile(
        "assembly template"
        : output operands
        : input operands
        : clobber list // memory, cc, t0
    );
    */
    __asm__ volatile("fence.i" ::: "memory");

    __asm__ volatile(
        "mv sp, %0\n" // %0 -> new_stack
        "mv a0, %1\n" // a0 is argument1, %1 -> hartid
        "mv a1, %2\n" // a1 is argument2, %2 -> fdt_addr
        "jr %3\n" // jump to new kernel entry
        :
        : "r"(new_stack), "r"(hartid), "r"(fdt_addr), "r"(new_entry)
        : "memory"
    );
}
