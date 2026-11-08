/**
 * *****************************************************************************
 *  @file       Middlewares/Third_Party/TinyUF2/ports/bmcu/boards.c
 *  @author     Baikal electronics SDK team
 *  @brief      TinyUF2 port source file
 *  @version    2.4.0
 *  @date       2026-10-06
 * *****************************************************************************
 *  @copyright Copyright (c) 2026 Baikal Electronics JSC
 *
 *  This software is licensed under terms that can be found in the LICENSE file
 *  in the root directory of this software component.
 *  If no LICENSE file comes with this software, it is provided AS-IS.
 * *****************************************************************************
 */

#include <stdio.h>
#include "board_api.h"
#include "bmcu_common.h"
#include "bmcu_cru.h"
#include "bmcu_eflash.h"
#include "bmcu_wdt.h"
#include "bsp.h"

#ifndef BUILD_NO_TINYUSB
#include "tusb.h"
#endif

#define BOARD_FLASH_SECTOR_NUM      (BOARD_FLASH_SIZE / BOARD_FLASH_SECTOR_SIZE)    /* The number of flash sectors */

static bool flash_sectors_erased[BOARD_FLASH_SECTOR_NUM] = {0};     /* Sector erase marker */

bool bl_flash_range_ok(uintptr_t addr, uint32_t len) {
    uintptr_t end = BOARD_FLASH_APP_ADDR + BOARD_FLASH_APP_SIZE;
    return len && !(addr & 3U) && !(len & 3U) &&
           addr >= BOARD_FLASH_APP_ADDR && addr < end && len <= end - addr;
}

static bool inactivity_timeout_state = false;           /* Inactivity timeout state */
static uint64_t inactivity_timeout_start_time;          /* Inactivity timeout start time */

//--------------------------------------------------------------------+
// System clock
//--------------------------------------------------------------------+

static void CLK_Init(void);
static void CLK_DeInit(void);

/**
 * @brief Configures system clocks.
 *
 *        PLL   = 200MHz
 *        CCLK  = 100MHz
 *        PCLK0 = 100MHz
 *        PCLK1 = 100MHz
 *        PCLK2 = 100MHz
 *        HCLK  = 100MHz
 */
void
CLK_Init (void) {
    CRU_PLL_BypassMode_Disable();

    CRU_PLL_SetSource(CRU_PLL_SRC_C0);

    CRU_PLL_ForceReset();
    CRU_PLL_Config(1UL, 48UL, 6UL, 48UL);
    CRU_PLL_ReleaseReset();

    __delay_cycles(500UL);

    CRU_SetCCLKPrescaler(CRU_CLK_DIV_2);
    CRU_CCLKPrescaler_Enable();
    CRU_SetCCLKSource(CRU_CLK_SRC_PLL);

    CRU_SetPCLK0Prescaler(CRU_CLK_DIV_2);
    CRU_PCLK0Prescaler_Enable();
    CRU_SetPCLK0Source(CRU_CLK_SRC_PLL);

    CRU_SetPCLK1Prescaler(CRU_CLK_DIV_2);
    CRU_PCLK1Prescaler_Enable();
    CRU_SetPCLK1Source(CRU_CLK_SRC_PLL);

    CRU_SetPCLK2Prescaler(CRU_CLK_DIV_2);
    CRU_PCLK2Prescaler_Enable();
    CRU_SetPCLK2Source(CRU_CLK_SRC_PLL);

    CRU_SetHCLKPrescaler(CRU_CLK_DIV_2);
    CRU_HCLKPrescaler_Enable();
    CRU_SetHCLKSource(CRU_CLK_SRC_PLL);

    CRU_SetCCLK1MHzPrescaler(99UL);
    CRU_SetHCLK1MHzPrescaler(99UL);

    CRU_USB_PHY_PLL_Enable();
    __delay_cycles(100UL);
    CRU_USB_PHY_ReleaseReset();
    __delay_us(500UL);
}

/**
 * @brief Resets system clocks to default values.
 */
void
CLK_DeInit (void) {
    /* Deinit USB PLL and PHY */
    CRU_USB_PHY_ForceReset();
    CRU_USB_PHY_PLL_Disable();

    /* Deinit CRU */
    CRU_DeInit();
}

//--------------------------------------------------------------------+
// Watchdog
//--------------------------------------------------------------------+

static void WDT0_Init(void);
static void WDT0_DeInit(void);

/**
 * @brief Initializes WDT0.
 */
