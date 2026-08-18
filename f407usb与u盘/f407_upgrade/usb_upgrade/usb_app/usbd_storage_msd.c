/**
  * @file    usbd_storage_msd.c
  * @brief   USB MSC 存储层 — 对接 W25Q128 FatFS 分区
  *
  * 暴露 FatFS 分区（0xBE0000, 4MB）给 PC 作为 U 盘读写。
  * 单 LUN，块大小 512 字节。
  */

#include "usbd_msc_mem.h"
#include "usb_conf.h"
#include "config.h"

/* SCSI 活动时间戳，供 usb_task.c 空闲检测使用 */
volatile uint32_t g_last_scsi_tick = 0;

/* 单 LUN */
#define STORAGE_LUN_NBR     1

/* FatFS 分区信息（来自 upgrade_config.h） */
#define STORAGE_BASE_ADDR   FATFS_BASE_ADDR       /* 0xBE0000 */
#define STORAGE_BLOCK_SIZE  512
#define STORAGE_BLOCK_NUM   FATFS_SECTOR_COUNT    /* 8192 */

/* SCSI 标准查询数据（单 LUN，36 字节） */
const int8_t  STORAGE_Inquirydata[] = {
  0x00,      /* LUN 0: 直接访问块设备 */
  0x80,      /* 可移动介质 */
  0x02,      /* SPC-2 */
  0x02,
  (USBD_STD_INQUIRY_LENGTH - 4),
  0x00,
  0x00,
  0x00,
  /* Vendor Identification (9 bytes) */
  'S', 'T', 'M', '3', '2', 'F', '4', '0', '7',
  /* Product Identification (15 bytes) */
  'S', 'P', 'I', ' ', 'F', 'l', 'a', 's', 'h',
  ' ', 'D', 'i', 's', 'k', ' ',
  /* Product Revision Level (4 bytes) */
  '1', '.', '0', ' ',
};

int8_t STORAGE_Init (uint8_t lun);
int8_t STORAGE_GetCapacity (uint8_t lun, uint32_t *block_num, uint32_t *block_size);
int8_t STORAGE_IsReady (uint8_t lun);
int8_t STORAGE_IsWriteProtected (uint8_t lun);
int8_t STORAGE_Read (uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
int8_t STORAGE_Write (uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
int8_t STORAGE_GetMaxLun (void);

USBD_STORAGE_cb_TypeDef USBD_MICRO_SDIO_fops =
{
  STORAGE_Init,
  STORAGE_GetCapacity,
  STORAGE_IsReady,
  STORAGE_IsWriteProtected,
  STORAGE_Read,
  STORAGE_Write,
  STORAGE_GetMaxLun,
  (int8_t *)STORAGE_Inquirydata,
};

USBD_STORAGE_cb_TypeDef *USBD_STORAGE_fops = &USBD_MICRO_SDIO_fops;

int8_t STORAGE_Init (uint8_t lun)
{
  (void)lun;
  /* Flash 已在 USB_Upgrade_Init 中初始化 */
  return 0;
}

int8_t STORAGE_GetCapacity (uint8_t lun, uint32_t *block_num, uint32_t *block_size)
{
  (void)lun;
  *block_size = STORAGE_BLOCK_SIZE;
  *block_num  = STORAGE_BLOCK_NUM;
  return 0;
}

int8_t STORAGE_IsReady (uint8_t lun)
{
  (void)lun;
  return 0;
}

int8_t STORAGE_IsWriteProtected (uint8_t lun)
{
  (void)lun;
  return 0;
}

int8_t STORAGE_Read (uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
  (void)lun;
  g_last_scsi_tick = Tick_GetMs();
  FlashService_Read(buf, STORAGE_BASE_ADDR + blk_addr * 512, blk_len * 512);
  return 0;
}

int8_t STORAGE_Write (uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
  (void)lun;
  g_last_scsi_tick = Tick_GetMs();
  FlashService_Write(buf, STORAGE_BASE_ADDR + blk_addr * 512, blk_len * 512);
  /* 不在此处 FlushCache — 匹配 F103 参考实现。
     FlashService_Write 内部缓存会在页切换时自动刷写，
     FatFs 读取时 FlashService_Read 会检查脏缓存保证数据一致。
     每次 SCSI 写都刷写会导致 50-400ms Flash 擦除阻塞 USB 轮询。 */
  return 0;
}

int8_t STORAGE_GetMaxLun (void)
{
  return (STORAGE_LUN_NBR - 1);
}
