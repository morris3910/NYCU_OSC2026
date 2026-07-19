#include <iostream>
#include <list>
#include <vector>

#define NUM_PAGES 0x280000
#define MAX_ORDER 10

struct page {
    int order = 0;
    int refcount = 0;
};

std::vector<page> mem_map;
std::vector<std::list<page*>> free_area;

struct page* get_buddy(struct page* page, unsigned int order) {
    return &mem_map[(page - mem_map.data()) ^ (1 << order)];
}

struct page* alloc_pages(unsigned int order) {
    // TODO: Implement this function
    if(order > MAX_ORDER) return nullptr;

    struct page* page;
    int free_order;
    for(free_order=order; free_order<= MAX_ORDER; free_order++){
        if(!free_area[free_order].empty()) break;
    }

    for(int i=free_order; i>order; i--){
        page = free_area[i].front();
        free_area[i].pop_front();

        struct page* buddy = get_buddy(page, i-1);
        page->order = i - 1;
        buddy->order = i - 1;
        buddy->refcount = 0;
        free_area[i-1].push_back(page);
        free_area[i-1].push_back(buddy);
    }

    page = free_area[order].front();
    free_area[order].pop_front();
    page->refcount = 1;
    page->order = order;

    return page;
}

void free_pages(struct page* page) {
    // TODO: Implement this function
    page->refcount = 0;
    int order = page->order;

    struct page* buddy = get_buddy(page, order);
    while(order < MAX_ORDER && !buddy->refcount && buddy->order == order){
        free_area[page->order].remove(buddy);
        if(buddy < page){
            page = buddy;
            page->refcount = 0;
        }
        order++;
        page->order = order;
        buddy = get_buddy(page, order);
    }

    free_area[page->order].push_back(page);
}

void dump() {
    for (int i = MAX_ORDER; i >= 0; i--)
        std::cout << "free_area[" << i << "] " << free_area[i].size()
                  << std::endl;
}

int main() {
    mem_map.resize(NUM_PAGES);
    free_area.resize(MAX_ORDER + 1);
    for (size_t i = 0; i < NUM_PAGES; i += (1 << MAX_ORDER)) {
        mem_map[i].order = MAX_ORDER;
        free_area[MAX_ORDER].push_back(&mem_map[i]);
    }

    std::cout << "\np1:\n";
    struct page* p1 = alloc_pages(1);
    dump();

    std::cout << "\np2:\n";
    struct page* p2 = alloc_pages(1);
    dump();

    std::cout << "\np3:\n";
    struct page* p3 = alloc_pages(1);
    dump();

    free_pages(p1);
    free_pages(p2);
    free_pages(p3);

    std::cout << "\nfree:\n";
    dump();
    return 0;
}
