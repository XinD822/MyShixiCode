#include "config.h"
#include "upgrade.h"
#include "mass_mal.h"
#include "error_handler.h"
#include "usb_msc_device.h"

extern FATFS sFLASH_FatFs;
extern volatile uint32_t g_last_scsi_tick;

static uint8_t usb_active = 0;
static uint8_t fatfs_mounted = 0;
static uint8_t prev_usb_configured = 0;
static uint32_t g_fw_size = 0;  // 固件大小，供Task_Run使用

static uint8_t MoveFirmwareToStaging(void);

void USB_MSC_Configuration(void)
{
	usb_msc_device_init();
}

static void Safe_FatFs_Mount(void)
{
	if (!fatfs_mounted) {
		FRESULT res = FatFs_Mount();
		if (res == FR_OK) {
			fatfs_mounted = 1;
		} else {
			printf("[SYS] ERR: mount %d\r\n", res);
		}
	}
}

static void Safe_FatFs_Unmount(void)
{
	if (fatfs_mounted) {
		f_mount(NULL, "0:", 1);
		fatfs_mounted = 0;
	}
}

static void USB_Shutdown(void)
{
	usb_msc_device_deinit();
}

void System_Init(void)
{
	SCB->VTOR = APP_ADDR;     // 先设置中断向量表到App区
	CPU_INT_ENABLE();         // 再开中断
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	TIM2_Init();
	Task_Time_Init();
	UsartInit();
	W25QXX_Init();

	// 读取升级状态和标志
	uint32_t flag = Upgrade_GetFlag();
	uint32_t state;
	W25QXX_Read((uint8_t*)&state, UPGRADE_STATE_ADDR, 4);

// 处理上次升级结果
	if (state == UPGRADE_STATE_DONE) {
		Upgrade_SetFlag(UPGRADE_FLAG_NO, 0);
		uint32_t confirmed = UPGRADE_STATE_CONFIRMED;
		W25QXX_Write_NoCheck((uint8_t*)&confirmed, UPGRADE_STATE_ADDR, 4);
	}

	if (state == UPGRADE_STATE_CONFIRMED) {
		Upgrade_SetFlag(UPGRADE_FLAG_NO, 0);
		uint32_t none = UPGRADE_STATE_NONE;
		W25QXX_Write_NoCheck((uint8_t*)&none, UPGRADE_STATE_ADDR, 4);
	}

	// Normal boot: mount FatFS, then init USB
	Safe_FatFs_Mount();
	USB_MSC_Configuration();
	Upgrade_Init();
	printf("[SYS] Ready\r\n");
}

void SensorDataTask(void)
{
	if (usb_active) return;
	// Your sensor code here
}

void Task_Run(void)
{
	uint8_t usb_configured = usb_msc_is_configured();
	
	// === USB connects ===
	if (!usb_active && usb_configured) {
		printf("[SYS] USB in\r\n");
		Safe_FatFs_Unmount();
		usb_active = 1;
		g_last_scsi_tick = Delay_GetTick();
		prev_usb_configured = 1;
		return;
	}

	// === USB idle detection ===
	if (usb_active) {
		uint8_t should_exit = 0;

		if (prev_usb_configured && !usb_configured) {
			should_exit = 1;
			printf("[SYS] USB out\r\n");
		}
		else if (usb_configured &&
				 (Delay_GetTick() - g_last_scsi_tick > 10000)) {
			should_exit = 1;
			printf("[SYS] USB idle\r\n");
		}

		if (should_exit) {
			USB_Shutdown();
			Delay_ms(500);
			MAL_FlushCache();
			Safe_FatFs_Mount();

			uint8_t fw_moved = MoveFirmwareToStaging();

			if (fw_moved) {
				Upgrade_SetFlag(UPGRADE_FLAG_YES, g_fw_size);
				printf("[SYS] Flag set, reboot\r\n");
				Delay_ms(500);
				NVIC_SystemReset();
			} else {
				usb_active = 0;
				// 重新初始化USB
				USB_MSC_Configuration();
			}
		}
	}

	prev_usb_configured = usb_configured;
	SensorDataTask();
}