void
WDT0_Init (void) {
    CRU_APB0_EnableClock(CRU_APB0_PERIPH_WDT0);

    WDT_InitStruct_TypeDef WDT_InitStruct;
    WDT_StructInit(&WDT_InitStruct);
    WDT_InitStruct.Mode = WDT_MODE_RESET;
    WDT_InitStruct.Rstlen = WDT_RST_LEN_32;
    WDT_InitStruct.Timeout = WDT_TIME_PERIOD_10; /* Do not set small timeout period, otherwise MCU
                                                   can restart endlessly due to boot ROM. See errata. */
    WDT_Init(WDT0, &WDT_InitStruct);
}

/**
 * @brief Deinitializes WDT0.
 */
void
WDT0_DeInit (void) {
    WDT_DeInit(WDT0);
}

/**
 * @brief Initializes the board basic peripherals.
 * @note  This API does not initialize USB that is done only if DFU is entered.
 */
void
board_init (void) {
    /* WDT reset does not disable it. It is a safety feature of the MCU.
       If necessary the WDT should be disabled manually. */
    WDT0_DeInit();

    /* Prepare eFlash to 100 MHz CCLK. See PLL configuration */
    EFLASH_Init(100000000UL);

    CLK_Init();

    CLIC_Config(1U, 1U);
    CLIC_SetLevelThreshold(0U);

    bsp_led_init();

#if USE_DFU_BUTTON
    bsp_btn_init();
#endif /* USE_DFU_BUTTON */

    bsp_serial_init();

    /* Disable buffered stdout (commented out to save flash space) */
    // setbuf(stdout, NULL);

    /* Enable interrupts globally */
    __enable_irq();
}

/**
 * @brief Resets all board peripherals.
 * @note  It is called prior to jumping to application.
 */
void
board_teardown (void) {
    /* Disable interrupts globally */
    __disable_irq();

    /* Deinit USB */
    tusb_deinit(BOARD_TUD_RHPORT);

    /* Restore stdout buffering (commented out to save flash space) */
    // setvbuf(stdout, NULL, _IOFBF, BUFSIZ);

    /* Deinit serial */
    bsp_serial_deinit();

    /* Deinit LED */
    bsp_led_deinit();

    /* Deinit CLIC */
    CLIC_Config(0U, 1U);
    CLIC_SetLevelThreshold(0U);

    /* Deinit CLK */
    CLK_DeInit();

    /* Deinit EFLASH */
    EFLASH_Init(25000000UL);
}

/**
 * @brief Initializes DFU mode.
 * @note  USB must be started here, because it is only needed in DFU mode.
 */
void
board_dfu_init (void) {
    /* Init USB stack */
    tusb_rhport_init_t dev_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO
    };
    tusb_init(BOARD_TUD_RHPORT, &dev_init);
}

/**
 * @brief Resets the board.
 */
void
board_reset (void) {
    /* Disable interrupts globally */
    __disable_irq();

    /* Use WDT to make software reset */
    WDT0_Init();

    /* Wait until WDT resets MCU */
    for (;;);
}

static uintptr_t written_app_end = BOARD_FLASH_APP_ADDR;

uint32_t
board_flash_get_written_size (void) {
    if (written_app_end <= BOARD_FLASH_APP_ADDR) return 0U;
    return (uint32_t)(written_app_end - BOARD_FLASH_APP_ADDR);
}

uint32_t
board_app_crc32 (uintptr_t start, uint32_t len) {
    const uint8_t *p = (const uint8_t *)start;
    uint32_t crc = 0xFFFFFFFFU;
    while (len--) {
        crc ^= *p++;
        for (int i = 0; i < 8; i++) {
            crc = (crc >> 1) ^ (0xEDB88320U & (0U - (crc & 1U)));
        }
    }
    return ~crc;
}

void
board_app_state_invalidate (void) {
    uint32_t zero = BOARD_APP_STATE_INVALID;
    EFLASH_ProgramWord(BOARD_FLASH_STATE_ADDR, zero, EFLASH_MAIN_ARRAY);
}

