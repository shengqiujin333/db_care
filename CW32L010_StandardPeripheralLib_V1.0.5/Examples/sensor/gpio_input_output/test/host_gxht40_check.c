/*
 * host_gxht40_check.c - GXHT40 驱动的实现侧自检 (宿主机 mock 总线, 不进入固件构建)
 *
 * 目的: 确定性验证 ITEM-003 的驱动行为:
 *   - 地址探测 0x44/0x45 (8bit 写 0x88/0x8A, 读 0x89/0x8B) 与缓存
 *   - 高重复率命令 0xFD + 6 字节读取 + 温度字/湿度字 CRC-8 校验
 *   - 整数换算 (含负温) 与 0..1000 湿度截断
 *   - 读 NACK(转换未完成) 的有界重读
 *   - 失败返回失败码且不修改输出 (NO_DEVICE / IO / CRC / RANGE)
 *   - 周期路径不发软复位 0x94 / 加热器命令
 *
 * 编译(在本目录, 需要 CW32 库头与 CMSIS Core 头):
 *   gcc -std=c11 -Wall -Wextra -Wno-int-to-pointer-cast \
 *       -I../USER/inc -I../COMMON -I../../../../Libraries/inc \
 *       -I<CMSIS 5.9.0 Core Include> \
 *       host_gxht40_check.c ../USER/src/gxht40.c ../USER/src/sf_i2c.c \
 *       ../USER/src/fw_core.c -lm -o host_gxht40_check.exe \
 *       && ./host_gxht40_check.exe
 *
 * 说明: mock 从机按 I²C 位时序重建; 板上电气/时序与真实器件交互仍由嵌入式测试
 *       按 TD-002 T-L2 用逻辑分析仪验证。
 *       帧向量由独立工具(非本驱动)按 gxht40.pdf §7.3/§7.5 计算, 其中
 *       CRC(0xBEEF)=0x92 为手册给出的参考向量。
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "gxht40.h"
#include "sensor_config.h"
#include "delay.h"

/* FWR-118 / tPU 时序观测 (delay_ms 桩计数, 见 [12]) */
static int delay_calls, delay_power_on_calls, delay_tmeas_calls;

void delay_ms(uint16_t ms)
{
    delay_calls++;
    if (ms == (uint16_t)GXHT40_POWER_ON_WAIT_MS) delay_power_on_calls++;
    if (ms == (uint16_t)GXHT40_MEASURE_WAIT_MS) delay_tmeas_calls++;
}

/* ---------------- mock bus ---------------- */
static int scl = 1, sda_master = 1, sda_dir = 1;
static int slave_driving, slave_sda;

/* mock 器件参数 */
static uint8_t mock_addr7 = 0x44;          /* 0 = 无器件 */
static uint8_t mock_frame[6];
static int mock_read_nack;                 /* 前 N 次读地址返回 NACK */

/* 观测 */
static int n_start, n_stop;
static int addr_w_count;
static uint8_t addr_w_seen[16];
static int cmd_count;
static uint8_t cmd_seen[16];
static int data_count;
static uint8_t data_seen[16];

/* slave engine */
enum { ST_IDLE = 0, ST_RX = 1, ST_TX = 2 };
static int st, bitidx, first_byte, ack_pending, ack_hold;
static uint8_t sh;
static int addr_match, addr_read;
static int txidx, n_tx_ack;

static void reset_slave(void)
{
    st = ST_RX; bitidx = 0; first_byte = 1; ack_pending = 0; ack_hold = 0;
    sh = 0; addr_match = 0; addr_read = 0; txidx = 0;
    slave_driving = 0; slave_sda = 1;
}

static uint8_t m_sda_read(void)
{
    if (slave_driving) return (uint8_t)slave_sda;
    if (sda_dir) return (uint8_t)sda_master;
    return 1; /* external pull-up */
}

static void m_sda(int level)
{
    if (scl == 1) {
        if (level == 0 && sda_master == 1) { n_start++; reset_slave(); }
        if (level == 1 && sda_master == 0) { n_stop++; }
    }
    sda_master = level;
}

