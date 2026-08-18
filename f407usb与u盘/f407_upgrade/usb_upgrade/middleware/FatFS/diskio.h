/*-----------------------------------------------------------------------/
/  Low level disk interface modlue include file   (C)ChaN, 2019          /
/-----------------------------------------------------------------------*/

#ifndef _DISKIO_DEFINED
#define _DISKIO_DEFINED


#include "config.h"


#ifdef __cplusplus
extern "C" {
#endif

/* 磁盘操作状态(DSTATUS是BYTE类型的别名) */
typedef BYTE	DSTATUS;

/* 磁盘函数的返回结果 */
typedef enum {
	RES_OK = 0,		/* 0: Successful 操作成功*/
	RES_ERROR,		/* 1: R/W Error 读写错误*/
	RES_WRPRT,		/* 2: Write Protected 磁盘写保护*/
	RES_NOTRDY,		/* 3: Not Ready 磁盘未就绪*/
	RES_PARERR		/* 4: Invalid Parameter 参数无效*/
} DRESULT;


/*---------------------------------------*/
/* 磁盘控制函数的原型声明 */
DSTATUS disk_initialize (BYTE pdrv);
DSTATUS disk_status (BYTE pdrv);
DRESULT disk_read (BYTE pdrv, BYTE* buff, LBA_t sector, UINT count);
DRESULT disk_write (BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count);
DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void* buff);


/* 磁盘状态位 (DSTATUS)  */
#define STA_NOINIT		0x01	/* Drive not initialized  驱动器未初始化 */
#define STA_NODISK		0x02	/* No medium in the drive  驱动器中无介质 */
#define STA_PROTECT		0x04	/* Write protected  介质被写保护 */


/* disk_ioctrl函数的命令码 */

/* 通用命令（供FatFs文件系统使用） */
#define CTRL_SYNC           0   /* 完成挂起的写操作（当FF_FS_READONLY == 0时需要） */
#define GET_SECTOR_COUNT    1   /* 获取介质总扇区数（当FF_USE_MKFS == 1时需要） */
#define GET_SECTOR_SIZE     2   /* 获取扇区大小（当FF_MAX_SS != FF_MIN_SS时需要） */
#define GET_BLOCK_SIZE      3   /* 获取擦除块大小（当FF_USE_MKFS == 1时需要） */
#define CTRL_TRIM           4   /* 通知设备某些扇区的数据不再使用（当FF_USE_TRIM == 1时需要） */

/* 通用命令（FatFs文件系统不使用） */
#define CTRL_POWER          5   /* 获取/设置电源状态 */
#define CTRL_LOCK           6   /* 锁定/解锁介质移除 */
#define CTRL_EJECT          7   /* 弹出介质 */
#define CTRL_FORMAT         8   /* 在介质上创建物理格式 */

/* MMC/SD卡特定的ioctl命令 */
#define MMC_GET_TYPE        10  /* 获取卡类型 */
#define MMC_GET_CSD         11  /* 获取CSD寄存器（卡特定数据） */
#define MMC_GET_CID         12  /* 获取CID寄存器（卡识别号） */
#define MMC_GET_OCR         13  /* 获取OCR寄存器（操作条件寄存器） */
#define MMC_GET_SDSTAT      14  /* 获取SD卡状态 */
#define ISDIO_READ          55  /* 从SD iSDIO寄存器读取数据 */
#define ISDIO_WRITE         56  /* 向SD iSDIO寄存器写入数据 */
#define ISDIO_MRITE         57  /* 掩码方式写入SD iSDIO寄存器 */

/* ATA/CF特定的ioctl命令 */
#define ATA_GET_REV         20  /* 获取固件版本 */
#define ATA_GET_MODEL       21  /* 获取型号名称 */
#define ATA_GET_SN          22  /* 获取序列号 */

#ifdef __cplusplus
}
#endif

#endif
