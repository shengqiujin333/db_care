/**
 * @file    uart_console.c
 * @brief   Phase 2.3 — UART Console Driver — Implementation
 *
 * MCU: CW32L010Y8M6 (TSSOP-20)
 * Module: uart_console (layer=driver)
 * Bus: UART1 on PA05 (RXD) / PA06 (TXD), AF1
 *
 * UART configuration:
 *   - Instance:   UART1 (CW_UART1)
 *   - Baud rate:  115200
 *   - Data bits:  8 (parity disabled → auto 8-bit)
 *   - Parity:     None
 *   - Stop bits:  1
 *   - Flow ctrl:  None
 *   - Mode:       TX only (UART_Mode_Tx)
 *   - Oversample: 16x (UART_Over_16)
 *   - Clock src:  PCLK (UART_Source_PCLK)
 *
 * TX method:
 *   - Blocking, polling TXE flag (UART_FLAG_TXE)
 *   - No interrupts, no DMA, no RX
 *
 * Sources:
 *   [1] cw32l010_uart.h      — UART_InitTypeDef, UART_Init(), UART_SendData_8bit()
 *   [2] cw32l010_gpio.h      — GPIO_InitTypeDef, GPIO_Init(), AF macros
 *   [3] cw32l010_sysctrl.h   — __SYSCTRL_UART1_CLK_ENABLE, __SYSCTRL_GPIOA_CLK_ENABLE
 *   [4] cw32l010.h#L84       — UART1_IRQn (not used, TX-only)
 *   [5] connectivity.json    — UART1_TXD=PA06, UART1_RXD=PA05
 *   [6] COMMON.md#5.1        — CONFIG_CONSOLE_ENABLE toggle
 */

#include "uart_console.h"
#include "cw32l010_uart.h"
#include "cw32l010_gpio.h"
#include "cw32l010_sysctrl.h"

#include <stdarg.h>
#include <stdint.h>

/* =========================================================================
 * Local Constants
 * ========================================================================= */

/** UART1 baud rate for console output */
#define UART_CONSOLE_BAUDRATE       115200UL

/** TX pin is PA06 (bit 6 in GPIOA) */
#define UART_TX_PIN                 GPIO_PIN_6

/** RX pin is PA05 (bit 5 in GPIOA) */
#define UART_RX_PIN                 GPIO_PIN_5

/** UART1 instance pointer */
#define UART_CONSOLE                CW_UART1

/* =========================================================================
 * Internal Helpers
 * ========================================================================= */

static inline void uart_wait_txe(void)
{
    /* Wait until TXE flag is set (data register empty) [1] */
    while (UART_GetFlagStatus(UART_CONSOLE, UART_FLAG_TXE) == RESET)
    {
        /* spin */;
    }
}

static inline void uart_wait_tc(void)
{
    /* Wait until TC flag is set (last byte fully shifted out) [1] */
    while (UART_GetFlagStatus(UART_CONSOLE, UART_FLAG_TC) == RESET)
    {
        /* spin */;
    }
}

/* =========================================================================
 * Initialization / De-initialization
 * ========================================================================= */

