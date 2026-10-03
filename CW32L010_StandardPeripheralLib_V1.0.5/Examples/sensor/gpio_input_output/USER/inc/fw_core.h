/*
 * fw_core.h - 固件纯逻辑核心(与硬件访问分离, 宿主机 L0 可测)
 *
 * 包含: CRC16-CCITT-FALSE、参数数值校验/换算/NVM 记录编解码、
 *       OPTCFG/1 帧解析校验、Manchester/OOK 解码器；
 *       以及本次需求新增: GXHT40 CRC-8/整数换算、光照滞回、条件上报判定。
 * 本头文件及其实现 fw_core.c 不依赖任何 MCU 寄存器/外设头
 * (数值常量取自 sensor_config.h 的纯数值段: 实现中定义 SENSOR_CONFIG_NO_MCU)。
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

/* ---------------- GXHT40 纯逻辑 (readme 1; gxht40.pdf §7.3/§7.5; FD-002 §6.1/§6.2) ---------------- */
/* CRC-8: poly 0x31, init 0xFF, 无反转, xorout 0x00; 空长度返回 init 0xFF; 参考 CRC(0xBEEF)=0x92 */
uint8_t fw_crc8_gxht(const uint8_t *p, uint16_t n);
/* 温度原始字 -> x10: T[0.1C] = -450 + round(1750*S/65536); 纯换算(不判量程, 值域 -450..1300) */
int16_t gxht40_temp_raw_to_x10(uint16_t raw);
/* 湿度原始字 -> x10: RH[0.1%] = -60 + round(1250*S/65536), 再截断 0..1000 */
uint16_t gxht40_hum_raw_to_x10(uint16_t raw);
/* 温度 x10 是否在有效域 -400..1250 (-40.0..125.0 C) */
bool gxht40_temp_x10_valid(int16_t temp_x10);
/* 组合换算: 温度有效时写 *temp_x10 与 *hum_x10 并返回 true; 否则返回 false 且不写输出 */
bool gxht40_raw_to_x10(uint16_t raw_t, uint16_t raw_rh,
                       int16_t *temp_x10, uint16_t *hum_x10);

/* ---------------- 光照滞回 (readme 2; FD-002 §6.3) ---------------- */
/* prev_dark 为当前状态: 已暗时 code <= LIGHT_DARK_EXIT 才转明; 已明时 code >= LIGHT_DARK_ENTER 才转暗 */
bool light_code_is_dark(uint16_t code, bool prev_dark);

/* ---------------- 条件上报判定 (readme 4; IC-002 §2; FD-002 §6.4) ---------------- */
/* report = ((prev - cur) > 0.9C 且 DARK) 或 (cur > 35.0C); 无前值时下降分支恒假 */
bool sensor_decide_report(int16_t prev_temp_x10, bool have_prev,
                          int16_t cur_temp_x10, bool dark);

#endif /* __FW_CORE_H */
