/**
 * @file fatfs_system.h
 * @brief FatFS 系统封装
 */

#ifndef __FATFS_SYSTEM_H
#define __FATFS_SYSTEM_H

#include "ff.h"

FRESULT FatFs_Mount(void);
FRESULT FatFs_MountNoFormat(void);
void    FatFs_Unmount(void);
FRESULT FatFs_Format(void);

#endif /* __FATFS_SYSTEM_H */
