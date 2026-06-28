#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static inline uint32_t bswap32(uint32_t x) {
    return __builtin_bswap32(x);
}

static inline uint64_t bswap64(uint64_t x) {
    return __builtin_bswap64(x);
}

static inline const void* align_up(const void* ptr, size_t align) {
    return (const void*)(((uintptr_t)ptr + align - 1) & ~(align - 1));
}

// read a 32-bit big-endian integer
static uint32_t read_be32(const void *ptr){
    uint32_t tmp;
    memcpy(&tmp, ptr, sizeof(tmp));
    return bswap32(tmp);
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
                ptr += strlen((const char *)ptr) + 1; // skip node name and '\0'
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
    size_t node_len = strlen(node_name);

    if(node_len == wanted_len && !memcmp(node_name, wanted, wanted_len))
        return 1;

    return node_len > wanted_len && node_name[wanted_len] == '@' &&
           !memchr(wanted, '@', wanted_len) &&
           !memcmp(node_name, wanted, wanted_len);
}

// find the child we wanted and return offset
static int find_child(const uint8_t *struct_base,
                      int parent_off,
                      const char *wanted,
                      size_t wanted_len){
    const uint8_t *ptr = struct_base + parent_off;
    if(read_be32(ptr) != FDT_BEGIN_NODE) return -1;
    
    ptr += sizeof(uint32_t); // skip parent token
    ptr += strlen((const char *)ptr) + 1; // skip node name and '\0'
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
    
    if(!fdt || !path || path[0] != '/' || bswap32(hdr->magic) != FDT_MAGIC)
        return -1;
    
    struct_base = (const uint8_t *)fdt + bswap32(hdr->off_dt_struct);
    
    if(read_be32(struct_base) != FDT_BEGIN_NODE) return -1;
    if(strcmp(path, "/") == 0) return 0; // root offset equals to the struct_base addr
    
    part = path + 1;
    while(*part != '\0'){
        const char *slash = strchr(part, '/'); // next '/' pos. else NULL
        size_t part_len = slash ? (size_t)(slash - part) : strlen(part);
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
    
    if(!fdt || !name || nodeoffset < 0 || bswap32(hdr->magic) != FDT_MAGIC)
        return NULL;
        
    struct_base = (const uint8_t *)fdt + bswap32(hdr->off_dt_struct);
    strings_base = (const uint8_t *)fdt + bswap32(hdr->off_dt_strings);
    ptr = struct_base + nodeoffset;
    if(read_be32(ptr) != FDT_BEGIN_NODE) return NULL;
    
    ptr += sizeof(uint32_t); // skip token
    ptr += strlen((const char *)ptr) + 1; // skip node name and '\0'
    ptr = align_up(ptr, 4);
    
    while(1){
        uint32_t tag = read_be32(ptr);
        switch(tag){
            case FDT_PROP:
                const uint32_t len = read_be32(ptr + sizeof(uint32_t));
                const uint32_t nameoff = read_be32(ptr + 2*sizeof(uint32_t));
                const char *prop_name = (const char *)strings_base + nameoff;
                const void *value = ptr+ 3 * sizeof(uint32_t);
                if(!strcmp(prop_name, name)){
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

int main() {
    /* Prepare the device tree blob */
    FILE* fp = fopen("qemu.dtb", "rb");
    if (!fp) {
        perror("fopen");
        return EXIT_FAILURE;
    }
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    void* fdt = malloc(sz);
    fseek(fp, 0, SEEK_SET);
    if (fread(fdt, 1, sz, fp) != sz) {
        fprintf(stderr, "Failed to read the device tree blob\n");
        free(fdt);
        fclose(fp);
        return EXIT_FAILURE;
    }
    fclose(fp);

    /* Find the node offset */
    int offset = fdt_path_offset(fdt, "/cpus/cpu@0/interrupt-controller");
    if (offset < 0) {
        fprintf(stderr, "fdt_path_offset\n");
        free(fdt);
        return EXIT_FAILURE;
    }

    /* Get the node property */
    int len;
    const void* prop = fdt_getprop(fdt, offset, "compatible", &len);
    if (!prop) {
        fprintf(stderr, "fdt_getprop\n");
        free(fdt);
        return EXIT_FAILURE;
    }
    printf("compatible: %.*s\n", len, (const char*)prop);

    offset = fdt_path_offset(fdt, "/memory");
    prop = fdt_getprop(fdt, offset, "reg", &len);
    const uint64_t* reg = (const uint64_t*)prop;
    printf("memory: base=0x%lx size=0x%lx\n", bswap64(reg[0]), bswap64(reg[1]));

    offset = fdt_path_offset(fdt, "/chosen");
    prop = fdt_getprop(fdt, offset, "linux,initrd-start", &len);
    const uint64_t* initrd_start = (const uint64_t*)prop;
    printf("initrd-start: 0x%lx\n", bswap64(initrd_start[0]));

    free(fdt);
    return 0;
}
