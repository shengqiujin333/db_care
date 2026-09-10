# Phase 2.8 — Schedule Module Test Plan

## 1. Overview

The schedule module is the top-level application coordinator. It contains `main()` and orchestrates the 150-second measurement cycle. Testing focuses on:

1. Boot/initialization correctness (clock, power, UART, TWI, sensor init)
2. Main loop cycle execution (measure → TX → sleep)
3. Error handling (sensor fail, RF fail, init fail)
4. Correct DeepSleep entry/exit via RTC AWT
5. IWDT refresh timing

### Test Environment

| Item | Value |
|------|-------|
| MCU | CW32L010Y8M6 (TSSOP-20) |
| Board | Custom hardware with AHT10 + UM2005C |
| Debug | SWD via CW-DAPLINK (J2) |
| Console | UART1 @ 115200 8N1 (J3, 3-pin connector) |
| Power | CR2032 (3V) or bench supply with current measurement |

---

## 2. Unit Tests (on-target, minimal hardware)

### UT1: schedule_init() — Cold Boot Clock Configuration

**Objective**: Verify that system clock is correctly configured to 48 MHz HSIOSC.

**Setup**: MCU powered from bench supply (3.0V), SWD connected.

**Procedure**:
1. Flash firmware with `CONFIG_CONSOLE_ENABLE=1`
2. Open serial terminal (115200 8N1)
3. Power cycle the board
4. Observe boot messages

**Pass Criteria**:
- Console shows `[CW32L010 Sensor Node v1.0]` on boot
- Console shows `[I] schedule: SystemCoreClock=48000000 Hz`
- Console shows `[I] schedule: initialized (cold_boot=1)`
- No error messages before entering main loop

**Measurement**: Measure HCLK frequency on MCO pin (if available) — should be 48 MHz ±2%.

**Source**: `cw32l010_sysctrl.h#L458`, `schedule.c#L96-L120`

---

### UT2: schedule_init() — Warm Boot (RTC Wake)

**Objective**: Verify cold/warm boot detection works correctly.

**Setup**: After completing UT1, let the system run through one full cycle.

**Procedure**:
1. After first DeepSleep entry (~150s), observe console output on wake
2. The second boot message should show `[SCHEDULE] RTC wake (reason=1)`

**Pass Criteria**:
- Console shows `[SCHEDULE] RTC wake (reason=1)` after first DeepSleep
- `schedule_is_cold_boot()` returns `false` after RTC wake (test via breakpoint or additional print)

**Note**: Reason 1 = `POWER_WAKE_REASON_RTC`.

**Source**: `output/modules/2.5_power_manager/interface.h#L208`

---

### UT3: schedule_run_cycle() — Full Cycle (Sensor + RF + Sleep)

**Objective**: Verify one complete measurement → TX → DeepSleep cycle.

**Setup**: Board with AHT10 and UM2005C populated.

**Procedure**:
1. Power cycle board (cold boot)
2. Observe console output for one full cycle
3. Use oscilloscope to measure:
   - I2C activity on PA03/PA04 (AHT10 transactions)
   - TWI activity on PB02/PB03 (UM2005C CLK/DATA)
   - Supply current drop after DeepSleep entry

**Pass Criteria**:
- Console shows:
  ```
  [SCHEDULE] Cold boot (POR)
  [I] sensor_mgr: initialized
  [I] sensor_mgr: avg T=25.4 H=62.3 (samples=2)
  [T=25.4C H=62.3%]
  [SCHEDULE] RF TX OK
  [SCHEDULE] Cycle 1 complete. Sleeping 150 s...
  ```
- I2C bus shows two AHT10 transaction bursts (~80ms apart)
- TWI bus shows 14-byte data transfer after sensor measurement
- Supply current drops to ~2 µA after `power_mgr_sleep_150s()` call

**Source**: `schedule.c#L190-L237`, `sensor_mgr_run`

---

### UT4: schedule_run_cycle() — Sensor Failure Handling

**Objective**: Verify graceful handling of sensor read failures.

**Setup**: Board with AHT10 disconnected or faulty.

**Procedure**:
1. Power cycle board
2. Observe console output

