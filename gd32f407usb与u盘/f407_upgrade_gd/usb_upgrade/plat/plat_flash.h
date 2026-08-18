/**
 * @file plat_flash.h
 * @brief SPI NOR Flash 抽象接口
 */
#ifndef __PLAT_FLASH_H
#define __PLAT_FLASH_H
#include "plat_config.h"
#include <stdint.h>

uint8_t  plat_flash_init(void);
uint16_t plat_flash_read_id(void);
void     plat_flash_read(uint8_t *buf, uint32_t addr, uint32_t len);
void     plat_flash_write_nocheck(const uint8_t *buf, uint32_t addr, uint32_t len);
void     plat_flash_erase_sector(uint32_t addr);   /* 4KB */
void     plat_flash_erase_block(uint32_t addr);    /* 64KB */
void     plat_flash_reset(void);

#endif /* __PLAT_FLASH_H */
