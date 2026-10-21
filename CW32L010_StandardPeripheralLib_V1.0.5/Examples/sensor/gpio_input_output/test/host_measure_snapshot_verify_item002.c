/*
 * host_measure_snapshot_verify_item002.c
 *
 * ITEM-002 independent verification harness (embedded_tester.embedded_verification).
 *
 * Compiles the REAL product source USER/src/measure.c against a shadow MCU layer
 * (test/mock_measure_ev/cw32l010_*.h -> mock_measure_hw.h) and scripted sensor
 * primitives, so the sampling-transaction logic runs unmodified on the host.
 *
 * What is checked (raw observation only; semantic judgement lives in the
 * independent Python reference model verify_measure_snapshot_item002.py):
 *   - per-cycle S-trace snapshot p/H/t/h/q/r/s from sensor_trace_fetch()
 *   - tempvalue/huminityvalue after the cycle
 *   - report_req after temperature_process()
 *   - whether send_data_to_gateway() invoked the RF stub in that cycle
 *   - report_req without a pending frozen frame performs no RF attempt
 *     (current ITEM-003 freeze contract; see the NO_PENDING_SENDS probe)
 *
 * Build (from gpio_input_output/):
 *   gcc -std=c11 -Wall -Wextra -I test/mock_measure_ev -I USER/inc -I UM2005C -I COMMON \
 *       -DVERIFY_LABEL='"current"' \
 *       test/host_measure_snapshot_verify_item002.c USER/src/measure.c \
 *       USER/src/fw_core.c USER/src/sf_i2c.c USER/src/encrytogate.c \
 *       -o <out.exe>
 *
 * The same harness compiled against the pre-ITEM-002 measure.c (exported read-only
 * from Git) is the negative control: it must FAIL the p-field expectations on
 * successful cycles (it prints the advanced value instead of the pre-decision one).
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "measure.h"
#include "sensor_config.h"
#include "gxht40.h"
#include "light.h"
#include "encrytogate.h"
#include "mock_measure_hw.h"

#ifndef VERIFY_LABEL
#define VERIFY_LABEL "current"
#endif

/* globals owned by measure.c */
extern int16_t  tempvalue;
extern uint16_t huminityvalue;
extern uint8_t  sample_flag;
extern uint8_t  report_req;
extern i2c_dev *temp_ptr;
extern uint8_t  mcu_uid[10];

/* ==================== shadow MCU layer ==================== */
GPIO_TypeDef mock_gpioa, mock_gpiob;
static int gpio_writes;

void GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *init) { (void)port; (void)init; }
void GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState st) { (void)port; (void)pin; (void)st; gpio_writes++; }
GPIO_PinState GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin) { (void)port; (void)pin; return GPIO_Pin_SET; }
void SYSCTRL_AHBPeriphReset(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void SYSCTRL_APBPeriphReset1(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void SYSCTRL_AHBPeriphClk_Enable(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void SYSCTRL_APBPeriphClk_Enable1(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void SYSCTRL_GotoDeepSleep(void) { }
void UART_Init(uint32_t u, UART_InitTypeDef *c) { (void)u; (void)c; }
void delay_ms(uint16_t ms) { (void)ms; }   /* flow logic under test, not wall time */

/* ==================== scripted stimulus ==================== */
typedef struct {
    gxht40_status_t st;   /* GXHT40_OK => valid sample */
    int16_t         t;    /* stimulus temperature x10 (written only on OK) */
    uint16_t        h;    /* stimulus humidity x10 (written only on OK) */
    uint8_t         dark; /* light_result_t.dark */
    uint8_t         lv;   /* light_result_t.valid */
} stim_t;

enum { N_CYCLES = 16 };

static const stim_t STIM[N_CYCLES] = {
    /* 0 */ { GXHT40_OK,           250, 500, 1, 1 },
    /* 1 */ { GXHT40_OK,           260, 510, 1, 1 },
    /* 2 */ { GXHT40_ERR_CRC,      999, 999, 1, 1 },   /* failure: sentinels must not be used */
    /* 3 */ { GXHT40_OK,           270, 520, 1, 1 },
    /* 4 */ { GXHT40_OK,           280, 530, 0, 0 },   /* lit / light invalid: no rise branch */
    /* 5 */ { GXHT40_ERR_IO,       999, 999, 1, 1 },   /* failure */
    /* 6 */ { GXHT40_OK,           290, 540, 1, 1 },
    /* 7 */ { GXHT40_OK,           351, 550, 0, 0 },   /* > 35.0 C, independent of light */
    /* 8 */ { GXHT40_ERR_CRC,      999, 999, 1, 1 },   /* failure */
    /* 9 */ { GXHT40_OK,             0,   0, 1, 1 },   /* legal zero sample, no rise */
    /* 10*/ { GXHT40_OK,           -10, 600, 1, 1 },   /* negative temperature */
    /* 11*/ { GXHT40_ERR_RANGE,    999, 999, 1, 1 },   /* failure */
    /* 12*/ { GXHT40_OK,             5, 610, 1, 1 },   /* rise from -10 => +15 */
    /* 13*/ { GXHT40_ERR_NO_DEVICE,999, 999, 1, 1 },   /* failure */
    /* 14*/ { GXHT40_OK,            15, 620, 1, 1 },
    /* 15*/ { GXHT40_ERR_IO,       999, 999, 1, 1 }    /* failure */
};

static int      cyc;
static int      light_calls, gxht_calls, gxht_init_calls, light_init_calls;
static int      send_calls;
static uint8_t  send_ret;
static uint8_t  last_frame[10];

void light_init(void) { light_init_calls++; }

light_result_t light_sample(void)
{
    light_result_t r;
    memset(&r, 0, sizeof(r));
    r.adc_ok      = true;
    r.samples_ok  = 8u;
    r.mean_adc_code = STIM[cyc].dark ? 3000u : 500u;
    r.code_min    = STIM[cyc].dark ? 2900u : 480u;
    r.code_max    = STIM[cyc].dark ? 3100u : 520u;
    r.valid       = STIM[cyc].lv ? true : false;
    r.dark        = STIM[cyc].dark ? true : false;
    light_calls++;
    return r;
}

void gxht40_init(i2c_dev *dev) { (void)dev; gxht_init_calls++; }

gxht40_status_t gxht40_measure(int16_t *t, uint16_t *h)
{
    gxht40_status_t st = STIM[cyc].st;
    gxht_calls++;
    if (st == GXHT40_OK) {          /* driver contract: outputs written only on success */
        if (t != NULL) *t = STIM[cyc].t;
        if (h != NULL) *h = STIM[cyc].h;
    }
    return st;
}

uint8_t gxht40_detected_addr7(void) { return 0x44u; }

void gxht40_diag_fetch(gxht40_diag_t *out)
{
    uint8_t i;
    if (out == NULL) return;
    memset(out, 0, sizeof(*out));
    out->status     = (uint8_t)STIM[cyc].st;
    out->ack44      = (STIM[cyc].st == GXHT40_ERR_NO_DEVICE) ? 0u : 1u;
    out->ack45      = 0u;
    out->read_retry = 2u;
    out->attempt    = 3u;
    out->raw_valid  = 0u;
    for (i = 0u; i < (uint8_t)GXHT40_RESULT_LEN; i++) out->raw[i] = (uint8_t)(0xB0u + i);
}

uint8_t app_um2005C_send_data_timeout(uint8_t *d, uint16_t n, uint32_t to)
{
    (void)to;
    send_calls++;
    memset(last_frame, 0, sizeof(last_frame));
    if (d != NULL && n >= 10u) memcpy(last_frame, d, 10u);
    return send_ret;
}

/* ==================== driver ==================== */
int main(void)
{
    debug_trace_sample_t tr;
    int i;

    memset(mcu_uid, 0, 10);
    mcu_uid[0] = 0x6A; mcu_uid[1] = 0x00; mcu_uid[2] = 0x2C; mcu_uid[3] = 0x00;
    for (i = 4; i < 10; i++) mcu_uid[i] = (uint8_t)(0x40 + i);
    send_ret = 1u;

    printf("RUN %s cycles=%d\n", VERIFY_LABEL, N_CYCLES);

    for (cyc = 0; cyc < N_CYCLES; cyc++) {
        int     sends_before = send_calls;
        uint8_t fetched;
        uint8_t req_before, req_after;
        int     sends;

        sample_flag = 1u;
        req_before  = report_req;
        temperature_process();
        req_after   = report_req;

        send_data_to_gateway();
        sends = send_calls - sends_before;

        memset(&tr, 0xEE, sizeof(tr));
        fetched = sensor_trace_fetch(&tr);

        printf("C idx=%d st=%d stimt=%d stimh=%u dark=%u lv=%u | p=%d H=%u t=%d h=%u q=%u r=%u s=%u diag=%u | reqb=%u req=%u sends=%d tv=%d hv=%u fetched=%u\n",
               cyc, (int)STIM[cyc].st, (int)STIM[cyc].t, (unsigned)STIM[cyc].h,
               (unsigned)STIM[cyc].dark, (unsigned)STIM[cyc].lv,
               (int)tr.prev_temp_x10, (unsigned)tr.have_prev,
               (int)tr.temp_x10, (unsigned)tr.hum_x10,
               (unsigned)tr.sample_ok, (unsigned)tr.report, (unsigned)tr.send,
               (unsigned)tr.diag_valid,
               (unsigned)req_before, (unsigned)req_after, sends,
               (int)tempvalue, (unsigned)huminityvalue, (unsigned)fetched);

        /* Current contract (FDS-003 DD-03/DD-04, T-L6-04): the 10-byte report frame is
         * frozen exactly once at trigger time inside temperature_process(); there is no
         * re-encode on retry and nothing survives into the next cycle. Therefore a bare
         * report_req without a pending frozen frame MUST NOT perform any RF attempt
         * (measure.c send_data_to_gateway(): s_pending_frame_valid==0 -> clear, no send).
         * The old "force report_req then send pre-failure values" stimulus exercised the
         * superseded ITEM-002 retry model; the "failed cycle did not advance prev"
         * invariant is asserted by the p/H fields of the following cycles above, and
         * frame freeze byte-identity is covered by host_measure_freeze_verify_item003.c. */
        if (cyc == 5 || cyc == 11) {
            int sends_before_probe = send_calls;
            report_req = 1u;                     /* spurious external pending flag */
            send_data_to_gateway();              /* no frozen frame -> no RF attempt */
            printf("NO_PENDING_SENDS idx=%d delta=%d\n", cyc, send_calls - sends_before_probe);
            report_req = 0u;
        }
    }

    /* second fetch in the same cycle must report "nothing pending" and not touch *out */
    {
        debug_trace_sample_t twice;
        uint8_t again;
        memset(&twice, 0x5A, sizeof(twice));
        again = sensor_trace_fetch(&twice);
        printf("FETCH_AGAIN ret=%u first_byte_dec=%u\n", (unsigned)again, (unsigned)((uint8_t *)&twice)[0]);
    }

    /* sample_flag gate: no sampling / no trace when the flag is clear */
    {
        int lc = light_calls, gc = gxht_calls;
        debug_trace_sample_t gate;
        sample_flag = 0u;
        temperature_process();
        memset(&gate, 0x77, sizeof(gate));
        printf("GATE ldelta=%d gdelta=%d fetch=%u\n",
               light_calls - lc, gxht_calls - gc, (unsigned)sensor_trace_fetch(&gate));
        if (gate.prev_temp_x10 != (int16_t)0x7777 || gate.have_prev != 0x77u) {
            printf("GATE_TOUCHED_OUT=1\n");
        } else {
            printf("GATE_TOUCHED_OUT=0\n");
        }
    }

    printf("COUNT light_calls=%d gxht_calls=%d gxht_init=%d light_init=%d send_calls=%d gpio_writes=%d\n",
           light_calls, gxht_calls, gxht_init_calls, light_init_calls, send_calls, gpio_writes);
    printf("TEMP_PTR_NULL=%d\n", temp_ptr == NULL ? 1 : 0);
    return 0;
}
