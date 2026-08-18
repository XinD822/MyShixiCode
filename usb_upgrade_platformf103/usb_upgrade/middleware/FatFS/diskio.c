/**
 * @file diskio.c
 * @brief FatFS 磁盘 I/O 底层驱动（HAL 解耦版）
 *
 * 通过 HAL_Flash 接口访问 Flash，不直接依赖任何芯片驱动。
 * 换 Flash 芯片时此文件不需要改。
 */

#include "ff.h"
#include "diskio.h"
#include "hal_config.h"
#include "upgrade_config.h"
#include "flash_service.h"

#define DEV_FLASH   0

DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv == DEV_FLASH) {
        if (HAL_Flash->read_id() != 0) {
            return 0;
        }
    }
    return STA_NOINIT;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv == DEV_FLASH) {
        HAL_Flash->init();
        return disk_status(pdrv);
    }
    return STA_NOINIT;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv == DEV_FLASH) {
        uint32_t addr = FATFS_BASE_ADDR + (sector << 9);
        /* 通过 FlashService 读取，保证缓存一致性 */
        FlashService_Read(buff, addr, count << 9);
        return RES_OK;
    }
    return RES_PARERR;
}

#if FF_FS_READONLY == 0
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != DEV_FLASH || !count) return RES_PARERR;

    uint32_t addr = FATFS_BASE_ADDR + (sector << 9);
    uint32_t len = count << 9;

    /* 通过 FlashService 统一写入，走同一套缓存 */
    FlashService_Write(buff, addr, len);

    return RES_OK;
}
#endif

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv == DEV_FLASH) {
        switch (cmd) {
            case CTRL_SYNC:
                FlashService_FlushCache();
                break;
            case GET_SECTOR_COUNT:
                *(DWORD *)buff = FATFS_SECTOR_COUNT;
                break;
            case GET_SECTOR_SIZE:
                *(WORD *)buff = 512;
                break;
            case GET_BLOCK_SIZE:
                *(DWORD *)buff = 8;
                break;
            default:
                return RES_PARERR;
        }
        return RES_OK;
    }
    return RES_PARERR;
}
