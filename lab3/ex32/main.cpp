#include <iostream>
#include <list>
#include <vector>

#define PAGE_SIZE (1UL << 12)
#define NUM_PAGES 0x280000
#define MAX_ORDER 10

typedef unsigned long phys_addr_t;

struct page {
    int order = 0;
    int refcount = 0;
};

std::vector<page> mem_map;
std::vector<std::list<page*>> free_area;

struct page* get_buddy(struct page* page, unsigned int order) {
    return &mem_map[(page - mem_map.data()) ^ (1 << order)];
}

void memory_reserve(phys_addr_t base, size_t size) {
    // TODO: Implement this function
    if(size == 0) return;

    unsigned long start_pfn = base / PAGE_SIZE;
    unsigned long end_pfn = (base + size + PAGE_SIZE - 1) / PAGE_SIZE; // round up to next pfn(page frame number)

    if(start_pfn >= NUM_PAGES) return;
    if(end_pfn > NUM_PAGES) end_pfn = NUM_PAGES;

    for(int order=MAX_ORDER; order>=0; order--){
        auto it = free_area[order].begin();
        while(it != free_area[order].end()){
            struct page* block = *it;
            unsigned long block_start_pfn = block - mem_map.data();
            unsigned long block_end_pfn = block_start_pfn + (1UL << order);

            // case 1 : no overlap
            if(block_end_pfn <= start_pfn || block_start_pfn >= end_pfn){
                it++;
                continue;
            }

            // case 2 : full overlap
            it = free_area[order].erase(it); // return next avilable iterator
            if(start_pfn <= block_start_pfn && end_pfn >= block_end_pfn){
                block->refcount = 1;
                block->order = order;
                continue;
            }

            // case 3 : partial overlap
            // just one page, must reserved
            if(order == 0){
                block->refcount = 1;
                block->order = order;
                continue;
            }

            struct page* buddy = get_buddy(block, order-1);
            block->order = order-1;
            buddy->order = order-1;
            block->refcount = 0;
            buddy->refcount = 0;

            // check them in next round
            free_area[order-1].push_back(block);
            free_area[order-1].push_back(buddy);
        }
    }
}

void dump() {
    for (int i = MAX_ORDER; i >= 0; i--)
        std::cout << "free_area[" << i << "] " << free_area[i].size()
                  << std::endl;
}

void mm_init() {
    mem_map.resize(NUM_PAGES);
    free_area.resize(MAX_ORDER + 1);
    for (size_t i = 0; i < NUM_PAGES; i += (1 << MAX_ORDER)) {
        mem_map[i].order = MAX_ORDER;
        free_area[MAX_ORDER].push_back(&mem_map[i]);
    }
    memory_reserve(0, 0x82a69510);
}

int main() {
    mm_init();
    dump();
    return 0;
}
