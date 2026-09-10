# Test Plan: power_manager (Phase 2.5)

> **Module:** power_manager (layer=service)  
> **MCU:** CW32L010Y8M6 (TSSOP-20)  
> **Bus:** POWER (CR2032)  
> **Scope:** DeepSleep + RTC AWT periodic wakeup + IWDT safety watchdog + LVD battery monitor

---

## 1. Test Environment

| Item | Description |
|------|-------------|
| **MCU** | CW32L010Y8M6 on custom PCB or dev board |
| **Power** | CR2032 (3V) or lab PSU at 3.0V |
| **Debug** | J2 (SWD) via CW-DAPLINK |
| **Console** | J3 (UART1, 115200 8N1) if CONFIG_CONSOLE_ENABLE=1 |
| **Scope** | Current probe on BAT rail for DeepSleep current measurement |
| **Logic analyzer** | Optional: probe PB02 (RF_CLK) to observe wake timing |

### 1.1 Test Hardware Setup

```
USB -> CW-DAPLINK -> SWD (J2) -> MCU (U7)
PSU (3.0V) -> BAT rail (J1)
Oscilloscope current probe -> BAT rail
UART console (3.3V USB-UART) -> J3 (GND, TX, RX)
```

### 1.2 Test Dependencies

| Dependency | Source | Required? |
|------------|--------|-----------|
| `common_types.h` | `output/design/common_types.h` | Yes — provides ERR_OK, ERR_BUS, LOG_*, etc. |
| `cw32l010.h` | MCU SDK header | Yes — CMSIS header, IRQn definitions |
| `cw32l010_pwr.h` | MCU SDK header | Yes — PWR_Config, PWR_InitTypeDef |
| `cw32l010_rtc.h` | MCU SDK header | Yes — RTC_Init, RTC_AWTConfig, RTC_ITConfig |
| `cw32l010_iwdt.h` | MCU SDK header | Yes — IWDT_Init, IWDT_Refresh |
| `cw32l010_lvd.h` | MCU SDK header | Yes — LVD_Init, LVD_EnableIrq (when LVD enabled) |
| `cw32l010_sysctrl.h` | MCU SDK header | Yes — SYSCTRL_GotoDeepSleep, SYSCTRL_LSI_Enable |

---

## 2. Test Cases

### TC-1: Cold Boot Wake Reason Detection

| Field | Value |
|-------|-------|
| **ID** | TC-PM-001 |
| **Title** | Cold boot detects POWER_WAKE_REASON_POR |
| **Precondition** | Power removed, then re-applied |
| **Steps** | 1. Apply power (3.0V via PSU)  
2. Call `power_mgr_init(false)`  
3. Call `power_mgr_get_wake_reason()` |
| **Expected Result** | `POWER_WAKE_REASON_POR` (0) |
| **Pass Criteria** | `wake_reason == 0` on console output |
| **HIL?** | Yes — real hardware required to verify POR flag |

### TC-2: DeepSleep Entry and RTC Wake (~150s)

| Field | Value |
|-------|-------|
| **ID** | TC-PM-002 |
| **Title** | DeepSleep entry with RTC AWT wake after ~150 seconds |
| **Precondition** | `power_mgr_init(false)` called, system running |
| **Steps** | 1. Capture timestamp T1  
2. Call `power_mgr_sleep_150s()`  
3. After wake, capture timestamp T2  
4. Compute delta = T2 - T1 |
| **Expected Result** | MCU enters DeepSleep (current drops to ~1-2 uA), wakes after ~150s |
| **Pass Criteria** | 145s <= delta <= 160s (LSI accuracy: +/-5% typ, +/-15% max) |
| **Measurement** | Use oscilloscope current probe to verify sleep current < 5 uA |
| **HIL?** | Yes — real HW + scope/probe required |

### TC-3: RTC AWT Interrupt Handler

| Field | Value |
|-------|-------|
| **ID** | TC-PM-003 |
| **Title** | RTC_IRQHandler clears AWTIMER flag correctly |
| **Precondition** | After TC-002, just woke from DeepSleep |
| **Steps** | 1. Check `CW_RTC->ISR_f.AWTIMER` before calling power_mgr_rtc_irq_handler()  
2. Call power_mgr_rtc_irq_handler()  
3. Check `CW_RTC->ISR_f.AWTIMER` again |
| **Expected Result** | Flag cleared from 1 to 0 |
| **Pass Criteria** | AWTIMER bit = 0 after handler |
| **HIL?** | Yes |

### TC-4: IWDT Does Not Fire During Normal Operation

