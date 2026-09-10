# Protocol Notes: UM2005C TWI Protocol -- Module 2.4

> Part of rf_twi_driver (phase 2.4)
> Bus instance: GPIO_IN_INTERRUPT (connectivity.json)
> MCU pins: PB02 (CLK), PB03 (DATA)

---

## 1. Source Documents

| Document | Section | Content |
|----------|---------|---------|
| UM2005C_数据手册_V1.2_V1.2.pdf | page 12 | TWI interface: 16-cycle R/W, CLK <= 1MHz falling-edge |
| UM2005C_数据手册_V1.2_V1.2.pdf | page 12 | Special commands: TWI_ON(32x0), TWI_OFF(0xFF02), TWI_RST(0xFF01), SOFT_RST(0xFF04) |
| UM2005C_数据手册_V1.2_V1.2.pdf | page 13 | OTP: 63 bytes, TWI addressable registers 0x00-0x3E |
| UM2005C_数据手册_V1.2_V1.2.pdf | page 13 | LBD: read voltage level after TWI_ON, range 2.0-3.1V |
| UM2005C_数据手册_V1.2_V1.2.pdf | page 14 | State diagram: Sleep -> Wake -> XTAL stable -> VCO cal -> OTP read -> Config -> TX -> Sleep |
| UM2005C_数据手册_V1.2_V1.2.pdf | page 6 | Timing: tSLP-TX=1.2ms, tXTAL=400us, tTUNE=300us |
| UM2005C_数据手册_V1.2_V1.2.pdf | page 7 | Digital IO: tCH>=500ns, tCL>=500ns, FSCL=10-1000kHz |
| UM2005C_数据手册_V1.2_V1.2.pdf | page 15 | Data modes: Online config mode (TWI config) and OTP mode (DATA edge wake) |
| UM2005C_数据手册_V1.2_V1.2.pdf | page 17 | Application circuit + matching table for 433MHz/13dBm |
| connectivity.json | buses[GPIO_IN_INTERRUPT] | RF_CLK=PB02, RF_DATA=PB03 net assignment |
| connectivity.json | conflicts[0] | User: "发射功率0DB" -- TX power = 0 dBm |

---

## 2. TWI Transaction Format

### 2.1 16-Cycle Transaction

```
CLK   +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+
      | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
      +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+
        ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^   ^
DATA  [W/R][A5 ][A4 ][A3 ][A2 ][A1 ][A0 ][ X ][D7 ][D6 ][D5 ][D4 ][D3 ][D2 ][D1 ][D0 ]
       --------Cycle 1-8: Address Byte-------- --------Cycle 9-16: Data Byte--------
       W/R=1: Write operation
       W/R=0: Read operation
```

- **Cycles 1-8 (Address byte)**: MCU drives DATA
  - Cycle 1: W/R bit (1=write, 0=read)
  - Cycles 2-7: Address bits A[5:0], MSB first
  - Cycle 8: Reserved (data sheet shows an 8th bit; we drive 0)
- **Cycles 9-16 (Data byte)**:
  - **Write (W/R=1)**: MCU drives DATA, D[7:0] MSB first
  - **Read (W/R=0)**: UM2005C drives DATA, MCU samples on CLK falling edge

### 2.2 Timing Parameters

| Parameter | Min | Max | Unit | Source |
|-----------|-----|-----|------|--------|
| CLK frequency (FSCL) | 10 | 1000 | kHz | UM2005C page 7 |
| CLK high time (tCH) | 500 | -- | ns | UM2005C page 7 |
| CLK low time (tCL) | 500 | -- | ns | UM2005C page 7 |
| CLK rise time (tCR) | -- | 50 | ns | UM2005C page 7 |
| CLK fall time (tCF) | -- | 50 | ns | UM2005C page 7 |
| Sampling edge | -- | Falling | -- | UM2005C page 12 |

**Implementation**: For 48MHz HCLK (20.83ns per cycle), each half-period requires:
- 24 NOPs ~ 500ns -> 1MHz CLK period = 1us -> 48 CPU cycles per CLK cycle
- This gives max CLK = 1MHz which is within spec

### 2.3 Special Commands

| Command | Hex | Address Byte | Data Byte | Description |
|---------|-----|-------------|-----------|-------------|
| TWI_ON | (32 zeros) | 32 CLK cycles DATA=0 | -- | Enter programming mode, reset TWI |
| TWI_OFF | 0xFF02 | W/R=1, A[5:0]=0x3F | 0x02 | Exit programming mode |
| SOFT_RST | 0xFF04 | W/R=1, A[5:0]=0x3F | 0x04 | Reset all digital except TWI |
| TWI_RST | 0xFF01 | W/R=1, A[5:0]=0x3F | 0x01 | Reset TWI module only |

Note: For TWI_OFF/SOFT_RST/TWI_RST, the "address" portion is 0x3F (all ones except W/R bit = 1).

---

## 3. RF Transmission Sequence