**Pass Criteria**:
- Console shows `[W] sensor_mgr: both samples failed`
- Console shows `[SCHEDULE] sensor read failed (consecutive=N)`
- After 5 consecutive failures, shows `[W] schedule: skipping RF TX`
- System still enters DeepSleep (current drops to ~2 µA)
- On each wake, system retries sensor read

**Edge case**: Even with sensor failures, a packet with error status should be transmitted (if `packet_valid`).

**Source**: `schedule.c#L206-L244`, `output/modules/2.7_sensor_mgr/interface.h#L146`

---

### UT5: schedule_run_cycle() — RF TX Failure Handling

**Objective**: Verify graceful handling of RF transmission failure.

**Setup**: Board with UM2005C disconnected or faulty.

**Procedure**:
1. Power cycle board
2. Observe console output

**Pass Criteria**:
- Console shows `[W] schedule: rf_twi_transmit failed err=X`
- System still enters DeepSleep
- On next wake, system retries sensor read + RF TX

**Source**: `schedule.c#L251-L268`

---

### UT6: IWDT Refresh During Active Cycle

**Objective**: Verify IWDT is refreshed before the active measurement cycle.

**Setup**: Measure IWDT pin or use debugger watchpoint on CW_IWDT->KR write.

**Procedure**:
1. Set breakpoint at `power_mgr_iwdt_refresh()` call in `schedule_run_cycle()`
2. Power cycle board
3. Verify breakpoint hits at start of each cycle

**Pass Criteria**:
- `__IWDT_REFRESH()` macro write (0xAAAA to CW_IWDT->KR) occurs at the start of `schedule_run_cycle()`
- No IWDT reset during the ~280ms active cycle

**Source**: `schedule.c#L196`, `cw32l010_iwdt.h#L94`

---

### UT7: Main init failure — Error Loop

**Objective**: Verify system behavior on critical init failure.

**Setup**: Modify `schedule_init()` to return `ERR_BUS` (e.g., by removing the power supply).

**Procedure**:
1. Flash firmware with intentional init failure
2. Observe behavior

**Pass Criteria**:
- Console shows `[FATAL] Init failed err=-9`
- System enters infinite loop
- IWDT fires after ~2s and resets MCU (POR restart)
- On restart, init may succeed or fail again

**Source**: `schedule.c#L303-L323`

---

## 3. Integration Tests

### IT1: 24-Hour Endurance Test

**Objective**: Verify system stability over extended period.

**Setup**: Board with all components, console logging to file.

**Procedure**:
1. Power board from CR2032 battery
2. Log console output for 24 hours (576 cycles = 150s × 576 / 3600 = 24 hours)
3. After 24h, analyze logs

**Pass Criteria**:
- No unexpected resets (no banner reprint mid-log)
- >99% of cycles show `[SCHEDULE] RF TX OK`
- Battery voltage within CR2032 range at end of test
- Total cycles count ≥ 570 (allow for minor timing drift)

---

### IT2: Power Consumption Measurement

**Objective**: Verify deep sleep current and average power budget.

**Setup**: Bench supply with µA resolution, oscilloscope.

**Procedure**:
1. Connect board to bench supply (3.0V)
2. Measure:
   - DeepSleep current (between cycles)
   - Active cycle current (during sensor read + RF TX)
   - Average current over 5 complete cycles

**Pass Criteria**:
- DeepSleep current ≤ 3 µA (at 3.0V, 25°C)
- Active peak current ≤ 20 mA (sensor read + RF TX)
- Average current ≤ 5 µA (over full 150s cycle)
- Estimated battery life ≥ 3 years (225 mAh / (5 µA × 8760 h)) = ~5.1 years

**Note**: DeepSleep current includes RTC + LSI + IWDT + RAM retention.

---

### IT3: Rapid Power Cycle (100x)

**Objective**: Verify cold boot reliability.

**Setup**: Power supply with fast rise time.

**Procedure**:
1. Power cycle board 100 times (3s on, 1s off)
2. After each power-up, verify boot banner

