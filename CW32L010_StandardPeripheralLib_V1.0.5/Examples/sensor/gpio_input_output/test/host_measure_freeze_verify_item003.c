/*
 * host_measure_freeze_verify_item003.c
 *
 * ITEM-003 independent verification harness (embedded_tester.embedded_verification).
 *
 * Compiles the REAL product source USER/src/measure.c (plus fw_core.c, sf_i2c.c,
 * encrytogate.c) against the shadow MCU layer (test/mock_measure_ev/), so the
 * trigger/freeze/send-transaction logic runs unmodified on the host.
 *
 * Scope under test (ITEM-003):
 *   - at trigger time encode_frame10() is called EXACTLY ONCE and the 10-byte
 *     buffer is frozen;
 *   - one send_data_to_gateway() call performs at most 3 attempts
 *     (SENSOR_RF_TX_RETRY), each 200 ms (SENSOR_RF_TX_TIMEOUT_MS), len 10,
 *     all attempts passing the SAME bytes;
 *   - success stops the loop; exhaustion stops it after 3; both clear the
 *     pending state; nothing survives into the next cycle;
 *   - the S-trace (measure.c snapshot) reports the FINAL local result s=0/1/2
 *     and y (retry counter) stays 0.
 *
 * encode_frame10 call counting uses GNU ld --wrap, so the real product encoder
 * is still the one that produces the bytes.
 *
 * Build (from gpio_input_output/):
 *   gcc -std=c11 -Wall -Wextra -I test/mock_measure_ev -I USER/inc -I UM2005C -I COMMON \
 *       -DVERIFY_LABEL='"current"' -Wl,--wrap=encode_frame10 \
 *       test/host_measure_freeze_verify_item003.c USER/src/measure.c \
 *       USER/src/fw_core.c USER/src/sf_i2c.c USER/src/encrytogate.c \
 *       -o <out.exe>
 *
 * The same harness compiled against the pre-ITEM-003 measure.c (exported
 * read-only from Git) is the negative control: it must be rejected on the
 * attempt-count / freeze / clear expectations.
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

void GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *init) { (void)port; (void)init; }
void GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState st) { (void)port; (void)pin; (void)st; }
GPIO_PinState GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin) { (void)port; (void)pin; return GPIO_Pin_SET; }
void SYSCTRL_AHBPeriphReset(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void SYSCTRL_APBPeriphReset1(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void SYSCTRL_AHBPeriphClk_Enable(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void SYSCTRL_APBPeriphClk_Enable1(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void SYSCTRL_GotoDeepSleep(void) { }
void UART_Init(uint32_t u, UART_InitTypeDef *c) { (void)u; (void)c; }
void delay_ms(uint16_t ms) { (void)ms; }

/* ==================== real encoder, counted via --wrap ==================== */
void __real_encode_frame10(uint8_t uid10[10], int16_t temp, uint16_t hum, uint8_t out10[10]);
static int encode_calls;

void __wrap_encode_frame10(uint8_t uid10[10], int16_t temp, uint16_t hum, uint8_t out10[10])
{
    encode_calls++;
    __real_encode_frame10(uid10, temp, hum, out10);
}

/* ==================== scripted stimulus ==================== */
typedef struct {
    gxht40_status_t st;    /* GXHT40_OK => valid sample */
    int16_t         t;     /* stimulus temperature x10 (written only on OK) */
    uint16_t        h;     /* stimulus humidity x10 (written only on OK) */
    uint8_t         dark;  /* light_result_t.dark */
    uint8_t         lv;    /* light_result_t.valid */
    uint8_t         seq[3];/* RF stub return per attempt: 0 = fail, 1 = success */
    uint8_t         mutate;/* 1 = change tempvalue/huminityvalue during attempt 1 */
} stim_t;

#define MAX_ATTEMPTS 3
enum { N_CYCLES = 16 };

