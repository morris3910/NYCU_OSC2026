OUTPUT_ARCH(riscv)
ENTRY(_start)
SECTIONS
{
    . = 0x00200000; /* for orange pi */
    /*. = 0x80200000;  for QEMU */
    _start = .;
    __image_start = .;
    .text : { *(.text.boot) *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : {
    	. = ALIGN(8);
        __bss_start = .;
        *(.sbss) *(.bss) *(COMMON)
        . = ALIGN(8);
        __bss_stop = .;
    }
    . = ALIGN(16);
    . = . + 0x4000;
    _end = .;
    __image_end = .;
}
