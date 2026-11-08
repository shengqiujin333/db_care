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
#define ENC_CAP 8
static int16_t  g_enc_temps[ENC_CAP];
static uint16_t g_enc_hums[ENC_CAP];
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
    if ((g_enc_calls >= 0) && (g_enc_calls < ENC_CAP)) {
        g_enc_temps[g_enc_calls] = t;
        g_enc_hums[g_enc_calls]  = h;
    }
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

    printf("[2] 升温恰好 0.9C (9) -> 不触发\n");
    cycle(GXHT40_OK, 209, 501, true);
    CHECK(report_req == 0u, "prev=200,cur=209 (rise 9) -> report_req=0");
    CHECK(huminityvalue == 501, "成功时更新最近有效湿度");

    printf("[3] 升温 1.1C (11) 且无光 -> 触发并冻结当次样本\n");
    cycle(GXHT40_OK, 220, 502, true);
    CHECK(report_req == 1u, "prev=209,cur=220 (rise 11) 且 DARK -> report_req=1");

    printf("[4] 待上报未发送期间的新触发不得覆盖冻结样本\n");
    cycle(GXHT40_OK, 231, 503, true);          /* 新触发, 但 req 已置位 */
    CHECK(report_req == 1u, "待上报标志保持 (不被无条件下周期清除)");
    g_enc_calls = 0; g_tx_calls = 0; g_tx_ok = 1u;
    send_data_to_gateway();
    CHECK(g_enc_calls == 1 && g_enc_temps[0] == 220 && g_enc_hums[0] == 502,
          "组帧仍使用首次触发的冻结样本 (220/502), 不被新采样 231/503 覆盖");
    CHECK(report_req == 0u, "发送成功后清除待上报状态");

    printf("[5] 升温满足但有光 -> 不触发\n");
    cycle(GXHT40_OK, 250, 504, false);
    CHECK(report_req == 0u, "prev=231,cur=250 (rise 19) 但 LIT -> report_req=0");

    printf("[6] 整周期失败: 不上报/不更新前值/不构造 0 值\n");
    report_req = 0u;
    cycle(GXHT40_ERR_CRC, 50, 999, true);
    CHECK(report_req == 0u, "失败周期不置 report_req");
    CHECK(tempvalue == 250, "tempvalue 保持上次有效值 250 (未被失败的 50 覆盖)");
    CHECK(huminityvalue == 504, "huminityvalue 保持上次有效值 504");
    CHECK(strcmp(g_order, "LT") == 0, "失败周期仍先光照后温湿度");

    printf("[7] 失败不污染前值: 恢复后按失败前的前值判定\n");
    cycle(GXHT40_OK, 261, 505, true);
    CHECK(report_req == 1u, "prev=250,cur=261 (rise 11) -> 上报 (证明失败周期未把 prev 改成 50)");

    printf("[8] 超温分支不受前值/光照限制\n");
    report_req = 0u;
    cycle(GXHT40_OK, 351, 506, false);
    CHECK(report_req == 1u, "cur=351 -> report_req=1");

    printf("[9] sample_flag==0 时不采样\n");
    sample_flag = 0u;
    g_ordn = 0; g_order[0] = '\0';
    (void)temperature_process();
    CHECK(g_order[0] == '\0', "无采样动作 (未调用 light/temp)");

    printf("[10] 条件上报发送路径与冻结/重试语义\n");
    /* 当前冻结样本 = (351,506) (周期 8) */
    report_req = 0u;
    g_enc_calls = 0; g_tx_calls = 0;
    send_data_to_gateway();
    CHECK(g_enc_calls == 0 && g_tx_calls == 0, "report_req=0 -> 不编码不发送");

    report_req = 1u;
    g_enc_calls = 0; g_tx_calls = 0; g_tx_ok = 1u;
    send_data_to_gateway();
    CHECK(g_enc_calls == 1 && g_tx_calls == 1, "report_req=1 -> 编码并发送一次");
    CHECK(g_enc_temp == 351 && g_enc_hum == 506, "编码使用冻结快照 (351/506)");
    CHECK(g_enc_len == SENSOR_RF_FRAME_LEN, "发送长度为 SENSOR_RF_FRAME_LEN(10)");
    CHECK(report_req == 0u, "发送成功后清除待上报状态");

    /* 新触发 -> 冻结 (362,600); 连续失败期间跨新采样仍用同一快照 */
    report_req = 0u;
    cycle(GXHT40_OK, 362, 600, true);
    CHECK(report_req == 1u, "prev=351,cur=362 (rise 11) -> 新触发");
    g_tx_ok = 0u; g_tx_calls = 0; g_enc_calls = 0;
    send_data_to_gateway();
    CHECK(g_tx_calls == 1 && report_req == 1u, "发送失败: 保留待上报 (重试)");
    CHECK(g_enc_temps[0] == 362 && g_enc_hums[0] == 600, "首次尝试使用冻结样本 (362/600)");

    cycle(GXHT40_OK, 373, 601, true);   /* 待上报期间的新采样 */
    CHECK(report_req == 1u, "待上报期间新采样不改变待上报标志");
    send_data_to_gateway();
    CHECK(g_tx_calls == 2 && report_req == 1u, "第二次失败: 仍保留待上报");
    CHECK(g_enc_temps[1] == 362 && g_enc_hums[1] == 600,
          "第二次尝试仍使用冻结样本 (362/600), 不随新采样 373/601 改变");
    send_data_to_gateway();
    CHECK(g_tx_calls == 3 && report_req == 0u, "达到 SENSOR_RF_TX_RETRY(3) 后放弃本轮");
    CHECK(g_enc_temps[2] == 362 && g_enc_hums[2] == 600, "第三次尝试仍使用同一冻结样本");

    g_tx_ok = 1u; g_tx_calls = 0; g_enc_calls = 0;
    send_data_to_gateway();
    CHECK(g_tx_calls == 0 && g_enc_calls == 0, "放弃后无待上报: 不再发送");

    report_req = 0u;
    cycle(GXHT40_OK, 385, 602, true);
    CHECK(report_req == 1u, "prev=373,cur=385 (rise 12) -> 新触发");
    g_tx_calls = 0; g_enc_calls = 0;
    send_data_to_gateway();
    CHECK(g_enc_temps[0] == 385 && g_enc_hums[0] == 602 && report_req == 0u,
          "放弃后下一轮重新冻结新样本 (385/602) 并成功发送");

    printf("[11] T1 诊断快照集成: 失败周期带 G 行数据 / 成功周期不带 (FWR-116)\n");
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

    printf("[12] T1 上电总线诊断扫描 (只观测; 不写命令/不改业务状态)\n");
    {
        debug_trace_bus_t bus;
        memset(&bus, 0xFF, sizeof(bus));
        CHECK(sensor_bus_diag_scan(&bus) == 1u, "扫描完成并填充结果");
        CHECK(bus.idle == 0x03u && bus.ack_count == 0u && bus.ack_truncated == 0u,
              "主机 mock 总线两线均高 (idle=3), 无器件应答 (ack_count=0)");
        CHECK(sensor_bus_diag_scan(&bus) == 1u, "可重复调用 (bsp_i2c_init 幂等, 不重复注册对象)");
        CHECK(temp_ptr == i2c_obj_find("i2c0"), "幂等初始化后仍能按名找到同一 I2C 对象");
    }

    printf("[13] E1 上电 I/O 自检: 主机拉低/回读 + 释放 + 角色对调 (E1)\n");
    {
        debug_trace_iotest_t io;
        memset(&io, 0xFF, sizeof(io));
        CHECK(sensor_io_diag_scan(&io) == 1u, "I/O 自检完成并填充结果");
        CHECK(io.sda_lo == 0u && io.scl_lo == 0u,
              "主机能把 SDA(PA04) 与 SCL(PA03) 拉低并回读为 0 (开漏模型)");
        CHECK(io.idle == 0x03u, "释放后两线回读为高 (idle=3)");
        CHECK(io.swap_count == 0u, "mock 总线无器件: 角色对调后仍无 ACK");
        CHECK(io.gc == 3u,
              "无器件应答时 gc=3 (0x00 也不 ACK, 未发出复位命令; 有界且不反复复位)");

        /* 负对照: 引脚无法拉低/回读恒高 (H5 形态) -> 字段必须区分得出来 */
        g_pin_stuck_high[3] = 1u;
        (void)sensor_io_diag_scan(&io);
        CHECK(io.sda_lo == 0u && io.scl_lo == 1u,
              "负对照: PA03 回读恒高时 scl_lo=1 而 sda_lo=0 (能区分主机拉不低)");
        g_pin_stuck_high[3] = 0u;
    }

    printf("[14] E1b 事务内逐位回读签名 (IOSIG): 期望值与 H12 负对照\n");
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

    printf("[15] T2 P 行数据: 冻结样本/发送尝试/打印时待上报状态\n");
    report_req = 0u;
    g_tx_ok = 1u;
    cycle(GXHT40_OK, 400, 700, false);      /* 超温分支触发 */
    CHECK(report_req == 1u, "cur=400 (>350) 且 LIT -> 仍触发, 冻结 (400/700)");
    g_tx_calls = 0;
    send_data_to_gateway();
    {
        debug_trace_sample_t tr;
        CHECK(sensor_trace_fetch(&tr) == 1u, "上报周期有轨迹快照");
        CHECK(tr.report == 1u && tr.send_attempted == 1u && tr.p_rq == 0u,
              "P 行条件成立 (report=1/send_attempted=1), 打印时 rq=0 (已发送成功)");
        CHECK(tr.p_temp_x10 == 400 && tr.p_hum_x10 == 700u,
              "P 行 t/h = 组帧实际使用的冻结样本 (400/700)");
        CHECK(tr.send == 1u, "P 行 s=1 (发送成功)");
    }
    report_req = 0u;

    printf("\n==== result: %d passed, %d failed ====\n", pass, fail);
    return fail ? 1 : 0;
}
