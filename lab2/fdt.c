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

static inline const void* align_up(const void* ptr, size_t align) {
    return (const void*)(((uintptr_t)ptr + align - 1) & ~(align - 1));
}

// read a 32-bit big-endian integer
static uint32_t read_be32(const void *ptr){
    const uint8_t *p = (const uint8_t *)ptr;

    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  |
           ((uint32_t)p[3]);
}

static size_t fdt_strlen(const char *s){
    const char *p = s;

    while(*p)
        p++;

    return (size_t)(p - s);
}

static int fdt_strcmp(const char *s1, const char *s2){
    while(*s1 && *s1 == *s2){
        s1++;
        s2++;
    }

    return (unsigned char)*s1 - (unsigned char)*s2;
}

static const char *fdt_strchr(const char *s, int c){
    char ch = (char)c;

    while(*s){
        if(*s == ch)
            return s;
        s++;
    }

    return ch == '\0' ? s : NULL;
}

static int fdt_memcmp(const void *s1, const void *s2, size_t n){
    const uint8_t *p1 = s1;
    const uint8_t *p2 = s2;

    while(n--){
        if(*p1 != *p2)
            return (int)*p1 - (int)*p2;
        p1++;
        p2++;
    }

    return 0;
}

static const void *fdt_memchr(const void *s, int c, size_t n){
    const uint8_t *p = s;
    uint8_t ch = (uint8_t)c;

    while(n--){
        if(*p == ch)
            return p;
        p++;
    }

    return NULL;
}

// skip entire node including its child node
static const uint8_t *skip_node(const uint8_t *ptr){
    int depth = 0;
    while(1){
        uint32_t tag = read_be32(ptr);
        ptr += sizeof(uint32_t); // skip token
        
        switch(tag){
            case FDT_BEGIN_NODE:
                depth++;
                ptr += fdt_strlen((const char *)ptr) + 1; // skip node name and '\0'
                ptr = align_up(ptr, 4);
                break;
            case FDT_END_NODE:
                depth--;
                if(!depth) return ptr;
                break;
            case FDT_PROP:
                uint32_t len = read_be32(ptr);
                ptr += 2 * sizeof(uint32_t); // skip prop_len and prop_off
                ptr += len; // skip prop data
                ptr = align_up(ptr, 4);
                break;
            case FDT_NOP:
                break;
            default:
                return NULL;
        }
    }
}

// match only node name or node name with unit addr (name@unit-addr).
static int node_name_matches(const char *node_name, const char *wanted, size_t wanted_len){
    size_t node_len = fdt_strlen(node_name);

    if(node_len == wanted_len && !fdt_memcmp(node_name, wanted, wanted_len))
        return 1;

    return node_len > wanted_len && node_name[wanted_len] == '@' &&
           !fdt_memchr(wanted, '@', wanted_len) &&
           !fdt_memcmp(node_name, wanted, wanted_len);
}

// find the child we wanted and return offset
static int find_child(const uint8_t *struct_base,
                      int parent_off,
                      const char *wanted,
                      size_t wanted_len){
    const uint8_t *ptr = struct_base + parent_off;
    if(read_be32(ptr) != FDT_BEGIN_NODE) return -1;
    
    ptr += sizeof(uint32_t); // skip parent token
    ptr += fdt_strlen((const char *)ptr) + 1; // skip node name and '\0'
    ptr = align_up(ptr, 4);
    
    while(1){
        uint32_t tag = read_be32(ptr);
        
        switch(tag){
            case FDT_BEGIN_NODE:
                const char *node_name = (const char*)(ptr + sizeof(uint32_t));
                if(node_name_matches(node_name, wanted, wanted_len))
                    return (int)(ptr - struct_base);
                ptr = skip_node(ptr); // skip unwanted child node, move to next child node
                if (!ptr) return -1;
                break;
            case FDT_NOP:
                ptr += sizeof(uint32_t);
                break;
            case FDT_PROP:
                uint32_t len = read_be32(ptr + sizeof(uint32_t));
                const uint8_t *value = ptr + 3 * sizeof(uint32_t);
                ptr = align_up(value + len, 4);
                break;
            default:
                return -1;
        }
    }
}

int fdt_path_offset(const void* fdt, const char* path) {
    // TODO: Implement this function
    const struct fdt_header *hdr = fdt;
    const uint8_t *struct_base;
    int offset = 0;
    const char *part;
    
    if(!fdt || !path || path[0] != '/' || read_be32(&hdr->magic) != FDT_MAGIC)
        return -1;
    
    struct_base = (const uint8_t *)fdt + read_be32(&hdr->off_dt_struct);
    
    if(read_be32(struct_base) != FDT_BEGIN_NODE) return -1;
    if(fdt_strcmp(path, "/") == 0) return 0; // root offset equals to the struct_base addr
    
    part = path + 1;
    while(*part != '\0'){
        const char *slash = fdt_strchr(part, '/'); // next '/' pos. else NULL
        size_t part_len = slash ? (size_t)(slash - part) : fdt_strlen(part);
        if(!part_len) return -1;
        
        offset = find_child(struct_base, offset, part, part_len);
        if(offset < 0) return -1;
        
        if(!slash) // last element
            return offset;
            
        part = slash+1;
    }
    return -1;
}

