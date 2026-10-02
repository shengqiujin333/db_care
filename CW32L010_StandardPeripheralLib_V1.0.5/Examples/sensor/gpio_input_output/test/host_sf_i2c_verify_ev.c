/*
 * Independent verification harness for ITEM-002 (sf_i2c register-less primitives).
 * Author: embedded_tester.embedded_verification.
 * Links the REAL ../USER/src/sf_i2c.c against a bit-level mock bus + mock slave,
 * so the exact wire sequence and the master ACK/NACK positions are checked
 * without an MCU. Deliberately an independent model (not a copy of the
 * implementer's test/host_sf_i2c_bus_check.c).
 *
 * Build:  gcc -std=c11 -Wall -Wextra -I../USER/inc i2c_verify.c ../USER/src/sf_i2c.c -o i2c_verify
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "sf_i2c.h"

/* ---------------- event log ---------------- */
enum { EV_START = 1, EV_STOP, EV_TX, EV_TXACK, EV_RX, EV_RXMACK };
static int ev_code[2048], ev_val[2048], ev_n;
static void ev_add(int c, int v) { if (ev_n < 2048) { ev_code[ev_n] = c; ev_val[ev_n] = v; ev_n++; } }
static void ev_reset(void) { ev_n = 0; }
static void ev_dump(const char *tag)
{
    printf("    transcript[%s]:", tag);
    for (int i = 0; i < ev_n; i++) {
        switch (ev_code[i]) {
        case EV_START:  printf(" START"); break;
        case EV_STOP:   printf(" STOP"); break;
        case EV_TX:     printf(" TX(%02X)", ev_val[i]); break;
        case EV_TXACK:  printf(" %s", ev_val[i] ? "NACK" : "ACK"); break;
        case EV_RX:     printf(" RX(%02X)", ev_val[i]); break;
        case EV_RXMACK: printf(" M-%s", ev_val[i] ? "NACK" : "ACK"); break;
        }
    }
    printf("\n");
}

/* ---------------- mock slave state ---------------- */
static int   slave_present = 1;
static uint8_t slave_a7 = 0x44;
static uint8_t rd_data[16];
static int   rd_len;
static uint8_t wr_bytes[16];
static int   wr_len;
static int   wr_idx_n = 0, nack_nth_wr = -1;   /* inject a NACK on the n-th written byte */
static int   saw_bad_addr = -1;

enum { ST_IDLE = 0, ST_ADDR_BITS, ST_ADDR_ACK, ST_WR_BITS, ST_WR_ACK, ST_RD_BITS, ST_RD_MACK };
static int st, bitcnt, pending_ack, addr_byte, rd_idx_sh;
static uint8_t sh;

static void slave_start_reset(void)
{
    st = ST_ADDR_BITS; bitcnt = 0; addr_byte = 0; pending_ack = 0; wr_idx_n = 0;
}
static void slave_stop(void) { st = ST_IDLE; }

/* ---------------- mock bus ---------------- */
static int scl = 1, m_sda = 1, m_drive = 1, slave_sda = 1;
static int bus(void) { return m_drive ? m_sda : (slave_sda ? 1 : 0); }

static void set_master_sda(int v)
{
    if (v == m_sda) return;
    if (m_drive && scl) {
        if (v == 0) { ev_add(EV_START, 0); slave_start_reset(); }   /* START */
        else        { ev_add(EV_STOP, 0);  slave_stop(); }          /* STOP  */
    }
    m_sda = v;
}
static void sda_low(void)  { set_master_sda(0); }
static void sda_high(void) { set_master_sda(1); }
static void dir_in(void)   { m_drive = 0; }
static void dir_out(void)  { m_drive = 1; }
static uint8_t sda_read(void) { return (uint8_t)bus(); }

