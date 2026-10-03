/*
 * Independent verification harness for ITEM-004 (light path).
 * Author: embedded_tester.embedded_verification.
 *
 * Compiles the REAL USER/src/light.c against an emulated MCU layer
 * (mockinc/cw32l010_{gpio,adc,sysctrl}.h shadow the vendor headers) so the actual
 * sampling sequence can be exercised on the host:
 *   PB05 high -> settle delay -> N ADC conversions on PB04/AIN11 -> average
 *   -> hysteresis DARK/LIT -> PB05 low.
 *
 * The hysteresis truth table is additionally checked directly on the real
 * light_code_is_dark() function.
 *
 * Build (from gpio_input_output/):
 *   gcc -std=c11 -Wall -Wextra -I <mockinc> -I USER/inc -I COMMON \
 *       test/host_light_verify_ev.c USER/src/light.c -o light_verify
 * (<mockinc> must precede any vendor include path; no vendor headers are used.)
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "light.h"
#include "fw_core.h"      /* light_code_is_dark moved here in ITEM-005 */
#include "sensor_config.h"
#include "delay.h"
#include "mock_cw32.h"

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
    printf("==== independent light path verification (embedded_tester) ====\n");
    mock_adc.isr_get = isr_get;   /* wire the counting ISR getter used by CW_ADC->ISR */

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

    /* ---- T2: LIT sample sequence ---- */
    printf("[T2] sample with bright reading (LIT)\n");
    log_reset(); script_const(100, LIGHT_ADC_SAMPLES);
    bool dark = light_sample();
    CHECK(dark == false, "code=100 -> LIT (false)");
    CHECK(light_last_code() == 100, "averaged code exposed as 100");
    CHECK(pb05_sets() == 1 && pb05_resets() == 1, "exactly one PB05 high and one PB05 low per sample");
    CHECK(first_write_is_pb05_high(), "PB05 raised BEFORE sampling");
    CHECK(last_write_is_pb05_low(), "PB05 lowered AFTER sampling");
    CHECK(other_pin_writes() == 0, "no other pin writes during sampling");
    CHECK(delay_n == 1 && delay_log[0] == LIGHT_SETTLE_MS, "settle delay of the configured 100 ms before sampling");
    CHECK(conv_calls == LIGHT_ADC_SAMPLES, "exactly LIGHT_ADC_SAMPLES(8) conversions per sample");
    CHECK(adc_enable_calls == 1 && adc_disable_calls == 1, "ADC enabled then disabled around the burst");
    CHECK(adc_enabled == 0, "ADC disabled after the burst");

    /* ---- T3: DARK sample ---- */
    printf("[T3] sample with dark reading (DARK)\n");
    log_reset(); script_const(400, LIGHT_ADC_SAMPLES);
    dark = light_sample();
    CHECK(dark == true, "code=400 (>= ENTER 350) -> DARK");
    CHECK(light_last_code() == 400, "averaged code 400");

    /* ---- T4: averaging of multiple samples ---- */
    printf("[T4] arithmetic mean of the burst\n");
    { const int v[8] = {0,4095,0,4095,0,4095,0,4095};
      log_reset(); script_set(v, LIGHT_ADC_SAMPLES);
      dark = light_sample();
      CHECK(light_last_code() == 2047, "mean of 4x0 + 4x4095 = 2047 (integer)");
      CHECK(dark == true, "decision uses the mean (2047 >= 350 -> DARK)");
      CHECK(conv_calls == 8, "all 8 samples used"); }

    /* ---- T5: hysteresis state machine through light_sample ---- */
    printf("[T5] hysteresis across successive samples\n");
    light_reset_state();
    log_reset(); script_const(400, 8);
    CHECK(light_sample() == true, "reset -> first sample 400 -> DARK");
    log_reset(); script_const(300, 8);
    CHECK(light_sample() == true, "DARK then code=300 (inside band) stays DARK");
    log_reset(); script_const(250, 8);
    CHECK(light_sample() == false, "code=250 (<= EXIT) turns LIT");
    log_reset(); script_const(300, 8);
    CHECK(light_sample() == false, "LIT then code=300 (< ENTER) stays LIT");
    log_reset(); script_const(350, 8);
    CHECK(light_sample() == true, "code=350 (= ENTER) turns DARK");
    log_reset(); script_const(349, 8);
    CHECK(light_sample() == true, "code=349 (> EXIT) stays DARK");

    /* ---- T6: partial conversion timeout -> average of successful only ---- */
    printf("[T6] partial EOC timeout\n");
    { const int v[8] = {-1,-1,1000,1000,1000,1000,1000,1000};
      light_reset_state();
      log_reset(); script_set(v, 8);
      dark = light_sample();
      CHECK(light_last_code() == 1000, "average over the 6 successful conversions only");
      CHECK(dark == true, "decision from the successful average");
      CHECK(conv_calls == 8, "a conversion was attempted for every slot"); }

    /* ---- T7: all conversions time out -> full scale -> DARK, bounded ---- */
    printf("[T7] all EOC timeouts (open/failed divider)\n");
    log_reset(); script_const(-1, 8);
    dark = light_sample();
    CHECK(dark == true, "all-timeout falls back to full scale -> DARK (tends to report, never blocks)");
    CHECK(light_last_code() == LIGHT_ADC_FULL_SCALE, "fallback code = 4095");
    CHECK(last_write_is_pb05_low(), "PB05 still lowered on the failure path");
    CHECK(adc_enabled == 0, "ADC disabled on the failure path");
    printf("    poll count = %lu (bounded guard, %lu..%lu expected)\n",
           (unsigned long)isr_poll_count,
           (unsigned long)(LIGHT_ADC_SAMPLES * LIGHT_ADC_EOC_GUARD),
           (unsigned long)(LIGHT_ADC_SAMPLES * (LIGHT_ADC_EOC_GUARD + 1u)));
    CHECK(isr_poll_count >= (uint32_t)LIGHT_ADC_SAMPLES * LIGHT_ADC_EOC_GUARD &&
          isr_poll_count <= (uint32_t)LIGHT_ADC_SAMPLES * (LIGHT_ADC_EOC_GUARD + 1u),
          "polling is bounded by LIGHT_ADC_EOC_GUARD per conversion (no infinite wait)");

    /* ---- T8: light_reset_state clears the hysteresis state ---- */
    printf("[T8] light_reset_state\n");
    light_reset_state();
    log_reset(); script_const(300, 8);
    CHECK(light_sample() == false, "after reset the first sample uses the ENTER threshold (300 -> LIT)");
    log_reset(); script_const(350, 8);
    CHECK(light_sample() == true, "then 350 -> DARK");

    /* ---- T9: direct truth table on the real pure function ---- */
    printf("[T9] light_code_is_dark truth table\n");
    CHECK(light_code_is_dark(0u, false) == false && light_code_is_dark(349u, false) == false, "LIT: 0/349 -> LIT");
    CHECK(light_code_is_dark(350u, false) == true && light_code_is_dark(4095u, false) == true, "LIT: 350/4095 -> DARK");
    CHECK(light_code_is_dark(251u, true) == true && light_code_is_dark(250u, true) == false, "DARK: 251 stays / 250 turns LIT");
    CHECK(light_code_is_dark(300u, true) == true && light_code_is_dark(300u, false) == false, "inside band keeps previous state");

    printf("==== result: %d passed, %d failed ====\n", pass, fail);
    return fail ? 1 : 0;
}
