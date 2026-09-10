# I2C Driver Protocol Notes

> Module: 2.1 i2c_driver
> MCU: CW32L010Y8M6 (TSSOP-20)
> Date: 2025-07-17
> Device: AHT10 Temperature/Humidity Sensor (addr 0x38)

---

## 1. I2C Bus Configuration

| Parameter | Value | Source |
|-----------|-------|--------|
| Bus kind | Bit-bang I2C master (not HW peripheral) | [cw32l010_gpio.h#L352-L368] |
| SCL pin | PA04 (net N05845), pin 14 | [meta/connectivity.json#components[0].pins[14]] |
| SDA pin | PA03 (net N06024), pin 13 | [meta/connectivity.json#components[0].pins[13]] |
| Pull-up resistors | 4.7 kOhm to BAT rail (R1=R2=4.7k) | [meta/connectivity.json#components R1,R2] |
| Speed | ~100 kHz (Standard-mode) | [comm_summary] |
| Bit order | MSB first | [UM10204 I2C spec] |

> **Important**: PA03 and PA04 on the TSSOP-20 package do NOT have hardware I2C
> alternate function. Per cw32l010_gpio.h#L352-L368:
>   - PA03 AF0-7 = GPIO, UART2_TXD, LPTIM_CH1, SPI1_MISO, BTIM1_ETR, IR_OUT, GTIM_CH4, ATIM_CH3
>   - PA04 AF0-7 = GPIO, UART2_RXD, LPTIM_CH2, SPI1_MOSI, MCO_OUT, VC2_OUT, GTIM1_CH3, ATIM_CH1N
> Neither pin supports I2C_SDA or I2C_SCL. Bit-bang is required.

---

## 2. Pin Driving Strategy

Since CW32L010 GPIO supports open-drain mode via the OPENDRAIN register:

| State | SCL/SDA Configuration |
|-------|----------------------|
| Drive LOW | Output push-pull with OPENDRAIN=1, write 0 via BRR (BSRR[pin+16]) |
| Release HIGH | Write 1 via BSR (BSRR[pin]) -- Hi-Z, external pull-up pulls HIGH |
| Read level | Read IDR register |

This strategy ensures:
- No driver conflict (only master drives lines low)
- External 4.7 kOhm resistors pull lines HIGH when released
- Slave devices can stretch SCL (clock stretching supported)

---

## 3. I2C Bus Timing (@ 48 MHz PCLK)

```
SCL period:        10 us  (100 kHz)
SCL half-period:   5 us
SDA setup time:    1 us  (>= 250 ns per spec)
SDA hold time:     1 us  (>= 0 ns per spec)
Timeout:           10 ms (slave clock stretching)
```

### 3.1 START Condition

```
SCL:  __________\___...
SDA:  ______\___...
       ^^^^ SCL HIGH
            ^ SDA falls while SCL HIGH
```

### 3.2 STOP Condition

```
SCL:  ...___/__________
SDA:  ...___/__________
            ^ SCL goes HIGH first
              ^ SDA rises while SCL HIGH
```

### 3.3 Byte Transfer

Each byte (8 bits + 1 ACK):
```
SCL:  _/_\_/_\_/_\_/_\_/_\_/_\_/_\_/_\_/_\_
SDA:  [D7][D6][D5][D4][D3][D2][D1][D0][ACK]
       ^^^ data changed while SCL LOW
           ^^^ data sampled while SCL HIGH
                                          ^^^ SDA released by master
                                              slave drives LOW=ACK
```

---

## 4. AHT10 Protocol

### 4.1 Device Information

| Parameter | Value |
|-----------|-------|
| Part number | AHT10 |
| I2C address | **0x38** (7-bit) |
| Write address byte | 0x70 (0x38 << 1) |
| Read address byte | 0x71 (0x38 << 1 | 0x01) |
| Supply voltage | 1.8-3.6 V (BAT = 1.9-3.6 V ✅) |
| Measurement range | 0-100% RH, -40 to +85 degC |
| Accuracy | +/-2% RH, +/-0.3 degC (typical) |

### 4.2 Measurement Sequence

```
POWER-UP (wait >=20 ms)
    |
    v
SEND INIT (0xBE or 0xE1)  --- optional; sensor may be pre-initialized
    |
    v wait >=20 ms
    |
    v
SEND TRIGGER (0xAC 0x33 0x00)
    |
    v wait >=75 ms (do NOT send STOP early -- sensor measures)
    |
    v
READ 6 BYTES:
    Byte 0: [hum_msb[7:0]]
    Byte 1: [hum_lsb[7:0]]
    Byte 2: [hum_xlsb[7:4] | temp_msb[3:0]]
    Byte 3: [temp_msb[7:4] | temp_lsb[3:0]]
    Byte 4: [temp_lsb[7:0]]
    Byte 5: [temp_xlsb[7:0]]  (bit 7 = 1: busy, 0: ready)
```

> **Note**: The 3-byte trigger command 0xAC 0x33 0x00 is the industry-standard
> AHT10 measurement trigger. The AHT10 datasheet was not available locally;
> these values come from common AHT10 implementations found in open-source
> projects. Verify on target hardware.

### 4.3 AHT10 Response Format (6 bytes)

```
Byte 0:  Humidity high byte    [HUM[19:12]]
Byte 1:  Humidity low byte     [HUM[11:4]]
Byte 2:  [HUM[3:0]] [TEMP[19:16]]
Byte 3:  Temperature high byte [TEMP[15:8]]
Byte 4:  Temperature low byte  [TEMP[7:0]]
Byte 5:  Reserved/Status
```

**Status byte** (Byte 5, bit 7):
- Bit 7 = 1: Device is busy (measuring)
- Bit 7 = 0: Device is idle, data ready

### 4.4 Data Conversion Formulas

```c
// Humidity: 20-bit raw value -> %RH x 10 (one decimal)
uint32_t raw_hum = ((uint32_t)buf[0] << 12) |
                   ((uint32_t)buf[1] << 4)  |
                   ((uint32_t)buf[2] >> 4);
int16_t humidity_x10 = (int16_t)((raw_hum * 1000.0 / 1048576.0));

// Temperature: 20-bit raw value -> degC x 10 (one decimal)
uint32_t raw_temp = ((uint32_t)(buf[2] & 0x0F) << 16) |
                    ((uint32_t)buf[3] << 8)            |
                    (uint32_t)buf[4];
int16_t temperature_x10 = (int16_t)(((raw_temp * 2000.0 / 1048576.0) - 500));
```

### 4.5 Command Reference

| Command | Byte Sequence | Description |
|---------|--------------|-------------|
| Init | 0xBE | Initialize sensor (sets calibration coefficients) |
| Trigger | 0xAC 0x33 0x00 | Start measurement (wait >=75 ms before read) |
| Soft Reset | 0xBA | Reset sensor (wait >=20 ms) |
| Read | -- | Read 6 bytes after trigger |

> **Source**: Industry-standard AHT10 protocol (no local datasheet available).
> Verify against official Aosong AHT10 datasheet.

---

## 5. Error Handling

| Condition | Behavior |
|-----------|----------|
| NACK on address | Return I2C_DRV_ERR_NACK, generate STOP |
| NACK on data | Return I2C_DRV_ERR_NACK, generate STOP |
| SCL held low >10 ms | Return I2C_DRV_ERR_TIMEOUT, call bus_clear |
| Bus busy (unexpected) | Call bus_clear (9 pulses + STOP), retry |
| Driver not initialized | Return I2C_DRV_ERR_BUSY |

---

## 6. Bus Clear Sequence

If a slave hangs the bus by holding SDA low:

```
FOR pulse = 1..9:
    Drive SCL HIGH -> wait
    If SDA is HIGH (released): STOP and exit
    Drive SCL LOW -> wait
Send STOP condition
```

This recovers most stuck I2C bus states without power cycling.

---

## 7. Open Issues

| # | Issue | Impact | Mitigation |
|---|-------|--------|------------|
| 1 | No AHT10 datasheet locally | Trigger command 0xAC 0x33 0x00 is industry-standard but unconfirmed | Verify on real HW; adjust if needed |
| 2 | PA03/PA04 lack HW I2C AF | Bit-bang uses CPU cycles (~200 us per 6-byte transaction) | Acceptable for 150s cycle; CPU idle otherwise |
| 3 | Timing via delay loops | Calibrated for 48 MHz HCLK; may shift with clock changes | Use SysTickDelay for >=1 ms; adjust loop constant if HCLK changes |

---

## 8. Sources Table

| Symbol/Value | Type | Source |
|-------------|------|--------|
| I2C_SCL_PIN = GPIO_PIN_4 | pin | meta/connectivity.json#components[0].pins[14] |
| I2C_SDA_PIN = GPIO_PIN_3 | pin | meta/connectivity.json#components[0].pins[13] |
| I2C_SCL_PORT = CW_GPIOA | reg | meta/connectivity.json + cw32l010.h |
| I2C_SDA_PORT = CW_GPIOA | reg | meta/connectivity.json + cw32l010.h |
| GPIO_MODE_OUTPUT_PP | fn | inputs/mcu_sdk/Libraries/inc/cw32l010_gpio.h#L55 |
| GPIO_Init() | fn | inputs/mcu_sdk/Libraries/inc/cw32l010_gpio.h#L481 |
| __SYSCTRL_GPIOA_CLK_ENABLE() | macro | inputs/mcu_sdk/Libraries/inc/cw32l010_sysctrl.h#L352 |
| SysTickDelay() | fn | inputs/mcu_sdk/Libraries/inc/cw32l010_systick.h#L65 |
| I2C slave addr 0x38 | wire_param | Industry standard (AHT10) |
| Trigger sequence 0xAC 0x33 0x00 | wire_param | Industry standard (AHT10) |
| No HW I2C on PA03/PA04 | pin | inputs/mcu_sdk/Libraries/inc/cw32l010_gpio.h#L352-L368 |
