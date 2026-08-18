/**
 * @file flash_service.c
 * @brief Flash 读写服务实现
 *
 * 封装 HAL_Flash，提供 4KB 缓存和互斥保护。
 * 业务代码不直接调 HAL_Flash，统一通过此服务。
 */

#include "flash_service.h"
#include "hal_config.h"
#include "mutex.h"
#include <string.h>

static uint8_t  svc_cache[4096];
static uint32_t svc_cache_addr = 0xFFFFFFFF;
static uint8_t  svc_cache_dirty = 0;
static Mutex_t  svc_mutex;

void FlashService_Init(void)
{
    Mutex_Init(&svc_mutex);
    svc_cache_addr = 0xFFFFFFFF;
    svc_cache_dirty = 0;
}

void FlashService_Read(uint8_t *buf, uint32_t addr, uint32_t len)
{
    if (!Mutex_Lock(&svc_mutex, 1000)) return;

    uint32_t cache_page = addr & ~0xFFF;
    uint16_t offset = addr & 0xFFF;

    /* 如果读取的数据在缓存中且缓存有脏数据，从缓存读 */
    if (cache_page == svc_cache_addr && svc_cache_dirty) {
        uint32_t copy_len = (len > (4096 - offset)) ? (4096 - offset) : len;
        memcpy(buf, svc_cache + offset, copy_len);
        if (copy_len < len) {
            HAL_Flash->read(buf + copy_len, addr + copy_len, len - copy_len);
        }
    } else {
        HAL_Flash->read(buf, addr, len);
    }

    Mutex_Unlock(&svc_mutex);
}

void FlashService_Write(const uint8_t *buf, uint32_t addr, uint32_t len)
{
    if (!Mutex_Lock(&svc_mutex, 1000)) return;

    while (len > 0) {
        uint32_t cache_page = addr & ~0xFFF;
        uint16_t offset = addr & 0xFFF;
        uint32_t chunk = (len > (4096 - offset)) ? (4096 - offset) : len;

        /* 如果缓存页不同，先刷写旧缓存 */
        if (cache_page != svc_cache_addr) {
            if (svc_cache_dirty) {
                HAL_Flash->erase_sector(svc_cache_addr);
                HAL_Flash->write_nocheck(svc_cache, svc_cache_addr, 4096);
                svc_cache_dirty = 0;
            }
            HAL_Flash->read(svc_cache, cache_page, 4096);
            svc_cache_addr = cache_page;
        }

        memcpy(svc_cache + offset, buf, chunk);
        svc_cache_dirty = 1;

        buf += chunk;
        addr += chunk;
        len -= chunk;
    }

    Mutex_Unlock(&svc_mutex);
}

void FlashService_FlushCache(void)
{
    if (!Mutex_Lock(&svc_mutex, 1000)) return;

    if (svc_cache_dirty) {
        HAL_Flash->erase_sector(svc_cache_addr);
        HAL_Flash->write_nocheck(svc_cache, svc_cache_addr, 4096);
        svc_cache_dirty = 0;
    }

    Mutex_Unlock(&svc_mutex);
}

void FlashService_EraseSector(uint32_t addr)
{
    if (!Mutex_Lock(&svc_mutex, 1000)) return;

    /* 如果要擦除的扇区包含脏缓存页，先丢弃 */
    if (svc_cache_dirty && (svc_cache_addr == (addr & ~0xFFF))) {
        svc_cache_dirty = 0;
    }
    HAL_Flash->erase_sector(addr);

    Mutex_Unlock(&svc_mutex);
}

void FlashService_EraseBlock(uint32_t addr)
{
    if (!Mutex_Lock(&svc_mutex, 1000)) return;

    /* 如果要擦除的块包含脏缓存页，先丢弃 */
    if (svc_cache_dirty &&
        svc_cache_addr >= addr &&
        svc_cache_addr < addr + 65536) {
        svc_cache_dirty = 0;
    }
    HAL_Flash->erase_block(addr);

    Mutex_Unlock(&svc_mutex);
}

void FlashService_InvalidateCache(void)
{
    svc_cache_dirty = 0;
    svc_cache_addr = 0xFFFFFFFF;
}

uint8_t FlashService_IsCacheDirty(void)
{
    return svc_cache_dirty;
}
