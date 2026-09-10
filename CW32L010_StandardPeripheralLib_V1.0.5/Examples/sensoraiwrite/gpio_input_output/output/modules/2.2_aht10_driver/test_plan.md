# AHT10 Driver — Test Plan

> Module: 2.2 aht10_driver
> MCU: CW32L010Y8M6 (TSSOP-20)
> Device: AHT10 Temperature/Humidity Sensor @ I2C addr 0x38
> I2C transport: Bit-bang via i2c_drv (module 2.1) on PA03(SDA)/PA04(SCL)
> Date: 2025-07-17

---

## 1. Test Strategy

| Layer | Test Level | Execution | Environment |
|-------|-----------|-----------|-------------|
| Unit | Static API validation | Compile-time | Host (any C compiler) |
| Integration | I2C transaction verification | On-target | CW32L010 + AHT10 on custom PCB |
| Integration | Conversion formula accuracy | Host (Python/C) | Offline with test vectors |
| System | End-to-end 150s cycle | On-target | Full system with RF + RTC |

---

## 2. Unit Tests (Static / Compile-Time Checks)

### 2.1 Header Inclusion Tests

```c
// TEST: Verify all includes resolve
#include "interface.h"
#include "aht10_driver.h"

// TEST: Verify type sizes
static_assert(sizeof(aht10_data_t) <= 16, "aht10_data_t too large");
static_assert(sizeof(aht10_error_t) == sizeof(int), "aht10_error_t wrong size");
```

**Pass criteria**: Clean compile with no errors or warnings.

### 2.2 Constant Validation

| Constant | Expected | Check |
|----------|----------|-------|
| AHT10_I2C_ADDR | 0x38 | Must match connectivity.json I2C device |
| AHT10_CMD_TRIGGER | 0xAC | Industry standard |
| AHT10_READ_LEN | 6 | AHT10 response size |
| AHT10_MEAS_DELAY_MS | 75 | >=75 ms per spec |

---

## 3. Integration Tests (On-Target)

### 3.1 TC-I2C-001: I2C Bus Initialization

**Setup**: Call `i2c_drv_init()` followed by `aht10_init()`

**Procedure**:
1. Measure SCL/SDA idle state (both HIGH)
2. Verify AHT10 ACKs probe (i2c_drv_probe returns true)

**Expected**:
- SCL = 3.0V (HIGH), SDA = 3.0V (HIGH) during idle
- `aht10_init()` returns `AHT10_OK`

**Pass/Fail**: □ Pass  □ Fail

### 3.2 TC-I2C-002: I2C Write — Trigger Command

**Setup**: After aht10_init()

**Procedure**:
1. Capture SDA/SCL with logic analyzer on PA03/PA04
2. Call `aht10_trigger()`
3. Verify on scope/analyzer:
   - START condition
   - Address byte = 0x70 (0x38 << 1 | W)
   - Data bytes: 0xAC, 0x33, 0x00
   - All bytes ACK'd (SDA low on 9th clock)
   - STOP condition

**Expected**: `aht10_trigger()` returns `AHT10_OK`

**Pass/Fail**: □ Pass  □ Fail

### 3.3 TC-I2C-003: I2C Read — Measurement Data

**Setup**: After aht10_init() + aht10_trigger() + 75ms delay

**Procedure**:
1. Call `aht10_read(&data)`
2. Verify on logic analyzer:
   - START condition
   - Address byte = 0x71 (0x38 << 1 | R)
   - 6 bytes received
   - Master ACKs bytes 0-4, NACKs byte 5
   - STOP condition
3. Print raw data: `raw[0..5] = 0x%02X`

**Expected**: `aht10_read()` returns `AHT10_OK`

**Pass/Fail**: □ Pass  □ Fail

### 3.4 TC-I2C-004: Full Measurement Cycle

**Setup**: After aht10_init()

