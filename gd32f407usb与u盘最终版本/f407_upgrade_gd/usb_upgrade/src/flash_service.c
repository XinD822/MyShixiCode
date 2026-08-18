/**
 * @file flash_service.c
 * @brief Flash 读写服务实现
 *
 * 封装 plat_flash 驱动，提供 4KB 缓存和互斥保护。
 * 互斥锁内联实现（裸机：关中断原子操作；uCOS-II：OS_MUTEX）。
 */

#include "flash_service.h"
#include "plat_flash.h"
#include "plat_tick.h"
#include "plat_log.h"
#include <string.h>

/* ═══════════════════════════════════════════════════════════
 * 内联互斥锁（替代原 mutex.c/h）
 * ═══════════════════════════════════════════════════════════ */
#if (PLAT_SELECT == PLAT_UCOS2)
#include "os.h"

typedef struct {
    OS_EVENT    *os_mutex;
    volatile uint8_t locked;
} SvcMutex_t;

static void SvcMutex_Init(SvcMutex_t *m)
{
    INT8U err;
    m->os_mutex = OSMutexCreate(OS_PRIO_MUTEX_CEIL_DIS, &err);
    m->locked = 0;
}

static uint8_t SvcMutex_Lock(SvcMutex_t *m, uint32_t timeout_ms)
{
    INT8U err;
    INT32U ticks = (timeout_ms == 0xFFFFFFFF) ? 0 :
                   (timeout_ms * OS_TICKS_PER_SEC + 999) / 1000;
    if (ticks == 0) ticks = 1;
    OSMutexPend(m->os_mutex, ticks, &err);
    if (err == OS_ERR_NONE) { m->locked = 1; return 1; }
    return 0;
}

static void SvcMutex_Unlock(SvcMutex_t *m)
{
    m->locked = 0;
    OSMutexPost(m->os_mutex);
}

#else /* 裸机模式 */

typedef struct {
    volatile uint8_t locked;
} SvcMutex_t;

static void SvcMutex_Init(SvcMutex_t *m)
{
    m->locked = 0;
}

static uint8_t SvcMutex_Lock(SvcMutex_t *m, uint32_t timeout_ms)
{
    uint32_t start = plat_get_tick_ms();
    while (1) {
        if (m->locked == 0) {
            uint32_t primask = __get_PRIMASK();
            __disable_irq();
            if (m->locked == 0) {
                m->locked = 1;
                __set_PRIMASK(primask);
                return 1;
            }
            __set_PRIMASK(primask);
        }
        if ((plat_get_tick_ms() - start) >= timeout_ms) return 0;
        plat_delay_ms(1);
    }
}

static void SvcMutex_Unlock(SvcMutex_t *m)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    m->locked = 0;
    __set_PRIMASK(primask);
}

#endif /* PLAT_SELECT */

/* ═══════════════════════════════════════════════════════════
 * Flash Service 实现
 * ═══════════════════════════════════════════════════════════ */
static uint8_t  svc_cache[4096];
static uint32_t svc_cache_addr = 0xFFFFFFFF;
static uint8_t  svc_cache_dirty = 0;
static SvcMutex_t svc_mutex;

void FlashService_Init(void)
{
    SvcMutex_Init(&svc_mutex);
    svc_cache_addr = 0xFFFFFFFF;
    svc_cache_dirty = 0;
}

void FlashService_Read(uint8_t *buf, uint32_t addr, uint32_t len)
{
    if (!SvcMutex_Lock(&svc_mutex, 1000)) return;

    uint32_t cache_page = addr & ~0xFFF;
    uint16_t offset = addr & 0xFFF;

    if (cache_page == svc_cache_addr && svc_cache_dirty) {
        uint32_t copy_len = (len > (4096 - offset)) ? (4096 - offset) : len;
        memcpy(buf, svc_cache + offset, copy_len);
        if (copy_len < len) {
            plat_flash_read(buf + copy_len, addr + copy_len, len - copy_len);
        }
    } else {
        plat_flash_read(buf, addr, len);
    }

    SvcMutex_Unlock(&svc_mutex);
}

void FlashService_Write(const uint8_t *buf, uint32_t addr, uint32_t len)
{
    if (!SvcMutex_Lock(&svc_mutex, 1000)) return;

    while (len > 0) {
        uint32_t cache_page = addr & ~0xFFF;
        uint16_t offset = addr & 0xFFF;
        uint32_t chunk = (len > (4096 - offset)) ? (4096 - offset) : len;

        if (cache_page != svc_cache_addr) {
            if (svc_cache_dirty) {
                plat_flash_erase_sector(svc_cache_addr);
                plat_flash_write_nocheck(svc_cache, svc_cache_addr, 4096);
                svc_cache_dirty = 0;
            }
            plat_flash_read(svc_cache, cache_page, 4096);
            svc_cache_addr = cache_page;
        }

        memcpy(svc_cache + offset, buf, chunk);
        svc_cache_dirty = 1;

        buf += chunk;
        addr += chunk;
        len -= chunk;
    }

    SvcMutex_Unlock(&svc_mutex);
}

void FlashService_FlushCache(void)
{
    if (!SvcMutex_Lock(&svc_mutex, 1000)) return;

    if (svc_cache_dirty) {
        plat_flash_erase_sector(svc_cache_addr);
        plat_flash_write_nocheck(svc_cache, svc_cache_addr, 4096);
        svc_cache_dirty = 0;
    }

    SvcMutex_Unlock(&svc_mutex);
}

void FlashService_EraseSector(uint32_t addr)
{
    if (!SvcMutex_Lock(&svc_mutex, 1000)) return;

    if (svc_cache_dirty && (svc_cache_addr == (addr & ~0xFFF))) {
        svc_cache_dirty = 0;
    }
    plat_flash_erase_sector(addr);

    SvcMutex_Unlock(&svc_mutex);
}

void FlashService_EraseBlock(uint32_t addr)
{
    if (!SvcMutex_Lock(&svc_mutex, 1000)) return;

    if (svc_cache_dirty &&
        svc_cache_addr >= addr &&
        svc_cache_addr < addr + 65536) {
        svc_cache_dirty = 0;
    }
    plat_flash_erase_block(addr);

    SvcMutex_Unlock(&svc_mutex);
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
