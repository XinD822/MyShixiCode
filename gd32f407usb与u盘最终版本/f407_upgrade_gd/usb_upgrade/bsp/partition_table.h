/**
 * @file partition_table.h
 * @brief Flash 分区表 — 单一配置源，Bootloader / APP / OTA 共用
 *
 * 物理分区与 OTA 工程对齐（绝对地址），升级模块用自己语义的宏名映射：
 *   OTA_FW           = 升级缓冲区  → 本模块 Slot A
 *   PART_APP_BACKUP  = App 备份区  → 本模块 Slot B
 *   PART_FILE        = 文件区      → FatFS 管理区
 *   OTA_INFO         = 升级配置区  → 前 32KB 给 OTA，后 32KB 给本模块升级配置
 * 加密区 / 保留区 / 字库区：本模块不使用，仅保留定义备查。
 */
#ifndef __PARTITION_TABLE_H
#define __PARTITION_TABLE_H
#include <stdint.h>

#define PARTITION_VERSION  0x00030000  /* v3.0 — 分区对齐 OTA 方案时递增 */

/* ═══ W25Q128 (16MB) 物理分区（与 OTA 工程对齐） ═══ */
/* 升级缓冲区：2 MB (0~2M) */
#define OTA_FW_ADDR                 0x000000
#define OTA_FW_SIZE                 0x200000
#define OTA_FW_MAX_SIZE             (2048 * 1024)

/* App 备份区：2 MB (2~4M) */
#define PART_APP_BACKUP_ADDR        0x200000
#define PART_APP_BACKUP_SIZE        0x200000

/* 文件区：3 MB (4~7M) */
#define PART_FILE_ADDR              0x400000
#define PART_FILE_SIZE              0x300000

/* 升级配置区：64 KB (7~7.0625M)，前 32KB OTA 配置 / 后 32KB 升级配置 */
#define OTA_INFO_ADDR               0x700000
#define WIFI_SSID_ADDR              0x701000
#define OTA_INFO_SIZE               0x10000

/* 加密区：64 KB（本模块不使用，保留备查） */
#define CHIP_BIND_FLASH_ADDR        0x710000
#define CHIP_BIND_FLASH_SIZE        0x10000

/* 保留区：~4.875MB（本模块不使用，保留备查） */
#define PART_RESERVED_ADDR          0x720000
#define PART_RESERVED_SIZE          0x4E0000

/* 字库区：4 MB (12~16M)（本模块不使用，保留备查） */
#define PART_FONT_ADDR              0xC00000
#define PART_FONT_SIZE              0x400000

/* ═══ 升级模块分区映射（语义化宏名 → 物理分区） ═══ */
#define FIRMWARE_SLOT_A_ADDR        OTA_FW_ADDR             /* Slot A = OTA 升级缓冲区（复合使用，用前必擦） */
#define FIRMWARE_SLOT_B_ADDR        PART_APP_BACKUP_ADDR    /* Slot B = App 备份区 */
#define FIRMWARE_SLOT_SIZE          OTA_FW_SIZE             /* 单槽 2MB */
#define FIRMWARE_BASE_ADDR          FIRMWARE_SLOT_A_ADDR

#define FATFS_BASE_ADDR             PART_FILE_ADDR          /* FatFS 管理区 */
#define FATFS_SIZE                  PART_FILE_SIZE          /* 3MB */
#define FATFS_SECTOR_COUNT          (FATFS_SIZE / 512)      /* 3MB/512 = 6144 */

/* 升级配置区：OTA_INFO 后 32KB（0x708000~0x710000），前 32KB 归 OTA */
#define CONFIG_AREA_ADDR            (OTA_INFO_ADDR + 0x8000)
#define CONFIG_AREA_SIZE            0x8000                  /* 32KB */

/* ═══ 配置区偏移（前 16B 标志 + 独立扇区防擦除冲突） ═══ */
#define UPGRADE_FLAG_ADDR       (CONFIG_AREA_ADDR + 0x0000)
#define UPGRADE_STATE_ADDR      (CONFIG_AREA_ADDR + 0x0004)
#define FIRMWARE_SIZE_ADDR      (CONFIG_AREA_ADDR + 0x0008)
#define FIRMWARE_CRC_ADDR       (CONFIG_AREA_ADDR + 0x000C)
#define ACTIVE_SLOT_ADDR        (CONFIG_AREA_ADDR + 0x0010)

#define FIRMWARE_MD5_BASE_ADDR  (CONFIG_AREA_ADDR + 0x2000)  /* MD5_A 独立扇区 */
#define FIRMWARE_MD5_A_ADDR     (FIRMWARE_MD5_BASE_ADDR + 0x0000)
#define FIRMWARE_MD5_B_ADDR     (CONFIG_AREA_ADDR + 0x3000)  /* MD5_B 独立扇区 */

#define USB_MODE_FLAG_ADDR      (CONFIG_AREA_ADDR + 0x1000)  /* USB 模式标志独立扇区 */
#define USB_MODE_FLAG_HOST      0xA5A5A5A5
#define USB_MODE_FLAG_DEVICE    0x00000000

#define PARTITION_VER_ADDR      (CONFIG_AREA_ADDR + 0x4000)  /* 分区版本号 */

/* ═══ 升级标志值 ═══ */
#define UPGRADE_FLAG_YES        0x12345678
#define UPGRADE_FLAG_NO         0x00000000

/* ═══ 升级状态 ═══ */
#define UPGRADE_STATE_NONE      0x00000000
#define UPGRADE_STATE_BURNING   0x11111111
#define UPGRADE_STATE_DONE      0x22222222
#define UPGRADE_STATE_CONFIRMED 0x33333333

/* ═══ 内部 Flash 分区 ═══ */
#define BOOTLOADER_ADDR         0x08000000
#define BOOTLOADER_SIZE         0x00010000
#define APP_ADDR                0x08010000
#define APP_SIZE                0x00070000
#define APP_FLASH_OFFSET        0x10000

/* ═══ 固件配置 ═══ */
#define FIRMWARE_FILENAME_MAX   64
#define FIRMWARE_BUF_SIZE       (4 * 1024)
#define FIRMWARE_MAX_SIZE       OTA_FW_MAX_SIZE   /* 2MB，对齐 Slot A 容量 */

/* ═══ 升级检测配置 ═══ */
#define UPGRADE_CHECK_INTERVAL  50
#define UPGRADE_DELAY_BEFORE_RESET  1000

#endif /* __PARTITION_TABLE_H */