static void slave_on_fall(void)
{
    if (st == ST_ADDR_ACK || st == ST_WR_ACK) {
        slave_sda = pending_ack ? 0 : 1;                 /* drive ACK on the next rise */
    } else if (st == ST_RD_BITS) {
        uint8_t b = (rd_idx_sh < rd_len) ? rd_data[rd_idx_sh] : 0xFF;
        slave_sda = ((b >> (7 - bitcnt)) & 1) ? 1 : 0;   /* slave drives data bit */
    } else if (st == ST_RD_MACK) {
        slave_sda = 1;                                   /* release for the master ack */
    }
}
static void slave_on_rise(void)
{
    int lvl = bus();
    switch (st) {
    case ST_ADDR_BITS:
        addr_byte = (addr_byte << 1) | lvl; bitcnt++;
        if (bitcnt == 8) {
            ev_add(EV_TX, addr_byte & 0xFF);
            pending_ack = (slave_present && ((addr_byte & 0xFE) == (slave_a7 << 1))) ? 1 : 0;
            if (!pending_ack) saw_bad_addr = addr_byte & 0xFF;
            st = ST_ADDR_ACK;
        }
        break;
    case ST_ADDR_ACK:
        ev_add(EV_TXACK, lvl ? 1 : 0);
        if (!pending_ack) { st = ST_IDLE; break; }
        if (addr_byte & 1) { st = ST_RD_BITS; bitcnt = 0; rd_idx_sh = 0; }
        else               { st = ST_WR_BITS; bitcnt = 0; }
        break;
    case ST_WR_BITS:
        addr_byte = (addr_byte << 1) | lvl; bitcnt++;
        if (bitcnt == 8) {
            ev_add(EV_TX, addr_byte & 0xFF);
            if (wr_len < 16) wr_bytes[wr_len++] = (uint8_t)addr_byte;
            wr_idx_n++;
            pending_ack = (slave_present && (wr_idx_n != nack_nth_wr)) ? 1 : 0;
            st = ST_WR_ACK;
        }
        break;
    case ST_WR_ACK:
        ev_add(EV_TXACK, lvl ? 1 : 0);
        if (!pending_ack) { st = ST_IDLE; break; }
        st = ST_WR_BITS; bitcnt = 0;
        break;
    case ST_RD_BITS:
        if (bitcnt == 0) sh = 0;
        sh = (uint8_t)((sh << 1) | lvl);
        bitcnt++;
        if (bitcnt == 8) { ev_add(EV_RX, sh); rd_idx_sh++; st = ST_RD_MACK; }
        break;
    case ST_RD_MACK:
        ev_add(EV_RXMACK, lvl ? 1 : 0);
        if (lvl == 0) { st = ST_RD_BITS; bitcnt = 0; }   /* master ACK -> more bytes */
        else          { st = ST_IDLE; }
        break;
    default: break;
    }
}
static void scl_low_(void)  { if (scl == 1) { scl = 0; if (st != ST_IDLE) slave_on_fall(); } }
static void scl_high_(void) { if (scl == 0) { scl = 1; if (st != ST_IDLE) slave_on_rise(); } }

static i2c_dev dev = {
    .name = "mock", .speed = 1,
    .port.sda_pin_out_low = sda_low, .port.sda_pin_out_high = sda_high,
    .port.scl_pin_out_low = scl_low_, .port.scl_pin_out_high = scl_high_,
    .port.sda_pin_read_level = sda_read,
    .port.sda_pin_dir_input = dir_in, .port.sda_pin_dir_output = dir_out,
};

static void bus_reset(void)
{
    ev_reset();
    scl = 1; m_sda = 1; m_drive = 1; slave_sda = 1;
    st = ST_IDLE; bitcnt = 0; pending_ack = 0; addr_byte = 0; sh = 0;
    rd_len = rd_idx_sh = wr_len = 0; saw_bad_addr = -1; wr_idx_n = 0; nack_nth_wr = -1;
}

static int pass, fail;
#define CHECK(c, m) do { if (c) { pass++; printf("    PASS  %s\n", m); } \
                         else   { fail++; printf("    FAIL  %s (line %d)\n", m, __LINE__); } } while (0)
static int ev_get(int code, int nth) { int c = 0; for (int i = 0; i < ev_n; i++) if (ev_code[i]==code && ++c==nth) return ev_val[i]; return -1; }
static int ev_count(int code) { int c = 0; for (int i = 0; i < ev_n; i++) if (ev_code[i]==code) c++; return c; }

