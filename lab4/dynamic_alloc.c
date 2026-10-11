#include "dynamic_alloc.h"

// In RV64, a list node contains two 8-byte pointers, so the minimum chunk is 16 bytes.
static const size_t chunk_sizes[CHUNK_POOL_NUM] = {
    16,
    32,
    64,
    128,
    256,
    512,
    1024,
    2048
};

static struct chunk_pool chunk_pools[CHUNK_POOL_NUM];

static int find_chunk_pool_id(size_t size){
    if(size == 0 || size > MAX_ALLOC_SIZE) return -1;

    int pool_id;
    for(pool_id = 0; pool_id < CHUNK_POOL_NUM; pool_id++){
        if(size <= chunk_sizes[pool_id]) return pool_id;
    }

    return -1;
}

void dynamic_alloc_init(void){
    int pool_id;
    for(pool_id = 0; pool_id < CHUNK_POOL_NUM; pool_id++){
        chunk_pools[pool_id].chunk_size = chunk_sizes[pool_id];
        list_init(&chunk_pools[pool_id].free_chunks);
        chunk_pools[pool_id].free_chunk_count = 0;
        chunk_pools[pool_id].page_count = 0;
    }

    uart_puts("[Dynamic] Initialization completed\n");
}

static int refill_chunk_pool(int pool_id){
    if(pool_id < 0 || pool_id >= CHUNK_POOL_NUM) return -1;

    struct chunk_pool *pool = &chunk_pools[pool_id];
    void *page = buddy_allocate_pages(0);
    if(page == NULL) return -1;

    if(buddy_set_chunk_pool(page, pool_id) < 0){
        buddy_free_pages(page);
        return -1;
    }

    uintptr_t page_address = (uintptr_t)page;
    size_t chunks_num = PAGE_SIZE / pool->chunk_size;

    size_t i;
    for(i = 0; i < chunks_num; i++){
        uintptr_t chunk_address = page_address + i * pool->chunk_size;
        struct list_node *chunk_node = (struct list_node *)chunk_address;

        list_init(chunk_node);
        list_add_tail(&pool->free_chunks, chunk_node);
        pool->free_chunk_count++;
    }

    pool->page_count++;

    uart_puts("[Chunk] Add page ");
    uart_hex(page_address);
    uart_puts(" to chunk pool size ");
    uart_putd((int)pool->chunk_size);
    uart_puts(". Chunk count: ");
    uart_putd((int)chunks_num);
    uart_puts("\n");

    return 0;
}

int size_to_buddy_order(size_t size){
    if(size == 0 || size > MAX_ALLOC_SIZE) return -1;

    size_t required_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    size_t block_pages = 1;
    int order = 0;

    while(block_pages < required_pages){
        block_pages <<= 1;
        order++;
    }

    if(order > (int)BUDDY_MAX_ORDER) return -1;
    return order;
}

static int chunk_is_already_free(struct chunk_pool *pool,
                                 struct list_node *target){
    struct list_node *head = &pool->free_chunks;
    struct list_node *cur = head->next;

    while(cur != head){
        if(cur == target) return 1;
        cur = cur->next;
    }

    return 0;
}

void *allocate(size_t size){
    if(size == 0 || size > MAX_ALLOC_SIZE) return NULL;

    // Requests larger than the largest chunk pool use the buddy allocator.
    if(size > MAX_CHUNK_SIZE){
        int order = size_to_buddy_order(size);
        if(order < 0) return NULL;

        return buddy_allocate_pages((unsigned int)order);
    }

    int pool_id = find_chunk_pool_id(size);
    if(pool_id < 0) return NULL;

    struct chunk_pool *pool = &chunk_pools[pool_id];
    if(list_empty(&pool->free_chunks)){
        if(refill_chunk_pool(pool_id) != 0) return NULL;
    }

    struct list_node *chunk = list_pop_front(&pool->free_chunks);
    if(chunk == NULL) return NULL;

    if(pool->free_chunk_count > 0) pool->free_chunk_count--;

    uintptr_t chunk_address = (uintptr_t)chunk;
    uart_puts("[Chunk] Allocate ");
    uart_hex(chunk_address);
    uart_puts(" at chunk size ");
    uart_putd((int)pool->chunk_size);
    uart_puts("\n");

    return (void *)chunk;
}

void free(void *address){
    if(address == NULL) return;

    int pool_id = buddy_get_chunk_pool_id(address);

    // A page without a chunk-pool ID is a normal buddy allocation.
    if(pool_id == -1){
        buddy_free_pages(address);
        return;
    }

    if(pool_id < 0 || pool_id >= CHUNK_POOL_NUM){
        uart_puts("[Chunk] Invalid chunk-pool ID\n");
        return;
    }

    struct chunk_pool *pool = &chunk_pools[pool_id];
    uintptr_t chunk_address = (uintptr_t)address;
    uintptr_t page_base_address = chunk_address & ~(PAGE_SIZE - 1UL);
    size_t offset = chunk_address - page_base_address;

    if(offset % pool->chunk_size != 0){
        uart_puts("[Chunk] Free address is not chunk-aligned\n");
        return;
    }

    struct list_node *chunk = (struct list_node *)chunk_address;
    if(chunk_is_already_free(pool, chunk)){
        uart_puts("[Chunk] Chunk is already free\n");
        return;
    }

    list_init(chunk);
    list_add_tail(&pool->free_chunks, chunk);
    pool->free_chunk_count++;

    uart_puts("[Chunk] Free ");
    uart_hex(chunk_address);
    uart_puts(" at chunk size ");
    uart_putd((int)pool->chunk_size);
    uart_puts("\n");
}
