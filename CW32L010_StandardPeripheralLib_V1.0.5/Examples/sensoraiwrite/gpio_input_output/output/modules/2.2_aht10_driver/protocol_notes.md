# AHT10 Protocol Notes

> Module: 2.2 aht10_driver
> MCU: CW32L010Y8M6 (TSSOP-20)
> Device: AHT10 Temperature/Humidity Sensor
> Bus: Bit-bang I2C master via i2c_drv (module 2.1) PA03(SDA)/PA04(SCL)
> Date: 2025-07-17

---

## 1. Device Information

| Parameter | Value | Source |
|-----------|-------|--------|
| Part number | AHT10 | [meta/connectivity.json#buses[I2C].devices] |
| Manufacturer | Aosong (Guangzhou) Electronics | Industry known |
| I2C address | **0x38** (7-bit) | Industry standard |
| Write address byte | 0x70 (0x38 << 1) | Derived from 7-bit address |
| Read address byte | 0x71 (0x38 << 1 \| 0x01) | Derived from 7-bit address |
| Supply voltage | 1.8 – 3.6 V | Industry standard |
| Our supply | BAT = 1.9 – 3.6 V (CR2032) | ✅ Compatible |
| Interface | I2C (standard mode, ≤ 400 kHz) | Per AHT10 typical spec |
| Measurement range | 0 – 100% RH, -40 to +85 °C | Industry standard |
| Accuracy (typical) | ±2% RH, ±0.3 °C | Industry standard |
| Resolution | 0.024% RH, 0.01 °C (20-bit) | Industry standard |
| Package | 4-pin SMD (no label) | Industry known |

> **⚠ Open Issue**: No official AHT10 datasheet available in `inputs/datasheets/`.
> All protocol details are from industry-standard / open-source documentation.
> Verify on real hardware before production deployment.

---

## 2. I2C Bus Configuration

| Parameter | Value | Source |
|-----------|-------|--------|
| Bus kind | Bit-bang I2C master (not HW peripheral) | [cw32l010_gpio.h#L352-L368] |
| SCL pin | PA04 (net N05845), MCU pin 14 | [meta/connectivity.json#buses[I2C]] |
| SDA pin | PA03 (net N06024), MCU pin 13 | [meta/connectivity.json#buses[I2C]] |
| Pull-up resistors | R1 = 4.7 kΩ (SCL), R2 = 4.7 kΩ (SDA) to BAT | [meta/connectivity.json] |
| Speed | ~100 kHz (standard mode) | Architecture decision |
| Bit order | MSB first | I2C specification |

### 2.1 Alternate Function Check

Per the CW32L010 GPIO header [cw32l010_gpio.h#L352-L368]:

**PA03 (pin 13) — SDA:**
| AF | Function | Has I2C? |
|----|----------|----------|
| AF0 | GPIO | ❌ |
| AF1 | UART2_TXD | ❌ |
| AF2 | LPTIM_CH1 | ❌ |
| AF3 | SPI1_MISO | ❌ |
| AF4 | BTIM1_ETR | ❌ |
| AF5 | IR_OUT | ❌ |
| AF6 | GTIM_CH4 | ❌ |
| AF7 | ATIM_CH3 | ❌ |

**PA04 (pin 14) — SCL:**
| AF | Function | Has I2C? |
|----|----------|----------|
| AF0 | GPIO | ❌ |
| AF1 | UART2_RXD | ❌ |
| AF2 | LPTIM_CH2 | ❌ |
| AF3 | SPI1_MOSI | ❌ |
| AF4 | MCO_OUT | ❌ |
| AF5 | VC2_OUT | ❌ |
| AF6 | GTIM1_CH3 | ❌ |
| AF7 | ATIM_CH1N | ❌ |

**Conclusion**: Neither PA03 nor PA04 supports a hardware I2C alternate function.
The TSSOP-20 package does not route I2C_SDA/I2C_SCL to these pins.
**Bit-bang I2C is required.**

> **Note**: The QFN20 package variant may have different AF mapping.
> Verify against the actual package used.

---

## 3. Communication Protocol

### 3.1 Power-Up Sequence

```
Apply VDD (1.8-3.6V)
  |
  v Wait at least 20 ms (VDD stable + sensor internal init)
  |
  v
Send INIT command 0xBE  (optional — some modules are pre-initialized)
  |
  v Wait 10-20 ms
  |
  v
Ready for measurements
```

### 3.2 Measurement Sequence (Standard)

```
STEP 1: TRIGGER
  Master: START -> 0x70 (SLA+W) -> 0xAC -> 0x33 -> 0x00 -> STOP
  Slave:  ACK -> ACK -> ACK -> ACK
  
STEP 2: WAIT
  Wait at least 75 milliseconds (do NOT communicate with sensor)
  Sensor internal state: measuring temperature and humidity
  
STEP 3: READ
  Master: START -> 0x71 (SLA+R) -> [read 6 bytes] -> STOP
  Slave:  ACK -> [6 data bytes]
  
  Master sends ACK after bytes 0-4, NACK after byte 5 (last)
```

### 3.3 Bus Timing Diagram (Conceptual)

```
TRIGGER:
  SDA:  \________/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \________...
        [START][0x70][ACK][0xAC][ACK][0x33][ACK][0x00][ACK][STOP]

WAIT >= 75 ms
  SDA:  ______________________...  (bus idle, both lines HIGH)

READ:
  SDA:  \________/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \...
        [START][0x71][ACK][B0]   [B1]   [B2]   [B3]   [B4]
                               [A]    [A]    [A]    [A]    [A]
        ... \_/ \____
           [B5][NACK][STOP]
```

---

## 4. Data Format

### 4.1 Raw Data — 6 Bytes After Trigger

| Byte | Name | Bits | Description |
|------|------|------|-------------|
| 0 | HUM_MSB | [7:0] | Humidity raw data[19:12] |
| 1 | HUM_LSB | [7:0] | Humidity raw data[11:4] |
| 2 | HUM_XLSB_TEMP_MSB | [7:4] | Humidity raw data[3:0] |
| | | [3:0] | Temperature raw data[19:16] |
| 3 | TEMP_LSB | [7:0] | Temperature raw data[15:8] |
| 4 | TEMP_XLSB | [7:0] | Temperature raw data[7:0] |
| 5 | STATUS | [7] | Busy flag: 1=measuring, 0=ready |
| | | [6:0] | Reserved / N/A |

### 4.2 Status Byte (Byte 5)

| Bit | Meaning |
|-----|---------|
| 7 | 1 = Device busy (measurement in progress), 0 = idle/ready |
| 6 | Reserved |
| 5:0 | Reserved (may indicate calibration status on some variants) |

### 4.3 Data Conversion Formulas

#### Humidity

```c
// Raw 20-bit humidity value
uint32_t raw_hum = (buf[0] << 12) | (buf[1] << 4) | (buf[2] >> 4);

// Convert to %RH (integer percentage with one decimal = %RH x10)
uint16_t humidity_x10 = (uint16_t)((raw_hum * 1000UL) / 1048576UL);
// Example: raw_hum = 672,400 → humidity_x10 = 641 → 64.1% RH
```

#### Temperature

```c
// Raw 20-bit temperature value
uint32_t raw_temp = ((uint32_t)(buf[2] & 0x0F) << 16) |
                    ((uint32_t)buf[3] << 8) |
                    (uint32_t)buf[4];

// Convert to degC (with one decimal = degC x10)
int16_t temperature_x10 = (int16_t)(((raw_temp * 2000UL) / 1048576UL) - 500);
// Example: raw_temp = 520,000 → temp_x10 = 491 → 49.1°C
```

#### Derivation

The AHT10 uses 20-bit ADC values. The conversion formulas are:

```
RH = (raw_hum / 2^20) * 100%
T  = (raw_temp / 2^20) * 200 - 50
```

Where 2^20 = 1,048,576. The x10 versions multiply by 10 for one decimal:

```
humidity_x10 = (raw_hum * 1000) / 1048576    // ×10 of %
temperature_x10 = (raw_temp * 2000) / 1048576 - 500  // ×10 of °C
```

### 4.4 Example Conversion

**Raw data**: `0x67 0x8C 0x8B 0x0B 0x85 0x00`

```
raw_hum  = (0x67 << 12) | (0x8C << 4) | (0x8B >> 4)
         = 0x678C8
         = 424,136
humidity_x10 = (424136 * 1000) / 1048576 = 404  → 40.4% RH

raw_temp = ((0x8B & 0x0F) << 16) | (0x0B << 8) | 0x85
         = 0xB0B85
         = 723,845
temp_x10 = (723845 * 2000) / 1048576 - 500 = 880  → 88.0°C
```

**Note**: 88.0°C is above typical room temperature. This is just a synthetic example;
actual readings at room temperature should be ~20-30°C and ~40-70% RH.

---

## 5. Command Reference

| Command | Byte Sequence | Description | Wait After |
|---------|--------------|-------------|------------|
| INIT | `0xBE` | Initialize sensor, load calibration | ≥20 ms |
| TRIGGER | `0xAC 0x33 0x00` | Start measurement | ≥75 ms |
| READ | `(read 6 bytes)` | Read measurement result | None |
| SOFT_RESET | `0xBA` | Soft reset sensor | ≥20 ms |

### 5.1 Important Notes

- **INIT (0xBE)**: Some AHT10 modules come pre-initialized from factory.
  Sending 0xBE is harmless but may not be required.
  The driver sends it during `aht10_init()` for compatibility.
  
- **TRIGGER (0xAC)**: The 0x33 and 0x00 arguments are essential.
  Some documentation shows 0xAC 0x33 0x00 is the standard trigger sequence.
  Do NOT send only 0xAC without the two argument bytes.

- **NO_DATA mode**: Some AHT10 variants support a "no-data" trigger where
  bytes 1-2 are omitted (just 0xAC). The standard 3-byte trigger is preferred.

- **CRC**: The basic AHT10 (not AHT21B) does NOT include CRC in the 6-byte
  response. CRC check is not implemented. If AHT21B is used, there is a 7th
  CRC byte.

---

## 6. Error Handling

| Condition | Detection | Handling |
|-----------|-----------|----------|
| Address NACK | I2C returns NACK on address byte | Return `AHT10_ERR_I2C`; caller may retry after delay |
| Data NACK | I2C returns NACK on data byte | Return `AHT10_ERR_I2C`; sensor may be busy |
| Sensor busy (bit 7=1) | Status byte bit 7 after read | Retry read every 5 ms up to 100 ms; return `AHT10_ERR_BUSY` if still busy |
| I2C timeout | SCL held low > 10 ms | Return `AHT10_ERR_I2C`; i2c_drv will do bus clear |
| Invalid parameter | NULL pointer | Return `AHT10_ERR_PARAM` |
| Driver not initialized | s_initialized == 0 | Return `AHT10_ERR_NOT_INIT` |

---

## 7. Comparison: AHT10 vs AHT20 vs AHT21B

| Feature | AHT10 | AHT20 | AHT21B |
|---------|-------|-------|--------|
| I2C address | 0x38 | 0x38 | 0x38 |
| Trigger command | 0xAC 0x33 0x00 | 0xAC 0x33 0x00 | 0xAC 0x33 0x00 |
| Read length | 6 bytes | 6 bytes | 7 bytes (incl. CRC) |
| CRC support | No | No | Yes (CRC8) |
| Accuracy | ±2%RH, ±0.3°C | ±2%RH, ±0.3°C | ±2%RH, ±0.3°C |
| Max speed | 400 kHz | 400 kHz | 400 kHz |

Our driver assumes **AHT10** (6 bytes, no CRC). If the installed device is AHT21B,
modify `AHT10_READ_LEN` to 7 and enable CRC checking.

---

## 8. Board-Level Integration Notes

### 8.1 PCB Traces

- Keep SCL (N05845) and SDA (N06024) traces short (< 5 cm) to reduce capacitance
- Route away from RF section (UM2005C and antenna matching network)
- Place decoupling cap (100 nF) near AHT10 VDD pin (pin 4)

### 8.2 Pull-Up Resistors

- R1 (SCL) = 4.7 kΩ to BAT rail
- R2 (SDA) = 4.7 kΩ to BAT rail
- These values are appropriate for ~100 kHz bus speed and ~100 pF bus capacitance

### 8.3 Power Supply

- AHT10 VDD (pin 4) is connected to BAT rail (shared with MCU and UM2005C)
- AHT10 VDD operating range: 1.8 – 3.6 V ✅
- BAT range: 1.9 – 3.6 V (CR2032) ✅
- No separate LDO needed for AHT10

---

## 9. Open Issues and Assumptions

| # | Issue | Type | Mitigation |
|---|-------|------|------------|
| 1 | No AHT10 datasheet in `inputs/datasheets/` | 🔴 Missing | Use industry-standard 0xAC trigger; verify on HW |
| 2 | Trigger command 0xAC 0x33 0x00 may vary by batch | 🟡 Assumption | Document in test report after HW verification |
| 3 | Sensor initialization (0xBE) may not be required | 🟡 Assumption | Driver sends it anyway; harmless if pre-initialized |
| 4 | Bus speed 100 kHz vs 400 kHz — AHT10 supports both | 🟡 Assumption | 100 kHz is safe and sufficient for 150s cycle |
| 5 | PA03/PA04 lack HW I2C AF on TSSOP-20 | ✅ Confirmed | Bit-bang works but uses CPU during transactions (~3 ms) |
| 6 | Sensor response time may exceed 75 ms | 🟡 Assumption | Driver polls status bit with 100 ms timeout |

---

## 10. Sources Table

| Symbol/Value | Type | Source |
|-------------|------|--------|
| AHT10_I2C_ADDR = 0x38 | wire_param | Industry standard (AHT10) |
| AHT10_CMD_TRIGGER = 0xAC | wire_param | Industry standard |
| AHT10_CMD_INIT = 0xBE | wire_param | Industry standard |
| AHT10_CMD_SOFT_RESET = 0xBA | wire_param | Industry standard |
| I2C SCL = PA04, pin 14 | pin | [meta/connectivity.json#buses[I2C]] |
| I2C SDA = PA03, pin 13 | pin | [meta/connectivity.json#buses[I2C]] |
| No HW I2C on PA03/PA04 | pin/af | [inputs/mcu_sdk/Libraries/inc/cw32l010_gpio.h#L352-L368] |
| i2c_drv_init() | fn | [output/modules/2.1_i2c_driver/interface.h#L67] |
| i2c_drv_master_write() | fn | [output/modules/2.1_i2c_driver/interface.h#L88] |
| i2c_drv_master_read() | fn | [output/modules/2.1_i2c_driver/interface.h#L101] |
| i2c_drv_probe() | fn | [output/modules/2.1_i2c_driver/interface.h#L132] |
| SysTickDelay() | fn | [inputs/mcu_sdk/Libraries/inc/cw32l010_systick.h#L65] |
| GetTick() | fn | [inputs/mcu_sdk/Libraries/inc/cw32l010_systick.h#L63] |
| Conversion formulas | wire_param | [output/modules/2.1_i2c_driver/protocol_notes.md#L149-L161] |
