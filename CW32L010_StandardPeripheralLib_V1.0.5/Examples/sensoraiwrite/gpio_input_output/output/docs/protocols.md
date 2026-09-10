# PROTOCOLS.md — Product Protocol Definitions

> This document defines every product-level protocol and frame format used by the firmware.
> Sources: UM2005C datasheet (TWI protocol), AHT10 known behavior, product design decisions.
> This is the canonical protocol reference for the sensor node.

---

## 1. I2C Protocol: MCU ↔ AHT10 (Temperature/Humidity Sensor)

### 1.1 Bus Parameters

| Parameter | Value | Source |
|-----------|-------|--------|
| Bus | I2C1 (hardware peripheral) | CW32L010_DataSheet_CN_V1.0.pdf#page=19 |
| Speed | 100 kHz (standard mode) | connectivity.json (unverified, assumed) |
| 7-bit address | 0x38 | Industry standard for AHT10 |
| Write address | 0x70 | (0x38 << 1) |
| Read address | 0x71 | (0x38 << 1) \| 1 |
| Pull-ups | 4.7KΩ to BAT (R1 on SCL, R2 on SDA) | connectivity.json nets N05845, N06024 |
| MCU Pins | PA03 (SDA, AF2), PA04 (SCL, AF2) | CW32L010_DataSheet_CN_V1.0.pdf#page=24 |

### 1.2 Sensor Commands

#### 1.2.1 Initialize Sensor (send once after power-up)

| Step | I2C Action | Data | Description |
|------|-----------|------|-------------|
| 1 | START + 0x70 + WA | — | Send write address |
| 2 | Write byte | 0xE1 | Initialize command (per AHT10 spec) |
| 3 | STOP | — | — |
| 4 | Wait | ≥10ms | Sensor initialization time |

**Assumption**: The init command 0xE1 is used by AHT10. Verify with actual datasheet.

#### 1.2.2 Trigger Measurement

| Step | I2C Action | Data | Description |
|------|-----------|------|-------------|
| 1 | START + 0x70 + WA | — | Send write address |
| 2 | Write byte 1 | 0xAC | Trigger measurement command |
| 3 | Write byte 2 | 0x33 | Data byte 1 (per AHT10 spec) |
| 4 | Write byte 3 | 0x00 | Data byte 2 (per AHT10 spec) |
| 5 | STOP | — | — |
| 6 | Wait | ≥75ms | Measurement time (max 80ms per datasheet) |

**Source**: Industry-standard AHT10 protocol; SDK measure.c uses similar pattern.

#### 1.2.3 Read Measurement (6 bytes)

| Step | I2C Action | Data | Description |
|------|-----------|------|-------------|
| 1 | START + 0x71 + RD | — | Send read address |
| 2 | Read byte 0 | status_byte | Bit[7]=1=busy, 0=ready; Bit[3]=calibration enable |
| 3 | Read byte 1 | hum_hi[7:0] | Humidity MSB (high 8 bits of 20-bit raw) |
| 4 | Read byte 2 | hum_lo_and_temp | Humidity bits[19:12] (low nibble) + temp bits[19:16] (high nibble) |
| 5 | Read byte 3 | temp_hi | Temperature bits[15:8] |
| 6 | Read byte 4 | temp_lo | Temperature bits[7:0] |
| 7 | Read byte 5 | reserved/crc | Reserved or CRC (AHT10: reserved; AHT21B: CRC) |
| 8 | NACK + STOP | — | Master signals end |

#### 1.2.4 Data Conversion

```c
// Raw data assembly (20-bit values)
uint32_t raw_hum = ((uint32_t)bytes[1] << 12) | ((uint32_t)bytes[2] << 4) | (bytes[3] >> 4);
uint32_t raw_temp = ((uint32_t)(bytes[3] & 0x0F) << 16) | ((uint32_t)bytes[4] << 8) | bytes[5];

// Convert to physical values (×10 precision)
int16_t temp_x10 = (int16_t)((raw_temp * 2000.0 / 1048576.0) - 500);  // -500..+850 = -50.0..+85.0°C

uint16_t hum_x10 = (uint16_t)(raw_hum * 1000.0 / 1048576.0);  // 0..1000 = 0.0..100.0%
```

