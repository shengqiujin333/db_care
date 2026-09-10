# I2C Driver Test Plan

> Module: 2.1 i2c_driver
> Component: Bit-bang I2C master on PA03(SDA)/PA04(SCL)
> Target device: AHT10 (addr 0x38)
> Date: 2025-07-17

---

## 1. Test Environment

| Item | Description |
|------|-------------|
| MCU | CW32L010Y8M6 on target PCB |
| I2C bus | PA03(SDA)/PA04(SCL), 4.7kOhm pull-ups to BAT |
| Target device | AHT10 temperature/humidity sensor (addr 0x38) |
| Debug | UART1 console @ 115200 8N1, or logic analyzer on SCL/SDA |
| Power | CR2032 (3V) or USB-powered debug probe |

### 1.1 Required Test Equipment

- Oscilloscope or logic analyzer (>=1 Msample/s) for timing verification
- Multimeter for pull-up resistor and voltage checks
- Optional: Second MCU as I2C slave for protocol validation

---

## 2. Test Cases

### TC-01: Initialization

| Field | Value |
|-------|-------|
| **ID** | TC-01 |
| **Name** | I2C Driver Initialization |
| **Type** | Unit / Functional |
| **Precondition** | MCU running at 48 MHz HSIOSC, GPIOA clock enabled |
| **Steps** | 1. Call `i2c_drv_init()` |
| | 2. Measure SCL and SDA voltages |
| **Expected result** | Both SCL and SDA are HIGH (>2.5V if BAT=3V) |
| **Pass criteria** | SCL > 0.7 x VDD, SDA > 0.7 x VDD |
| **Measured** | |

### TC-02: Bus Timing -- START Condition

| Field | Value |
|-------|-------|
| **ID** | TC-02 |
| **Name** | START Condition Timing |
| **Type** | Unit / Timing |
| **Precondition** | After TC-01 (driver initialized) |
| **Steps** | 1. Call `i2c_drv_probe(0x38)` |
| | 2. Capture SCL/SDA on oscilloscope |
| **Expected result** | SDA falls while SCL is HIGH |
| **Pass criteria** | SDA setup time before SCL falling >= 1 us |
| **Measured** | |

### TC-03: Address Transmission and ACK

| Field | Value |
|-------|-------|
| **ID** | TC-03 |
| **Name** | Slave Address ACK |
| **Type** | Integration |
| **Precondition** | TC-01 passed, AHT10 powered and on bus |
| **Steps** | 1. Call `i2c_drv_probe(0x38)` |
| | 2. Read return value |
| **Expected result** | Returns `true` (AHT10 ACKs its address) |
| **Pass criteria** | `i2c_drv_probe(0x38) == true` |
| **Measured** | |

### TC-04: Address NACK (No Device)

| Field | Value |
|-------|-------|
| **ID** | TC-04 |
| **Name** | Address NACK Detection |
| **Type** | Unit |
| **Precondition** | TC-01 passed |
| **Steps** | 1. Call `i2c_drv_probe(0x39)` (nonexistent address) |
| | 2. Read return value |
| **Expected result** | Returns `false` (no device at 0x39) |
| **Pass criteria** | `i2c_drv_probe(0x39) == false` |
| **Measured** | |

### TC-05: AHT10 Write Trigger

| Field | Value |
|-------|-------|
| **ID** | TC-05 |
| **Name** | AHT10 Trigger Measurement |
| **Type** | Integration |
| **Precondition** | TC-01 passed, AHT10 on bus |
| **Steps** | 1. Call `i2c_drv_master_write(0x38, (uint8_t[]){0xAC, 0x33, 0x00}, 3)` |
| | 2. Check return value |
| | 3. Wait 80 ms |
| **Expected result** | Returns `I2C_DRV_OK` |
| **Pass criteria** | `err == I2C_DRV_OK` |
| **Measured** | |

### TC-06: AHT10 Read Data

