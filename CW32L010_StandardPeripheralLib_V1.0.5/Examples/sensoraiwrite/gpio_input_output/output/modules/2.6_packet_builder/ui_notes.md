# UI/HMI Notes — Phase 2.6: packet_builder

## 1. UI Context Summary

**Source**: `meta/ui/UI_CONTEXT.md`

No LCD/serial-screen UI image assets were found. The product has no display.

## 2. User-Facing Output

The product has two forms of output:

### 2.1 UART Console (Debug, Compile-Time Toggle)
- **Hardware**: J3 connector, UART1 on PA05(RXD)/PA06(TXD), 115200 8N1
- **Format**: `[T=25.4C H=62.3% TX=OK]\n`
- **Toggle**: `#define CONFIG_CONSOLE_ENABLE 1/0` in board config
- **Source**: COMMON.md#5.1

### 2.2 RF Transmission (Primary Output)
- **Hardware**: UM2005C 433 MHz FSK transmitter, 0 dBm
- **Data**: 14-byte binary packet (no human-readable format over RF)
- **Receiver**: External device (not in scope)

## 3. packet_builder UI Responsibilities

**The packet_builder module has no UI responsibilities.** It is a service-layer module that:
1. Assembles sensor data into the binary RF packet format
2. Provides the formatted sensor data (temperature_x10, humidity_x10) that is used by the console print module (uart_console)

The console print formatting is handled by:
- `uart_console` module (phase 2.3) for actual UART TX
- `sensor_mgr` or `schedule` application layer for formatting the console string

## 4. Data Flow for Display

```
sensor_mgr (avg data)
    |
    ├──▶ packet_builder.pkt_build() ──▶ rf_twi_driver.rf_twi_transmit()
    |
    └──▶ uart_console.printf() ──▶ "%.1f°C %.1f%%" format
```

## 5. Related Files

| File | Role |
|------|------|
| `output/modules/2.3_uart_console/interface.h` | UART console API for debug output |
| `output/modules/2.3_uart_console/protocol_notes.md` | Console output format details |
| `output/design/COMMON.md#5.1` | Compile-time print toggle config |
| `meta/ui/UI_CONTEXT.md` | UI context (no display assets found) |
