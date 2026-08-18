/**
 * @file usb_host_task.h
 * @brief USB Host 模式任务（U盘读取升级）
 *
 * 工作流程：
 *   1. USBH_Init → 禁用中断（轮询模式）→ 注册 U盘升级来源
 *   2. 主循环：USBH_OTG_ISR_Handler → USBH_Process → 升级检测
 *   3. U盘枚举成功 → 挂载 FatFs → 检测 firmware.bin → 升级 → 复位
 */

#ifndef __USB_HOST_TASK_H
#define __USB_HOST_TASK_H

/**
 * @brief 初始化 USB Host 模式
 *        USBH_Init + 禁用中断 + 注册升级来源
 */
void USB_Host_Task_Init(void);

/**
 * @brief USB Host 主循环任务
 *        USBH_Process + U盘枚举检测 + 升级流程
 */
void USB_Host_Task_Run(void);

/**
 * @brief USB Host 中断轮询处理
 *        在主循环中调用，替代硬件中断
 */
void USB_Host_Poll_Handler(void);

/**
 * @brief 执行 U盘升级流程
 *        从 USBH_USR_MSC_Application 回调中调用（在 USBH_Process 内部），
 *        与正点原子 Host 例程结构一致：U盘枚举成功后直接挂载 FatFs、
 *        检测 firmware.bin、执行升级。
 *        升级成功后切换 DEVICE 模式并复位；失败则返回。
 */
void USB_Host_Do_Upgrade(void);

#endif /* __USB_HOST_TASK_H */
