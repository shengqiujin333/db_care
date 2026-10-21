/*
 * fw_core.h - 固件纯逻辑核心(与硬件访问分离, 宿主机 L0 可测)
 *
 * 包含: CRC16-CCITT-FALSE；以及本次需求新增的 GXHT40 CRC-8/整数换算、
 *       光照滞回、条件上报判定。
 * 本头文件及其实现 fw_core.c 不依赖任何 MCU 寄存器/外设头
 * (数值常量取自 sensor_config.h 的纯数值段: 实现中定义 SENSOR_CONFIG_NO_MCU)。
 *
 * ITEM-009: 原 OPTCFG/1 解码器与参数(NVM 阈值)纯逻辑随 hall/OPTCFG/params/history
 *           模块退役而移除；CRC16-CCITT-FALSE 作为通用纯工具保留(仍由宿主机测试覆盖)。
 */
#ifndef __FW_CORE_H
#define __FW_CORE_H

#include <stdint.h>
#include <stdbool.h>

/* ---------------- CRC16-CCITT-FALSE (poly 0x1021, init 0xFFFF, 无反射, xorout 0) ---------------- */
uint16_t fw_crc16_ccitt(const uint8_t *p, uint16_t n);

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

/* ---------------- 无光判据: 完全无光基准 1/3 (readme 2/7; FD-002 rev 4.0 §6.3) ---------------- */
/*
 * dark = valid && (uint32)3*mean_adc_code >= c_dark   (uint32 整数乘法, 边界相等为暗)
 * valid=false 时不得判暗 (光照无效不能证明无光); 每笔独立, 无滞回/无历史暗态。
 * c_dark 为全暗基准 LIGHT_DARK_REF_CODE (1..4095); 未标定时由调用方传 valid=false。
 */
bool light_is_dark(uint16_t mean_adc_code, bool valid, uint16_t c_dark);

/* ---------------- 条件上报判定 (readme 4; IC-002 §2; FD-002 §6.4) ---------------- */
/* report = ((prev - cur) > 0.9C 且 DARK) 或 (cur > 35.0C); 无前值时下降分支恒假 */
bool sensor_decide_report(int16_t prev_temp_x10, bool have_prev,
                          int16_t cur_temp_x10, bool dark);

#endif /* __FW_CORE_H */
