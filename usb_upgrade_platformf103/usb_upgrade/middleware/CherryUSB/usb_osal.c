/*
 * CherryUSB OSAL Layer
 * 支持裸机 和 uCOS-II，通过 CONFIG_USB_USE_UCOS2 宏切换
 *
 * 切换方式：
 *   方式1：usb_config.h 中注释/取消注释 CONFIG_USB_USE_UCOS2
 *   方式2：编译器预定义 -DCONFIG_USB_USE_UCOS2
 */

#include "usb_osal.h"
#include "usb_config.h"

/* ══════════════════════════════════════════════════════════════
 * 裸机模式
 * ══════════════════════════════════════════════════════════════ */
#ifndef CONFIG_USB_USE_UCOS2

#include "hal_config.h"

/* 临界区 */
size_t usb_osal_enter_critical_section(void)
{
    size_t flag = __get_PRIMASK();
    __disable_irq();
    return flag;
}

void usb_osal_leave_critical_section(size_t flag)
{
    __set_PRIMASK((uint32_t)flag);
}

/* 延时 */
void usb_osal_msleep(uint32_t delay)
{
    HAL_Tick->delay_ms(delay);
}

/* 裸机模式下以下函数不会被调用，空实现 */
usb_osal_thread_t usb_osal_thread_create(const char *name, uint32_t stack_size,
                                          uint32_t prio, usb_thread_entry_t entry, void *args)
{
    (void)name; (void)stack_size; (void)prio; (void)entry; (void)args;
    return NULL;
}

void usb_osal_thread_delete(usb_osal_thread_t thread) { (void)thread; }
void usb_osal_thread_schedule_other(void) {}

usb_osal_sem_t usb_osal_sem_create(uint32_t initial_count) { (void)initial_count; return NULL; }
usb_osal_sem_t usb_osal_sem_create_counting(uint32_t max_count) { (void)max_count; return NULL; }
void usb_osal_sem_delete(usb_osal_sem_t sem) { (void)sem; }
int usb_osal_sem_take(usb_osal_sem_t sem, uint32_t timeout) { (void)sem; (void)timeout; return 0; }
int usb_osal_sem_give(usb_osal_sem_t sem) { (void)sem; return 0; }
void usb_osal_sem_reset(usb_osal_sem_t sem) { (void)sem; }

usb_osal_mutex_t usb_osal_mutex_create(void) { return NULL; }
void usb_osal_mutex_delete(usb_osal_mutex_t mutex) { (void)mutex; }
int usb_osal_mutex_take(usb_osal_mutex_t mutex) { (void)mutex; return 0; }
int usb_osal_mutex_give(usb_osal_mutex_t mutex) { (void)mutex; return 0; }

usb_osal_mq_t usb_osal_mq_create(uint32_t max_msgs) { (void)max_msgs; return NULL; }
void usb_osal_mq_delete(usb_osal_mq_t mq) { (void)mq; }
int usb_osal_mq_send(usb_osal_mq_t mq, uintptr_t addr) { (void)mq; (void)addr; return 0; }
int usb_osal_mq_recv(usb_osal_mq_t mq, uintptr_t *addr, uint32_t timeout) { (void)mq; (void)addr; (void)timeout; return 0; }

struct usb_osal_timer *usb_osal_timer_create(const char *name, uint32_t timeout_ms,
                                              usb_timer_handler_t handler, void *argument, bool is_period)
{
    (void)name; (void)timeout_ms; (void)handler; (void)argument; (void)is_period;
    return NULL;
}
void usb_osal_timer_delete(struct usb_osal_timer *timer) { (void)timer; }
void usb_osal_timer_start(struct usb_osal_timer *timer) { (void)timer; }
void usb_osal_timer_stop(struct usb_osal_timer *timer) { (void)timer; }

void *usb_osal_malloc(size_t size) { (void)size; return NULL; }
void usb_osal_free(void *ptr) { (void)ptr; }


/* ══════════════════════════════════════════════════════════════
 * uCOS-II 模式
 * ══════════════════════════════════════════════════════════════ */
#else /* CONFIG_USB_USE_UCOS2 */

#include "os.h"
#include <stdlib.h>

/*
 * 静态任务栈（避免在中断中 malloc）
 * 大小由 usb_config.h 中 USB_UCOS_EP0_STK_SIZE / USB_UCOS_MSC_STK_SIZE 控制
 * 单位：OS_STK 个数（1个 = 4字节 on ARM）
 */
static OS_STK usb_ep0_task_stk[USB_UCOS_EP0_STK_SIZE];
static OS_STK usb_msc_task_stk[USB_UCOS_MSC_STK_SIZE];

/* ──── 临界区 ──── */

size_t usb_osal_enter_critical_section(void)
{
    OS_CPU_SR cpu_sr = 0;
    OS_ENTER_CRITICAL();
    return (size_t)cpu_sr;
}

