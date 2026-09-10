/**
 * @file    packet_builder.h
 * @brief   Phase 2.6 — packet_builder: Internal header
 *
 * MCU:     CW32L010Y8M6 (TSSOP-20)
 * Module:  packet_builder (layer=service)
 *
 * This header contains private types, internal state, and helper declarations
 * for the packet builder module. Consumers should include "interface.h".
 *
 * Sources:
 *   - output/design/common_types.h          (rf_packet_t, sensor_data_t, error_code_t)
 *   - inputs/mcu_sdk/Libraries/inc/cw32l010_digitalsign.h  (DIGITALSIGN_GetChipUid)
 *   - CW32L010_DataSheet_CN_V1.0.pdf#page=20 (CRC peripheral available but unused)
 */

#ifndef PACKET_BUILDER_H
#define PACKET_BUILDER_H

#include "interface.h"
#include "./../output/design/common_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ====================================================================
 * SDK Includes
 * ==================================================================== */

#include "cw32l010.h"
#include "cw32l010_digitalsign.h"

/* ====================================================================
 * Internal Constants
 * ==================================================================== */

/** @brief CRC-8/MAXIM polynomial: x^8 + x^5 + x^4 + 1 = 0x131 -> 0x31 */
#define PKT_CRC8_POLY               0x31U

/** @brief Initial value for CRC-8/MAXIM */
#define PKT_CRC8_INIT               0x00U

/** @brief Number of bytes covered by CRC (preamble + UID + temp + hum + status) */
#define PKT_CRC_COVERED_BYTES       13U

/* ====================================================================
 * Internal State
 * ==================================================================== */

/** @brief Packet builder module state */
typedef enum {
    PKT_STATE_UNINIT = 0,   /**< Module not initialized */
    PKT_STATE_READY          /**< Module initialized, UID cached */
} pkt_state_t;

/** @brief Packet builder instance data */
typedef struct {
    pkt_state_t  state;         /**< Module state */
    uint8_t      uid_cache[PKT_UID_SIZE]; /**< Cached 6-byte UID */
    bool         uid_cached;    /**< Whether UID has been read */
} pkt_builder_t;

/* ====================================================================
 * Internal Function Declarations
 * ==================================================================== */

/**
 * @brief  Compute CRC-8/MAXIM using bit-by-bit algorithm
 *
 * Internal implementation used by pkt_crc8().
 * Polynomial: 0x31, initial: 0x00, no final XOR.
 *
 * Source: output/design/common_types.h#L87 (CRC8 specified in rf_packet_t)
 *
 * @param  data    Data buffer
 * @param  length  Number of bytes
 * @return uint8_t CRC result
 */
static inline uint8_t pkt_crc8_compute(const uint8_t *data, uint16_t length)
{
    uint8_t crc = PKT_CRC8_INIT;

    for (uint16_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x80) {
                crc = (uint8_t)((crc << 1) ^ PKT_CRC8_POLY);
            } else {
                crc = (uint8_t)(crc << 1);
            }
        }
    }

    return crc;
}

#ifdef __cplusplus
}
#endif

#endif /* PACKET_BUILDER_H */
