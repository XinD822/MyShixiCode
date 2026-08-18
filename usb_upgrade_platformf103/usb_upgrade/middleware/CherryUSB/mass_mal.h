/**
 * @file mass_mal.h
 * @brief Medium Access Layer 接口（HAL 解耦版）
 */

#ifndef __MASS_MAL_H
#define __MASS_MAL_H

#include <stdint.h>

#define MAL_OK    0
#define MAL_FAIL  1
#define MAX_LUN   1

uint16_t MAL_Init(uint8_t lun);
uint16_t MAL_GetStatus(uint8_t lun);
uint16_t MAL_Read(uint8_t lun, uint32_t offset, uint32_t *buf, uint16_t len);
uint16_t MAL_Write(uint8_t lun, uint32_t offset, uint32_t *buf, uint16_t len);
void     MAL_FlushCache(void);

extern uint32_t Mass_Memory_Size[];
extern uint32_t Mass_Block_Size[];
extern uint32_t Mass_Block_Count[];

#endif /* __MASS_MAL_H */
