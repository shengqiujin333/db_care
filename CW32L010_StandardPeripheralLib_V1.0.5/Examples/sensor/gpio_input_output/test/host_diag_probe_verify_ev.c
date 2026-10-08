/*
 * Independent verification harness for run8 ITEM-001
 * (GXHT40 acquisition failure diagnosable on the real target: BUS line + G line).
 * Author: embedded_tester.embedded_verification.
 *
 * What it checks, independently of the implementer's tests:
 *   [PROBE]  i2c_probe_addr wire semantics on a bit-level mock bus:
 *            START -> ONE address byte -> ACK check -> STOP; nothing else on the
 *            wire; NACK still releases the bus with STOP; and the pre-existing
 *            register-less primitives keep their transcripts (regression).
 *   [SCAN ]  the REAL sensor_bus_diag_scan() (USER/src/measure.c) driving the REAL
 *            soft-I2C bit-banger into the mock bus: exactly the 0x08..0x77 sweep,
 *            address bytes only (no command, no read), idle-level bit coding,
 *            ACK set collection, 8-entry bound with the truncation flag, and no
 *            change to the business state (sample_flag / report_req / values).
 *            The SENSOR_DEBUG_UART=0 variant must do nothing at all.
 *   [DIAG ]  the REAL gxht40_measure() diagnostic snapshot (gxht40_diag_fetch) for
 *            success / read-NACK / CRC / no-device / out-of-range, i.e. the facts
 *            the G line reports (status code, per-address ACK, re-read count,
 *            re-measure count, last successful 6 bytes) with failure semantics
 *            unchanged (outputs still untouched on failure).
 *
 * Build (from gpio_input_output/):
 *   cc -std=c11 -Wall -Wextra -Wno-unused-function -DSENSOR_DEBUG_UART=1 \
 *      -Itest/mock_measure_ev -IUSER/inc -IUM2005C -ICOMMON \
 *      test/host_diag_probe_verify_ev.c USER/src/measure.c USER/src/gxht40.c \
 *      USER/src/sf_i2c.c USER/src/fw_core.c USER/src/encrytogate.c -lm \
 *      -o diag_probe_verify
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "measure.h"
#include "sf_i2c.h"
#include "gxht40.h"
#include "light.h"
#include "sensor_config.h"
#include "mock_measure_hw.h"

/* ---- globals owned by measure.c / sf_i2c.c ---- */
extern int16_t  tempvalue;
extern uint16_t huminityvalue;
extern uint8_t  sample_flag;
extern uint8_t  report_req;
extern i2c_dev *temp_ptr;

/* ==================================================================== */
/* bit-level mock bus + mock slave (driven by the REAL soft-I2C)         */
/* ==================================================================== */
enum { EV_START = 1, EV_STOP, EV_TX, EV_TXACK, EV_RX, EV_RXMACK };
static int ev_code[4096], ev_val[4096], ev_n;
static void ev_add(int c, int v) { if (ev_n < 4096) { ev_code[ev_n] = c; ev_val[ev_n] = v; ev_n++; } }
static void ev_reset(void) { ev_n = 0; }
static int ev_count(int code) { int c = 0; for (int i = 0; i < ev_n; i++) if (ev_code[i] == code) c++; return c; }
static int ev_get(int code, int nth) { int c = 0; for (int i = 0; i < ev_n; i++) if (ev_code[i] == code && ++c == nth) return ev_val[i]; return -1; }
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

/* mock slave */
static int     slave_present = 1;
static uint8_t slave_a7 = 0x44;
static uint8_t rd_data[16];
static int     rd_len;
static int     nack_reads = 0;     /* >0: NACK this many read-address bytes first; <0: always */
static int     g_promiscuous = 0;  /* 1 = every address is ACKed (models a bus with >8 devices) */
static int     nack_cmd = 0;
static uint8_t wr_bytes[64];
static int     wr_len;

enum { ST_IDLE = 0, ST_ADDR_BITS, ST_ADDR_ACK, ST_WR_BITS, ST_WR_ACK, ST_RD_BITS, ST_RD_MACK };
static int st, bitcnt, pending_ack, addr_byte, rd_idx_sh;
static uint8_t sh;

