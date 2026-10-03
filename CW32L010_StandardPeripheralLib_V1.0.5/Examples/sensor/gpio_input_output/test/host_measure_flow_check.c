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
#include "sensor_config.h"
#include "cw32l010_gpio.h"
#include "cw32l010_sysctrl.h"
#include "cw32l010_uart.h"

/* ---- 被采样流程调用的全局量 ---- */
extern int16_t  tempvalue;
extern uint16_t huminityvalue;
extern uint8_t  sample_flag;

/* ==================================================================== */
/* MCU 层桩 (影子头声明的函数)                                            */
/* ==================================================================== */
void          GPIO_Init(GPIO_TypeDef *p, GPIO_InitTypeDef *q) { (void)p; (void)q; }
void          GPIO_WritePin(GPIO_TypeDef *p, uint16_t pin, GPIO_PinState s) { (void)p; (void)pin; (void)s; }
GPIO_PinState GPIO_ReadPin(GPIO_TypeDef *p, uint16_t pin) { (void)p; (void)pin; return GPIO_Pin_SET; }
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

bool optcfg_window_active(void) { return false; }
bool hall_event_pending(void) { return false; }

/* ==================================================================== */
/* 可控的 GXHT40 / 光照桩 + 调用顺序记录                                  */
/* ==================================================================== */
static gxht40_status_t g_status = GXHT40_OK;
static int16_t         g_t      = 0;
static uint16_t        g_h      = 0;
static bool            g_dark   = false;
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

void gxht40_init(i2c_dev *dev) { (void)dev; }

bool light_sample(void)
{
    if (g_ordn < 14) g_order[g_ordn++] = 'L';
    g_order[g_ordn] = '\0';
    return g_dark;
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
    g_status = st; g_t = t; g_h = h; g_dark = dark;
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

    printf("\n==== result: %d passed, %d failed ====\n", pass, fail);
    return fail ? 1 : 0;
}
