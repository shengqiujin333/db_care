# Test Plan: Phase 2.7 — sensor_mgr (Application Layer)

## 1. Overview

**Module**: sensor_mgr (layer=application)  
**Dependencies**: aht10_driver (2.2), i2c_driver (2.1), packet_builder (2.6), power_manager (2.5), uart_console (2.3)  
**MCU**: CW32L010Y8M6 (TSSOP-20)  
**Bus**: I2C (AHT10 sensor at 0x38)

The sensor manager orchestrates: 2-sample measurement → averaging → RF packet build → console output.

---

## 2. Test Levels

| Level | Environment | Scope |
|-------|-------------|-------|
| L1 | PC/host (unit test) | sensor_mgr logic: averaging, error handling, validation, status tracking. No HW dependencies. |
| L2 | MCU + sensor simulator | sensor_mgr_run() with simulated AHT10 responses via I2C loopback/emulator. |
| L3 | MCU + real AHT10 | Full end-to-end sensor measurement cycle on real hardware. |
| L4 | System integration | sensor_mgr + schedule + rf_mgr: full 150s cycle. |

---

## 3. Unit Test Cases (L1 — PC/Host)

### TC-1.1: Init / Deinit
- **Input**: Call `sensor_mgr_init()` then `sensor_mgr_deinit()`
- **Expected**: `sensor_mgr_get_status()` shows `SENSOR_MGR_STATE_UNINIT` after deinit
- **Pass criteria**: State transitions correct

### TC-1.2: Normal 2-Sample Average
- **Input**: Both AHT10 samples succeed with known values
  - Sample 1: T=254, H=623
  - Sample 2: T=256, H=619
- **Expected**: 
  - Averaged output: T=255, H=621 (integer arithmetic)
  - Status: `SENSOR_STATUS_VALID` (no STALE flag)
  - RF packet built with correct byte layout
- **Pass criteria**: T_avg=255, H_avg=621, status=0x01

### TC-1.3: Sample #2 Failure (Stale)
- **Input**: Sample 1 succeeds (T=250, H=600), Sample 2 fails
- **Expected**: 
  - Output uses sample 1 alone
  - Status: `SENSOR_STATUS_VALID | SENSOR_STATUS_STALE` (0x03)
  - Console PRINT still executed
- **Pass criteria**: Out=250/600, status=0x03, stale_cycles++

### TC-1.4: Sample #1 Failure (Stale)
- **Input**: Sample 1 fails, Sample 2 succeeds (T=260, H=610)
- **Expected**: Same as TC-1.3 but sample 2 used
- **Pass criteria**: Out=260/610, status=0x03

### TC-1.5: Both Samples Fail
- **Input**: Both AHT10 samples fail
- **Expected**: 
  - Return `ERR_READ_FAIL`
  - Status: `SENSOR_STATUS_READ_FAIL` (0x08)
  - failed_cycles incremented
  - RF packet still built (with failure status)
- **Pass criteria**: Return -6, status=0x08

### TC-1.6: Data Validation — Temperature Out of Range
- **Input**: Sample returns T=9999 (impossible value)
- **Expected**: Sample treated as failure (validation rejects it)
- **Pass criteria**: Sample marked READ_FAIL, averages use only valid sample

### TC-1.7: Data Validation — Humidity Out of Range
- **Input**: Sample returns H=9999 (impossible value)
- **Expected**: Same as TC-1.6
- **Pass criteria**: Sample rejected

### TC-1.8: NULL Output Pointers
- **Input**: `sensor_mgr_run(NULL, NULL)`
- **Expected**: Return `ERR_INVALID_PARAM` (-3)
- **Pass criteria**: Return code -3

### TC-1.9: Uninitialized Module
- **Input**: `sensor_mgr_run()` without calling init
- **Expected**: Return `ERR_NOT_INIT` (-8)
- **Pass criteria**: Return code -8

### TC-1.10: Status Query
- **Input**: Run several cycles, call `sensor_mgr_get_status()`
- **Expected**: Status reflects total_cycles, failed_cycles, last_temp_x10, last_hum_x10
- **Pass criteria**: Counters match actual run count

