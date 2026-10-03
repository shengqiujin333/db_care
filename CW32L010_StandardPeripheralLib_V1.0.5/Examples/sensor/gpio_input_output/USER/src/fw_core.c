/*
 * fw_core.c - 固件纯逻辑核心实现(与硬件访问分离, 宿主机 L0 可测)
 *
 * 无任何 MCU 寄存器/外设依赖: 仅标准 C + params.h/optcfg.h 常量。
 */
#include "fw_core.h"
#include "params.h"
#include <string.h>
#include <math.h>

/* 纯数值配置(不含 MCU 引脚/外设宏): fw_core.c 不依赖 MCU 头 (FD-002 §3.2) */
#define SENSOR_CONFIG_NO_MCU
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

/* ------------------------------------------------------------------ */
/* binary32 little-endian 读写                                          */
/* ------------------------------------------------------------------ */
static float le_f32(const uint8_t b[4])
{
    uint32_t u = (uint32_t)b[0] | ((uint32_t)b[1] << 8) |
                 ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
    float f;
    memcpy(&f, &u, 4);
    return f;
}

static void f32_le(float f, uint8_t b[4])
{
    uint32_t u;
    memcpy(&u, &f, 4);
    b[0] = (uint8_t)(u & 0xFF);
    b[1] = (uint8_t)((u >> 8) & 0xFF);
    b[2] = (uint8_t)((u >> 16) & 0xFF);
    b[3] = (uint8_t)((u >> 24) & 0xFF);
}

/* 是否被拒绝: NaN / ±Inf / 负零; 允许 +0.0 与正常数 */
static bool f32_rejected(float v)
{
    uint32_t u;
    memcpy(&u, &v, 4);
    if (u == 0x80000000u) return true;
    if (isnan(v) || isinf(v)) return true;
    return false;
}

/* ------------------------------------------------------------------ */
/* 参数: 数值范围/关系校验 (IC-001 2 节)                                 */
/* ------------------------------------------------------------------ */
bool params_validate_floats(const float v[6])
{
    for (int i = 0; i < PARAMS_NUM; i++) {
        if (f32_rejected(v[i])) return false;
    }
    if (v[PARAMS_IDX_TEMP_DROP] < 0.0f || v[PARAMS_IDX_TEMP_DROP] > 20.0f) return false;
    if (v[PARAMS_IDX_HUM_DROP]  < 0.0f || v[PARAMS_IDX_HUM_DROP]  > 100.0f) return false;
    if (v[PARAMS_IDX_TEMP_LOW]  < -40.0f || v[PARAMS_IDX_TEMP_LOW]  > 85.0f) return false;
    if (v[PARAMS_IDX_HUM_LOW]   < 0.0f   || v[PARAMS_IDX_HUM_LOW]   > 100.0f) return false;
    if (v[PARAMS_IDX_TEMP_HIGH] < -40.0f || v[PARAMS_IDX_TEMP_HIGH] > 85.0f) return false;
    if (v[PARAMS_IDX_HUM_HIGH]  < 0.0f   || v[PARAMS_IDX_HUM_HIGH]  > 100.0f) return false;
    if (v[PARAMS_IDX_TEMP_LOW] >= v[PARAMS_IDX_TEMP_HIGH]) return false;
    if (v[PARAMS_IDX_HUM_LOW]  >= v[PARAMS_IDX_HUM_HIGH]) return false;
    return true;
}

static int16_t f_to_x10(float v)
{
    float x = v * 10.0f;
    x = (x >= 0.0f) ? (x + 0.5f) : (x - 0.5f);
    if (x > 32767.0f) return 32767;
    if (x < -32768.0f) return -32768;
    return (int16_t)x;
}

void params_floats_to_x10(const float v[6], int16_t out[6])
{
    for (int i = 0; i < PARAMS_NUM; i++) out[i] = f_to_x10(v[i]);
}