**Pass Criteria**:
- All 100 power cycles show `[CW32L010 Sensor Node v1.0]`
- No lock-ups or hangs
- Clock frequency consistent (48 MHz ±2%) every boot

---

## 4. HIL (Hardware-in-the-Loop) Test Cases

### HIL1: RF Packet Validation

**Objective**: Verify the transmitted RF packet matches the specification.

**Setup**: Spectrum analyzer + RF receiver capable of capturing UM2005C packets.

**Procedure**:
1. Let system run 10 cycles
2. Capture RF packets on spectrum analyzer
3. Decode and verify packet format

**Verification**:
| Byte Offset | Content | Expected |
|-------------|---------|----------|
| 0 | Preamble | 0xAA |
| 1 | Preamble | 0x55 |
| 2-7 | UID | Lower 48 bits of MCU UID |
| 8 | Temp high | temperature_x10 >> 8 |
| 9 | Temp low | temperature_x10 & 0xFF |
| 10 | Hum high | humidity_x10 >> 8 |
| 11 | Hum low | humidity_x10 & 0xFF |
| 12 | Status | Battery + sensor status bits |
| 13 | CRC8 | MAXIM polynomial 0x31 |

**Pass Criteria**: All 10 packets have correct preamble, valid CRC8, and sensible temperature/humidity values.

**Source**: `output/design/common_types.h#L94-L103`

---

### HIL2: Crystal Startup Timing

**Objective**: Verify UM2005C clock startup timing.

**Setup**: Oscilloscope on PB02 (RF_CLK) and UM2005C pin 1 (XTAL).

**Procedure**:
1. Capture TWI_ON command (32 CLK zeros) on PB02
2. Measure time from TWI_ON to first data transaction

**Pass Criteria**: 
- TWI_ON pulse width ≤ 32 µs (32 cycles × 1 µs at ≤1 MHz CLK)
- First data byte appears within 2 ms of TWI_ON start
- CLK frequency during data phase = 500 kHz ±10%

**Source**: `UM2005C_数据手册_V1.2_V1.2.pdf#page=12`

---

## 5. Test Code Snippet (for CI / simulation)

```c
/* 
 * Simple test harness for schedule module.
 * To be compiled with mock implementations of sensor_mgr, rf_twi, power_mgr.
 */
#include "output/modules/2.8_schedule/interface.h"
#include <stdio.h>
#include <assert.h>

int main(void)
{
    int32_t ret;

    /* Test schedule_init() */
    ret = schedule_init();
    assert(ret == ERR_OK);
    assert(schedule_get_cycle_count() == 0);
    assert(schedule_get_failure_count() == 0);

    /* Test schedule_is_cold_boot() */
    bool cold = schedule_is_cold_boot();
    printf("Cold boot: %s\n", cold ? "yes" : "no");

    /* Test schedule_run_cycle() */
    ret = schedule_run_cycle();
    assert(ret == ERR_OK);
    assert(schedule_get_cycle_count() == 1);

    /* Test schedule_reset_counters() */
    schedule_reset_counters();
    assert(schedule_get_cycle_count() == 0);
    assert(schedule_get_failure_count() == 0);

    printf("All schedule tests PASSED.\n");
    return 0;
}
```

---

## 6. Pass / Fail Criteria Summary

| Test ID | Name | Pass Criteria |
|---------|------|--------------|
| UT1 | Cold boot clock | 48 MHz HSIOSC, boot banner |
| UT2 | Warm boot detection | RTC wake reason detected |
| UT3 | Full cycle | Sensor → TX → DeepSleep |
| UT4 | Sensor failure | Graceful degradation, sleep |
| UT5 | RF failure | Logged error, continue sleep |
| UT6 | IWDT refresh | Refresh before active cycle |
| UT7 | Init failure | Error loop, IWDT reset |
| IT1 | 24-hour endurance | ≥570 cycles, no hangs |
| IT2 | Power consumption | ≤5 µA avg, ≤3 µA sleep |
| IT3 | Power cycle 100x | 100/100 boot success |
| HIL1 | RF packet | Valid preamble, CRC8 |
| HIL2 | Crystal timing | ≤2ms startup, 500kHz CLK |
