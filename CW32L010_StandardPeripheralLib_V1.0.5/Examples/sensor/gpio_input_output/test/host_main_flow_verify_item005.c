/*
 * host_main_flow_verify_item005.c
 *
 * 独立验证 harness (embedded_tester.embedded_verification) for ITEM-005:
 *   - main loop executes exactly ONE complete transaction per sampling opportunity:
 *     temperature_process -> send_data_to_gateway -> single S (trace) -> UART close -> sleep
 *   - BOOT/VDD/IOTEST/IOSIG/BUS happen before the UID-zero fatal branch
 *   - UID sum == 0: clears sample_flag/report_req, calls light_init() (PB05 low / ADC off),
 *     closes UART, then enters a real deep-sleep loop; no sampling/sending/spinning
 *   - the loop does not rely on additional rounds to complete RF retry
 *
 * Compiles the REAL USER/src/main.c unchanged (renamed entry with
 * -Dmain=firmware_main_entry) against the emulated MCU layer in test/mock_main_ev.
 * Every product dependency below is a recording stub; no product logic is replaced
 * except the entry points main() itself calls (temperature_process/send/trace/sleep).
 *
 * Build (from gpio_input_output/):
 *   gcc -std=c11 -Wall -Wextra -DSENSOR_DEBUG_UART=1 -Dmain=firmware_main_entry \
 *       -I test/mock_main_ev -I USER/inc -I UM2005C -I COMMON \
 *       test/host_main_flow_verify_item005.c USER/src/main.c -o harness.exe
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <setjmp.h>
#include "main.h"
#include "sensor_config.h"
#include "mock_main_hw.h"
#include "debug_trace.h"
#include "light.h"

/* ---------------- product globals normally owned by measure.c ---------------- */
uint8_t  sample_flag = 0;
uint8_t  report_req  = 0;
uint8_t  mcu_uid[10] = {0};

/* ---------------- event recorder ---------------- */
enum {
    EV_BOOT = 1, EV_VDD, EV_IOTEST, EV_IOSIG, EV_BUS,
    EV_TEMP, EV_SEND, EV_TRACE_FETCH, EV_S, EV_CLOSE, EV_GO_SLEEP, EV_DEEP_SLEEP,
    EV_LIGHT_INIT
};

static const char *ev_name(int e)
{
    switch (e) {
    case EV_BOOT: return "BOOT";
    case EV_VDD: return "VDD";
    case EV_IOTEST: return "IOTEST";
    case EV_IOSIG: return "IOSIG";
    case EV_BUS: return "BUS";
    case EV_TEMP: return "TEMPERATURE_PROCESS";
    case EV_SEND: return "SEND_DATA_TO_GATEWAY";
    case EV_TRACE_FETCH: return "SENSOR_TRACE_FETCH";
    case EV_S: return "S_LINE";
    case EV_CLOSE: return "DEBUG_TRACE_FLUSH_CLOSE";
    case EV_GO_SLEEP: return "GO_TO_SLEEP";
    case EV_DEEP_SLEEP: return "SYSCTRL_GotoDeepSleep";
    case EV_LIGHT_INIT: return "LIGHT_INIT";
    default: return "?";
    }
}

#define MAXEV 512
static int ev_seq[MAXEV];
static int ev_n;
static int ev_count[32];
static int ev_index[MAXEV];   /* event kind -> first sequence position (0 = absent) */

static void rec(int kind)
{
    if (ev_n < MAXEV) {
        ev_seq[ev_n] = kind;
        ev_n++;
    }
    if (kind >= 0 && kind < (int)(sizeof(ev_count) / sizeof(ev_count[0]))) {
        ev_count[kind]++;
        if (ev_index[kind] == 0) {
            ev_index[kind] = ev_n;
        }
    }
}

static void dump_events(void)
{
    int i;
    printf("EVENT_SEQUENCE");
    for (i = 0; i < ev_n; i++) {
        printf(" %s", ev_name(ev_seq[i]));
    }
    printf("\n");
    printf("EVENT_COUNTS");
    for (i = 1; i < (int)(sizeof(ev_count) / sizeof(ev_count[0])); i++) {
        if (ev_count[i] != 0) {
            printf(" %s=%d", ev_name(i), ev_count[i]);
        }
    }
    printf("\n");
}

/* ---------------- run control (setjmp out of the product infinite loop) ---------------- */
extern int32_t firmware_main_entry(void);

static jmp_buf  exit_jmp;
static int      deep_sleep_limit = 0;
static int      deep_sleep_count = 0;
static int      go_sleep_limit = 0;
static int      go_sleep_count = 0;
static int      schedule[64];     /* 1 = raise sample_flag at that go_to_sleep call */
static int      run_uid_zero = 0;

