#!/usr/bin/env python3
"""Exercise actual MSC and UF2 write functions with a simulated flash backend."""
import os
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]

def function(filename, name):
    source = (root / filename).read_text()
    match = re.search(r'(?m)^\s*(?:(?:int32_t|int|void|bool)\s+)?' + name + r'\s*\([^;]*\)\s*\{', source)
    start = source.rfind('\n', 0, match.start()) + 1
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
#include "boards.h"
#define MSC_STORAGE_BLOCK_SIZE 512U
#define BPB_SECTOR_SIZE 512
#define TUD_MSC_RET_ERROR -1
#define MAX_BLOCKS 2048
#define UF2_MAGIC_START0 0x0a324655U
#define UF2_MAGIC_START1 0x9e5d5157U
#define UF2_MAGIC_END 0x0ab16f30U
#define UF2_FLAG_FAMILYID 0x2000U
#define UF2_FLAG_NOFLASH 1U
#define STATE_WRITING_STARTED 1
#define STATE_WRITING_FINISHED 2
#define TUF2_LOG1(...) ((void)0)
typedef struct { uint32_t magicStart0, magicStart1, flags, targetAddr, payloadSize,
  blockNo, numBlocks, familyID; uint8_t data[476]; uint32_t magicEnd; } UF2_Block;
typedef struct { uint32_t numBlocks, numWritten; bool aborted;
  uint8_t writtenMask[MAX_BLOCKS/8]; } WriteState;
static bool first_write = true;
static WriteState write_state;
static unsigned erases, writes, resets, completions, invalidations;
static bool erase_fail, write_fail;
static void indicator_set(int state) { (void)state; }
bool board_inactivity_timeout_is_started(void) { return false; }
static void board_dfu_complete(void) { completions++; }
void board_flash_erase_reset(void) { resets++; }
void board_app_state_invalidate(void) { invalidations++; }
bool board_flash_erase_overlapping_sectors(void *p, uint32_t n) {
  (void)p; (void)n; erases++; return !erase_fail;
}
static bool board_flash_write(uint32_t addr, const void *p, uint32_t n) {
  (void)addr; (void)p; (void)n; writes++; return !write_fail;
}
static void board_flash_flush(void) {}
static bool is_uf2_block(UF2_Block *b) {
  return b->magicStart0 == UF2_MAGIC_START0 && b->magicStart1 == UF2_MAGIC_START1 && b->magicEnd == UF2_MAGIC_END;
}
''' + function('src/boards.c', 'bl_flash_range_ok') + '\n' + function('src/ghostfat.c', 'uf2_write_block') + '\n' + function('src/msc_cb.c', 'tud_msc_write10_cb') + '\n' + function('src/msc_cb.c', 'tud_msc_write10_complete_cb') + r'''
static UF2_Block block(void) {
  UF2_Block b = {0};
  b.magicStart0 = UF2_MAGIC_START0; b.magicStart1 = UF2_MAGIC_START1;
  b.magicEnd = UF2_MAGIC_END; b.flags = UF2_FLAG_FAMILYID;
  b.targetAddr = BOARD_FLASH_APP_ADDR; b.payloadSize = 256;
  b.numBlocks = 2; b.familyID = BOARD_UF2_FAMILY_ID;
  return b;
}
static void rejected(UF2_Block *b) {
  unsigned e = erases, w = writes;
  (void)tud_msc_write10_cb(0, 0, 0, (uint8_t *)b, sizeof(*b));
  assert(e == erases && w == writes);
}
int main(void) {
  UF2_Block b = block();
  assert(sizeof(b) == 512);
  b.targetAddr = BOARD_FLASH_BOOT_ADDR; rejected(&b);
  b = block(); b.targetAddr = BOARD_FLASH_ADDR_ZERO + BOARD_FLASH_SIZE - 4; rejected(&b);
  b = block(); b.payloadSize = UINT32_MAX; rejected(&b);
  b = block(); b.payloadSize = 0; rejected(&b);
  b = block(); b.familyID++; rejected(&b);
  b = block(); b.flags |= UF2_FLAG_NOFLASH; rejected(&b);
  b = block(); b.flags = 0; rejected(&b);
  b = block(); b.blockNo = 2; rejected(&b);
  b = block(); b.numBlocks = 0; rejected(&b);
  b = block(); b.magicEnd = 0; rejected(&b);
  b = block(); assert(tud_msc_write10_cb(0, 0, 0, (uint8_t *)&b, 12) == -1);
  assert(erases == 0 && writes == 0);
  assert(tud_msc_write10_cb(0, 0, 0, (uint8_t *)&b, 512) == 512);
  assert(erases == 1 && writes == 1 && resets == 1 && invalidations == 1 && write_state.numWritten == 1);
  assert(tud_msc_write10_cb(0, 0, 0, (uint8_t *)&b, 512) == 512);
  assert(write_state.numWritten == 1);
  b.blockNo = 1; b.targetAddr += 256;
  assert(tud_msc_write10_cb(0, 0, 0, (uint8_t *)&b, 512) == 512);
  tud_msc_write10_complete_cb(0);
  assert(completions == 1);
  first_write = true; b = block(); erase_fail = true;
  assert(tud_msc_write10_cb(0, 0, 0, (uint8_t *)&b, 512) == -1);
  assert(write_state.aborted); tud_msc_write10_complete_cb(0); assert(first_write);
  erase_fail = false; write_fail = true;
  assert(tud_msc_write10_cb(0, 0, 0, (uint8_t *)&b, 512) == -1);
  assert(write_state.aborted); tud_msc_write10_complete_cb(0); assert(first_write);
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
print('UF2 and MSC write tests passed')
