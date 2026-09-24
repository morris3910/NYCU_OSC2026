#ifndef BUDDY_H
#define BUDDY_H

#include "list.h"
#include "uart.h"
#include "memory.h"
#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE (1UL << 12)
#define BUDDY_MAX_ORDER 19
#define BUDDY_MAX_PAGE_COUNT (1UL << BUDDY_MAX_ORDER)

// frame array's entry's state
enum frame_state{
    FRAME_UNUSED = 0,
    FRAME_FREE, // a free block's start point
    FRAME_ALLOCATED,
    FRAME_INTERIOR, // a free block's body
    FRAME_RESERVED
};

// frame array's entry
struct frame{
    int order;
    int chunk_pool;
    enum frame_state state;
    struct list_node node;
};

void buddy_init();

int buddy_configure_memory(uintptr_t memory_start, uintptr_t memory_end);

uintptr_t buddy_get_memory_base(void);
uintptr_t buddy_get_memory_end(void);
size_t buddy_get_page_count(void);

// why not return uintptr_t, cause uintptr_t usually used to calculate addr, and we want to return a memory pointer
void *buddy_allocate_pages(unsigned int order); 

void buddy_free_pages(void *address);

size_t buddy_free_block_count(unsigned int order);

int buddy_set_chunk_pool(void* page, int pool_id);

int buddy_get_chunk_pool_id(void* address);

#endif