/* ---------------- trace stubs (SENSOR_DEBUG_UART=1) ---------------- */
void debug_trace_boot(const uint8_t *uid, uint32_t reset_flags)
{
    (void)uid; (void)reset_flags;
    rec(EV_BOOT);
}
void debug_trace_vdd(const debug_trace_vdd_t *v) { (void)v; rec(EV_VDD); }
void debug_trace_iotest(const debug_trace_iotest_t *t) { (void)t; rec(EV_IOTEST); }
void debug_trace_iosig(const debug_trace_iosig_t *s) { (void)s; rec(EV_IOSIG); }
void debug_trace_bus(const debug_trace_bus_t *b) { (void)b; rec(EV_BUS); }
void debug_trace_sample(const debug_trace_sample_t *s) { (void)s; rec(EV_S); }
void debug_trace_flush_close(void) { rec(EV_CLOSE); }

/* ---------------- product entry points used by main() ---------------- */
uint16_t temperature_process(void)
{
    rec(EV_TEMP);
    /* the real measure.c consumes the flag at the end of the measurement */
    sample_flag = 0u;
    return 0u;
}

void send_data_to_gateway(void)
{
    rec(EV_SEND);
    /* one main-loop call; internal 1..3 attempts belong to ITEM-003 */
    report_req = 0u;
}

uint8_t sensor_trace_fetch(debug_trace_sample_t *out)
{
    rec(EV_TRACE_FETCH);
    if (out != NULL) {
        out->tick = 0u;
        out->sample_ok = 0u;
        out->send = 0u;
    }
    return 1u;
}

void light_init(void)
{
    rec(EV_LIGHT_INIT);
}

void go_to_sleep(void)
{
    rec(EV_GO_SLEEP);
    go_sleep_count++;
    if (go_sleep_limit > 0 && go_sleep_count >= go_sleep_limit) {
        longjmp(exit_jmp, 1);   /* bounded observation: a spin is reported, not a hang */
    }
    if (go_sleep_count < (int)(sizeof(schedule) / sizeof(schedule[0])) && schedule[go_sleep_count] != 0) {
        sample_flag = 1u;   /* simulate an RTC sampling opportunity between sleeps */
    }
    /*
     * The real go_to_sleep() only calls SYSCTRL_GotoDeepSleep() when both
     * report_req and sample_flag are clear. main() calls it on every loop round,
     * so mirror that condition here to expose any busy loop.
     */
    if ((report_req == 0u) && (sample_flag == 0u)) {
        SYSCTRL_GotoDeepSleep();
    }
}

/* ---------------- emulated MCU layer ---------------- */
GPIO_TypeDef mock_gpioa, mock_gpiob;

void SYSCTRL_GotoDeepSleep(void)
{
    rec(EV_DEEP_SLEEP);
    deep_sleep_count++;
    if (deep_sleep_limit > 0 && deep_sleep_count >= deep_sleep_limit) {
        longjmp(exit_jmp, 1);
    }
}

void     RTC_Init(RTC_InitTypeDef *cfg) { (void)cfg; }
void     RTC_SetInterval(uint32_t interval) { (void)interval; }
void     RTC_ITConfig(uint32_t it, FunctionalState st) { (void)it; (void)st; }
void     RTC_Cmd(FunctionalState st) { (void)st; }
ITStatus RTC_GetITState(uint32_t it) { (void)it; return RESET; }
void     RTC_ClearITPendingBit(uint32_t it) { (void)it; }
void     NVIC_EnableIRQ(int irq) { (void)irq; }

void     GPIO_Init(GPIO_TypeDef *p, GPIO_InitTypeDef *q) { (void)p; (void)q; }
void     GPIO_WritePin(GPIO_TypeDef *p, uint16_t pin, GPIO_PinState s) { (void)p; (void)pin; (void)s; }
GPIO_PinState GPIO_ReadPin(GPIO_TypeDef *p, uint16_t pin) { (void)p; (void)pin; return GPIO_Pin_SET; }

