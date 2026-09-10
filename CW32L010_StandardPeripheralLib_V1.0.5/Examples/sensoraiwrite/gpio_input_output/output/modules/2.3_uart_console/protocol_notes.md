# Protocol Notes: UART Console (Phase 2.3)

## 1. Overview

This module provides a **debug console output** over UART1 on the CW32L010Y8M6.  
It is **not a communication protocol with an external device** — it is a firmware debug/logging interface.

**Protocol**: Standard UART (asynchronous serial)  
**Role**: MCU → PC (TX only, no RX processing)  
**Physical layer**: RS-232 levels via USB-UART adapter (J3 connector)

## 2. UART Configuration

| Parameter | Value | Notes |
|-----------|-------|-------|
| Baud rate | 115200 | ±2% tolerance; 48 MHz PCLK / (16 × BRRI) |
| Data bits | 8 | Fixed when parity = none [cw32l010_uart.h#L136] |
| Parity | None | UART_Parity_No |
| Stop bits | 1 | UART_StopBits_1 |
| Flow control | None | UART_HardwareFlowControl_None |
| Bit order | LSB first | Standard UART |
| Mode | TX only | UART_Mode_Tx |
| Oversampling | 16× | UART_Over_16 (more jitter tolerant) |
| Clock source | PCLK | UART_Source_PCLK (APB1 clock) |

## 3. Frame Format

```
Start Bit (low) → Data Bit 0 (LSB) → ... → Data Bit 7 (MSB) → Stop Bit (high)

Idle: TX line is high (MARK)
Start: high→low transition (SPACE)
Data: 8 bits, LSB first
Stop: returns high for 1 bit time
```

**Bit time**: 1/115200 ≈ 8.68 μs  
**Frame time**: 10 bits × 8.68 μs ≈ 86.8 μs per character

## 4. Line Discipline

| Feature | Behavior | Source |
|---------|----------|--------|
| Line ending | `\n` (0x0A) → `\r\n` (0x0D 0x0A) | uart_console.c uart_putchar() |
| Raw binary | `uart_write()` sends bytes as-is, no CR/LF expansion | uart_console.c uart_write() |
| Backspace | Not handled by driver; terminal handles BS/ECHO | N/A |

## 5. TX Timing

```
uart_putchar('A'):
  1. Poll UART_FLAG_TXE until set  [~0–86.8 μs wait if busy]
  2. Write 0x41 to UART_DR         [1 APB cycle]
  3. UART shifts out 10-bit frame  [~86.8 μs hardware]
  4. (Return; next char polls TXE again)

Peak TX throughput: ~11520 chars/sec (limited by UART line rate)
Bottleneck: No — TXE polling is faster than wire speed
```

## 6. Power Impact

| State | UART1 Clock | GPIO PA05/PA06 | Current |
|-------|-------------|----------------|---------|
| Active TX (@115200) | Enabled | AF1 (UART) | ~3-5 mA (MCU active) |
| Idle (after init) | Enabled | AF1 (UART), line idle high | ~3-5 mA (MCU active) |
| De-initialized | Disabled | GPIO input (high-Z) | <1 μA leakage |
| CONFIG_CONSOLE_ENABLE=0 | Never enabled | GPIO input (high-Z) | 0 additional current |

**Note**: For the battery-powered application (150s cycle, ~200ms active time), UART console should remain disabled (`CONFIG_CONSOLE_ENABLE=0`) in production to save power.

## 7. Connector Pinout (J3)

```
J3 (CON3, 3-pin header)
  Pin 1: UART1_TXD ← MCU PA06
  Pin 2: UART1_RXD → MCU PA05 (not used by TX-only console)
  Pin 3: GND
```

[connectivity.json#buses[UART], CW32L010_DataSheet_CN_V1.0.pdf#page=24]

## 8. Compile-Time Toggle Architecture

```c
// In interface.h:
#if CONFIG_CONSOLE_ENABLE
    #define CONSOLE_PRINT(fmt, ...)   uart_printf(fmt, ##__VA_ARGS__)
    // ... other macros expand to real functions
#else
    #define CONSOLE_PRINT(fmt, ...)   ((void)0)
    // ... all macros expand to no-ops
#endif
```

When disabled:
- All `CONSOLE_*` macros produce zero code (compiler optimizes away `((void)0)`)
- `uart_console_init()` must NOT be called (or wrapped with `#if CONFIG_CONSOLE_ENABLE`)
- UART1 peripheral stays in reset state (clock gated)
- PA05/PA06 default to GPIO input after reset → minimal leakage

## 9. Error Handling

| Condition | Behavior |
|-----------|----------|
| TXE poll timeout | None (no timeout — infinite spin) |
| UART overrun/error | Not monitored (TX only, no RX) |
| Buffer overflow | Not applicable (no RX buffer) |
| Deinit while sending | `uart_wait_tc()` ensures last byte is sent before clock disable |

## 10. Data Format Convention (for log output)

The application layer uses a space-separated key=value format:

```
[T=25.4 H=62.3 TX=OK UID=00A1B2C3D4E5]
```

Fields:
- `T=xx.x` : Temperature in °C (×10 format)
- `H=xx.x` : Relative humidity in % (×10 format)
- `TX=OK/FAIL` : RF transmission status
- `UID=xxxxxxxxxxxx` : Lower 48 bits of MCU UID in hex

This format is defined by `COMMON.md#1.2 Execution Flow`.

## 11. Sources

| Item | Source |
|------|--------|
| UART baud calculation | cw32l010_uart.h#L38-L43 |
| UART_InitTypeDef | cw32l010_uart.h#L36-L69 |
| UART_SendData_8bit | cw32l010_uart.h#L283 |
| UART_FLAG_TXE | cw32l010_uart.h#L257 |
| UART_FLAG_TC | cw32l010_uart.h#L256 |
| Pin mapping PA05=UART1_RXD | CW32L010_DataSheet_CN_V1.0.pdf#page=24 |
| Pin mapping PA06=UART1_TXD | CW32L010_DataSheet_CN_V1.0.pdf#page=24 |
| J3 connector | connectivity.json#buses[UART] |
| Print toggle requirement | inputs/user_req.txt (compile-time toggle) |
| UCLK baud formula | cw32l010_uart.h L40: BaudRate = UCLK / (16 * BRRI + BRRF) |
