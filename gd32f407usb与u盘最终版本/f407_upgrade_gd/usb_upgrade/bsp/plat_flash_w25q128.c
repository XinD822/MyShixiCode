/**
 * @file plat_flash_w25q128.c
 * @brief W25Q128 SPI Flash 实现 plat_flash 接口
 *        薄包装层：转发到已有的 w25q128_drv.c 驱动
 */
#include "plat_flash.h"
#include "w25q128_drv.h"

uint8_t plat_flash_init(void)
{
    Flash_Init();
    return 0;
}

uint16_t plat_flash_read_id(void)
{
    return Flash_ReadID();
}

void plat_flash_read(uint8_t *buf, uint32_t addr, uint32_t len)
{
    Flash_Read(buf, addr, len);
}

void plat_flash_write_nocheck(const uint8_t *buf, uint32_t addr, uint32_t len)
{
    Flash_WriteNoCheck(buf, addr, len);
}

void plat_flash_erase_sector(uint32_t addr)
{
    Flash_EraseSector(addr);
}

void plat_flash_erase_block(uint32_t addr)
{
    Flash_EraseBlock(addr);
}

void plat_flash_reset(void)
{
    Flash_Reset();
}
