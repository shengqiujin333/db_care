# Phase 2.8 — Schedule Module Protocol Notes

## 1. System-Level Protocol

The schedule module does not implement its own data protocol — it orchestrates the system-level execution cycle. The data protocol definition resides in:

| Protocol | Module | File |
|----------|--------|------|
| RF Packet Format (14-byte) | packet_builder (2.6) | `output/modules/2.6_packet_builder/protocol_notes.md` |
| AHT10 I2C Commands | aht10_driver (2.2) | `output/modules/2.2_aht10_driver/protocol_notes.md` |
| UM2005C TWI Protocol | rf_twi_driver (2.4) | `output/modules/2.4_rf_twi_driver/protocol_notes.md` |

This document describes the **system-level timing protocol**: the sequencing, timing, and state transitions of the 150-second measurement cycle.

---

## 2. Timing Protocol — 150-Second Measurement Cycle

### 2.1 Cycle State Machine

```
                    ┌─────────────┐
                    │  DeepSleep  │ ←──────┐
                    │  (~150 s)   │        │
                    └──────┬──────┘        │
                           │ RTC AWT fire  │
                           ▼               │
                    ┌─────────────┐        │
               ┌───→│  WAKE       │        │
               │    │  (4 µs)     │        │
               │    └──────┬──────┘        │
               │           ▼               │
               │    ┌─────────────┐        │
               │    │  SAMPLE #1  │        │
               │    │  (~80 ms)   │        │
               │    └──────┬──────┘        │
               │           ▼               │
               │    ┌─────────────┐        │
               │    │  WAIT       │        │
               │    │  (100 ms)   │        │
               │    └──────┬──────┘        │
               │           ▼               │
               │    ┌─────────────┐        │
               │    │  SAMPLE #2  │        │
               │    │  (~80 ms)   │        │
               │    └──────┬──────┘        │
               │           ▼               │
               │    ┌─────────────┐        │
               │    │  AVERAGE    │        │
               │    │  (~1 ms)    │        │
               │    └──────┬──────┘        │
               │           ▼               │
               │    ┌─────────────┐        │
               │    │  RF TX      │        │
               │    │  (~20 ms)   │        │
               │    └──────┬──────┘        │
               │           ▼               │
               │    ┌─────────────┐        │
               │    │  CONSOLE    │        │
               │    │  (~5 ms)    │        │
               │    └──────┬──────┘        │
               │           ▼               │
               │    ┌─────────────┐        │
               │    │  DEEPSLEEP  │────────┘
               │    │  (setup)    │
               │    └─────────────┘
               │
               └─── Error: retry on next cycle
```

### 2.2 Timing Budget

| Phase | Duration | Cumulative | Notes |
|-------|----------|------------|-------|
| DeepSleep wake | ~4 µs | 4 µs | HSIOSC startup |
| Sensor boot delay (cold boot only) | 50 ms | 50 ms | AHT10 power-up time |
| Sample #1 | ~80 ms | ~80 ms | I2C trigger → 75ms wait → read 6 bytes |
| Inter-sample gap | 100 ms | ~180 ms | Short delay between samples |
| Sample #2 | ~80 ms | ~260 ms | Same as sample #1 |
| Averaging + packet build | ~1 ms | ~261 ms | Integer math + UID lookup |
| RF TX (TWI) | ~20 ms | ~281 ms | TWI_ON → write 14 bytes → TWI_OFF |
| Console print | ~5 ms | ~286 ms | UART printf (blocking, ~46 chars) |
| DeepSleep entry | ~1 ms | ~287 ms | AWT config, IWDT refresh, WFI |
| **Total active** | **~287 ms** | — | **~0.19% duty cycle** |
| DeepSleep | ~149.713 s | ~150 s | RTC AWT counting down from 150 |

### 2.3 DeepSleep Entry Sequence (from schedule_run_cycle)

```
1. power_mgr_iwdt_refresh()         ← Pet watchdog for active work
2. sensor_mgr_run()                 ← Blocking ~260ms
3. rf_twi_transmit()                ← Blocking ~20ms
4. CONSOLE_PRINT()                  ← Blocking ~5ms
5. power_mgr_sleep_150s():          ← Blocking ~150s
   a. RTC_AWTConfig(150)            ← Set AWT reload = 150 (1 Hz tick)
   b. RTC_ITConfig(AWTIMER, ENABLE) ← Enable AWT interrupt
   c. PWR_Config(SLEEPDEEP=1)       ← Set SLEEPDEEP in SCB_SCR
   d. __IWDT_REFRESH()              ← Final pet before sleep
   e. SYSCTRL_GotoDeepSleep()       ← __WFI() → enter DeepSleep
   f. [RTC AWT fires after 150s]    
   g. RTC_IRQHandler()              ← ISR clears AWTIMER flag
   h. Return from WFI               ← Execution resumes here
6. Back to step 1                   ← Next cycle
```

