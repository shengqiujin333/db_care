/*
 * light.h - 光照通路 (readme 修改点 2/7; FD-002 rev 4.0 §2.3/§3.1/§6.3/§10)
 *
 * 拓扑 (sensor_hardware 网表核定):
 *   PB05(LIGTHT_POWER, 输出) -- R3 5M -- LIGHT_ADC(PB04/AIN11) -- 光敏电阻 -- GND
 * ADC 满量程参考 = VDD (比率式, 与电池电压无关):
 *   code = 4095 * R_photo / (R_photo + 5M)
 * 极性: 无光(暗) => 光敏阻值大 => 节点电压高 => 读数大。
 *
 * 不含: 采样节拍、温湿度、上报判定 (属其它模块/任务项)。
 */
#ifndef __LIGHT_H
#define __LIGHT_H

#include <stdint.h>
#include <stdbool.h>

/*
 * 一次光照采样的结构化结果 (FD-002 rev 4.0 §6.3; T2):
 *   valid  = adc_ok && LIGHT_DARK_CALIBRATED     (未标定/全部转换超时 -> false)
 *   dark   = valid && (uint32)3*mean_adc_code >= C_dark   (边界相等为暗, 每笔独立无滞回)
 * 全部转换超时: adc_ok=false、mean/min/max=0、valid=false、dark=false,
 * 不得用满量程合成暗态。
 */
typedef struct {
    bool     valid;          /* 光照是否可证明无光 (adc_ok 且已标定) */
    bool     adc_ok;         /* samples_ok > 0 */
    uint8_t  samples_ok;     /* 成功转换样本数 0..LIGHT_ADC_SAMPLES */
    uint16_t mean_adc_code;  /* 成功样本算术均值 (adc_ok==false 时为 0) */
    uint16_t code_min;       /* 成功样本最小值 (adc_ok==false 时为 0) */
    uint16_t code_max;       /* 成功样本最大值 (adc_ok==false 时为 0) */
    bool     dark;           /* 无光判定结果 */
} light_result_t;

/* 配置 PB05 为推挽输出(低)、PB04 为模拟输入(AIN11) 并配置 ADC (ADC 保持关闭, 采样时打开) */
void light_init(void);

/*
 * 一次光照采样:
 *   PB05 输出高 -> 稳定延时 -> PB04/AIN11 取样 LIGHT_ADC_SAMPLES 次求均值 -> PB05 置低;
 *   均值只取成功样本; 全部转换超时 -> adc_ok=false/valid=false (不合成暗态)。
 * 无跨周期状态 (无滞回)。
 */
light_result_t light_sample(void);

#endif /* __LIGHT_H */
