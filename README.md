# Baikal Bootloader

TinyUF2 bootloader for **Baikal BE-U1000** MCU (EVU-LI-2.1 board).

Appears as a USB mass storage device labeled `UF2` when in DFU mode.
Drop a `.uf2` file onto the drive to flash the application.

## Flash layout

| Region      | Address      | Size  |
|-------------|-------------|-------|
| Bootloader  | `0xA0000000` | 64 KB |
| Application | `0xA0010000` | 192 KB |

## Entering DFU mode

- Hold **USER button** during reset, or
- Double-reset within 500 ms

## Building locally

Requires [Baikal MCU SDK v2.4.0](https://mcu.baikalelectronics.ru/).

```sh
make SDK_DIR=/path/to/mcu-sdk BUILD_TYPE=release
```

Output files are in `output/release/`.

## CI

GitHub Actions builds on every push to `main`.
The SDK is downloaded from the official URL and cached between runs.
Artifact: `baikal-bootloader-EVU_LI_2_1` (ELF + BIN + HEX).

## Converting application to UF2

```sh
python3 uf2conv.py firmware.hex -c -o firmware.uf2 -f BE-UXXXX
# or from BIN:
python3 uf2conv.py firmware.bin -c -o firmware.uf2 -f BE-UXXXX -b 0xA0010000
```