### 2.4 Wake Sources

| Source | Trigger | Wake Time | Usage |
|--------|---------|-----------|-------|
| RTC AWT | Auto-Wake Timer underflow | ~4 µs | Normal 150s periodic wakeup |
| External NRST | Pin 4 (PB07) pulled low | ~4 µs | Manual reset / debug |
| IWDT | Watchdog timeout | Full reset | Safety — system hang recovery |

---

## 3. Error Handling Protocol

### 3.1 Sensor Failure Recovery

| Consecutive Failures | Action |
|----------------------|--------|
| 1-4 | Transmit packet with error status bits set |
| ≥5 | Skip RF TX to save battery; still enter DeepSleep |
| After any success | Reset failure counter to 0 |

### 3.2 RF Transmission Failure

- Logged to console (if enabled)
- System still enters DeepSleep normally
- No retry within the same cycle (time budget exhausted)
- Next cycle will retry sensor read + RF TX fresh

### 3.3 Critical Init Failure

- Console shows `[FATAL] Init failed err=X`
- System enters infinite error loop
- IWDT fires after ~2s timeout (IWDT configured with IWDT_OVERFLOW_ACTION_RESET)
- MCU resets and attempts cold boot again

---

## 4. Boot Protocol

### 4.1 Cold Boot Sequence

```
POR (Power-On Reset) or any full reset:
  1. SystemInit() — default CMSIS vector table + clock setup
  2. HSIOSC enable (48 MHz) — SYSCTRL_HSI_Enable(0)
  3. Wait for HSIOSC stable flag
  4. Switch SYSCLKSRC to HSI
  5. Configure HCLK = SYSCLK, PCLK = HCLK
  6. SystemCoreClockUpdate()
  7. InitTick(48000000) — SysTick @ 1 kHz
  8. power_mgr_init(true) — LSI, RTC, IWDT, LVD
  9. uart_console_init(48000000) — UART1 @ 115200
  10. rf_twi_init() — PB02/PB03 as TWI GPIO
  11. sensor_mgr_init() — AHT10 + pkt_builder
  12. Delay 50ms (AHT10 power-up)
  13. Enter main loop
```

### 4.2 Warm Boot (RTC Wake) Sequence

```
DeepSleep exit:
  1. RTC AWT underflow → RTC_IRQn
  2. RTC_IRQHandler clears AWTIMER flag
  3. Return from WFI → power_mgr_sleep_150s() returns
  4. Back to main loop top
  5. power_mgr_iwdt_refresh()
  6. sensor_mgr_run()...
```

**Note**: On warm boot, the system clock (HSIOSC) is **not** fully re-initialized — it was stopped in DeepSleep and restarted by the RTC wake sequence. The power_manager handles LSI/RTC state preservation.

---

## 5. Compile-Time Configuration

| Macro | Default | Effect |
|-------|---------|--------|
| `CONFIG_CONSOLE_ENABLE` | `1` | Set to 0 to disable all UART output |
| `LOG_LEVEL` | `LOG_LEVEL_INFO` | Compile-time log verbosity |
| `SCHEDULE_WAKE_INTERVAL_S` | `150` | DeepSleep duration in seconds |
| `SCHEDULE_MAX_FAILURES_BEFORE_SKIP` | `5` | Consecutive sensor failures before TX skip |

---

## 6. Source References

| Component | Source |
|-----------|--------|
| RTC AWT configuration | `inputs/mcu_sdk/Libraries/inc/cw32l010_rtc.h#L476` (RTC_AWTConfig) |
| DeepSleep entry | `inputs/mcu_sdk/Libraries/inc/cw32l010_sysctrl.h#L491` (SYSCTRL_GotoDeepSleep) |
| PWR configuration | `inputs/mcu_sdk/Libraries/inc/cw32l010_pwr.h#L104` (PWR_Config) |
| IWDT refresh | `inputs/mcu_sdk/Libraries/inc/cw32l010_iwdt.h#L94` (__IWDT_REFRESH) |
| HSIOSC enable | `inputs/mcu_sdk/Libraries/inc/cw32l010_sysctrl.h#L458` (SYSCTRL_HSI_Enable) |
| SysTick init | `inputs/mcu_sdk/Libraries/inc/cw32l010_systick.h#L58` (InitTick) |
| Power manager API | `output/modules/2.5_power_manager/interface.h` |
| Sensor manager API | `output/modules/2.7_sensor_mgr/interface.h` |
| RF TWI driver API | `output/modules/2.4_rf_twi_driver/interface.h` |
| System architecture | `output/design/COMMON.md#1.2` |
