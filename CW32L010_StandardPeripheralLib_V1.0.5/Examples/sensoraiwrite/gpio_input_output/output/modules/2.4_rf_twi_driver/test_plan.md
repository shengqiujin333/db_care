# Test Plan: rf_twi_driver (Phase 2.4)

> Module: rf_twi_driver -- Bit-banged TWI driver for UM2005C RF transmitter
> MCU: CW32L010Y8M6, Pins: PB02(CLK), PB03(DATA)
> Instance bus: GPIO_IN_INTERRUPT (connectivity.json)
> Sources: UM2005C_数据手册_V1.2_V1.2.pdf, cw32l010_gpio.h

---

## 1. Scope

### 1.1 In Scope

- GPIO initialization for PB02 (CLK) and PB03 (DATA) as digital push-pull output
- TWI_ON command: 32 CLK cycles with DATA=0
- TWI_OFF command: 0xFF02 special command
- SOFT_RST command: 0xFF04 special command
- TWI_RST command: 0xFF01 special command
- TWI write transaction: 16-cycle, W/R=1, address + data byte
- TWI read transaction: 16-cycle, W/R=0, address + receive data byte
- Multi-byte write (rf_twi_write_buf)
- Full RF transmission sequence (rf_twi_transmit)
- LBD read (rf_twi_read_lbd)
- De-initialization (pins to input/high-Z)
- Timing: CLK period ~1us (1MHz), tCH/tCL ~ 500ns each
- NOP-based microsecond and millisecond delays

### 1.2 Out of Scope

- OTP programming (requires UM2005C programmer tool)
- Register map configuration (depends on UM2005C SDK, not publicly documented)
- RF output power verification (requires spectrum analyzer)
- Antenna matching verification (hardware design)
- Crystal frequency accuracy measurement (requires frequency counter)

---

## 2. Test Environment

### 2.1 Hardware Required

| Item | Description | Quantity |
|------|-------------|----------|
| CW32L010 EVB or custom board | With UM2005C (U4) populated | 1 |
| USB-to-UART adapter | For debug console output | 1 |
| Oscilloscope | 4-channel, >=50MHz bandwidth, for CLK/DATA timing | 1 |
| Logic analyzer | Optional, for TWI protocol decode | 1 |
| Spectrum analyzer | Optional, for RF output verification | 1 |
| Multimeter | For voltage measurements | 1 |
| CR2032 battery | 3V nominal power | 1 |

### 2.2 Software Required

| Tool | Purpose |
|------|---------|
| ARM GCC / Keil MDK | Compile firmware |
| CW-DAPLINK or J-Link | Flash and debug |
| Serial terminal (115200 8N1) | Console output |
| Python 3 (optional) | For CRC calculation verification |

### 2.3 Test Configurations

```c
// In build configuration:
#define LOG_LEVEL           LOG_LEVEL_DEBUG   // Enable all logging
#define CONFIG_CONSOLE_ENABLE   1              // Enable UART output
```

---

## 3. Test Cases

### 3.1 Unit Test: GPIO Initialization

| Test ID | UT-01 |
|---------|-------|
| **Title** | GPIO init configures PB02/PB03 correctly |
| **Preconditions** | MCU running at 48MHz HSIOSC |
| **Steps** | 1. Call `rf_twi_init()`<br>2. Read GPIOB DIR register for PB02, PB03<br>3. Read GPIOB OPENDRAIN register<br>4. Read GPIOB ANALOG register<br>5. Read GPIOB PUR register |
| **Expected** | PB02_DIR=0 (output), PB03_DIR=0 (output)<br>PB02_OPENDRAIN=0 (push-pull), PB03_OPENDRAIN=0 (push-pull)<br>PB02_ANALOG=0 (digital), PB03_ANALOG=0 (digital)<br>PB02_PUR=0 (no pull-up), PB03_PUR=0 (no pull-up)<br>CLK=LOW, DATA=LOW |
| **Pass Criteria** | All register values match expected |
| **Source** | cw32l010_gpio.h macros |

