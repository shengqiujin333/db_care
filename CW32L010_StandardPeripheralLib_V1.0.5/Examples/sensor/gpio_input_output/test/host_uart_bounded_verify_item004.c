/*
 * host_uart_bounded_verify_item004.c
 * 作者: embedded_tester.embedded_verification（测试能力独立验证资产，不参与产品构建）
 *
 * 目的（TD-003 T-L0-16 / T-L0-17 / T-L0-22 / T-L6-10 / T-L1-16）:
 *   直接编译**当前产品** USER/src/debug_trace.c，在影子 UART 上观测:
 *     1) 正常路径三行（BOOT/VDD/IOTEST/IOSIG/BUS/S/G）的原始字节、CRLF、<=96 B、上电顺序；
 *     2) TXE 永不就绪时: 本行不写字节、轮询次数 = DEBUG_UART_LINE_GUARD+1、调用返回；
 *     3) 行中止后下一行重新获得预算并完整输出；
 *     4) S 与 G 各自独立预算（S 中止不影响 G 获得新的整行预算）；
 *     5) 关闭时 TXE / TXBUSY 卡死: 等待 = DEBUG_UART_CLOSE_GUARD+1，仍执行 UART1 复位×2、
 *        关时钟、PA05/PA06 置输入，不触碰 GPIOA 复位/时钟；幂等；
 *     6) 卡死前后业务模型结果与"睡眠"次数不变；传入快照不被修改。
 *
 * 本 harness 只打印原始观测（CASE/计数/HEX/引脚模式/业务序列），
 * 判据由独立 Python 模型 verify_uart_guard_item004.py 计算，避免"被测实现与判据同源"。
 *
 * 构建（在 gpio_input_output/ 下）:
 *   gcc -std=c11 -Wall -Wextra -DSENSOR_DEBUG_UART=1 -Itest/mock_uart_ev -IUSER/inc \
 *       test/host_uart_bounded_verify_item004.c USER/src/debug_trace.c -o <out>
 * 旧实现负向对照: 用父提交的 debug_trace.c 替换最后一个源文件，其余相同。
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <setjmp.h>

#include "debug_trace.h"
#include "mock_uart_hw.h"

/* ==================================================================== */
/* 影子外设状态                                                          */
/* ==================================================================== */
GPIO_TypeDef mock_gpioa, mock_gpiob;

#define MOCK_POLL_HARD_CAP 20000000u   /* 单 CASE 轮询硬上限: 旧实现无界时被捕获 */
#define CAP_MAX 2048
#define OPLOG_MAX 128

typedef enum { TX_READY = 0, TX_STUCK = 1, TX_SLOW = 2 } tx_mode_t;

static tx_mode_t g_tx_mode = TX_READY;
static uint32_t  g_slow_every = 1u;
static uint32_t  g_recover_after_abs = 0xFFFFFFFFu; /* TX_STUCK 下超过该绝对轮询数后恢复 */

static uint32_t  g_case_polls;      /* 本 CASE 内总轮询数 (硬上限用) */
static uint32_t  g_seg_txe;         /* 本段 TXE 轮询数 */
static uint32_t  g_seg_txbusy;      /* 本段 TXBUSY 轮询数 */
static uint32_t  g_txe_total;       /* 全程 TXE 轮询数 */
static uint32_t  g_txbusy_total;    /* 全程 TXBUSY 轮询数 */

static int       g_txbusy_stuck;
static int       g_tx_pending;

static uint32_t  g_pin_mode[8];
static int       g_uart_init_calls, g_uart_reset_calls, g_uart_clk_en, g_uart_clk_disc;
static int       g_gpioa_reset_calls, g_gpioa_clk_disc_calls, g_gpiob_reset_calls;
static char      g_oplog[OPLOG_MAX];
static size_t    g_oplog_n;
static UART_InitTypeDef g_last_cfg;

static char      g_cap[CAP_MAX];
static size_t    g_cap_n;

static jmp_buf   g_cap_jmp;
static const char *g_case_name = "?";

static void oplog(char c)
{
    if (g_oplog_n + 1u < sizeof(g_oplog)) {
        g_oplog[g_oplog_n++] = c;
        g_oplog[g_oplog_n] = '\0';
    }
}

