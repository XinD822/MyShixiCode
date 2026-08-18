/**
 * @file upgrade_source.h
 * @brief 升级来源抽象接口
 *
 * USB 拖拽、SD 卡、U 盘各自实现一套，升级状态机通过此接口调用。
 */

#ifndef __UPGRADE_SOURCE_H
#define __UPGRADE_SOURCE_H

#include "ff.h"
#include <stdint.h>

typedef struct {
    const char *name;           /* 来源名称 */
    uint8_t  (*detect)(void);   /* 检测到固件文件？ */
    FRESULT  (*open)(FIL *file, uint32_t *size);   /* 打开固件 */
    FRESULT  (*read)(FIL *file, void *buf, UINT btr, UINT *br);
    void     (*close)(FIL *file);
    void     (*cleanup)(void);  /* 清理（删文件等） */
} UpgradeSource_t;

/* 注册当前升级来源 */
void UpgradeSource_Register(const UpgradeSource_t *src);
const UpgradeSource_t *UpgradeSource_Get(void);

/* ──── 预定义来源声明 ──── */

#if UPGRADE_SRC_USB_DRAG
extern const UpgradeSource_t Source_USB_Drag;
#endif

#if UPGRADE_SRC_SD_CARD
extern const UpgradeSource_t Source_SD_Card;
#endif

#if UPGRADE_SRC_USB_DRIVE
extern const UpgradeSource_t Source_USB_Drive;
#endif

#endif /* __UPGRADE_SOURCE_H */