```
+---------------------------------------------------------------------+
| 1. CLK rising edge -> chip wakeup (if in sleep)                     |
| 2. Wait ~400us for XTAL startup (tXTAL)                            |
| 3. Wait ~300us for VCO tune (tTUNE)                                |
| 4. Send TWI_ON: 32 CLK cycles with DATA=0                          |
| 5. Wait ~200us for OTP load + stabilization                        |
| 6. [Optional] Write configuration registers via TWI (address 0x00-0x3E) |
|    - Configuration skipped if OTP already programmed                |
| 7. Write 14-byte payload packet via TWI write transactions         |
| 8. Send TWI_OFF (0xFF02) to start TX                               |
| 9. Hold CLK low >=15ms -> chip enters sleep                          |
+---------------------------------------------------------------------+

Total active time: ~1.2ms (wakeup) + 14xTWI (~224us) + 15ms sleep-entry
                    ~ 17ms total per cycle
```

---

## 4. Configuration Register Map (OTP)

63 bytes total, addresses 0x00-0x3E. Register definitions are internal to UM2005C. Key areas:

| Address Range | Content | Notes |
|--------------|---------|-------|
| 0x00-0x01 | Carrier frequency | Configured for 433MHz with 26MHz or 32MHz XTAL |
| 0x02 | Output power + modulation | Power: -20..+18dBm, step 1dB; MOD: GFSK/FSK/OOK |
| 0x03-0x05 | Data rate + shaping | FSK: 0.5-100kbps; Gaussian filter BT=0.5 |
| 0x06-0x08 | PA ramping | Ramp time 128us typ |
| 0x09-0x3D | Various | Packet config, FIFO, etc. |
| 0x3E | LBD threshold | 2.0-3.1V, 0.1V resolution |

**For development mode (no OTP)**: Registers can be written via TWI on each wakeup. The chip auto-loads OTP on power-up; if OTP is blank, default values apply. Writing registers via TWI overrides OTP values.

**For production**: OTP must be programmed once with correct carrier frequency, data rate, and output power (0 dBm per user req). See OTP programmer tool from UM2005C SDK.

**Note on crystal mismatch**: BOM specifies X322526MOB4SI (32 MHz) but UM2005C app note says 26 MHz for 433 MHz. The firmware must configure UM2005C PLL registers for the actual crystal frequency. If using the internal default, the carrier frequency will be off by a factor of 32/26.

---

## 5. Data Flow for RF Transmission

```
packet_builder (service layer)
    |
    +-- rf_packet_t (14 bytes): preamble + UID + temp + hum + status + CRC8
    |
    v
rf_twi_driver (this module)
    |
    +-- rf_twi_init()         -> Configure PB02(CLK), PB03(DATA) as GPIO
    +-- rf_twi_on()           -> 32 zeros -> enter programming mode
    +-- rf_twi_sleep_ms()     -> Delay wrapper
    +-- rf_twi_write()        -> 16-cycle TWI write transaction
    +-- rf_twi_read()         -> 16-cycle TWI read transaction
    +-- rf_twi_off()          -> 0xFF02 -> exit programming mode, start TX
    +-- rf_twi_deinit()       -> GPIO to input/low-power state
    |
    v
UM2005C (hardware)
    +-- CLK (PB02) <- TWI clock <= 1MHz
    +-- DATA (PB03) <- TWI bidirectional data
    +-- VDD (BAT) <- 1.9-3.6V
    +-- XTAL <- 32MHz crystal (assumed) -- **CRYSTAL FREQUENCY CONCERN**
    +-- RFO -> Matching network -> Antenna (433 MHz)
```

---

## 6. LBD (Low Battery Detection)

- Detected on power-up or TWI_ON [UM2005C page 13]
- Voltage range: 2.0-3.1V, resolution 0.1V
- Read via TWI: address 0x3E (read direction, W/R=0)
- Returned value maps to voltage: LBD_CODE = (Vbat - 2.0) / 0.1

---

## 7. Assumptions and Open Issues

| # | Issue | Impact | Assumption / Mitigation |
|---|-------|--------|------------------------|
| 1 | 32MHz vs 26MHz crystal | Carrier frequency offset | Register config must be adjusted for 32MHz; verify with spectrum analyzer |
| 2 | Register map details (0x00-0x3E) not publicly available | Cannot pre-configure for production | Register writes skipped during development (OTP pre-programmed by factory); use OTP programmer tool |
| 3 | TX power = 0 dBm | User request in connectivity.json | Set output power register to 0 dBm (requires register map details from manufacturer SDK) |
| 4 | No AHT10 datasheet | Protocol unconfirmed | Not relevant to this module |
| 5 | TWI address bit 8 (reserved) | Data sheet unclear | Drive as 0 (reserved); chip behavior not documented |

---

## 8. Byte-Level Packet Layout for TWI Transmission

The 14-byte RF packet is written to UM2005C via 14 consecutive TWI write transactions:

```
Packet Byte Index  ->  TWI Write Transaction
    0  (preamble[0]=0xAA)  ->  Write addr=0x00, data=0xAA
    1  (preamble[1]=0x55)  ->  Write addr=0x00, data=0x55
    2  (uid[0])            ->  Write addr=0x00, data=uid[0]
    ...                    ->  ... (14 transactions)
   13  (crc8)              ->  Write addr=0x00, data=crc8
```

**Note**: The exact register address for data FIFO/streaming depends on UM2005C register map (not publicly documented). As a fallback, bytes can be written one at a time to the data register. If the UM2005C supports direct data streaming after TWI_ON, the DATA pin can simply be toggled with CLK pulses for each bit of the payload.