static const stim_t STIM[N_CYCLES] = {
    /* 0 */ { GXHT40_OK,           250, 500, 1, 1, {1,1,1}, 0 }, /* first sample: no prev */
    /* 1 */ { GXHT40_OK,           260, 510, 1, 1, {1,0,0}, 0 }, /* rise 10 -> trigger, success 1st */
    /* 2 */ { GXHT40_OK,           270, 520, 1, 1, {0,0,0}, 0 }, /* trigger, all 3 attempts fail */
    /* 3 */ { GXHT40_OK,           280, 530, 1, 1, {0,0,1}, 0 }, /* trigger, fail/fail/success */
    /* 4 */ { GXHT40_OK,           280, 530, 1, 1, {1,1,1}, 0 }, /* no rise -> no trigger */
    /* 5 */ { GXHT40_OK,           290, 540, 1, 1, {0,0,1}, 1 }, /* mutation during retry */
    /* 6 */ { GXHT40_OK,           351, 550, 0, 0, {1,0,0}, 0 }, /* >35.0 C, light irrelevant */
    /* 7 */ { GXHT40_ERR_CRC,      999, 999, 1, 1, {1,1,1}, 0 }, /* failed cycle: no trigger */
    /* 8 */ { GXHT40_OK,            15, 620, 1, 1, {1,1,1}, 0 }, /* drop from 351 -> no trigger */
    /* 9 */ { GXHT40_OK,            25, 630, 1, 1, {0,1,0}, 0 }, /* trigger, success 2nd */
    /*10 */ { GXHT40_OK,            35, 640, 1, 1, {0,0,0}, 0 }, /* trigger, exhausted */
    /*11 */ { GXHT40_OK,            45, 650, 1, 1, {1,0,0}, 0 }, /* new trigger after exhaustion */
    /*12 */ { GXHT40_ERR_IO,       999, 999, 1, 1, {1,1,1}, 0 }, /* failed cycle */
    /*13 */ { GXHT40_OK,            55, 660, 1, 1, {1,0,0}, 0 }, /* rise 10 from 45 -> trigger */
    /*14 */ { GXHT40_OK,            55, 660, 0, 0, {1,1,1}, 0 }, /* no rise -> no trigger */
    /*15 */ { GXHT40_OK,            56, 661, 1, 1, {1,1,1}, 0 }  /* rise 1 (<= 9) -> no trigger */
};

static int      cyc;
static int      attempt_in_cycle;
static int      light_calls, gxht_calls;
static int      send_calls;
static uint8_t  send_frame[MAX_ATTEMPTS][10];
static uint8_t  send_len[MAX_ATTEMPTS];
static uint32_t send_to[MAX_ATTEMPTS];
static uint8_t  send_ret_log[MAX_ATTEMPTS];
static uint8_t  last_frame[10];

void light_init(void) { }

light_result_t light_sample(void)
{
    light_result_t r;
    memset(&r, 0, sizeof(r));
    r.adc_ok        = true;
    r.samples_ok    = 8u;
    r.mean_adc_code = STIM[cyc].dark ? 3000u : 500u;
    r.code_min      = STIM[cyc].dark ? 2900u : 480u;
    r.code_max      = STIM[cyc].dark ? 3100u : 520u;
    r.valid         = STIM[cyc].lv ? true : false;
    r.dark          = STIM[cyc].dark ? true : false;
    light_calls++;
    return r;
}

void gxht40_init(i2c_dev *dev) { (void)dev; }

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
    out->read_retry = 2u;
    out->attempt    = 3u;
    for (i = 0u; i < (uint8_t)GXHT40_RESULT_LEN; i++) out->raw[i] = (uint8_t)(0xB0u + i);
}

/*
 * RF stub. Records the exact bytes/length/timeout of every attempt.
 * With STIM[cyc].mutate=1 the first attempt also overwrites tempvalue/
 * huminityvalue (a new sample arriving while the transaction is in flight):
 * a correct implementation still sends the frozen buffer on attempts 2/3.
 */
uint8_t app_um2005C_send_data_timeout(uint8_t *d, uint16_t n, uint32_t to)
{
    int k = attempt_in_cycle;
    uint8_t ret = (k < MAX_ATTEMPTS) ? STIM[cyc].seq[k] : 0u;

    if (k < MAX_ATTEMPTS) {
        memset(send_frame[k], 0, 10);
        if (d != NULL) memcpy(send_frame[k], d, (n >= 10u) ? 10u : n);
        send_len[k] = n;
        send_to[k]  = to;
        send_ret_log[k] = ret;
    }
    if ((k == 0) && (STIM[cyc].mutate != 0u)) {
        tempvalue     = 999;
        huminityvalue = 999u;
    }
    attempt_in_cycle++;
    send_calls++;
    return ret;
}

/* ==================== driver ==================== */
static void hex(const uint8_t *b, int n, char *out)
{
    int i;
    for (i = 0; i < n; i++) sprintf(out + 2 * i, "%02X", b[i]);
    out[2 * n] = '\0';
}

static void print_ref(int idx, int16_t t, uint16_t h)
{
    uint8_t ref[10];
    char    s[32];
    __real_encode_frame10(mcu_uid, t, h, ref);
    hex(ref, 10, s);
    printf("REF cyc=%d t=%d h=%u frame=%s\n", idx, (int)t, (unsigned)h, s);
}

