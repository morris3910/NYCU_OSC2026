#ifndef INITRD_H
#define INITRD_H

#include "uart.h"
#include <stdint.h>
#include <stddef.h>

#define CPIO_MAGIC "070701"
#define CPIO_END "TRAILER!!!"

int initrd_init(uint8_t *rd_start, uint8_t *rd_end);
void initrd_list();
void initrd_cat(const char* filename);

#endif
