/*
 * light.c - 光照通路 (readme 修改点 2; FD-002 §2.3/§3.1/§6.3/§10)
 *
 * 引脚所有权 (FD-002 §11.4): PB04 只在本文件被配置为模拟输入(AIN11),
 * PB05 只在本文件被配置为输出; 其它模块不得再配置 PB04/PB05/PB06。
 *
 * 低功耗 (FD-002 §11.5): 非采样期 PB05 = 低(分压无电流), ADC 使能位关闭。
 */
#include "light.h"
#include "sensor_config.h"
#include "fw_core.h"      /* 纯滞回判定 light_code_is_dark (ITEM-005) */
#include "delay.h"

#include "cw32l010_gpio.h"
#include "cw32l010_adc.h"
#include "cw32l010_sysctrl.h"

static bool     s_dark_state = false;   /* 上电默认 LIT; 首个样本按进入阈值判定 */
static uint16_t s_last_code  = 0u;

/* T1: 最近一次采样的原始样本统计 (供 UART1 调试轨迹; 不参与判定) */
static uint8_t  s_last_ok   = 0u;
static uint16_t s_last_mean = 0u;
static uint16_t s_last_min  = 0u;
static uint16_t s_last_max  = 0u;

void light_reset_state(void)
{
    s_dark_state = false;
    s_last_code  = 0u;
}

/* ------------------------------------------------------------------ */
/* ADC 通路                                                            */
/* ------------------------------------------------------------------ */
static void light_adc_config(void)
{
    ADC_InitTypeDef adc = {0};

    __SYSCTRL_ADC_CLK_ENABLE();

    adc.ADC_ClkDiv      = LIGHT_ADC_CLK_DIV;                  /* ADCCLK = PCLK/8 = 1 MHz */
    adc.ADC_ConvertMode = ADC_ConvertMode_Once;               /* 单次转换 */
    adc.ADC_SQREns      = ADC_SqrEns0to0;                     /* 仅 IN0 通道 */
    adc.ADC_IN0.ADC_InputChannel = LIGHT_ADC_INPUT_CHANNEL;   /* PB04 = AIN11 */
    adc.ADC_IN0.ADC_SampTime     = LIGHT_ADC_SAMPLE_TIME;     /* 390 clk = 390 us, 适配 5M 源阻抗 */
    ADC_Init(&adc);
    ADC_ClearITPendingAll();
    ADC_Disable();                                            /* 非采样期关闭 */
}

void light_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __SYSCTRL_GPIOB_CLK_ENABLE();

    /* PB05 = LIGTHT_POWER: 推挽输出, 初始低 */
    gpio.Pins = LIGHT_POWER_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.IT   = GPIO_IT_NONE;
    GPIO_Init(LIGHT_POWER_PORT, &gpio);
    GPIO_WritePin(LIGHT_POWER_PORT, LIGHT_POWER_PIN, GPIO_Pin_RESET);

    /* PB04 = LIGHT_ADC: 模拟输入 (GPIO_Init 置 ANALOG 位, 关闭数字输入) */
    gpio.Pins = LIGHT_ADC_PIN;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.IT   = GPIO_IT_NONE;
    GPIO_Init(LIGHT_ADC_PORT, &gpio);

    light_adc_config();
    light_reset_state();
}

/* 单次 ADC 转换; 返回 false = 未在有限时间内完成 (不无限等待) */
static bool light_adc_read_once(uint16_t *out)
{
    uint32_t guard = LIGHT_ADC_EOC_GUARD;

    ADC_SoftwareStartConvCmd(ENABLE);
    while ((CW_ADC->ISR & ADC_ISR_EOC_Msk) == 0u) {
        if (guard == 0u) {
            return false;
        }
        guard--;
    }
    ADC_ClearITPendingBit(ADC_IT_EOC);
    *out = ADC_GetConversionValue(0);
    return true;
}

bool light_sample(void)
{
    uint32_t sum = 0u;
    uint8_t  i;
    uint8_t  ok = 0u;
    uint16_t code;
    uint16_t vmin = 0u;
    uint16_t vmax = 0u;

    /* 分压供电: PB05 = VDD, 等待 RC 建立与光敏器件响应 */
    GPIO_WritePin(LIGHT_POWER_PORT, LIGHT_POWER_PIN, GPIO_Pin_SET);
    delay_ms(LIGHT_SETTLE_MS);

    ADC_Enable();
    for (i = 0u; i < LIGHT_ADC_SAMPLES; i++) {
        uint16_t v = 0u;
        if (light_adc_read_once(&v)) {
            if (ok == 0u) {
                vmin = v;
                vmax = v;
            } else {
                if (v < vmin) { vmin = v; }
                if (v > vmax) { vmax = v; }
            }
            sum += (uint32_t)v;
            ok++;
        }
    }
    ADC_Disable();

#if LIGHT_IDLE_POWER_OFF
    GPIO_WritePin(LIGHT_POWER_PORT, LIGHT_POWER_PIN, GPIO_Pin_RESET);   /* 采样结束 PB05 低 */
#endif

    /* T1: 记录原始样本统计 (只读观测, 不改变判定) */
    s_last_ok = ok;
    if (ok == 0u) {
        s_last_mean = 0u;
        s_last_min  = 0u;
        s_last_max  = 0u;
        /* 全部转换超时: 保留既有满量程回退行为 (D-02 由 T2 修正为 valid=false) */
        code = LIGHT_ADC_FULL_SCALE;
    } else {
        s_last_mean = (uint16_t)(sum / (uint32_t)ok);
        s_last_min  = vmin;
        s_last_max  = vmax;
        code = s_last_mean;
    }

    s_last_code  = code;
    s_dark_state = light_code_is_dark(code, s_dark_state);
    return s_dark_state;
}

uint16_t light_last_code(void)
{
    return s_last_code;
}

uint8_t light_last_ok(void)
{
    return s_last_ok;
}

uint16_t light_last_mean(void)
{
    return s_last_mean;
}

uint16_t light_last_min(void)
{
    return s_last_min;
}

uint16_t light_last_max(void)
{
    return s_last_max;
}
