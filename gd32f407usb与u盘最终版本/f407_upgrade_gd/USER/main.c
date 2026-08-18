#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "usb_upgrade.h"     /* 升级模块对外接口 */
#include "usb_mode.h"        /* USB_Mode_Get / USB_Mode_ToggleFlag */
#include "usb_host_task.h"   /* USB_Host_Poll_Handler */
#include "config.h"          /* plat_* 接口 + LOGD 宏 */

extern void USB_Poll_Handler(void);  /* 定义在 usb_msc_device.c，轮询处理USB事件 */

int main(void)
{
	nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);

	/* ════ 向量表重定位到 APP 区 ════
	 * GD 的 system_gd32f4xx.c SystemInit 不像 ST 那样设置 VTOR，
	 * 此处显式设置，保证独立烧录 / Bootloader 跳转后中断向量正确。 */
	plat_set_vtor(APP_ADDR);

	delay_init(168);
	LED_Init();
	KEY_Init();

	/* ════ 恢复模式：KEY0 长按开机 → 强制 DEVICE 模式 ════
	 * 如果 W25Q128 中存有 HOST 标志导致 HOST 模式启动异常，
	 * 按住 KEY0 上电可绕过 Flash 标志，强制以 DEVICE 模式启动。
	 *
	 * 延时500ms后再检测：避免模式切换跳转时用户还没松开 KEY0，
	 * 导致误触恢复模式。 */
	for (volatile uint32_t i = 0; i < 5000000; i++) __NOP();  /* ≈500ms */
	if (KEY0 == 1) {
		USB_Mode_ForceDevice();
	}

	USB_Upgrade_Init();       /* 初始化升级模块（Tick/Flash/USB/FatFS） */
	                          /* 内部自动读 Flash 标志位选择 DEVICE/HOST 模式 */

	uint32_t led_tick = 0;
	uint32_t key_hold_start = 0;   /* KEY0 长按计时起点 */
	uint32_t boot_time = 0;        /* 记录启动时间 */

	while(1)
	{
		/* 新版本基于 SysTick 中断驱动 tick，无需 Tick_Poll() */

		/* 按模式分发 USB 中断轮询 */
		if (USB_Mode_Get() == USB_MODE_DEVICE) {
			USB_Poll_Handler();   /* Device 模式轮询USB事件 */
		} else {
			USB_Host_Poll_Handler();  /* Host 模式轮询USB事件 */
		}

		USB_Upgrade_Run();    /* 运行升级任务（内部按模式分发） */

		/* 按键切换 USB 模式：需要长按 KEY0 至少 1.5 秒才触发，
		 * 防止电气干扰导致误触发 */
		{
			uint32_t now = plat_get_tick_ms();
			if (boot_time == 0) boot_time = now;

			/* 启动后 3 秒内不扫描按键（避免上电瞬态干扰） */
			if (now - boot_time > 3000) {
				if (KEY0 == 1) {
					/* KEY0 按下（高电平有效） */
					if (key_hold_start == 0) {
						key_hold_start = now;
					} else if (now - key_hold_start >= 1500) {
				/* 持续按住超过 1.5 秒，触发模式切换 */
					LOGD("[KEY] KEY0 long press detected, toggling mode\r\n");
					USB_Mode_ToggleFlag();
					LOGD("[KEY] flag written, resetting...\r\n");

					/* 等待 UART 发送完成 + Flash 写入完成 */
					plat_delay_ms(500);

			/* ── 与升级路径完全一致的复位序列 ──
			 * 升级路径(usb_host_task.c)复位成功，切换路径照搬其做法：
			 *   1) 先回读验证标志已写入（与升级路径的 Final check 一致）
			 *   2) plat_flash_reset() 复位 W25Q128 内部状态机（不擦数据）
			 *   3) plat_system_reset() USB硬件复位 + 跳转 bootloader 冷启动 */
			{
				uint32_t verify = 0;
				plat_flash_read((uint8_t *)&verify, USB_MODE_FLAG_ADDR, 4);
				LOGD("[KEY] verify flag=0x%08X\r\n", (unsigned int)verify);
			}
			plat_flash_reset();
			LOGD("[KEY] resetting...\r\n");
			plat_system_reset();
					}
				} else {
					key_hold_start = 0;  /* KEY0 释放，重置计时 */
				}
			}
		}

		/* LED 心跳：800ms 翻转（使用 board_pin_config.h 的引脚宏） */
		{
			static uint8_t led_on = 0;
			uint32_t now = plat_get_tick_ms();
			if (now - led_tick >= 300) {
				led_tick = now;
				led_on = !led_on;
#if LED0_ACTIVE_LOW
				if (led_on)
					gpio_bit_reset(LED0_PORT, LED0_PIN);
				else
					gpio_bit_set(LED0_PORT, LED0_PIN);
#else
				if (led_on)
					gpio_bit_set(LED0_PORT, LED0_PIN);
				else
					gpio_bit_reset(LED0_PORT, LED0_PIN);
#endif
			}
		}
	}
}