bool
board_app_state_commit (uint32_t app_size, uint32_t crc32) {
    board_app_state_t st;
    st.state = BOARD_APP_STATE_VALID;
    st.app_size = app_size;
    st.crc32 = crc32;
    st.check = st.state ^ st.app_size ^ st.crc32 ^ BOARD_APP_STATE_CHECK_XOR;

    if (EFLASH_EraseSector(BOARD_FLASH_STATE_ADDR, EFLASH_MAIN_ARRAY) != EFLASH_COMPLETE) {
        return false;
    }

    if (EFLASH_ProgramWord(BOARD_FLASH_STATE_ADDR + 4, st.app_size, EFLASH_MAIN_ARRAY) != EFLASH_COMPLETE) return false;
    if (EFLASH_ProgramWord(BOARD_FLASH_STATE_ADDR + 8, st.crc32, EFLASH_MAIN_ARRAY) != EFLASH_COMPLETE) return false;
    if (EFLASH_ProgramWord(BOARD_FLASH_STATE_ADDR + 12, st.check, EFLASH_MAIN_ARRAY) != EFLASH_COMPLETE) return false;
    if (EFLASH_ProgramWord(BOARD_FLASH_STATE_ADDR, st.state, EFLASH_MAIN_ARRAY) != EFLASH_COMPLETE) return false;

    return true;
}

/**
 * @brief The function that is called when DFU is complete.
 */
void
board_dfu_complete (void) {
    uint32_t app_size = board_flash_get_written_size();
    if (app_size > 0 && app_size <= BOARD_FLASH_APP_SIZE) {
        uint32_t crc = board_app_crc32(BOARD_FLASH_APP_ADDR, app_size);
        board_app_state_commit(app_size, crc);
    }
    /* Set up inactivity timeout to give host OS time to write file metadata. */
    board_inactivity_timeout_set();
}

/**
 * @brief   Checks if application is valid.
 * @returns True, if valid, otherwise false.
 */
bool
board_app_valid (void) {
    volatile uint32_t magic_num = *(volatile uint32_t*)BOARD_FLASH_APP_MAGIC_ADDR;
    if (magic_num != BOARD_FLASH_APP_MAGIC_NUM) {
        return false;
    }

    const board_app_state_t *st = (const board_app_state_t *)BOARD_FLASH_STATE_ADDR;
    if (st->state == BOARD_APP_STATE_INVALID) {
        return false;
    }

    if (st->state == BOARD_APP_STATE_VALID) {
        if (st->check != (st->state ^ st->app_size ^ st->crc32 ^ BOARD_APP_STATE_CHECK_XOR)) {
            return false;
        }
        if (st->app_size == 0 || st->app_size > BOARD_FLASH_APP_SIZE) {
            return false;
        }
        if (board_app_crc32(BOARD_FLASH_APP_ADDR, st->app_size) != st->crc32) {
            return false;
        }
        return true;
    }

    if (st->state == BOARD_APP_STATE_ERASED) {
        return true;
    }

    return false;
}

/**
 * @brief Jumps to application.
 */
void
board_app_jump (void) {
    /* Jump to the application entry point */
    void (*app_entry)(void) = (void (*)(void))BOARD_FLASH_APP_ADDR;
    app_entry();
}

/**
 * @brief   Gets the board serial number.
 * @param   serial_id The serial number buffer (at least 16 bytes).
 * @returns The length of the serial number (0 if no unique ID is available).
 */
uint8_t
board_usb_get_serial (uint8_t serial_id[16]) {
    (void)serial_id;
    /* BE-U1000 MCU does not have a factory hardware UID register.
     * Returning 0 indicates that no unique serial number is available.
     * The USB device descriptor sets iSerialNumber = 0, letting the host
     * identify devices by physical USB port path rather than colliding on
     * a hardcoded identical serial string across all devices. */
    return 0U;
}

//--------------------------------------------------------------------+
// LED
//--------------------------------------------------------------------+

/**
 * @brief Controls LED.
 * @param state The LED state (0: off, 1: on).
 */
void
board_led_write (uint32_t state) {
    if (state > 0UL)
    {
        bsp_led_on();
    }
    else
    {
        bsp_led_off();
    }
}

//--------------------------------------------------------------------+
// Flash
//--------------------------------------------------------------------+

/**
 * @brief Initializes flash memory.
 */
void
board_flash_init (void) {
    board_flash_erase_reset();
}

void board_flash_erase_reset(void) {
    memset(flash_sectors_erased, 0, sizeof(flash_sectors_erased));
    written_app_end = BOARD_FLASH_APP_ADDR;
}

/**
 * @brief Gets the full size of flash memory.
 */
uint32_t
board_flash_size (void) {
    return BOARD_FLASH_APP_SIZE;
}

