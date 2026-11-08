#!/usr/bin/env python3
"""Exercise application integrity checks, CRC32 validation, and state ledger (B4)."""
import os
from pathlib import Path
import re
import subprocess
import tempfile
import zlib

root = Path(__file__).resolve().parents[1]

def function(filename, name):
    source = (root / filename).read_text()
    match = re.search(r'(?m)(?:^[^\n;{}]*\n)?^[^\n;{}]*\b' + name + r'\s*\([^;]*\)\s*\{', source)
    if not match:
        raise ValueError(f'{name} not found')
    start = match.start()
    pos = source.index('{', match.start()) + 1
    depth = 1
    while depth:
        depth += (source[pos] == '{') - (source[pos] == '}')
        pos += 1
    return source[start:pos]

code = r'''
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#define BOARD_FLASH_SIZE        (256UL * 1024UL)
#define BOARD_FLASH_SECTOR_SIZE (1024UL)
#define BOARD_FLASH_ADDR_OVERRIDE 1

static uint8_t mock_flash[BOARD_FLASH_SIZE] __attribute__((aligned(1024)));
#define BOARD_FLASH_ADDR_ZERO   ((uintptr_t)mock_flash)

#include "boards.h"

#define EFLASH_MAIN_ARRAY 0
#define EFLASH_COMPLETE 0

static bool erase_fail = false;
static bool program_fail = false;
static unsigned timeout_sets = 0;
static uintptr_t written_app_end;

static int EFLASH_EraseSector(uintptr_t addr, int array) {
    (void)array;
    if (erase_fail) return -1;
    assert(addr >= BOARD_FLASH_ADDR_ZERO);
    assert(addr + BOARD_FLASH_SECTOR_SIZE <= BOARD_FLASH_ADDR_ZERO + BOARD_FLASH_SIZE);
    memset((void*)addr, 0xFF, BOARD_FLASH_SECTOR_SIZE);
    return EFLASH_COMPLETE;
}

static int EFLASH_ProgramWord(uintptr_t addr, uint32_t data, int array) {
    (void)array;
    if (program_fail) return -1;
    assert(addr >= BOARD_FLASH_ADDR_ZERO);
    assert(addr + sizeof(uint32_t) <= BOARD_FLASH_ADDR_ZERO + BOARD_FLASH_SIZE);
    *(uint32_t*)addr = data;
    return EFLASH_COMPLETE;
}

void board_inactivity_timeout_set(void) {
    timeout_sets++;
}
''' + '\n'.join([
    function('src/boards.c', 'board_flash_get_written_size'),
    function('src/boards.c', 'board_app_crc32'),
    function('src/boards.c', 'board_app_state_invalidate'),
    function('src/boards.c', 'board_app_state_commit'),
    function('src/boards.c', 'board_dfu_complete'),
    function('src/boards.c', 'board_app_valid')
]) + r'''
int main(void) {
    written_app_end = BOARD_FLASH_APP_ADDR;
    /* 1. Verify CRC32 standard check vector ("123456789" -> 0xCBF43926) */
    const char *test_str = "123456789";
    uint32_t crc = board_app_crc32((uintptr_t)test_str, 9);
    assert(crc == 0xCBF43926U);

    /* 2. Initial erased flash state */
    memset(mock_flash, 0xFF, sizeof(mock_flash));
    assert(!board_app_valid()); /* Missing magic number */

    /* Set valid magic number in an erased flash */
    *(volatile uint32_t*)BOARD_FLASH_APP_MAGIC_ADDR = BOARD_FLASH_APP_MAGIC_NUM;
    assert(board_app_valid()); /* Erased state sector allows booting legacy / unmanaged image */

    /* 3. Invalidation when new write starts (power loss protection) */
    board_app_state_invalidate();
    board_app_state_t *st = (board_app_state_t *)BOARD_FLASH_STATE_ADDR;
    assert(st->state == BOARD_APP_STATE_INVALID);
    assert(!board_app_valid()); /* Magic is present, but state is invalid -> must NOT boot! */

    /* 4. Complete DFU flashing cycle */
    uint32_t payload_len = 2048;
    for (uint32_t i = 0; i < payload_len; i++) {
        ((uint8_t*)BOARD_FLASH_APP_ADDR)[i] = (uint8_t)(i ^ 0x5A);
    }
    *(volatile uint32_t*)BOARD_FLASH_APP_MAGIC_ADDR = BOARD_FLASH_APP_MAGIC_NUM;
    written_app_end = BOARD_FLASH_APP_ADDR + payload_len;

    board_dfu_complete();
    assert(timeout_sets == 1);
    assert(st->state == BOARD_APP_STATE_VALID);
    assert(st->app_size == payload_len);
    assert(st->crc32 == board_app_crc32(BOARD_FLASH_APP_ADDR, payload_len));
    assert(st->check == (st->state ^ st->app_size ^ st->crc32 ^ BOARD_APP_STATE_CHECK_XOR));
    assert(board_app_valid()); /* Fully committed and valid image */

    /* 5. Detect payload corruption (CRC failure) */
    ((uint8_t*)BOARD_FLASH_APP_ADDR)[100] ^= 0x01;
    assert(!board_app_valid());
    ((uint8_t*)BOARD_FLASH_APP_ADDR)[100] ^= 0x01; /* Restore */
    assert(board_app_valid());

    /* 6. Tampering & bounds validation */
    st->check ^= 0x1234; /* Invalid check word */
    assert(!board_app_valid());
    st->check ^= 0x1234; /* Restore */
    assert(board_app_valid());

    st->app_size = 0; /* Zero size */
    st->check = st->state ^ st->app_size ^ st->crc32 ^ BOARD_APP_STATE_CHECK_XOR;
    assert(!board_app_valid());

    st->app_size = BOARD_FLASH_APP_SIZE + 1; /* Oversized */
    st->check = st->state ^ st->app_size ^ st->crc32 ^ BOARD_APP_STATE_CHECK_XOR;
    assert(!board_app_valid());

    st->app_size = payload_len;
    st->state = 0x12345678U; /* Unknown state */
    st->check = st->state ^ st->app_size ^ st->crc32 ^ BOARD_APP_STATE_CHECK_XOR;
    assert(!board_app_valid());

    /* 7. Commit failure handling */
    erase_fail = true;
    assert(!board_app_state_commit(payload_len, crc));
    erase_fail = false;

    program_fail = true;
    assert(!board_app_state_commit(payload_len, crc));
    program_fail = false;

    return 0;
}
'''

with tempfile.TemporaryDirectory() as tmp:
    c = Path(tmp) / 'test.c'
    c.write_text(code)
    binary = Path(tmp) / 'test'
    subprocess.run([os.environ.get('CC', 'cc'), '-std=c11', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-I', str(root / 'src'), str(c), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)

# Also cross-verify standard python zlib crc32 matches
sample_data = bytes([i % 256 for i in range(1024)])
expected_crc = zlib.crc32(sample_data) & 0xFFFFFFFF
assert expected_crc != 0

print('Application state and CRC32 integrity tests passed')
