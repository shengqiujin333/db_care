# COMMON.md — Phase 2.0 Software Architecture & Common Interfaces

> MCU: CW32L010Y8M6 (TSSOP-20)  
> Project: Battery-powered temperature/humidity sensor with 433 MHz FSK RF transmission  
> Power: CR2032 coin cell (3V nominal)  
> Source: inputs/user_req.txt (User Requirements)

---

## 1. System Architecture Overview

```
┌────────────────────────────────────────────────────────────┐
│                     APPLICATION LAYER                       │
│  ┌──────────────┐  ┌──────────────────┐  ┌──────────────┐  │
│  │ schedule     │  │ sensor_mgr      │  │ rf_mgr       │  │
│  │ (RTC-trigger)│  │ (2-sample avg)  │  │ (TX + sleep) │  │
│  └──────┬───────┘  └────────┬─────────┘  └──────┬───────┘  │
│         │                   │                     │         │
├─────────┼───────────────────┼─────────────────────┼─────────┤
│         │     SERVICE LAYER │                     │         │
│  ┌──────┴───────┐  ┌───────┴────────┐  ┌─────────┴──────┐ │
│  │ power_mgr    │  │ data_logger   │  │ packet_builder │ │
│  │ (DeepSleep)  │  │ (print ctrl)  │  │ (protocol)     │ │
│  └──────┬───────┘  └────────────────┘  └────────┬───────┘ │
│         │                                        │         │
├─────────┼────────────────────────────────────────┼─────────┤
│         │               DRIVER LAYER              │         │
│  ┌──────┴───────┐  ┌──────────────┐  ┌───────────┴───────┐ │
│  │ i2c_driver   │  │ uart_console│  │ rf_twi_driver     │ │
│  │ (I2C1)       │  │ (UART1)     │  │ (PB02,PB03 TWI)   │ │
│  └──────┬───────┘  └──────┬───────┘  └──────────────────┘ │
│         │                 │                                │
├─────────┼─────────────────┼────────────────────────────────┤
│         │      HAL LAYER  │                                │
│  ┌──────┴───────┐  ┌─────┴──────┐  ┌────────────────────┐ │
│  │ CW32L010 I2C │  │ CW32L010  │  │ GPIO (PB02,PB03)   │ │
│  │ Registers    │  │ UART1 Reg │  │ Bit-bang TWI       │ │
│  │ PA03(SCL)    │  │ PA05(RXD) │  │                    │ │
│  │ PA04(SDA)    │  │ PA06(TXD) │  │                    │ │
│  └──────────────┘  └───────────┘  └────────────────────┘  │
└────────────────────────────────────────────────────────────┘
         │                    │                     │
    ┌────┴────┐          ┌───┴───┐          ┌──────┴──────┐
    │ AHT10  │          │  J3   │          │ UM2005C    │
    │ Sensor │          │ UART  │          │ RF TX      │
    │ I2C    │          │ 3-pin │          │ 433MHz FSK │
    └────────┘          └───────┘          └─────────────┘
```

### 1.1 Architecture Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| RTOS vs Bare-metal | **Bare-metal super-loop + RTC-triggered** | 4KB RAM insufficient for FreeRTOS; simple periodic workflow: read→avg→TX→sleep |
| Scheduling | RTC alarm wakes MCU every 150s from DeepSleep | Per user requirement: "定时150s进行一次" + "其余时刻低功耗" |
| I2C for AHT10 | Hardware I2C peripheral (AF2 on PA03/PA04) | CW32L010 has dedicated I2C; no bit-bang needed |
| RF interface | Bit-banged GPIO TWI on PB02/PB03 | UM2005C uses proprietary TWI; no HW TWI peripheral |
| RF modulation | FSK (per user requirement "fsk") | User req #2: "使用433mhz，fsk" |
| Console UART | UART1 on PA05/PA06, 115200 8N1, TX polling | Print toggleable at compile time per user req #1 |
| Sampling strategy | 2 samples → average → TX | User req #2: "采集两次温湿度后求平均值" |
| Deep Sleep | DeepSleep between cycles, RTC alarm wake | User req #3: "其余时刻，mcu进入低功耗状态" |
| Power TX power | 0 dBm | User note in connectivity.json: "发射功率0DB" |
| MCU UID | Lower 48 bits of 96-bit UID | User req #2: "输出温湿度和mcuid" |

### 1.2 Execution Flow (150s cycle)

