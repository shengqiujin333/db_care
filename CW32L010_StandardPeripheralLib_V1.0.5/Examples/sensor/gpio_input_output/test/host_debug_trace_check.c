/*
 * host_debug_trace_check.c - T1 自检: UART1 调试通道 (真实 USER/src/debug_trace.c)
 * 作者: firmware_engineer.firmware_implementation
 *
 * 目的 (TD-002 §4.4 T-L2-01/02/03/06/08/09/10 的实现侧自检):
 *   - 启动横幅字段与格式 (固件标识/UID 前 4 字节/复位来源/串口参数);
 *   - S 轨迹字段顺序冻结、仅整数、单行含行尾 <= SENSOR_DEBUG_UART_MAXLINE(96) 字节;
 *   - 最坏宽度 (int16 负值 + 4 位 ADC 码 + 5 位 tick) 仍在预算内;
 *   - 关闭语义: 等 TC、只复位/关闭 UART1、PA05/PA06 置输入、不复位 GPIOA、不关 GPIOA 时钟;
 *   - 幂等; 关闭后再次采样能重新初始化 (唤醒路径);
 *   - debug_trace_sample 不修改传入的状态快照 (无业务副作用)。
 *
 * 构建 (从 gpio_input_output/):
 *   gcc -std=c11 -Wall -Wextra -DSENSOR_DEBUG_UART=1 -Itest/mock_trace -IUSER/inc \
 *       test/host_debug_trace_check.c USER/src/debug_trace.c -o host_debug_trace_check
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "debug_trace.h"
#include "mock_trace_hw.h"

/* ==================================================================== */
/* 模拟 MCU 外设层 (记录调用, 捕获 UART1 字节)                            */
/* ==================================================================== */
GPIO_TypeDef mock_gpioa, mock_gpiob;

#define CAP_MAX 512
static char     cap[CAP_MAX];
static size_t   cap_n;
static uint32_t pin_mode[8];        /* bit0..7 -> PA0..PA7 */
static int      gpioa_reset_calls;
static int      gpioa_clk_disable_calls;
static int      uart_reset_calls;
static int      uart_clk_enable_calls;
static int      uart_clk_disable_calls;
static int      uart_init_calls;
static int      tc_poll_calls;
static int      txe_poll_calls;
static int      txbusy_poll_calls;
static int      tx_pending;          /* 尚未移出移位寄存器的字节 (mock 每轮询一次移出一字节) */
static int      pending_at_reset;    /* 复位 UART1 时刻仍在移位的字节数 (必须为 0) */
static UART_InitTypeDef last_uart_cfg;

static int pin_index(uint16_t pins)
{
    int i;
    for (i = 0; i < 8; i++) {
        if ((pins & (uint16_t)(1u << i)) != 0u) return i;
    }
    return -1;
}

void GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *init)
{
    (void)port;
    int i = pin_index(init->Pins);
    if (i >= 0) pin_mode[i] = init->Mode;
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

void UART_Init(uint32_t uart, UART_InitTypeDef *cfg)
{
    (void)uart;
    uart_init_calls++;
    last_uart_cfg = *cfg;
}

void UART_SendData_8bit(uint32_t uart, uint8_t data)
{
    (void)uart;
    if (cap_n < (CAP_MAX - 1u)) cap[cap_n++] = (char)data;
    cap[cap_n] = '\0';
    tx_pending++;                       /* 写入即进入发送队列, 需由 TXBUSY 排空 */
}

FlagStatus UART_GetFlagStatus(uint32_t uart, uint16_t flag)
{
    (void)uart;
    if (flag == UART_FLAG_TC)     tc_poll_calls++;
    if (flag == UART_FLAG_TXE)    txe_poll_calls++;
    if (flag == UART_FLAG_TXBUSY) {
        txbusy_poll_calls++;
        if (tx_pending > 0) {
            tx_pending--;               /* 每次轮询移出一字节 */
            return SET;                 /* 仍在移位 -> 忙 */
        }
        return RESET;
    }
    return SET;
}

void SYSCTRL_APBPeriphClk_Enable1(uint32_t periph, FunctionalState st)
{
    (void)periph;
    if (st == ENABLE) uart_clk_enable_calls++;
    else              uart_clk_disable_calls++;
}

void SYSCTRL_APBPeriphReset1(uint32_t periph, FunctionalState st)
{
    (void)periph; (void)st;
    if (pending_at_reset == 0) pending_at_reset = tx_pending;
    uart_reset_calls++;
}

void SYSCTRL_AHBPeriphReset(uint32_t periph, FunctionalState st)
{
    (void)periph; (void)st;
    gpioa_reset_calls++;
}

void SYSCTRL_AHBPeriphClk_Enable(uint32_t periph, FunctionalState st)
{
    (void)periph;
    if (st == DISABLE) gpioa_clk_disable_calls++;
}

/* ==================================================================== */
/* 断言与辅助                                                            */
/* ==================================================================== */
static int pass, fail;
#define CHECK(c, m) do { if (c) { pass++; printf("    PASS  %s\n", m); } \
                         else   { fail++; printf("    FAIL  %s (line %d)\n", m, __LINE__); } } while (0)

