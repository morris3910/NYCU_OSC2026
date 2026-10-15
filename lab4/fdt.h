#ifndef FDT_H
#define FDT_H

#include <stdint.h>
#include <stddef.h>

#define FDT_MAGIC 0xd00dfeed
#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE   0x00000002
#define FDT_PROP       0x00000003
#define FDT_NOP        0x00000004
#define FDT_END        0x00000009

struct fdt_header {
    uint32_t magic;
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
};

int fdt_path_offset(const void* fdt, const char* path);
const void* fdt_getprop(const void* fdt, int nodeoffset, const char* name, int* lenp);
int fdt_get_uart_base(const void *fdt, const char *uart_path, uintptr_t *base);
int fdt_get_initrd_range(const void *fdt, const char *initrd_path, uint8_t **rd_start, uint8_t **rd_end);
uintptr_t get_fdt_size(const void *fdt);
int fdt_get_memory_range(const void *fdt, const char *mem_path, uintptr_t *mem_start, uintptr_t *mem_end);
int fdt_reserved_memory(const void *fdt);
int fdt_get_timebase_freq(const void *fdt, const char *timebase_path, uint32_t *freqency);

#endif
