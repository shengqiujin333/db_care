/**
 * @file    interface.h
 * @brief   Phase 2.6 — packet_builder: Exported API for upper layers
 *
 * MCU:     CW32L010Y8M6 (TSSOP-20)
 * Module:  packet_builder (layer=service)
 *
 * This module provides:
 *   - RF packet assembly from sensor_data_t into 14-byte rf_packet_t
 *   - CRC-8/MAXIM computation (polynomial 0x31)
 *   - MCU UID read helper (lower 48 bits of 96-bit UID)
 *   - System status byte construction (battery, sensor, TX flags)
 *   - Packet validation (CRC check)
 *
 * Hardware notes:
 *   This is a PURE SOFTWARE service module — mcu_resources = [].
 *   No MCU peripheral, GPIO pin, IRQ, or DMA is owned by this module.
 *   It reads MCU UID via DIGITALSIGN_GetChipUid (digital signature memory,
 *   not a peripheral register) and produces data for the rf_twi_driver (2.4).
 *
 * Consumers:
 *   - sensor_mgr (application layer, phase 2.7): calls packet_build() after averaging
 *   - rf_mgr     (application layer, phase 2.8): calls pkt_build() then rf_twi_transmit()
 *
 * Sources:
 *   - output/design/common_types.h#L94-L103   (rf_packet_t definition)
 *   - output/design/common_types.h#L105-L107   (RF_PACKET_SIZE, preamble macros)
 *   - output/design/common_types.h#L110-L112   (SYS_STATUS_* bit flags)
 *   - output/design/common_types.h#L57-L62     (sensor_data_t definition)
 *   - inputs/mcu_sdk/Libraries/inc/cw32l010_digitalsign.h#L53-L56 (CHIP_UID_BASE, CHIP_UID_LENGTH)
 *   - inputs/mcu_sdk/Libraries/inc/cw32l010_digitalsign.h#L68     (DIGITALSIGN_GetChipUid)
 *   - COMMON.md#1.2 (packet format: [0xAA][0x55][UID6][temp2][hum2][status][CRC8])
 */

#ifndef PACKET_BUILDER_INTERFACE_H
#define PACKET_BUILDER_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>
#include "./../output/design/common_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ====================================================================
 * Constants
 * ==================================================================== */

/** @brief Size of MCU UID portion used in packet (bytes) */
#define PKT_UID_SIZE                6U

/** @brief Total RF packet size in bytes (defined in common_types.h) */
#define PKT_PACKET_SIZE             RF_PACKET_SIZE

/** @brief Index of each field in the packet byte array */
#define PKT_IDX_PREAMBLE_0          0U
#define PKT_IDX_PREAMBLE_1          1U
#define PKT_IDX_UID_START           2U
#define PKT_IDX_TEMP_HI             8U
#define PKT_IDX_TEMP_LO             9U
#define PKT_IDX_HUM_HI              10U
#define PKT_IDX_HUM_LO              11U
#define PKT_IDX_SYS_STATUS          12U
#define PKT_IDX_CRC                 13U

/* ====================================================================
 * Initialization
 * ==================================================================== */

/**
 * @brief  Initialize the packet builder module
 *
 * Reads the MCU UID once and caches it internally for all subsequent
 * packet builds. This avoids repeated calls to the UID read function.
 *
 * Source: inputs/mcu_sdk/Libraries/inc/cw32l010_digitalsign.h#L68
 *
 * @return ERR_OK on success
 */
int32_t pkt_builder_init(void);

/**
 * @brief  De-initialize the packet builder module
 *
 * Clears the cached UID. After this call, packet_build() will return
 * ERR_NOT_INIT until pkt_builder_init() is called again.
 */
void pkt_builder_deinit(void);

/* ====================================================================
 * Packet Assembly
 * ==================================================================== */

/**
 * @brief  Build a complete 14-byte RF packet from sensor data
 *
 * Constructs the packet with the following layout:
 *   [0]    = 0xAA (preamble byte 1)
 *   [1]    = 0x55 (preamble byte 2)
 *   [2-7]  = 6-byte MCU UID (lower 48 bits of 96-bit UID)
 *   [8]    = temperature_x10 >> 8 (MSB)
 *   [9]    = temperature_x10 & 0xFF (LSB)
 *   [10]   = humidity_x10 >> 8 (MSB)
 *   [11]   = humidity_x10 & 0xFF (LSB)
 *   [12]   = System status byte (see SYS_STATUS_* flags)
 *   [13]   = CRC-8/MAXIM over bytes 0-12 (poly 0x31)
 *
 * Source: COMMON.md#1.2 (execution flow section)
 * Source: output/design/common_types.h#L74-L107
 *
 * @param  sensor      Pointer to sensor data (temperature_x10, humidity_x10, status)
 * @param  bat_low     Battery low flag (true = battery low)
 * @param  out_packet  Pointer to rf_packet_t to fill
 * @return ERR_OK on success, ERR_INVALID_PARAM if NULL pointers,
 *         ERR_NOT_INIT if pkt_builder_init() not called
 */
