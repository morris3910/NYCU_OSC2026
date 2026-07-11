CROSS_COMPILE ?= riscv64-unknown-elf-

CC      := $(CROSS_COMPILE)gcc
LD      := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy

CFLAGS := -march=rv64imac_zicsr_zifencei -mabi=lp64 -fno-pie -mcmodel=medany -msmall-data-limit=0
UBOOT_LDFLAGS := -T Uboot_link.ld
MYBOOT_LDFLAGS := -T Myboot_link.ld

UBOOT_OBJS := start.o Uboot_main.o uart.o sbi_call.o load_image.o fdt.o relocate.o
MYBOOT_OBJS := Mystart.o Myboot_main.o uart.o sbi_call.o fdt.o initrd.o

.PHONY: all clean

all: kernel.fit Mykernel.bin

start.o: start.S
	$(CC) $(CFLAGS) -c $< -o $@
	
Mystart.o: Mystart.S
	$(CC) $(CFLAGS) -c $< -o $@

load_image.o: load_image.c
	$(CC) $(CFLAGS) -c $< -o $@
	
fdt.o: fdt.c
	$(CC) $(CFLAGS) -c $< -o $@
	
initrd.o: initrd.c
	$(CC) $(CFLAGS) -c $< -o $@
	
relocate.o: relocate.c
	$(CC) $(CFLAGS) -c $< -o $@

Uboot_main.o: Uboot_main.c
	$(CC) $(CFLAGS) -c $< -o $@
	
Myboot_main.o: Myboot_main.c
	$(CC) $(CFLAGS) -c $< -o $@

uart.o: uart.c
	$(CC) $(CFLAGS) -c $< -o $@

sbi_call.o: sbi_call.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel.elf: $(UBOOT_OBJS) Uboot_link.ld
	$(LD) $(UBOOT_LDFLAGS) -o $@ $(UBOOT_OBJS)

Mykernel.elf: $(MYBOOT_OBJS) Myboot_link.ld
	$(LD) $(MYBOOT_LDFLAGS) -o $@ $(MYBOOT_OBJS)

kernel.bin: kernel.elf
	$(OBJCOPY) -O binary $< $@

Mykernel.bin: Mykernel.elf
	$(OBJCOPY) -O binary $< $@

kernel.fit: kernel.bin kernel.its x1_orangepi-rv2.dtb
	mkimage -f kernel.its $@

clean:
	rm -f *.o *.d *.fit *.elf *.bin