int32_t uart_console_init(uint32_t uclk_freq)
{
    UART_InitTypeDef uart_cfg;
    GPIO_InitTypeDef gpio_cfg;

    /* --- Enable peripheral clocks --- */
    /* GPIOA clock (AHB): PA05, PA06 are on GPIOA [3] */
    __SYSCTRL_GPIOA_CLK_ENABLE();

    /* UART1 clock (APB1) [3] */
    __SYSCTRL_UART1_CLK_ENABLE();

    /* --- Configure GPIO pins for UART1 alternate function --- */

    /* PA06 as UART1_TXD: output push-pull [2] */
    gpio_cfg.Pins = UART_TX_PIN;
    gpio_cfg.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_cfg.IT   = GPIO_IT_NONE;
    GPIO_Init(CW_GPIOA, &gpio_cfg);

    /* PA05 as UART1_RXD: input pull-up [2] */
    gpio_cfg.Pins = UART_RX_PIN;
    gpio_cfg.Mode = GPIO_MODE_INPUT_PULLUP;
    gpio_cfg.IT   = GPIO_IT_NONE;
    GPIO_Init(CW_GPIOA, &gpio_cfg);

    /* Enable digital function on PA05 and PA06 [2] */
    PA05_DIGTAL_ENABLE();
    PA06_DIGTAL_ENABLE();

    /* Route alternate function 1 (UART1) to PA06 (TXD) and PA05 (RXD) [2] */
    PA06_AFx_UART1TXD();
    PA05_AFx_UART1RXD();

    /* --- Configure UART1 --- */
    /* UART_InitStructure defaults [1] */
    uart_cfg.UART_BaudRate          = UART_CONSOLE_BAUDRATE;
    uart_cfg.UART_Over              = UART_Over_16;         /* 16x oversampling */
    uart_cfg.UART_Source            = UART_Source_PCLK;     /* PCLK as UCLK */
    uart_cfg.UART_UclkFreq          = uclk_freq;            /* e.g. SystemCoreClock */
    uart_cfg.UART_StartBit          = UART_StartBit_FE;     /* falling-edge detect */
    uart_cfg.UART_StopBits          = UART_StopBits_1;      /* 1 stop bit */
    uart_cfg.UART_Parity            = UART_Parity_No;       /* no parity → 8 data bits */
    uart_cfg.UART_HardwareFlowControl = UART_HardwareFlowControl_None;
    uart_cfg.UART_Mode              = UART_Mode_Tx;          /* TX only */

    UART_Init(UART_CONSOLE, &uart_cfg);

    return ERR_OK;
}

void uart_console_deinit(void)
{
    /* Wait for any pending transmission to complete */
    uart_wait_tc();

    /* De-initialize UART1 registers [1] */
    UART1_DeInit();

    /* Reconfigure PA05, PA06 as GPIO input (high-Z, lowest leakage) [2] */
    GPIO_InitTypeDef gpio_cfg;
    gpio_cfg.Pins = UART_TX_PIN | UART_RX_PIN;
    gpio_cfg.Mode = GPIO_MODE_INPUT;    /* high-Z input */
    gpio_cfg.IT   = GPIO_IT_NONE;
    GPIO_Init(CW_GPIOA, &gpio_cfg);

    /* Disable UART1 clock to save power [3] */
    __SYSCTRL_UART1_CLK_DISABLE();
}

/* =========================================================================
 * Character Output
 * ========================================================================= */

void uart_putchar(char ch)
{
    /* Handle newline expansion: LF → CR+LF */
    if (ch == '\n')
    {
        uart_wait_txe();
        UART_SendData_8bit(UART_CONSOLE, (uint8_t)'\r');
    }

    uart_wait_txe();
    UART_SendData_8bit(UART_CONSOLE, (uint8_t)ch);
}

void uart_puts(const char *str)
{
    while (*str != '\0')
    {
        uart_putchar(*str++);
    }
}

void uart_write(const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
    {
        uart_putchar((char)data[i]);
    }
}

/* =========================================================================
 * Lightweight printf
 * =========================================================================
 *
 * Supports: %s, %d, %i, %u, %x, %X, %c, %%, and basic %f (2 decimal places).
 * Does NOT support: width/precision specifiers, %ld, %lu, %p, %e, %g.
 * Uses a small stack buffer — keep format strings short (< 64 chars).
 * ========================================================================= */

#define PRINTF_BUF_SIZE 64

