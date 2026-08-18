#ifndef __UPGRADE_CONFIG_H
#define __UPGRADE_CONFIG_H

/**
 * @brief W25Q128 Flash 分区方案（Bootloader版本）
 *
 * W25Q128 (16MB) Flash 分区方案：
 * ┌───────────────────────────────────────────────────────────────────┐
 * │ 0x000000 - 0x5DFFFF   资源区 (字库/图片)   5.875MB               │
 * │ 0x5E0000 - 0x8DFFFF   固件Slot A (当前)    3MB                  │
 * │ 0x8E0000 - 0xBDFFFF   固件Slot B (备份)    3MB                  │
 * │ 0xBE0000 - 0xFDFFFF   数据区 (FatFS)       4MB                  │
 * │ 0xFE0000 - 0xFFFFFF   配置区               128KB                 │
 * └───────────────────────────────────────────────────────────────────┘
 *
 * STM32F103 内部Flash (512KB)：
 * ┌───────────────────────────────────────────────────────────────────┐
 * │ 0x08000000 - 0x0800FFFF   Bootloader        64KB                 │
 * │ 0x08010000 - 0x0807FFFF   APP (USB拖拽)     448KB                │
 * └───────────────────────────────────────────────────────────────────┘
 *
 * 升级流程：
 *   PC拖入firmware.bin → FatFS写入数据区 → APP搬到Slot A → 设标志 → 重启
 *   Bootloader读标志 → 备份当前APP到Slot B → 拷贝Slot A到内部Flash → 重启
 */

/* ─────────────── W25Q128 分区地址 ─────────────── */

#define ASSET_BASE_ADDR         0x000000    // 资源区起始
#define ASSET_SIZE              0x5E0000    // 5.875MB

#define FIRMWARE_SLOT_A_ADDR    0x5E0000    // 固件Slot A（当前）
#define FIRMWARE_SLOT_B_ADDR    0x8E0000    // 固件Slot B（备份）
#define FIRMWARE_SLOT_SIZE      0x300000    // 每槽 3MB
#define FIRMWARE_BASE_ADDR      FIRMWARE_SLOT_A_ADDR  // APP 写入新固件的目标地址

#define FATFS_BASE_ADDR         0xBE0000    // FatFS 数据区
#define FATFS_SIZE              0x400000    // 4MB
#define FATFS_SECTOR_COUNT      8192        // 4MB / 512

#define CONFIG_AREA_ADDR        0xFE0000    // 配置区
#define CONFIG_AREA_SIZE        0x020000    // 128KB

/* ─────────────── 配置区内部偏移 ─────────────── */

#define UPGRADE_FLAG_ADDR       (CONFIG_AREA_ADDR + 0x0000)  // 升级标志
#define UPGRADE_STATE_ADDR      (CONFIG_AREA_ADDR + 0x0004)  // 升级状态
#define FIRMWARE_SIZE_ADDR      (CONFIG_AREA_ADDR + 0x0008)  // 固件大小
#define FIRMWARE_CRC_ADDR       (CONFIG_AREA_ADDR + 0x000C)  // 固件CRC
#define ACTIVE_SLOT_ADDR        (CONFIG_AREA_ADDR + 0x0010)  // 当前Slot

/* ─────────────── 升级标志值 ─────────────── */

#define UPGRADE_FLAG_YES        0x12345678
#define UPGRADE_FLAG_NO         0x00000000

/* ─────────────── 升级状态 ─────────────── */

#define UPGRADE_STATE_NONE      0x00000000  // 正常
#define UPGRADE_STATE_BURNING   0x11111111  // 正在烧写
#define UPGRADE_STATE_DONE      0x22222222  // 烧写完成待验证
#define UPGRADE_STATE_CONFIRMED 0x33333333  // App确认存活

/* ─────────────── 内部Flash分区 ─────────────── */

#define BOOTLOADER_ADDR         0x08000000  // Bootloader起始
#define BOOTLOADER_SIZE         0x00010000  // 64KB
#define APP_ADDR                0x08010000  // App起始
#define APP_SIZE                0x00070000  // 448KB

/* APP 相对 Flash 起始的偏移，用于设置 VTOR。 */
#define APP_FLASH_OFFSET        (APP_ADDR - 0x08000000)

/* ─────────────── 固件文件配置 ─────────────── */

#define FIRMWARE_FILENAME_MAX   64
#define FIRMWARE_BUF_SIZE       (4 * 1024)                      // 4KB
#define FIRMWARE_MAX_SIZE       (3 * 1024 * 1024)               // 3MB

/* ─────────────── 版本信息 ─────────────── */

#define VERSION_ADDR            (CONFIG_AREA_ADDR + 0x0100)
#define VERSION_SIZE            32

/* ─────────────── 升级检测配置 ─────────────── */

#define UPGRADE_CHECK_INTERVAL  50
#define UPGRADE_DELAY_BEFORE_RESET  1000

#endif /* __UPGRADE_CONFIG_H */
