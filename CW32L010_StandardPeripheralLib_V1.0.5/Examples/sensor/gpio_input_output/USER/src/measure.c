/*
 * measure.c - 传感器采样流程 (readme 修改点 1/2/4; FD-002 §4/§6/§10)
 *
 * 每个采样周期: 光照 (light_sample) -> 温湿度 (gxht40_measure) -> 状态推进。
 *   成功: 更新前一有效温度与最近有效湿度, 并按 sensor_decide_report 置待上报标志。
 *   失败: 不上报、不更新前一有效温度、不构造 0 值 (tempvalue/huminityvalue 保持上次有效值)。
 * GXHT40 的读 NACK / CRC 错有界重试在驱动 gxht40_measure() 内按 sensor_config.h 上限完成。
 *
 * 边界: 3 分钟节拍属 ITEM-007; 433 发送路径对齐属 ITEM-008; 旧 hall/OPTCFG/params/history
 *       退役属 ITEM-009。本文件不再使用 AHT21 序列。
 */
#include "measure.h"
#include "sf_i2c.h"
#include "gxht40.h"
#include "light.h"
#include "fw_core.h"
#include "sensor_config.h"
#include "cw32l010_gpio.h"
#include "cw32l010_sysctrl.h"
#include "app_um2005c.h"
#include "encrytogate.h"

/* ==================================================================== */
/* 软 I2C 端口 (PA04 = SDA, PA03 = SCL)                                  */
/* ==================================================================== */
void i2c0_sda_pin_out_low(void)
{
    GPIO_WritePin(CW_GPIOA, GPIO_PIN_4, GPIO_Pin_RESET);
}

void i2c0_sda_pin_out_high(void)
{
    GPIO_WritePin(CW_GPIOA, GPIO_PIN_4, GPIO_Pin_SET);
}

void i2c0_scl_pin_out_low(void)
{
    GPIO_WritePin(CW_GPIOA, GPIO_PIN_3, GPIO_Pin_RESET);
}

void i2c0_scl_pin_out_high(void)
{
    GPIO_WritePin(CW_GPIOA, GPIO_PIN_3, GPIO_Pin_SET);
}

uint8_t i2c0_sda_pin_read_level(void)
{
    return GPIO_ReadPin(CW_GPIOA, GPIO_PIN_4) ? 1u : 0u;
}

void i2c0_sda_pin_dir_input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pins = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.IT   = GPIO_IT_NONE;
    GPIO_Init(CW_GPIOA, &GPIO_InitStruct);
}

void i2c0_scl_pin_dir_input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pins = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.IT   = GPIO_IT_NONE;
    GPIO_Init(CW_GPIOA, &GPIO_InitStruct);
}

void i2c0_sda_pin_dir_output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pins = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_Init(CW_GPIOA, &GPIO_InitStruct);
}

static i2c_dev i2c0_dev = {
    .name                    = "i2c0",
    .speed                   = 100,
    .port.sda_pin_out_low    = i2c0_sda_pin_out_low,
    .port.sda_pin_out_high   = i2c0_sda_pin_out_high,
    .port.scl_pin_out_low    = i2c0_scl_pin_out_low,
    .port.scl_pin_out_high   = i2c0_scl_pin_out_high,
    .port.sda_pin_read_level = i2c0_sda_pin_read_level,
    .port.sda_pin_dir_input  = i2c0_sda_pin_dir_input,
    .port.sda_pin_dir_output = i2c0_sda_pin_dir_output,
};

static void i2c0_phy_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __SYSCTRL_GPIOA_CLK_ENABLE();
    __SYSCTRL_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pins = GPIO_PIN_4 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_Init(CW_GPIOA, &GPIO_InitStruct);
}

i2c_dev *temp_ptr = NULL;

void bsp_i2c_init(void)
{
    i2c0_phy_init();
    i2c_init(&i2c0_dev);
    temp_ptr = i2c_obj_find("i2c0");
}