| Field | Value |
|-------|-------|
| **ID** | TC-06 |
| **Name** | AHT10 Read 6-Byte Measurement |
| **Type** | Integration |
| **Precondition** | TC-05 passed (trigger sent, 80 ms elapsed) |
| **Steps** | 1. Call `i2c_drv_master_read(0x38, buf, 6)` |
| | 2. Check return value |
| | 3. Print hex dump of buf[0..5] |
| **Expected result** | Returns `I2C_DRV_OK`, buf[0..4] contain valid data, buf[5] bit 7 = 0 |
| **Pass criteria** | `err == I2C_DRV_OK` AND buf[0..4] not all 0xFF |
| **Measured** | |

### TC-07: Combined Write-Read (Full Measurement Cycle)

| Field | Value |
|-------|-------|
| **ID** | TC-07 |
| **Name** | Complete AHT10 Measurement Cycle |
| **Type** | Integration / End-to-End |
| **Precondition** | TC-01 passed, AHT10 on bus |
| **Steps** | 1. `i2c_drv_master_write(0x38, {0xAC,0x33,0x00}, 3)` -> expect OK |
| | 2. `SysTickDelay(80)` |
| | 3. `i2c_drv_master_read(0x38, buf, 6)` -> expect OK |
| | 4. Convert raw data to temperature and humidity |
| | 5. Print "T=xx.xC H=xx.x%" via UART |
| **Expected result** | Temperature: 20-30 degC (room temp), Humidity: 30-70% |
| **Pass criteria** | Temperature in range -10 to +60 degC, humidity 0-100% |
| **Measured** | |

### TC-08: Clock Stretch Handling

| Field | Value |
|-------|-------|
| **ID** | TC-08 |
| **Name** | Slave Clock Stretch Timeout |
| **Type** | Unit / Negative |
| **Precondition** | TC-01 passed |
| **Steps** | 1. Wire SCL to GND via 1kOhm resistor (simulate stuck) |
| | 2. Call `i2c_drv_probe(0x38)` |
| | 3. Check return value |
| **Expected result** | Returns `false` (timeout after 10 ms) |
| **Pass criteria** | Function returns within 15 ms, returns error |
| **Measured** | |

### TC-09: Bus Clear Recovery

| Field | Value |
|-------|-------|
| **ID** | TC-09 |
| **Name** | Bus Clear After Hang |
| **Type** | Unit |
| **Precondition** | TC-01 passed |
| **Steps** | 1. Force SDA LOW by shorting to GND |
| | 2. Call `i2c_drv_bus_clear()` |
| | 3. Measure SDA with multimeter |
| | 4. Call `i2c_drv_probe(0x38)` |
| **Expected result** | SDA returns HIGH after bus_clear, probe succeeds |
| **Pass criteria** | SDA > 2.5V after clear, probe returns true |
| **Measured** | |

### TC-10: Multiple Consecutive Reads (Stability)

| Field | Value |
|-------|-------|
| **ID** | TC-10 |
| **Name** | Repeated Measurement Stability |
| **Type** | Integration / Stress |
| **Precondition** | TC-01 passed, AHT10 on bus |
| **Steps** | 1. For i = 1..10: |
| | 2.   Write trigger (0xAC 0x33 0x00) |
| | 3.   Wait 80 ms |
| | 4.   Read 6 bytes |
| | 5.   Convert and store values |
| | 6. Check for I2C errors |
| **Expected result** | All 10 transactions succeed, temperature varies <=2 degC, humidity <=5% |
| **Pass criteria** | 10/10 OK, no error codes |
| **Measured** | |

---

## 3. Test Execution Log Format

```
[TC-xx] Description
  Step 1: result
  Step 2: result
  PASS/FAIL: criteria check
```

### Example:

```
[TC-01] I2C Driver Initialization
  Step 1: i2c_drv_init() returned I2C_DRV_OK
  Step 2: SCL=3.08V, SDA=3.08V
  PASS: Both lines HIGH
```

---

## 4. Pass/Fail Summary

