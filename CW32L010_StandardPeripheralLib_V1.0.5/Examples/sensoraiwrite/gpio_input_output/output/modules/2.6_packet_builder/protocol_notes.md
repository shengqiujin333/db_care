# Protocol Notes — Phase 2.6: packet_builder

## RF Packet Protocol — UM2005C 433 MHz FSK

### 1. Packet Format

The RF packet is 14 bytes total, transmitted via TWI to the UM2005C RF transmitter.

#### Byte Layout

| Byte Offset | Field | Size (B) | Value / Encoding | Description |
|-------------|-------|----------|-----------------|-------------|
| 0 | Preamble 0 | 1 | `0xAA` | Sync byte 1 — receiver uses this to detect start of frame |
| 1 | Preamble 1 | 1 | `0x55` | Sync byte 2 — alternating bit pattern for bit clock recovery |
| 2-7 | UID | 6 | Lower 48 bits of MCU 96-bit UID | Unique device identifier, read from CW32L010 digital signature memory at `0x001007B0` |
| 8 | Temp MSB | 1 | `(temperature_x10 >> 8) & 0xFF` | Temperature high byte (big-endian) |
| 9 | Temp LSB | 1 | `temperature_x10 & 0xFF` | Temperature low byte |
| 10 | Humidity MSB | 1 | `(humidity_x10 >> 8) & 0xFF` | Humidity high byte (big-endian) |
| 11 | Humidity LSB | 1 | `humidity_x10 & 0xFF` | Humidity low byte |
| 12 | Status | 1 | Bitfield (see below) | System status flags |
| 13 | CRC8 | 1 | CRC-8/MAXIM over bytes 0-12 | Packet integrity check |

#### Status Byte Bit Definitions

| Bit | Mask | Name | Description |
|-----|------|------|-------------|
| 0 | `0x01` | BAT_LOW | Battery voltage low (LVD threshold < 2.6V) |
| 1 | `0x02` | SENSOR_OK | Sensor data valid (read successful) |
| 2 | `0x04` | RF_TXDONE | RF transmission completed successfully |
| 3-7 | — | Reserved | Set to 0 for future use |

### 2. Data Encoding

#### Temperature
- **Type**: `int16_t`, signed two's complement
- **Resolution**: 0.1°C per LSB
- **Range**: -50.0°C to +85.0°C (limited by MCU operating range)
- **Example**: 25.4°C → `254` → bytes: `[0x00, 0xFE]`
- **Example**: -5.3°C → `-53` → bytes: `[0xFF, 0xCB]`
- **Source**: AHT10 conversion formula: `temp = (raw32 * 200.0 / 1048576.0 - 50.0) * 10`

#### Humidity
- **Type**: `uint16_t`, unsigned
- **Resolution**: 0.1% RH per LSB
- **Range**: 0.0% to 100.0%
- **Example**: 62.3% → `623` → bytes: `[0x02, 0x6F]`
- **Source**: AHT10 conversion formula: `hum = (raw32 * 100.0 / 1048576.0) * 10`

#### MCU UID
- **Full UID**: 10 bytes (80 bits) at ROM address `0x001007B0`
- **Used in packet**: First 6 bytes (lower 48 bits)
- **Source**: `inputs/mcu_sdk/Libraries/inc/cw32l010_digitalsign.h#L53-L56`
- **User requirement**: connectivity.json conflicts[0].user_note: "UID用Mcu的"

### 3. CRC-8/MAXIM Algorithm

The CRC-8/MAXIM (Dallas 1-Wire) polynomial is used for packet integrity.

#### Parameters
| Parameter | Value |
|-----------|-------|
| Polynomial | `0x31` (x⁸ + x⁵ + x⁴ + 1) |
| Initial value | `0x00` |
| Final XOR | None |
| Coverage | Bytes 0-12 (13 bytes total) |
| Stored at | Byte 13 |

#### Implementation (Bit-by-bit)
```c
uint8_t crc8_maxim(const uint8_t *data, uint16_t len)
{
    uint8_t crc = 0x00;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x80) {
                crc = (uint8_t)((crc << 1) ^ 0x31);
            } else {
                crc = (uint8_t)(crc << 1);
            }
        }
    }
    return crc;
}
```

#### Test Vectors
| Input (hex) | Expected CRC (hex) |
|------------|-------------------|
| `{}` (empty) | `0x00` |
| `{0x00}` | `0x00` |
| `{0xAA, 0x55}` | `0x3B` |

### 4. Protocol Sources

| Aspect | Source | Status |
|--------|--------|--------|
| Packet field layout | COMMON.md#1.2 (architecture decision) | ✅ Confirmed |
| rf_packet_t struct | `output/design/common_types.h#L94-L103` | ✅ Generated |
| CRC-8/MAXIM poly 0x31 | `output/design/common_types.h#L87` | ✅ Specified |
| MCU UID read API | `inputs/mcu_sdk/Libraries/inc/cw32l010_digitalsign.h#L68` | ✅ SDK |
| MCU UID address | `inputs/mcu_sdk/Libraries/inc/cw32l010_digitalsign.h#L53` (`0x001007B0`) | ✅ SDK |
| UID user requirement | connectivity.json conflicts[0].user_note | ✅ User stated |
| Temperature formula | `inputs/mcu_sdk/Examples/sensor/gpio_input_output/USER/src/measure.c` | ✅ SDK example |
| Humidity formula | `inputs/mcu_sdk/Examples/sensor/gpio_input_output/USER/src/measure.c` | ✅ SDK example |
| Packet size = 14 bytes | `output/design/common_types.h#L105` | ✅ Confirmed |
| Preamble 0xAA 0x55 | `output/design/common_types.h#L106-L107` | ✅ Confirmed |

### 5. Notes & Assumptions

1. **Byte ordering**: Packet fields are big-endian (network byte order) for compatibility with common RF receivers.

2. **UID size**: The CW32L010 provides 10 bytes of UID. We use the first 6 bytes (48 bits) for a reasonable trade-off between uniqueness and packet overhead. 48 bits provides ~2.8×10¹⁴ possible values, sufficient for this application.

3. **CRC coverage**: CRC covers bytes 0-12 (all fields except the CRC byte itself). This is standard practice.

4. **Zero CRC initial value**: Unlike some CRC-8/MAXIM variants that use initial value `0x00`, we stick with standard `0x00` since no explicit variation is specified.

5. **Receiver expectation**: The receiver (not in scope) should validate preamble (0xAA 0x55), extract UID for device identification, decode temp/humidity, check CRC, and ignore reserved status bits.

6. **Module purity**: The packet_builder module is a pure-software service layer. It does not own or configure any MCU hardware peripheral. The packet data it produces is consumed by the rf_twi_driver module (2.4), which handles the actual TWI bit-banging on PB02/PB03.
