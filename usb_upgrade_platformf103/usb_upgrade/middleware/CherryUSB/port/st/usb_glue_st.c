/*
 * CherryUSB FSDEV Glue Layer for STM32F103
 * Modified for Standard Peripheral Library (not HAL)
 */

/* Include stm32f10x.h FIRST, then CherryUSB will undef conflicting macros */
#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "misc.h"  /* NVIC */

#include "usbd_core.h"

#ifndef CONFIG_USBDEV_FSDEV_PMA_ACCESS
#error "please define CONFIG_USBDEV_FSDEV_PMA_ACCESS in usb_config.h"
#endif

/* USB低层硬件初始化 */
void usb_dc_low_level_init(uint8_t busid)
{
    (void)busid;
    /* 使能USB时钟 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);

    /* 配置USB中断 */
    NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 1);
    NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
}

/* USB低层硬件反初始化 */
void usb_dc_low_level_deinit(uint8_t busid)
{
    (void)busid;
    /* 禁用USB时钟 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, DISABLE);

    /* 禁用USB中断 */
    NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);
}

/* USB低优先级中断处理函数 */
void USB_LP_CAN1_RX0_IRQHandler(void)
{
    USBD_IRQHandler(0);
}