void     SYSCTRL_HSI_Enable(uint32_t d) { (void)d; }
void     SYSCTRL_LSI_Enable(void) { }
void     SYSCTRL_HCLKPRS_Config(uint32_t d) { (void)d; }
void     SYSCTRL_PCLKPRS_Config(uint32_t d) { (void)d; }
void     SYSCTRL_APBPeriphClk_Enable2(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void     SYSCTRL_AHBPeriphClk_Enable(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void     SYSCTRL_APBPeriphClk_Enable1(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void     SYSCTRL_AHBPeriphReset(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void     SYSCTRL_APBPeriphReset1(uint32_t p, FunctionalState s) { (void)p; (void)s; }
uint32_t SYSCTRL_GetHClkFreq(void) { return 8000000u; }
uint32_t SYSCTRL_GetPClkFreq(void) { return 8000000u; }
uint32_t SYSCTRL_GetAllRstFlag(void) { return 0x0004u; }

void     UART_Init(uint32_t u, UART_InitTypeDef *c) { (void)u; (void)c; }
void     UART_SendData_8bit(uint32_t u, uint8_t d) { (void)u; (void)d; }
FlagStatus UART_GetFlagStatus(uint32_t u, uint32_t f) { (void)u; (void)f; return SET; }

void     DIGITALSIGN_GetChipUid(uint8_t uid[10])
{
    int i;
    if (run_uid_zero) {
        for (i = 0; i < 10; i++) uid[i] = 0u;
    } else {
        for (i = 0; i < 10; i++) uid[i] = (uint8_t)(i + 1);
    }
}
void     FLASH_SetReadOutLevel(uint32_t level) { (void)level; }

/* ---------------- diagnostic stubs of the power-on scans ---------------- */
uint8_t light_read_vdd_mv(uint16_t *mv, uint16_t *code, uint16_t *bgr)
{
    if (mv) *mv = 3300u;
    if (code) *code = 1480u;
    if (bgr) *bgr = 1189u;
    return 1u;
}
uint8_t sensor_io_diag_scan(debug_trace_iotest_t *out) { (void)out; return 1u; }
uint8_t sensor_io_sig_scan(debug_trace_iosig_t *out) { (void)out; return 1u; }
uint8_t sensor_bus_diag_scan(debug_trace_bus_t *out) { (void)out; return 1u; }

/* ---------------- checks ---------------- */
static int pass, fail;
#define CHECK(c, m) do { if (c) { pass++; printf("PASS  %s\n", m); } \
                         else   { fail++; printf("FAIL  %s (line %d)\n", m, __LINE__); } } while (0)

static int count_between(int from_exclusive, int to_exclusive, int kind)
{
    int i, n = 0;
    for (i = from_exclusive; i < to_exclusive && i < ev_n; i++) {
        if (ev_seq[i] == kind) n++;
    }
    return n;
}

static int first_pos(int kind)
{
    int i;
    for (i = 0; i < ev_n; i++) {
        if (ev_seq[i] == kind) return i + 1;
    }
    return 0;
}

static void reset_run(void)
{
    ev_n = 0;
    memset(ev_count, 0, sizeof(ev_count));
    memset(ev_index, 0, sizeof(ev_index));
    memset(schedule, 0, sizeof(schedule));
    deep_sleep_count = 0;
    go_sleep_count = 0;
    go_sleep_limit = 0;
    deep_sleep_limit = 0;
    sample_flag = 0u;
    report_req = 0u;
}

static void run_main(void)
{
    if (setjmp(exit_jmp) == 0) {
        (void)firmware_main_entry();
    }
}

int main(void)
{
    printf("==== independent main-flow / UID-zero verification (embedded_tester) ====\n");
    printf("CONFIG SENSOR_DEBUG_UART=%d SENSOR_SAMPLE_TICKS=%u SENSOR_RTC_TICK_PERIOD_MIN=%u\n",
           (int)SENSOR_DEBUG_UART, (unsigned)SENSOR_SAMPLE_TICKS,
           (unsigned)SENSOR_RTC_TICK_PERIOD_MIN);

    /* ================= Scenario 1: UID zero fatal path ================= */
    printf("\n[UID-ZERO] UID sum == 0, sample_flag/report_req pre-set to 1\n");
    reset_run();
    run_uid_zero = 1;
    sample_flag = 1u;
    report_req = 1u;
    deep_sleep_limit = 5;
    go_sleep_limit = 50;
    run_main();
    dump_events();

    CHECK(ev_count[EV_BOOT] == 1, "BOOT line emitted once");
    CHECK(ev_count[EV_VDD] == 1 && ev_count[EV_IOTEST] == 1 &&
          ev_count[EV_IOSIG] == 1 && ev_count[EV_BUS] == 1,
          "power-on diagnostics emitted once each");
    CHECK(ev_count[EV_LIGHT_INIT] == 1, "light_init() called exactly once on the UID-zero path");
    CHECK(ev_count[EV_CLOSE] == 1, "UART closed exactly once on the UID-zero path");
    CHECK(ev_count[EV_TEMP] == 0, "no temperature_process() on the UID-zero path");
    CHECK(ev_count[EV_SEND] == 0, "no send_data_to_gateway() on the UID-zero path");
    CHECK(ev_count[EV_TRACE_FETCH] == 0 && ev_count[EV_S] == 0,
          "no sample trace / no S line on the UID-zero path");
    CHECK(ev_count[EV_GO_SLEEP] == 0, "go_to_sleep() (which can spin on flags) not used");
    CHECK(ev_count[EV_DEEP_SLEEP] == 5, "deep sleep entered repeatedly (5/5 bounded observations)");
    CHECK(sample_flag == 0u, "sample_flag cleared on the UID-zero path");
    CHECK(report_req == 0u, "report_req cleared on the UID-zero path");
    CHECK(ev_count[EV_DEEP_SLEEP] > 0 &&
          count_between(first_pos(EV_DEEP_SLEEP) - 1, ev_n, EV_TEMP) == 0 &&
          count_between(first_pos(EV_DEEP_SLEEP) - 1, ev_n, EV_SEND) == 0,
          "after entering the fatal deep-sleep loop no business call occurs (no spin)");
    CHECK(first_pos(EV_BOOT) > 0 && first_pos(EV_BOOT) < first_pos(EV_LIGHT_INIT),
          "BOOT precedes the UID-zero branch action");
    CHECK(first_pos(EV_BUS) > 0 && first_pos(EV_BUS) < first_pos(EV_LIGHT_INIT) &&
          first_pos(EV_BUS) < first_pos(EV_DEEP_SLEEP),
          "all power-on diagnostics precede the fatal branch");

    /* ================= Scenario 2: one complete transaction per opportunity ================= */
    printf("\n[NORMAL] initial opportunity + 1 scheduled opportunity; 6 idle rounds\n");
    reset_run();
    run_uid_zero = 0;
    sample_flag = 1u;          /* power-on immediate sampling opportunity (measure.c initial value) */
    deep_sleep_limit = 8;
    go_sleep_limit = 50;
    schedule[2] = 1;           /* raise a new opportunity at the 2nd go_to_sleep() */
    run_main();
    dump_events();

    CHECK(ev_count[EV_TEMP] == 2, "one temperature_process() per opportunity (2)");
    CHECK(ev_count[EV_SEND] == 2, "one send_data_to_gateway() per opportunity (2)");
    CHECK(ev_count[EV_TRACE_FETCH] == 2, "one trace fetch per opportunity (2)");
    CHECK(ev_count[EV_S] == 2, "exactly one S line per opportunity (2)");
    CHECK(ev_count[EV_GO_SLEEP] == 9, "one go_to_sleep() per main-loop round (9 rounds)");
    CHECK(ev_count[EV_DEEP_SLEEP] == 8,
          "deep sleep on every round whose flags are clear (8 of 9; one round raised the next opportunity)");

    /* order inside transaction 1: TEMP, SEND, FETCH, S, CLOSE, GO_SLEEP, DEEP */
    {
        int p1 = first_pos(EV_TEMP), p2 = first_pos(EV_SEND), p3 = first_pos(EV_TRACE_FETCH),
            p4 = first_pos(EV_S), p5 = first_pos(EV_CLOSE), p6 = first_pos(EV_GO_SLEEP),
            p7 = first_pos(EV_DEEP_SLEEP);
        CHECK(p1 > 0 && p1 < p2 && p2 < p3 && p3 < p4 && p4 < p5 && p5 < p6 && p6 < p7,
              "transaction order: temp -> send -> trace -> S -> close -> sleep -> deep sleep");
    }

    /* after the last transaction, the remaining idle rounds must not re-send or re-sample */
    {
        int last_s = 0, i;
        for (i = 0; i < ev_n; i++) {
            if (ev_seq[i] == EV_S) last_s = i;
        }
        CHECK(last_s > 0 && count_between(last_s, ev_n, EV_SEND) == 0 &&
              count_between(last_s, ev_n, EV_TEMP) == 0,
              "idle rounds after the last opportunity perform no extra send/measure");
    }

    /* one S per opportunity and no burst: never two S lines without a send in between */
    CHECK(ev_count[EV_S] == ev_count[EV_SEND] && ev_count[EV_SEND] == ev_count[EV_TEMP],
          "S/send/temperature counts are one-to-one");

    /* the main loop does not rely on multiple rounds: idle rounds have zero business calls */
    {
        int i, n = 0, ok = 1, biz_since_sleep = 0;
        for (i = 0; i < ev_n; i++) {
            if (ev_seq[i] == EV_TEMP || ev_seq[i] == EV_SEND ||
                ev_seq[i] == EV_TRACE_FETCH || ev_seq[i] == EV_S) {
                biz_since_sleep++;
            }
            if (ev_seq[i] == EV_GO_SLEEP) {
                n++;
                /* rounds without a raised opportunity must have zero business calls */
                if (n >= 4 && biz_since_sleep != 0) ok = 0;
                biz_since_sleep = 0;
            }
        }
        CHECK(ok, "idle rounds perform zero sampling/sending (no multi-round retry)");
    }

    /* ================= Scenario 3: flags cleared by transaction ================= */
    printf("\n[STATE] after a transaction the pending flags are consumed\n");
    CHECK(sample_flag == 0u, "sample_flag consumed by the transaction");
    CHECK(report_req == 0u, "report_req consumed by the send call");

    printf("\n==== result: %d passed, %d failed ====\n", pass, fail);
    return fail ? 1 : 0;
}
