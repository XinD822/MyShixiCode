/**
 * @file mutex.h
 * @brief 互斥锁模块头文件
 * 
 * 功能说明：
 *   实现简单的互斥锁，用于保护共享资源
 *   主要用于保护SPI Flash的并发访问
 * 
 * 使用场景：
 *   USB MSC和FatFS都需要访问W25Q128
 *   如果同时访问会导致数据损坏
 *   使用互斥锁确保同一时间只有一个模块访问
 * 
 * 锁的持有者：
 *   0 = 无
 *   1 = USB MSC
 *   2 = FatFS
 */

#ifndef __MUTEX_H__
#define __MUTEX_H__

#include "stm32f10x.h"

/**
 * @brief 互斥锁结构体
 */
typedef struct {
    volatile uint8_t locked;    /* 锁状态：0=未锁，1=已锁 */
    volatile uint8_t owner;     /* 持有者ID（0=无，1=USB，2=FatFS） */
} Mutex_t;

/**
 * @brief 初始化互斥锁
 * 
 * @param mutex 互斥锁指针
 */
void Mutex_Init(Mutex_t *mutex);

/**
 * @brief 加锁（带超时）
 * 
 * 使用关中断实现原子操作
 * 如果锁已被占用，会等待直到超时
 * 
 * @param mutex 互斥锁指针
 * @param timeout_ms 超时时间（毫秒）
 * @return 1=成功，0=超时
 */
uint8_t Mutex_Lock(Mutex_t *mutex, uint32_t timeout_ms);

/**
 * @brief 解锁
 * 
 * 使用关中断实现原子操作
 * 
 * @param mutex 互斥锁指针
 */
void Mutex_Unlock(Mutex_t *mutex);

/**
 * @brief 尝试加锁（非阻塞）
 * 
 * 如果锁已被占用，立即返回0
 * 
 * @param mutex 互斥锁指针
 * @return 1=成功，0=已被占用
 */
uint8_t Mutex_TryLock(Mutex_t *mutex);

/**
 * @brief 检查锁状态
 * 
 * @param mutex 互斥锁指针
 * @return 1=已锁，0=未锁
 */
uint8_t Mutex_IsLocked(Mutex_t *mutex);

#endif /* __MUTEX_H__ */