const void* fdt_getprop(const void* fdt,
                        int nodeoffset,
                        const char* name,
                        int* lenp) {
    // TODO: Implement this function
    const struct fdt_header *hdr = fdt;
    const uint8_t *struct_base;
    const uint8_t *strings_base;
    const uint8_t *ptr;
    
    if(!fdt || !name || nodeoffset < 0 || read_be32(&hdr->magic) != FDT_MAGIC)
        return NULL;
        
    struct_base = (const uint8_t *)fdt + read_be32(&hdr->off_dt_struct);
    strings_base = (const uint8_t *)fdt + read_be32(&hdr->off_dt_strings);
    ptr = struct_base + nodeoffset;
    if(read_be32(ptr) != FDT_BEGIN_NODE) return NULL;
    
    ptr += sizeof(uint32_t); // skip token
    ptr += fdt_strlen((const char *)ptr) + 1; // skip node name and '\0'
    ptr = align_up(ptr, 4);
    
    while(1){
        uint32_t tag = read_be32(ptr);
        switch(tag){
            case FDT_PROP:
                const uint32_t len = read_be32(ptr + sizeof(uint32_t));
                const uint32_t nameoff = read_be32(ptr + 2*sizeof(uint32_t));
                const char *prop_name = (const char *)strings_base + nameoff;
                const void *value = ptr+ 3 * sizeof(uint32_t);
                if(!fdt_strcmp(prop_name, name)){
                    if(lenp)
                        *lenp = (int)len;
                    return value;
                }
                ptr = align_up((const uint8_t *)value + len, 4);
                break;
            case FDT_NOP:
                ptr += sizeof(uint32_t);
                break;
            default:
                return NULL;
        }
    }
}

int fdt_get_uart_base(const void *fdt,
                      const char *uart_path,
                      uintptr_t *base){
    if(!base) {
        return -1;
    }

    /* Find the node offset */
    int soc_offset = fdt_path_offset(fdt, "/soc");
    int uart_offset = fdt_path_offset(fdt, uart_path);

    if (soc_offset < 0 || uart_offset < 0) {
        return -1;
    }

    /* Get the node property */
    uint32_t address_cells;
    int len;
    
    const void* prop = fdt_getprop(fdt, soc_offset, "#address-cells", &len);
    if(!prop || len < (int)sizeof(uint32_t)) {
        return -1;
    }
    address_cells = read_be32(prop);
    
    if(address_cells == 0 || address_cells > 2) {
        return -1;
    }
    
    prop = fdt_getprop(fdt, uart_offset, "reg", &len);
    if(!prop || len < (int)(address_cells * sizeof(uint32_t))) {
        return -1;
    }
    
    uintptr_t address = 0;
    const uint8_t *reg = (const uint8_t *)prop;
    for (uint32_t i = 0; i < address_cells; i++) {
        address = (address << 32) | read_be32(reg + i * 4U); // reg is store in big endian
    }
    
    *base = (uintptr_t)address;
    
    return 0;
}

static uintptr_t read_fdt_addr(const void *prop, int len){
    const uint8_t *ptr = (const uint8_t *)prop;
    
    if(len == 4) return (uintptr_t)read_be32(ptr);
    else if(len == 8){
        uint64_t hi = read_be32(ptr);
        uint64_t lo = read_be32(ptr+4);
        
        return (uintptr_t)((hi << 32) | lo);
    }
    
    return 0;
}

int fdt_get_initrd_range(const void *fdt,
                      const char *initrd_path,
                      uint8_t **rd_start,
                      uint8_t **rd_end){
    /* Find the node offset */
    int node_offset = fdt_path_offset(fdt, initrd_path);

    if (node_offset < 0) {
        return -1;
    }

    /* Get the node property */
    uintptr_t start, end;
    int len;
    
    const void* prop = fdt_getprop(fdt, node_offset, "linux,initrd-start", &len);
    if(!prop) return -1;
    start = read_fdt_addr(prop, len);
    
    prop = fdt_getprop(fdt, node_offset, "linux,initrd-end", &len);
    if(!prop) return -1;
    end = read_fdt_addr(prop, len);
    
    if(start == 0 || end == 0 || end <= start) {
        return -1;
    }
    
    *rd_start = (uint8_t *)start;
    *rd_end = (uint8_t *)end;
    
    return 0;
}

uintptr_t get_fdt_size(const void *fdt){
    return (uintptr_t)read_be32(fdt+4);
}

int fdt_get_memory_range(const void *fdt,
                      const char *mem_path,
                      uintptr_t *mem_start,
                      uintptr_t *mem_end){
    /* Find the node offset */
    int root_offset = fdt_path_offset(fdt, "/");
    int node_offset = fdt_path_offset(fdt, mem_path);

    if(root_offset < 0 || node_offset < 0){
        return -1;
    }

    /* Get the node property */
    uint32_t address_cells, size_cells;
    int len;
    
    const void* prop = fdt_getprop(fdt, root_offset, "#address-cells", &len);
    if(!prop || len < (int)sizeof(uint32_t)) {
        return -1;
    }
    address_cells = read_be32(prop);

    prop = fdt_getprop(fdt, root_offset, "#size-cells", &len);
    if(!prop || len < (int)sizeof(uint32_t)) {
        return -1;
    }
    size_cells = read_be32(prop);
    
    if(address_cells == 0 || address_cells > 2 || size_cells == 0 || size_cells > 2) {
        return -1;
    }
    
    prop = fdt_getprop(fdt, node_offset, "reg", &len);
    if(!prop || len < (int)(address_cells * sizeof(uint32_t))) {
        return -1;
    }
    
    uintptr_t address = 0;
    const uint8_t *reg = (const uint8_t *)prop;
    for (uint32_t i = 0; i < address_cells; i++) {
        address = (address << 32) | read_be32(reg + i * 4U); // reg is store in big endian
    }
    *mem_start = address;

    address = 0;
    reg = (const uint8_t *)(prop + address_cells * 4U);
    for (uint32_t i = 0; i < address_cells; i++) {
        address = (address << 32) | read_be32(reg + i * 4U); // reg is store in big endian
    }
    *mem_end = *mem_start + address;
    
    return 0;
}
