/*
 * fw_core.h - 固件纯逻辑核心(与硬件访问分离, 宿主机 L0 可测)
 *
 * 包含: CRC16-CCITT-FALSE、参数数值校验/换算/NVM 记录编解码、
 *       OPTCFG/1 帧解析校验、Manchester/OOK 解码器。
 * 本头文件及其实现 fw_core.c 不依赖任何 MCU 寄存器/外设头。
 */
#ifndef __FW_CORE_H
#define __FW_CORE_H

#include <stdint.h>
#include <stdbool.h>

/* ---------------- CRC16-CCITT-FALSE (IC-001 3.2/5, 与 encrytogate 一致) ---------------- */
uint16_t fw_crc16_ccitt(const uint8_t *p, uint16_t n);

/* ---------------- 参数纯函数 (FR-105/205, IC-001 2/3.2) ---------------- */
bool params_validate_floats(const float v[6]);
void params_floats_to_x10(const float v[6], int16_t out[6]);
void params_build_record(const float v[6], uint32_t txn, uint8_t rec[33]);
bool params_parse_record(const uint8_t rec[33], float v[6], uint32_t *txn);

/* ---------------- OPTCFG/1 协议常量与解码器 (FR-103..105, IC-001 3) ---------------- */
#define OPTCFG_FRAME_LEN        35      /* 字节帧长 */
#define OPTCFG_PREAMBLE_LEN     16      /* 前导 0x55 字节数 */
#define OPTCFG_MAGIC0           0x44    /* 'D' */
#define OPTCFG_MAGIC1           0x42    /* 'B' */
#define OPTCFG_VERSION          0x01
#define OPTCFG_MSG_SET_THRESH   0x01
#define OPTCFG_PAYLOAD_LEN      0x18    /* 24 */
#define OPTCFG_BIT_MS           40      /* Manchester 位周期 */
#define OPTCFG_HALF_MS          20
#define OPTCFG_DARK_RESET_SAMP  60      /* >300ms 无有效信号复位 (5ms 采样) */

typedef struct {
    uint8_t state;          /* 0=IDLE 1=PRE_CENTER0 2=PREAMBLE 3=DATA */
    uint8_t last_level;     /* 上一采样电平 */
    uint8_t dark_count;     /* 连续无沿样本数(暗区复位) */
    uint8_t since_center;   /* 距最近位中心(或前导起点)的样本数 */
    uint8_t pre_bits;       /* 已解前导位数 (1..128) */
    uint8_t bit_count;      /* 当前字节内位数 (DATA) */
    uint8_t byte_count;     /* 已收帧字节数 (DATA) */
    uint8_t frame[OPTCFG_FRAME_LEN];  /* 已解帧 */
    uint8_t frame_ready;    /* 已收满一帧 */
} optcfg_decoder_t;

void optcfg_decoder_init(optcfg_decoder_t *d);
/*
 * 每 5 ms 调用一次, level=0/1。收满一帧后置 *frame_ready=true, 帧位于 d->frame。
 * 解码策略(确定性, 无相位歧义):
 *   - IDLE: >300ms 暗区后的首个沿 = 前导起点(位边界);
 *   - PRE_CENTER0: 起点 +20ms 处沿 = bit0 位中心;
 *   - PREAMBLE: 后续每 40ms 沿 = bit1..127 位中心(0x55);
 *   - DATA: 每 40ms 位中心(Manchester 必变)解码, 忽略 20ms 处边界沿。
 */
void optcfg_decoder_tick(optcfg_decoder_t *d, uint8_t level, bool *frame_ready);

/* 帧解析+校验(纯): magic/version/type/len/CRC/范围/关系 (IC-001 3.2/2) */
bool optcfg_parse_and_validate(const uint8_t frame[OPTCFG_FRAME_LEN],
                               float out6[6], uint32_t *txn);

#endif /* __FW_CORE_H */
