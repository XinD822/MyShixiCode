/**
 * @file usb_upgrade.c
 * @brief 升级模块入口 — 封装所有内部逻辑
 *
 * 对外只暴露 USB_Upgrade_Init() 和 USB_Upgrade_Run()
 * 内部初始化顺序、状态管理、FatFS 挂载等全部在此处理
 */

#include "usb_upgrade.h"
#include "config.h"
#include "usb_msc_device.h"
#include "usb_task.h"

/* ──── 升级状态处理（启动时检查上次升级结果） ──── */
static void Upgrade_StateCheck(void)
{
    uint32_t state;
    FlashService_Read((uint8_t *)&state, UPGRADE_STATE_ADDR, 4);

    if (state == UPGRADE_STATE_DONE) {
        Upgrade_SetFlag(UPGRADE_FLAG_NO, 0);
        uint32_t confirmed = UPGRADE_STATE_CONFIRMED;
        FlashService_Write((uint8_t *)&confirmed, UPGRADE_STATE_ADDR, sizeof(confirmed));
        FlashService_FlushCache();
    }

    if (state == UPGRADE_STATE_CONFIRMED) {
        Upgrade_SetFlag(UPGRADE_FLAG_NO, 0);
        uint32_t none = UPGRADE_STATE_NONE;
        FlashService_Write((uint8_t *)&none, UPGRADE_STATE_ADDR, sizeof(none));
        FlashService_FlushCache();
    }
}

void USB_Upgrade_Init(void)
{
    /* 初始化 Tick */
    HAL_Tick->init();

    /* 初始化调试串口（可选） */
#ifdef USB_UPGRADE_USE_UART
    HAL_Uart->init(USART_BAUDRATE);
#endif

    /* Flash 硬件初始化（GPIO / SPI 配置） */
    HAL_Flash->init();

    /* Flash 服务初始化（互斥锁 + 缓存） */
    FlashService_Init();

    /* 处理上次升级结果 */
    Upgrade_StateCheck();

    /* USB 升级任务初始化（挂载 FatFS + 初始化 USB + 注册来源） */
    USB_Task_Init();

    DBG_PRINTF("[USB] Upgrade module ready\r\n");
}

void USB_Upgrade_Run(void)
{
    USB_Task_Run();
}