```
RTC Alarm Wake
  ↓
[DeepSleep exit] → HSIOSC startup (~4μs)
  ↓
SAMPLE #1 (t=0s):
  - I2C start → 0xAC 0x33 0x00 trigger → wait 75ms → read 6 bytes → store
  ↓
Wait ~100ms (short delay)
  ↓
SAMPLE #2 (t=~0.2s):
  - I2C trigger → wait 75ms → read 6 bytes → store
  ↓
COMPUTE AVERAGE:
  - temp_avg = (sample1.temp + sample2.temp) / 2
  - hum_avg  = (sample1.hum + sample2.hum) / 2
  ↓
RF TRANSMIT:
  - Build packet: preamble + UID + temp_avg + hum_avg + status + CRC8
  - TWI_ON (32 CLK zeros) → write config → write 14B payload → TWI_OFF
  ↓
CONSOLE PRINT (if enabled at compile time):
  - UART TX: "[T=25.4C H=62.3% TX=OK]\n"
  ↓
DEEP SLEEP:
  - Configure RTC alarm for +150s
  - Enter DeepSleep (HSIOSC off, LSI+RTC active)
  ↓
[wait ~150s...]
```

**Active time per cycle**: ~200ms (2 × 75ms AHT10 wait + ~50ms computation/TWI)
**DeepSleep power**: ~1-2μA (RTC + LSI + RAM retention)
**Average current**: ~3-5μA → CR2032 (225mAh) → **~5-8 years estimated life**

---

## 2. Module Table (Machine-Readable)

```json
[
  {
    "id": "2.1",
    "name": "i2c_driver",
    "layer": "driver",
    "depends_on_bus": "I2C",
    "comm_summary": "I2C master @ 100kHz (standard mode), 7-bit addr, polling mode, for AHT10",
    "mcu_resources": ["I2C1", "PA03", "PA04", "GPIOA_AFR2"],
    "status": "planned"
  },
  {
    "id": "2.2",
    "name": "aht10_driver",
    "layer": "driver",
    "depends_on_bus": "I2C",
    "comm_summary": "AHT10 temp/humidity sensor, I2C addr 0x38, trigger 0xAC 0x33 0x00, read 6 bytes, polling",
    "mcu_resources": ["I2C1"],
    "status": "planned"
  },
  {
    "id": "2.3",
    "name": "uart_console",
    "layer": "driver",
    "depends_on_bus": "UART",
    "comm_summary": "UART1 @ 115200 8N1, TX polling only, compile-time toggle for power saving",
    "mcu_resources": ["UART1", "PA05", "PA06", "GPIOA_AFR1"],
    "status": "planned"
  },
  {
    "id": "2.4",
    "name": "rf_twi_driver",
    "layer": "driver",
    "depends_on_bus": "GPIO_IN_INTERRUPT",
    "comm_summary": "Bit-banged TWI master on PB02(CLK)/PB03(DATA), CLK ≤ 1MHz falling-edge, 16-cycle R/W transactions for UM2005C",
    "mcu_resources": ["PB02", "PB03"],
    "status": "planned"
  },
  {
    "id": "2.5",
    "name": "power_manager",
    "layer": "service",
    "depends_on_bus": "POWER",
    "comm_summary": "CR2032 power management: RTC periodic wakeup every 150s, DeepSleep, IWDT safety watchdog",
    "mcu_resources": ["RTC", "IWDT", "RTC_IRQn"],
    "status": "planned"
  },
  {
    "id": "2.6",
    "name": "packet_builder",
    "layer": "service",
    "depends_on_bus": "GPIO_IN_INTERRUPT",
    "comm_summary": "RF packet assembly: [0xAA][0x55][UID6][temp2][hum2][status][CRC8], 14 bytes total",
    "mcu_resources": [],
    "status": "planned"
  },
  {
    "id": "2.7",
    "name": "sensor_mgr",
    "layer": "application",
    "depends_on_bus": "I2C",
    "comm_summary": "Orchestrates 2-sample averaging: read AHT10 twice → average temp/humidity → pass to packet_builder + console",
    "mcu_resources": [],
    "status": "planned"
  },
  {
    "id": "2.8",
    "name": "schedule",
    "layer": "application",
    "depends_on_bus": "POWER",
    "comm_summary": "Main scheduler: RTC alarm → wake → sensor_mgr.run() → rf_mgr.run() → DeepSleep",
    "mcu_resources": ["RTC"],
    "status": "planned"
  }
]
```

