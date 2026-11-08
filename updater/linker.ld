OUTPUT_ARCH(riscv)
OUTPUT_FORMAT(elf32-littleriscv)

ENTRY(_start)

MEMORY {
    ROM (rx)  : ORIGIN = 0xA0004000, LENGTH = 239K /* App partition at offset 16KB */
    RAM (rw)  : ORIGIN = 0x70000000, LENGTH = 32K  /* SRAM */
    TCM (rwx) : ORIGIN = 0x40010000, LENGTH = 64K  /* TCMA */
}

SECTIONS {
    .startup : {
        /* Block header to be parsed by bootloader / bootROM */
        . = ORIGIN(ROM);
        LONG(__HEADER_JUMP_OVER__)
        . = ORIGIN(ROM) + __HEADER_OFFSET_MAGIC_NUM__;
        LONG(__HEADER_MAGIC_NUM__)
        . = ORIGIN(ROM) + __HEADER_OFFSET_COPY_ADDR__;
        LONG(0x4000) /* Copy from offset 0x4000 in Flash */
        . = ORIGIN(ROM) + __HEADER_OFFSET_COPY_SIZE__;
        LONG(__HEADER_COPY_SIZE__)
        . = ORIGIN(ROM) + __HEADER_SIZE__;

        *(.startup._start)
    } > ROM

    .note.gnu.build-id : ALIGN(4) {
        KEEP(*(.note.gnu.build-id))
    } > ROM

    .text : ALIGN(4) {
        KEEP(*(.text.isr_vector))
        *(.text)
        *(.text.*)
    } > TCM AT> ROM

    .rodata : ALIGN(4) {
        *(.srodata)
        *(.srodata.*)
        *(.rodata)
        *(.rodata.*)
    } > TCM AT> ROM

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

__global_pointer$      = MIN(__DATA_START__ + 0x800,
                 MAX(__DATA_START__ + 0x800, __BSS_END__ - 0x800));
