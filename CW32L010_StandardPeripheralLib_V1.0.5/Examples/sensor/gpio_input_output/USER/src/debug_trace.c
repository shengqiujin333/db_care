/*
 * debug_trace.c - UART1 调试通道实现 (readme 修改点 8; FD-002 rev 4.0 §2.4/§6.5)
 *
 * 行为:
 *   - debug_trace_boot():     打开 UART1, 打印一条启动横幅 (单行)。
 *   - debug_trace_sample():   打印一行 S 轨迹 (字段固定、仅整数、<= 96 字节含行尾)。
 *   - debug_trace_flush_close(): 等 TC 后关闭 UART1 (幂等)。
 *
 * 字节预算: S 行的字面格式与最坏宽度见本文末尾注释; 单行最坏 86 字节 (含 CRLF),
 *           不超过 SENSOR_DEBUG_UART_MAXLINE=96。
 *
 * 所有权 (FD-002 §2.4/§11.15): UART1 与 PA05/PA06 仅由本文件配置;
 * 关闭时只复位/关闭 UART1 外设与时钟并把 PA05/PA06 置为输入, 不触碰 GPIOA 其它引脚,
 * 不复位 GPIOA, 不关闭 GPIOA 时钟。
 */
#include "debug_trace.h"
#include "sensor_config.h"

/* ==================================================================== */
/* 关闭编译开关时本文件不产生任何符号 (debug_trace.h 提供空操作内联定义)  */
/* ==================================================================== */
#if SENSOR_DEBUG_UART

#include "cw32l010_gpio.h"
#include "cw32l010_uart.h"
#include "cw32l010_sysctrl.h"

static uint8_t s_uart_open = 0u;

/* ==================================================================== */
/* 输出原语 (轮询, 逐字节有界)                                           */
/* ==================================================================== */
static void trace_putc(char c)
{
    while (UART_GetFlagStatus(DEBUG_UARTx, UART_FLAG_TXE) == RESET) {
        /* 轮询等待发送保持寄存器空; 有界由固定行长保证 */
    }
    UART_SendData_8bit(DEBUG_UARTx, (uint8_t)c);
}

static void trace_puts(const char *s)
{
    while (*s != '\0') {
        trace_putc(*s++);
    }
}

/* 无符号十进制 (最多 10 位), width 为最小位数 (不足左补 '0') */
static void trace_u32(uint32_t v, uint8_t width)
{
    char    b[10];
    uint8_t n = 0u;

    if (v == 0u) {
        b[n++] = '0';
    }
    while ((v != 0u) && (n < (uint8_t)sizeof(b))) {
        b[n++] = (char)('0' + (uint8_t)(v % 10u));
        v /= 10u;
    }
    while (n < width) {
        b[n++] = '0';
    }
    while (n > 0u) {
        trace_putc(b[--n]);
    }
}

/* 有符号十进制 (输入为 int16 范围, 不涉及 INT32_MIN 溢出) */
static void trace_i32(int32_t v)
{
    if (v < 0) {
        trace_putc('-');
        trace_u32((uint32_t)(-v), 1u);
    } else {
        trace_u32((uint32_t)v, 1u);
    }
}

/* 固定位数十六进制 (大写) */
static void trace_hex(uint32_t v, uint8_t digits)
{
    static const char hx[] = "0123456789ABCDEF";
    int8_t            i;

    for (i = (int8_t)(digits - 1u); i >= 0; i--) {
        trace_putc(hx[(v >> (4u * (uint8_t)i)) & 0x0Fu]);
    }
}

/* ==================================================================== */
/* UART1 开关                                                            */
/* ==================================================================== */
static void trace_uart_open(void)
{
    GPIO_InitTypeDef gpio = {0};
    UART_InitTypeDef uart = {0};

    if (s_uart_open != 0u) {
        return;
    }

    __SYSCTRL_GPIOA_CLK_ENABLE();
    SYSCTRL_APBPeriphClk_Enable1(DEBUG_UART_CLK, ENABLE);

    /* PA06 = UART1_TXD: 推挽输出 + AF6=UART1TXD */
    gpio.Pins = DEBUG_UART_TX_GPIO_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.IT   = GPIO_IT_NONE;
    GPIO_Init(DEBUG_UART_TX_GPIO_PORT, &gpio);

    /* PA05 = UART1_RXD: 上拉输入 + AF5=UART1RXD */
    gpio.Pins = DEBUG_UART_RX_GPIO_PIN;
    gpio.Mode = GPIO_MODE_INPUT_PULLUP;
    gpio.IT   = GPIO_IT_NONE;
    GPIO_Init(DEBUG_UART_RX_GPIO_PORT, &gpio);

    DEBUG_UART_AFTX;
    DEBUG_UART_AFRX;

    uart.UART_BaudRate            = DEBUG_UART_BaudRate;
    uart.UART_Over                = UART_Over_16;
    uart.UART_Source              = UART_Source_PCLK;
    uart.UART_UclkFreq            = DEBUG_UART_UclkFreq;
    uart.UART_StartBit            = UART_StartBit_FE;
    uart.UART_StopBits            = UART_StopBits_1;
    uart.UART_Parity              = UART_Parity_No;
    uart.UART_HardwareFlowControl = UART_HardwareFlowControl_None;
    uart.UART_Mode                = UART_Mode_Rx | UART_Mode_Tx;
    UART_Init(DEBUG_UARTx, &uart);

    s_uart_open = 1u;
}

