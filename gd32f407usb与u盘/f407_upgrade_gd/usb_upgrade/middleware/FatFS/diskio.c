/**
 * @file diskio.c
 * @brief FatFS 磁盘 I/O 底层驱动
 *
 * 支持两个卷：
 *   pdrv=0  SPI Flash（Device 模式拖拽升级用）
 *   pdrv=1  USB U盘（Host 模式读取升级用）
 */

#include "ff.h"
#include "diskio.h"
#include "board_config.h"
#include "upgrade_config.h"
#include "flash_service.h"
#include "config.h"       /* plat_flash_* / LOGD */

#if UPGRADE_SRC_USB_DRIVE
#include "usbh_usr.h"
/* USB Host 全局实例（定义在 usb_host_task.c） */
extern usbh_host usb_host_msc;
#endif

#define DEV_FLASH   0
#define DEV_USB     1

/* Flash 初始化标志 — 避免每次 disk_status 都调 read_id() 绕过互斥锁 */
static DSTATUS disk_stat_flash = STA_NOINIT;
#if UPGRADE_SRC_USB_DRIVE
static DSTATUS disk_stat_usb   = STA_NOINIT;
#endif

DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv == DEV_FLASH) {
        return disk_stat_flash;
    }
#if UPGRADE_SRC_USB_DRIVE
    if (pdrv == DEV_USB) {
        return disk_stat_usb;
    }
#endif
    return STA_NOINIT;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv == DEV_FLASH) {
        plat_flash_init();
        /* 不同批次/品牌 SPI NOR 的读 ID 不同（NM25Q128=0x5217、
           W25Q128=0xEF18 等）。只要求能读到有效 ID 即视为就绪，
           否则 FatFs 的 f_mount/f_mkfs 会直接返回 FR_NOT_READY。 */
        uint16_t fid = plat_flash_read_id();
        if (fid != 0xFFFF && fid != 0x0000) {
            disk_stat_flash = 0;           /* 就绪 */
        } else {
            disk_stat_flash = STA_NOINIT;  /* 未就绪 */
        }
        return disk_stat_flash;
    }
#if UPGRADE_SRC_USB_DRIVE
    if (pdrv == DEV_USB) {
        /* U盘初始化由 USBH_Process 处理，这里只检查连接状态 */
        if (USBH_UDISK_Status()) {
            disk_stat_usb = 0;
        } else {
            disk_stat_usb = STA_NOINIT;
        }
        return disk_stat_usb;
    }
#endif
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
#if UPGRADE_SRC_USB_DRIVE
    if (pdrv == DEV_USB) {
        uint8_t r = USBH_UDISK_Read(buff, sector, count);
        if (r != 0) {
            LOGD("[DISK] USB read fail: sector=%lu cnt=%lu r=%d\r\n",
                       (unsigned long)sector, (unsigned long)count, (int)r);
        }
        return (r == 0) ? RES_OK : RES_ERROR;
    }
#endif
    return RES_PARERR;
}

#if FF_FS_READONLY == 0
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv == DEV_FLASH) {
        if (!count) return RES_PARERR;
        uint32_t addr = FATFS_BASE_ADDR + (sector << 9);
        uint32_t len = count << 9;
        FlashService_Write(buff, addr, len);
        return RES_OK;
    }
#if UPGRADE_SRC_USB_DRIVE
    if (pdrv == DEV_USB) {
        if (USBH_UDISK_Write((BYTE *)buff, sector, count) == 0) {
            return RES_OK;
        }
        return RES_ERROR;
    }
#endif
    return RES_PARERR;
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
#if UPGRADE_SRC_USB_DRIVE
    if (pdrv == DEV_USB) {
        switch (cmd) {
            case CTRL_SYNC:
                break;
            case GET_SECTOR_SIZE:
                *(WORD *)buff = 512;
                break;
            case GET_SECTOR_COUNT:
                /* GD 库：READ CAPACITY(10) 返回的 block_nbr 是最后一个 LBA（0 基），
                   总扇区数 = block_nbr + 1（与 STM32 版 MSCapacity + 1 一致，
                   否则 FatFS 认为 U 盘少一个扇区）。 */
                {
                    msc_lun info;
                    if (USBH_OK == usbh_msc_lun_info_get(&usb_host_msc, 0U, &info)) {
                        *(DWORD *)buff = (DWORD)info.capacity.block_nbr + 1;
                    } else {
                        return RES_ERROR;
                    }
                }
                break;
            case GET_BLOCK_SIZE:
                *(DWORD *)buff = 8;
                break;
            default:
                return RES_PARERR;
        }
        return RES_OK;
    }
#endif
    return RES_PARERR;
}
