/**
 * @file plat_button_gd32.c
 * @brief GD32 按钮实现 — 正点原子 F407 KEY0=PE4
 */
#include "plat_button.h"
#include "board_pin_config.h"
#include "plat_tick.h"

static uint8_t button_raw_read(void)
{
#if MODE_BTN_ACTIVE_LOW
    return gpio_input_bit_get(MODE_BTN_PORT, MODE_BTN_PIN) ? 0 : 1;
#else
    return gpio_input_bit_get(MODE_BTN_PORT, MODE_BTN_PIN) ? 1 : 0;
#endif
}

plat_btn_event_t plat_button_read(void)
{
    static uint8_t last = 0;
    static uint32_t press_start = 0;
    uint8_t cur = button_raw_read();
    uint32_t now = plat_get_tick_ms();

    if (cur && !last) {
        press_start = now;
    }
    if (!cur && last) {
        uint32_t dur = now - press_start;
        if (dur >= 2000) return PLAT_BTN_LONG_PRESS;
        if (dur >= 50)  return PLAT_BTN_SHORT_PRESS;
    }
    last = cur;
    return PLAT_BTN_NONE;
}

uint8_t plat_button_check_mode_switch(uint32_t timeout_ms)
{
    uint32_t start = plat_get_tick_ms();
    while ((plat_get_tick_ms() - start) < timeout_ms) {
        if (plat_button_read() == PLAT_BTN_LONG_PRESS)
            return 1;
        plat_delay_ms(10);
    }
    return 0;
}