### 3.2 Unit Test: TWI ON Command

| Test ID | UT-02 |
|---------|-------|
| **Title** | TWI_ON generates 32 CLK cycles with DATA=0 |
| **Preconditions** | rf_twi_init() called |
| **Steps** | 1. Connect oscilloscope to PB02(CLK) and PB03(DATA)<br>2. Call `rf_twi_on()`<br>3. Capture waveform |
| **Expected** | 32 CLK pulses observed on PB02<br>DATA (PB03) remains LOW throughout all 32 cycles<br>CLK period ~ 1us (500ns high, 500ns low)<br>After 32 cycles, CLK=LOW, DATA=LOW |
| **Pass Criteria** | CLK count = 32, DATA = 0 throughout |
| **Source** | UM2005C_数据手册_V1.2_V1.2.pdf#page=12 |

### 3.3 Unit Test: TWI Write Transaction

| Test ID | UT-03 |
|---------|-------|
| **Title** | TWI write sends correct address byte + data byte (16 cycles) |
| **Preconditions** | rf_twi_on() called |
| **Steps** | 1. Call `rf_twi_write(0x15, 0xA5)`<br>2. Capture CLK and DATA on oscilloscope<br>3. Decode the 16 CLK cycles |
| **Expected** | Cycles 1-8 (address): DATA = [1][0][1][0][1][0][1][0] = 0xAB<br>  - Cycle 1: W/R=1 (write)<br>  - Cycles 2-7: A[5:0] = 010101 = 0x15<br>  - Cycle 8: reserved = 0<br>Cycles 9-16 (data): DATA = [1][0][1][0][0][1][0][1] = 0xA5<br>After: CLK=LOW, DATA=LOW |
| **Pass Criteria** | Full 16 cycles with correct bit pattern |
| **Source** | UM2005C_数据手册_V1.2_V1.2.pdf#page=12 |

### 3.4 Unit Test: TWI Read Transaction

| Test ID | UT-04 |
|---------|-------|
| **Title** | TWI read receives byte correctly |
| **Preconditions** | rf_twi_on() called, UM2005C powered |
| **Steps** | 1. Call `rf_twi_read(0x3E, &value)` (read LBD register)<br>2. Capture CLK and DATA on oscilloscope<br>3. Verify received byte |
| **Expected** | Cycles 1-8: MCU drives address with W/R=0<br>  - Cycle 1: W/R=0 (read)<br>  - Cycles 2-7: A[5:0] = 111110 = 0x3E<br>  - Cycle 8: reserved = 0<br>Cycles 9-16: UM2005C drives DATA, MCU samples on falling edge<br>After: CLK=LOW, DATA=LOW (MCU re-asserts) |
| **Pass Criteria** | Read completes without error, LBD value returned |
| **Source** | UM2005C_数据手册_V1.2_V1.2.pdf#page=12 |

### 3.5 Unit Test: TWI OFF Command

| Test ID | UT-05 |
|---------|-------|
| **Title** | TWI_OFF sends 0xFF02 command |
| **Preconditions** | rf_twi_on() called |
| **Steps** | 1. Call `rf_twi_off()`<br>2. Capture waveform |
| **Expected** | Address byte: [1][1][1][1][1][1][1][0] = 0xFE (W/R=1, A=0x3F)<br>Data byte: [0][0][0][0][0][0][1][0] = 0x02<br>After: CLK=LOW, DATA=LOW |
| **Pass Criteria** | Correct bit pattern for 0xFF02 command |
| **Source** | UM2005C_数据手册_V1.2_V1.2.pdf#page=12 |

### 3.6 Unit Test: SOFT_RST Command

