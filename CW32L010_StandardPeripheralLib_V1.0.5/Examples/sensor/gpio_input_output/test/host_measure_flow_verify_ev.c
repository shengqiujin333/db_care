/*
 * Independent verification harness for ITEM-006 (measure.c sampling flow).
 * Author: embedded_tester.embedded_verification.
 *
 * Compiles the REAL USER/src/measure.c against an emulated MCU layer
 * (mock_measure/cw32l010_{gpio,adc,sysctrl,uart}.h shadow the vendor headers) and
 * scripts the sensor primitives (light_sample / gxht40_measure), so the flow logic
 * is exercised for real:
 *   - light is sampled BEFORE temperature/humidity in every cycle
 *   - a failed cycle must not report, must not advance the previous valid temperature
 *     and must not fabricate a 0 value
 *   - a successful cycle advances previous temperature + last valid humidity and uses
 *     the PRE-update previous value for the report decision
 * A small reference model of the intended flow is maintained in the harness and
 * compared against measure.c's globals after every cycle.
 *
 * Build (from gpio_input_output/):
 *   gcc -std=c11 -Wall -Wextra -Wno-unused-function -I test/mock_measure -I USER/inc \
 *       -I UM2005C -I COMMON test/host_measure_flow_verify_ev.c USER/src/measure.c \
 *       USER/src/fw_core.c USER/src/sf_i2c.c USER/src/encrytogate.c -lm -o measure_verify
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "measure.h"
#include "sensor_config.h"
#include "gxht40.h"
#include "light.h"
#include "mock_measure_hw.h"

/* globals owned by measure.c */
extern int16_t  tempvalue;
extern uint16_t huminityvalue;
extern uint8_t  sample_flag;
extern i2c_dev *temp_ptr;

/* ---------------- emulated MCU layer ---------------- */
GPIO_TypeDef mock_gpioa, mock_gpiob;
static int gpio_init_calls;
static int i2c_phy_init_calls;      /* GPIO_Init with PA3|PA4 open-drain */
static int gpio_writes;
static int deep_sleep_calls;
static int uart_init_calls;

void GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *init)
{
    (void)port; gpio_init_calls++;
    if (init->Pins == (GPIO_PIN_4 | GPIO_PIN_3) && init->Mode == GPIO_MODE_OUTPUT_OD) i2c_phy_init_calls++;
}
void GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState st) { (void)port; (void)pin; (void)st; gpio_writes++; }
GPIO_PinState GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin) { (void)port; (void)pin; return GPIO_Pin_SET; }
void SYSCTRL_AHBPeriphReset(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void SYSCTRL_APBPeriphReset1(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void SYSCTRL_AHBPeriphClk_Enable(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void SYSCTRL_APBPeriphClk_Enable1(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void SYSCTRL_GotoDeepSleep(void) { deep_sleep_calls++; }
void UART_Init(uint32_t u, UART_InitTypeDef *c) { (void)u; (void)c; uart_init_calls++; }

/* ---------------- scripted sensor primitives ---------------- */
#define MAXC 64
static bool             scr_dark[MAXC];
static gxht40_status_t  scr_gxht[MAXC];
static int16_t          scr_t[MAXC];
static uint16_t         scr_h[MAXC];
static int              scr_n;
static int              cyc;                    /* cycle index consumed by the primitives */
static char             order[2 * MAXC + 1];
static int              order_n;
static int              light_calls, gxht_calls, gxht_init_calls, light_init_calls;
static int              send_calls;

void light_init(void) { light_init_calls++; }
bool light_sample(void)
{
    bool d = (cyc < scr_n) ? scr_dark[cyc] : false;
    light_calls++; order[order_n++] = 'L';
    return d;
}
void gxht40_init(i2c_dev *dev) { (void)dev; gxht_init_calls++; }
gxht40_status_t gxht40_measure(int16_t *t, uint16_t *h)
{
    gxht40_status_t st = (cyc < scr_n) ? scr_gxht[cyc] : GXHT40_ERR_IO;
    gxht_calls++; order[order_n++] = 'G';
    if (st == GXHT40_OK) {                      /* driver contract: write outputs only on success */
        if (t) *t = scr_t[cyc];
        if (h) *h = scr_h[cyc];
    }
    return st;
}
uint8_t gxht40_detected_addr7(void) { return 0x44u; }
bool optcfg_window_active(void) { return false; }
bool hall_event_pending(void) { return false; }
uint8_t app_um2005C_send_data_timeout(uint8_t *d, uint16_t n, uint32_t to) { (void)d; (void)n; (void)to; send_calls++; return 1u; }

/* ---------------- reference model of the intended flow ---------------- */
static int16_t  ref_temp = 0, ref_prev = 0;
static uint16_t ref_hum = 0;
static bool     ref_have = false;
static uint8_t  ref_report = 0;
static bool     ref_rep_now = false;   /* this cycle's report decision */
static int      fail_cycles;

static void ref_cycle(void)
{
    bool dark = (cyc < scr_n) ? scr_dark[cyc] : false;
    gxht40_status_t st = (cyc < scr_n) ? scr_gxht[cyc] : GXHT40_ERR_IO;
    if (st == GXHT40_OK) {
        int16_t cur = scr_t[cyc];
        bool rep = (cur > SENSOR_REPORT_HIGH_X10) ||
                   (cur < SENSOR_REPORT_HIGH_X10 && ref_have && dark &&
                    ((int32_t)ref_prev - (int32_t)cur > SENSOR_REPORT_DROP_X10));
        ref_rep_now = rep;
        if (rep) ref_report = 1u;
        ref_prev = cur; ref_have = true;
        ref_temp = cur; ref_hum = scr_h[cyc];
    } else {
        ref_rep_now = false;
        fail_cycles++;
    }
}

static int pass, fail;
#define CHECK(c, m) do { if (c) { pass++; printf("    PASS  %s\n", m); } \
                         else   { fail++; printf("    FAIL  %s (line %d)\n", m, __LINE__); } } while (0)

int main(void)
{
    printf("==== independent measure.c flow verification (embedded_tester) ====\n");

    /* scripted cycles: dark, gxht status, temp_x10, hum_x10 */
    const bool    d[MAXC] = { false,false,true,true,true,true,true,false,false,true,false,true,true,false };
    const gxht40_status_t g[MAXC] = { GXHT40_OK,GXHT40_OK,GXHT40_OK,GXHT40_ERR_IO,GXHT40_OK,GXHT40_ERR_CRC,GXHT40_ERR_RANGE,
                                      GXHT40_OK,GXHT40_OK,GXHT40_OK,GXHT40_OK,GXHT40_OK,GXHT40_ERR_CRC,GXHT40_OK };
    const int16_t t[MAXC] = { 250,240,220,0,210,0,0,351,350,340,330,300,0,280 };
    const uint16_t h[MAXC] = { 500,510,600,0,610,0,0,700,710,720,730,740,0,750 };
    scr_n = 14;
    for (int i = 0; i < scr_n; i++) { scr_dark[i] = d[i]; scr_gxht[i] = g[i]; scr_t[i] = t[i]; scr_h[i] = h[i]; }

    /* ---- run the cycles ---- */
    static bool newly_set[MAXC];
    for (cyc = 0; cyc < scr_n; cyc++) {
        if (cyc == 6) {
            /* failing cycle while a report is already pending: it must stay pending */
            report_req = 1u;
            ref_cycle();
            sample_flag = 1u;
            temperature_process();
            printf("  cycle %2d: dark=%d gxht=%d (pending-report cycle) -> report_req=%u tempvalue=%d hum=%u\n",
                   cyc, (int)d[cyc], (int)g[cyc], report_req, (int)tempvalue, (unsigned)huminityvalue);
            CHECK(report_req == 1u, "a failing cycle does not clear an already pending report");
            CHECK(tempvalue == ref_temp && huminityvalue == ref_hum, "state unchanged on the pending-report failure cycle");
            CHECK(sample_flag == 0u, "sample flag consumed after the cycle");
            newly_set[cyc] = false;
            report_req = 0u;
            continue;
        }
        report_req = 0u;                 /* simulate the previous report having been sent */
        ref_cycle();
        sample_flag = 1u;
        temperature_process();
        newly_set[cyc] = (report_req != 0u);

        printf("  cycle %2d: dark=%d gxht=%d -> tempvalue=%d hum=%u report_req=%u (ref decision=%d)\n",
               cyc, (int)d[cyc], (int)g[cyc], (int)tempvalue, (unsigned)huminityvalue, report_req,
               (int)ref_rep_now);
        CHECK(tempvalue == ref_temp, "last valid temperature matches the reference model");
        CHECK(huminityvalue == ref_hum, "last valid humidity matches the reference model");
        CHECK(newly_set[cyc] == ref_rep_now, "a report is raised exactly when the successful cycle decides to");
        CHECK(sample_flag == 0u, "sample flag consumed after the cycle");
    }

    /* ---- structural checks ---- */
    printf("[S1] lazy init and per-cycle primitive usage\n");
    CHECK(light_init_calls == 1 && gxht_init_calls == 1, "light_init/gxht40_init called exactly once (lazy init)");
    CHECK(i2c_phy_init_calls == 1, "I2C phy configured exactly once");
    CHECK(temp_ptr != NULL, "i2c_obj_find(\"i2c0\") returned a bound device");
    CHECK(light_calls == scr_n, "light sampled in every cycle (including failed ones)");
    CHECK(gxht_calls == scr_n, "temperature/humidity attempted once per cycle");
    CHECK(gpio_writes > 0, "GPIO activity observed");
    CHECK(uart_init_calls == 0, "debug UART not configured by the sampling path (low power)");

    printf("[S2] light is always sampled BEFORE temperature/humidity\n");
    { int bad = 0;
      for (int i = 0; i < order_n; i += 2) if (!(order[i] == 'L' && order[i+1] == 'G')) bad++;
      CHECK(order_n == 2 * scr_n, "one light + one gxht call per cycle");
      CHECK(bad == 0, "every cycle's call order is light -> gxht"); }

    printf("[S3] failure cycles do not fabricate values\n");
    CHECK(fail_cycles == 4, "scripted failures were exercised (ERR_IO/ERR_CRC/ERR_RANGE)");
    CHECK(ref_temp != 0, "reference model holds a non-zero valid temperature");
    CHECK(tempvalue == 280, "tempvalue equals the last successful measurement (never 0/never a failure artifact)");
    CHECK(huminityvalue == 750u, "huminityvalue equals the last successful measurement");
    CHECK(report_req == 0u, "final pending-report state matches the model (last cycle decided not to report)");

    printf("[S4] failure does not advance the previous valid temperature\n");
    /* cycle 3 failed (ERR_IO) while prev=240; cycle 4 measured 210 -> drop 30 (>9) and dark -> must report.
       Had the failed cycle polluted prev (e.g. reset to 0), the drop would be negative and no report. */
    CHECK(newly_set[3] == false, "the failed cycle itself raises no report");
    CHECK(newly_set[4] == true, "the drop after the failed cycle is measured against the pre-failure value");

    printf("[S5] current-cycle light state is used (no leakage from failed cycles)\n");
    CHECK(newly_set[11] == true, "dark + drop reports");
    /* cycle 12 failed while dark=true (light result must be discarded); cycle 13 is lit with a drop */
    CHECK(newly_set[12] == false, "a failed cycle raises no report even when dark");
    CHECK(newly_set[13] == false, "the lit cycle after the dark failure does not report");

    printf("[S6] go_to_sleep gating\n");
    { int before = deep_sleep_calls;
      report_req = 1u; sample_flag = 0u; go_to_sleep();
      CHECK(deep_sleep_calls == before, "pending report blocks deep sleep");
      report_req = 0u; sample_flag = 1u; go_to_sleep();
      CHECK(deep_sleep_calls == before, "pending sample blocks deep sleep");
      sample_flag = 0u; go_to_sleep();
      CHECK(deep_sleep_calls == before + 1, "idle state enters deep sleep"); }

    printf("[S7] sample flag gating\n");
    { int lc = light_calls, gc = gxht_calls; int16_t tv = tempvalue;
      sample_flag = 0u; temperature_process();
      CHECK(light_calls == lc && gxht_calls == gc, "no sampling when the flag is clear");
      CHECK(tempvalue == tv, "no state change when the flag is clear"); }

    printf("==== result: %d passed, %d failed ====\n", pass, fail);
    return fail ? 1 : 0;
}
