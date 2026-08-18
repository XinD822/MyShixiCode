#include "FatFs_system.h"
#include "mutex.h"
#include "error_handler.h"

FATFS sFLASH_FatFs;
FIL file;
BYTE work[FF_MAX_SS];

/* Mutex for FatFS access */
static Mutex_t fatfs_mutex;

FRESULT exf_getfree(char *drv, u32 *total, u32 *free)
{
	FATFS *fs1;
	FRESULT res;
	u32 fre_clust = 0, fre_sect = 0, tot_sect = 0;

	res = f_getfree((const TCHAR*)drv, (DWORD*)&fre_clust, &fs1);
	if (res == 0)
	{
		tot_sect = (fs1->n_fatent - 2) * fs1->csize;
		fre_sect = fre_clust * fs1->csize;
#if FF_MAX_SS != 512
		tot_sect *= fs1->ssize / 512;
		fre_sect *= fs1->ssize / 512;
#endif
		*total = tot_sect >> 1;
		*free = fre_sect >> 1;
	}

	printf("[FatFS] Total = %d KB, Free = %d KB\r\n", *total, *free);
	return res;
}

void FatFs_format(void)
{
	FRESULT MyFile_Res;

	MyFile_Res = f_mkfs("0:", 0, work, sizeof(work));
	if (MyFile_Res == FR_OK)
	{
		printf("[FatFS] Format OK\r\n");
	}
	else
	{
		printf("[FatFS] Format fail, code=%d\r\n", MyFile_Res);
	}
}

FRESULT FatFs_Mount(void)
{
	FRESULT MyFile_Res;
	u32 dtsize, dfsize;

	static uint8_t mutex_initialized = 0;
	if (!mutex_initialized) {
		Mutex_Init(&fatfs_mutex);
		mutex_initialized = 1;
	}

	if (!Mutex_Lock(&fatfs_mutex, 1000)) {
		Error_Handler_Record(ERR_MUTEX_TIMEOUT);
		printf("[FatFS] Mount timeout\r\n");
		return FR_TIMEOUT;
	}

	MyFile_Res = f_mount(&sFLASH_FatFs, "0:", 1);

	if (MyFile_Res == FR_OK)
		printf("[FatFS] Mount OK\r\n");
	else
		printf("[FatFS] Mount fail, code=%d\r\n", MyFile_Res);

	// 检查文件系统完整性（不自动格式化！）
	if (MyFile_Res == FR_OK) {
		MyFile_Res = exf_getfree("0:", &dtsize, &dfsize);
		if (MyFile_Res != FR_OK) {
			printf("[FatFS] WARNING: filesystem integrity check failed (code=%d)\r\n", MyFile_Res);
			printf("[FatFS] Run Format_Flash() manually if reformat is needed\r\n");
		}
	}

	Mutex_Unlock(&fatfs_mutex);
	return MyFile_Res;
}

FRESULT FatFs_mkdir_wrapper(const char *path)
{
	FRESULT res = f_mkdir(path);

	printf("[FatFS] mkdir: %s\r\n", path);

	if (res == FR_OK)
		printf("[FatFS] mkdir OK\r\n");
	else
		printf("[FatFS] mkdir fail, code=%d\r\n", res);

	return res;
}

FRESULT FatFs_unlink_wrapper(const char *path)
{
	FRESULT res = f_unlink(path);

	if (res == FR_OK)
		printf("[FatFS] unlink OK: %s\r\n", path);
	else
		printf("[FatFS] unlink fail, code=%d\r\n", res);

	return res;
}

void FatFs_write_file(const char *path, const char *content)
{
	printf("[FatFS] Write file: %s, content: %s\r\n", path, content);

	FRESULT res = f_open(&file, path, FA_CREATE_NEW | FA_WRITE);

	if (res == FR_OK)
	{
		printf("[FatFS] File %s opened, writing...\r\n", path);
		UINT bw;
		res = f_write(&file, content, strlen(content), &bw);

		f_close(&file);

		if (bw != strlen(content))
		{
			printf("[FatFS] Write fail, code=%d\r\n", res);
		}
		else
		{
			printf("[FatFS] Write OK\r\n");
		}
	}
	else
	{
		printf("[FatFS] Open fail, code=%d\r\n", res);
	}
}

void FatFs_read_file(const char *path)
{
	char buffer[128];

	FRESULT res = f_open(&file, path, FA_READ);

	if (res == FR_OK)
	{
		UINT br;

		f_read(&file, buffer, sizeof(buffer), &br);
		buffer[br] = '\0';

		printf("[FatFS] Read: %s\r\n", buffer);

		f_close(&file);
	}
	else
		printf("[FatFS] Read fail, code=%d\r\n", res);
}

void FatFs_list_dir(const char *path)
{
	DIR dir;
	FILINFO fno;

	FRESULT res = f_opendir(&dir, path);

	if (res == FR_OK)
	{
		char list[512] = "";

		while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0])
		{
			strcat(list, fno.fname);
			strcat(list, "\r\n");
		}
		printf("[FatFS] Dir: %s", list);

		f_closedir(&dir);
	}
	else
		printf("[FatFS] List dir fail, code=%d\r\n", res);
}

void FatFs_Test(const char *wrapperpath, const char *filepath, const char *content)
{
	FRESULT res;

	res = FatFs_mkdir_wrapper(wrapperpath);

	if (res == FR_OK)
		printf("[FatFS] Dir %s created\r\n", wrapperpath);
	else if (res == FR_EXIST)
		printf("[FatFS] Dir already exists\r\n");
	else
	{
		printf("[FatFS] Dir create fail, code=%d\r\n", res);
		return;
	}

	FatFs_write_file(filepath, content);

	FatFs_read_file(filepath);
}
