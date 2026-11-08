/*
 * Embed the compiled bootloader binary into the updater.
 */
    .section .rodata
    .global bootloader_bin
    .global bootloader_bin_len
    .balign 4

bootloader_bin:
    .incbin "output/release/baikal-uf2-bootloader.bin"
bootloader_bin_end:

    .balign 4
bootloader_bin_len:
    .word bootloader_bin_end - bootloader_bin