/* ==================================================================== */
/* 采样状态 (深睡保持)                                                    */
/* ==================================================================== */
int16_t  tempvalue     = 0;    /* 当前有效温度 x10 (0.1 C, 有符号) */
uint16_t huminityvalue = 0;    /* 当前有效湿度 x10 (0.1 %RH) */

uint8_t sample_flag = 1;       /* 由 RTC 节拍置位 (ITEM-007) */
uint8_t report_req  = 0;       /* 待上报标志 (置位见 temperature_process) */
static uint8_t report_retry = 0;

static int16_t  s_prev_temp_x10 = 0;    /* 前一有效测量温度 (判定用) */
static bool     s_have_prev     = false;/* 是否已有前一有效样本 */
static uint16_t s_last_hum_x10  = 0;    /* 最近有效湿度 */
static light_result_t s_light;          /* 本周期光照结构化结果 (判定与轨迹的唯一来源; T2) */
static bool     s_sensor_ready  = false;/* 驱动/光照是否已初始化 */

/* T1: UART1 调试轨迹状态 (只读快照, 不参与判定/发送) */
static uint8_t  s_trace_pending = 0u;   /* 本周期存在待发布轨迹 */
static int16_t  s_trace_temp_x10 = 0;   /* 本周期展示用温度 (失败周期为最近有效值) */
static uint16_t s_trace_hum_x10  = 0u;  /* 本周期展示用湿度 */
static uint8_t  s_trace_sample_ok = 0u; /* 本周期温湿度采样是否有效 */
static uint8_t  s_trace_report    = 0u; /* 本周期条件上报判定结果 */
static uint8_t  s_last_send_result = 0u;/* 最近一次 433 发送结果: 0=未发送 1=成功 2=失败 */

/* ==================================================================== */
/* 调试 UART1                                                           */
/* ==================================================================== */
/* UART1 (PA06/PA05, 9600 8N1) 的初始化/打印/关闭全部由 debug_trace.c 拥有
 * (FD-002 rev 4.0 §2.4/§11.15)。本文件不再配置或关闭 UART1, 也不得复位 GPIOA。
 * 采样路径只通过 sensor_trace_fetch() 提供只读轨迹状态, 由 main.c 发布。 */

/* ==================================================================== */
/* 采样                                                                  */
/* ==================================================================== */
/*
 * 一次完整测量: 光照 -> 温湿度。
 * 返回 true = 本次有效; tempvalue/huminityvalue/s_last_dark 已更新。
 * 返回 false = 整周期失败; 不修改 tempvalue/huminityvalue (不构造 0 值)。
 */
static bool measure_sample(void)
{
    int16_t  t = 0;
    uint16_t h = 0;

    if (!s_sensor_ready) {
        bsp_i2c_init();          /* 软 I2C 物理层/对象 */
        gxht40_init(temp_ptr);   /* 绑定 GXHT40 (地址探测缓存清零) */
        light_init();            /* PB05 输出低 + PB04 模拟输入(AIN11) + ADC 配置 */
        s_sensor_ready = true;
    }

    /* 1) 光照: PB05 高 -> 稳定 -> ADC 均值 -> PB05 低; 结构化结果供判定与轨迹共用 */
    s_light = light_sample();

    if (gxht40_measure(&t, &h) != GXHT40_OK) {     /* 2) 温湿度: 0xFD + 6B + 双字 CRC, 驱动内有界重试 */
        return false;
    }

    tempvalue     = t;
    huminityvalue = h;
    return true;
}

/*
 * 采样节拍回调: sample_flag 置位时执行一次采样 (FD-002 §4 数据流)。
 * 成功: 用更新前的 prev 判定上报, 再推进前一有效温度与最近有效湿度。
 * 失败: 不上报、不更新前一有效温度、不构造 0 值。
 */
