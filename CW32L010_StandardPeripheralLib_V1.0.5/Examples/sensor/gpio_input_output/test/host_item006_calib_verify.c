/*
 * host_item006_calib_verify.c - independent verification harness for ITEM-006
 * (calibration configuration gate semantics, uncalibrated gating, 1/3 boundary,
 *  100 ms / 8 samples / integer mean preservation).
 *
 * Author: embedded_tester.embedded_verification (test-owned asset; not part of the
 * firmware build and does not modify product source).
 *
 * It compiles the REAL USER/src/light.c and USER/src/fw_core.c against this
 * capability's own MCU shadow layer (test/mock_vdd_ev) so the actual sample
 * sequence and the actual gate function are exercised:
 *   PB05 high -> delay(LIGHT_SETTLE_MS) -> LIGHT_ADC_SAMPLES conversions -> mean
 *   -> valid = adc_ok && LIGHT_DARK_CALIBRATED -> dark = light_is_dark(...)
 *   -> PB05 low
 *
 * The same binary is compiled four times from the same source to cover the
 * configuration-dependent behaviour:
 *   - default delivery configuration (LIGHT_DARK_CALIBRATED=0)
 *   - calibrated variants: REF=1, REF=3000, REF=4095
 * The expected values are taken from the preprocessor macros of the translation
 * unit, i.e. from the real configuration, while the decision model is computed
 * independently in this harness.
 *
 * Build (from gpio_input_output/):
 *   gcc -std=c11 -Wall -Wextra -DSENSOR_CONFIG_NO_MCU \
 *       -Itest/mock_vdd_ev -IUSER/inc -ICOMMON \
 *       test/host_item006_calib_verify.c USER/src/light.c USER/src/fw_core.c \
 *       -o calib_verify
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "light.h"
#include "fw_core.h"
#include "sensor_config.h"
#include "delay.h"
#include "mock_vdd_hw.h"

/* ---------------- mock MCU state (owned by this harness) ---------------- */
GPIO_TypeDef mock_gpiob;
ADC_TypeDef  mock_adc;
volatile uint16_t mock_bgr_trim_value = 1200u;   /* not used: light_read_vdd_mv is not exercised here */

static int pin_write_pin[64], pin_write_val[64], pin_write_n;
static int gpio_mode[8];
static int gpio_init_calls;
static ADC_InitTypeDef adc_cfg; static int adc_init_calls;
static int adc_enable_calls, adc_disable_calls, adc_enabled;
static int conv_calls;
static uint16_t conv_value;
static uint32_t isr_poll_count;
static uint16_t delay_log[64]; static int delay_n;
static int script[16]; static int script_n, script_i;   /* <0 = EOC never set */

void delay_ms(uint16_t ms) { if (delay_n < 64) delay_log[delay_n++] = ms; }

void GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *cfg)
{
    (void)port; gpio_init_calls++;
    for (int b = 0; b < 8; b++) if (cfg->Pins & (1u << b)) gpio_mode[b] = (int)cfg->Mode;
}
void GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state)
{
    (void)port;
    if (pin_write_n < 64) {
        pin_write_pin[pin_write_n] = pin;
        pin_write_val[pin_write_n] = (int)state;
        pin_write_n++;
    }
}
void ADC_Init(ADC_InitTypeDef *cfg) { adc_cfg = *cfg; adc_init_calls++; }
void ADC_ClearITPendingAll(void) { mock_adc.ISR = 0; }
void ADC_Disable(void) { adc_disable_calls++; adc_enabled = 0; }
ErrorStatus ADC_Enable(void) { adc_enable_calls++; adc_enabled = 1; return SUCCESS; }
void ADC_ClearITPendingBit(uint16_t it) { (void)it; mock_adc.ISR = 0; }
uint16_t ADC_GetConversionValue(uint32_t idx) { (void)idx; return conv_value; }

void ADC_SoftwareStartConvCmd(FunctionalState state)
{
    (void)state;
    conv_calls++;
    mock_adc.ISR = 0;
    if (script_i < script_n) {
        if (script[script_i] >= 0) {
            conv_value = (uint16_t)script[script_i];
            mock_adc.ISR = ADC_ISR_EOC_Msk;
        }
        script_i++;
    }
}