**Procedure**:
1. Call `aht10_measure(&data)`
2. Verify return value is `AHT10_OK`
3. Print result: `"T=%d C H=%d %%RH raw_status=0x%02X"`
   (with x10 fixed-point: T=253 => 25.3°C, H=624 => 62.4%RH)

**Expected**:
- `temperature_x10` in range -400 to +850 (i.e. -40.0°C to +85.0°C)
- `humidity_x10` in range 0 to 1000 (0.0% to 100.0% RH)
- `status_byte` bit 7 = 0 (not busy)

**Pass/Fail**: □ Pass  □ Fail

### 3.5 TC-I2C-005: Sensor Not Present Detection

**Setup**: Remove AHT10 from socket or disconnect I2C bus

**Procedure**:
1. Call `aht10_init()`
2. Check return value

**Expected**: `aht10_init()` returns `AHT10_ERR_I2C` (probe fails on address NACK)

**Pass/Fail**: □ Pass  □ Fail

### 3.6 TC-I2C-006: Repeated Measurements

**Setup**: After aht10_init()

**Procedure**:
1. Call `aht10_measure(&d1)` → wait 100ms
2. Call `aht10_measure(&d2)` → wait 100ms
3. Call `aht10_measure(&d3)`
4. Compare d1, d2, d3 for reasonable variation

**Expected**:
- All three return `AHT10_OK`
- Temperature variation < 2.0°C between consecutive reads
- Humidity variation < 5% RH between consecutive reads

**Pass/Fail**: □ Pass  □ Fail

### 3.7 TC-I2C-007: Soft Reset

**Setup**: After aht10_init() + one measurement

**Procedure**:
1. Call `aht10_soft_reset()`
2. Wait 20ms
3. Call `aht10_measure(&data)`
4. Verify valid data returned

**Expected**: Both calls return `AHT10_OK`, measurement yields sensible values

**Pass/Fail**: □ Pass  □ Fail

---

## 4. Conversion Formula Validation (Offline)

Use the following test vectors to verify `aht10_raw_to_data()`:

| Test | Raw[0..5] (hex) | Expected temp_x10 | Expected hum_x10 | Source |
|------|-----------------|-------------------|------------------|--------|
| V1 | 67 8C 8B 0B 85 00 | 259 (25.9°C) | 640 (64.0%RH) | Known-good capture |
| V2 | 66 00 00 0C 4C 00 | 301 (30.1°C) | 609 (60.9%RH) | Known-good capture |
| V3 | 6B B3 33 09 99 00 | 200 (20.0°C) | 669 (66.9%RH) | Synthetic |
| V4 | 00 00 00 00 00 00 | -500 (-50.0°C) | 0 (0.0%RH) | Zero edge case |
| V5 | FF FF FF FF FF 00 | 850 (85.0°C) | 999 (99.9%RH) | Near max edge case |

### 4.1 Offline Test Script (Python)

```python
#!/usr/bin/env python3
"""
Test AHT10 conversion formulas offline.
"""

RAW_MAX = 1048576
HUM_MULT = 1000
TEMP_MULT = 2000
TEMP_OFFSET = 500

def aht10_raw_to_data(raw_bytes):
    raw_hum = (raw_bytes[0] << 12) | (raw_bytes[1] << 4) | (raw_bytes[2] >> 4)
    raw_temp = ((raw_bytes[2] & 0x0F) << 16) | (raw_bytes[3] << 8) | raw_bytes[4]
    
    hum_x10 = (raw_hum * HUM_MULT) // RAW_MAX
    temp_x10 = (raw_temp * TEMP_MULT) // RAW_MAX - TEMP_OFFSET
    
    return temp_x10, hum_x10

# Test vectors
test_vectors = [
    ([0x67, 0x8C, 0x8B, 0x0B, 0x85, 0x00], 259, 640),
    ([0x66, 0x00, 0x00, 0x0C, 0x4C, 0x00], 301, 609),
    ([0x6B, 0xB3, 0x33, 0x09, 0x99, 0x00], 200, 669),
    ([0x00, 0x00, 0x00, 0x00, 0x00, 0x00], -500, 0),
    ([0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00], 850, 999),
]

print("Testing AHT10 conversion formulas...")
all_pass = True
for i, (raw, exp_t, exp_h) in enumerate(test_vectors):
    t, h = aht10_raw_to_data(raw)
    ok = (t == exp_t) and (h == exp_h)
    status = "PASS" if ok else "FAIL"
    if not ok:
        all_pass = False
    print(f"  V{i+1}: raw={[f'0x{b:02X}' for b in raw]} -> "
          f"T={t} (exp={exp_t}), H={h} (exp={exp_h}) [{status}]")

print(f"\n{'All tests PASSED!' if all_pass else 'SOME TESTS FAILED!'}")
```

