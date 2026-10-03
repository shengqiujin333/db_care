/*
 * mock_measure_mcu/cw32l010_uart.h - host shadow of the vendor UART header
 * (plus the GPIO alternate-function macros referenced by measure.h).
 */
#ifndef __MOCK_MEASURE_CW32L010_UART_H
#define __MOCK_MEASURE_CW32L010_UART_H

#include <stdint.h>

typedef struct { int unused; } UART_TypeDef;

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

#define CW_UART1 ((UART_TypeDef *)0)

#define UART_Over_16                  0u
#define UART_Source_PCLK              0u
#define UART_StartBit_FE              0u
#define UART_StopBits_1               0u
#define UART_Parity_No                0u
#define UART_HardwareFlowControl_None 0u
#define UART_Mode_Rx                  1u
#define UART_Mode_Tx                  2u

#define PA05_AFx_UART1RXD() do { } while (0)
#define PA06_AFx_UART1TXD() do { } while (0)

void UART_Init(UART_TypeDef *UARTx, UART_InitTypeDef *init);

#endif /* __MOCK_MEASURE_CW32L010_UART_H */