/* ------------------------------------------------------------------ */
/* 参数: NVM 记录构建/解析                                               */
/*   布局: magic[0..1] LE, version[2], txn[3..6] LE, float[7..30],     */
/*         crc16[31..32] BE(覆盖 0..30)                                 */
/* ------------------------------------------------------------------ */
void params_build_record(const float v[6], uint32_t txn, uint8_t rec[PARAMS_REC_LEN])
{
    memset(rec, 0xFF, PARAMS_REC_LEN);
    rec[0] = (uint8_t)(PARAMS_MAGIC & 0xFF);
    rec[1] = (uint8_t)((PARAMS_MAGIC >> 8) & 0xFF);
    rec[2] = PARAMS_VERSION;
    rec[3] = (uint8_t)(txn & 0xFF);
    rec[4] = (uint8_t)((txn >> 8) & 0xFF);
    rec[5] = (uint8_t)((txn >> 16) & 0xFF);
    rec[6] = (uint8_t)((txn >> 24) & 0xFF);
    for (int i = 0; i < PARAMS_NUM; i++) {
        f32_le(v[i], &rec[7 + i * 4]);
    }
    uint16_t crc = fw_crc16_ccitt(rec, 31);
    rec[31] = (uint8_t)(crc >> 8);
    rec[32] = (uint8_t)(crc & 0xFF);
}

bool params_parse_record(const uint8_t rec[PARAMS_REC_LEN], float v[6], uint32_t *txn)
{
    if (rec[0] != (uint8_t)(PARAMS_MAGIC & 0xFF) ||
        rec[1] != (uint8_t)((PARAMS_MAGIC >> 8) & 0xFF)) return false;
    if (rec[2] != PARAMS_VERSION) return false;
    uint16_t crc_calc = fw_crc16_ccitt(rec, 31);
    uint16_t crc_rec  = ((uint16_t)rec[31] << 8) | rec[32];
    if (crc_calc != crc_rec) return false;
    if (txn) {
        *txn = (uint32_t)rec[3] | ((uint32_t)rec[4] << 8) |
               ((uint32_t)rec[5] << 16) | ((uint32_t)rec[6] << 24);
    }
    for (int i = 0; i < PARAMS_NUM; i++) {
        v[i] = le_f32(&rec[7 + i * 4]);
    }
    return true;
}

/* ------------------------------------------------------------------ */
/* OPTCFG: 帧解析+校验 (IC-001 3.2/2)                                    */
/* ------------------------------------------------------------------ */
bool optcfg_parse_and_validate(const uint8_t frame[OPTCFG_FRAME_LEN],
                               float out6[6], uint32_t *txn)
{
    if (frame[0] != OPTCFG_MAGIC0 || frame[1] != OPTCFG_MAGIC1) return false;
    if (frame[2] != OPTCFG_VERSION) return false;
    if (frame[3] != OPTCFG_MSG_SET_THRESH) return false;
    if (frame[4] != OPTCFG_PAYLOAD_LEN) return false;

    uint16_t crc_calc = fw_crc16_ccitt(frame, 33);
    uint16_t crc_rx   = ((uint16_t)frame[33] << 8) | frame[34];   /* big-endian */
    if (crc_calc != crc_rx) return false;

    if (txn) {
        *txn = (uint32_t)frame[5] | ((uint32_t)frame[6] << 8) |
               ((uint32_t)frame[7] << 16) | ((uint32_t)frame[8] << 24);
    }
    float v[6];
    for (int i = 0; i < 6; i++) v[i] = le_f32(&frame[9 + i * 4]);
    if (!params_validate_floats(v)) return false;
    if (out6) memcpy(out6, v, sizeof(v));
    return true;
}

/* ------------------------------------------------------------------ */
/* OPTCFG: Manchester/OOK 解码器                                          */
/*  10ms 采样; >300ms 暗区后首沿=前导起点; 位中心沿每 40ms 解码,           */
/*  数据相位忽略 20ms 处边界沿; 确定性相位, 无歧义。                        */
/* ------------------------------------------------------------------ */
void optcfg_decoder_init(optcfg_decoder_t *d)
{
    memset(d, 0, sizeof(*d));
    d->last_level = 0;
}