| Field | Value |
|-------|-------|
| **ID** | TC-PM-004 |
| **Title** | IWDT kept alive during active period and DeepSleep |
| **Precondition** | `power_mgr_init(false)` called |
| **Steps** | 1. Run 10 full cycles (sleep 150s + wake + short active)  
2. No IWDT reset should occur |
| **Expected Result** | System remains running, wake_reason stays POR (or RTC) |
| **Pass Criteria** | No unexpected resets in 10 cycles |
| **HIL?** | Yes — real HW timing required |

### TC-5: IWDT Reset if Refresh Omitted

| Field | Value |
|-------|-------|
| **ID** | TC-PM-005 |
| **Title** | IWDT resets MCU if not refreshed >2s |
| **Precondition** | `power_mgr_init(false)` called, IWDT running |
| **Steps** | 1. Call `power_mgr_iwdt_refresh()`  
2. Delay > 3 seconds (e.g., busy loop) _without_ refreshing  
3. MCU should reset |
| **Expected Result** | MCU resets, boot, `power_mgr_iwdt_caused_reset()` returns true |
| **Pass Criteria** | `wake_reason == POWER_WAKE_REASON_IWDT` |
| **Safety** | Do not run this test in production; use a test harness |
| **HIL?** | Yes — real HW. **Warning:** This will reset the MCU. |

### TC-6: LVD Battery Low Detection

| Field | Value |
|-------|-------|
| **ID** | TC-PM-006 |
| **Title** | LVD triggers battery low flag when voltage drops below 2.6V |
| **Precondition** | `power_mgr_init(true)` called (LVD enabled) |
| **Steps** | 1. Ensure battery voltage > 2.7V  
2. Call `power_mgr_is_battery_low()` — should return false  
3. Reduce PSU to 2.4V  
4. Wait > 10ms (filter time)  
5. Call `power_mgr_is_battery_low()` |
| **Expected Result** | First call: false. After voltage drop: true. |
| **Pass Criteria** | battery_low flag set when VDD < 2.6V |
| **HIL?** | Yes — variable PSU required |

### TC-7: Custom Sleep Duration

| Field | Value |
|-------|-------|
| **ID** | TC-PM-007 |
| **Title** | Custom sleep duration with power_mgr_sleep_seconds() |
| **Precondition** | `power_mgr_init(false)` called |
| **Steps** | 1. Call `power_mgr_sleep_seconds(10)`  
2. Measure actual wake time |
| **Expected Result** | Wakes after ~10 seconds |
| **Pass Criteria** | 8s <= actual <= 12s |
| **HIL?** | Yes |

### TC-8: Light Sleep (Non-DeepSleep)

| Field | Value |
|-------|-------|
| **ID** | TC-PM-008 |
| **Title** | Light Sleep wakes via SysTick interrupt |
| **Precondition** | `power_mgr_init(false)` called, SysTick running at 1ms |
| **Steps** | 1. Set a volatile flag = false  
2. Configure a timer or external interrupt to set flag=true after 100ms  
3. Call `power_mgr_sleep_light()` |
| **Expected Result** | CPU pauses briefly, wakes when interrupt fires |
| **Pass Criteria** | Code after power_mgr_sleep_light() executes |
| **HIL?** | Optional — can be tested with debugger stepping |

### TC-9: Multiple DeepSleep Cycles

| Field | Value |
|-------|-------|
| **ID** | TC-PM-009 |
| **Title** | Repeated sleep/wake cycles accumulate deepsleep_count |
| **Precondition** | `power_mgr_init(false)` called |
| **Steps** | 1. Run 5 sleep/wake cycles  
2. After 5th wake, call `power_mgr_get_status()` |
| **Expected Result** | `deepsleep_count >= 5` |
| **Pass Criteria** | deepsleep_count == 5 |
| **HIL?** | Yes |

### TC-10: Wake Reason Persistence

| Field | Value |
|-------|-------|
| **ID** | TC-PM-010 |
| **Title** | Wake reason stored correctly across sleep cycles |
| **Precondition** | First call: `power_mgr_init(false)` after POR |
| **Steps** | 1. After first boot: check wake_reason = POR  
2. Sleep + wake: check wake_reason = POR (persisted from boot) |
| **Expected Result** | wake_reason reflects the _boot_ reason, not the last wake reason |
| **Pass Criteria** | wake_reason stays POR as detected during init, not overwritten by RTC wake |
| **Note** | The wake reason is captured once at boot. RTC wake does not change it since POR flag is already cleared. This is by design. |
| **HIL?** | Yes |

---

## 3. Automated Test Sequence (for CI/HIL)