### 2.1 Bus Cross-Reference

| Module ID | depends_on_bus | Bus in connectivity.buses[]? | Verified |
|-----------|---------------|------------------------------|----------|
| 2.1, 2.2, 2.7 | I2C | ✅ `kind: "I2C"` nets: N05845, N06024 | ✅ |
| 2.3 | UART | ✅ `kind: "UART"` nets: UART1_TXD, UART1_RXD | ✅ |
| 2.4, 2.6 | GPIO_IN_INTERRUPT | ✅ `kind: "GPIO_IN_INTERRUPT"` nets: RF_CLK, RF_DATA | ✅ |
| 2.5, 2.8 | POWER | ✅ `kind: "POWER"` nets: BAT, GND, N12331 | ✅ |

### 2.2 MCU Pin Coverage

| Net | MCU Pin | Port | Function | Module(s) | AF | Source |
|-----|---------|------|----------|-----------|----|--------|
| N05845 (I2C_SCL) | 14 | PA04 | I2C Clock | i2c_driver | AF2 | [CW32L010_DataSheet_CN_V1.0.pdf#page=24] |
| N06024 (I2C_SDA) | 13 | PA03 | I2C Data | i2c_driver | AF2 | [CW32L010_DataSheet_CN_V1.0.pdf#page=24] |
| UART1_RXD | 15 | PA05 | UART Receive | uart_console | AF1 | [CW32L010_DataSheet_CN_V1.0.pdf#page=24] |
| UART1_TXD | 16 | PA06 | UART Transmit | uart_console | AF1 | [CW32L010_DataSheet_CN_V1.0.pdf#page=24] |
| RF_CLK | 19 | PB02 | TWI CLK | rf_twi_driver | GPIO | [CW32L010_DataSheet_CN_V1.0.pdf#page=24] |
| RF_DATA | 20 | PB03 | TWI DATA | rf_twi_driver | GPIO | [CW32L010_DataSheet_CN_V1.0.pdf#page=24] |
| SWDIO | 17 | PA07 | SWD Data | debug | SWDIO | [CW32L010_DataSheet_CN_V1.0.pdf#page=23] |
| SWCLK | 18 | PA08 | SWD Clock | debug | SWCLK | [CW32L010_DataSheet_CN_V1.0.pdf#page=23] |
| NRST | 4 | PB07 | Reset | system | NRST | [CW32L010_DataSheet_CN_V1.0.pdf#page=23] |
| VDD | 9 | VDD | Power | power_manager | — | [CW32L010_DataSheet_CN_V1.0.pdf#page=23] |
| Vcore | 8 | Vcore | Regulator | (passive) | — | [CW32L010_DataSheet_CN_V1.0.pdf#page=23] |

**Pin coverage check**: All 9 functional I/O pins (non-power, non-NC) are covered by active modules. ✅

---

## 3. Task Table (Bare-metal, RTC-Driven)

| Task ID | Name | Trigger | Stack (B) | Period | Description |
|---------|------|---------|-----------|--------|-------------|
| T1 | schedule_init | Boot | 256 | Once | Clock, GPIO, I2C, UART, TWI GPIO init, IWDT start, RTC config |
| T2 | sample_aht10 | Called by T4 | 128 | 150s | I2C trigger AHT10 → wait 75ms → read 6 bytes |
| T3 | compute_avg | After 2 samples | 64 | 150s | Average temp/humidity from 2 samples |
| T4 | rf_transmit | After avg | 256 | 150s | TWI_ON → write payload → TWI_OFF → sleep UM2005C |
| T5 | console_print | After avg | 128 | 150s | UART print (compile-time toggleable) |
| T6 | deep_sleep | End of cycle | 64 | 150s | Set RTC alarm +150s → enter DeepSleep |

---

## 4. Data Flow Diagram

```
                    ┌─────────────────────┐
                    │     AHT10 Sensor    │
                    │     (I2C, 0x38)     │
                    └──────────┬──────────┘
                    ┌──────────┴──────────┐
                    │   Sample #1 (t=0)   │
                    │   Sample #2 (+100ms)│
                    └──────────┬──────────┘
                               │
                               ▼
                    ┌─────────────────────┐
                    │   sensor_mgr        │
                    │   temp_avg = (s1+s2)/2 │
                    │   hum_avg  = (s1+s2)/2 │
                    └──────────┬──────────┘
                    ┌──────────┴──────────┐
                    │                     │
                    ▼                     ▼
        ┌───────────────────┐   ┌─────────────────────────┐
        │  console_print    │   │  packet_builder         │
        │  (if ENABLED)     │   │  [0xAA][0x55][UID6]    │
        │  UART1 TX         │   │  [temp2][hum2][sts][CRC]│
        └───────────────────┘   └───────────┬─────────────┘
                                            │ TWI bit-bang
                                            ▼
                              ┌────────────────────────────┐
                              │     UM2005C RF TX           │
                              │     433 MHz, FSK, 0 dBm    │
                              └────────────────────────────┘
```

---

## 5. Common Types & Error Codes

See `output/design/common_types.h` for full definitions.

**Summary:**
- `error_code_t` enum: ERR_OK(0), ERR_BUSY(-1), ERR_TIMEOUT(-2), ERR_INVALID_PARAM(-3), ERR_NACK(-4), ERR_CRC(-5), ERR_READ_FAIL(-6), ERR_WRITE_FAIL(-7), ERR_NOT_INIT(-8), ERR_BUS(-9)
- `sensor_data_t`: {int16_t temperature_x10, uint16_t humidity_x10, uint8_t status, uint32_t timestamp_ms}
- `rf_packet_t`: 14-byte packed struct with preamble, UID, temp, humidity, status, CRC8
- Log macros: LOG_ERROR, LOG_WARN, LOG_INFO, LOG_DEBUG (compile-time toggle via LOG_LEVEL)

### 5.1 Compile-Time Print Toggle

Per user req #1: "打印做成灵活的，可以随时关闭后编译，为了省电"

```c
// In board.h or build config:
#define CONFIG_CONSOLE_ENABLE   1   // Set to 0 to disable all UART output

#if CONFIG_CONSOLE_ENABLE
    #define PRINT(fmt, ...)  uart_printf(fmt, ##__VA_ARGS__)
#else
    #define PRINT(fmt, ...)  ((void)0)
#endif
```

When `CONFIG_CONSOLE_ENABLE = 0`:
- UART1 peripheral clock can be disabled
- PA05/PA06 pins can be configured as GPIO input (high-Z) for even lower leakage
- No UART TX current consumption (~3-5mA when actively transmitting)

---

## 6. Connectivity Conflict Resolution

### 6.1 Conflict: RF_CLK/RF_DATA — SPI or GPIO?

**connectivity.json claim**: "Net names suggest SPI but no other SPI signals (CS, MISO, MOSI) present"

**Resolution (from UM2005C datasheet)**: The UM2005C uses a proprietary **TWI (Two-Wire Interface)** protocol, not SPI. Pins 3 (DATA) and 4 (CLK) form the TWI bus. MCU uses **bit-banged GPIO** on PB02/PB03. ✅ **Resolved.**

### 6.2 Unresolved Net: N12331 (MCU Pin 8 = Vcore)

**Datasheet verification**: MCU pin 8 = Vcore (internal LDO output, requires 100nF–470nF decap). C12 (100nF) present. **No firmware action required.** ✅

### 6.3 Crystal Mismatch: 32 MHz vs 26 MHz

BOM lists X322526MOB4SI (32 MHz) but UM2005C app note specifies 26 MHz for 433 MHz. Firmware must configure UM2005C PLL for the installed crystal frequency. **Documented as HW risk.**

---

## 7. HMI Architecture

No LCD/serial-screen UI assets found. User-facing output is:
- **UART console** (J3, 3-pin connector) for debug logging (compile-time toggle)
- **RF transmission** (433 MHz FSK) to remote receiver

Firmware responsibilities:
- TX: Sensor data log after each measurement (if CONFIG_CONSOLE_ENABLE=1)
- No display rendering or page management required

---

## 8. Open Issues

| # | Issue | Impact | Mitigation |
|---|-------|--------|------------|
| 1 | AHT10 datasheet missing | I2C command sequence unconfirmed | Use 0xAC/0x33/0x00 trigger (industry standard); verify on real HW |
| 2 | 32 MHz vs 26 MHz crystal | Carrier frequency may be off | Configure UM2005C PLL for actual crystal; test with spectrum analyzer |
| 3 | CR2032 RF TX current | UM2005C TX ~17mA @ 0dBm | 20ms TX × 150s cycle → negligible avg current |
