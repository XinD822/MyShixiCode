/**
 * @file mutex.c
 * @brief 互斥锁模块实现（支持裸机/uCOS-II双模式）
 *
 * 解耦方式：
 *   不依赖 CherryUSB 头文件
 *   模式由 mutex.h 自动检测或通过 CONFIG_USB_USE_UCOS2 宏控制
 */

#include "mutex.h"
#include "Delay.h"

/* ══════════════════════════════════════════════════════════════
 * 裸机模式
 * ══════════════════════════════════════════════════════════════ */
#ifndef CONFIG_USB_USE_UCOS2

void Mutex_Init(Mutex_t *mutex)
{
    mutex->locked = 0;
    mutex->owner = 0;
}

uint8_t Mutex_Lock(Mutex_t *mutex, uint32_t timeout_ms)
{
    uint32_t start = Delay_GetTick();

    while (1) {
        if (mutex->locked == 0) {
            uint32_t primask = __get_PRIMASK();
            __disable_irq();
            if (mutex->locked == 0) {
                mutex->locked = 1;
                mutex->owner = 1;
                __set_PRIMASK(primask);
                return 1;
            }
            __set_PRIMASK(primask);
        }
        if ((Delay_GetTick() - start) >= timeout_ms) {
            return 0;
        }
        Delay_ms(1);
    }
}

void Mutex_Unlock(Mutex_t *mutex)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    mutex->locked = 0;
    mutex->owner = 0;
    __set_PRIMASK(primask);
}

uint8_t Mutex_TryLock(Mutex_t *mutex)
{
    if (mutex->locked == 0) {
        uint32_t primask = __get_PRIMASK();
        __disable_irq();
        if (mutex->locked == 0) {
            mutex->locked = 1;
            mutex->owner = 1;
            __set_PRIMASK(primask);
            return 1;
        }
        __set_PRIMASK(primask);
    }
    return 0;
}

uint8_t Mutex_IsLocked(Mutex_t *mutex)
{
    return mutex->locked;
}


/* ══════════════════════════════════════════════════════════════
 * uCOS-II 模式
 * ══════════════════════════════════════════════════════════════ */
#else /* CONFIG_USB_USE_UCOS2 */

void Mutex_Init(Mutex_t *mutex)
{
    INT8U err;
    mutex->os_mutex = OSMutexCreate(OS_PRIO_MUTEX_CEIL_DIS, &err);
    mutex->locked = 0;
}

uint8_t Mutex_Lock(Mutex_t *mutex, uint32_t timeout_ms)
{
    INT8U err;
    INT32U ticks;

    if (timeout_ms == 0xFFFFFFFF) {
        ticks = 0;  /* 永久等待 */
    } else {
        ticks = (timeout_ms * OS_TICKS_PER_SEC + 999) / 1000;
        if (ticks == 0) ticks = 1;
    }

    OSMutexPend(mutex->os_mutex, ticks, &err);
    if (err == OS_ERR_NONE) {
        mutex->locked = 1;
        return 1;
    }
    return 0;
}

void Mutex_Unlock(Mutex_t *mutex)
{
    mutex->locked = 0;
    OSMutexPost(mutex->os_mutex);
}

uint8_t Mutex_TryLock(Mutex_t *mutex)
{
    INT8U err;
    OSMutexPend(mutex->os_mutex, 1, &err);
    if (err == OS_ERR_NONE) {
        mutex->locked = 1;
        return 1;
    }
    return 0;
}

uint8_t Mutex_IsLocked(Mutex_t *mutex)
{
    return mutex->locked;
}

#endif /* CONFIG_USB_USE_UCOS2 */
