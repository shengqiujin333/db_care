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
 * 地址探测:
 *   - 稳态快路径 (有缓存地址 且 上轮未失败): 只试缓存地址; 失败则转入完整重探。
 *   - 完整重探 (首次访问 / 上轮失败): 先做有界总线恢复, 再**依次探测 0x44 与 0x45 两个候选**;
 *     即使 0x44 已成功也要把 0x45 探完 (FWR-118 / TD-002 B18: 任一失败后的下一周期必须重探两地址)。
 *   - 两候选均失败时返回最后一次探测的结果码 (既有语义, 与地址 ACK 诊断字段一致)。
 */
static gxht40_status_t gxht40_acquire(uint8_t *buf)
{
    gxht40_status_t st;
    gxht40_status_t st_b;
    uint8_t buf_b[GXHT40_RESULT_LEN];
    uint8_t have_a = 0u;

    /* 1) 稳态快路径 (仅当缓存有效且上一轮未失败) */
    if (s_addr7 != 0u) {
        st = gxht40_start_and_read(s_addr7, buf);
        if (st == GXHT40_OK) {
            return GXHT40_OK;
        }
        s_addr7 = 0u;               /* 缓存地址失败: 本轮转入完整重探 */
    }

    /* 2) 完整重探: 重探前先做有界总线恢复, 再依次探测 0x44 -> 0x45 */
    (void)i2c_bus_recover(s_dev);

    st = gxht40_start_and_read(GXHT40_ADDR_7BIT_A, buf);
    if (st == GXHT40_OK) {
        s_addr7 = GXHT40_ADDR_7BIT_A;
        have_a = 1u;
    }

    st_b = gxht40_start_and_read(GXHT40_ADDR_7BIT_B, buf_b);
    if (st_b == GXHT40_OK) {
        if (have_a == 0u) {
            uint8_t i;
            for (i = 0u; i < (uint8_t)GXHT40_RESULT_LEN; i++) {
                buf[i] = buf_b[i];  /* 用 B 的帧 (不直接读入 buf, 避免破坏 A 的结果) */
            }
            s_addr7 = GXHT40_ADDR_7BIT_B;
        }
        return GXHT40_OK;
    }

    if (have_a != 0u) {
        return GXHT40_OK;           /* A 成功且 B 未应答: A 的帧仍然有效 */
    }
    return st_b;                    /* 两候选均失败: 沿用“最后一次探测结果码”语义 */
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
            s_addr7 = 0u;           /* FWR-118: 任一测量失败都失效缓存地址 */
            continue;               /* 丢弃整帧, 重测 */
        }

        raw_t = (uint16_t)(((uint16_t)buf[0] << 8) | (uint16_t)buf[1]);
        raw_h = (uint16_t)(((uint16_t)buf[3] << 8) | (uint16_t)buf[4]);

        /* 整数换算 + 有效域判定 (纯逻辑在 fw_core.c; 无效时不写输出) */
        if (!gxht40_raw_to_x10(raw_t, raw_h, &t, &h)) {
            last = GXHT40_ERR_RANGE;
            s_addr7 = 0u;           /* FWR-118: 任一测量失败都失效缓存地址 */
            continue;               /* 超出 -40.0..125.0 C: 视为无效测量 */
        }

        /* 仅在完全成功时写输出 (失败不修改输出) */
        *temp_x10 = t;
        *hum_x10  = h;
        s_diag.status = (uint8_t)GXHT40_OK;
        return GXHT40_OK;
    }

    /*
     * 结果码与地址 ACK 事实自洽 (OBS-2, 由本能力决定): 两个候选都探完后, 若本轮
     * 有任一候选地址应答过(器件在总线上)则不得报“无器件”; 此时更准确的码是
     * “器件在但读不通”(ERR_IO)。这样 G 行的 s= 与 a44/a45 不会再互相矛盾。
     */
    if ((last == GXHT40_ERR_NO_DEVICE) &&
        ((s_diag.ack44 != 0u) || (s_diag.ack45 != 0u))) {
        last = GXHT40_ERR_IO;
    }

    /* FWR-118: 本轮测量失败 -> 下一轮必须重新探测 0x44/0x45 (重探前做有界总线恢复) */
    s_addr7 = 0u;
    s_diag.status = (uint8_t)last;
    return last;
}
