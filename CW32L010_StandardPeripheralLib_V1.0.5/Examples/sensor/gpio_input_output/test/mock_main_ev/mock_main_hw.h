/*
 * mock_main_hw.h - emulated MCU layer for host verification of the REAL USER/src/main.c
 * RTC tick accumulator (ITEM-007). Author: embedded_tester.embedded_verification.
 *
 * The shadow headers cw32l010_{gpio,adc,sysctrl,systick,rtc,uart,digitalsign,flash}.h,
 * interrupts_cw32l010.h and system_cw32l010.h include this file and are placed before
 * the vendor include path, so main.c compiles unchanged on the host.
 */
#ifndef __MOCK_MAIN_HW_H
#define __MOCK_MAIN_HW_H

#include <stdint.h>

typedef enum { RESET = 0, SET = 1 } FlagStatus, ITStatus;
typedef enum { DISABLE = 0, ENABLE = 1 } FunctionalState;
typedef enum { ERROR = 0, SUCCESS = 1 } ErrorStatus;
typedef enum { GPIO_Pin_RESET = 0, GPIO_Pin_SET = 1 } GPIO_PinState;

/* ---- GPIO ---- */
#define GPIO_PIN_0 ((uint16_t)0x0001)
#define GPIO_PIN_1 ((uint16_t)0x0002)
#define GPIO_PIN_2 ((uint16_t)0x0004)
#define GPIO_PIN_3 ((uint16_t)0x0008)
#define GPIO_PIN_4 ((uint16_t)0x0010)
#define GPIO_PIN_5 ((uint16_t)0x0020)
#define GPIO_PIN_6 ((uint16_t)0x0040)
#define GPIO_PIN_7 ((uint16_t)0x0080)
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
void GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *init);
void GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);
GPIO_PinState GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin);

/* ---- ADC constants used by sensor_config.h ---- */
#define ADC_InputCH11      0xBu
#define ADC_Clk_Div8       8u
#define ADC_SampTime390Clk 7u

/* ---- SYSCTRL ---- */
#define SYSCTRL_HSIOSC_DIV6       0u
#define SYSCTRL_HCLK_DIV1         0u
#define SYSCTRL_PCLK_DIV1         0u
#define SYSCTRL_AHB_PERIPH_GPIOA  0u
#define SYSCTRL_APB1_PERIPH_UART1 0u
#define SYSCTRL_APB2_PERIPH_RTC   0u
#define SYSCTRL_APB2_PERIPH_LPTIM 0u
#define __SYSCTRL_GPIOA_CLK_ENABLE()  do { } while (0)
#define __SYSCTRL_GPIOB_CLK_ENABLE()  do { } while (0)
#define __SYSCTRL_FLASH_CLK_ENABLE()  do { } while (0)
void SYSCTRL_HSI_Enable(uint32_t div);
void SYSCTRL_LSI_Enable(void);
void SYSCTRL_HCLKPRS_Config(uint32_t div);
void SYSCTRL_PCLKPRS_Config(uint32_t div);
void SYSCTRL_APBPeriphClk_Enable2(uint32_t periph, FunctionalState st);
void SYSCTRL_AHBPeriphClk_Enable(uint32_t periph, FunctionalState st);
void SYSCTRL_APBPeriphClk_Enable1(uint32_t periph, FunctionalState st);
void SYSCTRL_AHBPeriphReset(uint32_t periph, FunctionalState st);
void SYSCTRL_APBPeriphReset1(uint32_t periph, FunctionalState st);
void SYSCTRL_GotoDeepSleep(void);
uint32_t SYSCTRL_GetHClkFreq(void);
uint32_t SYSCTRL_GetPClkFreq(void);

/* ---- RTC ---- */
#define RTC_Month_January    1u
#define RTC_Weekday_Sunday   0u
#define RTC_RTCCLK_FROM_LSI  0u
#define RTC_H12_AM           0u
#define RTC_HOUR12           0u
#define RTC_IT_INTERVAL      0x01u
#define RTC_IT_ALL           0xFFu
#define RTC_INTERVAL_EVERY_1M 0x03u   /* vendor value: 1 minute interval */
#define RTC_IRQn             3

typedef struct { uint32_t Day, Month, Year, Week; } RTC_DateTypeDef;
typedef struct { uint32_t AMPM, H24, Hour, Minute, Second; } RTC_TimeTypeDef;
typedef struct {
    RTC_DateTypeDef DateStruct;
    RTC_TimeTypeDef TimeStruct;
    uint32_t RTC_ClockSource;
} RTC_InitTypeDef;

void     RTC_Init(RTC_InitTypeDef *cfg);
void     RTC_SetInterval(uint32_t interval);
void     RTC_ITConfig(uint32_t it, FunctionalState st);
void     RTC_Cmd(FunctionalState st);
ITStatus RTC_GetITState(uint32_t it);
void     RTC_ClearITPendingBit(uint32_t it);
void     NVIC_EnableIRQ(int irq);

/* ---- UART (debug, only compiled, not exercised here) ---- */
#define CW_UART1 0
#define UART_Over_16 0u
#define UART_Source_PCLK 0u
#define UART_StartBit_FE 0u
#define UART_StopBits_1 0u
#define UART_Parity_No 0u
#define UART_HardwareFlowControl_None 0u
#define UART_Mode_Rx 1u
#define UART_Mode_Tx 2u
#define UART_FLAG_TXE 0u
typedef struct {
    uint32_t UART_BaudRate, UART_Over, UART_Source, UART_UclkFreq, UART_StartBit,
             UART_StopBits, UART_Parity, UART_HardwareFlowControl, UART_Mode;
} UART_InitTypeDef;
void     UART_Init(uint32_t uart, UART_InitTypeDef *cfg);
void     UART_SendData_8bit(uint32_t uart, uint8_t data);
FlagStatus UART_GetFlagStatus(uint32_t uart, uint32_t flag);

/* ---- DIGITALSIGN / FLASH ---- */
#define FLASH_RDLEVEL2 0u
void DIGITALSIGN_GetChipUid(uint8_t uid[10]);
void FLASH_SetReadOutLevel(uint32_t level);

/* ---- misc ---- */
#define PA05_AFx_UART1RXD() do { } while (0)
#define PA06_AFx_UART1TXD() do { } while (0)

#endif /* __MOCK_MAIN_HW_H */
