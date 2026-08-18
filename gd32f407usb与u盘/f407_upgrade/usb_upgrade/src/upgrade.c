/**
 * @file upgrade.c
 * @brief 升级状态机实现
 *
 * 零硬件依赖。通过 FlashService + UpgradeSource_t 操作。
 */

#include "upgrade.h"
#include "upgrade_source.h"
#include "upgrade_config.h"
#include "firmware_check.h"
#include "flash_service.h"
#include "config.h"
#include <string.h>

static UpgradeState  g_state = UPGRADE_IDLE;
static UpgradeResult g_last_err = UPGRADE_OK;
static uint8_t fw_buf[FIRMWARE_BUF_SIZE];
static uint32_t g_fw_size = 0;

void Upgrade_Init(void)
{
    g_state = UPGRADE_IDLE;
    g_last_err = UPGRADE_OK;
    g_fw_size = 0;
}

UpgradeResult Upgrade_Check(void)
{
    const UpgradeSource_t *src = UpgradeSource_Get();
    if (src == NULL) {
        g_last_err = UPGRADE_ERR_NO_FILE;
        return UPGRADE_ERR_NO_FILE;
    }

    g_state = UPGRADE_CHECKING;

    if (!src->detect()) {
        g_state = UPGRADE_IDLE;
        g_last_err = UPGRADE_ERR_NO_FILE;
        return UPGRADE_ERR_NO_FILE;
    }

    g_state = UPGRADE_READY;
    g_last_err = UPGRADE_OK;
    return UPGRADE_OK;
}

UpgradeResult Upgrade_Execute(void)
{
    const UpgradeSource_t *src = UpgradeSource_Get();
    FIL file;
    UINT br;
    uint32_t total_read = 0;
    uint32_t flash_addr = FIRMWARE_SLOT_A_ADDR;
    uint32_t fw_size = 0;

    DBG_PRINTF("[UPG] Execute: state=%d\r\n", (int)g_state);

    if (g_state != UPGRADE_READY) {
        g_last_err = UPGRADE_ERR_STATE;
        DBG_PRINTF("[UPG] ERR: state not READY\r\n");
        return UPGRADE_ERR_STATE;
    }

    if (src == NULL || src->open == NULL) {
        g_last_err = UPGRADE_ERR_NO_FILE;
        DBG_PRINTF("[UPG] ERR: no source\r\n");
        return UPGRADE_ERR_NO_FILE;
    }

    g_state = UPGRADE_BURNING;

    /* 打开固件文件 */
    FRESULT open_res = src->open(&file, &fw_size);
    DBG_PRINTF("[UPG] open result=%d, fw_size=%lu\r\n", (int)open_res, fw_size);
    if (open_res != FR_OK) {
        g_state = UPGRADE_IDLE;
        g_last_err = UPGRADE_ERR_BURN;
        return UPGRADE_ERR_BURN;
    }

    /* 擦除 Slot A */
    uint32_t blocks = (fw_size + 65535) / 65536;
    if (blocks == 0) blocks = 1;
    DBG_PRINTF("[UPG] erasing %d blocks at 0x%06X\r\n", (int)blocks, (unsigned int)FIRMWARE_SLOT_A_ADDR);
    for (uint32_t i = 0; i < blocks; i++) {
        FlashService_EraseBlock(FIRMWARE_SLOT_A_ADDR + i * 65536);
    }
    DBG_PRINTF("[UPG] erase done\r\n");

    /* 读取固件 → 写入 Slot A */
    uint32_t loop_cnt = 0;
    while (1) {
        FRESULT rd_res = src->read(&file, fw_buf, FIRMWARE_BUF_SIZE, &br);
        if (rd_res != FR_OK) {
            DBG_PRINTF("[UPG] read err=%d at %lu\r\n", (int)rd_res, total_read);
            break;
        }
        if (br == 0) break;
        FlashService_Write(fw_buf, flash_addr, br);
        flash_addr += br;
        total_read += br;
        if (++loop_cnt % 16 == 0) {
            DBG_PRINTF("[UPG] written %lu/%lu\r\n", total_read, fw_size);
        }
        if (br < FIRMWARE_BUF_SIZE) break;
    }
    src->close(&file);
    DBG_PRINTF("[UPG] file closed, total_read=%lu\r\n", total_read);

    /* 刷写缓存到硬件 Flash，确保校验读到的是刚写入的数据 */
    FlashService_FlushCache();

    /* 校验 */
    uint8_t verify_header[4];
    FlashService_Read(verify_header, FIRMWARE_SLOT_A_ADDR, 4);
    DBG_PRINTF("[UPG] header: %02X %02X %02X %02X\r\n",
               verify_header[0], verify_header[1],
               verify_header[2], verify_header[3]);
    if (Firmware_CheckHeader(verify_header)) {
        DBG_PRINTF("[UPG] Verify FAIL\r\n");
        g_state = UPGRADE_ERROR;
        g_last_err = UPGRADE_ERR_BURN;
        return UPGRADE_ERR_BURN;
    }

    g_state = UPGRADE_DONE;
    g_fw_size = total_read;

    /* 设置升级标志，Bootloader 重启后据此搬运固件 */
    DBG_PRINTF("[UPG] setting flag...\r\n");
    Upgrade_SetFlag(UPGRADE_FLAG_YES, total_read);

    DBG_PRINTF("[UPG] %dB written, flag set, will reset\r\n", total_read);
    return UPGRADE_OK;
}

void Upgrade_SetFlag(uint32_t flag, uint32_t size)
{
    /* 擦除配置区扇区（flag、size、state 都在 CONFIG_AREA_ADDR 的前 16 字节内） */
    FlashService_EraseSector(CONFIG_AREA_ADDR);
    FlashService_InvalidateCache();
    /* 写入 flag、size、state（FlashService 内部缓存会合并多次写入） */
    uint32_t burning = UPGRADE_STATE_BURNING;
    FlashService_Write((uint8_t *)&flag, UPGRADE_FLAG_ADDR, sizeof(flag));
    FlashService_Write((uint8_t *)&size, FIRMWARE_SIZE_ADDR, sizeof(size));
    FlashService_Write((uint8_t *)&burning, UPGRADE_STATE_ADDR, sizeof(burning));
    FlashService_FlushCache();
    DBG_PRINTF("[UPG] flag=0x%08X, size=%d, state=BURNING\r\n",
               (unsigned int)flag, (unsigned int)size);
}

uint32_t Upgrade_GetFlag(void)
{
    uint32_t flag = 0;
    FlashService_Read((uint8_t *)&flag, UPGRADE_FLAG_ADDR, sizeof(flag));
    return flag;
}

UpgradeState Upgrade_GetState(void)
{
    return g_state;
}

UpgradeResult Upgrade_GetLastError(void)
{
    return g_last_err;
}

uint32_t Upgrade_GetFirmwareSize(void)
{
    return g_fw_size;
}