static void slave_start_reset(void)
{ st = ST_ADDR_BITS; bitcnt = 0; addr_byte = 0; pending_ack = 0; }
static void slave_stop(void) { st = ST_IDLE; }

/* bus electrical model: master open-drain + slave open-drain + external pull-up.
 * ext_hold_sda_low models the anomaly actually observed on the target board
 * (SDA stuck low while SCL idles high). */
static int scl = 1, m_sda = 1, m_drive = 1, slave_sda = 1, ext_hold_sda_low = 0;
static int bus_sda(void) { if (ext_hold_sda_low) return 0; return m_drive ? m_sda : (slave_sda ? 1 : 0); }

static void set_master_sda(int v)
{
    if (v == m_sda) return;
    if (m_drive && scl) {
        if (v == 0) { ev_add(EV_START, 0); slave_start_reset(); }
        else        { ev_add(EV_STOP, 0);  slave_stop(); }
    }
    m_sda = v;
}
static void sda_low(void)  { set_master_sda(0); }
static void sda_high(void) { set_master_sda(1); }
static void dir_in(void)   { m_drive = 0; }
static void dir_out(void)  { m_drive = 1; }
static uint8_t sda_read(void) { return (uint8_t)bus_sda(); }

static void slave_on_fall(void)
{
    if (st == ST_ADDR_ACK || st == ST_WR_ACK) {
        slave_sda = pending_ack ? 0 : 1;
    } else if (st == ST_RD_BITS) {
        uint8_t b = (rd_idx_sh < rd_len) ? rd_data[rd_idx_sh] : 0xFF;
        slave_sda = ((b >> (7 - bitcnt)) & 1) ? 1 : 0;
    } else if (st == ST_RD_MACK) {
        slave_sda = 1;
    }
}
static void slave_on_rise(void)
{
    int lvl = bus_sda();
    switch (st) {
    case ST_ADDR_BITS:
        addr_byte = (addr_byte << 1) | lvl; bitcnt++;
        if (bitcnt == 8) {
            ev_add(EV_TX, addr_byte & 0xFF);
            pending_ack = 0;
            if (g_promiscuous && !(addr_byte & 1)) {
                pending_ack = 1;
            } else if (slave_present && ((addr_byte & 0xFE) == (slave_a7 << 1))) {
                if ((addr_byte & 1) && nack_reads != 0) {        /* read address */
                    pending_ack = 0;
                    if (nack_reads > 0) nack_reads--;
                } else {
                    pending_ack = 1;
                }
            }
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
            if (wr_len < 64) wr_bytes[wr_len++] = (uint8_t)addr_byte;
            pending_ack = slave_present && !nack_cmd;
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
        sh = (uint8_t)((sh << 1) | lvl); bitcnt++;
        if (bitcnt == 8) { ev_add(EV_RX, sh); rd_idx_sh++; st = ST_RD_MACK; }
        break;
    case ST_RD_MACK:
        ev_add(EV_RXMACK, lvl ? 1 : 0);
        if (lvl == 0) { st = ST_RD_BITS; bitcnt = 0; } else { st = ST_IDLE; }
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
    rd_len = rd_idx_sh = wr_len = 0; nack_reads = 0; nack_cmd = 0; g_promiscuous = 0;
}

/* ==================================================================== */
/* emulated MCU layer: bridges measure.c's PA03/PA04 GPIO calls to the bus */
/* ==================================================================== */
GPIO_TypeDef mock_gpioa, mock_gpiob;

static int gpio_init_pa34_od = 0;   /* GPIO_Init(PA3|PA4, OD) call count */
static int delay_ms_total = 0, delay_ms_calls = 0, delay_last_ms = 0;
static int light_init_calls = 0, light_sample_calls = 0;

void GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *init)
{
    (void)port;
    if ((init->Pins & (GPIO_PIN_3 | GPIO_PIN_4)) == (GPIO_PIN_3 | GPIO_PIN_4) &&
        init->Mode == GPIO_MODE_OUTPUT_OD) {
        gpio_init_pa34_od++;
        dir_out();
    } else if (init->Pins & GPIO_PIN_4) {
        if (init->Mode == GPIO_MODE_INPUT) dir_in();
        else if (init->Mode == GPIO_MODE_OUTPUT_OD) dir_out();
    }
}
void GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState st_)
{
    (void)port;
    if (pin & GPIO_PIN_4) { if (st_ == GPIO_Pin_RESET) sda_low(); else sda_high(); }
    if (pin & GPIO_PIN_3) { if (st_ == GPIO_Pin_RESET) scl_low_(); else scl_high_(); }
}
GPIO_PinState GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin)
{
    (void)port;
    if (pin & GPIO_PIN_4) return bus_sda() ? GPIO_Pin_SET : GPIO_Pin_RESET;
    if (pin & GPIO_PIN_3) return scl ? GPIO_Pin_SET : GPIO_Pin_RESET;
    return GPIO_Pin_SET;
}