static int pin_index(uint16_t pins)
{
    int i;
    for (i = 0; i < 8; i++) {
        if ((pins & (uint16_t)(1u << i)) != 0u) return i;
    }
    return -1;
}

/* ---- GPIO ---- */
void GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *init)
{
    int i = pin_index(init->Pins);
    (void)port;
    if (i >= 0) {
        g_pin_mode[i] = init->Mode;
        if (i == 5) oplog('5');
        if (i == 6) oplog('6');
    }
}

void GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state)
{
    (void)port; (void)pin; (void)state;
}

GPIO_PinState GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin)
{
    (void)port; (void)pin;
    return GPIO_Pin_SET;
}

/* ---- UART ---- */
void UART_Init(uint32_t uart, UART_InitTypeDef *cfg)
{
    (void)uart;
    g_uart_init_calls++;
    g_last_cfg = *cfg;
    oplog('I');
}

void UART_SendData_8bit(uint32_t uart, uint8_t data)
{
    (void)uart;
    if (g_cap_n + 1u < sizeof(g_cap)) {
        g_cap[g_cap_n++] = (char)data;
        g_cap[g_cap_n] = '\0';
    }
    g_tx_pending++;
}

FlagStatus UART_GetFlagStatus(uint32_t uart, uint16_t flag)
{
    (void)uart;

    if (flag == UART_FLAG_TXE) {
        g_seg_txe++;
        g_txe_total++;
        g_case_polls++;
        if (g_case_polls > MOCK_POLL_HARD_CAP) {
            longjmp(g_cap_jmp, 1);
        }
        switch (g_tx_mode) {
        case TX_READY:
            return SET;
        case TX_STUCK:
            return (g_seg_txe > g_recover_after_abs) ? SET : RESET;
        case TX_SLOW:
        default:
            return ((g_seg_txe % g_slow_every) == 0u) ? SET : RESET;
        }
    }

    if (flag == UART_FLAG_TXBUSY) {
        g_seg_txbusy++;
        g_txbusy_total++;
        g_case_polls++;
        if (g_case_polls > MOCK_POLL_HARD_CAP) {
            longjmp(g_cap_jmp, 1);
        }
        if (g_txbusy_stuck != 0) {
            return SET;
        }
        if (g_tx_pending > 0) {
            g_tx_pending--;
            return SET;
        }
        return RESET;
    }

    if (flag == UART_FLAG_TC) {
        return SET;
    }
    return SET;
}

/* ---- SYSCTRL ---- */
void SYSCTRL_APBPeriphClk_Enable1(uint32_t periph, FunctionalState st)
{
    (void)periph;
    if (st == ENABLE) { g_uart_clk_en++; oplog('E'); }
    else              { g_uart_clk_disc++; oplog('C'); }
}

void SYSCTRL_APBPeriphReset1(uint32_t periph, FunctionalState st)
{
    (void)periph; (void)st;
    g_uart_reset_calls++;
    oplog('R');
}

void SYSCTRL_AHBPeriphReset(uint32_t periph, FunctionalState st)
{
    (void)periph; (void)st;
    g_gpioa_reset_calls++;
    oplog('A');
}

void SYSCTRL_AHBPeriphClk_Enable(uint32_t periph, FunctionalState st)
{
    (void)periph;
    if (st == DISABLE) { g_gpioa_clk_disc_calls++; oplog('D'); }
}

/* ==================================================================== */
/* 观测辅助                                                              */
/* ==================================================================== */
static void seg_reset(void)
{
    g_seg_txe = 0u;
    g_seg_txbusy = 0u;
}

static void cap_reset(void)
{
    g_cap_n = 0u;
    g_cap[0] = '\0';
}

static void io_reset_counters(void)
{
    g_uart_init_calls = 0; g_uart_reset_calls = 0;
    g_uart_clk_en = 0; g_uart_clk_disc = 0;
    g_gpioa_reset_calls = 0; g_gpioa_clk_disc_calls = 0; g_gpiob_reset_calls = 0;
    g_oplog_n = 0u; g_oplog[0] = '\0';
}

static void print_hex_bytes(void)
{
    size_t i;
    printf("BYTES %u\n", (unsigned)g_cap_n);
    printf("CAP_HEX ");
    for (i = 0u; i < g_cap_n; i++) {
        printf("%02X", (unsigned)(uint8_t)g_cap[i]);
    }
    printf("\n");
}

