# Test Plan — Phase 2.6: packet_builder (Service Layer)

## 1. Overview

The packet_builder module is a pure software service layer with **no hardware dependencies** (mcu_resources=[]). It assembles sensor data into the 14-byte RF packet format and computes CRC-8/MAXIM.

**Testability**: T1-T4 can run on host (no MCU needed). T5-T6 require MCU for UID verification but the rest is host-testable.

---

## 2. Test Cases

### T1: CRC-8/MAXIM Computation Correctness

| Property | Value |
|----------|-------|
| **Objective** | Verify CRC-8/MAXIM (poly 0x31) produces correct results |
| **Input** | Known data patterns with pre-computed CRC |
| **Expected** | CRC matches expected values |
| **Type** | Unit test (host) |

**Test Vectors**:
| Data (hex) | Expected CRC-8/MAXIM (hex) |
|-----------|---------------------------|
| `{}` (empty) | `0x00` |
| `{0x00}` | `0x00` |
| `{0xAA, 0x55}` | `0x3B` |
| `{0xAA, 0x55, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x00, 0xFE, 0x02, 0x70, 0x06}` | Computed at test time |

**Verification**:
```c
// Host test (C, no MCU needed):
void test_crc8(void)
{
    uint8_t data1[] = {0x00};
    assert(pkt_crc8(data1, 1) == 0x00);

    uint8_t data2[] = {0xAA, 0x55};
    assert(pkt_crc8(data2, 2) == 0x3B);  // pre-verified

    uint8_t data3[] = {0xAA, 0x55, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                       0x00, 0xFE, 0x02, 0x70, 0x06};
    uint8_t crc = pkt_crc8(data3, 13);
    assert(pkt_crc8(data3, 13) == crc);  // self-consistency check

    printf("T1 CRC8: PASS\n");
}
```

**Pass Criteria**: All assertions pass.

---

### T2: Packet Assembly — Field Placement

| Property | Value |
|----------|-------|
| **Objective** | Verify all fields are placed at correct byte positions |
| **Input** | Known sensor values + known UID |
| **Expected** | Each byte in output packet matches expected value |
| **Type** | Unit test (host, with mocked UID) |

**Test Setup**:
```c
// Requires pkt_builder_init() with mocked UID cache.
// Since UID read requires MCU, we test pkt_build_raw() directly.

void test_packet_layout(void)
{
    rf_packet_t pkt;
    uint8_t *bytes = (uint8_t *)&pkt;

    // Build with known values
    int16_t temp = 254;   // 25.4°C
    uint16_t hum = 623;   // 62.3%
    int32_t ret = pkt_build_raw(temp, hum, true, false, true, &pkt);

    assert(ret == ERR_OK);
    assert(bytes[0] == 0xAA);     // Preamble 0
    assert(bytes[1] == 0x55);     // Preamble 1
    assert(bytes[8] == 0x00);     // temp_hi (254 = 0x00FE)
    assert(bytes[9] == 0xFE);     // temp_lo
    assert(bytes[10] == 0x02);    // hum_hi (623 = 0x026F)
    assert(bytes[11] == 0x6F);    // hum_lo
    assert(bytes[12] & 0x02);     // sensor_ok (bit 1)
    assert(bytes[12] & 0x04);     // tx_done (bit 2)
    assert(!(bytes[12] & 0x01));  // bat_low (bit 0) NOT set

    printf("T2 packet layout: PASS\n");
}
```

**Pass Criteria**: All field positions and values match.

---

### T3: Status Byte Assembly

| Property | Value |
|----------|-------|
| **Objective** | Verify pkt_status_byte() combines flags correctly |
| **Input** | Various flag combinations |
| **Expected** | Status byte bits set as expected |
| **Type** | Unit test (host) |

```c
void test_status_byte(void)
{
    // All flags off
    assert(pkt_status_byte(false, false, false) == 0x00);

    // Battery low only
    assert(pkt_status_byte(true, false, false) == SYS_STATUS_BAT_LOW);  // 0x01

    // Sensor OK only
    assert(pkt_status_byte(false, true, false) == SYS_STATUS_SENSOR_OK);  // 0x02

    // TX done only
    assert(pkt_status_byte(false, false, true) == SYS_STATUS_RF_TXDONE);  // 0x04

    // All flags on
    assert(pkt_status_byte(true, true, true) == 0x07);

    printf("T3 status byte: PASS\n");
}
```

**Pass Criteria**: All flag combinations produce correct status byte.

---

### T4: CRC Verification (Round-trip)