/* ---------------- helpers ---------------- */
static void log_reset(void)
{
    pin_write_n = 0; delay_n = 0; conv_calls = 0; script_i = 0; isr_poll_count = 0;
    adc_enable_calls = 0; adc_disable_calls = 0;
}
static void script_const(int value, int n) { script_n = n; for (int i = 0; i < n; i++) script[i] = value; }
static void script_set(const int *v, int n) { script_n = n; for (int i = 0; i < n; i++) script[i] = v[i]; }
static int pb05_sets(void)
{
    int c = 0;
    for (int i = 0; i < pin_write_n; i++)
        if (pin_write_pin[i] == GPIO_PIN_5 && pin_write_val[i] == 1) c++;
    return c;
}
static int pb05_resets(void)
{
    int c = 0;
    for (int i = 0; i < pin_write_n; i++)
        if (pin_write_pin[i] == GPIO_PIN_5 && pin_write_val[i] == 0) c++;
    return c;
}
static int other_pin_writes(void)
{
    int c = 0;
    for (int i = 0; i < pin_write_n; i++) if (pin_write_pin[i] != GPIO_PIN_5) c++;
    return c;
}
static int first_write_is_pb05_high(void)
{
    return pin_write_n > 0 && pin_write_pin[0] == GPIO_PIN_5 && pin_write_val[0] == 1;
}
static int last_write_is_pb05_low(void)
{
    return pin_write_n > 0 && pin_write_pin[pin_write_n - 1] == GPIO_PIN_5 &&
           pin_write_val[pin_write_n - 1] == 0;
}

static int pass, fail;
#define CHECK(c, m) do { if (c) { pass++; printf("    PASS  %s\n", m); } \
                         else   { fail++; printf("    FAIL  %s (line %d)\n", m, __LINE__); } } while (0)

/* independent reference model: valid && 3*mean >= C, integer arithmetic only */
static bool ref_dark(uint16_t mean, bool valid, uint16_t c)
{
    if (!valid) return false;
    return ((uint32_t)3u * (uint32_t)mean) >= (uint32_t)c;
}
static uint32_t ceil3(uint32_t c) { return (c + 2u) / 3u; }

