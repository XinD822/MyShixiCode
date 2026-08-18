#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "usb_upgrade.h"     /* 升级模块对外接口 */
#include "usb_mode.h"        /* USB_Mode_Get / USB_Mode_ToggleFlag */
#include "usb_host_task.h"   /* USB_Host_Poll_Handler */
#include "board_config.h"    /* 板级配置（芯片选择、引脚等） */
#include "tick_drv.h"        /* Tick_Poll() / Tick_GetMs() */
#include "config.h"          /* DBG_PRINTF 宏定义 */
#include "platform.h"        /* Platform_USB_ClockDisable / Platform_USB_NVICDisable */
#include "w25q128_drv.h"     /* Flash_Reset / Flash_Read */
#include "upgrade_config.h"  /* USB_MODE_FLAG_ADDR */

extern void USB_Poll_Handler(void);  /* 定义在 usbd_usr.c，轮询处理USB事件 */

int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	delay_init(168);
	LED_Init();
	KEY_Init();

	/* ════ 恢复模式：KEY0 长按开机 → 强制 DEVICE 模式 ════
	 * 如果 W25Q128 中存有 HOST 标志导致 HOST 模式启动异常，
	 * 按住 KEY0 上电可绕过 Flash 标志，强制以 DEVICE 模式启动。
	 * 进入 DEVICE 模式后可正常使用，再长按 KEY0 切换回 HOST。 */
	if (KEY0 == 1) {
		USB_Mode_ForceDevice();
		/* 快闪 LED0 提示恢复模式已激活 */
		for (int i = 0; i < 6; i++) {
			LED0 = !LED0;
			for (volatile uint32_t j = 0; j < 200000; j++) __NOP();
		}
	}

	USB_Upgrade_Init();       /* 初始化升级模块（串口/Flash/Tick/USB/FatFS） */
	                          /* 内部自动读 Flash 标志位选择 DEVICE/HOST 模式 */

	uint32_t led_tick = 0;
	uint32_t key_hold_start = 0;   /* KEY0 长按计时起点 */
	uint32_t boot_time = 0;        /* 记录启动时间 */

	while(1)
	{
		Tick_Poll();          /* 轮询TIM3累加毫秒计数 */

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
			uint32_t now = Tick_GetMs();
			if (boot_time == 0) boot_time = now;

			/* 启动后 3 秒内不扫描按键（避免上电瞬态干扰） */
			if (now - boot_time > 3000) {
				if (KEY0 == 1) {
					/* KEY0 按下（高电平有效） */
					if (key_hold_start == 0) {
						key_hold_start = now;
					} else if (now - key_hold_start >= 1500) {
				/* 持续按住超过 1.5 秒，触发模式切换 */
					DBG_PRINTF("[KEY] KEY0 long press detected, toggling mode\r\n");
					USB_Mode_ToggleFlag();
					DBG_PRINTF("[KEY] flag written, resetting...\r\n");

					/* 等待 UART 发送完成 + Flash 写入完成 */
					for (volatile uint32_t i = 0; i < 500000; i++) __NOP();

			/* ── 与升级路径完全一致的复位序列 ──
			 * 升级路径(usb_host_task.c)复位成功，切换路径照搬其做法：
			 *   1) 先回读验证标志已写入（与升级路径的 Final check 一致）
			 *   2) Flash_Reset() 复位 W25Q128 内部状态机（不擦数据）
			 *   3) Platform_SystemReset() 带 IWDG 兜底自动复位
			 * 注意：Flash_Reset 只复位 W25Q128 的内部状态机/读指针，
			 * 不会擦除已写入的扇区数据，模式标志安全保留。 */
			{
				uint32_t verify = 0;
				Flash_Read((uint8_t *)&verify, USB_MODE_FLAG_ADDR, 4);
				DBG_PRINTF("[KEY] verify flag=0x%08X\r\n", (unsigned int)verify);
			}
			Flash_Reset();
			DBG_PRINTF("[KEY] resetting...\r\n");
			Platform_SystemReset();
					}
				} else {
					key_hold_start = 0;  /* KEY0 释放，重置计时 */
				}
			}
		}

		/* LED 心跳：800ms 翻转 */
		{
			uint32_t now = Tick_GetMs();
			if (now - led_tick >= 800) {
				led_tick = now;
				LED0 = !LED0;
			}
		}
	}
}
