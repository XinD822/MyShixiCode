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
        /* 数据区没有有效 FAT 文件系统（全新空盘 或 此前从未成功格式化）。
         * 此时格式化是安全的：盘上本来就没有可读文件，格式化只会创建
         * 一个全新的 FAT16，Windows 可直接挂载。
         * ⚠ 只有 FR_NO_FILESYSTEM 才自动格式化；FR_NOT_READY / FR_DISK_ERR
         * 等错误（介质未就绪/IO 异常）一律保留现场，绝不擦盘。 */
        LOGD("[FAT] no filesystem (res=%d) -> format FAT16\r\n", (int)res);
        f_mount(NULL, "0:", 1);
        res = FatFs_Format();
        if (res == FR_OK) {
            res = f_mount(&fatfs_vol, "0:", 1);
            LOGD("[FAT] format done, mount=%d\r\n", res);
        } else {
            LOGD("[FAT] format FAILED res=%d\r\n", (int)res);
        }
    } else if (res != FR_OK) {
        /* 非文件系统错误：保留数据，不格式化（可能是 USB 连接期间
         * Windows 正占用卷，或介质短暂异常）。 */
        LOGD("[FAT] mount fail res=%d, keep data, NO format\r\n", (int)res);
        f_mount(NULL, "0:", 1);
    }

    return res;
}

/* 与 FatFs_Mount 相同，但禁止自动格式化。
 * 用于 USB 连接期间的 10s 空闲检测：此时 Windows 正通过 MSC 占用
 * 该卷，若挂载失败就格式化会清空 Windows 正在使用的文件系统。 */
FRESULT FatFs_MountNoFormat(void)
{
    FRESULT res = f_mount(&fatfs_vol, "0:", 1);

    if (res != FR_OK) {
        LOGD("[FAT] mount fail res=%d (no-format mode), keep data\r\n", (int)res);
        f_mount(NULL, "0:", 1);
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
    /* 4MB 数据区用 FAT12/16（按容量自动选 FAT16）。FAT32 在 <32MB 盘上
       Windows 无法挂载，会每次插入都提示格式化。 */
    MKFS_PARM opt = {FM_FAT, 0, 0, 0, 0};
    return f_mkfs("0:", &opt, work, sizeof(work));
}