### TC-1.11: Consecutive Failures
- **Input**: Run 3 cycles with both samples failing
- **Expected**: consecutive_failures increments each cycle
- **Pass criteria**: consecutive_failures == 3 after 3 failed cycles

### TC-1.12: Reset Failures
- **Input**: After consecutive failures, call `sensor_mgr_reset_failures()`
- **Expected**: Counter reset to 0
- **Pass criteria**: Counter == 0

### TC-1.13: Battery Low Flag Passed to Packet
- **Input**: Override `power_mgr_is_battery_low()` to return true
- **Expected**: RF packet sys_status byte has `SYS_STATUS_BAT_LOW` (0x01) bit set
- **Pass criteria**: Packet byte[12] & 0x01 == 0x01

### TC-1.14: Console Output Format
- **Input**: Successful measurement with T=254, H=623
- **Expected**: CONSOLE_PRINT receives "[T=25.4C H=62.3%]\r\n"
- **Pass criteria**: Format matches "[T=%d.%dC H=%u.%u%%]\r\n"

---

## 4. Integration Test Cases (L2 — MCU + Sensor Simulator)

### TC-2.1: I2C Bus Error Recovery
- **Input**: Inject I2C NACK on sample #1, success on sample #2
- **Expected**: Module uses sample #2, marks STALE
- **Pass criteria**: Module continues operation, stale_cycles++

### TC-2.2: Intermittent Bus Errors
- **Input**: Run 10 cycles, randomly fail 20% of I2C transactions
- **Expected**: Module never crashes; produces data with appropriate status flags
- **Pass criteria**: No asserts/hangs, all returns codes valid

### TC-2.3: Timing Verification
- **Input**: Measure actual timing of sensor_mgr_run()
- **Expected**: Total time ~260ms (2×75ms wait + 100ms inter-sample + overhead)
- **Pass criteria**: Duration within ±20ms of expected

---

## 5. Hardware Test Cases (L3 — Real AHT10)

### TC-3.1: First Power-Up Measurement
- **Input**: Cold boot, power on sensor, call sensor_mgr_run()
- **Expected**: Valid temperature/humidity reading after ~260ms
- **Pass criteria**: T in range 15-35°C, H in range 30-80% (lab conditions)

### TC-3.2: Repeated Cycles (Long-Run)
- **Input**: Run sensor_mgr_run() 100 times with 150s interval (4+ hours)
- **Expected**: No degradation, no I2C lockups, consistent data
- **Pass criteria**: 100% success rate, no bus errors

### TC-3.3: Sensor Disconnect / Reconnect
- **Input**: Physically disconnect AHT10 during operation, then reconnect
- **Expected**: Module reports failures while disconnected, recovers after reconnect
- **Pass criteria**: Automatic recovery without reset

---

## 6. System Integration Test (L4)

### TC-4.1: Full 150s Cycle
- **Input**: schedule (2.8) calls sensor_mgr_run() → rf_mgr_run()
- **Expected**: Between sleep cycles, average is computed, packet is transmitted
- **Pass criteria**: Valid RF packet transmitted with avg temp/hum

### TC-4.2: DeepSleep Recovery
- **Input**: After DeepSleep, call sensor_mgr_run()
- **Expected**: Module state retained, measurement succeeds
- **Pass criteria**: Valid data after wake

---

## 7. Pass/Fail Criteria Summary

| Category | Criterion | 
|----------|-----------|
| Unit tests | All TC-1.x pass on host test runner |
| Integration | TC-2.x pass on MCU + simulator |
| Hardware | TC-3.x pass on real AHT10 |
| Code review | No HAL-style API calls (CW32L010 SDK only) |
| Code review | All sources documented in B0 table |
| Code review | Error handling covers all aht10 return codes |

---

## 8. Test Implementation Notes

- For L1 testing: mock `aht10_measure()`, `power_mgr_is_battery_low()`, `SysTickDelay()`, `GetTick()`, `CONSOLE_PRINT`
- For L2 testing: use I2C emulator board (e.g., Arduino Nano as AHT10 simulator)
- For L3 testing: connect JST SH 1.0 4-pin to real AHT10 module
- For L4 testing: full system with RF receiver to verify packet content
- Use CW32L010 UART debug console to print module status for manual verification