static void print_io(void)
{
    printf("UART_INIT %d\n", g_uart_init_calls);
    printf("UART_RESET %d\n", g_uart_reset_calls);
    printf("UART_CLK_EN %d\n", g_uart_clk_en);
    printf("UART_CLK_DIS %d\n", g_uart_clk_disc);
    printf("GPIOA_RESET %d\n", g_gpioa_reset_calls);
    printf("GPIOA_CLK_DIS %d\n", g_gpioa_clk_disc_calls);
    printf("PIN5_MODE 0x%08X\n", (unsigned)g_pin_mode[5]);
    printf("PIN6_MODE 0x%08X\n", (unsigned)g_pin_mode[6]);
    printf("OPLOG %s\n", g_oplog);
}

static void begin_case(const char *name)
{
    g_case_name = name;
    g_case_polls = 0u;
    seg_reset();
    cap_reset();
    io_reset_counters();
    printf("CASE %s\n", name);
}

static void end_case(void)
{
    printf("ENDCASE\n");
}

/* 业务模型: 与 UART 无关的确定性推进, 用于比较卡死/正常两种情形的业务结果 */
typedef struct {
    int16_t  temp_x10;
    uint16_t hum_x10;
    uint8_t  report;
    uint8_t  send;
    uint32_t lcg;
} biz_t;

static void biz_step(biz_t *b)
{
    b->lcg = b->lcg * 1664525u + 1013904223u;
    b->temp_x10 = (int16_t)((int32_t)((b->lcg >> 16) % 600u) - 100);
    b->hum_x10 = (uint16_t)((b->lcg >> 8) % 1001u);
    b->report = (uint8_t)((b->lcg >> 5) & 1u);
    b->send = (uint8_t)((b->lcg >> 7) % 3u);
}

static void fill_sample(debug_trace_sample_t *s, const biz_t *b, uint32_t tick)
{
    memset(s, 0, sizeof(*s));
    s->tick = tick;
    s->prev_temp_x10 = (int16_t)(b->temp_x10 - 5);
    s->have_prev = 1u;
    s->light_valid = 1u;
    s->light_ok = 8u;
    s->light_min = 10u + (uint16_t)(tick % 7u);
    s->light_max = 20u + (uint16_t)(tick % 9u);
    s->light_mean = 15u + (uint16_t)(tick % 5u);
    s->light_dark = (uint8_t)((tick & 1u) != 0u);
    s->temp_x10 = b->temp_x10;
    s->hum_x10 = b->hum_x10;
    s->sample_ok = 1u;
    s->report = b->report;
    s->send = b->send;
    s->retry = 0u;
    s->diag_valid = 0u;
}

/* ==================================================================== */
/* 用例                                                                  */
/* ==================================================================== */
static void case_normal_boot_order(void)
{
    uint8_t uid[10] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA };
    debug_trace_vdd_t    v;
    debug_trace_iotest_t io;
    debug_trace_iosig_t  sig;
    debug_trace_bus_t    b;

    begin_case("N_BOOT_ORDER");

    debug_trace_boot(uid, 0x0001u);           /* main.c 上电顺序: BOOT -> VDD -> IOTEST -> IOSIG -> BUS */
    memset(&v, 0, sizeof(v));
    v.ok = 1u; v.code = 1632u; v.bgr_mv = 1200u; v.mv = 3011u;
    debug_trace_vdd(&v);

    memset(&io, 0, sizeof(io));
    io.sda_lo = 0u; io.scl_lo = 0u; io.idle = 3u; io.swap_count = 0u; io.gc = 0u;
    debug_trace_iotest(&io);

    memset(&sig, 0, sizeof(sig));
    sig.scl20 = 0x95555u; sig.sda20 = 0x30303u;
    debug_trace_iosig(&sig);

    memset(&b, 0, sizeof(b));
    b.idle = 3u; b.ack_count = 0u;
    debug_trace_bus(&b);

    printf("TXE_POLLS %u\n", (unsigned)g_seg_txe);
    printf("TXBUSY_POLLS %u\n", (unsigned)g_seg_txbusy);
    print_hex_bytes();
    print_io();
    printf("UART_BAUD %u\n", (unsigned)g_last_cfg.UART_BaudRate);
    printf("UART_UCLK %u\n", (unsigned)g_last_cfg.UART_UclkFreq);
    end_case();
}