**Source**: Derived from SDK measure.c formulas and common AHT10 conversion.

### 1.3 Error Handling

| Condition | Behavior | Error Code |
|-----------|----------|------------|
| NACK on address | Retry 3x, then report ERR_NACK | -4 |
| Bus timeout (>10ms) | Reset I2C peripheral, report ERR_TIMEOUT | -2 |
| Status byte bit[7]=1 for >200ms | Report ERR_TIMEOUT (sensor stuck busy) | -2 |

---

## 2. TWI Protocol: MCU ↔ UM2005C (RF Transmitter)

### 2.1 Bus Parameters

| Parameter | Value | Source |
|-----------|-------|--------|
| Interface | Bit-banged GPIO (proprietary TWI) | UM2005C_数据手册_V1.2_V1.2.pdf#page=12 |
| CLK pin | PB02 (MCU pin 19), GPIO PP output | connectivity.json net RF_CLK |
| DATA pin | PB03 (MCU pin 20), GPIO bidirectional | connectivity.json net RF_DATA |
| CLK frequency | ≤ 1 MHz | UM2005C_数据手册_V1.2_V1.2.pdf#page=12 |
| Sampling edge | Falling edge | UM2005C_数据手册_V1.2_V1.2.pdf#page=12 |
| Supply | 1.9–3.6V (BAT rail, CR2032) | UM2005C_数据手册_V1.2_V1.2.pdf#page=1 |
| Carrier frequency | 433 MHz (assumed based on matching network) | UM2005C_数据手册_V1.2_V1.2.pdf#page=17 |
| TX power | 0 dBm | User note: "发射功率0DB" |

### 2.2 TWI Transaction Format

Each TWI read/write transaction is **16 CLK cycles**:

```
CLK   ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐
      ┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─
        ▲   ▲   ▲   ▲   ▲   ▲   ▲   ▲   ▲   ▲   ▲   ▲   ▲   ▲   ▲   ▲
DATA  [W/R][A5 ][A4 ][A3 ][A2 ][A1 ][A0 ][D7 ][D6 ][D5 ][D4 ][D3 ][D2 ][D1 ][D0 ]
       ───────────────────── ▶───────── ───────────────────── ▶─────────
            Address byte (A[5:0] + W/R)             Data byte (D[7:0])
           Write: W/R=1, Read: W/R=0            
```

- Cycles 1-8: Address byte → DATA driven by MCU (input)
  - Cycle 1: W/R bit (1=write, 0=read)
  - Cycles 2-7: Address bits A5-A0 (MSB first)
  - Cycle 8: Reserved (must be 1? per datasheet)
- Cycles 9-16: Data byte
  - **Write (W/R=1)**: DATA driven by MCU (input), D7-D0
  - **Read (W/R=0)**: DATA driven by UM2005C (output), D7-D0

**Source**: UM2005C_数据手册_V1.2_V1.2.pdf#page=12

### 2.3 Special TWI Commands

| Command | Value (hex) | TWI Transaction | Description |
|---------|-------------|----------------|-------------|
| TWI_ON | 32× CLK with DATA=0 | 32 CLK cycles, DATA=0 throughout | Enter programming mode, reset TWI |
| TWI_OFF | 0xFF02 | Write addr=0x3F, data=0x02 | Exit programming mode |
| SOFT_RST | 0xFF04 | Write addr=0x3F, data=0x04 | Reset all digital except TWI |
| TWI_RST | 0xFF01 | Write addr=0x3F, data=0x01 | Reset TWI module only |
| SLEEP | 0xFFFF | See 2.4 | Enter sleep mode |

**Source**: UM2005C_数据手册_V1.2_V1.2.pdf#page=12

Note: For special commands, "0xFF02" means:
- First 8 cycles (address): W/R=1, A5-A0 = 0x3F → address = 0x3F (write direction)
- Second 8 cycles (data): 0x02

### 2.4 Sleep Mode Entry

