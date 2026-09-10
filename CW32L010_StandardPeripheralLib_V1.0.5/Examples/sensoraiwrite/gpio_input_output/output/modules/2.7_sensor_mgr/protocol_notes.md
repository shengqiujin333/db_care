# Protocol Notes: Phase 2.7 — sensor_mgr

> The sensor_mgr module does NOT implement its own hardware protocol. It is an application-layer orchestrator that uses existing protocol drivers (I2C via aht10_driver, RF packet assembly via packet_builder).

## 1. Role in Protocol Stack

```
AHT10 sensor  <--I2C-->  aht10_driver (2.2)  <--api-->  sensor_mgr (2.7)
                                                               |
                                                    RF packet assembly via pkt_build()
                                                               |
                                                    rf_twi_driver (2.4) --> UM2005C TX
```

## 2. Data Averaging Protocol

### 2.1 Sample Strategy

The sensor_mgr takes **two** AHT10 measurements per 150s cycle, spaced ~100ms apart.

**Rationale**: Short-term fluctuations in temperature/humidity (e.g., from local air currents, sensor self-heating) are smoothed by averaging two quick successive readings. The 100ms gap is sufficient for the AHT10 to complete one measurement cycle (75ms) before starting the next.

### 2.2 Averaging Algorithm

- **Arithmetic mean**: `(sample1 + sample2) / 2`
- **Integer division**: Values are in 0.1°C / 0.1% RH fixed-point; integer arithmetic (truncation toward zero)
- **Valid samples**: Only samples with `SENSOR_STATUS_VALID` (0x01) flag set are included
- **Partial averaging**: If one sample fails, the other is used alone (marked as `SENSOR_STATUS_STALE`)
- **Complete failure**: If both samples fail, `ERR_READ_FAIL` is returned

### 2.3 Data Validation

Each sample is validated against plausible ranges:

| Parameter | Min | Max | Unit |
|-----------|-----|-----|------|
| Temperature | -40.0 (-400) | +85.0 (+850) | °C (×10) |
| Humidity | 0.0 (0) | 100.0 (1000) | % RH (×10) |

Values outside these ranges are treated as read failures.

---

## 3. Failure Recovery Protocol

### 3.1 Consecutive Failure Tracking

| Counter | Reset Condition | Purpose |
|---------|----------------|---------|
| consecutive_failures | Any successful measurement | Track degrading HW |
| total_cycles | power_mgr_init() | Lifetime operation counter |
| failed_cycles | power_mgr_init() | Measurement reliability metric |
| stale_cycles | power_mgr_init() | Partial data availability metric |

### 3.2 Behavior on Failure

| Scenario | Action | Return Code | Status Flags |
|----------|--------|-------------|--------------|
| Sample 1 fail, Sample 2 OK | Use sample 2 alone | ERR_OK | VALID + STALE |
| Sample 1 OK, Sample 2 fail | Use sample 1 alone | ERR_OK | VALID + STALE |
| Both fail | Build packet with fail status | ERR_READ_FAIL | READ_FAIL |
| Packet builder fail | Skip packet, return data | ERR_OK | VALID (data only) |

---

## 4. Console Print Protocol

### 4.1 Output Format

When `CONFIG_CONSOLE_ENABLE == 1`:

```
[T=25.4C H=62.3%]
```

Format string: `"[T=%d.%dC H=%u.%u%%]\r\n"`

- Temperature: integer part + one decimal place
- Humidity: integer part + one decimal place
- Terminated with CRLF for terminal compatibility

### 4.2 Compile-Time Toggle

Per user req #1 ("打印做成灵活的，可以随时关闭后编译，为了省电"):

```c
// Set CONFIG_CONSOLE_ENABLE = 0 in board.h to disable all UART output
#define CONFIG_CONSOLE_ENABLE  0
```

When disabled:
- `CONSOLE_PRINT` expands to `((void)0)` — zero code footprint
- UART1 peripheral can be fully disabled by uart_console driver
- No power consumed by UART during measurement

---

## 5. Timing Budget

| Phase | Duration (ms) | Cumulative (ms) |
|-------|---------------|-----------------|
| Sample #1 (trigger + 75ms wait + read + convert) | ~80 | ~80 |
| Inter-sample delay | ~100 | ~180 |
| Sample #2 (trigger + 75ms wait + read + convert) | ~80 | ~260 |
| Averaging + packet build + console | <5 | ~265 |
| **Total active time** | **~265ms** | |

Within a 150-second cycle, the active duty cycle is ~0.18%.

---

## 6. Status Byte Mapping (for RF Packet)

The `sys_status` byte in the RF packet (`rf_packet_t.sys_status`) is built by `pkt_build()` using these flags:

| Bit | Flag | Source | Meaning |
|-----|------|--------|---------|
| 0 | `SYS_STATUS_BAT_LOW` | `power_mgr_is_battery_low()` | 1 = CR2032 voltage below ~2.6V |
| 1 | `SYS_STATUS_SENSOR_OK` | sensor_data_t.status has VALID | 1 = at least one valid sample |
| 2 | `SYS_STATUS_RF_TXDONE` | Set by rf_mgr after TX | 1 = RF transmission completed |

Status byte is computed by `pkt_status_byte()` in packet_builder.  
Source: `output/modules/2.6_packet_builder/interface.h#L217`