void debug_trace_flush_close(void)
{
    GPIO_InitTypeDef gpio = {0};

    if (s_uart_open == 0u) {
        return;
    }

    /*
     * 等发送器完全排空后再关闭 (D-ITEM001-1 修复):
     *   1) 先等 TXE: 确认最后一字节已从发送数据寄存器进入移位寄存器;
     *   2) 再等 TXBUSY 清零: 确认移位寄存器已排空。
     * 不能只用 TC: TC 是可清除标志, 上一帧完成后可能保持置位, 使等待成为空操作,
     * 随后的 UART1 复位会截断仍在移位的尾字节 (实测丢失 S 行末尾 CR+LF)。
     * vendor UART_SendString() 同样以 TXBUSY 排空。
     */
    while (UART_GetFlagStatus(DEBUG_UARTx, UART_FLAG_TXE) == RESET) {
    }
    while (UART_GetFlagStatus(DEBUG_UARTx, UART_FLAG_TXBUSY) == SET) {
    }

    /* 只复位/关闭 UART1 外设与时钟; 绝不动 GPIOA 复位或 GPIOA 时钟 */
    SYSCTRL_APBPeriphReset1(DEBUG_UART_CLK, ENABLE);
    SYSCTRL_APBPeriphReset1(DEBUG_UART_CLK, DISABLE);
    SYSCTRL_APBPeriphClk_Enable1(DEBUG_UART_CLK, DISABLE);

    /* PA06/PA05 置为输入 (不外驱动); 不触碰 PA03/PA04(软 I2C) 与 PA07/PA08(SWD) */
    gpio.Pins = DEBUG_UART_TX_GPIO_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.IT   = GPIO_IT_NONE;
    GPIO_Init(DEBUG_UART_TX_GPIO_PORT, &gpio);

    gpio.Pins = DEBUG_UART_RX_GPIO_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    GPIO_Init(DEBUG_UART_RX_GPIO_PORT, &gpio);

    s_uart_open = 0u;
}

/* ==================================================================== */
/* 轨迹输出                                                              */
/* ==================================================================== */
void debug_trace_boot(const uint8_t *uid, uint32_t reset_flags)
{
    if (uid == NULL) {
        return;
    }

    trace_uart_open();
    if (s_uart_open == 0u) {
        return;
    }

    trace_puts("BOOT fw=FD-002r4 uid=");
    trace_hex(uid[0], 2u);
    trace_hex(uid[1], 2u);
    trace_hex(uid[2], 2u);
    trace_hex(uid[3], 2u);
    trace_puts(" rst=");
    trace_hex(reset_flags, 4u);
    trace_puts(" uart=");
    trace_u32((uint32_t)DEBUG_UART_BaudRate, 1u);
    trace_puts("\r\n");
}

void debug_trace_bus(const debug_trace_bus_t *b)
{
    uint8_t i;

    if (b == NULL) {
        return;
    }
    if (s_uart_open == 0u) {
        trace_uart_open();
    }
    if (s_uart_open == 0u) {
        return;
    }

    /*
     * 上电总线诊断行 (FWR-116; 字段固定):
     *   BUS idle=<0..3> scl=<0|1> sda=<0|1> ack=<addr7[,addr7...]|none[+]>
     * 单位/编码: idle 为位掩码 (bit0=SCL, bit1=SDA, 1=高), scl/sda 为同一位的展开;
     *           ack 为 7bit 地址的 2 位大写十六进制列表, 最多 SENSOR_BUS_DIAG_MAX_ACK 个,
     *           超出以 '+' 结尾, 无地址应答时为 none。
     * 只报告软件读到的事实, 不代表位级电气/时序结论。
     */
    trace_puts("BUS idle=");
    trace_u32((uint32_t)b->idle, 1u);
    trace_puts(" scl=");
    trace_u32((uint32_t)(((b->idle & 0x01u) != 0u) ? 1u : 0u), 1u);
    trace_puts(" sda=");
    trace_u32((uint32_t)(((b->idle & 0x02u) != 0u) ? 1u : 0u), 1u);
    trace_puts(" ack=");
    if (b->ack_count == 0u) {
        trace_puts("none");
    } else {
        for (i = 0u; i < b->ack_count; i++) {
            if (i != 0u) {
                trace_putc(',');
            }
            trace_hex((uint32_t)b->ack_addr[i], 2u);
        }
    }
    if (b->ack_truncated != 0u) {
        trace_putc('+');
    }
    trace_puts("\r\n");
}

