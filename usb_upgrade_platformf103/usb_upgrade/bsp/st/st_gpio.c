/**
 * @file st_gpio.c
 * @brief STM32 GPIO / LED 驱动实现（可完全禁用）
 */

#include "hal_config.h"
#include "board_config.h"

#ifdef PLATFORM_STM32
#ifdef USB_UPGRADE_USE_LED

static void st_gpio_led_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOE, ENABLE);

    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_InitStruct.GPIO_Pin = LED0_PIN;
    GPIO_Init(LED0_PORT, &GPIO_InitStruct);
    GPIO_SetBits(LED0_PORT, LED0_PIN);

    GPIO_InitStruct.GPIO_Pin = LED1_PIN;
    GPIO_Init(LED1_PORT, &GPIO_InitStruct);
    GPIO_SetBits(LED1_PORT, LED1_PIN);
}

static void st_gpio_led_set(uint8_t led_id, uint8_t state)
{
    if (led_id == LED_ID_0) {
        if (state) GPIO_ResetBits(LED0_PORT, LED0_PIN);
        else       GPIO_SetBits(LED0_PORT, LED0_PIN);
    } else if (led_id == LED_ID_1) {
        if (state) GPIO_ResetBits(LED1_PORT, LED1_PIN);
        else       GPIO_SetBits(LED1_PORT, LED1_PIN);
    }
}

static void st_gpio_led_toggle(uint8_t led_id)
{
    if (led_id == LED_ID_0) {
        LED0_PORT->ODR ^= LED0_PIN;
    } else if (led_id == LED_ID_1) {
        LED1_PORT->ODR ^= LED1_PIN;
    }
}

const HAL_Gpio_Drv_t ST_Gpio_Drv = {
    .led_init   = st_gpio_led_init,
    .led_set    = st_gpio_led_set,
    .led_toggle = st_gpio_led_toggle,
};

const HAL_Gpio_Drv_t *HAL_Gpio = &ST_Gpio_Drv;

/* ──── 兼容旧接口（可选） ──── */

#ifdef USB_UPGRADE_COMPAT_FUNCTIONS
void LED_Init(void) { st_gpio_led_init(); }
#endif

#endif /* USB_UPGRADE_USE_LED */
#endif /* PLATFORM_STM32 */