---

## 5. Boundary Tests

### 5.1 Long-Term Stability

| Test | Duration | Sample Rate | Criteria |
|------|----------|-------------|----------|
| Continuous reads | 24 hours | Every 10 seconds | No I2C errors, no sensor lockup |
| Temperature drift | 1 hour in thermal chamber | Every 10 seconds | Reading within +/-3°C of reference sensor |

### 5.2 Power Cycle

1. Power off system (remove battery)
2. Wait 10 seconds
3. Power on
4. Call `aht10_init()` + `aht10_measure()`
5. Verify valid data

**Expected**: Sensor initializes and returns correct data after power cycle

### 5.3 Bus Recovery

1. Force I2C bus into hung state (hold SDA low manually)
2. Call `aht10_init()`
3. Verify driver recovers via bus clear sequence

**Expected**: i2c_drv_bus_clear() generates 9 SCL pulses + STOP, then probe succeeds

---

## 6. Resource Usage Verification

| Resource | Expected | Measurement Method |
|----------|----------|-------------------|
| Code size (flash) | ~1-2 KB | Build map file |
| RAM (stack + data) | < 64 bytes | Static analysis + map file |
| Active time per measure | ~78 ms (75 ms wait + 3 ms I2C) | GPIO toggle + oscilloscope |
| I2C bus idle state | SCL=HIGH, SDA=HIGH | Voltmeter on PA03/PA04 |

---

## 7. Pass / Fail Summary

| TC ID | Test Name | Result | Notes |
|-------|-----------|--------|-------|
| TC-I2C-001 | Bus Initialization | □ Pass □ Fail | |
| TC-I2C-002 | Trigger Command | □ Pass □ Fail | |
| TC-I2C-003 | Read Data | □ Pass □ Fail | |
| TC-I2C-004 | Full Measurement | □ Pass □ Fail | |
| TC-I2C-005 | No Sensor Detection | □ Pass □ Fail | |
| TC-I2C-006 | Repeated Measurements | □ Pass □ Fail | |
| TC-I2C-007 | Soft Reset | □ Pass □ Fail | |
| V1-V5 | Conversion Formulas | □ Pass □ Fail | Run Python script |
| Long-Term | 24h Stability | □ Pass □ Fail | |
| Power Cycle | Startup After Reset | □ Pass □ Fail | |

**Overall Result**: □ Pass  □ Fail  (all must pass)

---

## 8. Hardware Dependencies for Testing

| Item | Purpose | Notes |
|------|---------|-------|
| CW32L010 dev board or custom PCB | MCU target | TSSOP-20 package |
| AHT10 sensor module | Device under test | I2C, 3.3V |
| Logic analyzer (4+ channels) | Capture I2C transactions | Recommend 24 MHz+ sampling |
| Multimeter | Check pin voltages, pull-ups | — |
| Oscilloscope (optional) | Timing verification | For SCL frequency check |
| UART-to-USB converter | Console output | Connected to J3 (PA05/PA06) |
| CR2032 battery or 3V supply | Power | For realistic power testing |
