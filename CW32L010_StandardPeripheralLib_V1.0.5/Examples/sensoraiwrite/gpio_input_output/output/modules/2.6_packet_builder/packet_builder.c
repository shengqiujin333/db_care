/**
 * @file    packet_builder.c
 * @brief   Phase 2.6 — packet_builder: RF packet assembly implementation
 *
 * MCU:     CW32L010Y8M6 (TSSOP-20)
 * Module:  packet_builder (layer=service)
 *
 * This module assembles sensor data into the 14-byte RF packet format
 * defined in common_types.h. It handles:
 *   - CRC-8/MAXIM computation (polynomial 0x31)
 *   - MCU UID cache (lower 48 bits of 96-bit UID)
 *   - System status byte assembly from flag bits
 *   - Packet CRC validation
 *
 * The module has NO hardware dependencies (mcu_resources=[]). It is pure
 * software logic operating on data structures. No MCU peripheral, pin,
 * IRQ, or DMA is claimed or configured by this module.
 *
 * Sources:
 *   - output/design/common_types.h#L74-L107   (rf_packet_t, packet format constants)
 *   - output/design/common_types.h#L110-L112   (SYS_STATUS_* bit flags)
 *   - inputs/mcu_sdk/Libraries/inc/cw32l010_digitalsign.h#L53-L68 (UID read API)
 *   - COMMON.md#1.2 (execution flow: packet format description)
 *   - connectivity.json conflicts[0].user_note: "UID用Mcu的" (user requirement)
 *
 * Consumers: sensor_mgr (2.7), rf_mgr (2.8)
 */

/* ====================================================================
 * DRIVER SUMMARY VALIDATION BLOCK
 * ====================================================================
 *
 * This block is read by ConnectivityAgent to verify hardware mapping.
 * Since packet_builder is a pure-software service module with no MCU
 * resources, instance is null, pins is empty, addr is null.
 *
 * ```json
 * {
 *   "module": "packet_builder",
 *   "instance": null,
 *   "pins": {},
 *   "addr": null,
 *   "baudrate": null,
 *   "flow_control": null,
 *   "rx_mode": null,
 *   "rx_dma": null,
 *   "rx_irq": null,
 *   "notes": "Pure software service module. No MCU peripheral owned. Reads MCU UID via DIGITALSIGN_GetChipUid from digital signature memory."
 * }
 * ```
 * ==================================================================== */

#include "packet_builder.h"

/* ====================================================================
 * Module-Level State (Singleton)
 * ==================================================================== */

/** Singleton packet builder state */
static pkt_builder_t s_pkt = {
    .state = PKT_STATE_UNINIT,
    .uid_cache = {0},
    .uid_cached = false
};

/* ====================================================================
 * Initialization and De-initialization
 * ==================================================================== */

int32_t pkt_builder_init(void)
{
    uint8_t raw_uid[CHIP_UID_LENGTH];  /* 10 bytes from SDK */

    if (s_pkt.state == PKT_STATE_READY) {
        return ERR_OK;  /* Already initialized, idempotent */
    }

    /* Read the full 10-byte UID from MCU digital signature memory */
    /* Source: inputs/mcu_sdk/Libraries/src/cw32l010_digitalsign.c#L89-L103 */
    DIGITALSIGN_GetChipUid(raw_uid);

    /* Take the first 6 bytes (lower 48 bits of 96-bit UID) */
    /* User req: connectivity.json conflicts[0].user_note: "UID用Mcu的" */
    for (uint8_t i = 0; i < PKT_UID_SIZE; i++) {
        s_pkt.uid_cache[i] = raw_uid[i];
    }

    s_pkt.uid_cached = true;
    s_pkt.state = PKT_STATE_READY;

    return ERR_OK;
}

void pkt_builder_deinit(void)
{
    s_pkt.state = PKT_STATE_UNINIT;
    s_pkt.uid_cached = false;

    /* Clear cached UID for security/safety */
    for (uint8_t i = 0; i < PKT_UID_SIZE; i++) {
        s_pkt.uid_cache[i] = 0;
    }
}

/* ====================================================================
 * CRC-8/MAXIM Computation
 * ==================================================================== */

uint8_t pkt_crc8(const uint8_t *data, uint16_t length)
{
    if (data == NULL || length == 0) {
        return 0;
    }

    return pkt_crc8_compute(data, length);
}

/* ====================================================================
 * System Status Byte Assembly
 * ==================================================================== */

uint8_t pkt_status_byte(bool bat_low, bool sensor_ok, bool tx_done)
{
    uint8_t status = 0;

    if (bat_low) {
        status |= SYS_STATUS_BAT_LOW;       /* bit 0 */
    }
    if (sensor_ok) {
        status |= SYS_STATUS_SENSOR_OK;     /* bit 1 */
    }
    if (tx_done) {
        status |= SYS_STATUS_RF_TXDONE;     /* bit 2 */
    }

    return status;
}

/* ====================================================================
 * Packet Assembly
 * ==================================================================== */

int32_t pkt_build(const sensor_data_t *sensor, bool bat_low, rf_packet_t *out_packet)
{
    if (sensor == NULL || out_packet == NULL) {
        return ERR_INVALID_PARAM;
    }
    if (s_pkt.state != PKT_STATE_READY) {
        return ERR_NOT_INIT;
    }

    /* Build using the raw value helper */
    return pkt_build_raw(
        sensor->temperature_x10,
        sensor->humidity_x10,
        (sensor->status & SENSOR_STATUS_VALID) ? true : false,
        bat_low,
        false,  /* tx_done = false (we're building before TX) */
				out_packet
		);
}

