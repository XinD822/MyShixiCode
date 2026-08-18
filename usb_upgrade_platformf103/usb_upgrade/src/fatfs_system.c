/**
 * @file fatfs_system.c
 * @brief FatFS 系统封装
 */

#include "fatfs_system.h"
#include "hal_config.h"
#include "upgrade_config.h"

static FATFS fatfs_vol;

FRESULT FatFs_Mount(void)
{
    return f_mount(&fatfs_vol, "0:", 1);
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
