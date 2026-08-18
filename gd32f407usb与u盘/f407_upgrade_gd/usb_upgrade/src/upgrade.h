/**
 * @file upgrade.h
 * @brief 升级模块接口
 */

#ifndef __UPGRADE_H
#define __UPGRADE_H

#include <stdint.h>

typedef enum {
    UPGRADE_IDLE = 0,
    UPGRADE_CHECKING,
    UPGRADE_READY,
    UPGRADE_BURNING,
    UPGRADE_DONE,
    UPGRADE_ERROR
} UpgradeState;

typedef enum {
    UPGRADE_OK = 0,
    UPGRADE_ERR_NO_FILE,
    UPGRADE_ERR_CHECK,
    UPGRADE_ERR_BURN,
    UPGRADE_ERR_STATE
} UpgradeResult;

void          Upgrade_Init(void);
UpgradeResult Upgrade_Check(void);
UpgradeResult Upgrade_Execute(void);
void          Upgrade_SetFlag(uint32_t flag, uint32_t size);
uint32_t      Upgrade_GetFlag(void);
UpgradeState  Upgrade_GetState(void);
UpgradeResult Upgrade_GetLastError(void);
uint32_t      Upgrade_GetFirmwareSize(void);

#endif /* __UPGRADE_H */
