#include "buddy.h"

// RAM
static uintptr_t RAM_memory_base;
static uintptr_t RAM_memory_end;
static size_t RAM_page_count;

// The frame array
static struct frame frame_array[BUDDY_MAX_PAGE_COUNT];

// A free list for every order
static struct list_node free_area[BUDDY_MAX_ORDER + 1];

static uintptr_t align_up(uintptr_t value, uintptr_t align){
    return (value + align - 1) & ~(align - 1);
}

static uintptr_t align_down(uintptr_t value, uintptr_t align){
    return value & ~(align - 1);
}

int buddy_configure_memory(uintptr_t memory_start, uintptr_t memory_end){
    if(memory_start >= memory_end) return -1;

    // Buddy system only handle 4KB pages
    uintptr_t align_start = align_up(memory_start, PAGE_SIZE);
    uintptr_t align_end = align_down(memory_end, PAGE_SIZE);

    if(align_start >= align_end) return -1;

    size_t page_counts = (align_end - align_start) / PAGE_SIZE;
    if(page_counts > BUDDY_MAX_PAGE_COUNT) return -1;

    RAM_memory_base = align_start;
    RAM_memory_end = align_end;
    RAM_page_count = page_counts;
    
    return 0;
}

uintptr_t buddy_get_memory_base(void){
    return RAM_memory_base;
}

uintptr_t buddy_get_memory_end(void){
    return RAM_memory_end;
}

size_t buddy_get_page_count(void){
    return RAM_page_count;
}

static void print_remove_pages(size_t idx, unsigned int order){
    uart_puts("[-] Remove page ");
    uart_putd((int)idx);
    uart_puts(" from order ");
    uart_putd((int)order);
    uart_puts(". Range of pages: [");
    uart_putd((int)idx);
    uart_puts(", ");
    uart_putd((int)(idx + (1UL << order) - 1));
    uart_puts("]\n");
}

static void print_add_pages(size_t idx, unsigned int order){
    uart_puts("[+] Add page ");
    uart_putd((int)idx);
    uart_puts(" to order ");
    uart_putd((int)order);
    uart_puts(". Range of pages: [");
    uart_putd((int)idx);
    uart_puts(", ");
    uart_putd((int)(idx + (1UL << order) - 1));
    uart_puts("]\n");
}

static void print_find_buddy(size_t buddy_idx, size_t idx, unsigned int order){
    uart_puts("[*] Buddy found! buddy idx: ");
    uart_putd((int)buddy_idx);
    uart_puts(" for page ");
    uart_putd((int)idx);
    uart_puts(" with order ");
    uart_putd((int)order);
    uart_puts("\n");
}

static void print_page_allocate(size_t idx, unsigned int order){
    uintptr_t address = RAM_memory_base + idx * PAGE_SIZE;
    uintptr_t next_address = address + (1UL << order) * PAGE_SIZE;

    uart_puts("[Page] Allocate ");
    uart_hex(address);
    uart_puts(" at order ");
    uart_putd((int)order);
    uart_puts(", page ");
    uart_putd((int)idx);
    uart_puts(". Next address at order ");
    uart_putd((int)order);
    uart_puts(": ");
    uart_hex(next_address);
    uart_puts("\n");
}

static void print_page_free(uintptr_t address, size_t idx, unsigned int order){
    uintptr_t next_address = RAM_memory_base
                           + (idx + (1UL << order)) * PAGE_SIZE;

    uart_puts("[Page] Free ");
    uart_hex(address);
    uart_puts(" and add back to order ");
    uart_putd((int)order);
    uart_puts(", page ");
    uart_putd((int)idx);
    uart_puts(". Next address at order ");
    uart_putd((int)order);
    uart_puts(": ");
    uart_hex(next_address);
    uart_puts("\n");
}

// Cut remaining pages into a maximum-order aligned block.
static unsigned int buddy_find_available_order(size_t idx, size_t remain_pages){
    unsigned int order = BUDDY_MAX_ORDER;

    while(order > 0){
        size_t block_pages = (1UL << order);
        if(block_pages <= remain_pages && !(idx & (block_pages - 1))){
            return order;
        }
        order--;
    }

    return 0;
}

// Mark one block in the frame array.
static void buddy_mark_block(size_t idx,
                             unsigned int order,
                             enum frame_state head_state){
    size_t block_pages = (1UL << order);

    frame_array[idx].order = (int)order;
    frame_array[idx].state = head_state;
    frame_array[idx].chunk_pool = -1;
    list_init(&frame_array[idx].node);

    size_t i;
    for(i = 1; i < block_pages; i++){
        frame_array[idx + i].order = -1;
        frame_array[idx + i].state = FRAME_INTERIOR;
        frame_array[idx + i].chunk_pool = -1;
        list_init(&frame_array[idx + i].node);
    }
}

// Put a free block into free_area[order].
static void buddy_add_free_block(size_t idx, unsigned int order){
    buddy_mark_block(idx, order, FRAME_FREE);
    list_add_tail(&free_area[order], &frame_array[idx].node);
    print_add_pages(idx, order);
}

static void buddy_mark_reserved_region(uintptr_t start, uintptr_t end){
    if(start < RAM_memory_base) start = RAM_memory_base;
    if(end > RAM_memory_end) end = RAM_memory_end;

    if(start >= end) return;

    size_t start_idx = (start - RAM_memory_base) / PAGE_SIZE;
    size_t end_idx = (end - RAM_memory_base) / PAGE_SIZE;
    size_t i;

    // end is exclusive
    for(i = start_idx; i < end_idx; i++){
        frame_array[i].state = FRAME_RESERVED;
        frame_array[i].order = -1;
        frame_array[i].chunk_pool = -1;
        list_init(&frame_array[i].node);
    }
}

