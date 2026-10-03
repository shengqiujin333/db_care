/*
 * Independent verification harness for ITEM-007 (3-minute sampling cadence).
 * Author: embedded_tester.embedded_verification.
 *
 * Compiles the REAL USER/src/main.c against an emulated MCU layer
 * (mock_main/cw32l010_*.h shadow the vendor headers) and drives the RTC minute
 * tick callback directly, verifying:
 *   - the RTC interval is configured to the vendor 1-minute value
 *   - the sample flag is raised only on every SENSOR_SAMPLE_TICKS(=3) ticks
 *   - the tick counter wraps to 0 exactly at that point (bounded 0..2)
 *   - no flag is raised for a tick without the interval interrupt pending
 *
 * Build (from gpio_input_output/):
 *   gcc -c -ffunction-sections -fdata-sections -Dmain=firmware_main_entry \
 *       -I test/mock_main -I USER/inc -I UM2005C -I COMMON USER/src/main.c -o main_ev.o
 *   gcc -c -ffunction-sections -fdata-sections -I test/mock_main -I USER/inc \
 *       -I UM2005C -I COMMON test/host_rtc_cadence_verify_ev.c -o rtc_ev.o
 *   gcc -Wl,--gc-sections main_ev.o rtc_ev.o -o rtc_verify && ./rtc_verify
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "sensor_config.h"
#include "mock_main_hw.h"

/* globals owned by main.c / measure.c */
extern uint8_t rtc_set_cnt;
uint8_t sample_flag = 0;          /* normally defined in measure.c; provided here */
uint8_t mcu_uid[10];              /* normally defined in measure.c */

/* main.c entry points under test (declared in interrupts_cw32l010.h in the real build) */
void RTC_IRQHandlerCallBack(void);
void RTC_Configuration(void);

/* stubs for the rest of main()'s dependencies (not exercised by this harness) */
void     params_init(void) { }
void     history_init(void) { }
void     optcfg_init(void) { }
void     hall_init(void) { }
bool     hall_event_pending(void) { return false; }
void     hall_event_clear(void) { }
bool     hall_debounced_active(void) { return false; }
void     optcfg_window_start(void) { }
void     optcfg_process(void) { }
uint16_t temperature_process(void) { return 0u; }
void     send_data_to_gateway(void) { }
void     go_to_sleep(void) { }

/* ---------------- emulated MCU layer ---------------- */
GPIO_TypeDef mock_gpioa, mock_gpiob;
static int      it_pending;
static uint32_t rtc_interval_set;
static int      rtc_init_calls, rtc_it_config_calls, rtc_cmd_calls, nvic_calls, lsi_calls;

ITStatus RTC_GetITState(uint32_t it) { (void)it; return it_pending ? SET : RESET; }
void     RTC_ClearITPendingBit(uint32_t it) { (void)it; it_pending = 0; }
void     RTC_Init(RTC_InitTypeDef *cfg) { (void)cfg; rtc_init_calls++; }
void     RTC_SetInterval(uint32_t interval) { rtc_interval_set = interval; }
void     RTC_ITConfig(uint32_t it, FunctionalState st) { (void)it; (void)st; rtc_it_config_calls++; }
void     RTC_Cmd(FunctionalState st) { (void)st; rtc_cmd_calls++; }
void     NVIC_EnableIRQ(int irq) { (void)irq; nvic_calls++; }
void     SYSCTRL_LSI_Enable(void) { lsi_calls++; }

