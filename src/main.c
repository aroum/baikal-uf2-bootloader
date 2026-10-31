/**
 * *****************************************************************************
 *  @file       Projects/HAL_examples/USB/Device/USB_UF2_Bootloader/main.c
 *  @author     Baikal electronics SDK team
 *  @brief      USB UF2 bootloader example
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

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "bmcu_common.h"
#include "bmcu_cru.h"
#include "bmcu_eflash.h"
#include "tusb.h"
#include "board_api.h"
#include "uf2.h"
#include "bsp.h"

static uint32_t indicator_state = STATE_BOOTLOADER_STARTED;
static uint32_t led_cycle_length = 300; // ms (fast blink by default before USB mount)

static bool check_dfu_mode(void);

int
__io_putchar (int ch) {
    return bsp_serial_putchar(ch);
}

void
indicator_set (uint32_t state) {
    indicator_state = state;

    switch (indicator_state)
    {
        case STATE_USB_PLUGGED:
        {
            led_cycle_length = 3000; // slow breathing/blink when USB connected & ready
        }
        break;

        case STATE_BOOTLOADER_STARTED:
        case STATE_USB_UNPLUGGED:
        {
            led_cycle_length = 300; // fast blink when disconnected / waiting
        }
        break;

        case STATE_WRITING_STARTED:
        {
            led_cycle_length = 100; // rapid blinking while flashing
            board_led_write(1UL);
        }
        break;

        case STATE_WRITING_FINISHED:
        default:
        {
            led_cycle_length = 3000;
            board_led_write(0UL);
        }
        break;
    }
}

static void
led_task (void) {
    if (indicator_state == STATE_WRITING_STARTED)
    {
        board_led_write(1UL);
        return;
    }

    uint32_t millis = tusb_time_millis_api();
    uint32_t cycle = millis % led_cycle_length;
    uint32_t half_cycle = led_cycle_length / 2;

    if (cycle < half_cycle)
    {
        board_led_write(1UL);
    }
    else
    {
        board_led_write(0UL);
    }
}

/* Invoked when device is mounted */
void
tud_mount_cb (void) {
    indicator_set(STATE_USB_PLUGGED);
}

/* Invoked when device is unmounted */
void
tud_umount_cb (void) {
    indicator_set(STATE_USB_UNPLUGGED);
}

/* Invoked when USB bus is suspended */
void
tud_suspend_cb (bool remote_wakeup_en) {
    (void)remote_wakeup_en;
    indicator_set(STATE_USB_UNPLUGGED);
}

/* Invoked when USB bus is resumed */
void
tud_resume_cb (void) {
    if (tud_mounted())
    {
        indicator_set(STATE_USB_PLUGGED);
    }
    else
    {
        indicator_set(STATE_USB_UNPLUGGED);
    }
}

bool
check_dfu_mode (void) {
    bool retval = false;

    if (!board_app_valid())
    {
        TUF2_LOG1("App invalid\r\n");
        retval = true;
    }
    else if ((board_app_valid2) && (!board_app_valid2()))
    {
        TUF2_LOG1("App invalid\r\n");
        retval = true;
    }
    else
    {
#if USE_DFU_BUTTON
        /* Jitter filter */
        uint32_t pressed_cnt = 0UL;
        for (uint32_t i = 0UL; i < 1000UL; i++)
        {
            if (board_button_ispressed() != 0UL)
            {
                pressed_cnt++;
                if (pressed_cnt >= 100UL)
                {
                    retval = true;
                    break;
                }
            }
            else
            {
                pressed_cnt = 0UL;
            }
        }
#endif /* USE_DFU_BUTTON */
    }

#if TINYUF2_DBL_TAP_DFU
    if (!retval)
    {
        TUF2_LOG1_HEX(TINYUF2_DBL_TAP_REG);

        if (TINYUF2_DBL_TAP_REG == DBL_TAP_MAGIC_QUICK_BOOT)
        {
            /* Boot to app quickly. DBL_TAP_MAGIC_ERASE_APP may be set from app
               before reset to bypass double-tap detection delay. */
            TUF2_LOG1("Quick boot to App\r\n");
            TINYUF2_DBL_TAP_REG = 0x0UL;
        }
        else if (TINYUF2_DBL_TAP_REG == DBL_TAP_MAGIC)
        {
            /* Double tap occurred */
            TUF2_LOG1("Double Tap Reset\r\n");
            TINYUF2_DBL_TAP_REG = 0x0UL;

            board_led_write(1UL);
            __delay_ms(10UL);

            retval = true;
        }
        else if (TINYUF2_DBL_TAP_REG == DBL_TAP_MAGIC_ERASE_APP)
        {
            /* DBL_TAP_MAGIC_ERASE_APP may be set from app before reset to erase app memory */
            TUF2_LOG1("Erase app\r\n");
            TINYUF2_DBL_TAP_REG = 0x0UL;

            indicator_set(STATE_WRITING_STARTED);
            board_flash_erase_app();
            indicator_set(STATE_WRITING_FINISHED);

            retval = true;
        }
        else
        {
            /* Register the first reset tap for double reset tap detection */
            TINYUF2_DBL_TAP_REG = DBL_TAP_MAGIC;

            uint64_t curr_time_us;
            uint64_t start_time_us = __get_time();

            /* Delay to detect second Reset tap */
            do
            {
                curr_time_us = __get_time();
            }
            while ((curr_time_us - start_time_us) < (TINYUF2_DBL_TAP_DELAY * 1000U));

            /* Clear magic number if the reset has not been tapped second time */
            TINYUF2_DBL_TAP_REG = 0x0UL;
        }
    }
    else
    {
        /* App is invalid or bootloader has been invoked with the button */
        board_led_write(1UL);
        __delay_ms(10UL);
    }
#endif /* TINYUF2_DBL_TAP_DFU */

    return retval;
}

int
main (void) {
    /* Initialize board */
    board_init();
    if (board_init2)
        board_init2();
    TUF2_LOG1("TinyUF2\r\n");

#if TINYUF2_PROTECT_BOOTLOADER
    /* Protect bootloader */
    board_flash_protect_bootloader(true);
#endif

    /* Jump to app, if it is valid and
       DFU mode has not been requested */
    if (!check_dfu_mode())
    {
        TUF2_LOG1("Jump to application\r\n");
        if (board_teardown)
            board_teardown();
        if (board_teardown2)
            board_teardown2();
        board_app_jump();
        TUF2_LOG1("Failed to jump\r\n");
        for (;;);
    }

    /* Initialize DFU mode */
    TUF2_LOG1("Start DFU mode\r\n");
    board_flash_init();
    uf2_init();
    indicator_set(STATE_USB_UNPLUGGED);
    board_dfu_init();

    for (;;)
    {
        tud_task();
        led_task();

        if (board_inactivity_timeout_is_ellapsed())
        {
            /* Reset board */
            board_reset();  
        }
    }

    return 0;
}
