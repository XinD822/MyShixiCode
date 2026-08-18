/**
 * @file source_usb_host.c
 * @brief U盘升级来源实现（UpgradeSource_t 接口）
 *
 * 从 USB Host U盘（FatFs 卷 "1:"）读取 firmware.bin。
 * 与 source_usb_drag.c 结构相同，仅卷号不同。
 */

#include "upgrade_source.h"
#include "upgrade_config.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

#if UPGRADE_SRC_USB_DRIVE

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

static uint8_t usb_host_detect(void)
{
    DIR dir;
    FILINFO fno;

    if (f_opendir(&dir, "1:/") != FR_OK) return 0;

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

static FRESULT usb_host_open(FIL *file, uint32_t *size)
{
    DIR dir;
    FILINFO fno;
    char path[FIRMWARE_FILENAME_MAX + 4];

    if (f_opendir(&dir, "1:/") != FR_OK) return FR_NO_FILE;

    while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0]) {
        if (fno.fattrib & AM_DIR) continue;
        if (strcasecmp_local(fno.fname, FIRMWARE_NAME) == 0) {
            if (fno.fsize > 256 && fno.fsize <= FIRMWARE_MAX_SIZE) {
                *size = fno.fsize;
                snprintf(path, sizeof(path), "1:/%s", fno.fname);
                f_closedir(&dir);
                return f_open(file, path, FA_READ);
            }
        }
    }

    f_closedir(&dir);
    return FR_NO_FILE;
}

static FRESULT usb_host_read(FIL *file, void *buf, UINT btr, UINT *br)
{
    return f_read(file, buf, btr, br);
}

static void usb_host_close(FIL *file)
{
    f_close(file);
}

static void usb_host_cleanup(void)
{
    DIR dir;
    FILINFO fno;

    if (f_opendir(&dir, "1:/") != FR_OK) {
        LOGD("[UPG] cleanup opendir fail\r\n");
        return;
    }

    while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0]) {
        if (fno.fattrib & AM_DIR) continue;
        if (strcasecmp_local(fno.fname, FIRMWARE_NAME) == 0) {
            char path[64];
            snprintf(path, sizeof(path), "1:/%s", fno.fname);
            FRESULT res = f_unlink(path);
            LOGD("[UPG] cleanup unlink %s -> %d\r\n", path, res);
            break;
        }
    }

    f_closedir(&dir);
}

const UpgradeSource_t Source_USB_Drive = {
    .name    = "usb_host",
    .detect  = usb_host_detect,
    .open    = usb_host_open,
    .read    = usb_host_read,
    .close   = usb_host_close,
    .cleanup = usb_host_cleanup,
};

#endif /* UPGRADE_SRC_USB_DRIVE */
