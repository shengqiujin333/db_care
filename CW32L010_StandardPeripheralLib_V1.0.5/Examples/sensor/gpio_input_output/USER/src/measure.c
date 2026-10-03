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
#include "cw32l010_uart.h"
#include "app_um2005c.h"
#include "encrytogate.h"
#include "optcfg.h"      /* optcfg_window_active(): 配置窗口内延后上报 (ITEM-009 退役) */
#include "hall.h"        /* hall_event_pending(): 睡眠门控 (ITEM-009 退役) */

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
static bool     s_last_dark     = false;/* 最近一次光照判据 (true=无光) */
static bool     s_sensor_ready  = false;/* 驱动/光照是否已初始化 */

/* ==================================================================== */
/* 调试 UART (保留既有函数; 采样路径不再使用, 保持关闭)                    */
/* ==================================================================== */
void DebugUART_Close(void)
{
    SYSCTRL_AHBPeriphReset(SYSCTRL_AHB_PERIPH_GPIOA, ENABLE);
    SYSCTRL_AHBPeriphReset(SYSCTRL_AHB_PERIPH_GPIOA, DISABLE);
    SYSCTRL_APBPeriphReset1(SYSCTRL_APB1_PERIPH_UART1, ENABLE);
    SYSCTRL_APBPeriphReset1(SYSCTRL_APB1_PERIPH_UART1, DISABLE);

    SYSCTRL_AHBPeriphClk_Enable(SYSCTRL_AHB_PERIPH_GPIOA, DISABLE);
    SYSCTRL_APBPeriphClk_Enable1(SYSCTRL_APB1_PERIPH_UART1, DISABLE);
}

void UartGPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.Pins = DEBUG_UART_TX_GPIO_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Init(DEBUG_UART_TX_GPIO_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.Pins = DEBUG_UART_RX_GPIO_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT_PULLUP;
    GPIO_Init(DEBUG_UART_RX_GPIO_PORT, &GPIO_InitStructure);

    DEBUG_UART_AFTX;
    DEBUG_UART_AFRX;
}

void UART1_Configure(void)
{
    UART_InitTypeDef UART_InitStructure = {0};

    UartGPIO_Configuration();

    UART_InitStructure.UART_BaudRate = DEBUG_UART_BaudRate;
    UART_InitStructure.UART_Over = UART_Over_16;
    UART_InitStructure.UART_Source = UART_Source_PCLK;
    UART_InitStructure.UART_UclkFreq = DEBUG_UART_UclkFreq;
    UART_InitStructure.UART_StartBit = UART_StartBit_FE;
    UART_InitStructure.UART_StopBits = UART_StopBits_1;
    UART_InitStructure.UART_Parity = UART_Parity_No;
    UART_InitStructure.UART_HardwareFlowControl = UART_HardwareFlowControl_None;
    UART_InitStructure.UART_Mode = UART_Mode_Rx | UART_Mode_Tx;
    UART_Init(DEBUG_UARTx, &UART_InitStructure);
}

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
    bool     dark;

    if (!s_sensor_ready) {
        bsp_i2c_init();          /* 软 I2C 物理层/对象 */
        gxht40_init(temp_ptr);   /* 绑定 GXHT40 (地址探测缓存清零) */
        light_init();            /* PB05 输出低 + PB04 模拟输入(AIN11) + ADC 配置 */
        s_sensor_ready = true;
    }

    dark = light_sample();                         /* 1) 光照: PB05 高 -> 稳定 -> ADC 均值 -> PB05 低 */

    if (gxht40_measure(&t, &h) != GXHT40_OK) {     /* 2) 温湿度: 0xFD + 6B + 双字 CRC, 驱动内有界重试 */
        return false;
    }

    tempvalue     = t;
    huminityvalue = h;
    s_last_dark   = dark;
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

    if (measure_sample()) {
        if (sensor_decide_report(s_prev_temp_x10, s_have_prev,
                                 tempvalue, s_last_dark)) {
            report_req = 1u;
        }
        s_prev_temp_x10 = tempvalue;    /* 仅成功后推进 (失败不污染前值) */
        s_have_prev     = true;
        s_last_hum_x10  = huminityvalue;
    }

    sample_flag = 0u;
    return 0u;
}

uint8_t mcu_uid[10];
uint8_t send_data[10];

/* ==================================================================== */
/* 上报发送 (ITEM-008 将对齐本路径; 本项不改动帧布局/加密)                 */
/* ==================================================================== */
void send_data_to_gateway(void)
{
    uint8_t *ptr = &mcu_uid[1];

    if (report_req == 0) {
        return;
    }
    if (optcfg_window_active()) {   /* 配置窗口内延后上报 (ITEM-009 退役) */
        return;
    }

    send_data[0] = ptr[0];
    send_data[1] = ptr[3];
    send_data[2] = ptr[6];
    send_data[3] = ptr[8];

    send_data[4] = (tempvalue >> 8) & 0xff;       /* 温度 int16 小端 */
    send_data[5] = tempvalue & 0xff;

    send_data[6] = (huminityvalue >> 8) & 0xff;   /* 湿度 uint16 小端 */
    send_data[7] = huminityvalue & 0xff;

    encode_frame10(mcu_uid, tempvalue, huminityvalue, send_data);
    if (app_um2005C_send_data_timeout(send_data, 10, SENSOR_RF_TX_TIMEOUT_MS)) {
        report_req  = 0;               /* 仅成功后清除待上报状态 */
        report_retry = 0;
    } else {
        report_retry++;                /* 失败不记成功 */
        if (report_retry >= SENSOR_RF_TX_RETRY) {
            report_req  = 0;           /* 本轮放弃; 下次 3 分钟周期自然重试 */
            report_retry = 0;
        }
    }
}

void go_to_sleep(void)
{
    if ((report_req == 0) && (sample_flag == 0) &&
        (!hall_event_pending()) && (!optcfg_window_active())) {
        SYSCTRL_GotoDeepSleep();
    }
}