static void print_dec(int32_t val)
{
    char buf[12];
    char *p = buf + sizeof(buf) - 1;
    uint32_t uval;
    int neg = 0;

    if (val < 0)
    {
        neg = 1;
        uval = (uint32_t)(-val);
    }
    else
    {
        uval = (uint32_t)val;
    }

    *p = '\0';
    if (uval == 0)
    {
        *--p = '0';
    }
    else
    {
        while (uval > 0)
        {
            *--p = '0' + (uval % 10);
            uval /= 10;
        }
    }

    if (neg)
    {
        *--p = '-';
    }

    uart_puts(p);
}

static void print_hex(uint32_t val, int upper)
{
    char buf[12];
    char *p = buf + sizeof(buf) - 1;
    const char *hex = upper ? "0123456789ABCDEF" : "0123456789abcdef";

    *p = '\0';
    if (val == 0)
    {
        *--p = '0';
    }
    else
    {
        while (val > 0)
        {
            *--p = hex[val & 0x0F];
            val >>= 4;
        }
    }

    uart_puts(p);
}

static void print_unsigned(uint32_t val)
{
    char buf[12];
    char *p = buf + sizeof(buf) - 1;

    *p = '\0';
    if (val == 0)
    {
        *--p = '0';
    }
    else
    {
        while (val > 0)
        {
            *--p = '0' + (val % 10);
            val /= 10;
        }
    }

    uart_puts(p);
}

/* Format a float as "int.dec" with 2 decimal places.
 * E.g. 254 -> "25.4" (if scale=10), 2541 -> "25.41" (if scale=100) */
static void print_fixed(int32_t val, int scale, int decimals)
{
    int32_t int_part = val / scale;
    uint32_t frac_part;
    char buf[16];
    char *p = buf + sizeof(buf) - 1;

    if (val < 0)
    {
        int_part = -(-val / scale);
        frac_part = (uint32_t)((-val) % scale);
        if (int_part == 0)
        {
            /* -0.5x, print "-0.xx" */
            *--p = '\0';
        }
    }
    else
    {
        frac_part = (uint32_t)(val % scale);
    }

    /* Build fractional part from right */
    *p = '\0';
    for (int i = 0; i < decimals; i++)
    {
        *--p = '0' + (frac_part % 10);
        frac_part /= 10;
    }
    *--p = '.';

    /* Integer part */
    if (int_part == 0 && val < 0)
    {
        *--p = '0';
        *--p = '-';
    }
    else
    {
        uint32_t u = (int_part < 0) ? (uint32_t)(-int_part) : (uint32_t)int_part;
        if (u == 0) *--p = '0';
        else
        {
            while (u > 0)
            {
                *--p = '0' + (u % 10);
                u /= 10;
            }
        }
        if (int_part < 0) *--p = '-';
    }

    uart_puts(p);
}

void uart_vprintf(const char *fmt, va_list args)
{
    for (const char *c = fmt; *c != '\0'; c++)
    {
        if (*c != '%')
        {
            uart_putchar(*c);
            continue;
        }

        c++; /* skip '%' */
        switch (*c)
        {
            case 'd':
            case 'i':
                print_dec(va_arg(args, int32_t));
                break;

            case 'u':
                print_unsigned(va_arg(args, uint32_t));
                break;

            case 'x':
                print_hex(va_arg(args, uint32_t), 0);
                break;

            case 'X':
                print_hex(va_arg(args, uint32_t), 1);
                break;

            case 's':
                uart_puts(va_arg(args, const char *));
                break;

            case 'c':
                uart_putchar((char)va_arg(args, int));
                break;

            case '%':
                uart_putchar('%');
                break;

            case 'f':
            {
                /* Basic float: expects int scaled by 1000, prints with 3 decimals
                 * e.g. 25412 -> "25.412" */
                int32_t fv = va_arg(args, int32_t);
                print_fixed(fv, 1000, 3);
                break;
            }

            default:
                /* Unknown format — print as-is */
                uart_putchar('%');
                uart_putchar(*c);
                break;
        }
    }
}

void uart_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    uart_vprintf(fmt, args);
    va_end(args);
}
