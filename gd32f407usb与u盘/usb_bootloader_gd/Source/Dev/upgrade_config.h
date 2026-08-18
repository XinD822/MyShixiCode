#ifndef __UPGRADE_CONFIG_H
#define __UPGRADE_CONFIG_H

/**
 * @brief W25Q128 Flash 分区方案（Bootloader版本）— 与 OTA 工程对齐
 *
 * W25Q128 (16MB) Flash 分区方案：
 * ┌───────────────────────────────────────────────────────────────────┐
 * │ 0x000000 - 0x1FFFFF   升级缓冲区 OTA_FW (Slot A)  2MB             │
 * │ 0x200000 - 0x3FFFFF   App 备份区 PART_APP_BACKUP (Slot B)  2MB   │
 * │ 0x400000 - 0x6FFFFF   文件区 PART_FILE (FatFS)  3MB              │
 * │ 0x700000 - 0x707FFF   升级配置区 OTA_INFO 前 32KB (OTA 使用)      │
 * │ 0x708000 - 0x70FFFF   升级配置区 后 32KB (本模块升级配置)         │
 * │ 0x710000 - 0x71FFFF   加密区 CHIP_BIND  64KB (未使用)            │
 * │ 0x720000 - 0xBFFFFF   保留区 PART_RESERVED  ~4.875MB (未使用)    │
 * │ 0xC00000 - 0xFFFFFF   字库区 PART_FONT  4MB (未使用)             │
 * └───────────────────────────────────────────────────────────────────┘
 *
 * 内部 Flash (GD32F407)：
 * ┌───────────────────────────────────────────────────────────────────┐
 * │ 0x08000000 - 0x0800FFFF   Bootloader        64KB                 │
 * │ 0x08010000 - 0x0807FFFF   APP (USB拖拽)     448KB                │
 * └───────────────────────────────────────────────────────────────────┘
 *
 * 升级流程：
 *   U盘/PC拖入 firmware.bin → FatFS 写入文件区 → APP 搬到 Slot A → 设标志 → 重启
 *   Bootloader 读标志 → 校验 Slot A MD5 → 备份当前 APP 到 Slot B →
 *   拷贝 Slot A 到内部 Flash → 校验 → 重启
 */

/* ─────────────── W25Q128 物理分区（与 OTA 工程对齐） ─────────────── */

#define OTA_FW_ADDR                 0x000000    // 升级缓冲区 (Slot A) 起始
#define OTA_FW_SIZE                 0x200000    // 2MB
#define OTA_FW_MAX_SIZE             (2048 * 1024)

#define PART_APP_BACKUP_ADDR        0x200000    // App 备份区 (Slot B) 起始
#define PART_APP_BACKUP_SIZE        0x200000    // 2MB

#define PART_FILE_ADDR              0x400000    // 文件区 (FatFS) 起始
#define PART_FILE_SIZE              0x300000    // 3MB

#define OTA_INFO_ADDR               0x700000    // 升级配置区 起始（64KB）
#define WIFI_SSID_ADDR              0x701000    // OTA 侧 WIFI 配置（未使用）
#define OTA_INFO_SIZE               0x10000

#define CHIP_BIND_FLASH_ADDR        0x710000    // 加密区（未使用）
#define CHIP_BIND_FLASH_SIZE        0x10000

#define PART_RESERVED_ADDR          0x720000    // 保留区（未使用）
#define PART_RESERVED_SIZE          0x4E0000

#define PART_FONT_ADDR              0xC00000    // 字库区（未使用）
#define PART_FONT_SIZE              0x400000

/* ─────────────── 升级模块分区映射 ─────────────── */

#define FIRMWARE_SLOT_A_ADDR        OTA_FW_ADDR             // Slot A = OTA 升级缓冲区（复合使用，用前必擦）
#define FIRMWARE_SLOT_B_ADDR        PART_APP_BACKUP_ADDR    // Slot B = App 备份区
#define FIRMWARE_SLOT_SIZE          OTA_FW_SIZE             // 单槽 2MB
#define FIRMWARE_BASE_ADDR          FIRMWARE_SLOT_A_ADDR    // APP 写入新固件的目标地址

#define FATFS_BASE_ADDR             PART_FILE_ADDR          // FatFS 文件区
#define FATFS_SIZE                  PART_FILE_SIZE          // 3MB
#define FATFS_SECTOR_COUNT          (FATFS_SIZE / 512)      // 3MB/512 = 6144

/* 升级配置区：OTA_INFO 后 32KB（0x708000~0x710000），前 32KB 归 OTA */
#define CONFIG_AREA_ADDR            (OTA_INFO_ADDR + 0x8000)
#define CONFIG_AREA_SIZE            0x8000                  // 32KB

/* ─────────────── 配置区内部偏移 ─────────────── */

#define UPGRADE_FLAG_ADDR       (CONFIG_AREA_ADDR + 0x0000)  // 升级标志
#define UPGRADE_STATE_ADDR      (CONFIG_AREA_ADDR + 0x0004)  // 升级状态
#define FIRMWARE_SIZE_ADDR      (CONFIG_AREA_ADDR + 0x0008)  // 固件大小
#define FIRMWARE_CRC_ADDR       (CONFIG_AREA_ADDR + 0x000C)  // 固件CRC
#define ACTIVE_SLOT_ADDR        (CONFIG_AREA_ADDR + 0x0010)  // 当前Slot

/* ─────────────── 固件 MD5（各 16B，分占两个独立扇区） ───────────────
 * 独立扇区存放，避免被 SetFlag/StateCheck 擦除配置区首个扇区时连带清掉
 * （MD5_B 需持久保存供回滚校验）。
 * A、B 分占不同扇区：备份时只擦 MD5_B 扇区写 MD5_B，不清 MD5_A ——
 * 断电后重试升级时仍可校验 Slot A。 */
#define FIRMWARE_MD5_BASE_ADDR  (CONFIG_AREA_ADDR + 0x2000)  // Slot A MD5 独立扇区
#define FIRMWARE_MD5_A_ADDR     (FIRMWARE_MD5_BASE_ADDR + 0x0000)  // Slot A 固件 MD5（升级时写入）
#define FIRMWARE_MD5_B_ADDR     (CONFIG_AREA_ADDR + 0x3000)  // Slot B 备份 MD5（独立扇区，备份时写入）

/* USB 模式标志独立扇区 */
#define USB_MODE_FLAG_ADDR      (CONFIG_AREA_ADDR + 0x1000)
#define USB_MODE_FLAG_HOST      0xA5A5A5A5
#define USB_MODE_FLAG_DEVICE    0x00000000

#define PARTITION_VER_ADDR      (CONFIG_AREA_ADDR + 0x4000)  // 分区版本号

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
#define FIRMWARE_MAX_SIZE       OTA_FW_MAX_SIZE                 // 2MB，对齐 Slot A 容量

/* ─────────────── 版本信息 ─────────────── */

#define VERSION_ADDR            (CONFIG_AREA_ADDR + 0x0100)
#define VERSION_SIZE            32

/* ─────────────── 升级检测配置 ─────────────── */

#define UPGRADE_CHECK_INTERVAL  50
#define UPGRADE_DELAY_BEFORE_RESET  1000

#endif /* __UPGRADE_CONFIG_H */
