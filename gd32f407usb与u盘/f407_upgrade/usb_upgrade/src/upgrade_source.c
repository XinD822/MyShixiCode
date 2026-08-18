/**
 * @file upgrade_source.c
 * @brief 升级来源注册管理
 */

#include "upgrade_source.h"
#include <stddef.h>

static const UpgradeSource_t *g_current_source = NULL;

void UpgradeSource_Register(const UpgradeSource_t *src)
{
    g_current_source = src;
}

const UpgradeSource_t *UpgradeSource_Get(void)
{
    return g_current_source;
}