int32_t pkt_build(const sensor_data_t *sensor, bool bat_low, rf_packet_t *out_packet);

/**
 * @brief  Build a raw byte array RF packet from component values
 *
 * Same as pkt_build() but accepts raw values instead of sensor_data_t.
 * Useful for testing or when sensor data is not in sensor_data_t format.
 *
 * @param  temp_x10    Temperature in 0.1°C (e.g. 254 = 25.4°C)
 * @param  hum_x10     Humidity in 0.1% (e.g. 623 = 62.3%)
 * @param  sensor_ok   Sensor data valid flag
 * @param  bat_low     Battery low flag
 * @param  tx_done     RF TX completed flag
 * @param  out_packet  Pointer to rf_packet_t to fill
 * @return ERR_OK on success, ERR_INVALID_PARAM if out_packet is NULL,
 *         ERR_NOT_INIT if pkt_builder_init() not called
 */
int32_t pkt_build_raw(int16_t temp_x10, uint16_t hum_x10,
                      bool sensor_ok, bool bat_low, bool tx_done,
                      rf_packet_t *out_packet);

/* ====================================================================
 * Packet Validation
 * ==================================================================== */

/**
 * @brief  Validate an RF packet by checking its CRC8
 *
 * Computes CRC-8/MAXIM over bytes 0-12 and compares with the stored CRC
 * in byte 13.
 *
 * @param  packet  Pointer to rf_packet_t to validate
 * @return true if CRC matches, false otherwise
 */
bool pkt_verify_crc(const rf_packet_t *packet);

/**
 * @brief  Compute CRC-8/MAXIM over a data buffer
 *
 * Polynomial: 0x31 (x^8 + x^5 + x^4 + 1)
 * Initial value: 0x00
 * No final XOR
 *
 * Source: output/design/common_types.h#L87 (CRC8 specified in rf_packet_t)
 *
 * @param  data    Pointer to data buffer
 * @param  length  Number of bytes
 * @return uint8_t CRC-8 value
 */
uint8_t pkt_crc8(const uint8_t *data, uint16_t length);

/* ====================================================================
 * UID Access
 * ==================================================================== */

/**
 * @brief  Read MCU unique ID into a buffer
 *
 * Reads the full 10-byte UID from the MCU's digital signature memory.
 * Only the first PKT_UID_SIZE (6) bytes are used in the RF packet,
 * but the full 10 bytes are returned for potential use by other modules.
 *
 * Source: inputs/mcu_sdk/Libraries/inc/cw32l010_digitalsign.h#L68
 * Source: inputs/mcu_sdk/Libraries/src/cw32l010_digitalsign.c#L89-L103
 *
 * @param  out_uid  Pointer to buffer (must be at least CHIP_UID_LENGTH=10 bytes)
 * @return ERR_OK on success, ERR_INVALID_PARAM if out_uid is NULL
 */
int32_t pkt_read_uid(uint8_t *out_uid);

/**
 * @brief  Get pointer to cached 6-byte UID (lower 48 bits)
 *
 * Returns a pointer to the internal cache, avoiding a second UID read.
 * Only valid after pkt_builder_init() has been called.
 *
 * @return const uint8_t*  Pointer to 6-byte UID, or NULL if not initialized
 */
const uint8_t *pkt_get_cached_uid(void);

/* ====================================================================
 * Utility
 * ==================================================================== */

/**
 * @brief  Build the system status byte from flags
 *
 * Combines individual flag bits into the SYS_STATUS byte:
 *   bit 0: battery low
 *   bit 1: sensor data valid
 *   bit 2: RF TX done
 *
 * Source: output/design/common_types.h#L110-L112
 *
 * @param  bat_low     Battery low flag
 * @param  sensor_ok   Sensor data valid flag
 * @param  tx_done     RF transmission completed flag
 * @return uint8_t     Combined status byte
 */
uint8_t pkt_status_byte(bool bat_low, bool sensor_ok, bool tx_done);

#ifdef __cplusplus
}
#endif

#endif /* PACKET_BUILDER_INTERFACE_H */