int32_t pkt_build_raw(int16_t temp_x10, uint16_t hum_x10,
                      bool sensor_ok, bool bat_low, bool tx_done,
                      rf_packet_t *out_packet)
{
    uint8_t *packet_bytes;
    uint8_t status;

    if (out_packet == NULL) {
        return ERR_INVALID_PARAM;
    }
    if (s_pkt.state != PKT_STATE_READY) {
        return ERR_NOT_INIT;
    }

    /* Get byte-level access to the packed struct */
    packet_bytes = (uint8_t *)out_packet;

    /* ----------------------------------------------------------------
     * Byte 0: Preamble first byte 0xAA
     * Source: output/design/common_types.h#L106
     * ---------------------------------------------------------------- */
    packet_bytes[PKT_IDX_PREAMBLE_0] = RF_PREAMBLE_0;  /* 0xAA */

    /* ----------------------------------------------------------------
     * Byte 1: Preamble second byte 0x55
     * Source: output/design/common_types.h#L107
     * ---------------------------------------------------------------- */
    packet_bytes[PKT_IDX_PREAMBLE_1] = RF_PREAMBLE_1;  /* 0x55 */

    /* ----------------------------------------------------------------
     * Bytes 2-7: MCU UID (lower 48 bits of 96-bit UID)
     * Source: connectivity.json conflicts[0].user_note: "UID用Mcu的"
     * ---------------------------------------------------------------- */
    for (uint8_t i = 0; i < PKT_UID_SIZE; i++) {
        packet_bytes[PKT_IDX_UID_START + i] = s_pkt.uid_cache[i];
    }

    /* ----------------------------------------------------------------
     * Bytes 8-9: Temperature (int16_t, 0.1°C resolution)
     * Big-endian: MSB first, LSB second
     * ---------------------------------------------------------------- */
    packet_bytes[PKT_IDX_TEMP_HI] = (uint8_t)((uint16_t)temp_x10 >> 8);
    packet_bytes[PKT_IDX_TEMP_LO] = (uint8_t)((uint16_t)temp_x10 & 0xFF);

    /* ----------------------------------------------------------------
     * Bytes 10-11: Humidity (uint16_t, 0.1% resolution)
     * Big-endian: MSB first, LSB second
     * ---------------------------------------------------------------- */
    packet_bytes[PKT_IDX_HUM_HI] = (uint8_t)(hum_x10 >> 8);
    packet_bytes[PKT_IDX_HUM_LO] = (uint8_t)(hum_x10 & 0xFF);

    /* ----------------------------------------------------------------
     * Byte 12: System status
     * Source: output/design/common_types.h#L110-L112
     * ---------------------------------------------------------------- */
    status = pkt_status_byte(bat_low, sensor_ok, tx_done);
    packet_bytes[PKT_IDX_SYS_STATUS] = status;

    /* ----------------------------------------------------------------
     * Byte 13: CRC-8/MAXIM over bytes 0-12
     * Polynomial 0x31, initial 0x00, no final XOR
     * Source: output/design/common_types.h#L87 (rf_packet_t.crc8 comment)
     * ---------------------------------------------------------------- */
    packet_bytes[PKT_IDX_CRC] = pkt_crc8_compute(packet_bytes, PKT_CRC_COVERED_BYTES);

    /* Also fill the struct fields for convenience */
    out_packet->preamble[0] = RF_PREAMBLE_0;
    out_packet->preamble[1] = RF_PREAMBLE_1;
    for (uint8_t i = 0; i < PKT_UID_SIZE; i++) {
        out_packet->uid[i] = s_pkt.uid_cache[i];
    }
    out_packet->temp_hi = packet_bytes[PKT_IDX_TEMP_HI];
    out_packet->temp_lo = packet_bytes[PKT_IDX_TEMP_LO];
    out_packet->hum_hi  = packet_bytes[PKT_IDX_HUM_HI];
    out_packet->hum_lo  = packet_bytes[PKT_IDX_HUM_LO];
    out_packet->sys_status = status;
    out_packet->crc8   = packet_bytes[PKT_IDX_CRC];

    return ERR_OK;
}

/* ====================================================================
 * Packet Validation
 * ==================================================================== */

bool pkt_verify_crc(const rf_packet_t *packet)
{
    const uint8_t *bytes;

    if (packet == NULL) {
        return false;
    }

    bytes = (const uint8_t *)packet;

    /* Compute CRC over bytes 0-12 and compare with stored CRC at byte 13 */
    uint8_t computed = pkt_crc8_compute(bytes, PKT_CRC_COVERED_BYTES);
    return (computed == packet->crc8);
}

/* ====================================================================
 * UID Access
 * ==================================================================== */

int32_t pkt_read_uid(uint8_t *out_uid)
{
    uint8_t raw_uid[CHIP_UID_LENGTH];

    if (out_uid == NULL) {
        return ERR_INVALID_PARAM;
    }

    /* Read from MCU digital signature memory */
    /* Source: inputs/mcu_sdk/Libraries/inc/cw32l010_digitalsign.h#L68 */
    DIGITALSIGN_GetChipUid(raw_uid);

    /* Copy all 10 bytes */
    for (uint8_t i = 0; i < CHIP_UID_LENGTH; i++) {
        out_uid[i] = raw_uid[i];
    }

    return ERR_OK;
}

const uint8_t *pkt_get_cached_uid(void)
{
    if (s_pkt.state != PKT_STATE_READY) {
        return NULL;
    }
    return s_pkt.uid_cache;
}
