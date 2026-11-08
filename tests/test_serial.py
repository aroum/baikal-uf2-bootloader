#!/usr/bin/env python3
"""Exercise USB serial number handling and descriptor generation."""
import os
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]

def function(filename, name):
    source = (root / filename).read_text()
    match = re.search(r'(?m)(?:^[^\n;{}]*\n)?^[^\n;{}]*\b' + name + r'\s*\([^;]*\)\s*\{', source)
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

#define CFG_TUD_ENDPOINT0_SIZE 64
#define USB_VID 0x6666
#define USB_PID 0x4002
#define STRID_LANGID 0
#define STRID_MANUFACTURER 1
#define STRID_PRODUCT 2
#define STRID_SERIAL 3
#define STRID_MSC 4
#define TUSB_DESC_DEVICE 1
#define TUSB_DESC_STRING 3
#define TU_ATTR_ALIGNED(n) __attribute__((aligned(n)))

typedef struct {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t bcdUSB;
  uint8_t bDeviceClass;
  uint8_t bDeviceSubClass;
  uint8_t bDeviceProtocol;
  uint8_t bMaxPacketSize0;
  uint16_t idVendor;
  uint16_t idProduct;
  uint16_t bcdDevice;
  uint8_t iManufacturer;
  uint8_t iProduct;
  uint8_t iSerialNumber;
  uint8_t bNumConfigurations;
} tusb_desc_device_t;

tusb_desc_device_t desc_device = {
  .bLength = sizeof(tusb_desc_device_t),
  .bDescriptorType = TUSB_DESC_DEVICE,
  .bcdUSB = 0x0200,
  .bDeviceClass = 0x00,
  .bDeviceSubClass = 0x00,
  .bDeviceProtocol = 0x00,
  .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
  .idVendor = USB_VID,
  .idProduct = USB_PID,
  .bcdDevice = 0x0100,
  .iManufacturer = STRID_MANUFACTURER,
  .iProduct = STRID_PRODUCT,
  .iSerialNumber = 0,
  .bNumConfigurations = 0x01
};

static uint16_t _desc_str[32 + 1];
char const *string_desc_arr[] = {
  (const char[]) { 0x09, 0x04 },
  "Baikal",
  "BE-U1000",
  NULL,
  "UF2"
};

''' + function('src/boards.c', 'board_usb_get_serial') + '\n' + function('src/usb_descriptors.c', 'tud_descriptor_device_cb') + '\n' + function('src/usb_descriptors.c', 'tud_descriptor_string_cb') + r'''
int main(void) {
  // Test 1: board_usb_get_serial with misaligned pointer
  uint8_t raw_buffer[32];
  uint8_t *unaligned_buf = &raw_buffer[1]; // Intentionally unaligned
  uint8_t len = board_usb_get_serial(unaligned_buf);
  assert(len == 0);

  // Test 2: Device descriptor sets iSerialNumber to 0 when serial is empty
  const tusb_desc_device_t *desc = (const tusb_desc_device_t *)tud_descriptor_device_cb();
  assert(desc->iSerialNumber == 0);

  // Test 3: String descriptor returns NULL for STRID_SERIAL when serial is empty
  assert(tud_descriptor_string_cb(STRID_SERIAL, 0x0409) == NULL);

  // Test 4: Other string descriptors continue to work as expected
  const uint16_t *mfr = tud_descriptor_string_cb(STRID_MANUFACTURER, 0x0409);
  assert(mfr != NULL);
  assert((mfr[0] >> 8) == TUSB_DESC_STRING);

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
print('USB serial and descriptor tests passed')