void usb_osal_leave_critical_section(size_t flag)
{
    (void)flag;
    OS_EXIT_CRITICAL();
}

/* ──── 延时 ──── */

void usb_osal_msleep(uint32_t delay)
{
    if (delay == 0) delay = 1;
    OSTimeDly((delay * OS_TICKS_PER_SEC + 999) / 1000);
}

/* ──── 线程 ──── */

usb_osal_thread_t usb_osal_thread_create(const char *name, uint32_t stack_size,
                                          uint32_t prio, usb_thread_entry_t entry, void *args)
{
    INT8U err;
    INT8U os_prio;
    OS_STK *stk;
    uint32_t stk_size;

    /* CherryUSB prio: 数字越小优先级越高，和 uCOS-II 一致 */
    os_prio = (INT8U)prio;
    if (os_prio >= OS_LOWEST_PRIO) os_prio = OS_LOWEST_PRIO - 1;

    /*
     * 根据 CherryUSB 请求的栈大小（字节）选择预分配的静态栈
     * USB_UCOS_EP0_STK_SIZE 和 USB_UCOS_MSC_STK_SIZE 是 OS_STK 个数
     * 转换为字节：个数 * sizeof(OS_STK)
     *
     * EP0 栈用于较小的任务（EP0 处理），MSC 栈用于较大的任务（MSC 读写）
     * 如果请求的栈超过 MSC 栈大小，回退到 malloc
     */
    uint32_t ep0_bytes = USB_UCOS_EP0_STK_SIZE * sizeof(OS_STK);
    uint32_t msc_bytes = USB_UCOS_MSC_STK_SIZE * sizeof(OS_STK);

    if (stack_size <= ep0_bytes) {
        stk = usb_ep0_task_stk;
        stk_size = USB_UCOS_EP0_STK_SIZE;
    } else if (stack_size <= msc_bytes) {
        stk = usb_msc_task_stk;
        stk_size = USB_UCOS_MSC_STK_SIZE;
    } else {
        /* 超出预分配范围，回退到 malloc */
        stk_size = stack_size / sizeof(OS_STK);
        if (stk_size < 64) stk_size = 64;
        stk = (OS_STK *)malloc(stk_size * sizeof(OS_STK));
        if (stk == NULL) return NULL;
    }

    err = OSTaskCreate(entry, args, &stk[stk_size - 1], os_prio);
    if (err != OS_ERR_NONE) {
        return NULL;
    }

    return (usb_osal_thread_t)(uintptr_t)os_prio;
}

void usb_osal_thread_delete(usb_osal_thread_t thread)
{
    INT8U prio = (INT8U)(uintptr_t)thread;
    if (prio == OSPrioCur) {
        OSTaskDel(OS_PRIO_SELF);
    } else {
        OSTaskDel(prio);
    }
}

void usb_osal_thread_schedule_other(void)
{
    OSTimeDly(1);
}

/* ──── 信号量 ──── */

usb_osal_sem_t usb_osal_sem_create(uint32_t initial_count)
{
    return (usb_osal_sem_t)OSSemCreate((INT16U)initial_count);
}

usb_osal_sem_t usb_osal_sem_create_counting(uint32_t max_count)
{
    (void)max_count;
    return (usb_osal_sem_t)OSSemCreate(0);
}

void usb_osal_sem_delete(usb_osal_sem_t sem)
{
    INT8U err;
    OSSemDel((OS_EVENT *)sem, OS_DEL_ALWAYS, &err);
}

int usb_osal_sem_take(usb_osal_sem_t sem, uint32_t timeout)
{
    INT8U err;
    INT32U ticks;

    if (timeout == USB_OSAL_WAITING_FOREVER) {
        ticks = 0;
    } else {
        ticks = (timeout * OS_TICKS_PER_SEC + 999) / 1000;
        if (ticks == 0) ticks = 1;
    }

    OSSemPend((OS_EVENT *)sem, ticks, &err);
    return (err == OS_ERR_NONE) ? 0 : -1;
}

int usb_osal_sem_give(usb_osal_sem_t sem)
{
    return (OSSemPost((OS_EVENT *)sem) == OS_ERR_NONE) ? 0 : -1;
}

void usb_osal_sem_reset(usb_osal_sem_t sem)
{
    INT8U err;
    OSSemDel((OS_EVENT *)sem, OS_DEL_ALWAYS, &err);
    (void)err;
}

/* ──── 互斥锁 ──── */

usb_osal_mutex_t usb_osal_mutex_create(void)
{
    INT8U err;
    return (usb_osal_mutex_t)OSMutexCreate(OS_PRIO_MUTEX_CEIL_DIS, &err);
}

