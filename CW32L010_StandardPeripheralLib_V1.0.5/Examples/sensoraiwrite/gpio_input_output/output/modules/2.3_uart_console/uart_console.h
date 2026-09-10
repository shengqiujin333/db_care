/**
 * @file    uart_console.h
 * @brief   Phase 2.3 — UART Console Driver — Module Header
 *
 * MCU: CW32L010Y8M6 (TSSOP-20)
 * Module: uart_console (layer=driver)
 * Bus: UART1 on PA05 (RXD) / PA06 (TXD), AF1
 *
 * Provides:
 *   - uart_console_init()   : Initialize UART1 @ 115200 8N1 TX-only
 *   - uart_console_deinit() : De-initialize UART1 for power saving
 *   - uart_putchar()        : Blocking single character TX
 *   - uart_puts()           : Blocking string TX
 *   - uart_write()          : Blocking binary buffer TX
 *   - uart_printf()         : Lightweight formatted output
 *
 * Compile-time toggle via CONFIG_CONSOLE_ENABLE (see interface.h).
 *
 * Sources:
 *   - cw32l010_uart.h      (UART_InitTypeDef, UART_SendData_8bit, UART_GetFlagStatus)
 *   - cw32l010_gpio.h      (GPIO_InitTypeDef, GPIO_Init, pin AF macros)
 *   - cw32l010_sysctrl.h   (__SYSCTRL_UART1_CLK_ENABLE, __SYSCTRL_GPIOA_CLK_ENABLE)
 *   - connectivity.json#buses[UART]
 *   - COMMON.md#5.1
 */

#ifndef UART_CONSOLE_H
#define UART_CONSOLE_H

#include "interface.h"

/* =========================================================================
 * Internal Prototypes (not exported via interface.h)
 * ========================================================================= */

/**
 * @brief  Wait until UART TX data register is empty
 *
 * Polls UART_FLAG_TXE. Returns when TXE is set (data register ready).
 *
 * @param  UARTx  UART peripheral (CW_UART1)
 */
static inline void uart_wait_txe(void);

/**
 * @brief  Wait until UART transmission complete
 *
 * Polls UART_FLAG_TC. Returns when TC is set (last byte shifted out).
 * Used before deinit to ensure no pending TX.
 *
 * @param  UARTx  UART peripheral (CW_UART1)
 */
static inline void uart_wait_tc(void);

#endif /* UART_CONSOLE_H */
