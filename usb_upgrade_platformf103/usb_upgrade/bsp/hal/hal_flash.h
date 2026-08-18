/**
 * @file hal_flash.h
 * @brief SPI Flash HAL 接口
 *
 * 通过函数指针表实现多 Flash 芯片支持。
 * 换 Flash 芯片只需新增一个驱动实例，业务代码零改动。
 */

#ifndef __HAL_FLASH_H
#define __HAL_FLASH_H

#include <stdint.h>

/**
 * @brief Flash 驱动结构体（函数指针表）
 */
typedef struct {
    const char *name;           /* 芯片名称，如 "W25Q128" */
    uint32_t    capacity;       /* 总容量（字节） */
    uint32_t    sector_size;    /* 扇区大小（字节），通常 4096 */
    uint32_t    block_size;     /* 块大小（字节），通常 65536 */

    void     (*init)(void);
    uint16_t (*read_id)(void);
    void     (*read)(uint8_t *buf, uint32_t addr, uint32_t len);
    void     (*write_nocheck)(const uint8_t *buf, uint32_t addr, uint32_t len);
    void     (*write)(const uint8_t *buf, uint32_t addr, uint32_t len);
    void     (*erase_sector)(uint32_t addr);
    void     (*erase_block)(uint32_t addr);
    uint8_t  (*is_busy)(void);
} HAL_Flash_Drv_t;

/**
 * @brief 全局 Flash 驱动指针
 *
 * 在平台初始化代码中赋值：
 *   HAL_Flash = &W25Q128_Drv;
 */
extern const HAL_Flash_Drv_t *HAL_Flash;

/* ──── 驱动实例声明 ──── */

#if FLASH_DRIVER_W25Q128
extern const HAL_Flash_Drv_t W25Q128_Drv;
#endif

#if FLASH_DRIVER_W25Q256
extern const HAL_Flash_Drv_t W25Q256_Drv;
#endif

#if FLASH_DRIVER_INTERNAL
extern const HAL_Flash_Drv_t InternalFlash_Drv;
#endif

#endif /* __HAL_FLASH_H */
