OUTPUT_ARCH(riscv)
OUTPUT_FORMAT(elf32-littleriscv)

ENTRY(_start)

MEMORY {
    ROM (rx) : ORIGIN = 0xA0000000, LENGTH = 256K /* Region: EFLASH; Offset: 0x00 */
    RAM (rw) : ORIGIN = 0x70000000, LENGTH = 32K /* Region: SRAM; Offset: 0x00 */
    TCM (rwx) : ORIGIN = 0x40010000, LENGTH = 64K /* Region: TCMA; Offset: 0x00 */
}

SECTIONS {
    .startup : {
        /* Block header to be parsed by the bootROM */
        . = ORIGIN(ROM);
        LONG(__HEADER_JUMP_OVER__) /* Jump over the header structure */
        . = ORIGIN(ROM) + __HEADER_OFFSET_MAGIC_NUM__;
        LONG(__HEADER_MAGIC_NUM__)
        . = ORIGIN(ROM) + __HEADER_OFFSET_COPY_ADDR__;
        LONG(0x00) /* Copy data from the begining of memory region */
        . = ORIGIN(ROM) + __HEADER_OFFSET_COPY_SIZE__;
        LONG(__HEADER_COPY_SIZE__)
        . = ORIGIN(ROM) + __HEADER_SIZE__;
        /* End of the block header */

        *(.startup._start)  /* Reset vector */
    } > ROM

    .note.gnu.build-id : ALIGN(4) {
        KEEP(*(.note.gnu.build-id))
    } > ROM

    .text : ALIGN(4) {
        KEEP(*(.text.isr_vector))
        *(.text)
        *(.text.*)
    } > TCM AT> ROM

    /* .rodata section must be located right before .data section */
    .rodata : ALIGN(4) {
        *(.srodata)
        *(.srodata.*)
        *(.rodata)
        *(.rodata.*)
    } > TCM AT> ROM

    /* .data section must be located right after .rodata section */
	.data : ALIGN(4) {
        *(.data)
        *(.data.*)
        *(.sdata)
        *(.sdata.*)
    } > RAM AT> ROM

    .bss (NOLOAD) : ALIGN(4) {
        *(.sbss)
        *(.sbss.*)
        *(.bss)
        *(.bss.*)
        *(COMMON)
    } > RAM

    .heap (NOLOAD) : ALIGN(4) {
        _heap_start = .;
    } > RAM
}

/* Variables for the header to be parsed by the bootROM */
__HEADER_OFFSET_MAGIC_NUM__ = 0x20;
__HEADER_OFFSET_COPY_ADDR__ = 0x28;
__HEADER_OFFSET_COPY_SIZE__ = 0x2C;
__HEADER_SIZE__             = 0x30;

__HEADER_JUMP_OVER__        = 0x0300006F; /* jal x0, 48 */
__HEADER_MAGIC_NUM__        = 0x0BEF5262;
__HEADER_COPY_SIZE__        = LOADADDR(.data) + SIZEOF(.data) - LOADADDR(.startup);

/* Variables for startup code */
__TEXT_START__              = ADDR(.text);
__TEXT_END__                = ADDR(.text) + SIZEOF(.text);
__TEXT_IMAGE_START__        = LOADADDR(.text);
__RODATA_START__            = ADDR(.rodata);
__RODATA_END__              = ADDR(.rodata) + SIZEOF(.rodata);
__RODATA_IMAGE_START__      = LOADADDR(.rodata);
__DATA_START__              = ADDR(.data);
__DATA_END__                = ADDR(.data) + SIZEOF(.data);
__DATA_IMAGE_START__        = LOADADDR(.data);
__BSS_START__               = ADDR(.bss);
__BSS_END__                 = ADDR(.bss) + SIZEOF(.bss);
__SP                        = ORIGIN(RAM) + LENGTH(RAM);

/* "__global_pointer$" is a reserved GNU LD linker symbol.
   It serves as a special marker that indicates where
   the global pointer value should be placed in memory. */
__global_pointer$      = MIN(__DATA_START__ + 0x800,
                 MAX(__DATA_START__ + 0x800, __BSS_END__ - 0x800));
