/**
 * *****************************************************************************
 *  @file       Middlewares/Third_Party/TinyUF2/ports/bmcu/boards.h
 *  @author     Baikal electronics SDK team
 *  @brief      TinyUF2 port header file
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

#ifndef BOARDS_H_
#define BOARDS_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
 extern "C" {
#endif

//--------------------------------------------------------------------+
// Button
//--------------------------------------------------------------------+

// EVU_LI_2_1 has no user button; DFU entry via double-reset only
#define USE_DFU_BUTTON                  (0)

//--------------------------------------------------------------------+
// Flash
//--------------------------------------------------------------------+

#define BOARD_FLASH_SIZE                (256UL * 1024UL)            /* Flash overall size */
#define BOARD_FLASH_SECTOR_SIZE         (1024UL)                    /* Flash sector size */

#ifndef BOARD_FLASH_ADDR_ZERO
#define BOARD_FLASH_ADDR_ZERO           (0xA0000000UL)              /* Flash start address, used only for address validation */
#endif

#define BOARD_FLASH_BOOT_ADDR           BOARD_FLASH_ADDR_ZERO                                   /* Bootloader address */
#define BOARD_FLASH_BOOT_SIZE           (16UL * 1024UL)                                         /* Bootloader size */
#define BOARD_FLASH_STATE_ADDR          (BOARD_FLASH_ADDR_ZERO + BOARD_FLASH_SIZE - BOARD_FLASH_SECTOR_SIZE) /* Dedicated sector for application state ledger */
#define BOARD_FLASH_APP_ADDR            (BOARD_FLASH_ADDR_ZERO + BOARD_FLASH_BOOT_SIZE)         /* Application address */
#define BOARD_FLASH_APP_SIZE            (BOARD_FLASH_STATE_ADDR - BOARD_FLASH_APP_ADDR)         /* Application size (excluding bootloader and state sector) */
#define BOARD_FLASH_APP_START           BOARD_FLASH_APP_ADDR

#define BOARD_APP_STATE_ERASED          (0xFFFFFFFFU)
#define BOARD_APP_STATE_VALID           (0xB007C0DEU)
#define BOARD_APP_STATE_INVALID         (0x00000000U)
#define BOARD_APP_STATE_CHECK_XOR       (0xA5A5A5A5U)

typedef struct {
    uint32_t state;
    uint32_t app_size;
    uint32_t crc32;
    uint32_t check;
} board_app_state_t;

#if !defined(BOARD_FLASH_ADDR_OVERRIDE)
#if ((BOARD_FLASH_BOOT_ADDR % BOARD_FLASH_SECTOR_SIZE) != 0x0UL)
#error "BOARD_FLASH_BOOT_ADDR must be aligned on a sector boundary"
#endif

#if ((BOARD_FLASH_APP_ADDR % BOARD_FLASH_SECTOR_SIZE) != 0x0UL)
#error "BOARD_FLASH_APP_ADDR must be aligned on a sector boundary"
#endif
#endif

#define BOARD_FLASH_APP_MAGIC_NUM       (0x0BEF5262UL)                                          /* Application magic number used for validation */
#define BOARD_FLASH_APP_MAGIC_OFFSET    (0x20UL)                                                /* Application magic number offset */
#define BOARD_FLASH_APP_MAGIC_ADDR      (BOARD_FLASH_APP_ADDR + BOARD_FLASH_APP_MAGIC_OFFSET)   /* Application magic number address */

//--------------------------------------------------------------------+
// USB UF2
//--------------------------------------------------------------------+

#define BOARD_UF2_FAMILY_ID             (0x6ecb67a8)

#define USB_MANUFACTURER                "Baikal Electronics"
#define USB_PRODUCT                     "Baikal EVU-LI 2.1"

#define UF2_PRODUCT_NAME                "Baikal EVU-LI 2.1"
#define UF2_BOARD_ID                    "Baikal EVU-LI 2.1"
#define UF2_VOLUME_LABEL                "BAIKAL_LI  "
// #define UF2_INDEX_URL                "https://www.baikalelectronics.ru/"


#define TINYUF2_DBL_TAP_DFU             (1)
#define TINYUF2_DBL_TAP_REG             (*(__IO uint32_t*)TCMB_BASE)
#define TINYUF2_DBL_TAP_REG_SIZE        (32)
#define TINYUF2_DBL_TAP_DELAY           (500ULL)

#define BOARD_INACTIVITY_TIMEOUT_MS     (500ULL)    /* Inactivity timeout applied after firmware update. It is to give host OS time to write file metadata. */

/** @brief Erases the flash memory sectors overlapping with a buffer. */
bool board_flash_erase_overlapping_sectors(void* buffer, uint32_t len);
bool bl_flash_range_ok(uintptr_t addr, uint32_t len);
void board_flash_erase_reset(void);

uint32_t board_app_crc32(uintptr_t start, uint32_t len);
void board_app_state_invalidate(void);
bool board_app_state_commit(uint32_t app_size, uint32_t crc32);
uint32_t board_flash_get_written_size(void);

/** @brief Sets up inactivity timeout after firmware update. */
void board_inactivity_timeout_set(void);

/** @brief Checks whether the inactivity timeout is set up. */
bool board_inactivity_timeout_is_started(void);

/** @brief Checks whether the inactivity timeout occured. */
bool board_inactivity_timeout_is_ellapsed(void);

#ifdef __cplusplus
 }
#endif

#endif /* BOARDS_H_ */