After TWI_OFF command, the UM2005C enters sleep automatically after ~1ms idle. To force immediate sleep after TX:
1. Send TWI_OFF (0xFF02)
2. Hold CLK low for ≥15ms
3. Chip enters sleep mode (10 nA typ)
4. Wake on CLK rising edge or DATA edge trigger (if OTP configured)

**Source**: UM2005C_数据手册_V1.2_V1.2.pdf#page=14

### 2.5 RF Transmission Sequence

```
Power-on / Wake sequence:
┌─────────────────────────────────────────────────────────────────────┐
│ 1. CLK rising edge → chip wakeup (if in sleep)                     │
│ 2. Wait ~400μs for XTAL startup                                    │
│ 3. Send TWI_ON (32 CLK cycles with DATA=0)                         │
│ 4. Wait ~300μs for VCO calibration                                 │
│ 5. Write configuration registers via TWI (address range 0x00-0x3E) │
│ 6. Write data payload (14 bytes: preamble + UID + sensor data)     │
│ 7. Send TWI_OFF (0xFF02)                                           │
│ 8. Hold CLK low for ≥15ms → chip enters sleep                      │
└─────────────────────────────────────────────────────────────────────┘

Total active time: ~1.2ms (wakeup) + ~161ms (OTP max) or ~1ms(RAM config)
```

**Notes**: 
- The UM2005C OTP is programmed once; subsequent power-ups auto-load OTP.
- For production, OTP must be programmed with carrier frequency, data rate, output power, etc.
- For development, registers can be written to RAM on each wakeup (no OTP needed).

**Source**: UM2005C_数据手册_V1.2_V1.2.pdf#page=6 (timing), page=14 (state diagram)

### 2.6 Configuration Register Map (OTP)

Total: 63 bytes (addresses 0x00 to 0x3E)

