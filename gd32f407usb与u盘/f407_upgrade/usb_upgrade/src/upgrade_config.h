/**
 * @file upgrade_config.h
 * @brief 升级系统配置（分区地址、标志值）
 *
 * 两个平台共用，改分区只需改这个文件。
 */

#ifndef __UPGRADE_CONFIG_H
#define __UPGRADE_CONFIG_H

/**
 * W25Q128 (16MB) 分区方案：
 * ┌──────────────────────────────────────────────────┐
 * │ 0x000000 - 0x5DFFFF   资源区 (字库/图片)  5.875MB │
 * │ 0x5E0000 - 0x8DFFFF   固件 Slot A       3MB      │
 * │ 0x8E0000 - 0xBDFFFF   固件 Slot B       3MB      │
 * │ 0xBE0000 - 0xFDFFFF   数据区 (FatFS)    4MB      │
 * │ 0xFE0000 - 0xFFFFFF   配置区            128KB     │
 * └──────────────────────────────────────────────────┘
 */

/* ──── W25Q128 分区地址 ──── */
#define ASSET_BASE_ADDR         0x000000
#define ASSET_SIZE              0x5E0000

#define FIRMWARE_SLOT_A_ADDR    0x5E0000
#define FIRMWARE_SLOT_B_ADDR    0x8E0000
#define FIRMWARE_SLOT_SIZE      0x300000
#define FIRMWARE_BASE_ADDR      FIRMWARE_SLOT_A_ADDR

#define FATFS_BASE_ADDR         0xBE0000
#define FATFS_SIZE              0x400000
#define FATFS_SECTOR_COUNT      8192

#define CONFIG_AREA_ADDR        0xFE0000
#define CONFIG_AREA_SIZE        0x020000

/* ──── 配置区偏移 ──── */
#define UPGRADE_FLAG_ADDR       (CONFIG_AREA_ADDR + 0x0000)
#define UPGRADE_STATE_ADDR      (CONFIG_AREA_ADDR + 0x0004)
#define FIRMWARE_SIZE_ADDR      (CONFIG_AREA_ADDR + 0x0008)
#define FIRMWARE_CRC_ADDR       (CONFIG_AREA_ADDR + 0x000C)
#define ACTIVE_SLOT_ADDR        (CONFIG_AREA_ADDR + 0x0010)

/* ──── USB 模式标志（独立扇区 0xFE1000，避免擦除冲突） ──── */
#define USB_MODE_FLAG_ADDR      (CONFIG_AREA_ADDR + 0x1000)
#define USB_MODE_FLAG_HOST      0xA5A5A5A5
#define USB_MODE_FLAG_DEVICE    0x00000000

/* ──── 升级标志值 ──── */
#define UPGRADE_FLAG_YES        0x12345678
#define UPGRADE_FLAG_NO         0x00000000

/* ──── 升级状态 ──── */
#define UPGRADE_STATE_NONE      0x00000000
#define UPGRADE_STATE_BURNING   0x11111111
#define UPGRADE_STATE_DONE      0x22222222
#define UPGRADE_STATE_CONFIRMED 0x33333333

/* ──── 内部 Flash 分区 ──── */
#define BOOTLOADER_ADDR         0x08000000
#define BOOTLOADER_SIZE         0x00010000
#define APP_ADDR                0x08010000
#define APP_SIZE                0x00070000

/* APP 相对 Flash 起始的偏移（用于设置 VTOR）。
 * 配合 Bootloader 时为 0x10000，并同步修改 Keil IROM1 起始地址。
 * 独立运行时为 0。 */
#define APP_FLASH_OFFSET        0x10000

/* ──── 固件配置 ──── */
#define FIRMWARE_FILENAME_MAX   64
#define FIRMWARE_BUF_SIZE       (4 * 1024)
#define FIRMWARE_MAX_SIZE       (3 * 1024 * 1024)

/* ──── 升级检测配置 ──── */
#define UPGRADE_CHECK_INTERVAL  50
#define UPGRADE_DELAY_BEFORE_RESET  1000

#endif /* __UPGRADE_CONFIG_H */
