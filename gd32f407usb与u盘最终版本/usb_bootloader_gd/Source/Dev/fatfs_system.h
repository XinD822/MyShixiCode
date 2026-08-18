#ifndef __FATFS_SYSTEM_H
#define __FATFS_SYSTEM_H

#include "config.h"

FRESULT exf_getfree(char *drv,u32 *total,u32 *free);			
void FatFs_format(void);

FRESULT FatFs_Mount(void);												

FRESULT FatFs_mkdir_wrapper(const char *path); 
FRESULT FatFs_unlink_wrapper(const char *path); 

void FatFs_write_file(const char *path, const char *content); 
void FatFs_read_file(const char *path); 

void FatFs_list_dir(const char *path); 

void FatFs_Test(const char * wrapperpath,const char * filepath,const char *content); 


#endif
