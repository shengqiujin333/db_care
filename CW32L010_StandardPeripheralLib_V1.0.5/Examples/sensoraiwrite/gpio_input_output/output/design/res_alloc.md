# Resource Allocation Table — Phase 2.0 Common Design

> MCU: CW32L010Y8M6 (TSSOP-20)  
> Generated from: connectivity.json (nets, buses) + CW32L010 datasheet pin mapping  
> Date: 2025-07-17

---

## 1. GPIO Pin Allocation

| MCU Pin | Port | Net Name | Function | Alt Mode | Owner Module | Reserved By |
|---------|------|----------|----------|----------|-------------|-------------|
| 4 | PB07 | NRST | Reset input | Default (NRST) | (system) | connectivity |
| 7 | VSS | GND | Ground | — | (power) | connectivity |
| 8 | Vcore | N12331 | Regulator output (decap C12) | — | (power) | connectivity |
| 9 | VDD | BAT | Supply voltage (CR2032) | — | power_manager | connectivity |
| 13 | PA03 | N06024 (I2C_SDA) | I2C Data | AF2 (I2C_SDA) | i2c_driver | connectivity |
| 14 | PA04 | N05845 (I2C_SCL) | I2C Clock | AF2 (I2C_SCL) | i2c_driver | connectivity |
| 15 | PA05 | UART1_RXD | UART Receive | AF1 (UART1_RXD) | uart_console | connectivity |
| 16 | PA06 | UART1_TXD | UART Transmit | AF1 (UART1_TXD) | uart_console | connectivity |
| 17 | PA07 | SWDIO | SWD Data I/O | Default (SWDIO) | debug_swd | connectivity |
| 18 | PA08 | SWCLK | SWD Clock | Default (SWCLK) | debug_swd | connectivity |
| 19 | PB02 | RF_CLK | TWI Clock (bit-bang GPIO PP output) | GPIO | rf_twi_driver | connectivity |
| 20 | PB03 | RF_DATA | TWI Data (bit-bang GPIO bidir) | GPIO | rf_twi_driver | connectivity |

### NC Pins (available for future use)

| MCU Pin | Port | Note |
|---------|------|------|
| 1 | PA00 | NC — available |
| 2 | PA01 | NC — available |
| 3 | PA02 | NC — available (but shares UART1_CTS on AF1, LPTIM_CH1 on alternate) |
| 5 | PB00 | NC — available |
| 6 | PB01 | NC — available |
| 10 | PA09 | NC — available |
| 11 | PA10 | NC — available |
| 12 | PA11 | NC — available |

---

## 2. Peripheral Instance Allocation

| Peripheral | Instance | Owner Module | Usage | Consumed By Connectivity? |
|-----------|----------|-------------|-------|--------------------------|
| I2C | I2C1 | i2c_driver | Master @ 100kHz, polling, PA03(SDA)/PA04(SCL) | ✅ (N05845, N06024) |
| UART | UART1 | uart_console | 115200 8N1, PA05(RXD)/PA06(TXD) | ✅ (UART1_RXD, UART1_TXD) |
| RTC | RTC | power_manager | Periodic alarm every ~150s, LSI driven | No (power management, not in nets) |
| IWDT | IWDT | power_manager | Safety watchdog, ~2s timeout | No |
| GPIO Port A | GPIOA | Shared (i2c, uart, debug) | PA00-PA11 (pins 1-3, 10-18) | ✅ |
| GPIO Port B | GPIOB | rf_twi_driver | PB02, PB03 (pins 19, 20) — bit-bang TWI | ✅ (RF_CLK, RF_DATA) |

---

## 3. Timer Allocation

| Timer | Owner | Usage | Status | Notes |
|-------|-------|-------|--------|-------|
| ATIM | _free_ | — | **Available** | Advanced timer, 16-bit |
| GTIM | _free_ | — | **Available** | General-purpose timer, 16-bit |
| LPTIM | power_manager | Low-power timeout for wakeup sequencing | Proposed | Low-power timer, stays on in DeepSleep |
| BTIM1 | _free_ | — | **Available** | Basic timer |
| BTIM2 | _free_ | — | **Available** | Basic timer |
| BTIM3 | _free_ | — | **Available** | Basic timer |
| SysTick | system | 1ms system tick | Reserved | ARM SysTick, used for delay_ms() and timestamp |

> **All timers except SysTick are currently free.** Modules may <RES claim/> them for their own use. The power_manager may claim LPTIM for low-power timing. No module should claim a timer without explicit conflict check.

---

## 4. Interrupt Allocation

| IRQ | Owner | Priority | Notes | Consumed By Connectivity? |
|-----|-------|----------|-------|--------------------------|
| RTC_IRQn | power_manager | Lowest (3) | Periodic wakeup from DeepSleep | No |
| UART1_IRQn | uart_console | Low (2) | RX character reception | Optional (RX interrupt) |
| I2C1_IRQn | _free_ | — | Not used (polling mode) | No |
| SysTick_IRQn | system | Low (2) | 1ms system tick | No |
| PVD_IRQn | _free_ | — | **Available** for voltage monitoring | No |

> **IRQ priority scheme**: 0=highest, 3=lowest (Cortex-M0+ has 4 priority levels, 2 bits).

---

## 5. DMA Allocation

| DMA Channel | Owner | Usage | Status |
|-------------|-------|-------|--------|
| DMA1_CH1 | _free_ | — | **Available** |
| DMA1_CH2 | _free_ | — | **Available** |
| DMA1_CH3 | _free_ | — | **Available** |
| DMA1_CH4 | _free_ | — | **Available** |
| DMA1_CH5 | _free_ | — | **Available** |
| DMA1_CH6 | _free_ | — | **Available** |
| DMA1_CH7 | _free_ | — | **Available** |

> **All 7 DMA channels are free.** No module currently requires DMA (I2C polling, UART polling TX, bit-bang TWI). Modules may claim DMA via <RES claim/>.

---

## 6. Resource Allocation Summary

| Resource Type | Total | Used | Free |
|--------------|-------|------|------|
| GPIO pins (non-power, non-NC) | 9 (real I/O) | 9 (100%) | 0 |
| GPIO pins (NC, expandable) | 8 | 0 | 8 |
| I2C | 1 | 1 | 0 |
| UART | 2 | 1 | 1 (UART2 free) |
| SPI | 1 | 0 | 1 |
| ADC (12-bit) | 1 | 0 | 1 |
| ATIM | 1 | 0 | 1 |
| GTIM | 1 | 0 | 1 |
| LPTIM | 1 | 0 (proposed) | 1 |
| BTIM | 3 | 0 | 3 |
| DMA channels | 7 | 0 | 7 |
| EXTI (wakeup) | 16 | 0 | 16 |

---

## 7. Claim Procedure

Modules must call `<RES action="claim" resource="..." owner="..." />` before using any resource not already consumed by connectivity. Conflicts cause escalation.

**Pre-claimed resources (from connectivity, no further claim needed):**
- All GPIO pins in Section 1 table
- I2C1, UART1
- RTC, IWDT (implicit system use)
