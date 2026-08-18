/**
 * @file usb_upgrade.c
 * @brief 升级模块入口 — 封装所有内部逻辑
 */

#include "usb_upgrade.h"
#include "config.h"
#include "usb_msc_device.h"
#include "usb_task.h"

#ifdef USB_UPGRADE_USE_UART
#include <stdio.h>
#include <stdarg.h>

void USB_Upgrade_Printf(const char *fmt, ...)
{
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    Debug_UART_SendString(buf);
}
#endif

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
#ifdef USB_UPGRADE_USE_UART
    Debug_UART_Init(USART_BAUDRATE);
#endif

    DBG_PRINTF("\r\n[UPGR] Init start (v2 VTOR=0x10000)\r\n");

    DBG_PRINTF("[UPGR] Tick_Init...\r\n");
    Tick_Init();
    DBG_PRINTF("[UPGR] Tick_Init OK\r\n");

    DBG_PRINTF("[UPGR] Flash_Init...\r\n");
    Flash_Init();
    DBG_PRINTF("[UPGR] Flash_Init OK\r\n");

    uint16_t flash_id = Flash_ReadID();
    DBG_PRINTF("[UPGR] Flash ID=0x%04X, %s %dMB\r\n",
               flash_id, FLASH_NAME, FLASH_CAPACITY / 1024 / 1024);

    DBG_PRINTF("[UPGR] FlashService_Init...\r\n");
    FlashService_Init();
    DBG_PRINTF("[UPGR] FlashService_Init OK\r\n");

    DBG_PRINTF("[UPGR] Upgrade_StateCheck...\r\n");
    Upgrade_StateCheck();
    DBG_PRINTF("[UPGR] Upgrade_StateCheck OK\r\n");

    /* 从 Flash 配置区读取模式标志（上电二选一） */
    USB_Mode_Set(USB_Mode_ReadFlag());

    DBG_PRINTF("[UPGR] USB_Mode_Init...\r\n");
    USB_Mode_Init();
    DBG_PRINTF("[UPGR] USB_Mode_Init OK\r\n");

    DBG_PRINTF("[UPGR] Init done\r\n");
}

void USB_Upgrade_Run(void)
{
    USB_Mode_Run();
}