static void case_worst_s_and_g(void)
{
    debug_trace_sample_t s;

    begin_case("N_WORST_S");

    memset(&s, 0, sizeof(s));
    s.tick = 4294967295u; s.prev_temp_x10 = -1250; s.have_prev = 1u;
    s.light_valid = 1u; s.light_ok = 8u; s.light_min = 4095u; s.light_max = 4095u;
    s.light_mean = 4095u; s.light_dark = 1u; s.temp_x10 = -1250; s.hum_x10 = 1000u;
    s.sample_ok = 0u; s.report = 1u; s.send = 2u; s.retry = 2u;
    s.diag_valid = 0u;
    debug_trace_sample(&s);
    print_hex_bytes();
    end_case();

    begin_case("N_WORST_G");
    memset(&s, 0, sizeof(s));
    s.tick = 3u; s.temp_x10 = 456; s.hum_x10 = 678u; s.sample_ok = 0u;
    s.diag_valid = 1u; s.diag_status = 5u;
    s.diag_ack44 = 1u; s.diag_ack45 = 1u;
    s.diag_read_retry = 5u; s.diag_attempt = 3u; s.diag_raw_valid = 1u;
    {
        uint8_t k;
        for (k = 0u; k < 6u; k++) s.diag_raw[k] = (uint8_t)(0xABu + k);
    }
    debug_trace_sample(&s);
    print_hex_bytes();
    end_case();
}

static void case_worst_bus_iotest_iosig_vdd(void)
{
    debug_trace_iotest_t io;
    debug_trace_iosig_t  sig;
    debug_trace_bus_t    b;
    debug_trace_vdd_t    v;

    begin_case("N_WORST_BUS");
    memset(&b, 0, sizeof(b));
    b.idle = 0u; b.ack_count = 8u; b.ack_truncated = 1u;
    {
        uint8_t k;
        for (k = 0u; k < 8u; k++) b.ack_addr[k] = (uint8_t)(0x08u + k);
    }
    debug_trace_bus(&b);
    print_hex_bytes();
    end_case();

    begin_case("N_WORST_IOTEST");
    memset(&io, 0, sizeof(io));
    io.sda_lo = 1u; io.scl_lo = 1u; io.idle = 3u;
    io.swap_count = 2u; io.swap_addr[0] = 0x44u; io.swap_addr[1] = 0x45u; io.gc = 3u;
    debug_trace_iotest(&io);
    print_hex_bytes();
    end_case();

    begin_case("N_WORST_IOSIG");
    memset(&sig, 0, sizeof(sig));
    sig.scl20 = 0xFFFFFu; sig.sda20 = 0xFFFFFu;
    debug_trace_iosig(&sig);
    print_hex_bytes();
    end_case();

    begin_case("N_WORST_VDD");
    memset(&v, 0, sizeof(v));
    v.ok = 1u; v.code = 4095u; v.bgr_mv = 4095u; v.mv = 6000u;
    debug_trace_vdd(&v);
    print_hex_bytes();
    end_case();
}

static void case_slow_txe(void)
{
    debug_trace_sample_t s;
    biz_t b = { 0, 0u, 0u, 0u, 1u };

    begin_case("N_SLOW_TXE");
    g_tx_mode = TX_SLOW;
    g_slow_every = 7u;
    biz_step(&b);
    fill_sample(&s, &b, 3u);
    debug_trace_sample(&s);
    printf("TXE_POLLS %u\n", (unsigned)g_seg_txe);
    print_hex_bytes();
    end_case();
    g_tx_mode = TX_READY;
}