| Test ID | UT-06 |
|---------|-------|
| **Title** | SOFT_RST sends 0xFF04 command |
| **Preconditions** | rf_twi_init() called |
| **Steps** | 1. Call `rf_twi_soft_reset()`<br>2. Capture waveform |
| **Expected** | Address byte: 0xFE, Data byte: 0x04 |
| **Pass Criteria** | Correct command pattern |
| **Source** | UM2005C_数据手册_V1.2_V1.2.pdf#page=12 |

### 3.7 Unit Test: Multi-byte Write Buffer

| Test ID | UT-07 |
|---------|-------|
| **Title** | Write buffer sends all bytes sequentially |
| **Preconditions** | rf_twi_on() called |
| **Steps** | 1. Create test buffer: `{0xAA, 0x55, 0x01, 0x02, 0x03}`<br>2. Call `rf_twi_write_buf(0x00, buf, 5)`<br>3. Capture waveform |
| **Expected** | 5 TWI write transactions executed sequentially<br>Each transaction: 16 CLK cycles<br>Each data byte matches buffer content |
| **Pass Criteria** | All 5 bytes written correctly |
| **Source** | rf_twi_driver.c (implementation) |

### 3.8 Integration Test: Full RF Transmission

| Test ID | IT-01 |
|---------|-------|
| **Title** | Complete RF transmit sequence |
| **Preconditions** | UM2005C powered, antenna connected, debug console enabled |
| **Steps** | 1. Call `rf_twi_init()`<br>2. Create 14-byte test packet:<br>   `{0xAA, 0x55, 0x11,0x22,0x33,0x44,0x55,0x66, 0x00,0xFE, 0x02,0x6F, 0x07, 0x00}`<br>3. Call `rf_twi_transmit(packet)`<br>4. Observe timing with oscilloscope |
| **Expected** | Timeline:<br>- 1.2ms wake delay (XTAL + VCO startup)<br>- 32 CLK cycles TWI_ON<br>- 14x TWI write transactions (~224us @ 1MHz each 16 cycles)<br>- TWI_OFF command<br>- 15ms CLK low (sleep entry)<br>- Total ~17ms active time |
| **Pass Criteria** | All TWI transactions complete without error, return ERR_OK |
| **Source** | UM2005C_数据手册_V1.2_V1.2.pdf#page=14 |

### 3.9 Integration Test: LBD Read

| Test ID | IT-02 |
|---------|-------|
| **Title** | Read low battery detection value |
| **Preconditions** | UM2005C powered, rf_twi_init() called |
| **Steps** | 1. Call `rf_twi_on()`<br>2. Call `rf_twi_read_lbd(&lbd_value)`<br>3. Print LBD value |
| **Expected** | LBD value between 0x00-0x0B (maps to 2.0-3.1V)<br>With CR2032 fresh battery: ~3.0V -> LBD ~ 0x0A |
| **Pass Criteria** | LBD read returns ERR_OK, value in valid range |
| **Source** | UM2005C_数据手册_V1.2_V1.2.pdf#page=13 |

### 3.10 Error Handling: Uninitialized Access

| Test ID | EH-01 |
|---------|-------|
| **Title** | All API functions return ERR_NOT_INIT before init |
| **Preconditions** | rf_twi_init() NOT called |
| **Steps** | Call each API function |
| **Expected** | All return ERR_NOT_INIT |
| **Pass Criteria** | All functions return -8 (ERR_NOT_INIT) |

### 3.11 Error Handling: NULL Pointer

| Test ID | EH-02 |
|---------|-------|
| **Title** | Read/write with NULL pointer returns ERR_INVALID_PARAM |
| **Preconditions** | rf_twi_init() called, rf_twi_on() called |
| **Steps** | 1. Call `rf_twi_read(0x00, NULL)`<br>2. Call `rf_twi_write_buf(0x00, NULL, 5)`<br>3. Call `rf_twi_transmit(NULL)` |
| **Expected** | All return ERR_INVALID_PARAM |
| **Pass Criteria** | All return -3 (ERR_INVALID_PARAM) |

