/*
 * mock_cw32.h - MCU hardware-layer mock for host verification of light.c (ITEM-004).
 * Author: embedded_tester.embedded_verification.
 *
 * The three shadow headers (cw32l010_gpio.h / cw32l010_adc.h / cw32l010_sysctrl.h)
 * each include this file, and the mock include directory is placed BEFORE the vendor
 * library include path, so the REAL light.c compiles against this emulated MCU layer
 * while its logic (sampling sequence, averaging, hysteresis, timeout fallback) stays
 * untouched and is exercised for real.
 */
#ifndef __MOCK_CW32_H
#define __MOCK_CW32_H

#include <stdint.h>

/* ---- base types ---- */
typedef enum { RESET = 0, SET = 1 } FlagStatus, ITStatus;
typedef enum { DISABLE = 0, ENABLE = 1 } FunctionalState;
typedef enum { ERROR = 0, SUCCESS = 1 } ErrorStatus;
typedef enum { GPIO_Pin_RESET = 0, GPIO_Pin_SET = 1 } GPIO_PinState;

/* ---- GPIO ---- */
#define GPIO_PIN_0  ((uint16_t)0x0001)
#define GPIO_PIN_1  ((uint16_t)0x0002)
#define GPIO_PIN_2  ((uint16_t)0x0004)
#define GPIO_PIN_3  ((uint16_t)0x0008)
#define GPIO_PIN_4  ((uint16_t)0x0010)
#define GPIO_PIN_5  ((uint16_t)0x0020)
#define GPIO_PIN_6  ((uint16_t)0x0040)
#define GPIO_PIN_7  ((uint16_t)0x0080)

#define GPIO_MODE_INPUT       0u
#define GPIO_MODE_OUTPUT_PP   1u
#define GPIO_MODE_OUTPUT_OD   2u
#define GPIO_MODE_ANALOG      3u
#define GPIO_IT_NONE          0u

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
#define ADC_IT_EOC           1u
#define ADC_ISR_EOC_Msk      (1u << 0)

typedef struct { uint32_t ADC_InputChannel; uint32_t ADC_SampTime; } ADC_InitChannelTypeDef;
typedef struct {
    uint32_t ADC_ClkDiv;
    uint32_t ADC_ConvertMode;
    uint32_t ADC_SQREns;
    ADC_InitChannelTypeDef ADC_IN0;
} ADC_InitTypeDef;

typedef struct { volatile uint32_t isr; uint32_t (*isr_get)(void); } mock_adc_regs_t;
extern mock_adc_regs_t mock_adc;
/* light.c reads the EOC flag as CW_ADC->ISR; route it through a function-pointer member
   so the bounded-poll guard can be asserted deterministically (only light.c uses ISR). */
#define CW_ADC (&mock_adc)
#define ISR isr_get()
uint32_t isr_get(void);

void      ADC_Init(ADC_InitTypeDef *cfg);
void      ADC_ClearITPendingAll(void);
void      ADC_Disable(void);
ErrorStatus ADC_Enable(void);
void      ADC_SoftwareStartConvCmd(FunctionalState state);
void      ADC_ClearITPendingBit(uint16_t it);
uint16_t  ADC_GetConversionValue(uint32_t idx);

/* ---- SYSCTRL ---- */
#define __SYSCTRL_GPIOB_CLK_ENABLE() do { } while (0)
#define __SYSCTRL_ADC_CLK_ENABLE()   do { } while (0)

#endif /* __MOCK_CW32_H */
