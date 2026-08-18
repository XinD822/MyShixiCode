/**
 * @file flash_service.h
 * @brief Flash 读写服务（封装 Flash 驱动 + 4KB 缓存 + 互斥）
 *
 * 业务代码通过此接口访问 Flash，不直接调用 Flash 驱动。
 */

#ifndef __FLASH_SERVICE_H
#define __FLASH_SERVICE_H

#include <stdint.h>

void     FlashService_Init(void);
void     FlashService_Read(uint8_t *buf, uint32_t addr, uint32_t len);
void     FlashService_Write(const uint8_t *buf, uint32_t addr, uint32_t len);
void     FlashService_EraseSector(uint32_t addr);
void     FlashService_EraseBlock(uint32_t addr);
void     FlashService_FlushCache(void);
void     FlashService_InvalidateCache(void);
uint8_t  FlashService_IsCacheDirty(void);

#endif /* __FLASH_SERVICE_H */
