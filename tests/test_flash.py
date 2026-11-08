#!/usr/bin/env python3
"""Exercise the production flash bounds and erase logic with a simulated HAL."""
import os
import re
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/boards.c').read_text()


def function(name):
    start = re.search(r'(?m)^\s*(?:(?:bool|void)\s+)?' + name + r'\s*\([^;]*\)\s*\{', source).start()
    start = source.rfind('\n', 0, start) + 1
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


code = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "boards.h"
#define BOARD_FLASH_SECTOR_NUM (BOARD_FLASH_SIZE / BOARD_FLASH_SECTOR_SIZE)
#define EFLASH_MAIN_ARRAY 0
#define EFLASH_COMPLETE 0
#define TUF2_LOG1(...) ((void)0)
static bool flash_sectors_erased[BOARD_FLASH_SECTOR_NUM];
static uintptr_t written_app_end;
static unsigned calls, fail;
static int EFLASH_EraseSector(uintptr_t addr, int array) {
  (void)array;
  assert(addr >= BOARD_FLASH_APP_ADDR);
  assert(addr < BOARD_FLASH_ADDR_ZERO + BOARD_FLASH_SIZE);
  ++calls;
  return fail ? 1 : EFLASH_COMPLETE;
}
''' + function('bl_flash_range_ok') + '\n' + function('board_flash_erase_reset') + '\n' + function('board_flash_erase_overlapping_sectors') + '\n' + function('board_flash_erase_app') + r'''
int main(void) {
  const uintptr_t start = BOARD_FLASH_APP_ADDR;
  const uintptr_t end = BOARD_FLASH_APP_ADDR + BOARD_FLASH_APP_SIZE;
  assert(bl_flash_range_ok(start, 4));
  assert(bl_flash_range_ok(end - 4, 4));
  assert(!bl_flash_range_ok(start - 4, 4));
  assert(!bl_flash_range_ok(end, 4));
  assert(!bl_flash_range_ok(end - 4, 8));
  assert(!bl_flash_range_ok(start, 0));
  assert(!bl_flash_range_ok(start + 1, 4));
  assert(!bl_flash_range_ok(start, 3));
  assert(!bl_flash_range_ok(start, UINT32_MAX));
  assert(!board_flash_erase_overlapping_sectors((void *)(start - 4), 4));
  assert(calls == 0);
  assert(board_flash_erase_overlapping_sectors((void *)start, 4));
  assert(calls == 1);
  assert(board_flash_erase_overlapping_sectors((void *)start, 4));
  assert(calls == 1);
  board_flash_erase_reset();
  assert(board_flash_erase_overlapping_sectors((void *)start, 4));
  assert(calls == 2);
  board_flash_erase_reset();
  fail = 1;
  assert(!board_flash_erase_overlapping_sectors((void *)start, 4));
  assert(calls == 1002);
  fail = 0;
  assert(board_flash_erase_overlapping_sectors((void *)start, 4));
  assert(calls == 1003);
  board_flash_erase_reset();
  calls = 0;
  board_flash_erase_app();
  assert(calls == BOARD_FLASH_APP_SIZE / BOARD_FLASH_SECTOR_SIZE + 1);
  assert(board_flash_erase_overlapping_sectors((void *)start, 4));
  assert(calls == BOARD_FLASH_APP_SIZE / BOARD_FLASH_SECTOR_SIZE + 2);
  calls = 0;
  fail = 1;
  board_flash_erase_app();
  assert(calls == 1001);
  fail = 0;
  assert(board_flash_erase_overlapping_sectors((void *)start, 4));
  assert(calls == 1002);
  return 0;
}
'''

with tempfile.TemporaryDirectory() as temp:
    test_c = Path(temp) / 'test.c'
    test_c.write_text(code)
    binary = Path(temp) / 'test'
    subprocess.run([os.environ.get('CC', 'cc'), '-std=c11', '-Wall', '-Wextra',
                    '-Werror', '-fsanitize=address,undefined', '-I', str(root / 'src'),
                    str(test_c), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('flash bounds and erase tests passed')
