# HW_INTERFACE.md — Hardware Interface Map

> **This document is directly derived from the hardware connectivity map** (`meta/connectivity.md`).
> Generated: Phase 4 — Product Manual Generation
> Source: connectivity.json (netlist analysis) + CW32L010 datasheet pin mapping + UM2005C datasheet

---

## 1. MCU Overview

- Model: **CW32L010Y8M6**
- Package: TSSOP-20
- RefDes: U7
- Core: ARM Cortex-M0+, up to 48 MHz
- Flash: 64 KB, RAM: 4 KB

## 2. Bus Connection Summary

### 2.1 I2C Bus

| Net | MCU Pin | MCU Port | Function | Connected To | Alt Function | Pull-up |
|-----|---------|----------|----------|-------------|-------------|---------|
| N05845 (I2C_SCL) | 14 | PA04 | I2C Clock (SCL) | AHT10 pin 2 | AF2 (I2C_SCL) | R1 (4.7KΩ → BAT) |
| N06024 (I2C_SDA) | 13 | PA03 | I2C Data (SDA) | AHT10 pin 3 | AF2 (I2C_SDA) | R2 (4.7KΩ → BAT) |

**Bus Kind**: I2C (hardware peripheral)
**Speed**: 100 kHz (standard mode)
**Device**: AHT10 temperature/humidity sensor (7-bit address 0x38)

### 2.2 UART Bus

| Net | MCU Pin | MCU Port | Function | Connected To | Alt Function |
|-----|---------|----------|----------|-------------|-------------|
| UART1_TXD | 16 | PA06 | UART Transmit | J3 pin 1 | AF1 (UART1_TXD) |
| UART1_RXD | 15 | PA05 | UART Receive | J3 pin 2 | AF1 (UART1_RXD) |

**Bus Kind**: UART (hardware peripheral)
**Speed**: 115200 baud, 8N1
**Connector**: J3 (CON3, 3-pin header: 1=TXD, 2=RXD, 3=GND)

### 2.3 GPIO / TWI Bus (UM2005C RF Interface)

| Net | MCU Pin | MCU Port | Function | Connected To | Interface |
|-----|---------|----------|----------|-------------|-----------|
| RF_CLK | 19 | PB02 | TWI Clock (bit-banged) | UM2005C pin 4 (CLK) | GPIO push-pull output |
| RF_DATA | 20 | PB03 | TWI Data (bit-banged, bidir) | UM2005C pin 3 (DATA) | GPIO push-pull/input |

**Bus Kind**: GPIO_IN_INTERRUPT (bit-banged TWI)
**Speed**: ≤1 MHz, falling-edge sample
**Device**: UM2005C RF transmitter (433 MHz FSK)

### 2.4 POWER Bus

| Net | MCU Pin | Connected To | Notes |
|-----|---------|-------------|-------|
| BAT | 9 (VDD) | CR2032 battery (J1), AHT10 pin 4, UM2005C pin 6 | 1.9–3.6V supply rail |
| GND | 7 (VSS) | Ground plane | Common ground |
| N12331 | 8 (Vcore) | C12 (100nF → GND) | Internal LDO output, decap only |

**Bus Kind**: POWER
**Source**: CR2032 coin cell (3V nominal)

### 2.5 RESET_CLOCK Bus (SWD Debug)

| Net | MCU Pin | MCU Port | Function | Connected To |
|-----|---------|----------|----------|-------------|
| SWCLK | 18 | PA08 | SWD Clock | J2 pin 2 |
| SWDIO | 17 | PA07 | SWD Data | J2 pin 3 |
| NRST | 4 | PB07 | Reset | J2 pin 5 |

**Connector**: J2 (HEADER 5, 5-pin: 1=BAT, 2=SWCLK, 3=SWDIO, 4=GND, 5=NRST)

---

## 3. Module-to-Bus Mapping

