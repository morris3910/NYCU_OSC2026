#ifndef DYNAMIC_ALLOC_H
#define DYNAMIC_ALLOC_H

#include "list.h"
#include "buddy.h"
#include "uart.h"
#include <stddef.h>

#define CHUNK_POOL_NUM 8
#define MAX_CHUNK_SIZE 2048UL
#define MAX_ALLOC_SIZE (PAGE_SIZE << BUDDY_MAX_ORDER)

void dynamic_alloc_init();
void* allocate(size_t size);
void free(void* address);

#endif
