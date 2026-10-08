/*
 * sf_i2c 新增原语的实现侧自检 (宿主机 mock 总线, 不进入固件构建)
 *
 * 目的: 确定性验证 ITEM-002 的两个原语在"无寄存器地址"器件上的总线序列与 ACK 结果返回:
 *   - i2c_write_cmd  : START -> 地址+W -> 命令 -> STOP
 *   - i2c_read_bytes : START -> 地址+R -> N 字节(前 N-1 ACK, 末字节 NACK) -> STOP
 *   - 从机不响应时返回 SF_I2C_TIMEOUT 且总线被释放(STOP)
 *   - 既有 i2c 函数未被改动 (git diff 0 deletion; 本文件不覆盖其行为)
 *
 * 编译(在本目录):
 *   gcc -std=c11 -Wall -Wextra -I../USER/inc host_sf_i2c_bus_check.c ../USER/src/sf_i2c.c \
 *       -o host_sf_i2c_bus_check.exe && ./host_sf_i2c_bus_check.exe
 *
 * 说明: 这是实现侧的驱动级自检(mock 总线), 不依赖 MCU 寄存器; 板上电气/时序仍由
 *       嵌入式测试按 TD-002 T-L2 用逻辑分析仪验证。
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "sf_i2c.h"

/* ---------------- mock bus ---------------- */
static int scl = 1, sda_master = 1, sda_dir = 1;
static int slave_driving, slave_sda;
static int slave_present = 1;

/* slave engine */
enum { ST_IDLE = 0, ST_RX = 1, ST_TX = 2 };
static int st, bitidx, first_byte, ack_pending, ack_hold;
static uint8_t sh;
static int addr_match, addr_read;
static uint8_t txbuf[6] = {0x61, 0xA8, 0x00, 0x66, 0x66, 0x00};
static int txidx, n_tx_ack;
static int tx_ack_bits[16];

