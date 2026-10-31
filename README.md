# Baikal Bootloader

TinyUF2 bootloader for **Baikal BE-U1000** MCU (configured for **EVU-LI-2.1** board).

Exposes a USB Mass Storage device labeled `UF2`. Flashing applications is as simple as dragging and dropping a `.uf2` file onto the drive.

---

## 🗺️ Memory Layout

### Flash Memory (eFlash / External Flash mapped to `0xA0000000`)

Overall Flash size: **256 KB** (Sector size: **1 KB**).

| Region | Start Address | End Address | Size | Description |
|---|---|---|---|---|
| **Bootloader** | `0xA0000000` | `0xA0003FFF` | 16 KB | TinyUF2 bootloader image |
| **Application** | `0xA0004000` | `0xA003FFFF` | 240 KB | User application region |

> [!NOTE]
> The bootloader checks for a valid application signature at `0xA0004020` (Magic Number: `0x0BEF5262`). If this word matches, and DFU mode is not requested, control jumps to `0xA0004000`.

### Bootloader Runtime Memory (Internal RAM)

During bootloader execution, code and data are placed as follows:
- **TCMA (`0x40010000`, 64 KB)**: Execution code (`.text`) and read-only data (`.rodata`). Executing from TCM allows the bootloader to safely erase or rewrite flash without stalling instruction fetch.
- **SRAM (`0x70000000`, 32 KB)**: Mutable data (`.data`, `.bss`), heap, and stack.

---

## ⚡ How to Enter DFU (Bootloader) Mode

Since the **EVU-LI-2.1** board does not have a dedicated user button (only a Reset button):

### 1. Fast Double Reset (Recommended)
Press the **Reset** button twice within **500 ms**.
- On the first reset, the bootloader writes a magic flag into TCMB RAM (`TCMB_BASE`, preserved across soft reset) and waits 500 ms.
- If reset is pressed a second time within this window, the flag is detected and the board enters DFU mode.
- The onboard LED on **PA15** (`GPIO0_PIN_15`) blinks briefly to confirm that DFU mode has been activated.

### 2. Automatic Entry (Empty/Corrupted App)
If the application region does not contain a valid application (i.e. magic number at `0xA0004020` is not `0x0BEF5262`), the bootloader enters DFU mode automatically on boot and pulses the **PA15** LED.

### 3. Software Trigger from User Application
The running application can jump to bootloader by writing the double-tap magic value:
```c
#define TCMB_BASE 0x40020000UL /* or platform-specific TCMB base */
#define DBL_TAP_MAGIC 0xF01669EFUL

*(__IO uint32_t *)TCMB_BASE = DBL_TAP_MAGIC;
NVIC_SystemReset(); /* or equivalent RISC-V software reset */
```

### 💡 LED Indication (PA15 / GPIO0_PIN_15)
- **DFU Activation:** Brief pulse on PA15 confirms entry into the bootloader.
- **Flashing Process:** PA15 stays ON continuously while UF2 blocks are being written to flash memory.
- **Flashing Complete:** PA15 turns OFF and the MCU reboots into the updated application.

---

## 📦 Flashing Firmware

When in DFU mode, the board mounts as a USB drive named `UF2`.
The drive contains virtual files:
- `INFO_UF2.TXT` — board and bootloader details
- `INDEX.HTM` — web redirect to documentation
- `CURRENT.UF2` — current contents of flash memory

### Converting Your Firmware to UF2

Compile your user application with offset `0x4000` (16 KB offset, base address `0xA0004000`):

**From HEX:**
```sh
python3 scripts/uf2conv.py firmware.hex -c -o firmware.uf2 -f 0x6ecb67a8
```

**From BIN:**
```sh
python3 scripts/uf2conv.py firmware.bin -c -o firmware.uf2 -f 0x6ecb67a8 -b 0xA0004000
```

> **Family ID:** `0x6ecb67a8` (Baikal BE-U1000 / BMCU family)

Simply drag and drop `firmware.uf2` onto the `UF2` drive. The LED will illuminate while flashing is in progress, and the device will reboot into your application automatically when complete.

---

## 🛠️ Building the Bootloader

### GitHub Actions (Automatic)
Every push to `main` automatically builds `release` artifacts on GitHub Actions.
Precompiled binaries can be downloaded directly from the workflow run artifacts:
- `baikal-bootloader.bin`
- `baikal-bootloader.hex`
- `baikal-bootloader.elf`

### Local Build (Linux x86_64)
Requires the official [Baikal MCU SDK v2.4.0](https://mcu.baikalelectronics.ru/):

```sh
make SDK_DIR=/path/to/mcu-sdk-linux_2.4.0_20261006 BUILD_TYPE=release
```
Outputs are generated in `output/release/`.
