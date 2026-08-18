/**
 * @file plat_log.h
 * @brief 日志抽象
 */
#ifndef __PLAT_LOG_H
#define __PLAT_LOG_H
#include "plat_config.h"

typedef enum { PLAT_LOG_DBG_T=0, PLAT_LOG_INFO_T=1, PLAT_LOG_ERR_T=2 } plat_log_level_t;

#if PLAT_LOG_LEVEL <= PLAT_LOG_DBG
  #define LOGD(...) plat_log(PLAT_LOG_DBG_T, __FILE__, __LINE__, __VA_ARGS__)
#else
  #define LOGD(...) ((void)0)
#endif
#define LOGI(...) plat_log(PLAT_LOG_INFO_T, __FILE__, __LINE__, __VA_ARGS__)
#define LOGE(...) plat_log(PLAT_LOG_ERR_T, __FILE__, __LINE__, __VA_ARGS__)

void plat_log(plat_log_level_t lv, const char *file, int line, const char *fmt, ...);

#endif /* __PLAT_LOG_H */
