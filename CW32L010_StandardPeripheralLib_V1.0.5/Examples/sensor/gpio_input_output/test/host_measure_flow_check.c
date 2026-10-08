/*
 * host_measure_flow_check.c - measure.c 采样流程的实现侧自检 (ITEM-006, 宿主机)
 *
 * 用 mock_measure_mcu/ 的影子头替代厂商 MCU 头, 编译**真实的** USER/src/measure.c,
 * 并把 gxht40_measure()/light_sample() 替换为可控桩, 从而确定性验证:
 *   - 每个采样周期先取光照再取温湿度 (调用顺序 "L" 先于 "T")
 *   - 成功: 更新前一有效温度/最近有效湿度, 并按 sensor_decide_report 置 report_req
 *   - 失败: 不上报、不更新前一有效温度、不构造 0 值 (tempvalue/huminityvalue 保持上次有效值)
 *   - 待上报标志不被"无上报条件"的后续周期清除
 *   - sample_flag==0 时不采样
 *
 * 编译(在本目录):
 *   gcc -std=c11 -Wall -Wextra -DSENSOR_CONFIG_NO_MCU -Imock_measure_mcu \
 *       -I../USER/inc -I../COMMON -I../UM2005C \
 *       host_measure_flow_check.c ../USER/src/measure.c ../USER/src/sf_i2c.c \
 *       ../USER/src/fw_core.c -lm -o host_measure_flow_check.exe \
 *       && ./host_measure_flow_check.exe
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "measure.h"
#include "gxht40.h"
#include "light.h"
#include "sensor_config.h"
#include "cw32l010_gpio.h"
#include "cw32l010_sysctrl.h"
#include "cw32l010_uart.h"

/* ---- 被采样流程调用的全局量 ---- */
extern int16_t  tempvalue;
extern uint16_t huminityvalue;
extern uint8_t  sample_flag;
extern i2c_dev *temp_ptr;      /* T1: 总线诊断与采样共用的 I2C 对象 (幂等注册) */

/* ==================================================================== */
/* MCU 层桩 (影子头声明的函数)                                            */
/* ==================================================================== */
/* ---- MCU 层桩: 带开漏回读模型 (E1 上电 I/O 自检需要) ---- */
static uint32_t g_pin_mode[16];
static uint32_t g_pin_odr[16];
static uint32_t g_pin_stuck_high[16];   /* 负对照: 模拟引脚无法被拉低/回读恒高 */

void          GPIO_Init(GPIO_TypeDef *p, GPIO_InitTypeDef *q)
{
    int i;
    (void)p;
    for (i = 0; i < 16; i++) {
        if ((q->Pins & (1u << i)) != 0u) g_pin_mode[i] = q->Mode;
    }
}

void          GPIO_WritePin(GPIO_TypeDef *p, uint16_t pin, GPIO_PinState s)
{
    int i;
    (void)p;
    for (i = 0; i < 16; i++) {
        if ((pin & (1u << i)) != 0u) g_pin_odr[i] = (s == GPIO_Pin_SET) ? 1u : 0u;
    }
}

