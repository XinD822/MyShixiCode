/**
 * @file plat_button.h
 * @brief 按钮输入抽象 — 接口定义，由目标设备实现
 */
#ifndef __PLAT_BUTTON_H
#define __PLAT_BUTTON_H
#include "plat_config.h"
#include <stdint.h>

typedef enum {
    PLAT_BTN_NONE = 0,
    PLAT_BTN_SHORT_PRESS,
    PLAT_BTN_LONG_PRESS,
    PLAT_BTN_DOUBLE_PRESS,
} plat_btn_event_t;

plat_btn_event_t plat_button_read(void);
uint8_t plat_button_check_mode_switch(uint32_t timeout_ms);

#endif /* __PLAT_BUTTON_H */
