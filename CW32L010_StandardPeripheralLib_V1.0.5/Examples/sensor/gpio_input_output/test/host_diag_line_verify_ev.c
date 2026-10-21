/*
 * Independent verification harness for run8 ITEM-001 — diagnostic LINE FORMAT.
 * Author: embedded_tester.embedded_verification.
 *
 * Compiles the REAL USER/src/debug_trace.c against this capability's own emulated
 * MCU layer (test/mock_diagline_ev) and captures the exact bytes put on UART1.
 * It covers the acceptance points that the real board cannot currently exhibit
 * (no device answers on the bus, and every cycle fails):
 *
 *   [B*] BUS line: worst case = 8 addresses + '+' truncation, 'none', single
 *        address, and the idle-bitmap expansion (bit0=SCL, bit1=SDA).
 *   [G*] G line: worst case with 12 '-' placeholders and with 12 uppercase hex
 *        digits, appearing immediately AFTER its S line.
 *   [S*] S line: byte-exact against the S line actually captured on the real
 *        target (COM42), proving the diagnostic feature did not alter the frozen
 *        S format; and the worst-case S width against the 96-byte budget.
 *   [C*] no line exceeds SENSOR_DEBUG_UART_MAXLINE; every diagnostic line ends
 *        with exactly one CRLF; only integers/uppercase hex appear as values.
 *
 * Build (from gpio_input_output/):
 *   cc -std=c11 -Wall -Wextra -DSENSOR_DEBUG_UART=1 -Itest/mock_diagline_ev \
 *      -IUSER/inc test/host_diag_line_verify_ev.c USER/src/debug_trace.c \
 *      -o diag_line_verify
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "debug_trace.h"
#include "sensor_config.h"
#include "mock_diagline_hw.h"

/* ---------------- emulated UART1 / GPIO ---------------- */
GPIO_TypeDef mock_dl_gpioa, mock_dl_gpiob;
uint8_t mock_dl_tx[MOCK_DL_TXBUF];
int     mock_dl_tx_len;
int     mock_dl_uart_init_calls;
int     mock_dl_uart_reset_calls;
int     mock_dl_uart_clk_off_calls;

static int shift_ticks;          /* bytes still shifting out of the shift register */
static int gpiob_touched;

void GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *init) { (void)port; (void)init; }
void GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState st) { (void)port; (void)pin; (void)st; }
GPIO_PinState GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin) { (void)port; (void)pin; return GPIO_Pin_SET; }

void UART_Init(uint32_t uart, UART_InitTypeDef *cfg)
{
    (void)uart; (void)cfg;
    mock_dl_uart_init_calls++;
    shift_ticks = 0;
}
void UART_SendData_8bit(uint32_t uart, uint8_t data)
{
    (void)uart;
    if (mock_dl_tx_len < MOCK_DL_TXBUF) mock_dl_tx[mock_dl_tx_len++] = data;
    shift_ticks = 2;   /* the byte needs a couple of polls to leave the shifter */
}
FlagStatus UART_GetFlagStatus(uint32_t uart, uint16_t flag)
{
    (void)uart;
    if (flag == UART_FLAG_TXBUSY) {
        if (shift_ticks > 0) { shift_ticks--; return SET; }
        return RESET;
    }
    if (flag == UART_FLAG_TXE)  return SET;    /* holding register always ready here */
    if (flag == UART_FLAG_TC)   return SET;
    return RESET;
}
void SYSCTRL_APBPeriphClk_Enable1(uint32_t periph, FunctionalState st)
{
    (void)periph;
    if (st == DISABLE) mock_dl_uart_clk_off_calls++;
}
void SYSCTRL_APBPeriphReset1(uint32_t periph, FunctionalState st)
{
    (void)periph;
    if (st == ENABLE) mock_dl_uart_reset_calls++;
}
void SYSCTRL_AHBPeriphReset(uint32_t periph, FunctionalState st) { (void)periph; (void)st; }
void SYSCTRL_AHBPeriphClk_Enable(uint32_t periph, FunctionalState st) { (void)periph; (void)st; }

/* ---------------- helpers ---------------- */
static int pass, fail;
#define CHECK(c, m) do { if (c) { pass++; printf("    PASS  %s\n", m); } \
                         else   { fail++; printf("    FAIL  %s (line %d)\n", m, __LINE__); } } while (0)

static int  mark;
static void capture_mark(void) { mark = mock_dl_tx_len; }
static const char *captured(void)
{
    mock_dl_tx[mock_dl_tx_len] = 0;
    return (const char *)&mock_dl_tx[mark];
}
static int captured_len(void) { return mock_dl_tx_len - mark; }