static void case_txe_stuck_abort(void)
{
    debug_trace_sample_t s;
    debug_trace_sample_t ref;
    biz_t b = { 0, 0u, 0u, 0u, 1u };

    begin_case("F1_TXE_STUCK_LINE");
    g_tx_mode = TX_STUCK;
    g_recover_after_abs = 0xFFFFFFFFu;
    biz_step(&b);
    fill_sample(&s, &b, 3u);
    memcpy(&ref, &s, sizeof(ref));
    debug_trace_sample(&s);
    printf("SNAP_CHANGED %d\n", (memcmp(&ref, &s, sizeof(ref)) != 0) ? 1 : 0);
    printf("TXE_POLLS %u\n", (unsigned)g_seg_txe);
    print_hex_bytes();

    /* 本行中止后, 关闭路径仍必须有界并完成 teardown (TXE 仍卡死) */
    seg_reset();
    io_reset_counters();
    debug_trace_flush_close();
    printf("CLOSE_TXE_POLLS %u\n", (unsigned)g_seg_txe);
    printf("CLOSE_TXBUSY_POLLS %u\n", (unsigned)g_seg_txbusy);
    print_io();
    end_case();
    g_tx_mode = TX_READY;
}

static void case_recover_next_line(void)
{
    debug_trace_sample_t s;
    biz_t b = { 0, 0u, 0u, 0u, 1u };

    begin_case("F2_NEXT_LINE_RECOVERS");
    biz_step(&b);
    fill_sample(&s, &b, 3u);
    debug_trace_sample(&s);
    printf("TXE_POLLS %u\n", (unsigned)g_seg_txe);
    print_hex_bytes();
    end_case();
}

static void case_sg_independent_budget(void)
{
    debug_trace_sample_t s;

    begin_case("F3_S_G_INDEPENDENT_BUDGET");
    g_tx_mode = TX_STUCK;
    /* 前 LINE_GUARD+1 次轮询失败 (使 S 行中止), 之后恢复: G 行应获得新预算并完整输出 */
    g_recover_after_abs = (uint32_t)DEBUG_UART_LINE_GUARD + 1u;
    memset(&s, 0, sizeof(s));
    s.tick = 6u; s.prev_temp_x10 = 250; s.have_prev = 1u;
    s.light_valid = 1u; s.light_ok = 8u; s.light_min = 12u; s.light_max = 15u;
    s.light_mean = 13u; s.temp_x10 = 456; s.hum_x10 = 678u; s.sample_ok = 0u;
    s.diag_valid = 1u; s.diag_status = 2u;
    debug_trace_sample(&s);
    printf("TXE_POLLS %u\n", (unsigned)g_seg_txe);
    print_hex_bytes();
    end_case();
    g_tx_mode = TX_READY;
}

static void case_close_txe_stuck(void)
{
    debug_trace_sample_t s;
    biz_t b = { 0, 0u, 0u, 0u, 1u };

    begin_case("F4_CLOSE_TXE_STUCK");
    biz_step(&b);
    fill_sample(&s, &b, 3u);
    debug_trace_sample(&s);                 /* 正常写入一行, 产生待排空字节 */

    g_tx_mode = TX_STUCK;
    g_recover_after_abs = 0xFFFFFFFFu;
    seg_reset();
    io_reset_counters();
    debug_trace_flush_close();
    printf("CLOSE_TXE_POLLS %u\n", (unsigned)g_seg_txe);
    printf("CLOSE_TXBUSY_POLLS %u\n", (unsigned)g_seg_txbusy);
    print_io();

    /* 幂等: 第二次关闭不得重复外设操作 */
    seg_reset();
    io_reset_counters();
    debug_trace_flush_close();
    printf("IDEMPOTENT_RESET %d\n", g_uart_reset_calls);
    printf("IDEMPOTENT_CLK_DIS %d\n", g_uart_clk_disc);
    printf("IDEMPOTENT_GPIO_INIT %s\n", (g_oplog[0] == '\0') ? "none" : g_oplog);

    /* 关闭后重新打开: 恢复 READY 应能完整打印 */
    g_tx_mode = TX_READY;
    cap_reset();
    io_reset_counters();
    debug_trace_sample(&s);
    print_hex_bytes();
    end_case();
}

static void case_close_txbusy_stuck(void)
{
    debug_trace_sample_t s;
    biz_t b = { 0, 0u, 0u, 0u, 1u };

    begin_case("F5_CLOSE_TXBUSY_STUCK");
    biz_step(&b);
    fill_sample(&s, &b, 3u);
    debug_trace_sample(&s);

    g_txbusy_stuck = 1;
    seg_reset();
    io_reset_counters();
    debug_trace_flush_close();
    printf("CLOSE_TXE_POLLS %u\n", (unsigned)g_seg_txe);
    printf("CLOSE_TXBUSY_POLLS %u\n", (unsigned)g_seg_txbusy);
    print_io();
    g_txbusy_stuck = 0;
    end_case();
}

