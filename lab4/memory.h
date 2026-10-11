#ifndef MEMORY_H
#define MEMORY_H

#include "fdt.h"
#include <stddef.h>
#include <stdint.h>

#define MAX_RESERVED_REGIONS 32
#define PAGE_SIZE (1UL << 12)

struct memory_region{
    uint64_t start;
    uint64_t end;
};

struct reserved_region{
    uintptr_t start;
    uintptr_t end; // exclusive -> [start, end)
};

const struct memory_region *set_memory_region(const void *fdt);
const struct memory_region *get_memory_region();

int startup_init(uintptr_t start, uintptr_t end);
int startup_allocate(size_t bytes, uintptr_t* address);
void startup_finish();
void memory_reservation_reset();
int memory_reserve(uintptr_t start, size_t size);
size_t memory_reserved_region_count();
const struct reserved_region *memory_get_reserved_region(size_t idx);

#endif