| TC-ID | Name | Status | Date | Tester |
|-------|------|--------|------|--------|
| TC-01 | Initialization | :white_large_square: | | |
| TC-02 | START Timing | :white_large_square: | | |
| TC-03 | Address ACK | :white_large_square: | | |
| TC-04 | Address NACK | :white_large_square: | | |
| TC-05 | Write Trigger | :white_large_square: | | |
| TC-06 | Read Data | :white_large_square: | | |
| TC-07 | Full Cycle | :white_large_square: | | |
| TC-08 | Clock Stretch | :white_large_square: | | |
| TC-09 | Bus Clear | :white_large_square: | | |
| TC-10 | Stability | :white_large_square: | | |

**Overall**: :white_large_square: / 10 passed

---

## 5. Test Code Snippet

```c
#include "i2c_driver.h"
#include "cw32l010_systick.h"
#include <stdio.h>

void test_i2c_driver(void)
{
    uint8_t buf[6];
    i2c_drv_error_t err;

    // TC-01: Init
    err = i2c_drv_init();
    printf("TC-01 init: %s\r\n", i2c_drv_strerror(err));

    // TC-03: Probe
    bool found = i2c_drv_probe(0x38);
    printf("TC-03 probe 0x38: %s\r\n", found ? "ACK" : "NACK");

    // TC-04: Probe nonexistent
    found = i2c_drv_probe(0x39);
    printf("TC-04 probe 0x39: %s %s\r\n",
           found ? "ACK" : "NACK",
           found ? "FAIL" : "PASS");

    // TC-07: Full cycle (3 iterations)
    for (int i = 0; i < 3; i++) {
        uint8_t trigger[] = {0xAC, 0x33, 0x00};
        err = i2c_drv_master_write(0x38, trigger, 3);
        if (err != I2C_DRV_OK) {
            printf("TC-07.%d write FAIL: %s\r\n", i, i2c_drv_strerror(err));
            continue;
        }

        SysTickDelay(80);  // 80 ms

        err = i2c_drv_master_read(0x38, buf, 6);
        if (err != I2C_DRV_OK) {
            printf("TC-07.%d read FAIL: %s\r\n", i, i2c_drv_strerror(err));
            continue;
        }

        // Convert
        uint32_t raw_hum = ((uint32_t)buf[0] << 12) |
                           ((uint32_t)buf[1] << 4)  |
                           ((uint32_t)buf[2] >> 4);
        uint32_t raw_temp = ((uint32_t)(buf[2] & 0x0F) << 16) |
                            ((uint32_t)buf[3] << 8) |
                            (uint32_t)buf[4];

        int16_t temp_x10 = (int16_t)((raw_temp * 2000.0 / 1048576.0) - 500);
        int16_t hum_x10  = (int16_t)(raw_hum * 1000.0 / 1048576.0);

        printf("TC-07.%d T=%d.%dC H=%d.%d%%\r\n",
               i,
               temp_x10 / 10, abs(temp_x10 % 10),
               hum_x10 / 10, hum_x10 % 10);
    }
}
```

---

## 6. Verification on Target

### 6.1 Visual Inspection

- [ ] R1 = 4.7 kOhm between PA03 (SDA) and BAT rail
- [ ] R2 = 4.7 kOhm between PA04 (SCL) and BAT rail
- [ ] AHT10 pin 2 (SCL) -> PA04 via R2
- [ ] AHT10 pin 3 (SDA) -> PA03 via R1
- [ ] AHT10 pin 4 (VDD) -> BAT rail
- [ ] AHT10 pin 1 (GND) -> GND

### 6.2 Logic Analyzer Observation

An oscilloscope or logic analyzer on PA03/PA04 should show:

1. **Idle**: Both lines HIGH (~3V)
2. **START**: SDA goes LOW, SCL stays HIGH -> then SCL goes LOW
3. **Address byte**: 0x70 (11100000) with ACK bit
4. **Data**: 0xAC 0x33 0x00 (trigger)
5. **STOP**: SCL goes HIGH, SDA goes HIGH
6. **After 80 ms**: START -> 0x71 (read) -> ACK -> 6 data bytes -> NACK -> STOP
