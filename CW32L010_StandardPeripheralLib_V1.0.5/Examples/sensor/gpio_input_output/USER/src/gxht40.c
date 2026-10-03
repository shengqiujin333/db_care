/*
 * gxht40.c - GXHT40 温湿度传感器驱动 (readme 修改点 1; gxht40.pdf; FD-002 §3.1/§6/§10)
 *
 * 纯协议实现: 只依赖 sf_i2c (软 I²C 原语)、sensor_config.h (唯一配置点) 与 delay_ms。
 * 不使用软复位/加热器命令, 不在周期路径写 Flash (FD-002 §11)。
 */
#include "gxht40.h"
#include "sensor_config.h"
#include "fw_core.h"      /* 纯逻辑: fw_crc8_gxht / gxht40_raw_to_x10 (ITEM-005) */
#include "delay.h"

/* 绑定总线与地址探测缓存 */
static i2c_dev *s_dev = NULL;
static uint8_t  s_addr7 = 0u;   /* 0 = 未探测到 */

void gxht40_init(i2c_dev *dev)
{
    s_dev = dev;
    s_addr7 = 0u;
}

uint8_t gxht40_detected_addr7(void)
{
    return s_addr7;
}

/* ------------------------------------------------------------------ */
/* 纯计算 (整数, 无浮点; FD-002 §6.1/§6.2)                             */
/* ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ */
/* 总线时序                                                            */
/* ------------------------------------------------------------------ */

/*
 * 在指定 7bit 地址上: 发 0xFD -> 等待 tMEAS 上限 -> 读 6 字节(读 NACK 有界重读)。
 * 返回 GXHT40_OK / GXHT40_ERR_NO_DEVICE (命令未被 ACK) / GXHT40_ERR_IO (读失败)。
 */
static gxht40_status_t gxht40_start_and_read(uint8_t addr7, uint8_t *buf)
{
    sf_i2c_err e;
    uint8_t r;

    e = i2c_write_cmd(s_dev, (uint8_t)(addr7 << 1), GXHT40_CMD_MEASURE_HIGH_REP);
    if (e != SF_I2C_SUCCESS) {
        return GXHT40_ERR_NO_DEVICE;    /* 地址/命令未被 ACK: 该地址无器件 */
    }

    delay_ms(GXHT40_MEASURE_WAIT_MS);   /* tMEAS.H max 8.3 ms (VDD=1.6V) -> 10 ms */

    for (r = 0u; r < GXHT40_READ_RETRY; r++) {
        e = i2c_read_bytes(s_dev, (uint8_t)((addr7 << 1) | 0x01u),
                           buf, GXHT40_RESULT_LEN);
        if (e == SF_I2C_SUCCESS) {
            return GXHT40_OK;
        }
        /* 读地址 NACK = 转换未完成 (手册 §7.1); 等待后重读 */
        if ((uint8_t)(r + 1u) < GXHT40_READ_RETRY) {
            delay_ms(GXHT40_READ_RETRY_DELAY_MS);
        }
    }
    return GXHT40_ERR_IO;
}

/* 地址探测(带缓存): 先试缓存地址, 失败则依次试 0x44 / 0x45 */
static gxht40_status_t gxht40_acquire(uint8_t *buf)
{
    gxht40_status_t st;

    if (s_addr7 != 0u) {
        st = gxht40_start_and_read(s_addr7, buf);
        if (st == GXHT40_OK) {
            return GXHT40_OK;
        }
        if (st == GXHT40_ERR_IO) {
            return st;              /* 器件在但通信失败 */
        }
        s_addr7 = 0u;               /* 地址失配: 重新探测 */
    }

    st = gxht40_start_and_read(GXHT40_ADDR_7BIT_A, buf);
    if (st == GXHT40_OK) {
        s_addr7 = GXHT40_ADDR_7BIT_A;
        return GXHT40_OK;
    }
    if (st == GXHT40_ERR_IO) {
        return st;
    }

    st = gxht40_start_and_read(GXHT40_ADDR_7BIT_B, buf);
    if (st == GXHT40_OK) {
        s_addr7 = GXHT40_ADDR_7BIT_B;
        return GXHT40_OK;
    }
    return st;
}

/* ------------------------------------------------------------------ */
/* 对外: 一次测量                                                      */
/* ------------------------------------------------------------------ */
gxht40_status_t gxht40_measure(int16_t *temp_x10, uint16_t *hum_x10)
{
    uint8_t buf[GXHT40_RESULT_LEN];
    uint8_t attempt;
    gxht40_status_t last = GXHT40_ERR_IO;

    if ((s_dev == NULL) || (temp_x10 == NULL) || (hum_x10 == NULL)) {
        return GXHT40_ERR_PARAM;
    }

    for (attempt = 0u; attempt < GXHT40_MEAS_RETRY; attempt++) {
        gxht40_status_t st;
        uint16_t raw_t, raw_h;
        int16_t t;
        uint16_t h;

        st = gxht40_acquire(buf);
        if (st != GXHT40_OK) {
            last = st;
            continue;
        }

        /* 温度字 (buf[0..1]) 与湿度字 (buf[3..4]) 各带一个 CRC */
        if ((fw_crc8_gxht(&buf[0], 2u) != buf[2]) ||
            (fw_crc8_gxht(&buf[3], 2u) != buf[5])) {
            last = GXHT40_ERR_CRC;
            continue;               /* 丢弃整帧, 重测 */
        }

        raw_t = (uint16_t)(((uint16_t)buf[0] << 8) | (uint16_t)buf[1]);
        raw_h = (uint16_t)(((uint16_t)buf[3] << 8) | (uint16_t)buf[4]);

        /* 整数换算 + 有效域判定 (纯逻辑在 fw_core.c; 无效时不写输出) */
        if (!gxht40_raw_to_x10(raw_t, raw_h, &t, &h)) {
            last = GXHT40_ERR_RANGE;
            continue;               /* 超出 -40.0..125.0 C: 视为无效测量 */
        }

        /* 仅在完全成功时写输出 (失败不修改输出) */
        *temp_x10 = t;
        *hum_x10  = h;
        return GXHT40_OK;
    }

    return last;
}