/* 开漏模型: 外部拉低 -> 0; 开漏输出且输出锁存为 0 -> 0; 其余(释放/上拉/输入) -> 1 */
GPIO_PinState GPIO_ReadPin(GPIO_TypeDef *p, uint16_t pin)
{
    int i;
    (void)p;
    for (i = 0; i < 16; i++) {
        if ((pin & (1u << i)) != 0u) {
            if (g_pin_stuck_high[i] != 0u) return GPIO_Pin_SET;
            if (g_pin_mode[i] == GPIO_MODE_OUTPUT_OD && g_pin_odr[i] == 0u) return GPIO_Pin_RESET;
            return GPIO_Pin_SET;
        }
    }
    return GPIO_Pin_SET;
}
void          UART_Init(UART_TypeDef *p, UART_InitTypeDef *q) { (void)p; (void)q; }
void          SYSCTRL_AHBPeriphReset(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void          SYSCTRL_APBPeriphReset1(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void          SYSCTRL_AHBPeriphClk_Enable(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void          SYSCTRL_APBPeriphClk_Enable1(uint32_t p, FunctionalState s) { (void)p; (void)s; }
void          SYSCTRL_GotoDeepSleep(void) { }

/* ==================================================================== */
/* 同级模块桩                                                            */
/* ==================================================================== */
static int      g_enc_calls = 0;
static int16_t  g_enc_temp  = 0;
static uint16_t g_enc_hum   = 0;
static uint16_t g_enc_len   = 0;
static uint8_t  g_tx_ok     = 1u;   /* 1=发送成功, 0=失败 */
static int      g_tx_calls  = 0;

uint8_t app_um2005C_send_data_timeout(uint8_t *d, uint16_t l, uint32_t t)
{
    (void)d; (void)t;
    g_tx_calls++;
    g_enc_len = l;
    return g_tx_ok;
}

void encode_frame10(uint8_t u[10], int16_t t, uint16_t h, uint8_t o[10])
{
    (void)u; (void)o;
    g_enc_calls++;
    g_enc_temp = t;
    g_enc_hum  = h;
}

/* ==================================================================== */
/* 可控的 GXHT40 / 光照桩 + 调用顺序记录                                  */
/* ==================================================================== */
static gxht40_status_t g_status = GXHT40_OK;
static int16_t         g_t      = 0;
static uint16_t        g_h      = 0;
static light_result_t  g_light;              /* T2: light_sample() 返回结构化结果 */
static char            g_order[16];
static int             g_ordn   = 0;

gxht40_status_t gxht40_measure(int16_t *t, uint16_t *h)
{
    if (g_ordn < 14) g_order[g_ordn++] = 'T';
    g_order[g_ordn] = '\0';
    if (g_status != GXHT40_OK) {
        return g_status;            /* 失败: 不写输出 (与真实驱动一致) */
    }
    *t = g_t;
    *h = g_h;
    return GXHT40_OK;
}

/* T1 (FWR-116): 诊断快照桩 (真实驱动为只读 getter) */
static gxht40_diag_t g_diag;

void gxht40_diag_fetch(gxht40_diag_t *out)
{
    if (out != NULL) {
        *out = g_diag;
    }
}

void delay_ms(uint16_t ms) { (void)ms; }   /* 测试桩: 不真正等待 */

void gxht40_init(i2c_dev *dev) { (void)dev; }

light_result_t light_sample(void)
{
    if (g_ordn < 14) g_order[g_ordn++] = 'L';
    g_order[g_ordn] = '\0';
    return g_light;
}

void light_init(void) { }

/* ==================================================================== */
/* 测试                                                                  */
/* ==================================================================== */
static int pass, fail;
#define CHECK(c, m) do { if (c) { pass++; printf("  PASS  %s\n", m); } \
                         else   { fail++; printf("  FAIL  %s\n", m); } } while (0)

static void cycle(gxht40_status_t st, int16_t t, uint16_t h, bool dark)
{
    g_status = st; g_t = t; g_h = h;
    /* 流程测试只关心 dark 门控; 把一份自洽的光照结果整体置为对应状态 */
    g_light.valid         = dark;
    g_light.adc_ok        = true;
    g_light.samples_ok    = 8u;
    g_light.mean_adc_code = dark ? 2000u : 100u;
    g_light.code_min      = g_light.mean_adc_code;
    g_light.code_max      = g_light.mean_adc_code;
    g_light.dark          = dark;
    g_ordn = 0; g_order[0] = '\0';
    sample_flag = 1u;
    (void)temperature_process();
}

int main(void)
{
    printf("[1] 首样本: 先光照后温湿度; 无前值且未超温 -> 不上报\n");
    report_req = 0u;
    cycle(GXHT40_OK, 200, 500, true);
    CHECK(strcmp(g_order, "LT") == 0, "调用顺序 = light_sample -> gxht40_measure (\"LT\")");
    CHECK(tempvalue == 200 && huminityvalue == 500, "成功样本写入 tempvalue/huminityvalue");
    CHECK(report_req == 0u, "无前值 + cur=200 -> report_req=0");

    printf("[2] 下降恰好 0.9C (9) -> 不触发\n");
    cycle(GXHT40_OK, 191, 501, true);
    CHECK(report_req == 0u, "prev=200,cur=191 (drop 9) -> report_req=0");
    CHECK(huminityvalue == 501, "成功时更新最近有效湿度");

    printf("[3] 下降 1.0C (10) 且无光 -> 触发; 待上报不被后续无条件下周期清除\n");
    cycle(GXHT40_OK, 181, 502, true);
    CHECK(report_req == 1u, "prev=191,cur=181 (drop 10) 且 DARK -> report_req=1");
    cycle(GXHT40_OK, 181, 502, true);          /* 无上报条件 */
    CHECK(report_req == 1u, "待上报标志保持 (不被无条件下周期清除)");
    report_req = 0u;

    printf("[4] 下降满足但有光 -> 不触发\n");
    cycle(GXHT40_OK, 170, 503, false);
    CHECK(report_req == 0u, "prev=181,cur=170 (drop 11) 但 LIT -> report_req=0");

    printf("[5] 整周期失败: 不上报/不更新前值/不构造 0 值\n");
    report_req = 0u;
    cycle(GXHT40_ERR_CRC, 50, 999, true);
    CHECK(report_req == 0u, "失败周期不置 report_req");
    CHECK(tempvalue == 170, "tempvalue 保持上次有效值 170 (未被失败的 50 覆盖)");
    CHECK(huminityvalue == 503, "huminityvalue 保持上次有效值 503");
    CHECK(strcmp(g_order, "LT") == 0, "失败周期仍先光照后温湿度");

    printf("[6] 失败不污染前值: 恢复后按失败前的前值判定\n");
    cycle(GXHT40_OK, 160, 504, true);
    CHECK(report_req == 1u, "prev=170,cur=160 (drop 10) -> 上报 (证明失败周期未把 prev 改成 50)");

    printf("[7] 超温分支不受前值/光照限制\n");
    report_req = 0u;
    cycle(GXHT40_OK, 351, 505, false);
    CHECK(report_req == 1u, "cur=351 -> report_req=1");

    printf("[8] sample_flag==0 时不采样\n");
    sample_flag = 0u;
    g_ordn = 0; g_order[0] = '\0';
    (void)temperature_process();
    CHECK(g_order[0] == '\0', "无采样动作 (未调用 light/temp)");

    printf("[9] 条件上报发送路径 (ITEM-008)\n");
    /* 当前最近有效样本 = (351,505) (周期 7) */
    report_req = 0u;
    g_enc_calls = 0; g_tx_calls = 0;
    send_data_to_gateway();
    CHECK(g_enc_calls == 0 && g_tx_calls == 0, "report_req=0 -> 不编码不发送");

    report_req = 1u;
    g_enc_calls = 0; g_tx_calls = 0; g_tx_ok = 1u;
    send_data_to_gateway();
    CHECK(g_enc_calls == 1 && g_tx_calls == 1, "report_req=1 -> 编码并发送一次");
    CHECK(g_enc_temp == 351 && g_enc_hum == 505, "编码使用最近一次有效样本 (351/505)");
    CHECK(g_enc_len == SENSOR_RF_FRAME_LEN, "发送长度为 SENSOR_RF_FRAME_LEN(10)");
    CHECK(report_req == 0u, "发送成功后清除待上报状态");

    report_req = 1u;
    g_tx_ok = 0u; g_tx_calls = 0; g_enc_calls = 0;
    send_data_to_gateway();
    CHECK(g_tx_calls == 1 && report_req == 1u, "发送失败: 保留待上报 (重试)");
    send_data_to_gateway();
    CHECK(g_tx_calls == 2 && report_req == 1u, "第二次失败: 仍保留待上报");
    send_data_to_gateway();
    CHECK(g_tx_calls == 3 && report_req == 0u, "达到 SENSOR_RF_TX_RETRY(3) 后放弃本轮");
    g_tx_ok = 1u;
    report_req = 1u;
    g_tx_calls = 0;
    send_data_to_gateway();
    CHECK(g_tx_calls == 1 && report_req == 0u, "放弃后下一轮重试计数已复位 (1 次即成功)");

    printf("[10] T1 诊断快照集成: 失败周期带 G 行数据 / 成功周期不带 (FWR-116)\n");
    report_req = 0u;
    memset(&g_diag, 0, sizeof(g_diag));
    g_diag.status = (uint8_t)GXHT40_ERR_CRC;
    g_diag.ack44 = 1u; g_diag.read_retry = 2u; g_diag.attempt = (uint8_t)GXHT40_MEAS_RETRY;
    g_diag.raw_valid = 1u; g_diag.raw[0] = 0x66u; g_diag.raw[1] = 0x66u; g_diag.raw[2] = 0x00u;
    cycle(GXHT40_ERR_CRC, 50, 999, true);
    {
        debug_trace_sample_t tr;
        CHECK(sensor_trace_fetch(&tr) == 1u, "失败周期有轨迹快照");
        CHECK(tr.diag_valid == 1u && tr.diag_status == (uint8_t)GXHT40_ERR_CRC,
              "失败周期 diag_valid=1 且结果码来自驱动诊断快照");
        CHECK(tr.diag_ack44 == 1u && tr.diag_ack45 == 0u && tr.diag_read_retry == 2u &&
              tr.diag_attempt == (uint8_t)GXHT40_MEAS_RETRY && tr.diag_raw_valid == 1u &&
              tr.diag_raw[0] == 0x66u && tr.diag_raw[2] == 0x00u,
              "失败周期诊断字段 (地址 ACK/重读/重测/原始字节) 逐项传递到轨迹");
        CHECK(tr.sample_ok == 0u, "失败周期 sample_ok=0 (与 G 行出现条件一致)");
    }
    cycle(GXHT40_OK, 160, 504, true);
    {
        debug_trace_sample_t tr;
        CHECK(sensor_trace_fetch(&tr) == 1u, "成功周期有轨迹快照");
        CHECK(tr.sample_ok == 1u && tr.diag_valid == 0u,
              "成功周期 sample_ok=1 且 diag_valid=0 (不打印 G 行)");
    }
    report_req = 0u;

    printf("[11] T1 上电总线诊断扫描 (只观测; 不写命令/不改业务状态)\n");
    {
        debug_trace_bus_t bus;
        memset(&bus, 0xFF, sizeof(bus));
        CHECK(sensor_bus_diag_scan(&bus) == 1u, "扫描完成并填充结果");
        CHECK(bus.idle == 0x03u && bus.ack_count == 0u && bus.ack_truncated == 0u,
              "主机 mock 总线两线均高 (idle=3), 无器件应答 (ack_count=0)");
        CHECK(sensor_bus_diag_scan(&bus) == 1u, "可重复调用 (bsp_i2c_init 幂等, 不重复注册对象)");
        CHECK(temp_ptr == i2c_obj_find("i2c0"), "幂等初始化后仍能按名找到同一 I2C 对象");
    }

    printf("[12] E1 上电 I/O 自检: 主机拉低/回读 + 释放 + 角色对调 (E1)\n");
    {
        debug_trace_iotest_t io;
        memset(&io, 0xFF, sizeof(io));
        CHECK(sensor_io_diag_scan(&io) == 1u, "I/O 自检完成并填充结果");
        CHECK(io.sda_lo == 0u && io.scl_lo == 0u,
              "主机能把 SDA(PA04) 与 SCL(PA03) 拉低并回读为 0 (开漏模型)");
        CHECK(io.idle == 0x03u, "释放后两线回读为高 (idle=3)");
        CHECK(io.swap_count == 0u, "mock 总线无器件: 角色对调后仍无 ACK");

        /* 负对照: 引脚无法拉低/回读恒高 (H5 形态) -> 字段必须区分得出来 */
        g_pin_stuck_high[3] = 1u;
        (void)sensor_io_diag_scan(&io);
        CHECK(io.sda_lo == 0u && io.scl_lo == 1u,
              "负对照: PA03 回读恒高时 scl_lo=1 而 sda_lo=0 (能区分主机拉不低)");
        g_pin_stuck_high[3] = 0u;
    }

    printf("[13] E1b 事务内逐位回读签名 (IOSIG): 期望值与 H12 负对照\n");
    {
        debug_trace_iosig_t sig;
        memset(&sig, 0, sizeof(sig));
        CHECK(sensor_io_sig_scan(&sig) == 1u, "签名扫描完成并填充结果");
        CHECK(sig.scl20 == 0x95555u && sig.sda20 == 0x30303u,
              "无器件时签名 = scl=95555 / sda=30303 (地址 0x88 位序列与时钟均回读跟随)");

        /* 负对照: SDA 回读恒高 (位序列未真实送达) -> 签名必须不同 -> 命中 H12 形态 */
        g_pin_stuck_high[4] = 1u;
        (void)sensor_io_sig_scan(&sig);
        CHECK(sig.scl20 == 0x95555u && sig.sda20 == 0xFFFFFu,
              "负对照: SDA 回读恒高时 sda=FFFFF ≠ 30303 (能区分 H12)");
        g_pin_stuck_high[4] = 0u;
    }

    printf("\n==== result: %d passed, %d failed ====\n", pass, fail);
    return fail ? 1 : 0;
}