| software_module_hint | bus | device_ref | comm | notes |
|---|---|---|---|---|
| i2c_driver | I2C | U3 (AHT10) | I2C | Temperature/humidity sensor on I2C bus |
| uart_driver | UART | J3 | UART | Debug console via UART1 |
| rf_driver | GPIO_IN_INTERRUPT | U4 (UM2005C) | TWI (bit-banged) | RF transmitter, proprietary TWI protocol |
| power_manager | POWER | U7 (MCU) | N/A | Battery power management |
| debug_swd | RESET_CLOCK | J2 | SWD | Debug interface |

---

## 4. Component Pin Maps

### 4.1 AHT10 (U3) — Temperature/Humidity Sensor

| Pin# | Name | Connected To | Net Name |
|------|------|-------------|----------|
| 1 | GND | GND | GND |
| 2 | SCL | MCU PA04 (via R1 4.7K↑ to BAT) | N05845 |
| 3 | SDA | MCU PA03 (via R2 4.7K↑ to BAT) | N06024 |
| 4 | VDD | BAT rail | BAT |

### 4.2 UM2005C (U4) — RF Transmitter

| Pin# | Name | Type | Connected To | Net Name |
|------|------|------|-------------|----------|
| 1 | XTAL | AI | X322526MOB4SI (32 MHz crystal) | N09243 |
| 2 | GND | G | GND plane | GND |
| 3 | DATA | DIO | MCU PB03 | RF_DATA |
| 4 | CLK | DI | MCU PB02 | RF_CLK |
| 5 | RFO | RFO | Matching network → antenna | N09004 |
| 6 | VDD | P | BAT rail (1.9–3.6V) | BAT |

### 4.3 J2 — Debug Connector (HEADER 5)

| Pin | Signal | Connected To |
|-----|--------|-------------|
| 1 | BAT | VDD (MCU pin 9), battery |
| 2 | SWCLK | MCU PA08 |
| 3 | SWDIO | MCU PA07 |
| 4 | GND | Ground |
| 5 | NRST | MCU PB07 |

### 4.4 J3 — UART Connector (CON3)

| Pin | Signal | Connected To |
|-----|--------|-------------|
| 1 | UART1_TXD | MCU PA06 |
| 2 | UART1_RXD | MCU PA05 |
| 3 | GND | Ground |

---

## 5. Conflicts and Unresolved Items

| # | Item | Status | Resolution |
|---|------|--------|------------|
| 1 | RF_CLK/RF_DATA — SPI or GPIO? | ✅ **Resolved** | UM2005C uses proprietary TWI, not SPI. MCU PB02/PB03 used as bit-banged GPIO. |
| 2 | N12331 (MCU pin 8 = Vcore) | ✅ **Verified** | Internal LDO output; C12 (100nF) provides required decoupling. No firmware action. |
| 3 | N09243 (UM2005C pin 1 = XTAL) | ✅ **Verified** | Crystal oscillator input. BOM has 32 MHz crystal; UM2005C app note specifies 26 MHz for 433 MHz. **HW risk.** |
| 4 | N09004 (UM2005C pin 5 = RFO) | ✅ **Verified** | RF power amplifier output. Connected to matching network (L2=180nH, C10=6.8pF, L3=47nH, C11=2.7pF) then antenna. |
| 5 | AHT10 datasheet not found | ⚠️ **Open** | I2C protocol parameters assumed from industry practice. Verify with official datasheet before production. |

---

## 6. Pin Coverage Verification

All 9 functional I/O pins (non-power, non-NC) are allocated:

| Pin | Port | Function | Module |
|-----|------|----------|--------|
| 13 | PA03 | I2C_SDA | i2c_driver |
| 14 | PA04 | I2C_SCL | i2c_driver |
| 15 | PA05 | UART1_RXD | uart_console |
| 16 | PA06 | UART1_TXD | uart_console |
| 17 | PA07 | SWDIO | debug_swd |
| 18 | PA08 | SWCLK | debug_swd |
| 19 | PB02 | RF_CLK (TWI) | rf_twi_driver |
| 20 | PB03 | RF_DATA (TWI) | rf_twi_driver |
| 4 | PB07 | NRST | (system) |

✅ **100% pin coverage for active I/O pins.**
