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
static uint8_t  s_first_access = 1u;  /* 首次访问前需 tPU 上电余量 (GXHT40_POWER_ON_WAIT_MS) */

/* 诊断快照 (FWR-116): 只读观测, 不参与判定/重试/输出写入 */
static gxht40_diag_t s_diag;

static void gxht40_diag_clear(void)
{
    uint8_t i;

    s_diag.status     = (uint8_t)GXHT40_ERR_IO;   /* 尚未被填入时的占位 */
    s_diag.ack44      = 0u;
    s_diag.ack45      = 0u;
    s_diag.read_retry = 0u;
    s_diag.attempt    = 0u;
    s_diag.raw_valid  = 0u;
    for (i = 0u; i < (uint8_t)GXHT40_RESULT_LEN; i++) {
        s_diag.raw[i] = 0u;
    }
}

static void gxht40_diag_note_ack(uint8_t addr7)
{
    if (addr7 == (uint8_t)GXHT40_ADDR_7BIT_A) {
        s_diag.ack44 = 1u;
    } else if (addr7 == (uint8_t)GXHT40_ADDR_7BIT_B) {
        s_diag.ack45 = 1u;
    }
}

static void gxht40_diag_note_read(const uint8_t *buf, uint8_t failed_reads)
{
    uint8_t i;

    s_diag.read_retry = failed_reads;
    s_diag.raw_valid  = 1u;
    for (i = 0u; i < (uint8_t)GXHT40_RESULT_LEN; i++) {
        s_diag.raw[i] = buf[i];
    }
}

void gxht40_diag_fetch(gxht40_diag_t *out)
{
    if (out == NULL) {
        return;
    }
    *out = s_diag;
}

void gxht40_init(i2c_dev *dev)
{
    s_dev = dev;
    s_addr7 = 0u;
    s_first_access = 1u;
    gxht40_diag_clear();
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
    gxht40_diag_note_ack(addr7);        /* 诊断: 该候选地址本轮收到地址 ACK */

    delay_ms(GXHT40_MEASURE_WAIT_MS);   /* tMEAS.H max 8.3 ms (VDD=1.6V) -> 10 ms */

    for (r = 0u; r < GXHT40_READ_RETRY; r++) {
        e = i2c_read_bytes(s_dev, (uint8_t)((addr7 << 1) | 0x01u),
                           buf, GXHT40_RESULT_LEN);
        if (e == SF_I2C_SUCCESS) {
            gxht40_diag_note_read(buf, r);   /* 诊断: 重读次数 + 原始 6 字节 */
            return GXHT40_OK;
        }
        /* 读地址 NACK = 转换未完成 (手册 §7.1); 等待后重读 */
        if ((uint8_t)(r + 1u) < GXHT40_READ_RETRY) {
            delay_ms(GXHT40_READ_RETRY_DELAY_MS);
        }
    }
    s_diag.read_retry = (uint8_t)GXHT40_READ_RETRY;   /* 诊断: 读重试用尽 */
    return GXHT40_ERR_IO;
}

/*
 * 地址探测(带缓存) + FWR-118 失败后强制重探:
 *   - 缓存地址成功: 直接返回 (周期稳定时的快路径)。
 *   - 缓存地址**任一失败** (含读失败 ERR_IO / CRC / 量程): 失效缓存,
 *     不得因缓存地址让器件永久失联。
 *   - 重探前执行有界总线恢复 (i2c_bus_recover): 总线已空闲时立即返回(不产时钟),
 *     仅当从机仍握住 SDA 时才补 <=9 个 SCL 脉冲 + STOP。
 *   - 依次探测 0x44 -> 0x45; 0x44 读失败时仍重探 0x45 (不再提前返回)。
 */
static gxht40_status_t gxht40_acquire(uint8_t *buf)
{
    gxht40_status_t st;

    if (s_addr7 != 0u) {
        st = gxht40_start_and_read(s_addr7, buf);
        if (st == GXHT40_OK) {
            return GXHT40_OK;
        }
        s_addr7 = 0u;                   /* FWR-118: 任一失败都使缓存地址失效 */
    }

    (void)i2c_bus_recover(s_dev);       /* FWR-118: 重探前的有界总线恢复 */

    st = gxht40_start_and_read(GXHT40_ADDR_7BIT_A, buf);
    if (st == GXHT40_OK) {
        s_addr7 = GXHT40_ADDR_7BIT_A;
        return GXHT40_OK;
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

    gxht40_diag_clear();      /* 诊断快照只服务本轮; 不改变失败语义 */

    /* FD-002 §2.2 / 手册 tPU: 首次访问前留出器件上电稳定余量 (不进入周期路径) */
    if (s_first_access != 0u) {
        s_first_access = 0u;
        delay_ms(GXHT40_POWER_ON_WAIT_MS);
    }

    for (attempt = 0u; attempt < GXHT40_MEAS_RETRY; attempt++) {
        gxht40_status_t st;
        uint16_t raw_t, raw_h;
        int16_t t;
        uint16_t h;

        s_diag.attempt = (uint8_t)(attempt + 1u);

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
        s_diag.status = (uint8_t)GXHT40_OK;
        return GXHT40_OK;
    }

    s_diag.status = (uint8_t)last;
    return last;
}
