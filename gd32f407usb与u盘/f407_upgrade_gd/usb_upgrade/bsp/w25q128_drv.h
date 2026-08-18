/**
 * @file w25q128_drv.h
 * @brief W25Q128 SPI Flash 驱动接口（直接函数调用，无函数指针）
 */

#ifndef __W25Q128_DRV_H
#define __W25Q128_DRV_H

#include <stdint.h>

void     Flash_Init(void);
uint16_t Flash_ReadID(void);
void     Flash_Read(uint8_t *buf, uint32_t addr, uint32_t len);
void     Flash_WriteNoCheck(const uint8_t *buf, uint32_t addr, uint32_t len);
void     Flash_Write(const uint8_t *buf, uint32_t addr, uint32_t len);
void     Flash_EraseSector(uint32_t addr);
void     Flash_EraseBlock(uint32_t addr);
uint8_t  Flash_IsBusy(void);
void     Flash_Reset(void);

#endif /* __W25Q128_DRV_H */