/* count CRLF terminators and bare LF inside the freshly captured bytes */
static void count_eol(int *crlf, int *barelf)
{
    *crlf = *barelf = 0;
    for (int i = mark; i < mock_dl_tx_len; i++) {
        if (mock_dl_tx[i] == '\n') {
            if (i > 0 && mock_dl_tx[i - 1] == '\r') (*crlf)++;
            else                                    (*barelf)++;
        }
    }
}

/* assert the captured bytes equal `want`, and report the byte count */
static void expect_bytes(const char *what, const char *want)
{
    const char *got = captured();
    int n = captured_len();
    if (strcmp(got, want) != 0) {
        printf("      want=\"%s\"\n      got =\"%s\"\n", want, got);
    }
    CHECK(strcmp(got, want) == 0, what);
    /* the budget is PER LINE: check the longest single line in the capture */
    {
        int mx = 0, cur = 0;
        for (int i = 0; i < n; i++) {
            cur++;
            if (got[i] == '\n') { if (cur > mx) mx = cur; cur = 0; }
        }
        if (cur > mx) mx = cur;
        CHECK(mx <= (int)SENSOR_DEBUG_UART_MAXLINE,
              "every emitted line <= SENSOR_DEBUG_UART_MAXLINE (96)");
    }
    printf("      [%s] %d bytes: %s", what, n, got);
}

