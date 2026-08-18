/**
 * @file mutex.h
 * @brief 互斥锁模块头文件（支持裸机/uCOS-II双模式）
 *
 * 解耦方式：
 *   mutex.h 不依赖任何 CherryUSB 头文件
 *   模式切换通过全局宏 CONFIG_USB_USE_UCOS2 控制
 *   可以在编译器预定义，也可以在任意公共头文件中定义
 *
 *   裸机模式：不定义 CONFIG_USB_USE_UCOS2（默认）
 *   uCOS-II：定义 CONFIG_USB_USE_UCOS2
 */

#ifndef __MUTEX_H__
#define __MUTEX_H__

#include "hal_config.h"

/* ══════════════════════════════════════════════════════════════
 * 自动检测模式：如果已包含 uCOS-II 头文件，则自动启用 OS 模式
 * 也可以通过编译器 -DCONFIG_USB_USE_UCOS2 强制指定
 * ══════════════════════════════════════════════════════════════ */
#if !defined(CONFIG_USB_USE_UCOS2) && defined(OS_VERSION)
#define CONFIG_USB_USE_UCOS2
#endif

#ifdef CONFIG_USB_USE_UCOS2
#include "os.h"
#endif

/**
 * @brief 互斥锁结构体
 */
typedef struct {
#ifdef CONFIG_USB_USE_UCOS2
    OS_EVENT *os_mutex;         /* uCOS-II 互斥锁句柄 */
    volatile uint8_t locked;    /* 锁状态标记（用于 IsLocked 查询） */
#else
    volatile uint8_t locked;    /* 锁状态：0=未锁，1=已锁 */
    volatile uint8_t owner;     /* 持有者ID（0=无，1=USB，2=FatFS） */
#endif
} Mutex_t;

/**
 * @brief 初始化互斥锁
 */
void Mutex_Init(Mutex_t *mutex);

/**
 * @brief 加锁（带超时）
 * @param mutex 互斥锁指针
 * @param timeout_ms 超时时间（毫秒）
 * @return 1=成功，0=超时
 */
uint8_t Mutex_Lock(Mutex_t *mutex, uint32_t timeout_ms);

/**
 * @brief 解锁
 */
void Mutex_Unlock(Mutex_t *mutex);

/**
 * @brief 尝试加锁（非阻塞）
 * @return 1=成功，0=已被占用
 */
uint8_t Mutex_TryLock(Mutex_t *mutex);

/**
 * @brief 检查锁状态
 * @return 1=已锁，0=未锁
 */
uint8_t Mutex_IsLocked(Mutex_t *mutex);

#endif /* __MUTEX_H__ */
