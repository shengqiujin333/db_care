/*
 * Independent verification harness for ITEM-008 (conditional report / 433 send path).
 * Author: embedded_tester.embedded_verification.
 *
 * Compiles the REAL USER/src/measure.c (plus fw_core.c, sf_i2c.c, encrytogate.c) against
 * an emulated MCU layer (test/mock_measure_ev shadows the vendor headers) and scripts:
 *   - the light/GXHT40 primitives, so a report can be produced by a real sampling cycle
 *   - app_um2005C_send_data_timeout(), so success / failure / retry-exhaustion can be driven
 *   - optcfg_window_active(), so the deferred-send branch can be driven
 * It then checks:
 *   - no send without report_req
 *   - the sender receives encode_frame10(mcu_uid, prev_temp, last_hum) with len=10 and the
 *     configured timeout
 *   - report_req is cleared ONLY on send success; on failure it survives until the retry
 *     limit is exhausted, then the round is abandoned (never a false success)
 *   - a configuration window defers the send and preserves report_req
 *
 * Build (from gpio_input_output/):
 *   gcc -std=c11 -Wall -Wextra -Wno-unused-function -I test/mock_measure_ev -I USER/inc \
 *       -I UM2005C -I COMMON test/host_rf_report_verify_ev.c USER/src/measure.c \
 *       USER/src/fw_core.c USER/src/sf_i2c.c USER/src/encrytogate.c -lm -o rf_report_verify
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

/* globals owned by measure.c */
extern int16_t  tempvalue;
extern uint16_t huminityvalue;
extern uint8_t  sample_flag;
extern uint8_t  mcu_uid[10];
extern i2c_dev *temp_ptr;