static void m_scl(int level)
{
    if (level == 1) {
        if (st == ST_TX) {
            if (bitidx < 8) {
                bitidx++;
            } else {                            /* master ack clock */
                if (n_tx_ack < 16) n_tx_ack++;
                bitidx = 0;
                txidx++;
                if (txidx >= 6) st = ST_IDLE;
            }
        } else if (st == ST_RX) {
            if (bitidx < 8) {
                sh = (uint8_t)((sh << 1) | m_sda_read());
                bitidx++;
                if (bitidx == 8) ack_pending = 1;
            }
        }
    } else { /* falling edge */
        if (st == ST_TX) {
            if (bitidx < 8) {
                slave_driving = 1;
                slave_sda = (mock_frame[txidx] >> (7 - bitidx)) & 1;
            } else {
                slave_driving = 0;   /* release for master ack */
            }
        }
        if (ack_pending) {
            int is_addr = first_byte;
            int a7 = sh >> 1;
            int rd = sh & 1;
            int ack;
            if (!is_addr) {
                ack = 1;                       /* 数据字节: 器件在接收态则 ACK */
            } else if (mock_addr7 == 0 || a7 != (int)mock_addr7) {
                ack = 0;                       /* 地址不匹配: NACK */
            } else if (rd && mock_read_nack > 0) {
                mock_read_nack--;
                ack = 0;                       /* 读地址 NACK: 转换未完成 */
            } else {
                ack = 1;
            }
            ack_pending = 0;
            slave_driving = 1;
            slave_sda = ack ? 0 : 1;
            ack_hold = 1;
        } else if (ack_hold) {
            ack_hold = 0;
            slave_driving = 0;
            if (first_byte) {
                if (addr_w_count < 16) addr_w_seen[addr_w_count++] = sh;
                first_byte = 0;
                addr_match = (mock_addr7 != 0) && ((sh >> 1) == mock_addr7);
                addr_read = sh & 1;
                st = addr_match ? (addr_read ? ST_TX : ST_RX) : ST_IDLE;
                if (st == ST_TX) txidx = 0;
            } else if (addr_match && !addr_read) {
                if (cmd_count < 16) cmd_seen[cmd_count++] = sh;
                if (data_count < 16) data_seen[data_count++] = sh;
                st = ST_RX;
            } else {
                st = ST_IDLE;
            }
            sh = 0; bitidx = 0;
        }
    }
    scl = level;
}

static void p_sda_low(void)  { m_sda(0); }
static void p_sda_high(void) { m_sda(1); }
static void p_scl_low(void)  { m_scl(0); }
static void p_scl_high(void) { m_scl(1); }
static void p_dir_in(void)   { sda_dir = 0; }
static void p_dir_out(void)  { sda_dir = 1; }

static i2c_dev dev = {
    .name = "mock",
    .speed = 1,
    .port.sda_pin_out_low = p_sda_low,
    .port.sda_pin_out_high = p_sda_high,
    .port.scl_pin_out_low = p_scl_low,
    .port.scl_pin_out_high = p_scl_high,
    .port.sda_pin_read_level = m_sda_read,
    .port.sda_pin_dir_input = p_dir_in,
    .port.sda_pin_dir_output = p_dir_out,
};

/* ---------------- helpers ---------------- */
static int pass, fail;
#define CHECK(c, m) do { if (c) { pass++; printf("  PASS  %s\n", m); } \
                         else   { fail++; printf("  FAIL  %s\n", m); } } while (0)

static void obs_reset(void)
{
    n_start = 0; n_stop = 0; addr_w_count = 0; cmd_count = 0; data_count = 0; n_tx_ack = 0;
    reset_slave();
}

static void set_frame(const uint8_t f[6]) { memcpy(mock_frame, f, 6); }