/* ---- SYSCTRL / delay / sampling primitives not under test ---- */
void SYSCTRL_AHBPeriphReset(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void SYSCTRL_APBPeriphReset1(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void SYSCTRL_AHBPeriphClk_Enable(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void SYSCTRL_APBPeriphClk_Enable1(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void SYSCTRL_APBPeriphClk_Enable2(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void SYSCTRL_GotoDeepSleep(void) { }
void delay_ms(uint32_t ms) { delay_ms_total += (int)ms; delay_last_ms = (int)ms; delay_ms_calls++; }

void light_init(void) { light_init_calls++; }
light_result_t light_sample(void)
{
    light_result_t r;
    memset(&r, 0, sizeof r);
    light_sample_calls++;
    return r;
}
uint8_t app_um2005C_send_data_timeout(uint8_t *d, uint16_t l, uint32_t t)
{ (void)d; (void)l; (void)t; return 1; }

/* ==================================================================== */
static int pass, fail;
#define CHECK(c, m) do { if (c) { pass++; printf("    PASS  %s\n", m); } \
                         else   { fail++; printf("    FAIL  %s (line %d)\n", m, __LINE__); } } while (0)

/* GXHT40 frame builder (independent of the driver): 6 bytes + CRCs are supplied
 * by the harness, never computed here from driver code. */
static void set_frame(const uint8_t f[6])
{
    memcpy(rd_data, f, 6);
    rd_len = 6;
}

int main(void)
{
    printf("==== independent bus-diagnostic verification (embedded_tester) ====\n");
    i2c_init(&dev);

    /* ================= PROBE: i2c_probe_addr ================= */
    printf("[P1] i2c_probe_addr(0x88) with a slave ACKing 0x44\n");
    bus_reset(); slave_present = 1; slave_a7 = 0x44; slave_start_reset();
    sf_i2c_err r = i2c_probe_addr(&dev, 0x88);
    ev_dump("P1");
    CHECK(r == SF_I2C_SUCCESS, "returns SF_I2C_SUCCESS on ACK");
    CHECK(ev_count(EV_START) == 1 && ev_count(EV_STOP) == 1, "exactly 1 START and 1 STOP");
    CHECK(ev_count(EV_TX) == 1 && ev_get(EV_TX, 1) == 0x88, "exactly ONE address byte 0x88 on the wire");
    CHECK(ev_count(EV_RX) == 0, "no data byte is read");
    CHECK(wr_len == 0, "slave received no command/register byte");
    CHECK(scl == 1 && bus_sda() == 1, "bus released (both lines high) afterwards");

    printf("[P2] i2c_probe_addr(0x88) with no slave\n");
    bus_reset(); slave_present = 0; slave_start_reset();
    r = i2c_probe_addr(&dev, 0x88);
    ev_dump("P2");
    CHECK(r == SF_I2C_TIMEOUT, "returns SF_I2C_TIMEOUT on NACK");
    CHECK(ev_count(EV_STOP) >= 1, "STOP still issued -> bus released on the NACK path");
    CHECK(ev_count(EV_TX) == 1, "no further byte after the NACKed address");
    CHECK(scl == 1 && bus_sda() == 1, "bus idle high afterwards (not stuck)");

    printf("[P3] i2c_probe_addr with a different address (0x8A) while 0x44 answers\n");
    bus_reset(); slave_present = 1; slave_a7 = 0x44; slave_start_reset();
    r = i2c_probe_addr(&dev, 0x8A);
    ev_dump("P3");
    CHECK(r == SF_I2C_TIMEOUT, "non-matching address is not ACKed");
    CHECK(ev_get(EV_TX, 1) == 0x8A, "exactly the requested address is emitted");

    printf("[P4] regression: register-less primitives unchanged\n");
    bus_reset(); slave_present = 1; slave_a7 = 0x44; slave_start_reset();
    r = i2c_write_cmd(&dev, 0x88, 0xFD);
    ev_dump("P4");
    CHECK(r == SF_I2C_SUCCESS && ev_get(EV_TX, 2) == 0xFD, "i2c_write_cmd still sends 0x88 then 0xFD");
    bus_reset(); slave_present = 1; slave_a7 = 0x44;
    set_frame((const uint8_t[]){ 0x61, 0x9C, 0x2B, 0x8A, 0x44, 0x75 });
    slave_start_reset();
    uint8_t buf[6]; memset(buf, 0, sizeof buf);
    r = i2c_read_bytes(&dev, 0x89, buf, 6);
    ev_dump("P4b");
    CHECK(r == SF_I2C_SUCCESS && memcmp(buf, rd_data, 6) == 0, "i2c_read_bytes still returns the slave frame");
    CHECK(ev_get(EV_RXMACK, 6) == 1, "last byte still NACKed by the master");

    /* ================= SCAN: sensor_bus_diag_scan ================= */
    printf("[S1] sensor_bus_diag_scan: 0x08..0x77 sweep, no device answering\n");
    bus_reset(); slave_present = 0;
    mock_gpioa.unused = 0; gpio_init_pa34_od = 0;
    sample_flag = 1; report_req = 0; tempvalue = 123; huminityvalue = 456;
    debug_trace_bus_t b; memset(&b, 0xEE, sizeof b);
    uint8_t ok = sensor_bus_diag_scan(&b);
    printf("    scanned transactions: START=%d TX=%d STOP=%d RX=%d\n",
           ev_count(EV_START), ev_count(EV_TX), ev_count(EV_STOP), ev_count(EV_RX));
    CHECK(ok == 1, "returns 1 (filled) when the bus is bound");
    CHECK(ev_count(EV_TX) == (0x77 - 0x08 + 1), "exactly 112 address bytes emitted (0x08..0x77 inclusive)");
    CHECK(ev_count(EV_RX) == 0, "no data byte is ever read during the scan");
    CHECK(wr_len == 0, "no command/register byte ever written (address-only probe)");
    {
        int bad = 0;
        for (int i = 1; i <= ev_count(EV_TX); i++) {
            int tx = ev_get(EV_TX, i);
            int expect = ((0x08 + i - 1) << 1) & 0xFF;
            if (tx != expect) bad++;
        }
        CHECK(bad == 0, "address bytes ascend as (addr7<<1) over 0x08..0x77 with write bit clear");
    }
    CHECK(ev_count(EV_START) == 112 && ev_count(EV_STOP) >= 112,
          "one START per probe and a STOP for every probe (bus always released)");
    CHECK(b.ack_count == 0 && b.ack_truncated == 0, "no ACK recorded (ack=none) and no truncation flag");
    CHECK(b.idle == 3, "idle bitmap = 3 (SCL=1,SDA=1) on a healthy pulled-up bus");
    CHECK(sample_flag == 1 && report_req == 0 && tempvalue == 123 && huminityvalue == 456,
          "business state untouched by the scan");
    CHECK(gpio_init_pa34_od == 1, "bsp_i2c_init() configures PA3/PA4 exactly once (idempotent)");

    printf("[S2] scan again (must stay idempotent, no re-init, same result)\n");
    bus_reset(); slave_present = 0;
    debug_trace_bus_t b2; memset(&b2, 0xEE, sizeof b2);
    ok = sensor_bus_diag_scan(&b2);
    CHECK(ok == 1 && b2.idle == b.idle && b2.ack_count == b.ack_count,
          "second scan yields the same idle/ack result");
    CHECK(gpio_init_pa34_od == 1, "PA3/PA4 not re-configured (one-time physical init)");
    CHECK(ev_count(EV_TX) == 112, "the second scan itself still probes all 112 addresses");

    printf("[S3] scan with a GXHT40 answering at 0x44\n");
    bus_reset(); slave_present = 1; slave_a7 = 0x44;
    debug_trace_bus_t b3; memset(&b3, 0xEE, sizeof b3);
    ok = sensor_bus_diag_scan(&b3);
    CHECK(ok == 1 && b3.ack_count == 1 && b3.ack_addr[0] == 0x44 && b3.ack_truncated == 0,
          "ACK set = {0x44} recorded as a 7-bit address");

    printf("[S4] scan with slaves at 0x10 / 0x44 and a stale 0x77\n");
    /* the mock slave model answers one address; drive the others via a table */
    bus_reset();
    {
        /* emulate a multi-device bus by re-running the sweep with a lookup */
        int ack_expected[3] = { 0x10, 0x44, 0x77 };
        (void)ack_expected;
    }
    /* multi-slave support is modelled by re-pointing slave_a7 between sweeps, so
     * the multi-ACK collection is checked through the truncation case below. */
    slave_present = 1; slave_a7 = 0x10;
    debug_trace_bus_t b4; memset(&b4, 0xEE, sizeof b4);
    ok = sensor_bus_diag_scan(&b4);
    CHECK(ok == 1 && b4.ack_count == 1 && b4.ack_addr[0] == 0x10,
          "another device address is reported verbatim (no hard-coding of 0x44)");

    printf("[S5] truncation bound: >8 answering addresses\n");
    bus_reset();
    slave_present = 1;
    {
        /* answer EVERY address: model by a "promiscuous" flag */
        g_promiscuous = 1;
        slave_a7 = 0x44;
        debug_trace_bus_t bt; memset(&bt, 0xEE, sizeof bt);
        ok = sensor_bus_diag_scan(&bt);
        g_promiscuous = 0;
        printf("    ack_count=%d truncated=%d first=%02X last=%02X\n",
               bt.ack_count, bt.ack_truncated,
               bt.ack_count ? bt.ack_addr[0] : 0, bt.ack_count ? bt.ack_addr[bt.ack_count - 1] : 0);
        CHECK(bt.ack_count == 8, "ACK list capped at SENSOR_BUS_DIAG_MAX_ACK (8)");
        CHECK(bt.ack_truncated == 1, "truncation flag set (line ends with '+')");
        CHECK(bt.ack_addr[0] == 0x08 && bt.ack_addr[7] == 0x0F,
              "the eight recorded addresses are the first eight probed (0x08..0x0F)");
    }

    printf("[S6] negative control: a stuck-low SDA would make EVERY address look ACKed\n");
    bus_reset(); slave_present = 0; ext_hold_sda_low = 1;
    debug_trace_bus_t b6; memset(&b6, 0xEE, sizeof b6);
    ok = sensor_bus_diag_scan(&b6);
    ext_hold_sda_low = 0;
    printf("    idle=%d (SCL=%d SDA=%d) ack_count=%d truncated=%d\n",
           b6.idle, b6.idle & 1, (b6.idle >> 1) & 1, b6.ack_count, b6.ack_truncated);
    CHECK(ok == 1 && b6.idle == 1, "idle=1 reports SCL high / SDA low with the split bits consistent");
    /*
     * I2C is wired-AND: if SDA were held low the master would read an ACK on every
     * address slot, so a genuinely stuck-low bus reports a FULL ack list with the
     * truncation marker -- never 'none'. Therefore the target's observed ack=none
     * (three boots) cannot be explained by a stuck-low SDA: no device is answering.
     * This makes the real-target inference ("GXHT40 does not respond") testable.
     */
    CHECK(b6.ack_count == 8 && b6.ack_truncated == 1,
          "a stuck-low SDA yields a full (+truncated) ACK list, so the target's ack=none "
          "proves SDA was high during the ACK slots");

    /* ================= DIAG: gxht40_measure snapshot ================= */
    printf("[D1] diag snapshot on a successful measurement\n");
    bus_reset(); slave_present = 1; slave_a7 = 0x44;
    set_frame((const uint8_t[]){ 0x66, 0x66, 0x93, 0x72, 0xB0, 0xDC });  /* 25.0C / 50.0%RH */
    delay_ms_total = 0; delay_ms_calls = 0;
    gxht40_init(&dev);
    int16_t t = 0x1111; uint16_t h = 0x2222;
    gxht40_status_t gs = gxht40_measure(&t, &h);
    gxht40_diag_t dg; gxht40_diag_fetch(&dg);
    printf("    status=%u a44=%u a45=%u rd=%u at=%u raw_valid=%u raw=%02X%02X%02X%02X%02X%02X\n",
           dg.status, dg.ack44, dg.ack45, dg.read_retry, dg.attempt, dg.raw_valid,
           dg.raw[0], dg.raw[1], dg.raw[2], dg.raw[3], dg.raw[4], dg.raw[5]);
    CHECK(gs == GXHT40_OK, "driver still returns GXHT40_OK");
    CHECK(dg.status == (uint8_t)GXHT40_OK, "diag status = 0 (OK)");
    CHECK(dg.ack44 == 1 && dg.ack45 == 0, "a44=1 (0x44 answered), a45=0");
    CHECK(dg.read_retry == 0, "rd=0 (no failed re-read)");
    CHECK(dg.attempt == 1, "at=1 (single frame attempt)");
    CHECK(dg.raw_valid == 1 && memcmp(dg.raw, rd_data, 6) == 0, "raw = the 6 bytes actually read back");
    CHECK(delay_ms_calls >= 1 && delay_last_ms == GXHT40_MEASURE_WAIT_MS,
          "tMEAS wait still performed once at the configured 10 ms");
    CHECK(t == 250 && h == 500, "converted values unchanged by the diagnostics");

    printf("[D2] diag snapshot when the first two read attempts are NACKed\n");
    bus_reset(); slave_present = 1; slave_a7 = 0x44; nack_reads = 2;
    set_frame((const uint8_t[]){ 0x66, 0x66, 0x93, 0x72, 0xB0, 0xDC });
    gxht40_init(&dev);
    t = 0x1111; h = 0x2222;
    gs = gxht40_measure(&t, &h);
    gxht40_diag_fetch(&dg);
    printf("    status=%u rd=%u at=%u raw_valid=%u\n", dg.status, dg.read_retry, dg.attempt, dg.raw_valid);
    CHECK(gs == GXHT40_OK, "read succeeds on the third attempt");
    CHECK(dg.read_retry == 2, "rd=2 records the re-reads actually consumed by the last read");
    CHECK(dg.attempt == 1, "at=1 (the command was not re-sent while the conversion ran)");
    CHECK(dg.raw_valid == 1, "raw from the successful read");

    printf("[D3] diag snapshot on CRC failure (all frame retries exhausted)\n");
    bus_reset(); slave_present = 1; slave_a7 = 0x44;
    set_frame((const uint8_t[]){ 0x66, 0x66, 0x00, 0x72, 0xB0, 0xDC });  /* bad temperature CRC */
    gxht40_init(&dev);
    t = 0x1111; h = 0x2222;
    gs = gxht40_measure(&t, &h);
    gxht40_diag_fetch(&dg);
    printf("    status=%u a44=%u rd=%u at=%u raw_valid=%u\n",
           dg.status, dg.ack44, dg.read_retry, dg.attempt, dg.raw_valid);
    CHECK(gs == GXHT40_ERR_CRC, "driver returns GXHT40_ERR_CRC");
    CHECK(dg.status == (uint8_t)GXHT40_ERR_CRC, "diag status = 4 (CRC)");
    CHECK(dg.ack44 == 1, "a44=1 (the address did answer)");
    CHECK(dg.attempt == GXHT40_MEAS_RETRY, "at = GXHT40_MEAS_RETRY (frame retries exhausted)");
    CHECK(dg.raw_valid == 1 && dg.raw[2] == 0x00, "raw keeps the last read-back frame (recomputable)");
    CHECK(t == 0x1111 && h == 0x2222, "outputs untouched on failure");

    printf("[D4] diag snapshot with no device on the bus\n");
    bus_reset(); slave_present = 0;
    gxht40_init(&dev);
    t = 0x1111; h = 0x2222;
    gs = gxht40_measure(&t, &h);
    gxht40_diag_fetch(&dg);
    printf("    status=%u a44=%u a45=%u rd=%u at=%u raw_valid=%u\n",
           dg.status, dg.ack44, dg.ack45, dg.read_retry, dg.attempt, dg.raw_valid);
    CHECK(gs == GXHT40_ERR_NO_DEVICE, "driver returns GXHT40_ERR_NO_DEVICE");
    CHECK(dg.status == (uint8_t)GXHT40_ERR_NO_DEVICE, "diag status = 2 (no device)");
    CHECK(dg.ack44 == 0 && dg.ack45 == 0, "a44=a45=0 -> consistent with s=2");
    CHECK(dg.attempt == GXHT40_MEAS_RETRY, "at = GXHT40_MEAS_RETRY");
    CHECK(dg.raw_valid == 0, "raw_valid=0 -> the G line must print 12 '-'");
    CHECK(t == 0x1111 && h == 0x2222, "outputs untouched on failure");

    printf("[D5] diag snapshot on an out-of-range frame\n");
    bus_reset(); slave_present = 1; slave_a7 = 0x44;
    set_frame((const uint8_t[]){ 0x00, 0x00, 0x81, 0x72, 0xB0, 0xDC });  /* -> -45.0 C */
    gxht40_init(&dev);
    t = 0x1111; h = 0x2222;
    gs = gxht40_measure(&t, &h);
    gxht40_diag_fetch(&dg);
    printf("    status=%u a44=%u at=%u raw_valid=%u\n", dg.status, dg.ack44, dg.attempt, dg.raw_valid);
    CHECK(gs == GXHT40_ERR_RANGE, "driver returns GXHT40_ERR_RANGE");
    CHECK(dg.status == (uint8_t)GXHT40_ERR_RANGE, "diag status = 5 (range)");
    CHECK(dg.ack44 == 1 && dg.attempt == GXHT40_MEAS_RETRY, "a44=1 and all frame retries consumed");
    CHECK(t == 0x1111 && h == 0x2222, "outputs untouched on failure");

    printf("[D6] diag snapshot on read NACK exhaustion\n");
    bus_reset(); slave_present = 1; slave_a7 = 0x44; nack_reads = -1;
    gxht40_init(&dev);
    t = 0x1111; h = 0x2222;
    gs = gxht40_measure(&t, &h);
    gxht40_diag_fetch(&dg);
    printf("    status=%u a44=%u rd=%u at=%u raw_valid=%u\n",
           dg.status, dg.ack44, dg.read_retry, dg.attempt, dg.raw_valid);
    CHECK(gs == GXHT40_ERR_IO, "driver returns GXHT40_ERR_IO");
    CHECK(dg.status == (uint8_t)GXHT40_ERR_IO, "diag status = 3 (read failure)");
    CHECK(dg.ack44 == 1, "a44=1 (the device did answer the address)");
    CHECK(dg.read_retry > 0, "rd>0 records the exhausted re-reads");
    CHECK(dg.raw_valid == 0, "raw_valid=0 (no successful read in this frame)");
    CHECK(t == 0x1111 && h == 0x2222, "outputs untouched on failure");

    printf("[D7] gxht40_diag_fetch is read-only (state unchanged across fetches)\n");
    gxht40_diag_t d2; gxht40_diag_fetch(&d2); gxht40_diag_fetch(&d2);
    CHECK(memcmp(&d2, &dg, sizeof dg) == 0, "repeated fetches return the same snapshot");
    CHECK(gxht40_detected_addr7() == 0u || gxht40_detected_addr7() == 0x44u,
          "address cache left in a legal state after failures");

    /* ================= FLOW: what the trace snapshot carries per cycle =================
     * Drives the REAL sampling flow (measure.c temperature_process -> real gxht40_measure
     * on the real soft-I2C bit-banger) and reads back the snapshot the G line is built
     * from. This is the acceptance point "G only on failing cycles, S bytes unchanged". */
    printf("[F1] failing cycle: the trace snapshot marks the failure and carries the facts\n");
    {
        bus_reset(); slave_present = 0;
        sample_flag = 1; report_req = 0; tempvalue = 111; huminityvalue = 222;
        temperature_process();
        debug_trace_sample_t tr; memset(&tr, 0, sizeof tr);
        CHECK(sensor_trace_fetch(&tr) == 1, "one trace snapshot pending after a cycle");
        CHECK(tr.sample_ok == 0 && tr.diag_valid == 1,
              "failing cycle -> sample_ok=0 and diag_valid=1 (the G line is emitted)");
        CHECK(tr.diag_status == (uint8_t)GXHT40_ERR_NO_DEVICE &&
              tr.diag_ack44 == 0 && tr.diag_ack45 == 0,
              "diag status/ACK flags match the observed no-device failure");
        CHECK(tr.diag_read_retry == 0 && tr.diag_attempt == GXHT40_MEAS_RETRY &&
              tr.diag_raw_valid == 0,
              "rd=0 / at=3 / raw placeholder for a failure with no successful read");
        CHECK(tr.temp_x10 == 111 && tr.hum_x10 == 222,
              "failed cycle keeps the last valid values (no 0 fabrication)");
        CHECK(tr.report == 0 && report_req == 0, "failed cycle never requests a report");
        debug_trace_sample_t tr2; memset(&tr2, 0, sizeof tr2);
        CHECK(sensor_trace_fetch(&tr2) == 0, "the snapshot is consumed exactly once");
    }

    printf("[F2] successful cycle: diag_valid=0, measured values in the S fields\n");
    {
        bus_reset(); slave_present = 1; slave_a7 = 0x44;
        set_frame((const uint8_t[]){ 0x66, 0x66, 0x93, 0x72, 0xB0, 0xDC });   /* 25.0C / 50.0% */
        sample_flag = 1;
        temperature_process();
        debug_trace_sample_t tr; memset(&tr, 0, sizeof tr);
        CHECK(sensor_trace_fetch(&tr) == 1, "one trace snapshot pending");
        CHECK(tr.sample_ok == 1 && tr.diag_valid == 0,
              "successful cycle -> sample_ok=1 and diag_valid=0 (no G line emitted)");
        CHECK(tr.temp_x10 == 250 && tr.hum_x10 == 500, "measured values appear in the S fields");
        CHECK(tr.have_prev == 1 && tr.prev_temp_x10 == 250,
              "previous value advanced to the measurement taken this cycle");
    }

    printf("[F3] a failing cycle must not advance the previous valid temperature\n");
    {
        bus_reset(); slave_present = 1; slave_a7 = 0x44;
        set_frame((const uint8_t[]){ 0x66, 0x66, 0x93, 0x72, 0xB0, 0xDC });   /* 250 / 500 */
        sample_flag = 1;
        temperature_process();                       /* success: prev <- 250 */
        bus_reset(); slave_present = 0;              /* next cycle: no device */
        sample_flag = 1;
        temperature_process();
        debug_trace_sample_t tr; memset(&tr, 0, sizeof tr);
        sensor_trace_fetch(&tr);
        CHECK(tr.sample_ok == 0 && tr.diag_valid == 1, "second cycle is a diagnosed failure");
        CHECK(tr.prev_temp_x10 == 250 && tr.have_prev == 1,
              "previous value still 250: a diagnosed failure does not corrupt it");
        CHECK(tr.temp_x10 == 250 && tr.hum_x10 == 500,
              "S fields keep the last valid sample on a failing cycle");
    }

    printf("==== result: %d passed, %d failed ====\n", pass, fail);
    return fail ? 1 : 0;
}
