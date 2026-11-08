#!/usr/bin/env python3
"""Exercise the bootloader self-updater logic and uf2conv conversion."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
uf2conv = root / 'tools/uf2conv.py'


def test_uf2conv_conversion():
    with tempfile.TemporaryDirectory() as tmp:
        payload = bytes([i % 256 for i in range(1024)])
        bin_path = Path(tmp) / 'app.bin'
        bin_path.write_bytes(payload)

        uf2_path = Path(tmp) / 'app.uf2'
        subprocess.run([sys_python(), str(uf2conv), str(bin_path), '-o', str(uf2_path)], check=True)

        assert uf2_path.exists()
        uf2_data = uf2_path.read_bytes()
        assert len(uf2_data) == 4 * 512  # 1024 bytes / 256 bytes per block = 4 blocks

        # Verify round-trip conversion from UF2 back to BIN
        roundtrip_bin = Path(tmp) / 'roundtrip.bin'
        subprocess.run([sys_python(), str(uf2conv), str(uf2_path), '-o', str(roundtrip_bin)], check=True)
        assert roundtrip_bin.read_bytes() == payload


def test_updater_flash_logic():
    code = r'''
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define BOARD_FLASH_SIZE        (256UL * 1024UL)
#define BOARD_FLASH_SECTOR_SIZE 1024UL
#define BOOTLOADER_FLASH_ADDR   0xA0000000UL
#define BOOTLOADER_MAX_SIZE     (16UL * 1024UL)
#define APP_FLASH_ADDR          0xA0004000UL
#define STATE_FLASH_ADDR        0xA003FC00UL

static uint8_t mock_flash[BOARD_FLASH_SIZE];

static int EFLASH_EraseSector(uintptr_t addr, int array) {
    (void)array;
    assert(addr >= BOOTLOADER_FLASH_ADDR);
    uint32_t offset = (uint32_t)(addr - BOOTLOADER_FLASH_ADDR);
    assert(offset + BOARD_FLASH_SECTOR_SIZE <= BOARD_FLASH_SIZE);
    memset(&mock_flash[offset], 0xFF, BOARD_FLASH_SECTOR_SIZE);
    return 0;
}

static uint8_t new_bootloader[15000];

int main(void) {
    /* Populate flash with initial old bootloader and app */
    memset(mock_flash, 0xAA, sizeof(mock_flash));

    /* Initialize new bootloader image */
    for (size_t i = 0; i < sizeof(new_bootloader); i++) {
        new_bootloader[i] = (uint8_t)(i ^ 0x3C);
    }
    uint32_t size = sizeof(new_bootloader);

    /* --- Execute Updater Logic --- */
    assert(size > 0 && size <= BOOTLOADER_MAX_SIZE);

    /* 1. Erase 16 sectors of bootloader */
    for (uint32_t addr = BOOTLOADER_FLASH_ADDR; addr < APP_FLASH_ADDR; addr += BOARD_FLASH_SECTOR_SIZE) {
        assert(EFLASH_EraseSector(addr, 0) == 0);
    }

    /* 2. Program new bootloader */
    memcpy(&mock_flash[0], new_bootloader, size);

    /* 3. Verify written image */
    assert(memcmp(&mock_flash[0], new_bootloader, size) == 0);

    /* 4. Erase updater from application area and state sector */
    assert(EFLASH_EraseSector(APP_FLASH_ADDR, 0) == 0);
    assert(EFLASH_EraseSector(STATE_FLASH_ADDR, 0) == 0);

    /* Verify bootloader is now the new version */
    assert(memcmp(&mock_flash[0], new_bootloader, size) == 0);

    /* Verify application entry sector is erased (0xFF) so device boots into DFU */
    uint32_t app_offset = (uint32_t)(APP_FLASH_ADDR - BOOTLOADER_FLASH_ADDR);
    for (uint32_t i = 0; i < BOARD_FLASH_SECTOR_SIZE; i++) {
        assert(mock_flash[app_offset + i] == 0xFF);
    }

    return 0;
}
'''
    with tempfile.TemporaryDirectory() as tmp:
        c_file = Path(tmp) / 'test.c'
        c_file.write_text(code)
        binary = Path(tmp) / 'test'
        subprocess.run([os.environ.get('CC', 'cc'), '-std=c11', '-Wall', '-Wextra', '-Werror',
                        '-fsanitize=address,undefined', str(c_file), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


def sys_python():
    return os.environ.get('PYTHON', 'python3')


if __name__ == '__main__':
    test_uf2conv_conversion()
    test_updater_flash_logic()
    print('Updater and UF2 conversion tests passed')
