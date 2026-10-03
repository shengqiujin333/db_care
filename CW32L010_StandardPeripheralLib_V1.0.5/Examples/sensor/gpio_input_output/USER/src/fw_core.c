/*
 * fw_core.c - 固件纯逻辑核心实现(与硬件访问分离, 宿主机 L0 可测)
 *
 * 无任何 MCU 寄存器/外设依赖。
 * ITEM-009: 移除随 hall/OPTCFG/params/history 退役的 OPTCFG 解码器与参数(NVM 阈值)纯逻辑；
 *           CRC16-CCITT-FALSE 作为通用纯工具保留。
 */
#include "fw_core.h"
#include <stddef.h>      /* NULL */

/* 纯数值配置(不含 MCU 引脚/外设宏): fw_core.c 不依赖 MCU 头 (FD-002 §3.2) */
#ifndef SENSOR_CONFIG_NO_MCU
#define SENSOR_CONFIG_NO_MCU
#endif
#include "sensor_config.h"

/* ------------------------------------------------------------------ */
/* CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF, 无反射, xorout 0)     */
/* ------------------------------------------------------------------ */
uint16_t fw_crc16_ccitt(const uint8_t *p, uint16_t n)
{
    uint16_t crc = 0xFFFFu;
    uint16_t i, b;
    for (i = 0; i < n; i++) {
        crc ^= (uint16_t)p[i] << 8;
        for (b = 0; b < 8; b++) {
            crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u) : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

/* ------------------------------------------------------------------ */
/* GXHT40 纯逻辑 (readme 修改点 1; gxht40.pdf §7.3/§7.5; FD-002 §6.1/§6.2) */
/* ------------------------------------------------------------------ */
uint8_t fw_crc8_gxht(const uint8_t *p, uint16_t n)
{
    uint8_t  crc = (uint8_t)GXHT40_CRC8_INIT;
    uint16_t i;
    uint8_t  b;

    for (i = 0u; i < n; i++) {
        crc ^= p[i];
        for (b = 0u; b < 8u; b++) {
            crc = (crc & 0x80u)
                      ? (uint8_t)((crc << 1) ^ (uint8_t)GXHT40_CRC8_POLY)
                      : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

int16_t gxht40_temp_raw_to_x10(uint16_t raw)
{
    int32_t v = (int32_t)GXHT40_TEMP_OFFSET_X10
              + (int32_t)(((uint32_t)GXHT40_TEMP_SCALE_NUM * (uint32_t)raw
                           + GXHT40_RAW_ROUND) >> 16);
    return (int16_t)v;
}

uint16_t gxht40_hum_raw_to_x10(uint16_t raw)
{
    int32_t v = (int32_t)GXHT40_HUM_OFFSET_X10
              + (int32_t)(((uint32_t)GXHT40_HUM_SCALE_NUM * (uint32_t)raw
                           + GXHT40_RAW_ROUND) >> 16);

    if (v < (int32_t)GXHT40_HUM_X10_MIN) return (uint16_t)GXHT40_HUM_X10_MIN;
    if (v > (int32_t)GXHT40_HUM_X10_MAX) return (uint16_t)GXHT40_HUM_X10_MAX;
    return (uint16_t)v;
}

bool gxht40_temp_x10_valid(int16_t temp_x10)
{
    return (temp_x10 >= (int16_t)GXHT40_TEMP_X10_MIN) &&
           (temp_x10 <= (int16_t)GXHT40_TEMP_X10_MAX);
}

bool gxht40_raw_to_x10(uint16_t raw_t, uint16_t raw_rh,
                       int16_t *temp_x10, uint16_t *hum_x10)
{
    int16_t t = gxht40_temp_raw_to_x10(raw_t);

    if (!gxht40_temp_x10_valid(t)) {
        return false;                       /* 超有效域: 不写输出 */
    }
    if (temp_x10 != NULL) *temp_x10 = t;
    if (hum_x10 != NULL)  *hum_x10  = gxht40_hum_raw_to_x10(raw_rh);
    return true;
}

/* ------------------------------------------------------------------ */
/* 光照滞回 (readme 修改点 2; FD-002 §6.3)                              */
/* ------------------------------------------------------------------ */
bool light_code_is_dark(uint16_t code, bool prev_dark)
{
    if (prev_dark) {
        return !(code <= (uint16_t)LIGHT_DARK_EXIT);    /* 已暗: 低于退出阈值才转明 */
    }
    return (code >= (uint16_t)LIGHT_DARK_ENTER);        /* 已明: 达到进入阈值才转暗 */
}

/* ------------------------------------------------------------------ */
/* 条件上报判定 (readme 修改点 4; IC-002 §2; FD-002 §6.4)              */
/* ------------------------------------------------------------------ */
bool sensor_decide_report(int16_t prev_temp_x10, bool have_prev,
                          int16_t cur_temp_x10, bool dark)
{
    if (cur_temp_x10 > (int16_t)SENSOR_REPORT_HIGH_X10) {
        return true;                    /* 超温分支: 不受光照/前值限制 */
    }
    if (cur_temp_x10 == (int16_t)SENSOR_REPORT_HIGH_X10) {
        return false;                   /* IC-002: 恰好 35.0 C 时 T<35 与 T>35 均不成立 */
    }
    if (!have_prev) {
        return false;                   /* 无前一有效温度: 下降分支恒假 */
    }
    if (!dark) {
        return false;                   /* 需同时无光 */
    }
    return ((int32_t)prev_temp_x10 - (int32_t)cur_temp_x10)
           > (int32_t)SENSOR_REPORT_DROP_X10;   /* 下降 > 0.9 C (0.1C 量化下即 >=1.0 C) */
}