static const char *EXPECT_BOOT = "BOOT fw=FD-002r4 uid=01020304 rst=0001 uart=9600\r\n";
static const char *EXPECT_WORST =
    "S k=4294967295 p=-1250 H=1 V=1 o=8 n=4095 x=4095 a=4095 D=1 t=-1250 h=1000 q=0 r=1 s=2 y=2\r\n";
static const char *EXPECT_TYPICAL =
    "S k=3 p=250 H=1 V=1 o=8 n=12 x=15 a=13 D=0 t=456 h=678 q=1 r=0 s=0 y=0\r\n";

static void cap_reset(void) { cap_n = 0; cap[0] = '\0'; }

/* 按冻结顺序取出各字段出现位置; 返回 0 = 顺序正确 */
static int keys_in_order(const char *line)
{
    static const char *keys[] = { " k=", " p=", " H=", " V=", " o=", " n=", " x=",
                                  " a=", " D=", " t=", " h=", " q=", " r=", " s=", " y=" };
    const char *at = line;
    size_t i;
    for (i = 0; i < (sizeof(keys) / sizeof(keys[0])); i++) {
        at = strstr(at, keys[i]);
        if (at == NULL) return 1;
        at += strlen(keys[i]);
    }
    return 0;
}

int main(void)
{
    uint8_t  uid[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    debug_trace_sample_t s;
    debug_trace_sample_t ref;

    printf("==== T1 UART1 debug trace self-check (firmware_engineer) ====\n");

    printf("[T1] boot banner\n");
    cap_reset();
    debug_trace_boot(uid, 0x0001u);
    CHECK(strcmp(cap, EXPECT_BOOT) == 0, "banner bytes exactly match the frozen format");
    printf("        |%s", cap);
    CHECK(uart_init_calls == 1, "UART1 initialised once");
    CHECK(last_uart_cfg.UART_BaudRate == 9600u, "baud rate = 9600 (COM42 configuration preserved)");
    CHECK(last_uart_cfg.UART_UclkFreq == 8000000u, "PCLK = 8 MHz (HSI DIV6)");
    CHECK((last_uart_cfg.UART_Mode & UART_Mode_Tx) != 0u, "TX mode enabled");

    printf("[T2] worst-case S line: keys, integers, <=96 bytes\n");
    cap_reset();
    memset(&s, 0, sizeof(s));
    s.tick = 4294967295u; s.prev_temp_x10 = -1250; s.have_prev = 1u;
    s.light_valid = 1u; s.light_ok = 8u; s.light_min = 4095u; s.light_max = 4095u;
    s.light_mean = 4095u; s.light_dark = 1u; s.temp_x10 = -1250; s.hum_x10 = 1000u;
    s.sample_ok = 0u; s.report = 1u; s.send = 2u; s.retry = 2u;
    debug_trace_sample(&s);
    CHECK(strcmp(cap, EXPECT_WORST) == 0, "worst-case line bytes exactly match the frozen format");
    printf("        |%s", cap);
    CHECK(strlen(cap) <= (size_t)SENSOR_DEBUG_UART_MAXLINE,
          "worst-case line <= SENSOR_DEBUG_UART_MAXLINE (96) bytes");
    printf("        len(worst)=%u bytes (budget %u)\n", (unsigned)strlen(cap),
           (unsigned)SENSOR_DEBUG_UART_MAXLINE);
    CHECK(keys_in_order(cap) == 0, "S-line key order is the frozen contract order");
    CHECK(strchr(cap, '.') == NULL && strchr(cap, 'e') == NULL && strchr(cap, 'f') == NULL,
          "integers only (no float/percent formatting)");
    CHECK(cap[strlen(cap) - 2] == '\r' && cap[strlen(cap) - 1] == '\n',
          "line terminated with CRLF (no truncation)");

    printf("[T3] typical S line\n");
    cap_reset();
    memset(&s, 0, sizeof(s));
    s.tick = 3u; s.prev_temp_x10 = 250; s.have_prev = 1u;
    s.light_valid = 1u; s.light_ok = 8u; s.light_min = 12u; s.light_max = 15u;
    s.light_mean = 13u; s.light_dark = 0u; s.temp_x10 = 456; s.hum_x10 = 678u;
    s.sample_ok = 1u; s.report = 0u; s.send = 0u; s.retry = 0u;
    debug_trace_sample(&s);
    CHECK(strcmp(cap, EXPECT_TYPICAL) == 0, "typical line bytes exactly match the frozen format");
    printf("        |%s", cap);

    printf("[T4] no side effect on the state snapshot\n");
    memcpy(&ref, &s, sizeof(ref));
    debug_trace_sample(&s);
    CHECK(memcmp(&ref, &s, sizeof(ref)) == 0, "debug_trace_sample does not modify the caller snapshot");

    printf("[T5] flush/close semantics (only UART1 + PA05/PA06)\n");
    CHECK(tx_pending > 0, "mock models in-flight bytes (last bytes not yet shifted out)");
    gpioa_reset_calls = 0; gpioa_clk_disable_calls = 0;
    uart_reset_calls = 0; uart_clk_disable_calls = 0; tc_poll_calls = 0;
    txbusy_poll_calls = 0; pending_at_reset = 0;
    pin_mode[5] = 0xFFFFu; pin_mode[6] = 0xFFFFu;
    debug_trace_flush_close();
    CHECK(txbusy_poll_calls > 0, "drains with TXBUSY (vendor UART_SendString pattern), not TC");
    CHECK(pending_at_reset == 0,
          "D-ITEM001-1 guard: UART1 is NOT reset while bytes are still shifting (would drop trailing CR/LF)");
    CHECK(uart_reset_calls == 2, "UART1 peripheral reset asserted then released");
    CHECK(uart_clk_disable_calls == 1, "UART1 APB clock disabled");
    CHECK(pin_mode[6] == GPIO_MODE_INPUT, "PA06 (UART1_TXD) returned to input");
    CHECK(pin_mode[5] == GPIO_MODE_INPUT, "PA05 (UART1_RXD) returned to input");
    CHECK(gpioa_reset_calls == 0, "GPIOA peripheral is NOT reset (protects PA03/PA04 I2C)");
    CHECK(gpioa_clk_disable_calls == 0, "GPIOA clock is NOT disabled (protects I2C/SWD pins)");

    printf("[T6] idempotent close\n");
    uart_reset_calls = 0; uart_clk_disable_calls = 0;
    debug_trace_flush_close();
    CHECK(uart_reset_calls == 0 && uart_clk_disable_calls == 0,
          "second close is a no-op (no repeated reset/clock traffic)");

    printf("[T7] re-init after close (deep-sleep wake path)\n");
    cap_reset();
    uart_init_calls = 0; uart_clk_enable_calls = 0;
    debug_trace_sample(&s);
    CHECK(strcmp(cap, EXPECT_TYPICAL) == 0, "sample after close prints a full line again");
    CHECK(uart_init_calls == 1 && uart_clk_enable_calls == 1,
          "UART1 re-initialised before printing after close");

    printf("==== result: %d passed, %d failed ====\n", pass, fail);
    return fail ? 1 : 0;
}
