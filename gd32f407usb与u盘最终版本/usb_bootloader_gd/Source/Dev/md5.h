/**
 * @file md5.h
 * @brief MD5 消息摘要算法（RFC 1321）— 用于固件完整性校验
 *
 * 标准 MD5 实现，App 与 Bootloader 共用同一算法，便于与同事 OTA
 * 项目的 MD5 校验结果对齐（同一输入必得同一摘要）。
 * 接口风格与经典公共域实现一致：MD5_Init / MD5_Update / MD5_Final。
 */

#ifndef __MD5_H
#define __MD5_H

#include <stdint.h>

/* MD5 上下文（保存中间状态，流式计算用） */
typedef struct {
    uint32_t total[2];      /* 已处理字节数（64 位拆两半） */
    uint32_t state[4];      /* A/B/C/D 中间状态 */
    uint8_t  buffer[64];    /* 64 字节输入块缓冲 */
} MD5_CTX;

void MD5_Init(MD5_CTX *ctx);
void MD5_Update(MD5_CTX *ctx, const uint8_t *input, uint32_t len);
void MD5_Final(uint8_t digest[16], MD5_CTX *ctx);

#endif /* __MD5_H */