/* observations */
static uint8_t rx_bytes[16];
static int n_rx;
static int n_start, n_stop;

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
                bitidx++;   /* keep driving bit0 until the next falling edge */
            } else {                                /* master ack clock */
                if (n_tx_ack < 16) tx_ack_bits[n_tx_ack++] = (sda_dir ? sda_master : 1);
                bitidx = 0;
                txidx++;
                if (txidx >= 6) st = ST_IDLE;
            }
        } else if (st == ST_RX) {
            if (bitidx < 8) {
                sh = (uint8_t)((sh << 1) | m_sda_read());
                bitidx++;
                if (bitidx == 8) ack_pending = 1; /* drive ACK on next falling edge */
            }
            /* bitidx == 8: ack clock, master samples our ACK */
        }
    } else { /* falling edge */
        if (st == ST_TX) {
            if (bitidx < 8) {
                slave_driving = 1;
                slave_sda = (txbuf[txidx] >> (7 - bitidx)) & 1;
            } else {
                slave_driving = 0;   /* release for master ack */
            }
        }
        if (ack_pending) {
            int ack = slave_present && (!first_byte || ((sh >> 1) == 0x44u));
            ack_pending = 0;
            slave_driving = 1;
            slave_sda = ack ? 0 : 1;
            ack_hold = 1;
        } else if (ack_hold) {
            ack_hold = 0;
            slave_driving = 0;
            if (n_rx < 16) rx_bytes[n_rx++] = sh;
            if (first_byte) {
                first_byte = 0;
                addr_match = slave_present && ((sh >> 1) == 0x44u);
                addr_read = sh & 1;
                st = addr_match ? (addr_read ? ST_TX : ST_RX) : ST_IDLE;
                if (st == ST_TX) txidx = 0;
            } else {
                st = slave_present ? ST_RX : ST_IDLE;
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

/* ---------------- checks ---------------- */
static int pass, fail;
#define CHECK(c, m) do { if (c) { pass++; printf("  PASS  %s\n", m); } \
                         else   { fail++; printf("  FAIL  %s\n", m); } } while (0)

int main(void)
{
    sf_i2c_err e;
    uint8_t buf[6];

    i2c_init(&dev);

    printf("[1] i2c_write_cmd: START -> 0x88 -> 0xFD -> STOP\n");
    reset_slave(); n_rx = 0; n_start = 0; n_stop = 0; slave_present = 1;
    e = i2c_write_cmd(&dev, 0x88, 0xFD);
    CHECK(e == SF_I2C_SUCCESS, "返回 SF_I2C_SUCCESS (命令被 ACK)");
    CHECK(n_start == 1 && n_stop == 1, "恰好 1 个 START 与 1 个 STOP");
    CHECK(n_rx == 2 && rx_bytes[0] == 0x88 && rx_bytes[1] == 0xFD,
          "线上字节为 地址+W(0x88) 后接 命令(0xFD), 无寄存器地址");
    CHECK(m_sda_read() == 1 && scl == 1, "总线空闲释放(SCL/SDA 均为高)");

    printf("[2] i2c_read_bytes: START -> 0x89 -> 6B(ACKx5,NACK) -> STOP\n");
    reset_slave(); n_rx = 0; n_start = 0; n_stop = 0; n_tx_ack = 0; slave_present = 1;
    memset(buf, 0, sizeof(buf));
    e = i2c_read_bytes(&dev, 0x89, buf, 6);
    CHECK(e == SF_I2C_SUCCESS, "返回 SF_I2C_SUCCESS");
    CHECK(n_start == 1 && n_stop == 1, "恰好 1 个 START 与 1 个 STOP");
    CHECK(n_rx == 1 && rx_bytes[0] == 0x89, "仅先发读地址字节 0x89");
    if (memcmp(buf, txbuf, 6) != 0) {
        int k;
        printf("        got:");
        for (k = 0; k < 6; k++) printf(" %02X", buf[k]);
        printf("\n        exp:");
        for (k = 0; k < 6; k++) printf(" %02X", txbuf[k]);
        printf("\n");
    }
    CHECK(memcmp(buf, txbuf, 6) == 0, "读回 6 字节与从机数据逐字节一致");
    CHECK(n_tx_ack == 6 && tx_ack_bits[0] == 0 && tx_ack_bits[1] == 0 &&
          tx_ack_bits[2] == 0 && tx_ack_bits[3] == 0 && tx_ack_bits[4] == 0 &&
          tx_ack_bits[5] == 1, "前 5 字节主机 ACK、第 6 字节主机 NACK");

    printf("[3] 从机不响应: ACK 失败作为返回值\n");
    reset_slave(); n_rx = 0; n_start = 0; n_stop = 0; slave_present = 0;
    e = i2c_write_cmd(&dev, 0x88, 0xFD);
    CHECK(e == SF_I2C_TIMEOUT, "i2c_write_cmd 返回 SF_I2C_TIMEOUT");
    CHECK(n_stop >= 1, "超时路径已发出 STOP 释放总线");
    e = i2c_read_bytes(&dev, 0x89, buf, 6);
    CHECK(e == SF_I2C_TIMEOUT, "i2c_read_bytes 返回 SF_I2C_TIMEOUT (读地址未 ACK)");
    CHECK(n_stop >= 1, "超时路径已发出 STOP 释放总线");

    printf("[4] length==0: 无总线动作\n");
    n_start = 0; n_stop = 0; slave_present = 1;
    e = i2c_read_bytes(&dev, 0x89, buf, 0);
    CHECK(e == SF_I2C_SUCCESS, "返回 SF_I2C_SUCCESS");
    CHECK(n_start == 0 && n_stop == 0, "未产生任何 START/STOP");

    printf("[5] i2c_probe_addr: 只发地址字节的有界探测 (FWR-116)" "\n");
    reset_slave(); n_rx = 0; n_start = 0; n_stop = 0; slave_present = 1;
    e = i2c_probe_addr(&dev, 0x88);
    CHECK(e == SF_I2C_SUCCESS, "器件在 0x44: 返回 SF_I2C_SUCCESS (地址被 ACK)");
    CHECK(n_start == 1 && n_stop == 1, "恰好 1 个 START 与 1 个 STOP");
    CHECK(n_rx == 1 && rx_bytes[0] == 0x88, "线上只有地址写字节 0x88 (无命令字节/无数据)");
    CHECK(m_sda_read() == 1 && scl == 1, "探测后总线释放(SCL/SDA 均为高)");

    printf("[6] i2c_probe_addr: 无器件地址返回超时且释放总线\n");
    reset_slave(); n_rx = 0; n_start = 0; n_stop = 0; slave_present = 1;
    e = i2c_probe_addr(&dev, 0x8A);
    CHECK(e == SF_I2C_TIMEOUT, "0x45 无器件: 返回 SF_I2C_TIMEOUT");
    CHECK(n_stop >= 1, "NACK 后已发出 STOP 释放总线");
    CHECK(n_rx == 1 && rx_bytes[0] == 0x8A, "线上只有地址写字节 0x8A, 未继续写命令");
    CHECK(m_sda_read() == 1 && scl == 1, "超时后总线仍释放");

    printf("\n==== result: %d passed, %d failed ====\n", pass, fail);
    return fail ? 1 : 0;
}
