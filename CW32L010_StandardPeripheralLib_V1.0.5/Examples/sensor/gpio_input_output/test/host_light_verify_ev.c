/*
 * Independent verification harness for the light path (ITEM-004 / LIGHT-DEF-2).
 * Author: embedded_tester.embedded_verification.
 *
 * Compiles the REAL USER/src/light.c against an emulated MCU layer
 * (test/mock_mcu/cw32l010_{gpio,adc,sysctrl}.h shadow the vendor headers) so the
 * actual sampling sequence is exercised on the host:
 *   PB05 high -> settle delay -> N ADC conversions on PB04/AIN11 -> average
 *   -> validity gate/dark decision -> PB05 low.
 *
 * FD-002 rev 8 semantics under test:
 *   valid = adc_ok && (LIGHT_DARK_THRESHOLD_CODE != 0)
 *   dark  = valid && mean_adc_code >= LIGHT_DARK_THRESHOLD_CODE   (equality counts)
 * The delivery constant is 0 (criterion disabled). The harness is also built
 * with -DLIGHT_DARK_THRESHOLD_CODE=3000 to exercise the enabled wiring.
 * There is no hysteresis, no state and no full-scale fallback on ADC timeout.
 *
 * Build (from gpio_input_output/):
 *   gcc -std=c11 -Wall -Wextra -I test/mock_mcu -I USER/inc -I COMMON \
 *       test/host_light_verify_ev.c USER/src/light.c USER/src/fw_core.c \
 *       -o light_verify
 *   gcc ... -DLIGHT_DARK_THRESHOLD_CODE=3000u ... -o light_verify_enabled
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "light.h"
#include "fw_core.h"
#include "sensor_config.h"
#include "delay.h"
#include "mock_cw32.h"

#define THRESH_CODE ((int)LIGHT_DARK_THRESHOLD_CODE)

/* ---------------- mock MCU state ---------------- */
GPIO_TypeDef mock_gpiob;
mock_adc_regs_t mock_adc;

static int pin_write_pin[64], pin_write_val[64], pin_write_n;
static int gpio_mode[8];                       /* mode recorded per pin bit index */
static int gpio_init_calls;
static ADC_InitTypeDef adc_cfg; static int adc_init_calls;
static int adc_enable_calls, adc_disable_calls, adc_enabled;
static int conv_calls;
static uint16_t conv_value;
static uint32_t isr_poll_count;
static uint16_t delay_log[64]; static int delay_n;

static int script[16]; static int script_n, script_i;   /* <0 = EOC never set (timeout) */

uint32_t isr_get(void) { isr_poll_count++; return mock_adc.isr; }
void delay_ms(uint16_t ms) { if (delay_n < 64) delay_log[delay_n++] = ms; }

void GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *cfg)
{
    (void)port; gpio_init_calls++;
    for (int b = 0; b < 8; b++) if (cfg->Pins & (1u << b)) gpio_mode[b] = (int)cfg->Mode;
}
void GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state)
{
    (void)port;
    if (pin_write_n < 64) { pin_write_pin[pin_write_n] = pin; pin_write_val[pin_write_n] = (int)state; pin_write_n++; }
}
void ADC_Init(ADC_InitTypeDef *cfg) { adc_cfg = *cfg; adc_init_calls++; }
void ADC_ClearITPendingAll(void) { mock_adc.isr = 0; }
void ADC_Disable(void) { adc_disable_calls++; adc_enabled = 0; }
ErrorStatus ADC_Enable(void) { adc_enable_calls++; adc_enabled = 1; return SUCCESS; }
void ADC_ClearITPendingBit(uint16_t it) { (void)it; mock_adc.isr = 0; }
uint16_t ADC_GetConversionValue(uint32_t idx) { (void)idx; return conv_value; }

