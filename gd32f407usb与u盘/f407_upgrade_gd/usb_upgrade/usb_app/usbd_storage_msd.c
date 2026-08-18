/**
  * @file    usbd_storage_msd.c
  * @brief   USB MSC 存储层 — 对接 W25Q128 FatFS 分区（GD32F4xx_usb_library）
  *
  * 暴露 FatFS 分区（0xBE0000, 4MB）给 PC 作为 U 盘读写。
  * 单 LUN，块大小 512 字节。
  */

#include "usbd_msc_mem.h"
#include "usbd_conf.h"
#include "usb_conf.h"
#include "config.h"

/* SCSI 活动时间戳，供 usb_task.c 空闲检测使用 */
volatile uint32_t g_last_scsi_tick = 0;

/* FatFS 分区信息（来自 upgrade_config.h） */
#define STORAGE_BASE_ADDR   FATFS_BASE_ADDR       /* 0xBE0000 */
#define STORAGE_BLOCK_SIZE  512
#define STORAGE_BLOCK_NUM   FATFS_SECTOR_COUNT    /* 8192 */

/* SCSI 标准查询数据（单 LUN，36 字节） */
static uint8_t STORAGE_Inquirydata[] = {
  0x00,      /* LUN 0: 直接访问块设备 */
  0x80,      /* 可移动介质 */
  0x02,      /* SPC-2 */
  0x02,
  (USBD_STD_INQUIRY_LENGTH - 4),
  0x00,
  0x00,
  0x00,
  /* Vendor Identification (9 bytes) */
  'G', 'D', '3', '2', 'F', '4', '0', '7',
  /* Product Identification (15 bytes) */
  'F', 'l', 'a', 's', 'h', ' ', 'D', 'i',
  's', 'k', ' ', ' ', ' ', ' ', ' ',
  /* Product Revision Level (4 bytes) */
  '1', '.', '0', ' ',
};

int8_t STORAGE_Init        (uint8_t lun);
int8_t STORAGE_IsReady     (uint8_t lun);
int8_t STORAGE_IsProtected (uint8_t lun);
int8_t STORAGE_Read        (uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
int8_t STORAGE_Write       (uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
int8_t STORAGE_GetMaxLun   (void);

usbd_mem_cb USBD_STORAGE_fops =
{
  STORAGE_Init,
  STORAGE_IsReady,
  STORAGE_IsProtected,
  STORAGE_Read,
  STORAGE_Write,
  STORAGE_GetMaxLun,
  NULL,                                    /* mem_toc_data */
  {STORAGE_Inquirydata},                   /* mem_inquiry_data */
  {STORAGE_BLOCK_SIZE},                    /* mem_block_size */
  {STORAGE_BLOCK_NUM},                     /* mem_block_len */
};

usbd_mem_cb *usbd_mem_fops = &USBD_STORAGE_fops;

int8_t STORAGE_Init (uint8_t lun)
{
  (void)lun;
  /* Flash 已在 USB_Upgrade_Init 中初始化 */
  return 0;
}

int8_t STORAGE_IsReady (uint8_t lun)
{
  (void)lun;
  return 0;
}

int8_t STORAGE_IsProtected (uint8_t lun)
{
  (void)lun;
  return 0;
}

int8_t STORAGE_Read (uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
  (void)lun;
  g_last_scsi_tick = plat_get_tick_ms();
  /* GD32 USB 库在 scsi_read10 中已将扇区号 × 块大小（512），
     传入的 blk_addr 是字节偏移，不要再乘 512 */
  FlashService_Read(buf, STORAGE_BASE_ADDR + blk_addr,
                    (uint32_t)blk_len * STORAGE_BLOCK_SIZE);
  return 0;
}

int8_t STORAGE_Write (uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
  (void)lun;
  g_last_scsi_tick = plat_get_tick_ms();
  /* 同上：blk_addr 已是字节偏移 */
  FlashService_Write(buf, STORAGE_BASE_ADDR + blk_addr,
                     (uint32_t)blk_len * STORAGE_BLOCK_SIZE);
  FlashService_FlushCache();
  return 0;
}

int8_t STORAGE_GetMaxLun (void)
{
  return (MEM_LUN_NUM - 1);
}