/* only needed if the linker keeps more of main.c; harmless otherwise */
void     GPIO_Init(GPIO_TypeDef *p, GPIO_InitTypeDef *q) { (void)p; (void)q; }
void     GPIO_WritePin(GPIO_TypeDef *p, uint16_t pin, GPIO_PinState s) { (void)p; (void)pin; (void)s; }
GPIO_PinState GPIO_ReadPin(GPIO_TypeDef *p, uint16_t pin) { (void)p; (void)pin; return GPIO_Pin_SET; }
void     SYSCTRL_HSI_Enable(uint32_t d) { (void)d; }
void     SYSCTRL_HCLKPRS_Config(uint32_t d) { (void)d; }
void     SYSCTRL_PCLKPRS_Config(uint32_t d) { (void)d; }
void     SYSCTRL_APBPeriphClk_Enable2(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void     SYSCTRL_AHBPeriphClk_Enable(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void     SYSCTRL_APBPeriphClk_Enable1(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void     SYSCTRL_AHBPeriphReset(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void     SYSCTRL_APBPeriphReset1(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void     SYSCTRL_GotoDeepSleep(void) { }
uint32_t SYSCTRL_GetHClkFreq(void) { return 8000000u; }
uint32_t SYSCTRL_GetPClkFreq(void) { return 8000000u; }
void     UART_Init(uint32_t u, UART_InitTypeDef *c) { (void)u; (void)c; }
void     UART_SendData_8bit(uint32_t u, uint8_t d) { (void)u; (void)d; }
FlagStatus UART_GetFlagStatus(uint32_t u, uint32_t f) { (void)u; (void)f; return SET; }
void     DIGITALSIGN_GetChipUid(uint8_t uid[10]) { for (int i = 0; i < 10; i++) uid[i] = (uint8_t)i; }
void     FLASH_SetReadOutLevel(uint32_t l) { (void)l; }

static int pass, fail;
#define CHECK(c, m) do { if (c) { pass++; printf("    PASS  %s\n", m); } \
                         else   { fail++; printf("    FAIL  %s (line %d)\n", m, __LINE__); } } while (0)

/* one RTC minute tick: set the interval flag, run the callback, return whether the sample flag is set */
static int tick(void)
{
    it_pending = 1;
    RTC_IRQHandlerCallBack();
    return sample_flag != 0u;
}

int main(void)
{
    printf("==== independent RTC cadence verification (embedded_tester) ====\n");

    printf("[T1] RTC interval configuration\n");
    RTC_Configuration();
    CHECK(rtc_interval_set == RTC_INTERVAL_EVERY_1M, "RTC_SetInterval uses the vendor 1-minute interval value");
    CHECK(RTC_INTERVAL_EVERY_1M == 0x03u, "vendor RTC_INTERVAL_EVERY_1M == 0x03 (1 minute per cw32l010_rtc.h)");
    CHECK(rtc_init_calls == 1 && rtc_it_config_calls == 1 && rtc_cmd_calls == 1 && nvic_calls == 1 && lsi_calls == 1,
          "RTC initialised, interval interrupt enabled, NVIC enabled, LSI enabled");
    CHECK(SENSOR_RTC_TICK_PERIOD_MIN == 1u && SENSOR_SAMPLE_TICKS == 3u,
          "config: 1-minute tick x 3 ticks = 3 minutes");

    printf("[T2] a tick without the interval interrupt pending does nothing\n");
    it_pending = 0; sample_flag = 0; rtc_set_cnt = 0;
    RTC_IRQHandlerCallBack();
    CHECK(rtc_set_cnt == 0 && sample_flag == 0, "no counter advance and no sample flag");

    printf("[T3] first cycle: flag only on the 3rd minute tick\n");
    rtc_set_cnt = 0; sample_flag = 0;
    CHECK(tick() == 0 && rtc_set_cnt == 1, "tick 1: no flag, counter = 1");
    CHECK(tick() == 0 && rtc_set_cnt == 2, "tick 2: no flag, counter = 2");
    CHECK(tick() == 1 && rtc_set_cnt == 0, "tick 3: sample flag raised, counter wrapped to 0");

    printf("[T4] the flag is not raised again inside the next two ticks\n");
    sample_flag = 0;                       /* simulate the sampling cycle consuming the flag */
    CHECK(tick() == 0, "tick 4: no flag");
    CHECK(tick() == 0, "tick 5: no flag");
    CHECK(tick() == 1, "tick 6: flag raised again");

    printf("[T5] 30 ticks -> exactly 10 sample opportunities at ticks 3,6,...,30\n");
    rtc_set_cnt = 0; sample_flag = 0;
    { int flags = 0, at[32], n = 0, bounded_ok = 1;
      for (int i = 1; i <= 30; i++) {
          if (tick()) { flags++; if (n < 32) at[n++] = i; }
          if (rtc_set_cnt >= SENSOR_SAMPLE_TICKS) bounded_ok = 0;
          if (sample_flag) sample_flag = 0;       /* consume immediately */
      }
      CHECK(bounded_ok, "tick counter stays bounded below SENSOR_SAMPLE_TICKS throughout");
      printf("    flag ticks:");
      for (int i = 0; i < n; i++) printf(" %d", at[i]);
      printf("\n");
      CHECK(flags == 10, "exactly one sample opportunity per 3 minute ticks (10 in 30)");
      CHECK(n == 10 && at[0] == 3 && at[1] == 6 && at[9] == 30, "opportunities land on ticks 3,6,...,30"); }

    printf("[T6] one opportunity per three ticks (consumption verified in the ITEM-006 harness)\n");
    rtc_set_cnt = 0; sample_flag = 0;
    CHECK(tick() == 0 && tick() == 0 && tick() == 1, "exactly one flag per three minute ticks");
    /* The flag is consumed by the real measure.c temperature_process(), which clears it after one
       measurement; that behaviour is independently verified in the ITEM-006 flow harness (79/79).
       ISR-side generation + consumer-side clearing together give one measurement per 3 minutes. */
    CHECK(SENSOR_SAMPLE_TICKS == 3u, "3 ticks per sampling opportunity (config single point)");
    CHECK(SENSOR_RTC_TICK_PERIOD_MIN * SENSOR_SAMPLE_TICKS == 3u, "1 min x 3 ticks = 3 minutes");

    printf("==== result: %d passed, %d failed ====\n", pass, fail);
    return fail ? 1 : 0;
}
