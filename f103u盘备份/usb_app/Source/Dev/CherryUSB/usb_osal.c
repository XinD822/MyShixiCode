/*
 * CherryUSB OSAL Layer - Baremetal Version
 * For STM32F103 without RTOS
 * 
 * Later when switching to uCOS-II, replace the empty functions
 * with actual OS calls.
 */

#include "usb_osal.h"
#include "usb_config.h"
#include "stm32f10x.h"

/* ──── 临界区保护（裸机：关中断） ──── */

size_t usb_osal_enter_critical_section(void)
{
    size_t flag = __get_PRIMASK();
    __disable_irq();
    return flag;
}

void usb_osal_leave_critical_section(size_t flag)
{
    __set_PRIMASK(flag);
}

/* ──── 延时函数 ──── */

extern void Delay_ms(uint32_t ms);

void usb_osal_msleep(uint32_t delay)
{
    Delay_ms(delay);
}

/* ──── 以下函数在裸机模式下不会被调用 ──── */
/* ──── 后期上uCOS-II时替换为对应的OS函数 ──── */

// 线程（裸机不需要）
usb_osal_thread_t usb_osal_thread_create(const char *name, uint32_t stack_size,
                                          uint32_t prio, usb_thread_entry_t entry, void *args)
{
    (void)name; (void)stack_size; (void)prio; (void)entry; (void)args;
    return NULL;
}

void usb_osal_thread_delete(usb_osal_thread_t thread) { (void)thread; }
void usb_osal_thread_schedule_other(void) {}

// 信号量（裸机不需要）
usb_osal_sem_t usb_osal_sem_create(uint32_t initial_count) { (void)initial_count; return NULL; }
usb_osal_sem_t usb_osal_sem_create_counting(uint32_t max_count) { (void)max_count; return NULL; }
void usb_osal_sem_delete(usb_osal_sem_t sem) { (void)sem; }
int usb_osal_sem_take(usb_osal_sem_t sem, uint32_t timeout) { (void)sem; (void)timeout; return 0; }
int usb_osal_sem_give(usb_osal_sem_t sem) { (void)sem; return 0; }
void usb_osal_sem_reset(usb_osal_sem_t sem) { (void)sem; }

// 互斥锁（裸机不需要）
usb_osal_mutex_t usb_osal_mutex_create(void) { return NULL; }
void usb_osal_mutex_delete(usb_osal_mutex_t mutex) { (void)mutex; }
int usb_osal_mutex_take(usb_osal_mutex_t mutex) { (void)mutex; return 0; }
int usb_osal_mutex_give(usb_osal_mutex_t mutex) { (void)mutex; return 0; }

// 消息队列（裸机不需要）
usb_osal_mq_t usb_osal_mq_create(uint32_t max_msgs) { (void)max_msgs; return NULL; }
void usb_osal_mq_delete(usb_osal_mq_t mq) { (void)mq; }
int usb_osal_mq_send(usb_osal_mq_t mq, uintptr_t addr) { (void)mq; (void)addr; return 0; }
int usb_osal_mq_recv(usb_osal_mq_t mq, uintptr_t *addr, uint32_t timeout) { (void)mq; (void)addr; (void)timeout; return 0; }

// 定时器（裸机不需要）
struct usb_osal_timer *usb_osal_timer_create(const char *name, uint32_t timeout_ms,
                                              usb_timer_handler_t handler, void *argument, bool is_period)
{
    (void)name; (void)timeout_ms; (void)handler; (void)argument; (void)is_period;
    return NULL;
}
void usb_osal_timer_delete(struct usb_osal_timer *timer) { (void)timer; }
void usb_osal_timer_start(struct usb_osal_timer *timer) { (void)timer; }
void usb_osal_timer_stop(struct usb_osal_timer *timer) { (void)timer; }

// 内存分配（裸机不需要）
void *usb_osal_malloc(size_t size) { (void)size; return NULL; }
void usb_osal_free(void *ptr) { (void)ptr; }