/**
 * @brief  Move firmware from FatFS to firmware partition, then truncate in FatFS
 *         Call this after USB disconnect + FatFS mount, before upgrade check.
 *         This frees FatFS space immediately so sensor data won't be crowded out.
 * @return 1=moved successfully, 0=no firmware found
 */
static uint8_t MoveFirmwareToStaging(void)
{
	FIL file;
	FRESULT fres;
	UINT br;
	uint32_t flash_addr = FIRMWARE_SLOT_A_ADDR;
	uint32_t total_read = 0;
	uint8_t buf[512];
	char found_name[32] = {0};

	DIR dir;
	FILINFO fno;

	if (f_opendir(&dir, "0:/") != FR_OK) {
		printf("[SYS] ERR: opendir\r\n");
		return 0;
	}

	uint8_t found = 0;
	while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0]) {
		if (fno.fattrib & AM_DIR) continue;
		if (strncasecmp(fno.fname, "FIRMWARE", 8) != 0) continue;
		if (fno.fsize <= 256 || fno.fsize > FIRMWARE_MAX_SIZE) continue;
		strncpy(found_name, fno.fname, sizeof(found_name) - 1);
		found_name[sizeof(found_name) - 1] = '\0';
		found = 1;
		break;
	}
	f_closedir(&dir);

	if (!found) {
		printf("[SYS] No FW\r\n");
		return 0;
	}

	printf("[SYS] FW: %s (%dB)\r\n", found_name, fno.fsize);

	// Erase Slot A
	uint32_t blocks = (fno.fsize + 65535) / 65536;
	for (uint32_t i = 0; i < blocks; i++) {
		W25QXX_Erase_Block(FIRMWARE_SLOT_A_ADDR + i * 65536);
	}

	// Read from FatFS → Write to firmware partition
	fres = f_open(&file, found_name, FA_READ);
	if (fres != FR_OK) {
		printf("[SYS] ERR: open %d\r\n", fres);
		return 0;
	}

	while (1) {
		fres = f_read(&file, buf, sizeof(buf), &br);
		if (fres != FR_OK || br == 0) break;
		W25QXX_Write_NoCheck(buf, flash_addr, br);
		flash_addr += br;
		total_read += br;
	}
	f_close(&file);

	// Delete firmware file
	f_unlink(found_name);
	f_unlink("0:/FIRMWARE.BIN");
	f_unlink("0:/firmware.bin");

	g_fw_size = total_read;

	// 计算校验和
	uint32_t checksum = 0;
	uint8_t crc_buf[512];
	uint32_t crc_addr = FIRMWARE_SLOT_A_ADDR;
	uint32_t crc_remaining = total_read;
	while (crc_remaining > 0) {
		uint32_t chunk = (crc_remaining > 512) ? 512 : crc_remaining;
		W25QXX_Read(crc_buf, crc_addr, chunk);
		for (uint32_t i = 0; i < chunk; i++) {
			checksum += crc_buf[i];
		}
		crc_addr += chunk;
		crc_remaining -= chunk;
	}
	W25QXX_Write((uint8_t*)&checksum, FIRMWARE_CRC_ADDR, sizeof(checksum));

	printf("[SYS] Done, %dB, CRC:0x%08X\r\n", total_read, checksum);
	return 1;
}

/**
 * @brief  Format FatFS data area only (safe - does not touch bootloader/config)
 */
void Format_Flash(void)
{
	printf("[SYS] Format...\r\n");
	f_mount(NULL, "0:", 1);
	FatFs_format();
	f_mount(&sFLASH_FatFs, "0:", 1);
	printf("[SYS] Format OK\r\n");
	Delay_ms(500);
	NVIC_SystemReset();
}
