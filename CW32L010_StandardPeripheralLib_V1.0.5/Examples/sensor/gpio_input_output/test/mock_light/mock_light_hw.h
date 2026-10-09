/*
 * mock_light_hw.h - 宿主机 MCU 影子层: 供光照通路 light.c 自检使用 (T2)。
 * 作者: firmware_engineer.firmware_implementation
 *
 * 影子头 (cw32l010_{gpio,adc,sysctrl}.h) 覆盖厂商头并置于 include 路径最前,
 * 使真实 USER/src/light.c 在宿主机编译; 本目录不进入固件构建。
 */
#ifndef __MOCK_LIGHT_HW_H
#define __MOCK_LIGHT_HW_H

#include <stdint.h>
#include <stddef.h>

typedef enum { RESET = 0, SET = 1 } FlagStatus, ITStatus;
typedef enum { DISABLE = 0, ENABLE = 1 } FunctionalState;
typedef enum { ERROR = 0, SUCCESS = 1 } ErrorStatus;
typedef enum { GPIO_Pin_RESET = 0, GPIO_Pin_SET = 1 } GPIO_PinState;

/* ---- GPIO ---- */
#define GPIO_PIN_4 ((uint16_t)0x0010)
#define GPIO_PIN_5 ((uint16_t)0x0020)

#define GPIO_MODE_INPUT        0x10u
#define GPIO_MODE_OUTPUT_PP    0x20u
#define GPIO_MODE_OUTPUT_OD    0x30u
#define GPIO_MODE_ANALOG       0x40u
#define GPIO_IT_NONE           0x00u

typedef struct { uint32_t unused; } GPIO_TypeDef;
typedef struct { uint16_t Pins; uint32_t Mode; uint32_t IT; } GPIO_InitTypeDef;

extern GPIO_TypeDef mock_gpiob;
#define CW_GPIOB (&mock_gpiob)

void GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *cfg);
void GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);

/* ---- ADC ---- */
#define ADC_Clk_Div8         8u
#define ADC_ConvertMode_Once 0u
#define ADC_SqrEns0to0       0u
#define ADC_SampTime390Clk   7u
#define ADC_InputCH11        0xBu
#define ADC_InputVref1P2     0xFu   /* 内部 BGR1.2V 通道 (供电测量用) */
#define ADC_IT_EOC           1u
#define ADC_ISR_EOC_Msk      (1u << 0)

typedef struct { uint32_t ADC_InputChannel; uint32_t ADC_SampTime; } ADC_InitChannelTypeDef;
typedef struct {
    uint32_t ADC_ClkDiv;
    uint32_t ADC_ConvertMode;
    uint32_t ADC_SQREns;
    ADC_InitChannelTypeDef ADC_IN0;
} ADC_InitTypeDef;

/* light.c 读取 CW_ADC->ISR 判 EOC 并写 CW_ADC->CR_f.BGREN 使能/失能 BGR; 由 harness 驱动 */
typedef struct { volatile uint32_t BGREN : 1; volatile uint32_t rsv : 31; } ADC_CR_bits;
typedef struct { volatile uint32_t ISR; volatile ADC_CR_bits CR_f; } ADC_TypeDef;
extern ADC_TypeDef mock_adc;
#define CW_ADC (&mock_adc)

void        ADC_Init(ADC_InitTypeDef *cfg);
void        ADC_ClearITPendingAll(void);
void        ADC_Disable(void);
ErrorStatus ADC_Enable(void);
void        ADC_SoftwareStartConvCmd(FunctionalState state);
void        ADC_ClearITPendingBit(uint16_t it);
uint16_t    ADC_GetConversionValue(uint32_t idx);

/* ---- SYSCTRL ---- */
#define __SYSCTRL_GPIOB_CLK_ENABLE() do { } while (0)
#define __SYSCTRL_ADC_CLK_ENABLE()   do { } while (0)

#endif /* __MOCK_LIGHT_HW_H */