void buddy_init(void){
    unsigned int order;
    for(order = 0; order <= BUDDY_MAX_ORDER; order++){
        list_init(&free_area[order]);
    }

    size_t idx;
    for(idx = 0; idx < RAM_page_count; idx++){
        frame_array[idx].order = -1;
        frame_array[idx].chunk_pool = -1;
        frame_array[idx].state = FRAME_UNUSED;
        list_init(&frame_array[idx].node);
    }

    size_t reserved_count = memory_reserved_region_count();
    for(idx = 0; idx < reserved_count; idx++){
        const struct reserved_region *region = memory_get_reserved_region(idx);
        buddy_mark_reserved_region(region->start, region->end);
    }

    idx = 0;
    while(idx < RAM_page_count){
        // skip the reserved pages
        if(frame_array[idx].state == FRAME_RESERVED){
            idx++;
            continue;
        }

        // find a usable range, [range_start, range_end)
        size_t range_start = idx;
        while(idx < RAM_page_count && frame_array[idx].state != FRAME_RESERVED)
            idx++;
        size_t range_end = idx;

        size_t cur = range_start;
        while(cur < range_end){
            size_t remaining_pages = range_end - cur;
            unsigned int block_order = buddy_find_available_order(cur, remaining_pages);
            buddy_add_free_block(cur, block_order);
            cur += (1UL << block_order);
        }
    }
    uart_puts("[Buddy] Initialization completed\n");
}

size_t buddy_free_block_count(unsigned int order){
    if(order > BUDDY_MAX_ORDER) return 0;

    struct list_node *head = &free_area[order];
    struct list_node *cur = head->next;
    size_t count = 0;

    while(cur != head){
        count++;
        cur = cur->next;
    }

    return count;
}

void *buddy_allocate_pages(unsigned int order){
    if(order > BUDDY_MAX_ORDER) return NULL;

    unsigned int free_order = order;
    while(free_order <= BUDDY_MAX_ORDER
          && list_empty(&free_area[free_order])){
        free_order++;
    }

    if(free_order > BUDDY_MAX_ORDER) return NULL;

    struct list_node *node = list_pop_front(&free_area[free_order]);
    if(node == NULL) return NULL;

    struct frame *frame = list_entry(node, struct frame, node);
    size_t idx = (size_t)(frame - frame_array);

    // The selected block is removed from exactly one free list.
    print_remove_pages(idx, free_order);

    // Keep splitting the left half and release each redundant right half.
    while(free_order > order){
        free_order--;

        size_t half_pages = (1UL << free_order);
        size_t right_idx = idx + half_pages;

        buddy_mark_block(idx, free_order, FRAME_FREE);
        buddy_add_free_block(right_idx, free_order);
    }

    buddy_mark_block(idx, order, FRAME_ALLOCATED);
    print_page_allocate(idx, order);

    return (void *)(RAM_memory_base + idx * PAGE_SIZE);
}

void buddy_free_pages(void *address){
    if(address == NULL) return;

    uintptr_t phy_addr = (uintptr_t)address;

    // RAM_memory_end is treated as an exclusive upper bound.
    if(phy_addr < RAM_memory_base || phy_addr >= RAM_memory_end){
        uart_puts("[Buddy] Invalid free address\n");
        return;
    }

    if((phy_addr - RAM_memory_base) % PAGE_SIZE != 0){
        uart_puts("[Buddy] Free address is not page-aligned\n");
        return;
    }

    size_t idx = (phy_addr - RAM_memory_base) / PAGE_SIZE;
    if(frame_array[idx].state != FRAME_ALLOCATED){
        uart_puts("[Buddy] Block is not allocated\n");
        return;
    }

    unsigned int order = (unsigned int)frame_array[idx].order;
    buddy_mark_block(idx, order, FRAME_FREE);

    // Iteratively find and coalesce a free buddy of the same order.
    while(order < BUDDY_MAX_ORDER){
        size_t buddy_idx = idx ^ (1UL << order);
        if(buddy_idx >= RAM_page_count) break;

        if(frame_array[buddy_idx].order != (int)order
           || frame_array[buddy_idx].state != FRAME_FREE){
            break;
        }

        print_find_buddy(buddy_idx, idx, order);
        print_remove_pages(buddy_idx, order);
        list_del(&frame_array[buddy_idx].node);

        idx = (buddy_idx < idx) ? buddy_idx : idx;
        order++;
        buddy_mark_block(idx, order, FRAME_FREE);
    }

    buddy_add_free_block(idx, order);
    print_page_free(phy_addr, idx, order);
}

// Associate an allocated order-0 page with a dynamic-allocation pool.
int buddy_set_chunk_pool(void *page, int pool_id){
    if(page == NULL || pool_id < 0) return -1;

    uintptr_t phy_addr = (uintptr_t)page;
    if(phy_addr < RAM_memory_base || phy_addr >= RAM_memory_end){
        return -1;
    }

    if((phy_addr - RAM_memory_base) % PAGE_SIZE != 0) return -1;

    size_t idx = (phy_addr - RAM_memory_base) / PAGE_SIZE;
    if(frame_array[idx].state != FRAME_ALLOCATED || frame_array[idx].order != 0){
        return -1;
    }

    frame_array[idx].chunk_pool = pool_id;
    return 0;
}

int buddy_get_chunk_pool_id(void *address){
    if(address == NULL) return -1;

    uintptr_t phy_addr = (uintptr_t)address;
    if(phy_addr < RAM_memory_base || phy_addr >= RAM_memory_end){
        return -1;
    }

    size_t idx = (phy_addr - RAM_memory_base) / PAGE_SIZE;
    return frame_array[idx].chunk_pool;
}