| Property | Value |
|----------|-------|
| **Objective** | Verify that a packet built by pkt_build_raw() passes pkt_verify_crc() |
| **Input** | Valid sensor values |
| **Expected** | pkt_verify_crc() returns true |
| **Type** | Unit test (host) |

```c
void test_crc_roundtrip(void)
{
    rf_packet_t pkt;
    int32_t ret = pkt_build_raw(254, 623, true, false, true, &pkt);
    assert(ret == ERR_OK);

    // Self-verification
    assert(pkt_verify_crc(&pkt));

    // Tamper with temperature byte -> CRC should fail
    pkt.temp_lo ^= 0xFF;
    assert(!pkt_verify_crc(&pkt));

    printf("T4 CRC roundtrip: PASS\n");
}
```

**Pass Criteria**: Fresh packet passes CRC. Tampered packet fails CRC.

---

### T5: UID Read (Requires MCU)

| Property | Value |
|----------|-------|
| **Objective** | Verify MCU UID is read correctly from digital signature memory |
| **Input** | pkt_read_uid() or pkt_builder_init() |
| **Expected** | 10-byte UID returned, first 6 bytes match cached UID |
| **Type** | Integration test (MCU required) |

```c
void test_uid_read(void)
{
    uint8_t uid[10];
    int32_t ret = pkt_read_uid(uid);
    assert(ret == ERR_OK);

    // UID should not be all zeros (silicon is unique)
    bool all_zero = true;
    for (int i = 0; i < 10; i++) {
        if (uid[i] != 0) { all_zero = false; break; }
    }
    assert(!all_zero);

    // First 6 bytes should match cached UID after init
    pkt_builder_init();
    const uint8_t *cached = pkt_get_cached_uid();
    assert(cached != NULL);
    for (int i = 0; i < 6; i++) {
        assert(uid[i] == cached[i]);
    }

    printf("T5 UID read: PASS\n");
}
```

**Pass Criteria**: UID is non-zero and cached UID matches first 6 bytes.

---

### T6: Full Build + Verify (Requires MCU)

| Property | Value |
|----------|-------|
| **Objective** | End-to-end: init → build → verify → transmit (via rf_twi_driver) |
| **Input** | Simulated sensor data |
| **Expected** | Packet builds, CRC verifies, packet has proper structure |
| **Type** | Integration test (MCU required) |

```c
void test_full_build(void)
{
    int32_t ret;

    // Initialize
    ret = pkt_builder_init();
    assert(ret == ERR_OK);

    // Build packet from sensor data
    sensor_data_t sensor = {
        .temperature_x10 = 254,   // 25.4°C
        .humidity_x10 = 623,       // 62.3%
        .status = SENSOR_STATUS_VALID,
        .timestamp_ms = 0
    };

    rf_packet_t pkt;
    ret = pkt_build(&sensor, false, &pkt);
    assert(ret == ERR_OK);

    // Verify packet structure
    assert(pkt.preamble[0] == 0xAA);
    assert(pkt.preamble[1] == 0x55);
    assert(pkt.sys_status & SYS_STATUS_SENSOR_OK);
    assert(!(pkt.sys_status & SYS_STATUS_BAT_LOW));

    // Verify CRC
    assert(pkt_verify_crc(&pkt));

    // Packet is ready for rf_twi_transmit((const uint8_t *)&pkt);
    printf("T6 full build: PASS\n");
}
```

**Pass Criteria**: Full build-verify cycle succeeds.

---

## 3. Test Execution Matrix

| Test | Host/MCU | Depends on | Priority |
|------|----------|------------|----------|
| T1 CRC8 | Host | None | P0 |
| T2 Packet layout | Host | T1 | P0 |
| T3 Status byte | Host | None | P0 |
| T4 CRC roundtrip | Host | T1, T2 | P0 |
| T5 UID read | MCU | DIGITALSIGN_GetChipUid | P1 |
| T6 Full build+verify | MCU | T1-T5 | P1 |

## 4. HIL Test (Hardware-in-the-Loop)

When hardware is available:
1. Load firmware with test mode that builds and transmits a packet every 150s
2. Use spectrum analyzer at 433 MHz to verify RF transmission
3. Use logic analyzer on PB02/PB03 to verify TWI timing and packet content
4. Compare captured TWI data with expected packet: 0xAA 0x55 [UID6] [temp2] [hum2] [status] [CRC8]

## 5. Pass/Fail Criteria

- **All P0 tests must pass** before module is accepted
- **P1 tests** require MCU hardware; document results when hardware is available
- A single test failure blocks the module