void ADC_SoftwareStartConvCmd(FunctionalState state)
{
    (void)state;
    conv_calls++;
    mock_adc.isr = 0;
    if (script_i < script_n) {
        if (script[script_i] >= 0) { conv_value = (uint16_t)script[script_i]; mock_adc.isr = ADC_ISR_EOC_Msk; }
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
static int pb05_sets(void)   { int c = 0; for (int i = 0; i < pin_write_n; i++) if (pin_write_pin[i] == GPIO_PIN_5 && pin_write_val[i] == 1) c++; return c; }
static int pb05_resets(void) { int c = 0; for (int i = 0; i < pin_write_n; i++) if (pin_write_pin[i] == GPIO_PIN_5 && pin_write_val[i] == 0) c++; return c; }
static int other_pin_writes(void) { int c = 0; for (int i = 0; i < pin_write_n; i++) if (pin_write_pin[i] != GPIO_PIN_5) c++; return c; }
static int first_write_is_pb05_high(void) { return pin_write_n > 0 && pin_write_pin[0] == GPIO_PIN_5 && pin_write_val[0] == 1; }
static int last_write_is_pb05_low(void)   { return pin_write_n > 0 && pin_write_pin[pin_write_n-1] == GPIO_PIN_5 && pin_write_val[pin_write_n-1] == 0; }

static int pass, fail;
#define CHECK(c, m) do { if (c) { pass++; printf("    PASS  %s\n", m); } \
                         else   { fail++; printf("    FAIL  %s (line %d)\n", m, __LINE__); } } while (0)

int main(void)
{
    printf("==== independent light path verification (embedded_tester, LIGHT-DEF-2) ====\n");
    printf("     delivery LIGHT_DARK_THRESHOLD_CODE = %d (%s)\n",
           THRESH_CODE, THRESH_CODE == 0 ? "criterion DISABLED" : "criterion ENABLED");
    mock_adc.isr_get = isr_get;

    /* ---- T1: init configures PB05 output / PB04 analog and the ADC ---- */
    printf("[T1] light_init pin ownership and ADC configuration\n");
    for (int i = 0; i < 8; i++) gpio_mode[i] = -1;
    light_init();
    CHECK(gpio_mode[5] == GPIO_MODE_OUTPUT_PP, "PB05 configured as push-pull output (light power)");
    CHECK(gpio_mode[4] == GPIO_MODE_ANALOG, "PB04 configured as analog input (AIN11)");
    CHECK(gpio_mode[3] == -1 && gpio_mode[6] == -1, "no other pin configured by the light module");
    CHECK(pin_write_n >= 1 && pin_write_pin[0] == GPIO_PIN_5 && pin_write_val[0] == 0,
          "PB05 driven low at init (no divider current outside sampling)");
    CHECK(adc_init_calls == 1, "ADC_Init called once");
    CHECK(adc_cfg.ADC_IN0.ADC_InputChannel == ADC_InputCH11, "ADC channel = AIN11 (PB04)");
    CHECK(adc_cfg.ADC_ClkDiv == ADC_Clk_Div8, "ADCCLK = PCLK/8 (1 MHz)");
    CHECK(adc_cfg.ADC_IN0.ADC_SampTime == ADC_SampTime390Clk, "390 clk sample time (5 MOhm source)");
    CHECK(adc_cfg.ADC_ConvertMode == ADC_ConvertMode_Once, "single conversion mode");
    CHECK(adc_enabled == 0, "ADC left disabled after init");

    /* ---- T2: sampling sequence with a valid reading ---- */
    printf("[T2] sampling sequence and delivery-configured validity gate\n");
    log_reset(); script_const(100, LIGHT_ADC_SAMPLES);
    light_result_t r = light_sample();
    CHECK(r.adc_ok == true && r.samples_ok == LIGHT_ADC_SAMPLES, "all 8 conversions succeed");
    CHECK(r.mean_adc_code == 100u && r.code_min == 100u && r.code_max == 100u,
          "mean/min/max of the scripted burst = 100");
    CHECK(pb05_sets() == 1 && pb05_resets() == 1, "exactly one PB05 high and one PB05 low per sample");
    CHECK(first_write_is_pb05_high(), "PB05 raised BEFORE sampling");
    CHECK(last_write_is_pb05_low(), "PB05 lowered AFTER sampling");
    CHECK(other_pin_writes() == 0, "no other pin writes during sampling");
    CHECK(delay_n == 1 && delay_log[0] == LIGHT_SETTLE_MS, "settle delay of the configured 100 ms before sampling");
    CHECK(conv_calls == LIGHT_ADC_SAMPLES, "exactly LIGHT_ADC_SAMPLES(8) conversions per sample");
    CHECK(adc_enable_calls == 1 && adc_disable_calls == 1, "ADC enabled then disabled around the burst");
    CHECK(adc_enabled == 0, "ADC disabled after the burst");
    if (THRESH_CODE == 0) {
        CHECK(r.valid == false && r.dark == false,
              "T=0: adc_ok sample is NOT valid and NOT dark (criterion disabled)");
    } else {
        CHECK(r.valid == true && r.dark == (r.mean_adc_code >= (uint16_t)THRESH_CODE),
              "T!=0: valid=true and dark follows mean >= T");
    }

    /* ---- T3: arithmetic mean and min/max over the burst ---- */
    printf("[T3] arithmetic mean / min / max of the burst\n");
    { const int v[8] = {0,4095,0,4095,0,4095,0,4095};
      log_reset(); script_set(v, LIGHT_ADC_SAMPLES);
      r = light_sample();
      CHECK(r.adc_ok && r.samples_ok == 8, "all 8 conversions used");
      CHECK(r.mean_adc_code == 2047u, "mean of 4x0 + 4x4095 = 2047 (integer)");
      CHECK(r.code_min == 0u && r.code_max == 4095u, "min/max come from the successful samples");
      if (THRESH_CODE != 0) {
          CHECK(r.dark == (2047u >= (uint16_t)THRESH_CODE), "decision uses the mean, threshold form");
      } else {
          CHECK(r.valid == false && r.dark == false, "T=0 stays not-valid/not-dark regardless of mean");
      } }

    /* ---- T4: partial conversion timeout -> average of successful only ---- */
    printf("[T4] partial EOC timeout\n");
    { const int v[8] = {-1,-1,1000,1000,1000,1000,1000,1000};
      log_reset(); script_set(v, 8);
      r = light_sample();
      CHECK(r.adc_ok == true && r.samples_ok == 6u, "6 successful samples recorded (not 8)");
      CHECK(r.mean_adc_code == 1000u, "average over the successful conversions only");
      CHECK(r.code_min == 1000u && r.code_max == 1000u, "min/max from the successful samples");
      CHECK(conv_calls == 8, "a conversion was attempted for every slot");
      CHECK(last_write_is_pb05_low() && adc_enabled == 0, "PB05/ADC restored on the partial-failure path");
      if (THRESH_CODE == 0) {
          CHECK(r.valid == false && r.dark == false, "T=0 keeps the gate closed");
      } else {
          CHECK(r.valid == true && r.dark == (1000u >= (uint16_t)THRESH_CODE), "threshold applied to the mean");
      } }

    /* ---- T5: all conversions time out -> no full-scale synthesis, bounded ---- */
    printf("[T5] all EOC timeouts (open/failed divider)\n");
    log_reset(); script_const(-1, 8);
    r = light_sample();
    CHECK(r.adc_ok == false && r.samples_ok == 0u, "no successful conversion");
    CHECK(r.mean_adc_code == 0u && r.code_min == 0u && r.code_max == 0u,
          "mean/min/max stay 0 (no 4095 full-scale synthesis)");
    CHECK(r.valid == false && r.dark == false, "invalid sample is never valid/dark");
    CHECK(last_write_is_pb05_low(), "PB05 still lowered on the failure path");
    CHECK(adc_enabled == 0, "ADC disabled on the failure path");
    printf("    poll count = %lu (bounded guard, %lu..%lu expected)\n",
           (unsigned long)isr_poll_count,
           (unsigned long)(LIGHT_ADC_SAMPLES * LIGHT_ADC_EOC_GUARD),
           (unsigned long)(LIGHT_ADC_SAMPLES * (LIGHT_ADC_EOC_GUARD + 1u)));
    CHECK(isr_poll_count >= (uint32_t)LIGHT_ADC_SAMPLES * LIGHT_ADC_EOC_GUARD &&
          isr_poll_count <= (uint32_t)LIGHT_ADC_SAMPLES * (LIGHT_ADC_EOC_GUARD + 1u),
          "polling is bounded by LIGHT_ADC_EOC_GUARD per conversion (no infinite wait)");

    /* ---- T6: stateless per-sample decision (no hysteresis / no history) ---- */
    printf("[T6] stateless decision across successive samples\n");
    if (THRESH_CODE != 0) {
        log_reset(); script_const((int)THRESH_CODE, 8);
        r = light_sample();
        CHECK(r.valid && r.dark, "mean == T -> dark (equality counts)");
        log_reset(); script_const((int)THRESH_CODE - 1, 8);
        r = light_sample();
        CHECK(r.valid && !r.dark, "mean == T-1 -> lit");
        log_reset(); script_const((int)THRESH_CODE - 1, 8);
        r = light_sample();
        CHECK(r.valid && !r.dark, "repeating an in-band value never latches a previous state");
        log_reset(); script_const((int)THRESH_CODE, 8);
        r = light_sample();
        CHECK(r.valid && r.dark, "repeating T -> dark again (per-sample, no history)");
    } else {
        log_reset(); script_const(4095, 8);
        r = light_sample();
        CHECK(r.dark == false, "T=0: full-scale mean is still not dark (disabled criterion)");
        log_reset(); script_const(0, 8);
        r = light_sample();
        CHECK(r.dark == false, "T=0: zero mean is still not dark");
        CHECK(r.adc_ok == true && r.samples_ok == 8, "conversions still recorded when the criterion is disabled");
    }

    printf("==== result: %d passed, %d failed ====\n", pass, fail);
    return fail ? 1 : 0;
}
