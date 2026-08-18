/**
 * @file hal_gpio.h
 * @brief GPIO / LED HAL 接口
 */

#ifndef __HAL_GPIO_H
#define __HAL_GPIO_H

#include <stdint.h>

typedef struct {
    void (*led_init)(void);
    void (*led_set)(uint8_t led_id, uint8_t state);
    void (*led_toggle)(uint8_t led_id);
} HAL_Gpio_Drv_t;

extern const HAL_Gpio_Drv_t *HAL_Gpio;

/* LED ID 和状态定义（在 board_config.h 中定义） */
#ifndef LED_ID_0
#define LED_ID_0    0
#endif
#ifndef LED_ID_1
#define LED_ID_1    1
#endif
#define LED_OFF     0
#define LED_ON      1

#endif /* __HAL_GPIO_H */