void debug_trace_iotest(const debug_trace_iotest_t *t)
{
    uint8_t i;

    if (t == NULL) {
        return;
    }
    if (s_uart_open == 0u) {
        trace_uart_open();
    }
    if (s_uart_open == 0u) {
        return;
    }

    /*
     * 上电 I/O 自检行 (E1; 字段固定):
     *   IOTEST sda_lo=<0|1> scl_lo=<0|1> idle=<0..3> swap=<addr7[,addr7]|none>
     * sda_lo/scl_lo = 主机把该线拉低时回读到的电平 (0 = 确实拉低成功);
     * idle = 释放后回读位掩码 (bit0=SCL, bit1=SDA, 1=高);
     * swap = SDA/SCL 角色对调后收到 ACK 的候选地址 (无则 none)。
     * 只报告软件读到的事实, 不代表位级电气结论。
     */
    trace_puts("IOTEST sda_lo=");
    trace_u32((uint32_t)((t->sda_lo != 0u) ? 1u : 0u), 1u);
    trace_puts(" scl_lo=");
    trace_u32((uint32_t)((t->scl_lo != 0u) ? 1u : 0u), 1u);
    trace_puts(" idle=");
    trace_u32((uint32_t)t->idle, 1u);
    trace_puts(" swap=");
    if (t->swap_count == 0u) {
        trace_puts("none");
    } else {
        for (i = 0u; i < t->swap_count; i++) {
            if (i != 0u) {
                trace_putc(',');
            }
            trace_hex((uint32_t)t->swap_addr[i], 2u);
        }
    }
    trace_puts(" gc=");
    trace_u32((uint32_t)t->gc, 1u);
    trace_puts("\r\n");
}

void debug_trace_iosig(const debug_trace_iosig_t *s)
{
    if (s == NULL) {
        return;
    }
    if (s_uart_open == 0u) {
        trace_uart_open();
    }
    if (s_uart_open == 0u) {
        return;
    }

    /*
     * 事务内逐位回读签名行 (E1b; 字段固定):
     *   IOSIG scl=<5 位大写 hex> sda=<5 位大写 hex>
     * 各 20 bit = 20 个半位采样点 (S0..S19) 的回读电平, S0 为最高位;
     * 采样顺序: S0 START 建立 / S1 SCL 拉低 / S2..S17 八位数据 (每位低+高)
     *           / S18 ACK 时隙 SCL 低 / S19 ACK 时隙 SCL 高。
     * 只报告软件读到的事实, 不代表位级电气结论。
     */
    trace_puts("IOSIG scl=");
    trace_hex(s->scl20 & 0xFFFFFu, 5u);
    trace_puts(" sda=");
    trace_hex(s->sda20 & 0xFFFFFu, 5u);
    trace_puts("\r\n");
}

