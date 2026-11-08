/**
 * *****************************************************************************
 *  @file       updater/main.c
 *  @brief      Bootloader self-updater application for Baikal BE-U1000.
 * *****************************************************************************
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "bmcu_common.h"
#include "bmcu_cru.h"
#include "bmcu_eflash.h"
#include "bmcu_wdt.h"
#include "bsp.h"

extern const uint8_t bootloader_bin[];
extern const uint32_t bootloader_bin_len;

#define BOOTLOADER_FLASH_ADDR   0xA0000000UL
#define BOOTLOADER_MAX_SIZE     (16UL * 1024UL)
#define FLASH_SECTOR_SIZE       1024UL
#define APP_FLASH_ADDR          0xA0004000UL
#define STATE_FLASH_ADDR        0xA003FC00UL

static void
led_blink (int count) {
    bsp_led_init();
    for (int i = 0; i < count; i++) {
        bsp_led_on();
        __delay_ms(100);
        bsp_led_off();
        __delay_ms(100);
    }
}

void
board_reset (void) {
    CRU_APB0_EnableClock(CRU_APB0_PERIPH_WDT0);
    WDT_InitStruct_TypeDef wdt;
    WDT_StructInit(&wdt);
    wdt.Mode = WDT_MODE_RESET;
    wdt.Rstlen = WDT_RST_LEN_32;
    wdt.Timeout = WDT_TIME_PERIOD_0;
    WDT_Init(WDT0, &wdt);
    for (;;);
}

int
main (void) {
    /* Disable interrupts globally */
    __disable_irq();

    /* Disable WDT during flashing */
    WDT_DeInit(WDT0);

    /* Initialize eFlash controller at 100 MHz */
    EFLASH_Init(100000000UL);

    uint32_t size = bootloader_bin_len;
    if (size == 0 || size > BOOTLOADER_MAX_SIZE) {
        led_blink(10);
        for (;;);
    }

    /* 1. Erase bootloader sectors (16 sectors: 0xA0000000 - 0xA0004000) */
    for (uint32_t addr = BOOTLOADER_FLASH_ADDR; addr < APP_FLASH_ADDR; addr += FLASH_SECTOR_SIZE) {
        uint32_t retries = 1000;
        while (EFLASH_EraseSector(addr, EFLASH_MAIN_ARRAY) != EFLASH_COMPLETE && --retries) {
        }
        if (!retries) {
            led_blink(20);
            for (;;);
        }
    }

    /* 2. Program new bootloader image */
#if defined (BE_U1000)
    CRU_EFLASH_DMAMode_Enable();
#endif
    memcpy((void*)BOOTLOADER_FLASH_ADDR, bootloader_bin, size);
#if defined (BE_U1000)
    CRU_EFLASH_DMAMode_Disable();
#endif

    /* 3. Verify written image */
    if (memcmp((const void*)BOOTLOADER_FLASH_ADDR, bootloader_bin, size) != 0) {
        led_blink(30);
        for (;;);
    }

    /* 4. Erase updater from application area and clear state sector */
    EFLASH_EraseSector(APP_FLASH_ADDR, EFLASH_MAIN_ARRAY);
    EFLASH_EraseSector(STATE_FLASH_ADDR, EFLASH_MAIN_ARRAY);

    /* 5. Blink success signal and reboot into newly flashed bootloader */
    led_blink(3);
    board_reset();

    for (;;);
    return 0;
}