/**
 * @brief Reads flash memory.
 * @param addr   The address to read (relative to BOARD_FLASH_APP_START).
 * @param buffer The data buffer.
 * @param len    The number of bytes to read.
 */
void
board_flash_read (uint32_t addr, void* buffer, uint32_t len) {
    memcpy(buffer, (void*)addr, len);
}

/**
 * @brief Syncs flash memory content.
 */
void
board_flash_flush (void) {
    /* Write operation is not buffered, no need to flush */
}

/**
 * @brief Writes flash memory.
 * @param addr   The address to read (relative to the beginning).
 * @param buffer The data buffer.
 * @param len    The number of bytes to write.
 */
bool
board_flash_write (uint32_t addr, void const* data, uint32_t len) {
    if (!data || !bl_flash_range_ok(addr, len)) return false;
#if defined (BE_U1000)
    CRU_EFLASH_DMAMode_Enable();
#endif
    memcpy((void*)addr, data, len);
#if defined (BE_U1000)
    CRU_EFLASH_DMAMode_Disable();
#endif

    if (memcmp((const void*)addr, data, len) != 0) return false;
    if ((uintptr_t)addr + len > written_app_end) {
        written_app_end = (uintptr_t)addr + len;
    }
    return true;
}

/**
 * @brief Erases the application flash memory region.
 */
void
board_flash_erase_app (void) {
    board_flash_erase_reset();
    uint32_t sector_address = BOARD_FLASH_APP_ADDR;
    while (sector_address < (BOARD_FLASH_APP_ADDR + BOARD_FLASH_APP_SIZE))
    {
        if (!board_flash_erase_overlapping_sectors((void *)(uintptr_t)sector_address,
                                                  BOARD_FLASH_SECTOR_SIZE)) {
            TUF2_LOG1("Application erase failed\r\n");
            break;
        }
        sector_address += BOARD_FLASH_SECTOR_SIZE;
    }
    /* Also erase application state sector */
    EFLASH_EraseSector(BOARD_FLASH_STATE_ADDR, EFLASH_MAIN_ARRAY);
    /* A subsequent UF2 transfer must start with a fresh erase map. */
    board_flash_erase_reset();
}

/**
 * @brief Erases the flash memory sectors overlapping with a buffer.
 * @param buffer The buffer address.
 * @param len The buffer length.
 */
bool
board_flash_erase_overlapping_sectors (void* buffer, uint32_t len)
{
    uintptr_t start = (uintptr_t)buffer;
    if (!bl_flash_range_ok(start, len)) return false;
    uintptr_t end = start + len - 1U;

    uint32_t first = (uint32_t)((start - BOARD_FLASH_ADDR_ZERO) / BOARD_FLASH_SECTOR_SIZE);
    uint32_t last = (uint32_t)((end - BOARD_FLASH_ADDR_ZERO) / BOARD_FLASH_SECTOR_SIZE);

    for (uint32_t i = first; i <= last; i++)
    {
        if (!flash_sectors_erased[i])
        {
            uint32_t retries = 1000;
            while (EFLASH_EraseSector(BOARD_FLASH_ADDR_ZERO + i * BOARD_FLASH_SECTOR_SIZE, EFLASH_MAIN_ARRAY) != EFLASH_COMPLETE) {
                if (--retries == 0) return false;
            }
            flash_sectors_erased[i] = true;
        }
    }
    return true;
}

/**
 * @brief Sets the boolloader protection state.
 * @param protect The protection state (false: disabled, true: enabled).
 */
bool
board_flash_protect_bootloader (bool protect) {
    (void)protect;

    /* Not implemented in hardware */

    return false;
}

//--------------------------------------------------------------------+
// Miscellaneous
//--------------------------------------------------------------------+

/**
 * @brief Sets up inactivity timeout after firmware update.
 */
void
board_inactivity_timeout_set (void) {
    inactivity_timeout_start_time = __get_time();
    inactivity_timeout_state = true;
}

/**
 * @brief   Checks whether the inactivity timeout is set up.
 * @returns True, if timeout is set up, otherwise false.
 */
bool
board_inactivity_timeout_is_started (void) {
    return inactivity_timeout_state;
}

/**
 * @brief   Checks whether the inactivity timeout occured.
 * @returns True, if timeout is occured, otherwise false.
 */
bool
board_inactivity_timeout_is_ellapsed (void) {
    return (((__get_time() - inactivity_timeout_start_time) >= (BOARD_INACTIVITY_TIMEOUT_MS * 1000U)) && (inactivity_timeout_state));
}
