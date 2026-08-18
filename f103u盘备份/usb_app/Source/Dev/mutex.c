/**
 * @file mutex.c
 * @brief 互斥锁模块实现
 * 
 * 功能说明：
 *   使用关中断实现简单的互斥锁
 *   适用于裸机环境下的资源共享保护
 * 
 * 实现原理：
 *   1. 加锁时关中断，检查并设置locked标志
 *   2. 如果锁已被占用，开中断后等待重试
 *   3. 解锁时开中断
 * 
 * 注意事项：
 *   - 持锁时间不能太长，否则会影响中断响应
 *   - 不支持嵌套加锁
 *   - 不支持优先级继承
 */

#include "mutex.h"
#include "Delay.h"

/**
 * @brief 初始化互斥锁
 * 
 * @param mutex 互斥锁指针
 */
void Mutex_Init(Mutex_t *mutex)
{
    mutex->locked = 0;
    mutex->owner = 0;
}

/**
 * @brief 加锁（带超时）
 * 
 * 实现流程：
 *   1. 检查锁是否可用
 *   2. 关中断（原子操作）
 *   3. 再次检查并设置锁
 *   4. 开中断
 *   5. 如果获取失败，等待1ms后重试
 *   6. 超时返回失败
 * 
 * @param mutex 互斥锁指针
 * @param timeout_ms 超时时间（毫秒）
 * @return 1=成功，0=超时
 */
uint8_t Mutex_Lock(Mutex_t *mutex, uint32_t timeout_ms)
{
    uint32_t start = Delay_GetTick();
    
    while (1) {
        /* 快速检查：如果锁可用 */
        if (mutex->locked == 0) {
            /* 关中断，原子操作 */
            uint32_t primask = __get_PRIMASK();
            __disable_irq();
            
            /* 再次检查（防止竞态条件） */
            if (mutex->locked == 0) {
                mutex->locked = 1;
                mutex->owner = 1;  /* USB持有 */
                __set_PRIMASK(primask);
                return 1;  /* 成功 */
            }
            __set_PRIMASK(primask);
        }
        
        /* 检查超时 */
        if ((Delay_GetTick() - start) >= timeout_ms) {
            return 0;  /* 超时 */
        }
        
        /* 短暂等待后重试 */
        Delay_ms(1);
    }
}

/**
 * @brief 解锁
 * 
 * 使用关中断实现原子操作
 * 
 * @param mutex 互斥锁指针
 */
void Mutex_Unlock(Mutex_t *mutex)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    
    mutex->locked = 0;
    mutex->owner = 0;
    
    __set_PRIMASK(primask);
}

/**
 * @brief 尝试加锁（非阻塞）
 * 
 * 如果锁已被占用，立即返回0
 * 
 * @param mutex 互斥锁指针
 * @return 1=成功，0=已被占用
 */
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

/**
 * @brief 检查锁状态
 * 
 * @param mutex 互斥锁指针
 * @return 1=已锁，0=未锁
 */
uint8_t Mutex_IsLocked(Mutex_t *mutex)
{
    return mutex->locked;
}