void usb_osal_mutex_delete(usb_osal_mutex_t mutex)
{
    INT8U err;
    OSMutexDel((OS_EVENT *)mutex, OS_DEL_ALWAYS, &err);
}

int usb_osal_mutex_take(usb_osal_mutex_t mutex)
{
    INT8U err;
    OSMutexPend((OS_EVENT *)mutex, 0, &err);
    return (err == OS_ERR_NONE) ? 0 : -1;
}

int usb_osal_mutex_give(usb_osal_mutex_t mutex)
{
    return (OSMutexPost((OS_EVENT *)mutex) == OS_ERR_NONE) ? 0 : -1;
}

/* ──── 消息队列 ──── */

#define USB_MQ_MAX_SLOTS  16
static void *usb_mq_slots[USB_MQ_MAX_SLOTS];
static uint8_t usb_mq_slot_used = 0;

usb_osal_mq_t usb_osal_mq_create(uint32_t max_msgs)
{
    void *q_start;
    if (max_msgs > USB_MQ_MAX_SLOTS) max_msgs = USB_MQ_MAX_SLOTS;
    if (usb_mq_slot_used) return NULL;
    q_start = usb_mq_slots;
    usb_mq_slot_used = 1;
    return (usb_osal_mq_t)OSQCreate(q_start, max_msgs);
}

void usb_osal_mq_delete(usb_osal_mq_t mq)
{
    INT8U err;
    OSQDel((OS_EVENT *)mq, OS_DEL_ALWAYS, &err);
    usb_mq_slot_used = 0;
}

int usb_osal_mq_send(usb_osal_mq_t mq, uintptr_t addr)
{
    return (OSQPost((OS_EVENT *)mq, (void *)addr) == OS_ERR_NONE) ? 0 : -1;
}

int usb_osal_mq_recv(usb_osal_mq_t mq, uintptr_t *addr, uint32_t timeout)
{
    INT8U err;
    INT32U ticks;

    if (timeout == USB_OSAL_WAITING_FOREVER) {
        ticks = 0;
    } else {
        ticks = (timeout * OS_TICKS_PER_SEC + 999) / 1000;
        if (ticks == 0) ticks = 1;
    }

    *addr = (uintptr_t)OSQPend((OS_EVENT *)mq, ticks, &err);
    return (err == OS_ERR_NONE) ? 0 : -1;
}

/* ──── 定时器（uCOS-II 软件定时器，需要 OS_TMR_EN=1） ──── */

static void _usb_timer_callback(void *p_tmr, void *p_arg)
{
    (void)p_tmr;
    struct usb_osal_timer *timer = (struct usb_osal_timer *)p_arg;
    if (timer && timer->handler) {
        timer->handler(timer->argument);
    }
}

struct usb_osal_timer *usb_osal_timer_create(const char *name, uint32_t timeout_ms,
                                              usb_timer_handler_t handler, void *argument, bool is_period)
{
    struct usb_osal_timer *timer;
    INT8U err;
    INT32U ticks;

    (void)name;

    timer = (struct usb_osal_timer *)malloc(sizeof(struct usb_osal_timer));
    if (timer == NULL) return NULL;

    timer->handler = handler;
    timer->argument = argument;
    timer->is_period = is_period;
    timer->timeout_ms = timeout_ms;

    ticks = (timeout_ms * OS_TICKS_PER_SEC + 999) / 1000;
    if (ticks == 0) ticks = 1;

    timer->timer = (void *)OSTmrCreate(ticks,
                                         is_period ? ticks : 0,
                                         is_period ? OS_TMR_OPT_PERIODIC : OS_TMR_OPT_ONE_SHOT,
                                         _usb_timer_callback,
                                         (void *)timer,
                                         (INT8U *)name,
                                         &err);
    if (err != OS_ERR_NONE) {
        free(timer);
        return NULL;
    }

    return timer;
}

void usb_osal_timer_delete(struct usb_osal_timer *timer)
{
    INT8U err;
    if (timer == NULL) return;
    OSTmrStop((OS_TMR *)timer->timer, OS_TMR_OPT_NONE, NULL, &err);
    OSTmrDel((OS_TMR *)timer->timer, &err);
    free(timer);
}

void usb_osal_timer_start(struct usb_osal_timer *timer)
{
    INT8U err;
    if (timer == NULL) return;
    OSTmrStart((OS_TMR *)timer->timer, &err);
}

void usb_osal_timer_stop(struct usb_osal_timer *timer)
{
    INT8U err;
    if (timer == NULL) return;
    OSTmrStop((OS_TMR *)timer->timer, OS_TMR_OPT_NONE, NULL, &err);
}

/* ──── 内存分配 ──── */

void *usb_osal_malloc(size_t size)
{
    return malloc(size);
}

void usb_osal_free(void *ptr)
{
    free(ptr);
}

#endif /* CONFIG_USB_USE_UCOS2 */