void debug_trace_sample(const debug_trace_sample_t *s)
{
    uint8_t i;
    if (s == NULL) {
        return;
    }
    if (s_uart_open == 0u) {
        trace_uart_open();
    }
    if (s_uart_open == 0u) {
        return;
    }

    /*
     * 单行格式 (字段顺序冻结; 键名单字符以守住 <=96 B 预算):
     *   S k=<tick> p=<prev> H=<have> V=<lvalid> o=<ok> n=<min> x=<max> a=<mean>
     *     D=<dark> t=<temp> h=<hum> q=<sample_ok> r=<report> s=<send> y=<retry>
     * 单位: k=1 分钟 RTC 节拍; p/t=0.1C 有符号; H/V/D/q/r=0/1; o=成功样本数;
     *       n/x/a=ADC 原始码(0..4095); h=0.1%RH; s=0 未发送/1 成功/2 失败; y=重试计数。
     */
    trace_putc('S');
    trace_puts(" k=");  trace_u32(s->tick, 1u);
    trace_puts(" p=");  trace_i32((int32_t)s->prev_temp_x10);
    trace_puts(" H=");  trace_u32((uint32_t)s->have_prev, 1u);
    trace_puts(" V=");  trace_u32((uint32_t)s->light_valid, 1u);
    trace_puts(" o=");  trace_u32((uint32_t)s->light_ok, 1u);
    trace_puts(" n=");  trace_u32((uint32_t)s->light_min, 1u);
    trace_puts(" x=");  trace_u32((uint32_t)s->light_max, 1u);
    trace_puts(" a=");  trace_u32((uint32_t)s->light_mean, 1u);
    trace_puts(" D=");  trace_u32((uint32_t)s->light_dark, 1u);
    trace_puts(" t=");  trace_i32((int32_t)s->temp_x10);
    trace_puts(" h=");  trace_u32((uint32_t)s->hum_x10, 1u);
    trace_puts(" q=");  trace_u32((uint32_t)s->sample_ok, 1u);
    trace_puts(" r=");  trace_u32((uint32_t)s->report, 1u);
    trace_puts(" s=");  trace_u32((uint32_t)s->send, 1u);
    trace_puts(" y=");  trace_u32((uint32_t)s->retry, 1u);
    trace_puts("\r\n");

    /*
     * 失败周期诊断行 (FWR-116; 仅 s->diag_valid != 0 时追加):
     *   G s=<1..5> a44=<0|1> a45=<0|1> rd=<0..5> at=<1..3> raw=<12 位大写十六进制 | ------------>
     * 只报告本轮驱动观测到的事实 (结果码/两候选地址 ACK/重读重测次数/最近一次成功读回的 6 字节);
     * raw 未读成功时用 12 个 '-' 占位; 不改变任何业务状态。
     */
    if (s->diag_valid != 0u) {
        trace_putc('G');
        trace_puts(" s=");    trace_u32((uint32_t)s->diag_status, 1u);
        trace_puts(" a44=");  trace_u32((uint32_t)((s->diag_ack44 != 0u) ? 1u : 0u), 1u);
        trace_puts(" a45=");  trace_u32((uint32_t)((s->diag_ack45 != 0u) ? 1u : 0u), 1u);
        trace_puts(" rd=");   trace_u32((uint32_t)s->diag_read_retry, 1u);
        trace_puts(" at=");   trace_u32((uint32_t)s->diag_attempt, 1u);
        trace_puts(" raw=");
        if (s->diag_raw_valid != 0u) {
            for (i = 0u; i < (uint8_t)GXHT40_RESULT_LEN; i++) {
                trace_hex((uint32_t)s->diag_raw[i], 2u);
            }
        } else {
            /* 12 个 '-' 与 6 字节 hex 位宽一致 (每个字节 2 个十六进制位) */
            for (i = 0u; i < (uint8_t)(GXHT40_RESULT_LEN * 2u); i++) {
                trace_putc('-');
            }
        }
        trace_puts("\r\n");
    }
}

#endif /* SENSOR_DEBUG_UART */

/* ==================================================================== *
 * 行最坏宽度核算 (键名 1 字符 + '=' + 值, 字段间 1 空格, 行尾 CRLF):
 *   S 行:
 *   "S"=1  " k=4294967295"=13  " p=-1250"=8  " H=1"=4  " V=1"=4  " o=8"=4
 *   " n=4095"=7  " x=4095"=7  " a=4095"=7  " D=1"=4  " t=-1250"=8
 *   " h=1000"=7  " q=0"=4  " r=1"=4  " s=2"=4  " y=3"=4      => 90
 *   行尾 "\r\n" = 2                                          => 92
 *   BUS 行:
 *   "BUS"=3 " idle=3"=7 " scl=1"=6 " sda=1"=6 " ack="=5
 *   + 8 × "XX," = 24 (+ 截断标记 '+') + CRLF 2                      => 54
 *   IOTEST 行:
 *   "IOTEST"=6 " sda_lo=0"=9 " scl_lo=0"=9 " idle=3"=7 " swap="=6
 *   + "44,45"=5 + " gc=3"=5 + CRLF 2                                  => 49
 *   IOSIG 行:
 *   "IOSIG"=5 " scl="=5 + 5 位 hex + " sda="=5 + 5 位 hex + CRLF 2   => 27
 *   G 行:
 *   "G"=1 " s=2"=4 " a44=0"=6 " a45=0"=6 " rd=5"=5 " at=3"=5
 *   " raw="=5 + 12 位 hex + CRLF 2                                   => 46
 *   每行均 < SENSOR_DEBUG_UART_MAXLINE (96)。
 *   (宿主机自检 host_debug_trace_check.c 用最坏值实测各行字节数。)
 * ==================================================================== */
