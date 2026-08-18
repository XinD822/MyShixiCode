/**
 * @file plat_reset_gd32.c
 * @brief GD32F407 复位/中断/USB时钟 实现 plat_reset 接口
 *        从原 platform.c 提取，引脚改为引用 board_pin_config.h
 */
#include "plat_reset.h"
#include "plat_tick.h"          /* plat_tick_deinit: 跳转前关 tick 中断 */
#include "board_pin_config.h"

void plat_disable_irq(void)  { __disable_irq(); }
void plat_enable_irq(void)   { __set_PRIMASK(0); }
void plat_set_msp(uint32_t addr) { __set_MSP(addr); }

void plat_set_vtor(uint32_t addr)
{
    SCB->VTOR = addr;
}

void plat_system_reset(void)
{
    /* ──── GD32F407 复位方案（与原 platform.c 完全一致） ────
     *
     * GD32 的 USBFS 在 Host 模式下会阻塞 NVIC_SystemReset 和 FWDGT。
     * 解决方案：彻底关闭 USB → 清中断 → 跳转到 bootloader 入口。
     * Bootloader 的 startup 会重新初始化所有外设，等同于冷启动。 */

    /* 0) 关闭 tick 中断源（TIMER2 / 外部注入）
     * 必须最先做：若 tick 中断仍使能，跳转后可能命中 Bootloader
     * startup 里默认的死循环 handler（B .），导致卡死。 */
    plat_tick_deinit();

    /* 1) 关 USBFS 中断 + 全局中断 */
    nvic_irq_disable(USB_IRQn);
    __disable_irq();

    /* 2) 硬件复位 USBFS 外设 */
    RCU_AHB2RST |=  USB_PERIPH_RST;
    for (volatile uint32_t i = 0; i < 1000; i++) __NOP();
    RCU_AHB2RST &= ~USB_PERIPH_RST;
    for (volatile uint32_t i = 0; i < 1000; i++) __NOP();

    /* 3) 关 USBFS 时钟 */
    rcu_periph_clock_disable(USB_PERIPH_CLK);

    /* 4) 清除所有 NVIC pending 中断 */
    {
        int i;
        for (i = 0; i < 8; i++) {
            NVIC->ICPR[i] = 0xFFFFFFFF;
        }
    }

    /* 5) 跳转到 bootloader 入口（0x08000000）
     * 保持关中断跳转：不 __enable_irq()，中断由 Bootloader 的
     * startup 自行初始化，避免跳转窗口内任何中断踩到死循环 handler。 */
    {
        uint32_t bl_sp    = *(volatile uint32_t *)0x08000000;
        uint32_t bl_entry = *(volatile uint32_t *)0x08000004;

        __set_MSP(bl_sp);
        SCB->VTOR = 0x08000000;

        void (*reset_handler)(void) = (void (*)(void))bl_entry;
        reset_handler();
    }

    while (1) __NOP();
}

void plat_nvic_priority_group(uint32_t group)
{
    nvic_priority_group_set(group);
}

void plat_usb_clock_enable(void)
{
    rcu_periph_clock_enable(USB_GPIO_CLK);
    rcu_periph_clock_enable(USB_PERIPH_CLK);
}

void plat_usb_clock_disable(void)
{
    rcu_periph_clock_disable(USB_PERIPH_CLK);
}

void plat_usb_nvic_enable(void)
{
    nvic_irq_enable(USB_IRQn, USB_IRQ_PRIORITY, USB_IRQ_SUBPRIORITY);
}

void plat_usb_nvic_disable(void)
{
    nvic_irq_disable(USB_IRQn);
}
