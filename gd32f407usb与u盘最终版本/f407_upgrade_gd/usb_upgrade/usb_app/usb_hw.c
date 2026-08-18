/**
  * @file    usb_hw.c
  * @brief   GD32F407 USBFS 底层硬件钩子
  *
  * 实现 GD32F4xx_usb_library 的 drv_usb_hw.h 中声明的应用层钩子：
  *   usb_rcu_config / usb_gpio_config / usb_intr_config
  *   usb_udelay / usb_mdelay / usb_timer_init / system_clk_config_stop
  *
  * 同时提供 USBFS_IRQHandler（轮询模式下调用 usbd_isr）。
  *
  * 所有板级引脚/时钟/中断参数均来自 board_pin_config.h，延时函数来自 plat_tick.h。
  */

#include "gd32f4xx.h"
#include "drv_usb_hw.h"
#include "drv_usbd_int.h"
#include "drv_usbh_int.h"
#include "usbd_core.h"
#include "usb_msc_device.h"
#include "usb_mode.h"            /* USB_Mode_Get 判断 HOST/DEVICE */
#include "board_pin_config.h"    /* USB 引脚/时钟/中断宏定义 */
#include "plat_tick.h"           /* plat_delay_us / plat_delay_ms */

/* USBFS 全局句柄（定义在 usb_msc_device.c / usb_host_task.c） */
extern usb_core_driver usb_device;
extern usb_core_driver usbh_core;

/**
  * @brief  USB 外设时钟使能
  */
void usb_rcu_config(void)
{
    /* 复位 USBFS，确保状态干净 */
    rcu_periph_reset_enable(USB_PERIPH_RST);
    rcu_periph_reset_disable(USB_PERIPH_RST);

    /* 使能 USBFS 外设时钟 */
    rcu_periph_clock_enable(USB_PERIPH_CLK);

    /* 使能 USB D-/D+ 所在 GPIO 端口时钟 */
    rcu_periph_clock_enable(USB_GPIO_CLK);
}

/**
  * @brief USB D+ / D- 引脚配置 (AF 复用)
  */
void usb_gpio_config(void)
{
    /* DM/DP 开漏 + 上拉，引脚与 AF 由 board_pin_config.h 集中配置 */
    gpio_mode_set(USB_DM_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, USB_DM_PIN | USB_DP_PIN);
    gpio_output_options_set(USB_DM_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, USB_DM_PIN | USB_DP_PIN);
    gpio_af_set(USB_DM_PORT, USB_GPIO_AF, USB_DM_PIN | USB_DP_PIN);
}

/**
 * @brief USBFS 硬件中断使能——本工程以轮询模式处理，此处仅做 NVIC 配置兜底
 */
void usb_intr_config(void)
{
    nvic_irq_enable(USB_IRQn, USB_IRQ_PRIORITY, USB_IRQ_SUBPRIORITY);
}

/**
 * @brief 备份系统时钟（当前无需，占位空实现）
 */
void system_clk_config_stop(void)
{
}

/**
 * @brief 定时器初始化——延时函数已基于 plat_tick 实现，
 *        usb_udelay/usb_mdelay 已委托 plat_delay_us/ms，此处空实现保证链接。
 */
void usb_timer_init(void)
{
}

#ifdef USE_HOST_MODE
/**
 * @brief 配置 VBUS 电源开关（Host 模式需要给 U 盘供电）
 *
 * 本板硬件 VBUS 直连 USB_5V（常开，见 usb_bsp.c 注释），
 * 无电源开关引脚，此处空实现保证链接；usb_vbus_drive 同样为空。
 */
void usb_vbus_config(void)
{
}

/**
 * @brief 驱动 VBUS 电源开关
 */
void usb_vbus_drive(uint8_t state)
{
    (void)state;   /* VBUS 常开，无需软件控制 */
}
#endif /* USE_HOST_MODE */

/**
 * @brief 微秒级延时
 */
void usb_udelay(const uint32_t usec)
{
    plat_delay_us((uint32_t)usec);
}

/**
 * @brief 毫秒级延时
 */
void usb_mdelay(const uint32_t msec)
{
    plat_delay_ms((uint32_t)msec);
}

/**
 * @brief USBFS 硬件中断处理
 *        HOST 模式：调用 usbh_isr 处理 Host 事件（中断模式）
 *        DEVICE 模式：NVIC 已禁用（轮询模式），此函数正常情况下不会触发，
 *        若意外使能中断，调用 usbd_isr 处理并清中断。
 */
void USBFS_IRQHandler(void)
{
    if (USB_Mode_Get() == USB_MODE_HOST) {
        usbh_isr(&usbh_core);
    } else {
        usbd_isr(&usb_device);
    }
}
