/*
 * mock_diagline_hw.h - emulated MCU layer for the embedded_tester's OWN diagnostic
 * line-format harness (host_diag_line_verify_ev.c). Author:
 * embedded_tester.embedded_verification. Written from scratch for this verification
 * (it does not reuse the implementer's test/mock_trace shadow layer).
 *
 * Only the peripheral surface that USER/src/debug_trace.c actually touches is
 * modelled: GPIOA/GPIOB pin configuration, UART1 open/reset/send flags.
 */
#ifndef __MOCK_DIAGLINE_HW_H
#define __MOCK_DIAGLINE_HW_H

#include <stdint.h>
#include <stddef.h>

typedef enum { RESET = 0, SET = 1 } FlagStatus, ITStatus;
typedef enum { DISABLE = 0, ENABLE = 1 } FunctionalState;
typedef enum { ERROR = 0, SUCCESS = 1 } ErrorStatus;
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

typedef struct { uint16_t Pins; uint32_t Mode; uint32_t IT; } GPIO_InitTypeDef;

typedef struct { volatile uint32_t AFR5, AFR6; } mock_dl_afrl_t;
typedef struct { mock_dl_afrl_t AFRL_f; } GPIO_TypeDef;

extern GPIO_TypeDef mock_dl_gpioa, mock_dl_gpiob;
#define CW_GPIOA (&mock_dl_gpioa)
#define CW_GPIOB (&mock_dl_gpiob)

#define __SYSCTRL_GPIOA_CLK_ENABLE() do { } while (0)
#define __SYSCTRL_GPIOB_CLK_ENABLE() do { } while (0)
#define PA05_AFx_UART1RXD() do { } while (0)
#define PA06_AFx_UART1TXD() do { } while (0)

void          GPIO_Init(GPIO_TypeDef *port, GPIO_InitTypeDef *init);
void          GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);
GPIO_PinState GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin);

/* ---- UART1 ---- */
#define CW_UART1 0u
#define UART_Over_16                  0u
#define UART_Source_PCLK              0u
#define UART_StartBit_FE              0u
#define UART_StopBits_1               0u
#define UART_Parity_No                0u
#define UART_HardwareFlowControl_None 0u
#define UART_Mode_Rx                  1u
#define UART_Mode_Tx                  2u
#define UART_FLAG_TXE                 ((uint16_t)0x0001)
#define UART_FLAG_TC                  ((uint16_t)0x0002)
#define UART_FLAG_TXBUSY              ((uint16_t)0x4000)

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

#define MOCK_DL_TXBUF 4096
extern uint8_t mock_dl_tx[MOCK_DL_TXBUF];
extern int     mock_dl_tx_len;
extern int     mock_dl_uart_init_calls;
extern int     mock_dl_uart_reset_calls;
extern int     mock_dl_uart_clk_off_calls;

void       UART_Init(uint32_t uart, UART_InitTypeDef *cfg);
void       UART_SendData_8bit(uint32_t uart, uint8_t data);
FlagStatus UART_GetFlagStatus(uint32_t uart, uint16_t flag);

/* ---- SYSCTRL ---- */
#define SYSCTRL_APB1_PERIPH_UART1   0u
#define SYSCTRL_AHB_PERIPH_GPIOA    0u

void SYSCTRL_APBPeriphClk_Enable1(uint32_t periph, FunctionalState st);
void SYSCTRL_APBPeriphReset1(uint32_t periph, FunctionalState st);
void SYSCTRL_AHBPeriphReset(uint32_t periph, FunctionalState st);
void SYSCTRL_AHBPeriphClk_Enable(uint32_t periph, FunctionalState st);

#endif /* __MOCK_DIAGLINE_HW_H */