Key registers (from UM2005C_数据手册_V1.2_V1.2.pdf#page=13):

| Address | Name | Description | Typical Value (433MHz, 0dBm) |
|---------|------|-------------|-------------------------------|
| 0x00 | CFG1 | RF frequency configuration | TBD (depends on crystal) |
| 0x01 | CFG2 | Output power, modulation | 0x0X (0dBm GFSK) |
| 0x02-0x3D | Various | Data rate, shaping, packet config | Per template |
| 0x3E | LBD_CFG | Low battery detection threshold | 0x02 (2.4V) |

**Detailed register map**: See UM2005C_数据手册_V1.2_V1.2.pdf#page=13 for complete register definitions.

### 2.7 TWI Bit-bang Implementation Notes

```c
// Pseudocode for TWI bit-bang

void twi_delay(void) {
    // ~500ns half-period for 1MHz CLK
    // Implement with NOP loops calibrated to HCLK
    __NOP(); __NOP(); // adjust for 48MHz HCLK
}

void twi_write_byte(uint8_t w_r_bit, uint8_t addr_or_data) {
    // Cycle 1: W/R bit
    GPIO_Set(PB03, w_r_bit ? HIGH : LOW);
    GPIO_Set(PB02, HIGH); twi_delay();
    GPIO_Set(PB02, LOW);  twi_delay();
    
    // Cycles 2-8: A5-A0 + reserved bit (write), or D7-D0 (data byte)
    for (int i = 7; i >= 0; i--) {
        GPIO_Set(PB03, (addr_or_data >> i) & 1);
        GPIO_Set(PB02, HIGH); twi_delay();
        GPIO_Set(PB02, LOW);  twi_delay();
    }
}

uint8_t twi_read_byte(void) {
    uint8_t value = 0;
    GPIO_Set(PB03, INPUT_MODE);  // Release DATA line
    
    for (int i = 7; i >= 0; i--) {
        GPIO_Set(PB02, HIGH); twi_delay();
        if (GPIO_Read(PB03)) value |= (1 << i);
        GPIO_Set(PB02, LOW);  twi_delay();
    }
    
    GPIO_Set(PB03, OUTPUT_MODE);  // Re-assert control
    return value;
}

void twi_send_command(uint16_t command) {
    // command = (W/R||A5..A0<<8) | data_byte
    uint8_t addr_byte = (command >> 8) & 0xFF;
    uint8_t data_byte = command & 0xFF;
    twi_write_byte(1, addr_byte);  // Write address
    twi_write_byte(1, data_byte);  // Write data
}

void twi_on(void) {
    // 32 CLK cycles with DATA=0
    for (int i = 0; i < 32; i++) {
        GPIO_Set(PB03, LOW);
        GPIO_Set(PB02, HIGH); twi_delay();
        GPIO_Set(PB02, LOW);  twi_delay();
    }
}
```

---

## 3. RF Packet Protocol (Application Layer)

### 3.1 Packet Format

Total payload: **14 bytes** transmitted to UM2005C for RF modulation.

```
Offset  Size  Field          Description
─────────────────────────────────────────────────────
 0       2    Preamble       0xAA, 0x55 sync pattern
 2       6    MCU UID        Lower 48 bits of MCU unique ID
 8       2    Temperature    temperature_x10, big-endian (int16)
10       2    Humidity       humidity_x10, big-endian (uint16)
12       1    Status         System status byte
13       1    CRC8           CRC-8/MAXIM over bytes 0-12
─────────────────────────────────────────────────────
Total:   14 bytes
```

### 3.2 Field Definitions

**Preamble (2 bytes)**: 0xAA 0x55 — allows receiver to synchronize bit timing.

**MCU UID (6 bytes)**: The lower 48 bits of the CW32L010's 96-bit unique identifier, stored at Flash addresses:
- 0x1FFFF7AC: UID[31:0]  (4 bytes)
- 0x1FFFF7B0: UID[63:32] (4 bytes)
- 0x1FFFF7B4: UID[95:64] (4 bytes)

We use UID[47:0] from the first 6 bytes. Source: User note "UID用Mcu的".

**Temperature (2 bytes, big-endian)**: `temperature_x10` from sensor_data_t.
- Example: 25.4°C → 254 → 0x00 0xFE
- Range: -500..+850 → -50.0°C..+85.0°C (AHT10 range)
- Signed int16, big-endian

**Humidity (2 bytes, big-endian)**: `humidity_x10` from sensor_data_t.
- Example: 62.3% → 623 → 0x02 0x6F
- Range: 0..1000 → 0.0%..100.0%
- Unsigned uint16, big-endian

**Status (1 byte)**:
| Bit | Name | Description |
|-----|------|-------------|
| 0 | BAT_LOW | 1 = battery voltage low (<2.4V) |
| 1 | SENSOR_VALID | 1 = sensor data is fresh and valid |
| 2 | SENSOR_FAIL | 1 = last sensor read failed |
| 3 | RF_DONE | 1 = RF TX completed successfully |
| 4-7 | Reserved | Always 0 |

**CRC8 (1 byte)**: CRC-8/MAXIM (Dallas 1-Wire) polynomial 0x31 (x^8 + x^5 + x^4 + 1).
- Initial value: 0x00
- Final XOR: 0x00
- Covers bytes 0-12 inclusive (13 bytes)
- Receiver recalculates and compares

### 3.3 Concrete Packet Example

**Scenario**: Temperature = 25.4°C, Humidity = 62.3%, UID = 0xAABBCCDDEEFF, Status = 0x07

```
Byte  Value  Description
──────────────────────────────────
 0    0xAA   Preamble byte 0
 1    0x55   Preamble byte 1
 2    0xAA   UID byte 0
 3    0xBB   UID byte 1
 4    0xCC   UID byte 2
 5    0xDD   UID byte 3
 6    0xEE   UID byte 4
 7    0xFF   UID byte 5
 8    0x00   Temperature MSB (254 >> 8)
 9    0xFE   Temperature LSB (254 & 0xFF) = 25.4°C
10    0x02   Humidity MSB (623 >> 8)
11    0x6F   Humidity LSB (623 & 0xFF) = 62.3%
12    0x07   Status: BAT_LOW=1, SENSOR_VALID=1, RF_DONE=1
13    0x??   CRC8 over bytes 0-12
──────────────────────────────────
```

CRC8 computation Python reference:

```python
def crc8_maxim(data):
    crc = 0x00
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x80:
                crc = (crc << 1) ^ 0x31
            else:
                crc <<= 1
            crc &= 0xFF
    return crc
```

### 3.4 Transmit State Machine

```
IDLE ──→ WAKE_RF ──→ TWI_ON ──→ CFG_WRITE ──→ DATA_WRITE ──→ TWI_OFF ──→ SLEEP ──→ IDLE
  ▲                                                                                    │
  └────────────────────────────────────────────────────────────────────────────────────┘

State           Trigger         Action                                      Exit Cond
─────           ───────         ──────                                      ─────────
IDLE            sensor_data     Set PB02=PB03=0, start sequence             done → WAKE_RF
WAKE_RF         enter state     Delay ~1.2ms for XTAL+VCO startup           timeout → TWI_ON
TWI_ON          enter state     Toggle 32 CLK cycles with DATA=0            done → CFG_WRITE
CFG_WRITE       enter state     Write 63 OTP registers (or skip if OTP'd)   done → DATA_WRITE
DATA_WRITE      enter state     Write 14-byte packet via TWI                done → TWI_OFF
TWI_OFF         enter state     Send 0xFF02 command                         done → SLEEP
SLEEP           enter state     Hold CLK low ≥15ms                          timeout → IDLE
```

---

## 4. Power Management Protocol

### 4.1 DeepSleep Entry Sequence

```
Sensor read & RF TX complete
  ↓
1. Disable unused peripherals (I2C, UART if not needed)
2. Configure RTC alarm for next wake (current time + 150s)
3. Clear RTC flags
4. Set system to DeepSleep mode (CW32L010_PWR_DeepSleep)
   - HSIOSC off, HSE off
   - LSI keeps running (drives RTC)
   - IWDT keeps running
5. MCU enters DeepSleep
  ↓
[~150s later]
  ↓
RTC alarm event → wakes MCU
  ↓
1. HSIOSC restarts (~4μs)
2. Enable peripherals
3. Start new measurement cycle
```

**Source**: CW32L010_DataSheet_CN_V1.0.pdf#page=11 (low-power modes)

### 4.2 Battery Monitoring

- Initial: CR2032 provides ~3.0V fresh
- Low threshold (software): Monitor via ADC or LVD
- Below ~2.4V → Set BAT_LOW status bit in RF packet
- UM2005C has internal LBD (Low Battery Detection) readable via TWI, range 2.0-3.1V

**Source**: UM2005C_数据手册_V1.2_V1.2.pdf#page=13 (LBD)

---

## 5. UART Console Protocol

### 5.1 TX Output Format

After each measurement cycle:
```
[T=25.4C H=62.3% UID=AABBCCDDEEFF STS=0x07 CRC=OK]
```

### 5.2 RX Command Interface (Optional)

| Command | Format | Response | Description |
|---------|--------|----------|-------------|
| Read now | `r\n` | `[T=... H=...]` | Trigger immediate sensor read + RF TX |
| Set interval | `i 60\n` | `[Interval=60s]` | Change wake interval (10-600s) |
| Status | `s\n` | `[Uptime=... Count=...]` | Show system status |
| Sleep | `slp\n` | (none) | Enter DeepSleep immediately |

---

## 6. Protocol Source Summary

| Protocol | Document | Section |
|----------|----------|---------|
| AHT10 I2C | inputs/datasheets/AHT10.pdf (NOT FOUND locally; based on industry knowledge) | — |
| UM2005C TWI | UM2005C_数据手册_V1.2_V1.2.pdf | page 12 |
| UM2005C OTP | UM2005C_数据手册_V1.2_V1.2.pdf | page 13 |
| UM2005C States | UM2005C_数据手册_V1.2_V1.2.pdf | page 14 |
| UM2005C Timing | UM2005C_数据手册_V1.2_V1.2.pdf | page 6 |
| MCU DeepSleep | CW32L010_DataSheet_CN_V1.0.pdf | page 11 |
| MCU Pin AF | CW32L010_DataSheet_CN_V1.0.pdf | page 23-24 |
| MCU UID address | CW32L010 (standard STM32-compatible UID base) | 0x1FFFF7AC |

**⚠ Note**: AHT10 protocol details are assumed from industry practice and the SDK measure.c example. An official AHT10 datasheet should be obtained to verify command bytes and timing before production.
