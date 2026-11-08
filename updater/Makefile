# Bootloader self-updater for Baikal BE-U1000 (EVU-LI-2.1)
SDK_DIR ?= $(error SDK_DIR is not set. Usage: make SDK_DIR=/path/to/sdk)

BOARD = EVU_LI_2_1
PRJ_NAME = update-baikal-uf2-bootloader

CSRC += $(CURDIR)/main.c
ASRC += $(CURDIR)/../src/startup.S
ASRC += $(CURDIR)/bootloader_bin.S

# HAL drivers
CSRC += $(SDK_DIR)/Drivers/HAL/Src/bmcu_cru.c
CSRC += $(SDK_DIR)/Drivers/HAL/Src/bmcu_eflash.c
CSRC += $(SDK_DIR)/Drivers/HAL/Src/bmcu_wdt.c
CSRC += $(SDK_DIR)/Drivers/HAL/Src/bmcu_gpio.c

# BSP
CSRC += $(SDK_DIR)/BSP/$(BOARD)/led.c

# Syscalls
CSRC += $(SDK_DIR)/Middlewares/Baikal/syscalls.c

# Override startup from SDK
ASRC_EXCLUDE += $(SDK_DIR)/Tools/build/platform/$(MCU_MODEL)/startup_c0_c1.S

INC_DIR += $(CURDIR)
INC_DIR += $(CURDIR)/../src
INC_DIR += $(CURDIR)/..

ASFLAGS += -I$(CURDIR)/..

CFLAGS += -flto -Os
LDFLAGS += -flto -Os

LDSCRIPT = $(CURDIR)/linker.ld

include $(SDK_DIR)/Tools/build/common.mk