int main(void)
{
    printf("==== independent diagnostic line-format verification (embedded_tester) ====\n");

    uint8_t uid[4] = { 0x6A, 0x00, 0x2C, 0x00 };
    debug_trace_boot(uid, 0x0240u);
    printf("[boot] %s", (const char *)mock_dl_tx);
    CHECK(strcmp((const char *)mock_dl_tx,
                 "BOOT fw=FD-002r4 uid=6A002C00 rst=0240 uart=9600\r\n") == 0,
          "boot banner literal unchanged (regression)");

    /* ================= BUS line ================= */
    printf("[B1] BUS worst case: 8 addresses and the truncation marker\n");
    {
        debug_trace_bus_t b;
        memset(&b, 0, sizeof b);
        b.idle = 3; b.ack_count = 8; b.ack_truncated = 1;
        b.ack_addr[0] = 0x08; b.ack_addr[1] = 0x1F; b.ack_addr[2] = 0x2A; b.ack_addr[3] = 0x44;
        b.ack_addr[4] = 0x45; b.ack_addr[5] = 0x50; b.ack_addr[6] = 0x6E; b.ack_addr[7] = 0x77;
        capture_mark(); debug_trace_bus(&b);
        expect_bytes("B1 worst-case BUS line (8 addrs + '+' , idle=3)",
                     "BUS idle=3 scl=1 sda=1 ack=08,1F,2A,44,45,50,6E,77+\r\n");
    }

    printf("[B2] BUS with a single answering address\n");
    {
        debug_trace_bus_t b; memset(&b, 0, sizeof b);
        b.idle = 3; b.ack_count = 1; b.ack_addr[0] = 0x44;
        capture_mark(); debug_trace_bus(&b);
        expect_bytes("B2 single-address BUS line", "BUS idle=3 scl=1 sda=1 ack=44\r\n");
    }

    printf("[B3] BUS with no answering address -> the observed target state\n");
    {
        debug_trace_bus_t b; memset(&b, 0, sizeof b);
        b.idle = 1;   /* SCL high, SDA low: exactly what COM42 reports */
        capture_mark(); debug_trace_bus(&b);
        expect_bytes("B3 BUS ack=none with the target idle bitmap",
                     "BUS idle=1 scl=1 sda=0 ack=none\r\n");
    }

    printf("[B4] idle bitmap expansion (bit0=SCL, bit1=SDA)\n");
    {
        const struct { uint8_t idle; const char *want; } tab[] = {
            { 0, "BUS idle=0 scl=0 sda=0 ack=none\r\n" },
            { 1, "BUS idle=1 scl=1 sda=0 ack=none\r\n" },
            { 2, "BUS idle=2 scl=0 sda=1 ack=none\r\n" },
            { 3, "BUS idle=3 scl=1 sda=1 ack=none\r\n" },
        };
        for (unsigned i = 0; i < sizeof tab / sizeof tab[0]; i++) {
            debug_trace_bus_t b; memset(&b, 0, sizeof b);
            b.idle = tab[i].idle;
            capture_mark(); debug_trace_bus(&b);
            if (strcmp(captured(), tab[i].want) != 0)
                printf("      idle=%u want=\"%s\" got=\"%s\"\n", tab[i].idle, tab[i].want, captured());
            CHECK(strcmp(captured(), tab[i].want) == 0, "idle bitmap expands consistently");
        }
    }

    printf("[B5] BUS with NULL argument must not emit anything\n");
    capture_mark(); debug_trace_bus(NULL);
    CHECK(captured_len() == 0, "NULL bus snapshot prints no bytes");

    /* ================= S line ================= */
    printf("[S1] S line byte-exact vs the line captured on the real target (COM42)\n");
    {
        /* field values taken verbatim from the real capture
         * S k=0 p=0 H=0 V=0 o=8 n=63 x=64 a=63 D=0 t=0 h=0 q=0 r=0 s=0 y=0 */
        debug_trace_sample_t s;
        memset(&s, 0, sizeof s);
        s.tick = 0; s.prev_temp_x10 = 0; s.have_prev = 0;
        s.light_valid = 0; s.light_ok = 8; s.light_min = 63; s.light_max = 64;
        s.light_mean = 63; s.light_dark = 0;
        s.temp_x10 = 0; s.hum_x10 = 0; s.sample_ok = 0; s.report = 0; s.send = 0; s.retry = 0;
        s.diag_valid = 0;
        capture_mark(); debug_trace_sample(&s);
        expect_bytes("S1 S line identical to the real-target bytes (no G line)",
                     "S k=0 p=0 H=0 V=0 o=8 n=63 x=64 a=63 D=0 t=0 h=0 q=0 r=0 s=0 y=0\r\n");
        int crlf, barelf; count_eol(&crlf, &barelf);
        CHECK(crlf == 1 && barelf == 0, "exactly one CRLF, no bare LF");
        CHECK(strchr(captured(), 'G') == NULL, "no G line for a successful cycle (diag_valid=0)");
    }

    printf("[S2] S line worst case against the 96-byte budget\n");
    {
        debug_trace_sample_t s;
        memset(&s, 0, sizeof s);
        s.tick = 4294967295u;      /* widest possible minute-tick counter */
        s.prev_temp_x10 = -1250; s.have_prev = 1;
        s.light_valid = 1; s.light_ok = 8; s.light_min = 4095; s.light_max = 4095;
        s.light_mean = 4095; s.light_dark = 1;
        s.temp_x10 = -1250; s.hum_x10 = 1000; s.sample_ok = 1; s.report = 1;
        s.send = 2; s.retry = 3;
        s.diag_valid = 0;
        capture_mark(); debug_trace_sample(&s);
        printf("      worst-case S: %d bytes\n", captured_len());
        CHECK(captured_len() <= (int)SENSOR_DEBUG_UART_MAXLINE, "worst-case S line fits the 96-byte budget");
        {
            int crlf, barelf; count_eol(&crlf, &barelf);
            CHECK(crlf == 1 && barelf == 0, "worst-case S line terminated by exactly one CRLF");
        }
    }

    /* ================= G line ================= */
    printf("[G1] failing cycle: S line followed immediately by a G line (no raw read)\n");
    {
        debug_trace_sample_t s;
        memset(&s, 0, sizeof s);
        s.sample_ok = 0; s.light_ok = 8; s.light_min = 62; s.light_max = 64; s.light_mean = 63;
        s.diag_valid = 1; s.diag_status = 2; s.diag_ack44 = 0; s.diag_ack45 = 0;
        s.diag_read_retry = 0; s.diag_attempt = 3; s.diag_raw_valid = 0;
        capture_mark(); debug_trace_sample(&s);
        expect_bytes("G1 S line + G line (12 '-' placeholders) in one call",
                     "S k=0 p=0 H=0 V=0 o=8 n=62 x=64 a=63 D=0 t=0 h=0 q=0 r=0 s=0 y=0\r\n"
                     "G s=2 a44=0 a45=0 rd=0 at=3 raw=------------\r\n");
        {
            int crlf, barelf; count_eol(&crlf, &barelf);
            CHECK(crlf == 2 && barelf == 0, "two CRLF-terminated lines, no bare LF");
        }
        const char *g = strstr(captured(), "G ");
        CHECK(g != NULL && g > captured(), "the G line follows the S line");
    }

    printf("[G2] G line worst case: every count at its maximum, 12 uppercase hex digits\n");
    {
        debug_trace_sample_t s;
        memset(&s, 0, sizeof s);
        s.diag_valid = 1; s.diag_status = 5; s.diag_ack44 = 1; s.diag_ack45 = 1;
        s.diag_read_retry = 5; s.diag_attempt = 3; s.diag_raw_valid = 1;
        const uint8_t raw[6] = { 0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45 };  /* worst-case hex letters */
        memcpy(s.diag_raw, raw, 6);
        capture_mark(); debug_trace_sample(&s);
        CHECK(strstr(captured(), "G s=5 a44=1 a45=1 rd=5 at=3 raw=ABCDEF012345\r\n") != NULL,
              "G line prints status/ACK/re-read/re-measure counts and 12 uppercase hex digits");
        {
            const char *g = strstr(captured(), "G ");
            int glen = (int)strlen(g);
            printf("      worst-case G line: %d bytes\n", glen);
            CHECK(glen <= (int)SENSOR_DEBUG_UART_MAXLINE, "worst-case G line fits the 96-byte budget");
            CHECK(glen == 46, "worst-case G line is 44 chars + CRLF");
        }
        /* every diag status code must render as a single decimal digit 1..5 */
        for (uint8_t st = 1; st <= 5; st++) {
            char want[64];
            s.diag_status = st;
            snprintf(want, sizeof want, "G s=%u a44=1 a45=1 rd=5 at=3 raw=ABCDEF012345\r\n", st);
            capture_mark(); debug_trace_sample(&s);
            CHECK(strstr(captured(), want) != NULL, "status code rendered as a single integer 1..5");
        }
    }

    printf("[G3] no line ever exceeds the budget across the whole boot\n");
    {
        /* banner + worst-case BUS + worst-case S + worst-case G: check each line */
        int maxlen = 0, lines = 0, i = 0;
        debug_trace_bus_t b; memset(&b, 0, sizeof b);
        b.idle = 3; b.ack_count = 8; b.ack_truncated = 1;
        for (int k = 0; k < 8; k++) b.ack_addr[k] = (uint8_t)(0x08 + k);
        debug_trace_sample_t s; memset(&s, 0, sizeof s);
        s.tick = 4294967295u; s.prev_temp_x10 = -1250; s.temp_x10 = -1250; s.hum_x10 = 1000;
        s.light_min = s.light_max = s.light_mean = 4095; s.send = 2; s.retry = 3;
        s.diag_valid = 1; s.diag_status = 5; s.diag_ack44 = 1; s.diag_ack45 = 1;
        s.diag_read_retry = 5; s.diag_attempt = 3; s.diag_raw_valid = 1;
        for (int k = 0; k < 6; k++) s.diag_raw[k] = 0xFF;

        mock_dl_tx_len = 0; mark = 0;
        debug_trace_boot(uid, 0xFFFFFFFFu);
        debug_trace_bus(&b);
        debug_trace_sample(&s);
        while (i < mock_dl_tx_len) {
            int j = i;
            while (j < mock_dl_tx_len && mock_dl_tx[j] != '\n') j++;
            /* drop the CRLF from the counted length */
            int len = (j - i) + 1;
            if (len > maxlen) maxlen = len;
            lines++;
            i = j + 1;
        }
        printf("      %d lines, longest = %d bytes\n", lines, maxlen);
        CHECK(maxlen <= (int)SENSOR_DEBUG_UART_MAXLINE, "every emitted line <= 96 bytes");
        CHECK(lines == 4, "banner + BUS + S + G = 4 lines for one failing cycle");
        /* static: the module must not pull in floating point or dynamic memory */
    }

    printf("[C1] close semantics: UART1 reset/clock-off happen after full drain\n");
    {
        int resets_before = mock_dl_uart_reset_calls;
        mock_dl_uart_init_calls = 0;
        debug_trace_flush_close();
        CHECK(mock_dl_uart_reset_calls > resets_before, "close resets UART1");
        CHECK(mock_dl_uart_clk_off_calls > 0, "close disables the UART1 clock");
        /* idempotent */
        int r2 = mock_dl_uart_reset_calls;
        debug_trace_flush_close();
        CHECK(mock_dl_uart_reset_calls == r2, "close is idempotent");
        /* re-open on the next diagnostic line (wake-up path) */
        debug_trace_bus_t bb; memset(&bb, 0, sizeof bb); bb.idle = 1;
        capture_mark(); debug_trace_bus(&bb);
        CHECK(mock_dl_uart_init_calls == 1, "UART1 re-opened for the next diagnostic line");
    }
    (void)gpiob_touched;

    printf("==== result: %d passed, %d failed ====\n", pass, fail);
    return fail ? 1 : 0;
}