/* 独立工具计算好的帧向量 (gxht40.pdf §7.3/§7.5) */
static const uint8_t F_DATASHEET[6] = {0xBE,0xEF,0x92,0x12,0x34,0x37}; /* T=0xBEEF(CRC 0x92), RH=0x1234 */
static const uint8_t F_25C_50RH[6]  = {0x66,0x66,0x93,0x72,0xB0,0xDC}; /* 25.0C / 50.0%RH */
static const uint8_t F_M10C_0RH[6]  = {0x33,0x33,0x88,0x00,0x00,0x81}; /* -10.0C / 0.0%RH */
static const uint8_t F_RANGE_LO[6]  = {0x00,0x00,0x81,0x72,0xB0,0xDC}; /* -45.0C -> 超范围 */
static const uint8_t F_RANGE_HI[6]  = {0xF8,0xCA,0x32,0x72,0xB0,0xDC}; /* 125.1C -> 超范围 */
static const uint8_t F_BAD_CRC[6]   = {0x66,0x66,0x00,0x72,0xB0,0xDC}; /* 温度字 CRC 错 */

int main(void)
{
    gxht40_status_t st;
    gxht40_diag_t   d;
    int16_t t;
    uint16_t h;

    i2c_init(&dev);
    gxht40_init(&dev);

    printf("[1] 手册参考向量: T=0xBEEF(CRC 0x92) / RH=0x1234\n");
    mock_addr7 = 0x44; mock_read_nack = 0; set_frame(F_DATASHEET); obs_reset();
    t = -1; h = 0xFFFF;
    st = gxht40_measure(&t, &h);
    CHECK(st == GXHT40_OK, "返回 GXHT40_OK");
    CHECK(t == 855, "temp_x10 == 855");
    CHECK(h == 29, "hum_x10 == 29");
    CHECK(cmd_count == 1 && cmd_seen[0] == GXHT40_CMD_MEASURE_HIGH_REP, "发送命令 0xFD 一次");
    CHECK(addr_w_count == 3 && addr_w_seen[0] == GXHT40_ADDR_WRITE_A
          && addr_w_seen[1] == GXHT40_ADDR_READ_A && addr_w_seen[2] == GXHT40_ADDR_WRITE_B,
          "首次访问完整重探: 0x88(命令) + 0x89(读) 后仍探 0x8A (FWR-118)");
    CHECK(gxht40_detected_addr7() == 0x44, "缓存地址 = 0x44");
    CHECK(cmd_count == 1, "只有被 ACK 的 0xFD 计入命令 (B 候选未被 ACK)");
    CHECK(n_start == 3 && n_stop == 4,
          "三次事务各一次 START; 4 次 STOP = A 命令 1 + A 读取 1 + B 探测 2 (NACK 路径先发 STOP 再补 STOP, 既有写法)");
    gxht40_diag_fetch(&d);
    CHECK(d.status == GXHT40_OK && d.attempt == 1 && d.read_retry == 0,
          "诊断: 成功轮 status=OK/attempt=1/read_retry=0");
    CHECK(d.ack44 == 1 && d.ack45 == 0, "诊断: 0x44 收到地址 ACK, 0x45 未被探测到 ACK");
    CHECK(d.raw_valid == 1 && memcmp(d.raw, F_DATASHEET, 6) == 0,
          "诊断: raw 为最近一次成功读回的 6 字节");

    printf("[2] 25.0C / 50.0%%RH\n");
    set_frame(F_25C_50RH); obs_reset();
    t = 0; h = 0;
    st = gxht40_measure(&t, &h);
    CHECK(st == GXHT40_OK && t == 250 && h == 500, "temp_x10=250, hum_x10=500");

    printf("[3] 负温 -10.0C / 0.0%%RH\n");
    set_frame(F_M10C_0RH); obs_reset();
    t = 0; h = 0xFFFF;
    st = gxht40_measure(&t, &h);
    CHECK(st == GXHT40_OK && t == -100 && h == 0, "temp_x10=-100 (保留符号), hum_x10=0");

    printf("[4] 地址探测与缓存: 器件仅在 0x45\n");
    mock_addr7 = 0x45; set_frame(F_25C_50RH);
    gxht40_init(&dev);          /* 清缓存, 强制重新探测 */
    obs_reset();
    t = 0; h = 0;
    st = gxht40_measure(&t, &h);
    CHECK(st == GXHT40_OK, "在 0x45 上测量成功");
    CHECK(addr_w_count == 3 && addr_w_seen[0] == GXHT40_ADDR_WRITE_A
          && addr_w_seen[1] == GXHT40_ADDR_WRITE_B
          && addr_w_seen[2] == GXHT40_ADDR_READ_B, "依次 0x88 -> 0x8A -> 0x8B");
    CHECK(gxht40_detected_addr7() == 0x45, "缓存地址 = 0x45");
    obs_reset();
    t = 0; h = 0;
    st = gxht40_measure(&t, &h);
    CHECK(st == GXHT40_OK && addr_w_count == 2 && addr_w_seen[0] == GXHT40_ADDR_WRITE_B
          && addr_w_seen[1] == GXHT40_ADDR_READ_B,
          "第二次直接使用缓存 0x8A/0x8B (不再探测 0x88)");
    mock_addr7 = 0x44;

    printf("[5] 读 NACK(转换未完成) 有界重读\n");
    set_frame(F_25C_50RH);
    gxht40_init(&dev);
    mock_read_nack = 2;         /* 前 2 次读地址 NACK, 第 3 次成功 */
    obs_reset();
    t = 0; h = 0;
    st = gxht40_measure(&t, &h);
    CHECK(st == GXHT40_OK && t == 250 && h == 500, "重读后成功 (2 次 NACK 被容忍)");
    CHECK(cmd_count == 1, "读 NACK 不重发命令 (仅 1 次 0xFD, 同一次测量内重读)");
    gxht40_diag_fetch(&d);
    CHECK(d.read_retry == 2 && d.status == GXHT40_OK,
          "诊断: 读事务消耗的失败重读次数 = 2 (前 2 次 NACK)");
    mock_read_nack = 0;

    printf("[6] CRC 错误: 重测用尽后返回失败且不改输出\n");
    set_frame(F_BAD_CRC); obs_reset();
    t = 1234; h = 4321;
    st = gxht40_measure(&t, &h);
    CHECK(st == GXHT40_ERR_CRC, "返回 GXHT40_ERR_CRC");
    CHECK(t == 1234 && h == 4321, "输出未被修改");
    CHECK(cmd_count == GXHT40_MEAS_RETRY, "整帧重测次数 == GXHT40_MEAS_RETRY");
    gxht40_diag_fetch(&d);
    CHECK(d.status == (uint8_t)GXHT40_ERR_CRC && d.attempt == (uint8_t)GXHT40_MEAS_RETRY,
          "诊断: CRC 错 - status=4 且 attempt=GXHT40_MEAS_RETRY");
    CHECK(d.ack44 == 1 && d.raw_valid == 1 && memcmp(d.raw, F_BAD_CRC, 6) == 0,
          "诊断: 保留最近一次成功读回的原始 6 字节 (供独立 CRC-8 复算)");

    printf("[7] 无器件: 返回 NO_DEVICE 且不改输出\n");
    mock_addr7 = 0; gxht40_init(&dev); obs_reset();
    t = 1234; h = 4321;
    st = gxht40_measure(&t, &h);
    CHECK(st == GXHT40_ERR_NO_DEVICE, "返回 GXHT40_ERR_NO_DEVICE");
    CHECK(t == 1234 && h == 4321, "输出未被修改");
    {
        int i, saw_a = 0, saw_b = 0;
        for (i = 0; i < addr_w_count; i++) {
            if (addr_w_seen[i] == GXHT40_ADDR_WRITE_A) saw_a = 1;
            if (addr_w_seen[i] == GXHT40_ADDR_WRITE_B) saw_b = 1;
        }
        CHECK(saw_a && saw_b, "两个候选地址 0x88/0x8A 都被探测过");
    }
    gxht40_diag_fetch(&d);
    CHECK(d.status == (uint8_t)GXHT40_ERR_NO_DEVICE && d.ack44 == 0 && d.ack45 == 0,
          "诊断: 无器件 - status=2 且两候选地址 ACK 均为 0");
    CHECK(d.raw_valid == 0, "诊断: 未读成功时 raw_valid=0 (打印时用占位)");

    printf("[8] 换算超范围: -45.0C 与 125.1C 视为无效\n");
    mock_addr7 = 0x44; gxht40_init(&dev);
    set_frame(F_RANGE_LO); obs_reset();
    t = 1234; h = 4321;
    st = gxht40_measure(&t, &h);
    CHECK(st == GXHT40_ERR_RANGE, "T=-45.0C -> GXHT40_ERR_RANGE");
    CHECK(t == 1234 && h == 4321, "输出未被修改");
    set_frame(F_RANGE_HI); obs_reset();
    st = gxht40_measure(&t, &h);
    CHECK(st == GXHT40_ERR_RANGE, "T=125.1C -> GXHT40_ERR_RANGE");
    gxht40_diag_fetch(&d);
    CHECK(d.status == (uint8_t)GXHT40_ERR_RANGE && d.raw_valid == 1,
          "诊断: 量程无效 - status=5 且保留原始 6 字节");

    printf("[9] 入参非法: 未绑定总线\n");
    gxht40_init(NULL);
    t = 1234; h = 4321;
    st = gxht40_measure(&t, &h);
    CHECK(st == GXHT40_ERR_PARAM, "返回 GXHT40_ERR_PARAM");
    CHECK(t == 1234 && h == 4321, "输出未被修改");
    gxht40_init(&dev);

    printf("[10] 周期路径命令白名单: 只允许 0xFD\n");
    {
        int i, bad = 0;
        for (i = 0; i < cmd_count; i++) {
            if (cmd_seen[i] != GXHT40_CMD_MEASURE_HIGH_REP) bad = 1;
        }
        CHECK(bad == 0, "所有已发命令均为 0xFD (无 0x94 软复位/加热器命令)");
    }

    printf("[11] FWR-118: 缓存地址读失败后必须重探两个候选地址\n");
    mock_addr7 = 0x45; set_frame(F_25C_50RH); gxht40_init(&dev);
    obs_reset();
    t = 0; h = 0;
    st = gxht40_measure(&t, &h);                       /* 首测: 探测 A 失败 -> B 成功, 缓存 0x45 */
    CHECK(st == GXHT40_OK && gxht40_detected_addr7() == 0x45, "器件在 0x45 上测量成功并缓存");
    mock_read_nack = 99;                               /* 读地址持续 NACK -> ERR_IO */
    obs_reset();
    t = 1234; h = 4321;
    st = gxht40_measure(&t, &h);
    CHECK(st == GXHT40_ERR_IO, "读 NACK 用尽 -> GXHT40_ERR_IO");
    CHECK(t == 1234 && h == 4321, "失败仍不修改输出");
    CHECK(gxht40_detected_addr7() == 0u, "缓存地址在任一失败后被清空 (FWR-118)");
    {
        int i, saw_a = 0;
        for (i = 0; i < addr_w_count; i++) {
            if (addr_w_seen[i] == GXHT40_ADDR_WRITE_A) saw_a = 1;
        }
        CHECK(saw_a, "缓存地址(0x45)读失败后仍重探了 0x44 (不再提前返回)");
    }
    mock_read_nack = 0;
    mock_addr7 = 0x44;
    obs_reset();
    t = 0; h = 0;
    st = gxht40_measure(&t, &h);                       /* 下一轮: 重新探测两个候选地址 */
    CHECK(st == GXHT40_OK && t == 250 && h == 500, "下一轮重新探测后测量成功");
    CHECK(gxht40_detected_addr7() == 0x44, "重新探测后缓存更新为 0x44");
    CHECK(addr_w_count == 3 && addr_w_seen[0] == GXHT40_ADDR_WRITE_A
          && addr_w_seen[1] == GXHT40_ADDR_READ_A && addr_w_seen[2] == GXHT40_ADDR_WRITE_B,
          "重探从 0x44 开始 (0x88/0x89) 并把 0x45 也探完 (0x8A, FWR-118)");
    mock_addr7 = 0x44;

    printf("[12] 首次访问前 tPU 上电余量 (只一次, 不进周期路径)\n");
    gxht40_init(&dev);
    delay_calls = 0; delay_power_on_calls = 0; delay_tmeas_calls = 0;
    t = 0; h = 0;
    st = gxht40_measure(&t, &h);
    CHECK(st == GXHT40_OK, "首测成功");
    CHECK(delay_power_on_calls == 1, "首次访问前恰好一次 GXHT40_POWER_ON_WAIT_MS (手册 tPU)\n");
    CHECK(delay_tmeas_calls == 1, "tMEAS 等待仍为 1 次 10 ms (>= 手册 8.3 ms 上限)");
    delay_power_on_calls = 0;
    t = 0; h = 0;
    st = gxht40_measure(&t, &h);
    CHECK(st == GXHT40_OK && delay_power_on_calls == 0,
          "后续测量不再重复上电余量 (仅首访)");

    printf("[13] FWR-118/TD-002 B18: CRC 失败后下一轮完整重探两候选地址\n");
    mock_addr7 = 0x44; mock_read_nack = 0; gxht40_init(&dev); set_frame(F_25C_50RH);
    obs_reset(); t = 0; h = 0;
    st = gxht40_measure(&t, &h);
    CHECK(st == GXHT40_OK && gxht40_detected_addr7() == 0x44, "基线: 缓存地址 0x44 测量成功");
    set_frame(F_BAD_CRC); obs_reset(); t = 0x1111; h = 0x2222;
    st = gxht40_measure(&t, &h);
    CHECK(st == GXHT40_ERR_CRC, "CRC 错周期返回 GXHT40_ERR_CRC");
    CHECK(t == 0x1111 && h == 0x2222, "CRC 错周期不修改输出");
    CHECK(gxht40_detected_addr7() == 0u, "CRC 错后缓存地址失效 (不再保留 0x44)");
    set_frame(F_25C_50RH); obs_reset(); t = 0; h = 0;
    st = gxht40_measure(&t, &h);
    CHECK(st == GXHT40_OK && t == 250 && h == 500, "下一轮恢复正常读数");
    {
        int i, saw_a = 0, saw_b = 0;
        for (i = 0; i < addr_w_count; i++) {
            if (addr_w_seen[i] == GXHT40_ADDR_WRITE_A) saw_a = 1;
            if (addr_w_seen[i] == GXHT40_ADDR_WRITE_B) saw_b = 1;
        }
        CHECK(saw_a && saw_b, "CRC 失败后的下一轮重探了 0x44 与 0x45 两个候选地址");
    }

    printf("[14] FWR-118/TD-002 B18: 量程无效后下一轮完整重探两候选地址\n");
    mock_addr7 = 0x44; gxht40_init(&dev); set_frame(F_25C_50RH);
    obs_reset(); t = 0; h = 0;
    st = gxht40_measure(&t, &h);
    CHECK(st == GXHT40_OK && gxht40_detected_addr7() == 0x44, "基线: 缓存地址 0x44 测量成功");
    set_frame(F_RANGE_LO); obs_reset(); t = 0x1111; h = 0x2222;
    st = gxht40_measure(&t, &h);
    CHECK(st == GXHT40_ERR_RANGE, "量程无效周期返回 GXHT40_ERR_RANGE");
    CHECK(gxht40_detected_addr7() == 0u, "量程无效后缓存地址失效");
    set_frame(F_25C_50RH); obs_reset(); t = 0; h = 0;
    st = gxht40_measure(&t, &h);
    CHECK(st == GXHT40_OK && t == 250 && h == 500, "下一轮恢复正常读数");
    {
        int i, saw_a = 0, saw_b = 0;
        for (i = 0; i < addr_w_count; i++) {
            if (addr_w_seen[i] == GXHT40_ADDR_WRITE_A) saw_a = 1;
            if (addr_w_seen[i] == GXHT40_ADDR_WRITE_B) saw_b = 1;
        }
        CHECK(saw_a && saw_b, "量程无效后的下一轮重探了 0x44 与 0x45 两个候选地址");
    }

    printf("\n==== result: %d passed, %d failed ====\n", pass, fail);
    return fail ? 1 : 0;
}
