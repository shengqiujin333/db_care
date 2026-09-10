# USER_MANUAL.md — Temperature & Humidity Sensor Node (433 MHz FSK)

> **Product**: Battery-powered wireless temperature and humidity transmitter  
> **Model**: CW32L010-TH-Sensor v1.0  
> **Firmware Version**: 1.0  
> **Document Version**: 1.0 — July 2025

---

## Table of Contents

1. [Product Overview](#1-product-overview)
2. [Features](#2-features)
3. [Technical Specifications](#3-technical-specifications)
4. [Hardware Description](#4-hardware-description)
   - 4.1 Board Layout and Connectors
   - 4.2 LED Indicators
   - 4.3 Pinout Table
5. [Operation](#5-operation)
   - 5.1 Power On / Startup Sequence
   - 5.2 Normal Operation (Measurement Cycle)
   - 5.3 Two-Sample Averaging
   - 5.4 RF Transmission
6. [RF Packet Protocol](#6-rf-packet-protocol)
   - 6.1 Packet Format
   - 6.2 Field Definitions
   - 6.3 Packet Example
   - 6.4 CRC Computation
7. [Power Management](#7-power-management)
   - 7.1 CR2032 Battery Life
   - 7.2 DeepSleep Mode
   - 7.3 Battery Monitoring
8. [UART Debug Console](#8-uart-debug-console)
   - 8.1 Connecting to the Console
   - 8.2 Console Output Format
   - 8.3 Console Commands
   - 8.4 Compile-Time Disable
9. [Troubleshooting](#9-troubleshooting)
10. [Design Notes and Known Limitations](#10-design-notes-and-known-limitations)
11. [Reference Documents](#11-reference-documents)

---

## 1. Product Overview

The CW32L010-TH-Sensor is a battery-powered wireless sensor node that measures ambient temperature and relative humidity at regular intervals (every 150 seconds) and transmits the data over a 433 MHz FSK RF link. It is designed for remote environmental monitoring applications where wiring is impractical.

The product is built around three key components:

- **CW32L010Y8M6** — Ultra-low-power ARM Cortex-M0+ microcontroller (MCU)
- **AHT10** — Digital temperature and humidity sensor (I2C interface)
- **UM2005C** — 433 MHz FSK RF transmitter (TWI interface)

All components are powered by a single CR2032 coin cell battery, with an estimated battery life of 5–8 years under normal operating conditions.

---

## 2. Features

- **Temperature measurement**: –40°C to +85°C, ±0.3°C typical accuracy
- **Humidity measurement**: 0% to 100% RH, ±2% typical accuracy
- **Two-sample averaging**: Each reading is the average of two consecutive measurements for improved accuracy
- **433 MHz FSK RF transmission**: Up to ~100m range (line-of-sight)
- **Unique device identifier**: Each unit transmits its 48-bit MCU UID for receiver-side identification
- **CRC-8/MAXIM error detection**: Every packet includes a checksum for data integrity
- **CR2032 battery operation**: Ultra-low-power DeepSleep between transmissions
- **150-second measurement interval**: Fixed cycle time
- **UART debug console** (optional): Compile-time toggleable serial output for development and troubleshooting
- **RF output power**: 0 dBm (1 mW)

---

## 3. Technical Specifications

### 3.1 General

| Parameter | Value | Notes |
|-----------|-------|-------|
| MCU | CW32L010Y8M6 (TSSOP-20) | ARM Cortex-M0+, 48 MHz, 64 KB Flash, 4 KB RAM |
| Sensor | AHT10 | Digital temp/humidity, I2C, address 0x38 |
| RF Transmitter | UM2005C (SOT23-6L) | Sub-1GHz FSK/OOK transmitter |
| RF Frequency | 433 MHz | ISM band |
| RF Modulation | FSK (Frequency Shift Keying) | — |
| RF Output Power | 0 dBm (1 mW) | Configurable in OTP |
| Data Rate (RF) | 0.5–100 kbps | Configurable via UM2005C OTP registers |
| Antenna | PCB trace / wire | Via matching network (L2=180nH, C10=6.8pF, L3=47nH, C11=2.7pF) |
| Supply | CR2032 coin cell (3V nominal) | 1.9–3.6V input range |
| Battery Life | 5–8 years (estimated) | Depends on battery quality and temperature |
| Measurement Interval | 150 seconds | Fixed (configurable via UART command) |
| Measurement Averaging | 2 samples | Two consecutive reads averaged |
| Operating Temperature | –40°C to +85°C | MCU limit; AHT10 range –40°C to +85°C |
| Dimensions | ~25 mm × 20 mm (PCB) | Depends on enclosure |

### 3.2 Power Consumption

| Mode | Current | Duration per Cycle |
|------|---------|-------------------|
| Active (measure + TX) | ~8–12 mA (MCU + AHT10 + UM2005C) | ~200 ms |
| UART TX (when enabled) | ~3–5 mA additional | ~2 ms per line |
| DeepSleep | ~1–2 µA (RTC + LSI + RAM retention) | Remaining ~149.8 s |
| **Average current** | **~3–5 µA** | 150 s cycle |

### 3.3 RF Characteristics

| Parameter | Value | Notes |
|-----------|-------|-------|
| Frequency | 433.0–434.0 MHz (programmable) | Set via UM2005C OTP |
| Modulation | FSK / GFSK | OOK also supported by UM2005C |
| Output Power | 0 dBm (configurable: –13 to +18 dBm) | Via OTP register CFG2 |
| Data Rate | Configurable (0.5–100 kbps) | Via OTP register |
| Harmonic Emissions | < –36 dBm | Per EN 300 220 |
| Crystal Frequency | 32 MHz (X322526MOB4SI) | ⚠ App note specifies 26 MHz; verify |

---

## 4. Hardware Description

### 4.1 Board Layout and Connectors

```
┌──────────────────────────────────────┐
│   J2 (SWD Debug)  J3 (UART)         │
│   ┌─────┐          ┌────┐           │
│   │ 1 2 │          │ 1  │           │
│   │ 3 4 │          │ 2  │           │
│   │  5  │          │ 3  │           │
│   └─────┘          └────┘           │
│                                      │
│   U7 (CW32L010 MCU)                  │
│   ┌────────────────────────┐         │
│   │  TSSOP-20              │        │
│   └────────────────────────┘         │
│                                      │
│   U3 (AHT10 Sensor)   U6 (Crystal)  │
│   ┌────┐              ┌────────┐    │
│   │SOIC│              │32MHz   │    │
│   └────┘              └────────┘    │
│                                      │
│          U4 (UM2005C RF TX)         │
│          ┌──────────────┐           │
│          │   SOT23-6L   │           │
│          └──────────────┘           │
│                                      │
│   J1 (BAT)   Antenna (via matching) │
│   ┌────┐     ┌────────────────────┐ │
│   │ +  │     │   Matching Network │ │
│   │ –  │     │   (L2,C10,L3,C11) │ │
│   └────┘     └────────────────────┘ │
└──────────────────────────────────────┘
```

### 4.2 Connectors

#### J1 — Battery Connector (CON2)

| Pin | Signal | Description |
|-----|--------|-------------|
| 1 | BAT (+) | CR2032 positive terminal (3V) |
| 2 | GND (–) | CR2032 negative terminal |

#### J2 — SWD Debug Connector (HEADER 5)

| Pin | Signal | Direction | Description |
|-----|--------|-----------|-------------|
| 1 | BAT | Power | 3V supply output (for debug probe) |
| 2 | SWCLK | Input | SWD clock |
| 3 | SWDIO | Bidirectional | SWD data |
| 4 | GND | — | Ground |
| 5 | NRST | Input | Reset (active low) |

**Compatible debug probes**: CW-DAPLINK, J-Link, ST-Link/V2 (SWD mode), DAPLink

#### J3 — UART Console Connector (CON3)

| Pin | Signal | Direction | Description |
|-----|--------|-----------|-------------|
| 1 | TXD | Output | MCU UART1 transmit (115200 8N1) |
| 2 | RXD | Input | MCU UART1 receive (for commands) |
| 3 | GND | — | Ground |

**Connection**: Use a USB-UART adapter (e.g., CP2102, CH340) at 115200 baud, 8 data bits, no parity, 1 stop bit (8N1). No hardware flow control.

### 4.3 MCU Pinout

| Pin | MCU Port | Function | Connected To | Notes |
|-----|----------|----------|-------------|-------|
| 4 | PB07 | NRST | J2-5 | Reset input (active low) |
| 7 | VSS | GND | Ground plane | — |
| 8 | Vcore | Vcore out | C12 (100nF → GND) | Internal LDO decoupling |
| 9 | VDD | Power | BAT rail | 1.62–5.5V |
| 13 | PA03 | I2C_SDA (AF2) | AHT10 pin 3 | Pull-up R2 (4.7KΩ) to BAT |
| 14 | PA04 | I2C_SCL (AF2) | AHT10 pin 2 | Pull-up R1 (4.7KΩ) to BAT |
| 15 | PA05 | UART1_RXD (AF1) | J3-2 | UART receive |
| 16 | PA06 | UART1_TXD (AF1) | J3-1 | UART transmit |
| 17 | PA07 | SWDIO | J2-3 | SWD debug data |
| 18 | PA08 | SWCLK | J2-2 | SWD debug clock |
| 19 | PB02 | TWI_CLK (GPIO) | UM2005C pin 4 | Bit-banged TWI clock |
| 20 | PB03 | TWI_DATA (GPIO) | UM2005C pin 3 | Bit-banged TWI data |

### 4.4 UM2005C RF Pinout

| Pin | Name | Connected To | Notes |
|-----|------|-------------|-------|
| 1 | XTAL | Crystal oscillator (32 MHz) | Load caps TBD |
| 2 | GND | Ground | — |
| 3 | DATA | MCU PB03 | TWI data |
| 4 | CLK | MCU PB02 | TWI clock |
| 5 | RFO | Matching network → Antenna | Single-ended 50Ω output |
| 6 | VDD | BAT rail | 1.9–3.6V |

### 4.5 AHT10 Sensor Pinout

| Pin | Name | Connected To | Notes |
|-----|------|-------------|-------|
| 1 | GND | Ground | — |
| 2 | SCL | MCU PA04 (via 4.7KΩ pull-up) | I2C clock |
| 3 | SDA | MCU PA03 (via 4.7KΩ pull-up) | I2C data |
| 4 | VDD | BAT rail | 1.8–3.6V |

---

## 5. Operation

### 5.1 Power On / Startup Sequence

1. Insert a CR2032 battery into J1 (observe polarity: positive to BAT pin)
2. The MCU powers on and executes the boot sequence:
   - Clock initialization (HSIOSC at 48 MHz)
   - GPIO configuration for I2C, UART, TWI
   - I2C peripheral initialization (100 kHz)
   - UART initialization (115200 8N1, if console enabled)
   - IWDT start (independent watchdog, ~2s timeout)
   - RTC configuration (LSI-driven, alarm set for +150s)
3. First measurement cycle begins immediately after boot
4. After the first cycle, the MCU enters DeepSleep

**Note**: There is no power switch. The device starts transmitting immediately when the battery is inserted.

### 5.2 Normal Operation (Measurement Cycle)

The device operates on a repeating 150-second cycle:

```
┌─────────────────────────────────────────────────────────┐
│  RTC Alarm Wake                                         │
│      ↓                                                  │
│  [DeepSleep exit] → HSIOSC startup (~4μs)              │
│      ↓                                                  │
│  Sample #1: Trigger AHT10 → wait 75ms → read 6 bytes  │
│      ↓                                                  │
│  ~100ms delay                                           │
│      ↓                                                  │
│  Sample #2: Trigger AHT10 → wait 75ms → read 6 bytes  │
│      ↓                                                  │
│  Compute average temperature and humidity               │
│      ↓                                                  │
│  Build RF packet (14 bytes)                             │
│      ↓                                                  │
│  Wake UM2005C → TWI_ON → write config → write data     │
│      → TWI_OFF → sleep UM2005C                          │
│      ↓                                                  │
│  Print to UART console (if enabled at compile time)     │
│      ↓                                                  │
│  Set RTC alarm for +150s                                │
│      ↓                                                  │
│  Enter DeepSleep                                        │
│      ↓                                                  │
│  [wait ~150 seconds...]                                 │
└─────────────────────────────────────────────────────────┘
```

**Active time per cycle**: ~200 milliseconds  
**Sleep time per cycle**: ~149.8 seconds

### 5.3 Two-Sample Averaging

Per product requirements, each measurement cycle performs two consecutive temperature and humidity readings:

- **Sample #1** is taken at t=0 (immediately after wake)
- **Sample #2** is taken approximately 100 ms after Sample #1
- Both samples use the same AHT10 trigger sequence (0xAC 0x33 0x00) with a 75 ms wait

**Averaging formula**:
```
temperature_x10 = (sample1.temp + sample2.temp) / 2
humidity_x10    = (sample1.hum + sample2.hum) / 2
```

Where `_x10` means the value is multiplied by 10 for integer transmission (e.g., 25.4°C → 254).

### 5.4 RF Transmission

After averaging, the firmware:
1. Assembles a 14-byte RF packet (see [Section 6](#6-rf-packet-protocol))
2. Wakes the UM2005C RF transmitter via TWI protocol
3. Writes configuration and payload data
4. The UM2005C modulates and transmits the packet at 433 MHz FSK
5. The UM2005C is returned to sleep (10 nA standby)

The RF transmission sequence takes approximately 1–2 ms of active time. The total added current consumption from RF TX is negligible due to the 150 s cycle.

---

## 6. RF Packet Protocol

### 6.1 Packet Format

Each RF transmission contains a 14-byte packet:

```
Offset:   0   1   2   3   4   5   6   7   8   9  10  11  12  13
        ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
        │Preamble    │       UID (48 bits)       │ Temp  │ Hum  │STS│CRC│
        │0xAA|0x55   │B0│B1│B2│B3│B4│B5│  MSB│LSB│MSB│LSB│    │   │
        └───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘
Size:      2       6           2       2       1       1
                               │       │       │       │
                               │       │       │       └─ CRC-8/MAXIM
                               │       │       └───────── Status byte
                               │       └───────────────── Humidity ×10 (uint16 BE)
                               └───────────────────────── Temperature ×10 (int16 BE)
```

**Total size**: 14 bytes

### 6.2 Field Definitions

| Field | Size | Type | Description |
|-------|------|------|-------------|
| Preamble | 2 bytes | uint8[2] | Fixed sync pattern: `0xAA 0x55`. Used by receiver for bit timing synchronization. |
| MCU UID | 6 bytes | uint8[6] | Lower 48 bits of the CW32L010 96-bit unique identifier. Stored at Flash address 0x1FFFF7AC (first 6 bytes). Each device has a globally unique ID. |
| Temperature | 2 bytes | int16 big-endian | Temperature value × 10. Range: –500 to +850 (–50.0°C to +85.0°C). Example: 25.4°C = 254 = 0x00 0xFE. |
| Humidity | 2 bytes | uint16 big-endian | Relative humidity × 10. Range: 0 to 1000 (0.0% to 100.0%). Example: 62.3% = 623 = 0x02 0x6F. |
| Status | 1 byte | uint8 | System status bitfield (see below). |
| CRC-8 | 1 byte | uint8 | CRC-8/MAXIM checksum over bytes 0–12 (13 bytes). Polynomial 0x31. |

#### Status Byte (bitfield)

| Bit | Name | Description |
|-----|------|-------------|
| 0 | BAT_LOW | 1 = Battery voltage below ~2.4V threshold. Replace battery soon. |
| 1 | SENSOR_VALID | 1 = Sensor data is fresh and valid. If 0, ignore temperature/humidity values. |
| 2 | SENSOR_FAIL | 1 = Last sensor read attempt failed. Data may be stale or incorrect. |
| 3 | RF_DONE | 1 = RF transmission completed successfully. |
| 4–7 | Reserved | Always 0. Reserved for future use. |

### 6.3 Packet Example

**Scenario**: Temperature = 25.4°C, Humidity = 62.3%, UID = 0xAABBCCDDEEFF, Status = 0x07 (all bits valid)

```
Byte    Value    Description
─────────────────────────────────────
 0      0xAA     Preamble byte 0
 1      0x55     Preamble byte 1
 2      0xAA     UID byte 0 (LSB of UID word 0 at 0x1FFFF7AC)
 3      0xBB     UID byte 1
 4      0xCC     UID byte 2
 5      0xDD     UID byte 3
 6      0xEE     UID byte 4
 7      0xFF     UID byte 5 (MSB)
 8      0x00     Temperature MSB (254 >> 8)
 9      0xFE     Temperature LSB (254 & 0xFF) → 25.4°C
10      0x02     Humidity MSB (623 >> 8)
11      0x6F     Humidity LSB (623 & 0xFF) → 62.3%
12      0x07     Status: BAT_LOW=1, SENSOR_VALID=1, RF_DONE=1
13      0x??     CRC-8/MAXIM over bytes 0–12
─────────────────────────────────────
```

### 6.4 CRC Computation

The CRC-8 used is CRC-8/MAXIM (also known as Dallas 1-Wire CRC):

- **Polynomial**: 0x31 (x⁸ + x⁵ + x⁴ + 1)
- **Initial value**: 0x00
- **Final XOR**: 0x00
- **Input reflected**: No
- **Output reflected**: No
- **Coverage**: Bytes 0 through 12 inclusive (13 bytes)

Python reference implementation:

```python
def crc8_maxim(data: bytes) -> int:
    """Compute CRC-8/MAXIM over a byte array."""
    crc = 0x00
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x80:
                crc = ((crc << 1) ^ 0x31) & 0xFF
            else:
                crc = (crc << 1) & 0xFF
    return crc

# Example:
# packet = bytes([0xAA, 0x55, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
#                 0x00, 0xFE, 0x02, 0x6F, 0x07])
# crc = crc8_maxim(packet)
```

**Receiver behavior**: The receiving device recalculates the CRC-8 over the first 13 bytes and compares it to the transmitted CRC byte. A mismatch indicates data corruption, and the packet should be discarded.

---

## 7. Power Management

### 7.1 CR2032 Battery Life

The device is designed for long-term battery operation on a single CR2032 coin cell (nominal 225 mAh capacity).

| Parameter | Value | Notes |
|-----------|-------|-------|
| Battery capacity | 225 mAh | Typical for CR2032 at low drain |
| Average current | ~3–5 µA | 150 s cycle |
| Estimated life | **5–8 years** | Depends on battery self-discharge and temperature |
| Operating voltage range | 1.9–3.6V | Limited by UM2005C minimum (MCU works down to 1.62V) |
| End-of-life indicator | BAT_LOW status bit | Set when battery drops below ~2.4V |

**Battery life calculation**:
```
225 mAh / 5 µA = 45,000 hours ≈ 5.1 years
225 mAh / 3 µA = 75,000 hours ≈ 8.6 years
```

### 7.2 DeepSleep Mode

Between measurement cycles, the MCU enters DeepSleep mode:

**Active peripherals during DeepSleep**:
- LSI (Low-Speed Internal oscillator, ~38 kHz)
- RTC (keeps time, generates alarm after 150 s)
- IWDT (Independent Watchdog, safety reset)

**Disabled peripherals**:
- HSIOSC (48 MHz main clock)
- HSE (external crystal, not used for MCU)
- I2C, UART, all GPIO clocks

**Wake source**: RTC alarm (only)

**Wakeup time**: ~4 µs (HSIOSC restart) + ~2 µs (firmware resume) ≈ 6 µs total

### 7.3 Battery Monitoring

Battery voltage is monitored through the UM2005C's internal Low Battery Detection (LBD) feature:

- LBD threshold: Configurable 2.0–3.1V in 0.1V steps (default: 2.4V)
- LBD value readable via TWI register 0x3E
- When LBD triggers, the BAT_LOW status bit (bit 0) is set in the RF packet

**Recommended battery replacement voltage**: ~2.4V (set in UM2005C OTP LBD_CFG)

---

## 8. UART Debug Console

The device includes an optional UART debug console for development, testing, and field diagnostics. The console can be permanently disabled for production units to reduce power consumption.

### 8.1 Connecting to the Console

1. Connect a USB-UART adapter to J3:
   - Adapter TX → J3 pin 2 (MCU RXD)
   - Adapter RX → J3 pin 1 (MCU TXD)
   - Adapter GND → J3 pin 3
2. Configure the terminal software (PuTTY, TeraTerm, screen):
   - Baud rate: **115200**
   - Data bits: **8**
   - Parity: **None**
   - Stop bits: **1**
   - Flow control: **None**

### 8.2 Console Output Format

After each measurement cycle, the device outputs a single line:

```
[T=25.4C H=62.3% UID=AABBCCDDEEFF STS=0x07 CRC=OK]
```

| Field | Example | Description |
|-------|---------|-------------|
| `T=25.4C` | 25.4°C | Temperature in Celsius, one decimal place |
| `H=62.3%` | 62.3% | Relative humidity, one decimal place |
| `UID=AABBCCDDEEFF` | Unique ID | 48-bit MCU UID in hexadecimal |
| `STS=0x07` | Status byte | Hex value of status byte (see Section 6.2) |
| `CRC=OK` or `CRC=FAIL` | — | CRC verification (always OK from sender) |

Example output sequence (two cycles):
```
[T=25.4C H=62.3% UID=AABBCCDDEEFF STS=0x07 CRC=OK]
[T=25.3C H=62.5% UID=AABBCCDDEEFF STS=0x0B CRC=OK]
```

### 8.3 Console Commands

The device accepts single-character commands via the UART RX line:

| Command | Format | Response | Description |
|---------|--------|----------|-------------|
| Read now | `r` + newline | `[T=... H=...]` | Triggers an immediate sensor read and RF transmission, outside the normal cycle |
| Set interval | `i 60` + newline | `[Interval=60s]` | Changes the wake interval. Valid range: 10–600 seconds |
| Status | `s` + newline | `[Uptime=... Count=...]` | Shows system status: uptime in seconds, total transmit count |
| Sleep immediately | `slp` + newline | (no response) | Forces the device into DeepSleep immediately, skipping the current interval |

**Note**: The UART RX command interface is **not available** if the console is compiled out (`CONFIG_CONSOLE_ENABLE = 0`).

### 8.4 Compile-Time Disable

To reduce power consumption for production units, the UART console can be disabled at compile time:

```c
// In board.h or project configuration:
#define CONFIG_CONSOLE_ENABLE   0   // 0 = disable console, 1 = enable
```

When disabled:
- UART1 peripheral clock is gated off
- PA05 (RXD) and PA06 (TXD) pins are reconfigured as high-impedance GPIO inputs
- No UART TX current consumption (~3–5 mA saved during TX)
- Console commands are not processed

The firmware build process generates two binaries:
- `firmware_debug.hex` — with CONFIG_CONSOLE_ENABLE=1
- `firmware_production.hex` — with CONFIG_CONSOLE_ENABLE=0

---

## 9. Troubleshooting

| Symptom | Likely Cause | Solution |
|---------|-------------|----------|
| No RF signal at receiver | Battery not inserted or depleted | Check battery polarity and voltage (>2.0V required) |
| | UM2005C not waking | Verify MCU firmware is running (check with debug probe or UART) |
| | Crystal not oscillating | Check X322526MOB4SI crystal and load capacitors. ⚠ **Known issue**: Crystal is 32 MHz but UM2005C app note specifies 26 MHz. |
| | Antenna matching | Verify L2 (180nH), C10 (6.8pF), L3 (47nH), C11 (2.7pF) are correct values and properly soldered |
| Intermittent RF signal | Battery voltage dropping below 1.9V | Replace CR2032 battery |
| | Weak solder joints | Inspect UM2005C pins under microscope |
| UART console shows nothing | Console disabled at compile time | Use firmware_debug.hex build |
| | USB-UART adapter not connected correctly | Check TX↔RX crossover (MCU TXD → adapter RX) |
| | Baud rate mismatch | Ensure terminal is set to 115200 8N1 |
| Temperature reads -50°C or 0°C | AHT10 communication failure | Check I2C pull-up resistors R1, R2 (4.7KΩ each) |
| | Sensor not powered | Verify AHT10 pin 4 (VDD) has BAT voltage |
| | I2C address wrong | Verify AHT10 has 7-bit address 0x38 |
| Humidity reads 0% or very low | Sensor damaged or saturated | Replace AHT10 sensor |
| | I2C bus stuck | Power-cycle the device (remove and reinsert battery) |
| Device resets repeatedly | IWDT timeout — firmware stuck | Check for infinite loops in I2C or TWI communication |
| | Brown-out (voltage <1.62V) | Replace battery |
| No I2C communication (SCL/SDA stuck low) | Bus locked by incomplete transaction | Power-cycle device; check pull-up resistors |
| RF frequency off by large margin | Crystal frequency mismatch (32 MHz vs 26 MHz) | Replace crystal with 26 MHz part, or adjust UM2005C PLL configuration |

---

## 10. Design Notes and Known Limitations

### 10.1 AHT10 Datasheet Not Available

The AHT10 I2C protocol parameters used in this firmware (init command 0xE1, trigger 0xAC 0x33 0x00, read 6 bytes) are based on industry-standard practice and the CW32L010 SDK example (measure.c). An official AHT10 datasheet should be obtained to verify these parameters before production.

### 10.2 Crystal Frequency Mismatch

The BOM specifies a **32 MHz crystal** (X322526MOB4SI), but the UM2005C application note specifies **26 MHz** for 433 MHz operation. This may cause:
- Carrier frequency offset (expected ±20% of crystal error)
- Possible failure to lock PLL if outside supported range

**Mitigation**: Test with a spectrum analyzer. If frequency is incorrect, either:
1. Replace crystal with 26 MHz part (recommended)
2. Configure UM2005C PLL registers for 32 MHz input

### 10.3 UART RX Command Interface

The UART RX command interface (Section 8.3) is an **optional feature** intended for development. The RX interrupt handler consumes additional power. For production units, compile with `CONFIG_CONSOLE_ENABLE=0`.

### 10.4 No Display or User Input

This product has no LCD, LED indicators (other than possible debug LEDs), or user input buttons. The only output is the 433 MHz RF transmission. The only input is the UART console (optional).

### 10.5 Antenna Design

The matching network values (L2=180nH, C10=6.8pF, L3=47nH, C11=2.7pF) are taken from the UM2005C application note for 433 MHz at +13 dBm output. At 0 dBm output power, the matching network may need adjustment for optimal efficiency. Antenna tuning should be verified with a network analyzer.

---

## 11. Reference Documents

| Document | File | Sections Used |
|----------|------|---------------|
| CW32L010 Datasheet | `inputs/datasheets/CW32L010_DataSheet_CN_V1.0.pdf` | Pin mapping (p23-24), power modes (p11), I2C (p19), UART (p19), electrical (p31-32) |
| UM2005C Datasheet | `inputs/datasheets/UM2005C 数据手册 V1.2_V1.2.pdf` | TWI protocol (p12), OTP map (p13), states (p14), timing (p6), matching network (p17) |
| System Architecture | `output/design/COMMON.md` | Software architecture, data flow, module definitions |
| Protocol Definitions | `output/design/PROTOCOLS.md` | Detailed protocol specifications (also at `output/docs/protocols.md`) |
| Hardware Interface Map | `output/docs/HW_INTERFACE.md` | Complete pinout, bus mapping, connectivity |
| Connectivity Analysis | `meta/connectivity.md` | Raw connectivity map from netlist |
| Datasheet Analysis | `meta/datasheets/DATASHEETS.md` | Cross-validation of connectivity vs. datasheets |
| Resource Allocation | `output/design/res_alloc.md` | Pin, peripheral, DMA, IRQ allocation |

---

## Document History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2025-07-17 | EmbedDevOps | Initial production manual based on Phase 2-4 artifacts |

---

*END OF USER MANUAL*
