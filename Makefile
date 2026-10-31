# TinyUF2 bootloader for Baikal BE-U1000 (EVU-LI-2.1)
# Requires Baikal MCU SDK. Set SDK_DIR to the SDK root.
SDK_DIR ?= $(error SDK_DIR is not set. Usage: make SDK_DIR=/path/to/sdk)

BOARD = EVU_LI_2_1

CDEFS += -DCFG_TUSB_MCU=OPT_MCU_BMCU
CDEFS += -DBOARD_TUD_MAX_SPEED=OPT_MODE_HIGH_SPEED
CDEFS += -DTUF2_LOG=0

# TinyUSB device stack
CSRC += $(SDK_DIR)/Middlewares/Third_Party/TinyUSB/src/tusb.c
CSRC += $(SDK_DIR)/Middlewares/Third_Party/TinyUSB/src/common/tusb_fifo.c
CSRC += $(SDK_DIR)/Middlewares/Third_Party/TinyUSB/src/device/usbd.c
CSRC += $(SDK_DIR)/Middlewares/Third_Party/TinyUSB/src/class/msc/msc_device.c
CSRC += $(SDK_DIR)/Middlewares/Third_Party/TinyUSB/src/portable/bmcu/dcd_bmcu.c
CSRC += $(SDK_DIR)/Middlewares/Third_Party/TinyUSB/hw/bsp/bmcu/family.c

# TinyUF2
CSRC += $(SDK_DIR)/Middlewares/Third_Party/TinyUF2/src/ghostfat.c

# HAL drivers
CSRC += $(SDK_DIR)/Drivers/HAL/Src/bmcu_cru.c
CSRC += $(SDK_DIR)/Drivers/HAL/Src/bmcu_eflash.c
CSRC += $(SDK_DIR)/Drivers/HAL/Src/bmcu_wdt.c

# Syscalls
CSRC += $(SDK_DIR)/Middlewares/Baikal/syscalls.c

# Override startup from SDK (exclude the default multi-core one)
ASRC_EXCLUDE += $(SDK_DIR)/Tools/build/platform/$(MCU_MODEL)/startup_c0_c1.S

# Include paths for TinyUSB and TinyUF2
INC_DIR += $(SDK_DIR)/Middlewares/Third_Party/TinyUSB/src
INC_DIR += $(SDK_DIR)/Middlewares/Third_Party/TinyUSB/hw
INC_DIR += $(SDK_DIR)/Middlewares/Third_Party/TinyUF2/src
# src/ overrides TinyUF2 ports/bmcu: boards.h patched for EVU_LI_2_1 (no user button)
INC_DIR += $(CURDIR)/src

# Suppress warning from TinyUF2's ghostfat.c string literal usage
CFLAGS += -Wno-unterminated-string-initialization

# Use our own linker script instead of the auto-generated one
LDSCRIPT = $(CURDIR)/linker.ld

include $(SDK_DIR)/Tools/build/common.mk
