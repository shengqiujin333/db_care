/*
 * mock_measure_hw.h - emulated MCU hardware layer for host verification of the
 * REAL USER/src/measure.c sampling flow (ITEM-006).
 * Author: embedded_tester.embedded_verification.
 *
 * The shadow headers cw32l010_{gpio,adc,sysctrl,uart}.h each include this file and
 * are placed BEFORE the vendor include path, so measure.c compiles and runs on the
 * host while its flow/state logic stays untouched.
 */
#ifndef __MOCK_MEASURE_HW_H
#define __MOCK_MEASURE_HW_H

#include <stdint.h>

typedef enum { DISABLE = 0, ENABLE = 1 } FunctionalState;
typedef enum { GPIO_Pin_RESET = 0, GPIO_Pin_SET = 1 } GPIO_PinState;

/* ---- GPIO ---- */
#define GPIO_PIN_3 ((uint16_t)0x0008)
#define GPIO_PIN_4 ((uint16_t)0x0010)
#define GPIO_PIN_5 ((uint16_t)0x0020)
#define GPIO_PIN_6 ((uint16_t)0x0040)

#define GPIO_MODE_INPUT        0x10u
#define GPIO_MODE_INPUT_PULLUP 0x11u
#define GPIO_MODE_OUTPUT_PP    0x20u
#define GPIO_MODE_OUTPUT_OD    0x30u
#define GPIO_MODE_ANALOG       0x40u
#define GPIO_IT_NONE           0x00u
#define GPIO_IT_FALLING        0x01u

typedef struct { int unused; } GPIO_TypeDef;
typedef struct { uint16_t Pins; uint32_t Mode; uint32_t IT; } GPIO_InitTypeDef;

extern GPIO_TypeDef mock_gpioa, mock_gpiob;
#define CW_GPIOA (&mock_gpioa)
#define CW_GPIOB (&mock_gpiob)

#define __SYSCTRL_GPIOA_CLK_ENABLE() do { } while (0)
#define __SYSCTRL_GPIOB_CLK_ENABLE() do { } while (0)
#define PA05_AFx_UART1RXD() do { } while (0)
#define PA06_AFx_UART1TXD() do { } while (0)

void          GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *init);
void          GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);
GPIO_PinState GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin);

/* ---- ADC constants needed by sensor_config.h (light itself is stubbed here) ---- */
#define ADC_InputCH11      0xBu
#define ADC_Clk_Div8       8u
#define ADC_SampTime390Clk 7u

/* ---- SYSCTRL ---- */
#define SYSCTRL_AHB_PERIPH_GPIOA  0u
#define SYSCTRL_APB1_PERIPH_UART1 0u

void SYSCTRL_AHBPeriphReset(uint32_t periph, FunctionalState st);
void SYSCTRL_APBPeriphReset1(uint32_t periph, FunctionalState st);
void SYSCTRL_AHBPeriphClk_Enable(uint32_t periph, FunctionalState st);
void SYSCTRL_APBPeriphClk_Enable1(uint32_t periph, FunctionalState st);
void SYSCTRL_GotoDeepSleep(void);

/* ---- UART ---- */
#define CW_UART1 0
#define UART_Over_16 0u
#define UART_Source_PCLK 0u
#define UART_StartBit_FE 0u
#define UART_StopBits_1 0u
#define UART_Parity_No 0u
#define UART_HardwareFlowControl_None 0u
#define UART_Mode_Rx 1u
#define UART_Mode_Tx 2u

typedef struct {
    uint32_t UART_BaudRate;
    uint32_t UART_Over;
    uint32_t UART_Source;
    uint32_t UART_UclkFreq;
    uint32_t UART_StartBit;
    uint32_t UART_StopBits;
    uint32_t UART_Parity;
    uint32_t UART_HardwareFlowControl;
    uint32_t UART_Mode;
} UART_InitTypeDef;

void UART_Init(uint32_t uart, UART_InitTypeDef *cfg);

#endif /* __MOCK_MEASURE_HW_H */
