/**
  * @file    usb_msc_device.c
  * @brief   USB MSC 设备封装 — 对接 GD32F4xx_usb_library
  *
  * 暴露 usb_msc_device_init / usb_msc_is_configured / usb_msc_device_deinit
  * 供 usb_task.c 调用，内部调用 usbd_init / usbd_connect / usbd_disconnect。
  *
  * 关键设计：usbd_init 只调用一次（首次初始化），后续断开/重连仅使用
  * usbd_disconnect / usbd_connect 控制 D+ 上拉。
  * USB 事件处理采用主循环轮询 usbd_isr（NVIC 中断保持关闭），
  * 与 ST 版轮询模式行为一致。
  */

#include "usb_msc_device.h"
#include "config.h"
#include "usbd_core.h"
#include "usbd_conf.h"
#include "usbd_msc_core.h"
#include "usbd_msc_mem.h"
#include "usb_conf.h"
#include "drv_usb_core.h"
#include "drv_usb_hw.h"
#include "drv_usbd_int.h"
#include "delay.h"

/* 全局 USBFS 设备句柄（usb_hw.c 的 USBFS_IRQHandler 引用） */
usb_core_driver usb_device;

/* USB 核心是否已完成初始化（usbd_init 只允许调用一次） */
static uint8_t usb_core_inited = 0;

/* 设备状态（0=未配置, 1=已配置） */
static volatile uint8_t g_usb_configured = 0;

/**
 * @brief USB 轮询处理 — 在主循环中调用，替代中断
 *
 * 判定"已连接"的逻辑：
 *   - USBD_CONFIGURED                     → 已连接（枚举完成）
 *   - USBD_SUSPENDED 且 backup==CONFIGURED → 仍视为已连接
 *     这是 PC 选择性挂起（空闲停发 SOF）：D+ 上拉还在，主机 resume 后
 *     cur_status 自动恢复 CONFIGURED，设备不需要做任何事。
 *     若把挂起当拔出，usb_task 会 deinit+reconnect，Windows 每次重连
 *     都重新初始化卷 → 表现为"插入提示格式化"。
 *   - USBD_SUSPENDED 且 backup==DEFAULT    → 未连接
 *     未插线时 D+/D- 浮空产生的假挂起，backup 是 DEFAULT，不能算连接，
 *     否则固件会在未插线时误报 "USB in"。
 *   - SOF 超时（200ms 无 SOF）            → 主机已物理拔出
 */
void USB_Poll_Handler(void)
{
  usbd_isr(&usb_device);

  uint8_t st = usb_device.dev.cur_status;

  /* 与 STM32 版保持一致：
     CONFIGURED → 已连接
     SUSPENDED  → 主机拔出或挂起 → 标记为未连接
       （usb_task.c 有 1s 去抖，短暂挂起不会误触发重连）
     其他状态  → 未连接 */
  g_usb_configured = (st == (uint8_t)USBD_CONFIGURED) ? 1U : 0U;
}

void usb_msc_device_init(void)
{
  if (usb_core_inited) {
    /*
     * 核心已初始化 — 仅重新连接 D+ 上拉，不重复调用 usbd_init。
     *
     * 主机检测到 D+ 上拉后会发送 USB Reset，库的 Reset 处理会：
     *   1. 清除设备地址
     *   2. 重置所有端点
     *   3. 重新调用 MSC class Init（通过 SET_CONFIGURATION）
     * 因此无需手动重置 MSC 层状态。
     */
    LOGD("[MSC] Reconnect (Connect only)\r\n");

    /* 确保 NVIC 仍然关闭 — 用轮询模式处理 */
    plat_usb_nvic_disable();

    /* 重新使能 D+ 内部上拉 */
    usbd_connect(&usb_device);

    /* 从 DEFAULT 状态开始：清掉可能残留的 CONFIGURED/backup 状态，
       避免拔线后浮空 D+/D- 产生的假唤醒把设备误判为“已配置”。 */
    usb_device.dev.cur_status = (uint8_t)USBD_DEFAULT;
    usb_device.dev.backup_status = (uint8_t)USBD_DEFAULT;
    LOGD("[MSC] Reconnect done\r\n");
    return;
  }

  /* ════ 首次初始化（仅执行一次） ════ */
  LOGD("[MSC] usbd_init start\r\n");

  /* 硬件钩子：时钟 / GPIO */
  usb_rcu_config();
  usb_gpio_config();

  /* 初始化 USBFS 设备模式并加载 MSC 类驱动 */
  usbd_init(&usb_device, USB_CORE_ENUM_FS, &msc_desc, &msc_class);

  LOGD("[MSC] usbd_init done\r\n");

  /* 禁用 USB 硬件中断 — 改用主循环轮询 (USB_Poll_Handler) 处理 USB 事件。
     usbd_init 内部配置了核心全局中断，此处关闭 NVIC 使轮询模式生效。 */
  plat_usb_nvic_disable();

  /* usbd_init 内部已调用 usbd_connect（D+ 上拉），无需重复 */
  LOGD("[MSC] connected (via usbd_init)\r\n");

  usb_core_inited = 1;
  LOGD("[MSC] Init done (core_inited=1)\r\n");
}

void usb_msc_device_deinit(void)
{
  /* 断开 D+ 上拉，让电脑识别为拔出 */
  usbd_disconnect(&usb_device);

  /* 清状态，防止拔线后假唤醒把 cur_status 拉回 CONFIGURED */
  usb_device.dev.cur_status = (uint8_t)USBD_DEFAULT;
  usb_device.dev.backup_status = (uint8_t)USBD_DEFAULT;

  plat_usb_nvic_disable();
  g_usb_configured = 0;

  LOGD("[MSC] Disconnect done (core still inited)\r\n");
}

uint8_t usb_msc_is_configured(void)
{
  return g_usb_configured;
}
