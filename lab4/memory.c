#include "memory.h"
#include "uart.h"

static uintptr_t startup_cursor;
static uintptr_t startup_end;
static int startup_active;
static struct memory_region physical_memory;
static struct reserved_region reserved_regions[MAX_RESERVED_REGIONS];
static size_t reserved_region_count;

static uintptr_t align_up(uintptr_t value, uintptr_t align){
    return (value + align - 1) & ~(align - 1);
}

static uintptr_t align_down(uintptr_t value, uintptr_t align){
    return value & ~(align - 1);
}

const struct memory_region *set_memory_region(const void *fdt){
    uintptr_t mem_start, mem_end;
    if(fdt_get_memory_range(fdt, "/memory", &mem_start, &mem_end) < 0)
        return NULL;

    if (mem_start >= mem_end)
        return NULL;
    
    physical_memory.start = (uint64_t)mem_start;
    physical_memory.end = (uint64_t)mem_end;
    
    return &physical_memory;
}

const struct memory_region *get_memory_region(){
    if(physical_memory.start >= physical_memory.end) return NULL;

    return &physical_memory;
}

// set startup memory region after buddy_configure_memory(), which will align RAM memory range with page size
int startup_init(uintptr_t aligned_start, uintptr_t aligned_end){
    if(aligned_start >= aligned_end || aligned_start % PAGE_SIZE != 0 || aligned_end % PAGE_SIZE != 0)
        return -1;

    startup_cursor = aligned_start;
    startup_end = aligned_end;
    startup_active = 1;
    return 0;
}

int startup_allocate(size_t bytes, uintptr_t* address){
    if(!startup_active || !address || !bytes)
        return -1;

    // avoid align up overflow
    if(bytes > SIZE_MAX - (PAGE_SIZE-1))
        return -1;

    // align up
    size_t aligned_bytes = (bytes + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    uintptr_t candidate = startup_cursor;
    while(1){
        if(candidate >= startup_end || aligned_bytes > startup_end - candidate)
            return -1;

        uintptr_t candidate_end = candidate + aligned_bytes;
        int overlap = 0;
        
        for(size_t i=0; i<memory_reserved_region_count(); i++){
            const struct reserved_region* r = memory_get_reserved_region(i);

            // candidate overlap with reserved memory region
            if(candidate < r->end && r->start < candidate_end){
                candidate = r->end;
                overlap = 1;
                break;
            }
        }

        if(overlap) continue;

        // mark the memory we allcate a reserved memory region
        if(memory_reserve(candidate, aligned_bytes) < 0)
            return -1;
        
        startup_cursor = candidate_end;
        *address = candidate;
        return 0;
    }
}

void startup_finish(){
    startup_active = 0;
}

void memory_reservation_reset(){
    reserved_region_count = 0;
}

size_t memory_reserved_region_count(){
    return reserved_region_count;
}

int memory_reserve(uintptr_t start, size_t size){
    if(size == 0 || start > (UINTPTR_MAX - size)) return -1; // avoid start + size overflow

    uintptr_t end = start + size;
    uintptr_t memory_start = align_up((uintptr_t)physical_memory.start, PAGE_SIZE);
    uintptr_t memory_end = align_down((uintptr_t)physical_memory.end, PAGE_SIZE);

    if(memory_start >= memory_end) return -1;
    // not in the memory range, dont car
    if(end <= memory_start || start >= memory_end) return 0;
    
    if(start < memory_start) start = memory_start;
    if(end > memory_end) end = memory_end;

    uintptr_t aligned_start = align_down(start, PAGE_SIZE);
    uintptr_t aligned_end = align_up(end, PAGE_SIZE);

    if(aligned_start >= aligned_end) return 0;
    if(reserved_region_count >= MAX_RESERVED_REGIONS) return -1;

    struct reserved_region region;
    region.start = aligned_start;
    region.end = aligned_end;

    reserved_regions[reserved_region_count++] = region;

    uart_puts("[Reserve] ");
    uart_hex((unsigned long)aligned_start);
    uart_puts(" - ");
    uart_hex((unsigned long)aligned_end);
    uart_puts("\n");

    return 0;
}

const struct reserved_region *memory_get_reserved_region(size_t idx){
    if(idx >= reserved_region_count) return NULL;

    return &reserved_regions[idx];
}
