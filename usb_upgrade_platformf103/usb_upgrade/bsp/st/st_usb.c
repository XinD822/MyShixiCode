/**
 * @file st_usb.c
 * @brief STM32 USB 底层实现（支持 F103 FSDEV / F407 DWC2 OTG）
 */

#include "hal_config.h"

#ifdef PLATFORM_STM32

/* ═══════════════════════════════════════════════════════════
 * F103: USB FSDEV
 * ═══════════════════════════════════════════════════════════ */
#if defined(CHIP_SERIES_F103)

static void st_usb_clock_enable(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
}

static void st_usb_clock_disable(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, DISABLE);
}

static void st_usb_nvic_enable(void)
{
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

static void st_usb_nvic_disable(void)
{
    NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);
}

static void st_usb_reset(void)
{
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_USB, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_USB, DISABLE);
}

/* ═══════════════════════════════════════════════════════════
 * F407: USB DWC2 OTG FS
 * DWC2 驱动内部已处理时钟和中断，这里只提供空实现
 * ═══════════════════════════════════════════════════════════ */
#elif defined(CHIP_SERIES_F407)

static void st_usb_clock_enable(void)
{
    /* DWC2 驱动内部初始化时钟，这里不需要额外操作 */
}

static void st_usb_clock_disable(void)
{
    /* DWC2 驱动内部处理 */
}

static void st_usb_nvic_enable(void)
{
    /* DWC2 驱动内部初始化 NVIC，这里不需要额外操作 */
}

static void st_usb_nvic_disable(void)
{
    /* DWC2 驱动内部处理 */
}

static void st_usb_reset(void)
{
    /* DWC2 驱动内部处理 */
}

#endif /* CHIP_SERIES_xxx */

/* ──── 驱动实例 ──── */

const HAL_Usb_Drv_t ST_Usb_Drv = {
    .clock_enable  = st_usb_clock_enable,
    .clock_disable = st_usb_clock_disable,
    .nvic_enable   = st_usb_nvic_enable,
    .nvic_disable  = st_usb_nvic_disable,
    .reset         = st_usb_reset,
};

const HAL_Usb_Drv_t *HAL_Usb = &ST_Usb_Drv;

#endif /* PLATFORM_STM32 */
