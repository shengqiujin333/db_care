/**
 * @file    interface.h
 * @brief   Phase 2.3 — UART Console Driver — Exported API
 *
 * MCU: CW32L010Y8M6 (TSSOP-20)
 * Module: uart_console (layer=driver)
 * Bus: UART1 on PA05 (RXD) / PA06 (TXD), AF1
 *
 * This module provides a compile-time-toggleable console output via UART1.
 * When CONFIG_CONSOLE_ENABLE == 0, all output macros expand to ((void)0).
 *
 * Sources:
 *   - cw32l010_uart.h#L36-L69 (UART_InitTypeDef)
 *   - cw32l010_uart.h#L283 (UART_SendData_8bit)
 *   - cw32l010_uart.h#L301 (UART_GetFlagStatus)
 *   - cw32l010_gpio.h#L371 (PA05_AFx_UART1RXD)
 *   - cw32l010_gpio.h#L380 (PA06_AFx_UART1TXD)
 *   - cw32l010_sysctrl.h#L374 (__SYSCTRL_UART1_CLK_ENABLE)
 *   - cw32l010_sysctrl.h#L352 (__SYSCTRL_GPIOA_CLK_ENABLE)
 *   - connectivity.json#buses[UART] — UART1_TXD (PA06), UART1_RXD (PA05)
 *   - COMMON.md#5.1 — CONFIG_CONSOLE_ENABLE compile-time toggle
 */

#ifndef UART_CONSOLE_INTERFACE_H
#define UART_CONSOLE_INTERFACE_H

#include <stdint.h>
#include <stdarg.h>
#include "common_types.h"

/* =========================================================================
 * Compile-Time Toggle
 * =========================================================================
 * Set CONFIG_CONSOLE_ENABLE to 0 in board.h or build config to disable
 * all UART console output for power saving.
 * When disabled, UART1 peripheral clock can be gated and pins reconfigured
 * as GPIO input (high-Z) for minimum leakage.
 * ========================================================================= */
#ifndef CONFIG_CONSOLE_ENABLE
#define CONFIG_CONSOLE_ENABLE   1
#endif

/* =========================================================================
 * Initialization
 * ========================================================================= */

/**
 * @brief  Initialize UART1 for console output
 *
 * Configures:
 *   - GPIOA clock enabled
 *   - PA05 as UART1_RXD (AF1, input pull-up)
 *   - PA06 as UART1_TXD (AF1, output push-pull)
 *   - UART1 peripheral clock enabled
 *   - UART1 @ 115200 baud, 8N1, TX-only mode
 *
 * @param  uclk_freq  UART clock frequency in Hz (typically SystemCoreClock)
 * @return ERR_OK on success, or negative error code
 *
 * @note   uclk_freq should match the PCLK feeding UART1 (UART_Source_PCLK).
 *         Typically SystemCoreClock when HCLK:PCLK = 1:1.
 */
int32_t uart_console_init(uint32_t uclk_freq);

/**
 * @brief  De-initialize UART1 console (disable clock, release pins)
 *
 * After calling this:
 *   - UART1 peripheral clock is disabled
 *   - PA05/PA06 are reconfigured as GPIO input (high-Z)
 *   - UART1 registers are reset
 */
void uart_console_deinit(void);

/* =========================================================================
 * Output
 * ========================================================================= */

/**
 * @brief  Transmit a single character over UART1 (blocking, poll TXE)
 *
 * Waits for the TXE (transmit data register empty) flag before writing.
 *
 * @param  ch  Character to transmit
 */
void uart_putchar(char ch);

/**
 * @brief  Transmit a null-terminated string (blocking)
 *
 * Calls uart_putchar() for each character until '\\0'.
 *
 * @param  str  Null-terminated string
 */
void uart_puts(const char *str);

/**
 * @brief  Transmit a buffer of bytes (blocking)
 *
 * Useful for binary/hex output. Calls uart_putchar() for each byte.
 *
 * @param  data  Pointer to buffer
 * @param  len   Number of bytes to send
 */
void uart_write(const uint8_t *data, uint32_t len);

/**
 * @brief  va_list version of formatted printf (blocking)
 *
 * Supports: %s, %d, %u, %x, %c, %f (basic float via integer division)
 * Used by log_printf() to forward variadic arguments from LOG_* macros.
 *
 * @param  fmt   Printf-style format string
 * @param  args  va_list of arguments
 */
void uart_vprintf(const char *fmt, va_list args);

/**
 * @brief  Simple formatted printf (blocking)
 *
 * Supports: %s, %d, %u, %x, %c, %f (basic float via integer division)
 * Limited buffer on stack — suitable for short debug messages only.
 *
 * @param  fmt  Printf-style format string
 * @param  ...  Variable arguments
 */
void uart_printf(const char *fmt, ...);

/* =========================================================================
 * Convenience Macros (compile-time toggle)
 * =========================================================================
 * When CONFIG_CONSOLE_ENABLE == 0, these expand to ((void)0) and produce
 * no code.
 *
 * Example:
 *   CONSOLE_PRINT("Temp: %d.%d C\n", temp / 10, temp % 10);
 * ========================================================================= */

#if CONFIG_CONSOLE_ENABLE

#define CONSOLE_PRINT(fmt, ...)     uart_printf(fmt, ##__VA_ARGS__)
#define CONSOLE_PUTS(str)           uart_puts(str)
#define CONSOLE_PUTCHAR(ch)         uart_putchar(ch)
#define CONSOLE_WRITE(data, len)    uart_write((data), (len))

#else /* CONFIG_CONSOLE_ENABLE == 0 */

#define CONSOLE_PRINT(fmt, ...)     ((void)0)
#define CONSOLE_PUTS(str)           ((void)0)
#define CONSOLE_PUTCHAR(ch)         ((void)0)
#define CONSOLE_WRITE(data, len)    ((void)0)

#endif /* CONFIG_CONSOLE_ENABLE */

#endif /* UART_CONSOLE_INTERFACE_H */