static void run_business(int stuck, biz_t *seq, int *sleep_count)
{
    debug_trace_sample_t s;
    int i;

    (void)stuck;
    *sleep_count = 0;
    for (i = 0; i < 5; i++) {
        biz_t b;
        b.lcg = 1u;
        /* 用与循环次数相关的确定初值, 两轮完全一致 */
        b.lcg = (uint32_t)(i + 1) * 2654435761u;
        biz_step(&b);
        biz_step(&b);
        fill_sample(&s, &b, (uint32_t)(i + 1) * 3u);
        debug_trace_sample(&s);       /* 先判定/发送(此处由模型代表), 后打印 */
        debug_trace_flush_close();
        (*sleep_count)++;             /* 主循环随后 go_to_sleep() */
        seq[i] = b;
    }
}

static void case_business_invariance(void)
{
    biz_t seq_n[5], seq_s[5];
    int   sleep_n = 0, sleep_s = 0;
    int   i;

    begin_case("F6_BUSINESS_INVARIANCE");
    g_tx_mode = TX_READY;
    g_case_polls = 0u;
    run_business(0, seq_n, &sleep_n);

    g_case_polls = 0u;
    g_tx_mode = TX_STUCK;
    g_recover_after_abs = 0xFFFFFFFFu;
    g_txbusy_stuck = 1;
    run_business(1, seq_s, &sleep_s);
    g_tx_mode = TX_READY;
    g_txbusy_stuck = 0;

    printf("SLEEP_NORMAL %d\n", sleep_n);
    printf("SLEEP_STUCK %d\n", sleep_s);
    printf("BIZ_NORMAL");
    for (i = 0; i < 5; i++) {
        printf(" %d,%u,%u,%u", (int)seq_n[i].temp_x10, (unsigned)seq_n[i].hum_x10,
               (unsigned)seq_n[i].report, (unsigned)seq_n[i].send);
    }
    printf("\n");
    printf("BIZ_STUCK");
    for (i = 0; i < 5; i++) {
        printf(" %d,%u,%u,%u", (int)seq_s[i].temp_x10, (unsigned)seq_s[i].hum_x10,
               (unsigned)seq_s[i].report, (unsigned)seq_s[i].send);
    }
    printf("\n");
    printf("BIZ_EQUAL %d\n", (memcmp(seq_n, seq_s, sizeof(seq_n)) == 0) ? 1 : 0);
    end_case();
}

int main(void)
{
    int i;

    printf("HARNESS host_uart_bounded_verify_item004\n");
    printf("LINE_GUARD %u\n", (unsigned)DEBUG_UART_LINE_GUARD);
    printf("CLOSE_GUARD %u\n", (unsigned)DEBUG_UART_CLOSE_GUARD);
    printf("MAXLINE %u\n", (unsigned)SENSOR_DEBUG_UART_MAXLINE);
    printf("HARD_CAP %u\n", (unsigned)MOCK_POLL_HARD_CAP);

    for (i = 0; i < 8; i++) g_pin_mode[i] = 0xFFFFu;

    if (setjmp(g_cap_jmp) != 0) {
        printf("UNBOUNDED case=%s polls>%u\n", g_case_name, (unsigned)MOCK_POLL_HARD_CAP);
        printf("DONE_UNBOUNDED\n");
        return 2;
    }

    g_tx_mode = TX_READY;
    case_normal_boot_order();
    case_worst_s_and_g();
    case_worst_bus_iotest_iosig_vdd();
    case_slow_txe();
    case_txe_stuck_abort();
    case_recover_next_line();
    case_sg_independent_budget();
    case_close_txe_stuck();
    case_close_txbusy_stuck();
    case_business_invariance();

    printf("TXE_TOTAL %u\n", (unsigned)g_txe_total);
    printf("TXBUSY_TOTAL %u\n", (unsigned)g_txbusy_total);
    printf("DONE\n");
    return 0;
}