uint16_t temperature_process(void)
{
    if (sample_flag == 0u) {
        return 0u;
    }

    s_trace_sample_ok = 0u;
    s_trace_report    = 0u;

    if (measure_sample()) {
        uint8_t rep = sensor_decide_report(s_prev_temp_x10, s_have_prev,
                                           tempvalue, s_light.dark) ? 1u : 0u;
        if (rep != 0u) {
            report_req = 1u;
        }
        s_prev_temp_x10 = tempvalue;    /* 仅成功后推进 (失败不污染前值) */
        s_have_prev     = true;
        s_last_hum_x10  = huminityvalue;
        s_trace_sample_ok = 1u;
        s_trace_report    = rep;
    }

    /* 调试轨迹取本周期快照: 失败周期 temp/hum 保持最近有效值, 由 sample_ok=0 标记 */
    s_trace_temp_x10 = tempvalue;
    s_trace_hum_x10  = huminityvalue;
    s_trace_pending  = 1u;

    sample_flag = 0u;
    return 0u;
}

/*
 * T1/T2: 取出本采样周期的只读轨迹快照 (不改变任何业务状态)。
 * 光照字段来自本周期 light_result_t (s_light), 与判定使用同一份数据。
 */
uint8_t sensor_trace_fetch(debug_trace_sample_t *out)
{
    if ((out == NULL) || (s_trace_pending == 0u)) {
        return 0u;
    }
    s_trace_pending = 0u;

    out->tick          = (uint32_t)SENSOR_SAMPLE_TICKS;
    out->prev_temp_x10 = s_prev_temp_x10;
    out->have_prev     = s_have_prev ? 1u : 0u;
    out->light_valid   = s_light.valid ? 1u : 0u;
    out->light_ok      = s_light.samples_ok;
    out->light_min     = s_light.code_min;
    out->light_max     = s_light.code_max;
    out->light_mean    = s_light.mean_adc_code;
    out->light_dark    = s_light.dark ? 1u : 0u;
    out->temp_x10      = s_trace_temp_x10;
    out->hum_x10       = s_trace_hum_x10;
    out->sample_ok     = s_trace_sample_ok;
    out->report        = s_trace_report;
    out->send          = s_last_send_result;
    out->retry         = report_retry;
    return 1u;
}

uint8_t mcu_uid[10];
uint8_t send_data[10];

/* ==================================================================== */
/* 条件上报 (readme 修改点 4; IC-002 §2; FD-002 §6.4/§8.1)                */
/* report_req 由 temperature_process() 按 sensor_decide_report 置位。    */
/* 帧布局/字节序/Feistel 加密均由 encode_frame10() 决定, 本项不变。        */
/* ==================================================================== */
void send_data_to_gateway(void)
{
    if (report_req == 0) {
        s_last_send_result = 0u;       /* 无待上报: 本周期未发送 (调试轨迹用) */
        return;                        /* 不满足判据: 不上报 */
    }

    /* 发送最近一次有效样本 (与触发上报的样本一致): encode_frame10 内部组装
     * uid_pick(4) | temp_x10_LE(2) | hum_x10_LE(2) | crc16_LE(2) 并加密 */
    encode_frame10(mcu_uid, s_prev_temp_x10, s_last_hum_x10, send_data);

    if (app_um2005C_send_data_timeout(send_data, SENSOR_RF_FRAME_LEN,
                                      SENSOR_RF_TX_TIMEOUT_MS)) {
        report_req   = 0;              /* 仅在发送成功后清除待上报状态 */
        report_retry = 0;
        s_last_send_result = 1u;
    } else {
        report_retry++;                /* 失败不记成功 */
        s_last_send_result = 2u;
        if (report_retry >= SENSOR_RF_TX_RETRY) {
            report_req   = 0;          /* 按上限重试用尽, 放弃本轮; 下个周期自然重试 */
            report_retry = 0;
        }
    }
}

void go_to_sleep(void)
{
    if ((report_req == 0u) && (sample_flag == 0u)) {
        SYSCTRL_GotoDeepSleep();
    }
}