int main(void)
{
    debug_trace_sample_t tr;
    int i, k;

    memset(mcu_uid, 0, 10);
    mcu_uid[0] = 0x6A; mcu_uid[1] = 0x00; mcu_uid[2] = 0x2C; mcu_uid[3] = 0x00;
    for (i = 4; i < 10; i++) mcu_uid[i] = (uint8_t)(0x40 + i);

    printf("RUN %s cycles=%d\n", VERIFY_LABEL, N_CYCLES);

    for (cyc = 0; cyc < N_CYCLES; cyc++) {
        int     enc_before = encode_calls;
        int     send_before = send_calls;
        int     extra_before;
        uint8_t fetched;
        int     sends, extra;
        char    seq[16];

        /* stimulus sequence as configured (a scripted response, not a result) */
        sprintf(seq, "%u%u%u", (unsigned)STIM[cyc].seq[0],
                (unsigned)STIM[cyc].seq[1], (unsigned)STIM[cyc].seq[2]);
        printf("SEQ cyc=%d st=%d t=%d h=%u dark=%u lv=%u seq=%s mutate=%u\n",
               cyc, (int)STIM[cyc].st, (int)STIM[cyc].t, (unsigned)STIM[cyc].h,
               (unsigned)STIM[cyc].dark, (unsigned)STIM[cyc].lv, seq,
               (unsigned)STIM[cyc].mutate);

        attempt_in_cycle = 0;
        sample_flag = 1u;
        temperature_process();
        /* main.c order: process -> send -> trace fetch -> print */
        send_data_to_gateway();

        /* mutation stimulus is bookkeeping, not a sample: restore the trigger
         * values so the S-trace reflects the cycle's real sample */
        if (STIM[cyc].mutate != 0u) {
            tempvalue     = STIM[cyc].t;
            huminityvalue = STIM[cyc].h;
        }

        sends = send_calls - send_before;

        memset(&tr, 0xEE, sizeof(tr));
        fetched = sensor_trace_fetch(&tr);

        printf("C cyc=%d enc=%d sends=%d p=%d H=%u t=%d h=%u q=%u r=%u s=%u y=%u req=%u diag=%u fetched=%u\n",
               cyc, encode_calls - enc_before, sends,
               (int)tr.prev_temp_x10, (unsigned)tr.have_prev,
               (int)tr.temp_x10, (unsigned)tr.hum_x10,
               (unsigned)tr.sample_ok, (unsigned)tr.report, (unsigned)tr.send,
               (unsigned)tr.retry, (unsigned)report_req, (unsigned)tr.diag_valid,
               (unsigned)fetched);

        for (k = 0; k < sends && k < MAX_ATTEMPTS; k++) {
            char    f[32];
            hex(send_frame[k], 10, f);
            printf("A cyc=%d call=%d ret=%u len=%u to=%u frame=%s\n",
                   cyc, k, (unsigned)send_ret_log[k], (unsigned)send_len[k],
                   (unsigned)send_to[k], f);
        }

        /* reference bytes for the trigger sample (independent Python model decides
         * which cycles must trigger and compares every attempt against this) */
        if (STIM[cyc].st == GXHT40_OK) {
            print_ref(cyc, STIM[cyc].t, STIM[cyc].h);
        }

        /* After-cycle probe: pending state must be gone after success or
         * exhaustion -> a further send call must not reach the stub. */
        extra_before = send_calls;
        attempt_in_cycle = 0;
        send_data_to_gateway();
        extra = send_calls - extra_before;
        printf("AFTER cyc=%d extra_sends=%d\n", cyc, extra);
    }

    /* second fetch in the same cycle: nothing pending, *out untouched */
    {
        debug_trace_sample_t twice;
        uint8_t again;
        memset(&twice, 0x5A, sizeof(twice));
        again = sensor_trace_fetch(&twice);
        printf("FETCH_AGAIN ret=%u first_byte_dec=%u\n",
               (unsigned)again, (unsigned)((uint8_t *)&twice)[0]);
    }

    printf("COUNT light_calls=%d gxht_calls=%d encode_calls=%d send_calls=%d\n",
           light_calls, gxht_calls, encode_calls, send_calls);
    printf("TEMP_PTR_NULL=%d\n", temp_ptr == NULL ? 1 : 0);

    /* keep last_frame referenced (used by the frame dump above) */
    (void)last_frame;
    return 0;
}
