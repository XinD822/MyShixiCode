/**
 * @file plat_timer.c
 * @brief 定时器回调机制实现 — 极简轮询式
 */
#include "plat_timer.h"
#include <string.h>

#define PLAT_TIMER_MAX  4

typedef struct {
    plat_timer_cb_t cb;
    void            *arg;
    uint32_t         period_ms;
    uint32_t         counter;
    uint8_t          active;
} plat_timer_slot_t;

static plat_timer_slot_t s_slots[PLAT_TIMER_MAX];

int plat_timer_register(plat_timer_cb_t cb, void *arg, uint32_t period_ms)
{
    int i;
    for (i = 0; i < PLAT_TIMER_MAX; i++) {
        if (!s_slots[i].active) {
            s_slots[i].cb = cb;
            s_slots[i].arg = arg;
            s_slots[i].period_ms = period_ms;
            s_slots[i].counter = 0;
            s_slots[i].active = 1;
            return i;
        }
    }
    return -1;
}

void plat_timer_unregister(int handle)
{
    if (handle >= 0 && handle < PLAT_TIMER_MAX)
        s_slots[handle].active = 0;
}

void plat_timer_poll(void)
{
    int i;
    for (i = 0; i < PLAT_TIMER_MAX; i++) {
        if (s_slots[i].active && s_slots[i].cb) {
            s_slots[i].counter++;
            if (s_slots[i].counter >= s_slots[i].period_ms) {
                s_slots[i].counter = 0;
                s_slots[i].cb(s_slots[i].arg);
            }
        }
    }
}