int main(void)
{
    printf("==== independent sf_i2c verification (embedded_tester) ====\n");
    i2c_init(&dev);

    /* ---- T1: i2c_write_cmd, present slave ---- */
    printf("[T1] i2c_write_cmd(0x88,0xFD) with ACKing slave\n");
    bus_reset(); slave_present = 1; slave_a7 = 0x44; slave_start_reset();
    sf_i2c_err r = i2c_write_cmd(&dev, 0x88, 0xFD);
    ev_dump("T1");
    CHECK(r == SF_I2C_SUCCESS, "returns SF_I2C_SUCCESS");
    CHECK(ev_count(EV_START) == 1 && ev_count(EV_STOP) == 1, "exactly 1 START and 1 STOP");
    CHECK(ev_get(EV_TX, 1) == 0x88 && ev_get(EV_TX, 2) == 0xFD, "wire bytes = 0x88 then 0xFD (no register address)");
    CHECK(ev_get(EV_TXACK, 1) == 0 && ev_get(EV_TXACK, 2) == 0, "both bytes ACKed");
    CHECK(wr_len == 1 && wr_bytes[0] == 0xFD, "slave received exactly the command byte");
    CHECK(scl == 1 && bus() == 1, "bus released (idle high) after transaction");
    CHECK(ev_get(EV_RX, 1) == -1, "no read bytes in a command write");

    /* ---- T2: i2c_write_cmd, absent slave ---- */
    printf("[T2] i2c_write_cmd with no slave response\n");
    bus_reset(); slave_present = 0; slave_start_reset();
    r = i2c_write_cmd(&dev, 0x88, 0xFD);
    ev_dump("T2");
    CHECK(r == SF_I2C_TIMEOUT, "returns SF_I2C_TIMEOUT");
    CHECK(ev_count(EV_STOP) >= 1, "bus released with STOP on the failure path");
    CHECK(ev_get(EV_TX, 2) == -1, "command byte NOT sent after address NACK (fail fast)");
    CHECK(scl == 1 && bus() == 1, "bus idle high afterwards");

    /* ---- T3: i2c_read_bytes, present slave, 6 bytes ---- */
    printf("[T3] i2c_read_bytes(0x89,6)\n");
    bus_reset(); slave_present = 1;
    rd_data[0]=0x61; rd_data[1]=0x9C; rd_data[2]=0x2B; rd_data[3]=0x8A; rd_data[4]=0x44; rd_data[5]=0x75;
    rd_len = 6; slave_start_reset();
    uint8_t buf[8]; memset(buf, 0, sizeof buf);
    r = i2c_read_bytes(&dev, 0x89, buf, 6);
    ev_dump("T3");
    CHECK(r == SF_I2C_SUCCESS, "returns SF_I2C_SUCCESS");
    CHECK(ev_count(EV_START) == 1 && ev_count(EV_STOP) == 1, "exactly 1 START and 1 STOP");
    CHECK(ev_get(EV_TX, 1) == 0x89, "read address byte 0x89 sent first");
    CHECK(memcmp(buf, rd_data, 6) == 0, "6 bytes received match slave data");
    CHECK(ev_count(EV_RXMACK) == 6, "6 master response bits observed");
    CHECK(ev_get(EV_RXMACK, 1)==0 && ev_get(EV_RXMACK,2)==0 && ev_get(EV_RXMACK,3)==0 &&
          ev_get(EV_RXMACK, 4)==0 && ev_get(EV_RXMACK,5)==0 && ev_get(EV_RXMACK,6)==1,
          "master ACK on bytes 1-5, NACK on last byte");
    CHECK(scl == 1 && bus() == 1, "bus released after read");

    /* ---- T4: i2c_read_bytes, absent slave (e.g. GXHT40 conversion not ready) ---- */
    printf("[T4] i2c_read_bytes with no slave response\n");
    bus_reset(); slave_present = 0; slave_start_reset();
    memset(buf, 0xAA, sizeof buf);
    r = i2c_read_bytes(&dev, 0x89, buf, 6);
    ev_dump("T4");
    CHECK(r == SF_I2C_TIMEOUT, "returns SF_I2C_TIMEOUT when read address is NACKed");
    CHECK(ev_count(EV_RX) == 0, "no data bytes read");
    CHECK(ev_count(EV_STOP) >= 1, "STOP issued on failure path (bus not stuck)");

    /* ---- T5: length == 0 short circuit ---- */
    printf("[T5] i2c_read_bytes length=0\n");
    bus_reset(); slave_present = 1; slave_start_reset();
    r = i2c_read_bytes(&dev, 0x89, buf, 0);
    CHECK(r == SF_I2C_SUCCESS, "returns SF_I2C_SUCCESS");
    CHECK(ev_n == 0, "no bus activity for empty request");

    /* ---- T6: write-form address accepted as well ---- */
    printf("[T6] i2c_read_bytes with write-form address 0x88\n");
    bus_reset(); slave_present = 1; rd_data[0]=0x11; rd_len = 1; slave_start_reset();
    r = i2c_read_bytes(&dev, 0x88, buf, 1);
    ev_dump("T6");
    CHECK(r == SF_I2C_SUCCESS && ev_get(EV_TX, 1) == 0x89, "address byte becomes 0x89 on the wire");
    CHECK(buf[0] == 0x11, "byte read correctly");

    /* ---- T6b: command byte NACKed while the address is ACKed ---- */
    printf("[T6b] i2c_write_cmd: address ACKed, command byte NACKed\n");
    bus_reset(); slave_present = 1; slave_a7 = 0x44; slave_start_reset(); nack_nth_wr = 1;
    r = i2c_write_cmd(&dev, 0x88, 0xFD);
    ev_dump("T6b");
    CHECK(r == SF_I2C_TIMEOUT, "returns SF_I2C_TIMEOUT when the command byte is NACKed");
    CHECK(ev_count(EV_STOP) >= 1, "bus released with STOP");
    CHECK(scl == 1 && bus() == 1, "bus idle high afterwards");

    /* ---- T7: regression - existing i2c_write_multi_byte unchanged ---- */
    printf("[T7] regression: i2c_write_multi_byte(0x70,0xAC,{0x33,0x00},2)\n");
    bus_reset(); slave_present = 1; slave_a7 = 0x38; slave_start_reset();
    uint8_t wr[2] = {0x33, 0x00};
    r = i2c_write_multi_byte(&dev, 0x70, 0xAC, wr, 2);
    ev_dump("T7");
    CHECK(r == SF_I2C_SUCCESS, "returns SF_I2C_SUCCESS");
    CHECK(ev_get(EV_TX,1)==0x70 && ev_get(EV_TX,2)==0xAC && ev_get(EV_TX,3)==0x33 && ev_get(EV_TX,4)==0x00,
          "legacy sequence addr|reg|data0|data1 preserved");
    CHECK(ev_count(EV_START)==1 && ev_count(EV_STOP)==1, "legacy single START/STOP framing preserved");

    /* ---- T8: regression - existing i2c_read_multi_byte (repeated START) ---- */
    printf("[T8] regression: i2c_read_multi_byte(0x70,0x71,buf,5)\n");
    bus_reset(); slave_present = 1; slave_a7 = 0x38;
    for (int i=0;i<5;i++) rd_data[i] = (uint8_t)(0xA0+i);
    rd_len = 5; slave_start_reset();
    uint8_t rb[5]; memset(rb, 0, sizeof rb);
    i2c_read_multi_byte(&dev, 0x70, 0x71, rb, 5);
    ev_dump("T8");
    CHECK(ev_count(EV_START)==2, "legacy read uses START ... repeated START");
    CHECK(ev_get(EV_TX,1)==0x70 && ev_get(EV_TX,2)==0x71 && ev_get(EV_TX,3)==0x71,
          "legacy sequence write-addr|reg then read-addr");
    CHECK(memcmp(rb, rd_data, 5)==0, "legacy read data unchanged");
    CHECK(ev_get(EV_RXMACK,5)==1, "legacy read NACKs the last byte");

    printf("==== result: %d passed, %d failed ====\n", pass, fail);
    return fail ? 1 : 0;
}