static void optcfg_decoder_resync(optcfg_decoder_t *d)
{
    d->state        = 0;   /* IDLE */
    d->dark_count   = 0;
    d->since_center = 0;
    d->pre_bits     = 0;
    d->bit_count    = 0;
    d->byte_count   = 0;
    d->frame_ready  = 0;
}

static void optcfg_push_bit(optcfg_decoder_t *d, uint8_t bit)
{
    uint8_t sh = (uint8_t)(7 - (d->bit_count & 7));
    d->frame[d->byte_count] |= (uint8_t)(bit << sh);
    d->bit_count++;
    if (d->bit_count == 8) {
        d->bit_count = 0;
        d->byte_count++;
    }
}

void optcfg_decoder_tick(optcfg_decoder_t *d, uint8_t level, bool *frame_ready)
{
    uint8_t t;

    if (frame_ready) *frame_ready = false;
    if (d->frame_ready) return;   /* 帧已收满, 等待上层处理后复位 */

    /* 活动状态(1/2/3)每样本递增 since_center(距最近位中心的样本数, 只在中心复位)
     * 5ms 采样: 半位 20ms±25%=15..25ms → 3..5 样本; 位 40ms±25%=30..50ms → 6..10 样本 */
    if (d->state >= 1) d->since_center++;
    if ((d->state == 2 || d->state == 3) && d->since_center > 12)
        optcfg_decoder_resync(d);              /* 漏中心沿 → 重同步 */

    if (level == d->last_level) {
        /* 无沿 */
        if (d->dark_count < 250) d->dark_count++;
        return;
    }

    /* 有沿: level != last_level */
    t = d->since_center;   /* 距最近中心/起点的样本数(已含本样本) */

    switch (d->state) {
    case 0:   /* IDLE */
        if (d->dark_count >= OPTCFG_DARK_RESET_SAMP) {
            /* 暗区≥300ms 后首沿 = 前导起点 (位边界) */
            d->state = 1;                       /* PRE_CENTER0 */
            d->since_center = 0;
        }
        /* 否则视为噪声沿, 忽略 */
        break;

    case 1:   /* PRE_CENTER0: 起点+20ms 沿 = bit0 中心 (15..25ms → 3..5 样本) */
        if (t >= 3 && t <= 5) {
            d->pre_bits = 1;
            d->state = 2;                       /* PREAMBLE */
            d->since_center = 0;
        } else {
            optcfg_decoder_resync(d);
        }
        break;

    case 2:   /* PREAMBLE: 每 40ms 沿 = bit1..127 中心 (30..50ms → 6..10 样本) */
        if (t >= 6 && t <= 10) {
            d->pre_bits++;
            d->since_center = 0;
            if (d->pre_bits >= 128) {
                d->state = 3;                   /* DATA */
                d->bit_count  = 0;
                d->byte_count = 0;
                memset(d->frame, 0, OPTCFG_FRAME_LEN);
            }
        } else {
            optcfg_decoder_resync(d);
        }
        break;

    case 3:   /* DATA: 位中心沿(30..50ms → t 6..10)解码; 边界沿(15..25ms → t 2..5)忽略 */
        if (t >= 2 && t <= 5) {
            /* 边界沿: 忽略, 中心仍在预期位置(不复位 since_center) */
        } else if (t >= 6 && t <= 10) {
            /* 位中心: bit = 首半电平取反 (首半亮(1)→0, 灭(0)→1) */
            uint8_t bit = (d->last_level == 0) ? 1u : 0u;
            optcfg_push_bit(d, bit);
            d->since_center = 0;
            if (d->byte_count >= OPTCFG_FRAME_LEN) {
                d->frame_ready = 1;
                if (frame_ready) *frame_ready = true;
                optcfg_decoder_resync(d);       /* 单帧结束, 回到 IDLE 等下一帧 */
            }
        } else {
            optcfg_decoder_resync(d);
        }
        break;

    default:
        optcfg_decoder_resync(d);
        break;
    }

    d->last_level = level;
    d->dark_count = 0;
}
