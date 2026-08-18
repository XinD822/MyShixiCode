/**
 * @file chip_select.h
 * @brief Bootloader 芯片系列选择（唯一需要手动切换的地方）
 *
 * 解耦设计：
 *   选择 F103 或 F407 后，所有源码、驱动、Flash 擦写方式、跳转检查
 *   均自动切换，无需再修改其他文件。
 *
 * 切换方式：
 *   1. 注释/取消注释下面两行之一
 *   2. 或在 Keil C/C++ Define 中直接定义 CHIP_SERIES_F103 / CHIP_SERIES_F407
 */

#ifndef __CHIP_SELECT_H__
#define __CHIP_SELECT_H__

/* ──── 芯片系列选择 ────
 * 优先级：
 *   1) Keil 工程 C/C++ Define 中定义 CHIP_SERIES_F103 / CHIP_SERIES_F407
 *   2) 都未定义时，此处默认 F407（GD32 版 bootloader 仅支持 F407）
 * 两者同时定义会报错。
 */
#if defined(CHIP_SERIES_F103) && defined(CHIP_SERIES_F407)
  #error "Only one of CHIP_SERIES_F103 / CHIP_SERIES_F407 can be defined"
#endif

#if !defined(CHIP_SERIES_F103) && !defined(CHIP_SERIES_F407)
  #define CHIP_SERIES_F407
#endif

#endif /* __CHIP_SELECT_H__ */
