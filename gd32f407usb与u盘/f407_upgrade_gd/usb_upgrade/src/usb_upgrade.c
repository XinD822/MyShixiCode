/**
 * @file usb_upgrade.c
 * @brief 升级模块入口 — 封装所有内部逻辑
 */

#include "usb_upgrade.h"
#include "config.h"

static void Upgrade_StateCheck(void)
{
    uint32_t state;
    uint32_t saved_size = 0;
    FlashService_Read((uint8_t *)&state, UPGRADE_STATE_ADDR, 4);
    /* 保留固件大小：清升级标志时不能把 size 清零，否则 Bootloader
       之后无法从 Slot B 回滚（RestoreSlotToApp 依赖 size 非 0） */
    FlashService_Read((uint8_t *)&saved_size, FIRMWARE_SIZE_ADDR, 4);

    if (state == UPGRADE_STATE_DONE) {
        Upgrade_SetFlag(UPGRADE_FLAG_NO, saved_size);
        uint32_t confirmed = UPGRADE_STATE_CONFIRMED;
        FlashService_Write((uint8_t *)&confirmed, UPGRADE_STATE_ADDR, sizeof(confirmed));
        FlashService_FlushCache();
    }

    if (state == UPGRADE_STATE_CONFIRMED) {
        Upgrade_SetFlag(UPGRADE_FLAG_NO, saved_size);
        uint32_t none = UPGRADE_STATE_NONE;
        FlashService_Write((uint8_t *)&none, UPGRADE_STATE_ADDR, sizeof(none));
        FlashService_FlushCache();
    }
}

void USB_Upgrade_Init(void)
{
    LOGD("\r\n[UPGR] Init start (v2 VTOR=0x10000)\r\n");

    LOGD("[UPGR] plat_tick_init...\r\n");
    plat_tick_init();
    LOGD("[UPGR] plat_tick_init OK\r\n");

    LOGD("[UPGR] plat_flash_init...\r\n");
    plat_flash_init();
    LOGD("[UPGR] plat_flash_init OK\r\n");

    uint16_t flash_id = plat_flash_read_id();
    LOGD("[UPGR] Flash ID=0x%04X, %s %dMB\r\n",
         flash_id, FLASH_NAME, FLASH_CAPACITY / 1024 / 1024);

    LOGD("[UPGR] FlashService_Init...\r\n");
    FlashService_Init();
    LOGD("[UPGR] FlashService_Init OK\r\n");

    LOGD("[UPGR] Upgrade_StateCheck...\r\n");
    Upgrade_StateCheck();
    LOGD("[UPGR] Upgrade_StateCheck OK\r\n");

    /* 从 Flash 配置区读取模式标志（上电二选一） */
    USB_Mode_Set(USB_Mode_ReadFlag());

    LOGD("[UPGR] USB_Mode_Init...\r\n");
    USB_Mode_Init();
    LOGD("[UPGR] USB_Mode_Init OK\r\n");

    LOGD("[UPGR] Init done\r\n");
}

void USB_Upgrade_Run(void)
{
    USB_Mode_Run();
}