### 3.12 Power Consumption Test

| Test ID | PC-01 |
|---------|-------|
| **Title** | Verify low leakage after deinit |
| **Preconditions** | rf_twi_init() called, then rf_twi_deinit() called |
| **Steps** | 1. Measure current on PB02 and PB03 pins<br>2. Measure GPIOB block current |
| **Expected** | Both pins in input/high-Z mode<br>Leakage < 1uA per pin (IOZ spec) |
| **Pass Criteria** | Measured leakage within spec |
| **Source** | CW32L010_DataSheet_CN_V1.0.pdf#page=31 |

### 3.13 Timing Accuracy Test

| Test ID | TM-01 |
|---------|-------|
| **Title** | CLK frequency and duty cycle within UM2005C spec |
| **Preconditions** | rf_twi_init() called, rf_twi_on() called |
| **Steps** | 1. Measure CLK with oscilloscope during TWI_ON<br>2. Measure CLK during data write transaction<br>3. Record: frequency, tCH, tCL, tCR, tCF |
| **Expected** | Frequency: 0.8-1.2 MHz (target 1MHz +/-20%)<br>tCH >= 500ns<br>tCL >= 500ns<br>tCR < 50ns<br>tCF < 50ns |
| **Pass Criteria** | All timing within UM2005C spec limits |
| **Source** | UM2005C_数据手册_V1.2_V1.2.pdf#page=7 |

---

## 4. Pass/Fail Criteria

| Chapter | Min Pass Rate | Critical Tests |
|---------|---------------|----------------|
| Unit Tests (UT-xx) | 100% | UT-01, UT-02, UT-03, UT-04, UT-05 |
| Integration Tests (IT-xx) | 100% | IT-01 (full TX sequence) |
| Error Handling (EH-xx) | 100% | EH-01, EH-02 |
| Power Consumption (PC-xx) | 100% | PC-01 |
| Timing (TM-xx) | 90% | TM-01 |

**Gate**: All critical tests must pass before this module is considered stable.

---

## 5. Test Log Template

```text
=== Test Run: 2.4 rf_twi_driver ===
Date: YYYY-MM-DD
Build: <git-hash>
Tester: <name>

UT-01 GPIO init:          [PASS/FAIL]  Notes: ...
UT-02 TWI_ON:             [PASS/FAIL]  Notes: ...
UT-03 TWI write:          [PASS/FAIL]  Notes: ...
UT-04 TWI read:           [PASS/FAIL]  Notes: ...
UT-05 TWI_OFF:            [PASS/FAIL]  Notes: ...
UT-06 SOFT_RST:           [PASS/FAIL]  Notes: ...
UT-07 Write buffer:       [PASS/FAIL]  Notes: ...
IT-01 Full TX:            [PASS/FAIL]  Notes: ...
IT-02 LBD read:           [PASS/FAIL]  Notes: ...
EH-01 Uninit access:      [PASS/FAIL]  Notes: ...
EH-02 NULL pointer:       [PASS/FAIL]  Notes: ...
PC-01 Power consumption:  [PASS/FAIL]  Notes: ...
TM-01 Timing accuracy:    [PASS/FAIL]  Notes: ...

Overall: [PASS/FAIL]
```

---

## 6. Known Limitations

1. **No hardware loopback**: The UM2005C is a transmit-only device. There's no way to verify data integrity in software. TWI read is only available for LBD register.
2. **Register map dependency**: Full configuration register write depends on UM2005C register map which is not publicly documented.
3. **Crystal frequency**: The 32MHz (BOM) vs 26MHz (app note) crystal mismatch may affect RF carrier frequency. The TWI protocol itself is not affected.
4. **TWI address bit 8**: The datasheet does not clearly specify the function of address bit 8. We drive it as 0 (reserved).