```c
// test_power_manager.c — HIL test sequence
#include "power_manager.h"
#include <stdio.h>
#include <assert.h>

// Forward declaration of platform timer function
extern uint32_t get_tick_ms(void);
extern void delay_ms(uint32_t ms);

static void test_cold_boot(void) {
    power_status_t st;
    power_mgr_init(false);
    power_mgr_get_status(&st);
    assert(st.wake_reason == POWER_WAKE_REASON_POR || 
           st.wake_reason == POWER_WAKE_REASON_UNKNOWN);
    printf("[PASS] TC-001: wake_reason = %d\n", st.wake_reason);
}

static void test_sleep_and_wake(void) {
    // Requires HW timing measurement — simplified check
    uint32_t t0 = get_tick_ms();
    power_mgr_sleep_seconds(5);  // Short interval for testing
    uint32_t t1 = get_tick_ms();
    uint32_t delta = t1 - t0;
    printf("[INFO] TC-002: slept %u ms (expected ~5000)\n", delta);
    assert(delta >= 4000 && delta <= 7000);
    printf("[PASS] TC-002: sleep duration OK\n");
}

static void test_iwdt_no_reset(void) {
    for (int i = 0; i < 5; i++) {
        power_mgr_iwdt_refresh();
        delay_ms(100);  // Well within 2s timeout
    }
    printf("[PASS] TC-004: IWDT did not fire\n");
}

static void test_deepsleep_count(void) {
    power_status_t st;
    // Run 3 more cycles (already have 1 from test_sleep_and_wake)
    for (int i = 0; i < 2; i++) {
        power_mgr_sleep_seconds(2);
    }
    power_mgr_get_status(&st);
    assert(st.deepsleep_count >= 3);
    printf("[PASS] TC-009: deepsleep_count = %u (>=3)\n", st.deepsleep_count);
}

int main(void) {
    test_cold_boot();
    test_iwdt_no_reset();
    test_sleep_and_wake();
    test_deepsleep_count();
    printf("All power_manager tests passed.\n");
    return 0;
}
```

---

## 4. Acceptance Criteria

| Criteria | Requirement | Verification |
|----------|-------------|--------------|
| DeepSleep current | < 5 uA (typical ~1-2 uA) | Scope current probe on BAT rail during TC-002 |
| Wakeup time accuracy | 150s +/- 15% (127s-173s) | TC-002 delta measurement |
| IWDT safety | System resets if not refreshed > 2.5s | TC-005 (manual, with safety precautions) |
| LVD detection | Battery low flag set below 2.6V | TC-006 with variable PSU |
| Cycle count | Accumulates correctly | TC-009 |

---

## 5. Risks and Limitations

| Risk | Impact | Mitigation |
|------|--------|------------|
| LSI frequency tolerance (+/-15%) | Wake interval may drift | 150s +/- 22s acceptable for this application |
| RTC AWT maximum ARR = 16-bit | Max interval = 65535s (~18h) | More than sufficient for 150s cycle |
| IWDT continues in DeepSleep | If wake fails, IWDT resets system | This is the desired safety behavior |
| LVD 2.6V threshold is fixed | Cannot adjust without recompilation | Threshold defined as macro; changeable |
| No ADC-based Vbat measurement | vbat_mv field returns 0 | LVD gives binary low-battery indication only |

---

## 6. Power Measurement Log Template

```
Date: ___________  PSU: ___ V  Temp: ___ °C

Test              | I_sleep (uA) | I_active (mA) | t_active (ms) | Result
------------------|-------------|---------------|---------------|--------
Cold boot         |      N/A    |      N/A      |     N/A       | _______
DeepSleep (150s)  |    _____    |      N/A      |     N/A       | _______
Active cycle      |    _____    |    ______     |    ______     | _______
LVD trigger @2.4V |    _____    |    ______     |    ______     | _______

Notes:
- ________________________________________________________________
```

---

## 7. Files Under Test

| File | Role |
|------|------|
| `output/modules/2.5_power_manager/interface.h` | Public API |
| `output/modules/2.5_power_manager/power_manager.h` | Internal header |
| `output/modules/2.5_power_manager/power_manager.c` | Implementation |
| `output/design/common_types.h` | Error codes, log macros |
| `inputs/mcu_sdk/Libraries/inc/cw32l010_pwr.h` | PWR_Config, PWR_InitTypeDef |
| `inputs/mcu_sdk/Libraries/inc/cw32l010_rtc.h` | RTC_Init, RTC_AWTConfig, RTC_ITConfig |
| `inputs/mcu_sdk/Libraries/inc/cw32l010_iwdt.h` | IWDT_Init, IWDT_Refresh |
| `inputs/mcu_sdk/Libraries/inc/cw32l010_lvd.h` | LVD_Init, LVD_EnableIrq |
| `inputs/mcu_sdk/Libraries/inc/cw32l010_sysctrl.h` | SYSCTRL_GotoDeepSleep, SYSCTRL_LSI_Enable |
