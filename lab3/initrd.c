#include "initrd.h"

static const uint8_t *initrd_start;
static const uint8_t *initrd_end;

int initrd_init(uint8_t *rd_start, uint8_t *rd_end){
    initrd_start = (const uint8_t *)rd_start;
    initrd_end = (const uint8_t *)rd_end;
    return 0;
}

struct cpio_t {
    char magic[6];
    char ino[8];
    char mode[8];
    char uid[8];
    char gid[8];
    char nlink[8];
    char mtime[8];
    char filesize[8];
    char devmajor[8];
    char devminor[8];
    char rdevmajor[8];
    char rdevminor[8];
    char namesize[8];
    char check[8];
};

/**
 * @brief Convert a hexadecimal string to integer
 *
 * @param s hexadecimal string
 * @param n length of the string
 * @return integer value
 */
static int hextoi(const char* s, int n) {
    int r = 0;
    while (n-- > 0) {
        char c = *s++;
        r <<= 4;

        if (c >= '0' && c <= '9') {
            r += c - '0';
        } else if (c >= 'A' && c <= 'F') {
            r += c - 'A' + 10;
        } else if (c >= 'a' && c <= 'f') {
            r += c - 'a' + 10;
        } else {
            return 0;
        }
    }
    return r;
}

/**
 * @brief Align a number to the nearest multiple of a given number
 *
 * @param n number
 * @param byte alignment
 * @return aligned number
 */
static int align(int n, int byte) {
    return (n + byte - 1) & ~(byte - 1);
}

static int initrd_memcmp(const void *s1, const void *s2, size_t n){
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

static int initrd_strcmp(const char *s1, const char *s2){
    while(*s1 && *s1 == *s2){
        s1++;
        s2++;
    }

    return (unsigned char)*s1 - (unsigned char)*s2;
}

void initrd_list() {
    // TODO: Implement this function
    int offset = 0;
    if(!initrd_start || !initrd_end) {
        uart_puts("inited: not found.\n");
        return;
    }
    const uint8_t *base = initrd_start;

    while(1){
        if (base + offset + sizeof(struct cpio_t) > initrd_end) {
            uart_puts("initrd: corrupted archive\n");
            return;
        }
    
        const struct cpio_t *hdr = (const struct cpio_t *)(base + offset);
        if(initrd_memcmp(hdr->magic, CPIO_MAGIC, 6)) return;

        int name_size = hextoi(hdr->namesize, 8);
        int file_size = hextoi(hdr->filesize, 8);
        
        int name_off = offset + sizeof(struct cpio_t);
        int file_off = align(name_off + name_size, 4);
        int next_off = align(file_off + file_size, 4);
        
        const char *filename = (const char *)(base + name_off);
        if(!initrd_strcmp(filename, CPIO_END)) return;
        
        uart_putd(file_size);
        uart_puts(" ");
        uart_puts(filename);
        uart_puts("\n");

        offset = next_off;
    }
    return;
}

void initrd_cat(const char* filename) {
    // TODO: Implement this function
    int offset = 0;
    if(!initrd_start || !initrd_end) {
        uart_puts("inited: not found.\n");
        return;
    }
    const uint8_t *base = initrd_start;

    while(1){
        if (base + offset + sizeof(struct cpio_t) > initrd_end) {
            uart_puts("initrd: corrupted archive\n");
            return;
        }
        
        const struct cpio_t *hdr = (const struct cpio_t *)(base + offset);
        if(initrd_memcmp(hdr->magic, CPIO_MAGIC, 6)) return;

        int name_size = hextoi(hdr->namesize, 8);
        int file_size = hextoi(hdr->filesize, 8);
        int name_off = offset + sizeof(struct cpio_t);
        int file_off = align(name_off + name_size, 4);
        int next_off = align(file_off + file_size, 4);
        
        const char *file_name = (const char *)(base + name_off);
        if(!initrd_strcmp(file_name, CPIO_END)){
            uart_puts("cat: ");
            uart_puts(filename);
            uart_puts(": No such file\n");
            return;
        }
        if(!initrd_strcmp(file_name, filename) || (file_name[0] == '.' && file_name[1] == '/' && !initrd_strcmp(file_name+2, filename))){ // +2 -> ./hello.txt
            const char *file_data = (const char *)(base + file_off);
            for (int i = 0; i < file_size; i++) {
                uart_putc(file_data[i]); 
            }
            uart_puts("\n");
            return;
        }

        offset = next_off;
    }
    return;
}
