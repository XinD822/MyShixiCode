/**
 * @file source_usb_drag.c
 * @brief USB 拖拽升级来源实现
 *
 * 实现 UpgradeSource_t 接口。
 * 通过 FatFS 访问 W25Q128 数据区中的固件文件。
 */

#include "upgrade_source.h"
#include "upgrade_config.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

#if UPGRADE_SRC_USB_DRAG

static const char *FIRMWARE_NAME = "firmware.bin";

static int strcasecmp_local(const char *s1, const char *s2)
{
    while (*s1 && *s2) {
        char c1 = (*s1 >= 'A' && *s1 <= 'Z') ? (*s1 + 32) : *s1;
        char c2 = (*s2 >= 'A' && *s2 <= 'Z') ? (*s2 + 32) : *s2;
        if (c1 != c2) return c1 - c2;
        s1++; s2++;
    }
    return *s1 - *s2;
}

static uint8_t usb_drag_detect(void)
{
    DIR dir;
    FILINFO fno;

    if (f_opendir(&dir, "0:/") != FR_OK) return 0;

    while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0]) {
        if (fno.fattrib & AM_DIR) continue;
        if (strcasecmp_local(fno.fname, FIRMWARE_NAME) == 0) {
            if (fno.fsize > 256 && fno.fsize <= FIRMWARE_MAX_SIZE) {
                f_closedir(&dir);
                return 1;
            }
        }
    }

    f_closedir(&dir);
    return 0;
}

static FRESULT usb_drag_open(FIL *file, uint32_t *size)
{
    DIR dir;
    FILINFO fno;
    char path[FIRMWARE_FILENAME_MAX + 4];

    if (f_opendir(&dir, "0:/") != FR_OK) return FR_NO_FILE;

    while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0]) {
        if (fno.fattrib & AM_DIR) continue;
        if (strcasecmp_local(fno.fname, FIRMWARE_NAME) == 0) {
            if (fno.fsize > 256 && fno.fsize <= FIRMWARE_MAX_SIZE) {
                *size = fno.fsize;
                snprintf(path, sizeof(path), "0:/%s", fno.fname);
                f_closedir(&dir);
                return f_open(file, path, FA_READ);
            }
        }
    }

    f_closedir(&dir);
    return FR_NO_FILE;
}

static FRESULT usb_drag_read(FIL *file, void *buf, UINT btr, UINT *br)
{
    return f_read(file, buf, btr, br);
}

static void usb_drag_close(FIL *file)
{
    f_close(file);
}

static void usb_drag_cleanup(void)
{
    DIR dir;
    FILINFO fno;

    if (f_opendir(&dir, "0:/") != FR_OK) {
        DBG_PRINTF("[UPG] cleanup opendir fail\r\n");
        return;
    }

    while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0]) {
        if (fno.fattrib & AM_DIR) continue;
        if (strcasecmp_local(fno.fname, FIRMWARE_NAME) == 0) {
            char path[64];
            snprintf(path, sizeof(path), "0:/%s", fno.fname);
            FRESULT res = f_unlink(path);
            DBG_PRINTF("[UPG] cleanup unlink %s -> %d\r\n", path, res);
            break;
        }
    }

    f_closedir(&dir);
}

const UpgradeSource_t Source_USB_Drag = {
    .name   = "usb_drag",
    .detect = usb_drag_detect,
    .open   = usb_drag_open,
    .read   = usb_drag_read,
    .close  = usb_drag_close,
    .cleanup = usb_drag_cleanup,
};

#endif /* UPGRADE_SRC_USB_DRAG */