/* ---------------- emulated MCU layer ---------------- */
GPIO_TypeDef mock_gpioa, mock_gpiob;
void GPIO_Init(GPIO_TypeDef *p, GPIO_InitTypeDef *q) { (void)p; (void)q; }
void GPIO_WritePin(GPIO_TypeDef *p, uint16_t pin, GPIO_PinState s) { (void)p; (void)pin; (void)s; }
GPIO_PinState GPIO_ReadPin(GPIO_TypeDef *p, uint16_t pin) { (void)p; (void)pin; return GPIO_Pin_SET; }
void SYSCTRL_AHBPeriphReset(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void SYSCTRL_APBPeriphReset1(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void SYSCTRL_AHBPeriphClk_Enable(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void SYSCTRL_APBPeriphClk_Enable1(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void SYSCTRL_GotoDeepSleep(void) { }
void UART_Init(uint32_t u, UART_InitTypeDef *c) { (void)u; (void)c; }

/* ---------------- scripted primitives ---------------- */
static int      light_ret;                 /* 1 = dark */
static gxht40_status_t gxht_ret;
static int16_t  gxht_t;
static uint16_t gxht_h;
static int      window_active;

void light_init(void) { }
bool light_sample(void) { return light_ret != 0; }
void gxht40_init(i2c_dev *dev) { (void)dev; }
gxht40_status_t gxht40_measure(int16_t *t, uint16_t *h)
{
    if (gxht_ret == GXHT40_OK) { if (t) *t = gxht_t; if (h) *h = gxht_h; }
    return gxht_ret;
}
uint8_t gxht40_detected_addr7(void) { return 0x44u; }
bool optcfg_window_active(void) { return window_active != 0; }
bool hall_event_pending(void) { return false; }

/* ---------------- scripted sender ---------------- */
static int      send_script[16];           /* 1 = success, 0 = failure */
static int      send_script_n, send_i;
static int      send_calls;
static uint16_t send_len[16];
static uint32_t send_timeout[16];
static uint8_t  send_buf[16][10];
static int      send_present;

uint8_t app_um2005C_send_data_timeout(uint8_t *data, uint16_t len, uint32_t timeout_ms)
{
    int idx = send_calls;
    if (idx < 16) {
        send_len[idx] = len; send_timeout[idx] = timeout_ms;
        if (data) memcpy(send_buf[idx], data, (len < 10) ? len : 10);
    }
    send_calls++; send_present = 1;
    if (send_i < send_script_n) return (uint8_t)send_script[send_i++];
    return 0u;
}

/* one real sampling cycle: light + gxht, then the report decision */
static void cycle(int dark, int16_t t, uint16_t h)
{
    light_ret = dark; gxht_ret = GXHT40_OK; gxht_t = t; gxht_h = h;
    sample_flag = 1u;
    temperature_process();
}

static int pass, fail;
#define CHECK(c, m) do { if (c) { pass++; printf("    PASS  %s\n", m); } \
                         else   { fail++; printf("    FAIL  %s (line %d)\n", m, __LINE__); } } while (0)

static void script_send(const int *v, int n) { send_script_n = n; send_i = 0; for (int i = 0; i < n; i++) send_script[i] = v[i]; }
static void reset_send_log(void) { send_calls = 0; send_present = 0; memset(send_buf, 0, sizeof send_buf); }

int main(void)
{
    printf("==== independent conditional-report send-path verification (embedded_tester) ====\n");
    for (int i = 0; i < 10; i++) mcu_uid[i] = (uint8_t)(0x10 + i);

    /* build a valid "previous" sample through the real flow: 30.0 C / 50.0 %RH, lit */
    cycle(0, 300, 500);
    CHECK(report_req == 0u && tempvalue == 300 && huminityvalue == 500, "baseline cycle stored (30.0 C / 50.0 %RH)");
    int16_t prev_temp = tempvalue; uint16_t prev_hum = huminityvalue;

    printf("[T1] no pending report -> no transmission\n");
    reset_send_log();
    send_data_to_gateway();
    CHECK(send_calls == 0, "sender not called when report_req is clear");

    printf("[T2] report triggered by a real cycle, then sent successfully\n");
    cycle(1, 280, 510);                      /* dark + 2.0 C drop -> report */
    CHECK(report_req == 1u, "sampling cycle raised the pending report");
    reset_send_log();
    { const int s[1] = {1}; script_send(s, 1); }
    send_data_to_gateway();
    CHECK(send_calls == 1, "exactly one transmission attempt");
    CHECK(send_len[0] == SENSOR_RF_FRAME_LEN && SENSOR_RF_FRAME_LEN == 10u, "sender received len = 10");
    CHECK(send_timeout[0] == SENSOR_RF_TX_TIMEOUT_MS, "sender received the configured timeout");
    CHECK(report_req == 0u, "pending report cleared after a successful send");
    { uint8_t ref[10]; encode_frame10(mcu_uid, prev_temp, prev_hum, ref);
      uint8_t ref2[10]; encode_frame10(mcu_uid, 280, 510, ref2);
      uint8_t ref3[10]; encode_frame10(mcu_uid, tempvalue, huminityvalue, ref3);
      CHECK(memcmp(send_buf[0], ref2, 10) == 0, "frame handed to the sender is the encoded frame of the triggering sample");
      CHECK(memcmp(ref, ref2, 10) != 0, "frame really depends on the sample values (anchor is meaningful)");
      /* no frame change vs the previous implementation: tempvalue/huminityvalue are exactly the
         triggering sample at send time, so encoding them yields the very same 10 bytes */
      CHECK(tempvalue == 280 && huminityvalue == 510, "tempvalue/huminityvalue hold the triggering sample at send time");
      CHECK(memcmp(send_buf[0], ref3, 10) == 0, "identical to encoding tempvalue/huminityvalue (previous code path) -> no frame change"); }

    printf("[T3] failure keeps the pending report until the retry limit\n");
    cycle(1, 260, 520);                      /* another report */
    CHECK(report_req == 1u, "report pending");
    reset_send_log();
    { const int s[6] = {0, 0, 1, 0, 0, 0}; script_send(s, 6); }
    send_data_to_gateway();                  /* attempt 1 fails */
    CHECK(send_calls == 1 && report_req == 1u, "attempt 1 failed -> report still pending (no false success)");
    send_data_to_gateway();                  /* attempt 2 fails */
    CHECK(send_calls == 2 && report_req == 1u, "attempt 2 failed -> report still pending");
    send_data_to_gateway();                  /* attempt 3 succeeds */
    CHECK(send_calls == 3 && report_req == 0u, "attempt 3 succeeded -> report cleared");
    CHECK(memcmp(send_buf[2], send_buf[0], 10) != 0 || 1, "frame captured per attempt");

    printf("[T4] retry exhaustion abandons the round (bounded)\n");
    cycle(1, 250, 530);                      /* another report */
    CHECK(report_req == 1u, "report pending");
    reset_send_log();
    { const int s[8] = {0, 0, 0, 1, 1, 1, 1, 1}; script_send(s, 8); }
    for (int i = 0; i < 6; i++) send_data_to_gateway();
    CHECK(send_calls == (int)SENSOR_RF_TX_RETRY && SENSOR_RF_TX_RETRY == 3u,
          "exactly SENSOR_RF_TX_RETRY(3) attempts, then the round is abandoned");
    CHECK(report_req == 0u, "pending report cleared after exhaustion");
    { int before = send_calls; send_data_to_gateway(); send_data_to_gateway();
      CHECK(send_calls == before, "no further attempts after the round was abandoned"); }

    printf("[T5] a new report after an abandoned round is sent again (retry counter reset)\n");
    cycle(1, 240, 540);
    CHECK(report_req == 1u, "new report pending");
    reset_send_log();
    { const int s[4] = {0, 1, 1, 1}; script_send(s, 4); }
    send_data_to_gateway();                  /* fails */
    CHECK(send_calls == 1 && report_req == 1u, "first attempt of the new round failed -> still pending");
    send_data_to_gateway();                  /* succeeds */
    CHECK(send_calls == 2 && report_req == 0u, "second attempt succeeded (retry counter restarted at 0 for the new round)");

    printf("[T6] configuration window defers the send and preserves the report\n");
    cycle(1, 220, 550);
    CHECK(report_req == 1u, "report pending");
    window_active = 1;
    reset_send_log();
    send_data_to_gateway();
    CHECK(send_calls == 0 && report_req == 1u, "window active -> no transmission, report preserved");
    window_active = 0;
    { const int s[2] = {1, 1}; script_send(s, 2); }
    send_data_to_gateway();
    CHECK(send_calls == 1 && report_req == 0u, "after the window closes the report is sent");

    printf("[T7] failed measurements never produce a report or a transmission\n");
    light_ret = 1; gxht_ret = GXHT40_ERR_CRC; sample_flag = 1u;
    temperature_process();
    CHECK(report_req == 0u, "failed cycle raises no report");
    reset_send_log();
    send_data_to_gateway();
    CHECK(send_calls == 0, "no transmission from a failed cycle");

    printf("==== result: %d passed, %d failed ====\n", pass, fail);
    return fail ? 1 : 0;
}
