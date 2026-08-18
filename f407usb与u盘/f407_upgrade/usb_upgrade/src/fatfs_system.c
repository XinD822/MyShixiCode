/**
 * @file fatfs_system.c
 * @brief FatFS 系统封装
 */

#include "fatfs_system.h"
#include "config.h"

static FATFS fatfs_vol;

FRESULT FatFs_Mount(void)
{
    FRESULT res = f_mount(&fatfs_vol, "0:", 1);

    if (res == FR_NO_FILESYSTEM) {
        /* 检查 FAT 区是否全 0xFF（全新/擦除过的 Flash）。
         * 仅当 Flash 是空的才自动格式化，避免误格式化有传感器数据的盘。 */
        uint8_t probe[4];
        FlashService_Read(probe, FATFS_BASE_ADDR, 4);
        if (probe[0] == 0xFF && probe[1] == 0xFF &&
            probe[2] == 0xFF && probe[3] == 0xFF) {
            DBG_PRINTF("[FAT] blank flash, format start\r\n");
            res = FatFs_Format();
            if (res == FR_OK) {
                res = f_mount(&fatfs_vol, "0:", 1);
                DBG_PRINTF("[FAT] format done, mount=%d\r\n", res);
            }
        } else {
            DBG_PRINTF("[FAT] corrupted but has data, skip format\r\n");
        }
    }

    return res;
}

void FatFs_Unmount(void)
{
    f_mount(NULL, "0:", 1);
}

FRESULT FatFs_Format(void)
{
    BYTE work[FF_MAX_SS];
    MKFS_PARM opt = {FM_FAT32, 0, 0, 0, 0};
    return f_mkfs("0:", &opt, work, sizeof(work));
}
