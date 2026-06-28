CROSS_COMPILE ?= riscv64-unknown-elf-

CC      := $(CROSS_COMPILE)gcc
LD      := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy

LDFLAGS := -T link.ld

OBJS := start.o main.o uart.o sbi_call.o

.PHONY: all clean

all: kernel.fit

start.o: start.S
	$(CC) -c $< -o $@

main.o: main.c
	$(CC) -c $< -o $@

uart.o: uart.c
	$(CC) -c $< -o $@

sbi_call.o: sbi_call.c
	$(CC) -c $< -o $@

kernel.elf: $(OBJS) link.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

kernel.bin: kernel.elf
	$(OBJCOPY) -O binary $< $@

kernel.fit: kernel.bin kernel.its x1_orangepi-rv2.dtb
	mkimage -f kernel.its $@

clean:
	rm -f *.o *.d kernel.elf kernel.bin kernel.fit
