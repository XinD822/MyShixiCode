/**
 * @file usb_upgrade.h
 * @brief USB 拖拽升级模块 — 对外唯一接口
 *
 * 使用方法（在你的 main.c 中加 3 行）：
 *
 *   #include "usb_upgrade.h"
 *   #include "tick_drv.h"      // 主循环需要调 Tick_Poll()
 *
 *   int main(void)
 *   {
 *       // ... 你原来的初始化 ...
 *       USB_Upgrade_Init();
 *
 *       while (1)
 *       {
 *           Tick_Poll();            // 轮询 TIM3 累加毫秒计数
 *           USB_Upgrade_Run();
 *       }
 *   }
 *
 * 将来加 Host 模式时：
 *   #include "usb_mode.h"
 *   USB_Mode_Set(USB_MODE_HOST);   // 在 USB_Upgrade_Init 之前调用
 */

#ifndef __USB_UPGRADE_H
#define __USB_UPGRADE_H

#include <stdint.h>

/**
 * @brief 初始化升级模块
 *
 * 内部自动完成：Flash 初始化、FatFS 挂载、USB MSC 初始化、升级模块初始化
 * 调用一次即可，放在 main 初始化阶段
 */
void USB_Upgrade_Init(void);

/**
 * @brief 运行升级任务
 *
 * 每次主循环调用一次，内部处理：
 * - USB 连接/断开检测
 * - 固件文件检测
 * - 固件搬运到 Slot A
 * - 设升级标志并重启
 *
 * 放在 while(1) 中
 */
void USB_Upgrade_Run(void);

#endif /* __USB_UPGRADE_H */