int main(void)
{
    printf("==== independent ITEM-006 calibration/gating verification (embedded_tester) ====\n");
    printf("CONFIG CALIBRATED=%d REF_CODE=%u LIGHT_SETTLE_MS=%u LIGHT_ADC_SAMPLES=%u BUILD_ID=%s\n",
           (int)LIGHT_DARK_CALIBRATED, (unsigned)LIGHT_DARK_REF_CODE,
           (unsigned)LIGHT_SETTLE_MS, (unsigned)LIGHT_ADC_SAMPLES, SENSOR_FW_BUILD_ID);
    (void)ref_dark;   /* referenced only by the calibrated variant */

    /* sanity of the delivered configuration domain itself */
    CHECK(LIGHT_DARK_CALIBRATED == 0 || LIGHT_DARK_CALIBRATED == 1,
          "LIGHT_DARK_CALIBRATED is 0 or 1 in this variant (gate would have rejected otherwise)");
    CHECK(LIGHT_SETTLE_MS == 100u, "settle delay unchanged at 100 ms");
    CHECK(LIGHT_ADC_SAMPLES == 8u, "burst count unchanged at 8 conversions");
    CHECK(strcmp(SENSOR_FW_BUILD_ID, "RUN10-FDS003") == 0,
          "build identifier constant is RUN10-FDS003");

    /* ---- T1: init ownership ---- */
    printf("[T1] light_init pin ownership / ADC configuration\n");
    for (int i = 0; i < 8; i++) gpio_mode[i] = -1;
    light_init();
    CHECK(gpio_mode[5] == GPIO_MODE_OUTPUT_PP, "PB05 = push-pull output (light power)");
    CHECK(gpio_mode[4] == GPIO_MODE_ANALOG, "PB04 = analog input (AIN11)");
    CHECK(gpio_mode[3] == -1 && gpio_mode[6] == -1, "no other pin configured by the light module");
    CHECK(pin_write_n >= 1 && pin_write_pin[0] == GPIO_PIN_5 && pin_write_val[0] == 0,
          "PB05 driven low at init");
    CHECK(adc_init_calls == 1, "ADC_Init called once");
    CHECK(adc_cfg.ADC_IN0.ADC_InputChannel == ADC_InputCH11, "ADC channel = AIN11 (PB04)");
    CHECK(adc_cfg.ADC_ClkDiv == ADC_Clk_Div8, "ADCCLK = PCLK/8");
    CHECK(adc_cfg.ADC_IN0.ADC_SampTime == ADC_SampTime390Clk, "390 clk sample time");
    CHECK(adc_enabled == 0, "ADC left disabled after init");

    /* ---- T2: timing / burst / integer mean ---- */
    printf("[T2] sample sequence: 100 ms settle, 8 conversions, integer mean\n");
    {
        const int v[8] = {1, 2, 3, 4, 5, 6, 7, 8};
        log_reset(); script_set(v, 8);
        light_result_t r = light_sample();
        CHECK(delay_n == 1 && delay_log[0] == 100u, "exactly one 100 ms settle delay");
        CHECK(conv_calls == 8, "exactly 8 conversions attempted");
        CHECK(r.samples_ok == 8, "8 successful samples recorded");
        CHECK(r.adc_ok == true, "adc_ok=true when conversions succeed");
        CHECK(r.mean_adc_code == 4, "mean = 36/8 = 4 (truncating integer division)");
        CHECK(r.code_min == 1 && r.code_max == 8, "min/max captured");
        CHECK(pb05_sets() == 1 && pb05_resets() == 1,
              "exactly one PB05 high and one PB05 low per sample");
        CHECK(first_write_is_pb05_high(), "PB05 raised before the burst");
        CHECK(last_write_is_pb05_low(), "PB05 lowered after the burst");
        CHECK(other_pin_writes() == 0, "no other pin writes");
        CHECK(adc_enable_calls == 1 && adc_disable_calls == 1,
              "ADC enabled then disabled around the burst");
        CHECK(adc_enabled == 0, "ADC disabled after the burst");
#if LIGHT_DARK_CALIBRATED
        CHECK(r.valid == true, "calibrated variant: valid = adc_ok && calibrated = true");
        CHECK(r.dark == ref_dark(r.mean_adc_code, true, (uint16_t)LIGHT_DARK_REF_CODE),
              "calibrated variant: dark matches independent 3*mean >= C model");
#else
        CHECK(r.valid == false, "uncalibrated: valid forced false even with 8/8 ADC success");
        CHECK(r.dark == false, "uncalibrated: dark false even with 8/8 ADC success");
#endif
    }

#if LIGHT_DARK_CALIBRATED
    /* ---- T3: exact 1/3 boundaries for this compiled C_dark ---- */
    printf("[T3] calibrated boundary mean=ceil(C/3) vs ceil(C/3)-1\n");
    {
        uint32_t c = (uint32_t)LIGHT_DARK_REF_CODE;
        uint32_t edge = ceil3(c);
        int ev = (int)edge, ev1 = (int)edge - 1;

        log_reset(); script_const(ev, 8);
        light_result_t r = light_sample();
        CHECK(r.mean_adc_code == edge, "mean equals ceil(C/3)");
        CHECK(r.valid == true, "valid at the boundary");
        CHECK(r.dark == true, "mean = ceil(C/3) -> DARK (equality counts as dark)");

        log_reset(); script_const(ev1, 8);
        r = light_sample();
        CHECK(r.mean_adc_code == (uint16_t)ev1, "mean equals ceil(C/3)-1");
        CHECK(r.dark == false, "mean = ceil(C/3)-1 -> LIGHT for the same C");
    }
#else
    /* ---- T3: uncalibrated never judges dark, never fabricates full scale ---- */
    printf("[T3] uncalibrated: no dark decision, no full-scale fallback\n");
    {
        log_reset(); script_const(4095, 8);
        light_result_t r = light_sample();
        CHECK(r.adc_ok == true && r.samples_ok == 8, "ADC path executed (8/8 ok)");
        CHECK(r.mean_adc_code == 4095, "engineering mean reported as 4095");
        CHECK(r.valid == false, "valid=false with LIGHT_DARK_CALIBRATED=0");
        CHECK(r.dark == false, "dark=false even at maximum code when uncalibrated");

        log_reset(); script_const(-1, 8);      /* every conversion times out */
        r = light_sample();
        CHECK(r.adc_ok == false && r.samples_ok == 0, "all-timeout -> adc_ok=false, 0 samples");
        CHECK(r.mean_adc_code == 0 && r.code_min == 0 && r.code_max == 0,
              "all-timeout -> mean/min/max remain 0 (no 4095 synthesis)");
        CHECK(r.valid == false && r.dark == false, "all-timeout -> valid=false, dark=false");
        CHECK(pb05_resets() == 1, "PB05 still lowered on the failure path");
        CHECK(adc_enabled == 0, "ADC still disabled on the failure path");
        CHECK(isr_poll_count <= (uint32_t)LIGHT_ADC_SAMPLES * (LIGHT_ADC_EOC_GUARD + 1u),
              "EOC polling bounded by LIGHT_ADC_EOC_GUARD per sample");
    }
#endif

    /* ---- T4: temperature-rise branch follows light validity ---- */
    printf("[T4] rise branch: light_valid/dark from the real light path\n");
    {
        light_result_t bright, darkish;
        log_reset(); script_const(0, 8);
        bright = light_sample();               /* mean 0 -> never dark */
        log_reset();
#if LIGHT_DARK_CALIBRATED
        script_const((int)ceil3((uint32_t)LIGHT_DARK_REF_CODE), 8);
#else
        script_const(4095, 8);
#endif
        darkish = light_sample();

        CHECK(sensor_gate_decide(true, 200, true, 210, bright.valid, bright.dark) == false,
              "no rise report when light is not dark");
        CHECK(sensor_gate_decide(true, 200, true, 210, darkish.valid, darkish.dark) ==
#if LIGHT_DARK_CALIBRATED
              true,
#else
              false,
#endif
              "rise report follows configuration: calibrated+dark -> report, uncalibrated -> no report");
        CHECK(sensor_gate_decide(true, 200, true, 209, darkish.valid, darkish.dark) == false,
              "exactly +0.9 C (diff=9) does not trigger the rise branch");
        CHECK(sensor_gate_decide(true, 200, true, 351, false, false) == true,
              ">35.0 C branch is independent of light validity");
        CHECK(sensor_gate_decide(false, 200, true, 400, darkish.valid, darkish.dark) == false,
              "invalid temperature sample never reports");
    }

    /* ---- T5: exhaustive 1/3 domain over the pure function ---- */
    printf("[T5] exhaustive light_is_dark domain for C_dark=1..4095\n");
    {
        uint32_t cases = 0, bad = 0, first_bad_c = 0, first_bad_mean = 0;
        bool first_bad_dark = false;
        for (uint32_t c = 1; c <= 4095u; c++) {
            uint32_t edge = ceil3(c);
            bool at_edge = light_is_dark((uint16_t)edge, true, (uint16_t)c);
            bool below   = light_is_dark((uint16_t)(edge - 1u), true, (uint16_t)c);
            cases += 2u;
            if (at_edge != true || below != false) {
                bad++;
                if (first_bad_c == 0) {
                    first_bad_c = c;
                    first_bad_mean = (at_edge != true) ? edge : (edge - 1u);
                    first_bad_dark = at_edge;
                }
            }
        }
        CHECK(bad == 0, "ceil(C/3) is dark and ceil(C/3)-1 is light for every C in 1..4095");
        if (bad != 0) {
            printf("    first mismatch: C=%lu mean=%lu dark=%d\n",
                   (unsigned long)first_bad_c, (unsigned long)first_bad_mean,
                   (int)first_bad_dark);
        }
        printf("    exhaustive assertions: %lu (mismatches: %lu)\n",
               (unsigned long)cases, (unsigned long)bad);
        CHECK(light_is_dark(4095u, false, 1u) == false &&
              light_is_dark(4095u, false, 4095u) == false,
              "valid=false never yields dark regardless of mean/C");
        CHECK(light_is_dark(0u, true, 1u) == false, "C=1, mean=0 -> light");
        CHECK(light_is_dark(1u, true, 1u) == true, "C=1, mean=1 -> dark (equality)");
        CHECK(light_is_dark(1365u, true, 4095u) == true, "C=4095, mean=1365 -> dark");
        CHECK(light_is_dark(1364u, true, 4095u) == false, "C=4095, mean=1364 -> light");
    }

    printf("==== config variant CALIBRATED=%d REF=%u: %d passed, %d failed ====\n",
           (int)LIGHT_DARK_CALIBRATED, (unsigned)LIGHT_DARK_REF_CODE, pass, fail);
    return fail ? 1 : 0;
}
