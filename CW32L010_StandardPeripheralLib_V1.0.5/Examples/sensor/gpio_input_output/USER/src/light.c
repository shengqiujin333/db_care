/*
 * light.c - 光照通路 (readme 修改点 2/7; FD-002 rev 4.0 §2.3/§3.1/§6.3/§10)
 *
 * 引脚所有权 (FD-002 §11.4): PB04 只在本文件被配置为模拟输入(AIN11),
 * PB05 只在本文件被配置为输出; 其它模块不得再配置 PB04/PB05/PB06。
 *
 * 低功耗 (FD-002 §11.5): 非采样期 PB05 = 低(分压无电流), ADC 使能位关闭。
 *
 * T2 (rev 4.0): 结果改为结构化 light_result_t{valid,adc_ok,samples_ok,mean,min,max,dark}；
 *   无光判据 = 完全无光基准 1/3 (light_is_dark)；删除旧 350/250 滞回与满量程回退。
 *   未标定 (LIGHT_DARK_CALIBRATED=0) 时 valid=false -> 不得判暗。
 */
#include "light.h"
#include "sensor_config.h"
#include "fw_core.h"      /* 纯逻辑: light_is_dark (FD-002 §6.3) */
#include "delay.h"

#include "cw32l010_gpio.h"
#include "cw32l010_adc.h"
#include "cw32l010_sysctrl.h"

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

#ifndef LIGHT_BGR_TRIM_MV
/*
 * 出厂修调值: 内部 BGR1.2V 的实际电压 (mV), 厂商在 0x001007D2 处标定
 * (同厂商示例 Examples/ADC/adc_sgl_sw_vdd)。宿主机 harness 用
 * -DLIGHT_BGR_TRIM_MV=<mv> 覆盖, 从而不访问绝对地址。
 */
#define LIGHT_BGR_TRIM_MV   (*(volatile const uint16_t *)0x001007D2u)
#endif

uint8_t light_read_vdd_mv(uint16_t *mv_out, uint16_t *code_out, uint16_t *bgr_mv_out)
{
    ADC_InitTypeDef adc = {0};
    uint32_t guard = LIGHT_ADC_EOC_GUARD;
    uint16_t code;
    uint32_t bgr_mv;
    uint32_t mv;

    if ((mv_out == NULL) || (code_out == NULL) || (bgr_mv_out == NULL)) {
        return 0u;
    }
    *mv_out     = 0u;
    *code_out   = 0u;
    *bgr_mv_out = 0u;

    __SYSCTRL_ADC_CLK_ENABLE();

    adc.ADC_ClkDiv      = LIGHT_ADC_CLK_DIV;
    adc.ADC_ConvertMode = ADC_ConvertMode_Once;
    adc.ADC_SQREns      = ADC_SqrEns0to0;
    adc.ADC_IN0.ADC_InputChannel = ADC_InputVref1P2;   /* 内部 BGR1.2V */
    adc.ADC_IN0.ADC_SampTime     = LIGHT_ADC_SAMPLE_TIME;
    ADC_Init(&adc);
    ADC_ClearITPendingAll();

    CW_ADC->CR_f.BGREN = 1u;      /* 使能 BGR (厂商: 启动约 30 us) */
    delay_ms(1u);                 /* 有界等待: 1 ms >> 30 us */
    ADC_Enable();

    ADC_SoftwareStartConvCmd(ENABLE);
    while ((CW_ADC->ISR & ADC_ISR_EOC_Msk) == 0u) {
        if (guard == 0u) {
            CW_ADC->CR_f.BGREN = 0u;
            ADC_Disable();
            return 0u;            /* 超时: 不伪造读数 */
        }
        guard--;
    }
    ADC_ClearITPendingBit(ADC_IT_EOC);
    code   = ADC_GetConversionValue(0);
    bgr_mv = (uint32_t)LIGHT_BGR_TRIM_MV;

    ADC_Disable();
    CW_ADC->CR_f.BGREN = 0u;      /* 关闭 BGR (低功耗) */

    *code_out   = code;
    *bgr_mv_out = (uint16_t)bgr_mv;
    if (code == 0u) {
        return 0u;                /* 除零保护: 视为无效 */
    }
    mv = (4095uL * bgr_mv) / (uint32_t)code;   /* VDD[mV] = 4095 * Vref[mV] / code */
    if (mv > 6000uL) {
        mv = 6000uL;              /* 合理上限保护 (MCU 最大 5.5 V) */
    }
    *mv_out = (uint16_t)mv;
    return 1u;
}

light_result_t light_sample(void)
{
    light_result_t r;
    uint32_t sum = 0u;
    uint8_t  i;
    uint8_t  ok = 0u;
    uint16_t vmin = 0u;
    uint16_t vmax = 0u;

    r.valid         = false;
    r.adc_ok        = false;
    r.samples_ok    = 0u;
    r.mean_adc_code = 0u;
    r.code_min      = 0u;
    r.code_max      = 0u;
    r.dark          = false;

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

    /*
     * 全部转换超时: adc_ok=false, 均值/极值保持 0, valid=false -> dark=false。
     * 不得用满量程 4095 合成暗态 (FD-002 rev 4.0 §6.3 规则 1/5; TD-002 T-L3-04)。
     */
    r.samples_ok = ok;
    r.adc_ok     = (ok > 0u);
    if (r.adc_ok) {
        r.mean_adc_code = (uint16_t)(sum / (uint32_t)ok);
        r.code_min      = vmin;
        r.code_max      = vmax;
    }

    /* 有效性门禁: ADC 成功 且 全暗基准已标定 */
    r.valid = r.adc_ok && (LIGHT_DARK_CALIBRATED != 0);
    r.dark  = light_is_dark(r.mean_adc_code, r.valid, (uint16_t)LIGHT_DARK_REF_CODE);

    return r;
}
