/*
 * measure.h
 *
 *  Created on: Feb 22, 2024
 *      Author: kason
 */

#ifndef MEASURE_H_
#define MEASURE_H_

#include "stdint.h"

	//UARTx
#define  DEBUG_UARTx                   CW_UART1
#define  DEBUG_UART_CLK                SYSCTRL_APB1_PERIPH_UART1
#define  DEBUG_UART_APBClkENx          SYSCTRL_APBPeriphClk_Enable1
#define  DEBUG_UART_BaudRate           9600
#define  DEBUG_UART_UclkFreq           8000000

//UARTx GPIO
#define  DEBUG_UART_GPIO_CLK           (SYSCTRL_AHB_PERIPH_GPIOA)
#define  DEBUG_UART_TX_GPIO_PORT       CW_GPIOA
#define  DEBUG_UART_TX_GPIO_PIN        GPIO_PIN_6
#define  DEBUG_UART_RX_GPIO_PORT       CW_GPIOA
#define  DEBUG_UART_RX_GPIO_PIN        GPIO_PIN_5

//GPIO AF
#define  DEBUG_UART_AFTX               PA05_AFx_UART1RXD()
#define  DEBUG_UART_AFRX               PA06_AFx_UART1TXD()






#define start_measure   (0x0001<<0)
#define read_value   (0x0001<<1)
#define calculate_value   (0x0001<<2)
#define wait_data       (0x0001<<3)
#define prepare (0x0001<<4)

void temperature_task_init( void );
void bsp_i2c_init(void);
uint16_t temperature_process( void );

void UART1_Configure(void);
void DebugUART_Close(void);

void send_data_to_gateway(void);
void go_to_sleep(void);
#endif /* MEASURE_H_ */
