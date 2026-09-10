/**
 * @file    interface.h
 * @brief   Phase 2.7 — sensor_mgr: Exported API for upper layers (schedule, rf_mgr)
 *
 * MCU:     CW32L010Y8M6 (TSSOP-20)
 * Module:  sensor_mgr (layer=application)
 * Bus:     I2C (via AHT10 driver)
 *
 * This module orchestrates the sensor measurement cycle:
 *   1. Read AHT10 (sample #1)
 *   2. Wait ~100ms
 *   3. Read AHT10 (sample #2)
 *   4. Compute average temperature and humidity
 *   5. Build RF packet via packet_builder
 *   6. Print result via console (compile-time toggle)
 *   7. Return averaged data + RF packet for RF transmission
 *
 * Sources:
 *   - aht10_init:        output/modules/2.2_aht10_driver/interface.h#L96
 *   - aht10_measure:     output/modules/2.2_aht10_driver/interface.h#L129
 *   - aht10_data_t:      output/modules/2.2_aht10_driver/interface.h#L72-L77
 *   - pkt_builder_init:  output/modules/2.6_packet_builder/interface.h#L81
 *   - pkt_build:         output/modules/2.6_packet_builder/interface.h#L118
 *   - sensor_data_t:     output/design/common_types.h#L57-L62
 *   - rf_packet_t:       output/design/common_types.h#L94-L103
 *   - CONSOLE_PRINT:     output/modules/2.3_uart_console/interface.h#L129
 *   - power_mgr_is_battery_low: output/modules/2.5_power_manager/interface.h#L202
 *   - COMMON.md#1.2 Execution Flow
 */

#ifndef SENSOR_MGR_INTERFACE_H
#define SENSOR_MGR_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>
#include "./../output/design/common_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ====================================================================
 * Constants
 * ==================================================================== */

/** @brief Maximum number of consecutive read failures before reporting error */
#define SENSOR_MGR_MAX_FAILURES       3U

/** @brief Interval between sample #1 and sample #2 in milliseconds */
#define SENSOR_MGR_SAMPLE_INTERVAL_MS 100U

/* ====================================================================
 * Module State
 * ==================================================================== */

/** @brief Sensor manager operating state */
typedef enum {
    SENSOR_MGR_STATE_UNINIT = 0,    /**< Module not initialized */
    SENSOR_MGR_STATE_READY           /**< Module initialized, ready to measure */
} sensor_mgr_state_t;

/** @brief Sensor manager status snapshot */
typedef struct {
    sensor_mgr_state_t  state;              /**< Module state */
    uint32_t            total_cycles;       /**< Total measurement cycles completed */
    uint32_t            failed_cycles;      /**< Cycles where both samples failed */
    uint32_t            stale_cycles;        /**< Cycles where sample #2 failed, used #1 */
    int16_t             last_temp_x10;      /**< Last temperature value (0.1\u00b0C) */
    uint16_t            last_hum_x10;       /**< Last humidity value (0.1% RH) */
    bool                battery_low;        /**< Last known battery status */
} sensor_mgr_status_t;

/* ====================================================================
 * Initialization
 * ==================================================================== */

/**
 * @brief  Initialize the sensor manager module
 *
 * Performs:
 *   1. Call aht10_init() to initialize the AHT10 sensor
 *   2. Call pkt_builder_init() to cache MCU UID for packet assembly
 *   3. Clear internal state counters
 *
 * @return ERR_OK on success
 * @return ERR_NOT_INIT if AHT10 init fails
 * @return ERR_BUS if packet builder init fails
 *
 * @note  Safe to call multiple times; re-initializes sub-modules each time.
 *
 * Source: aht10_init [output/modules/2.2_aht10_driver/interface.h#L96]
 * Source: pkt_builder_init [output/modules/2.6_packet_builder/interface.h#L81]
 */
int32_t sensor_mgr_init(void);

/**
 * @brief  De-initialize the sensor manager module
 *
 * Calls aht10_deinit() and pkt_builder_deinit().
 * After this call, sensor_mgr_run() returns ERR_NOT_INIT.
 *
 * Source: aht10_deinit [output/modules/2.2_aht10_driver/interface.h#L104]
 * Source: pkt_builder_deinit [output/modules/2.6_packet_builder/interface.h#L89]
 */
void sensor_mgr_deinit(void);

/* ====================================================================
 * Measurement Cycle
 * ==================================================================== */

/**
 * @brief  Execute one complete measurement cycle (blocking, ~260ms)
 *
 * Sequence:
 *   1. Check battery status via power_mgr_is_battery_low()
 *   2. Read AHT10 (sample #1) — blocking ~80ms (75ms measurement + overhead)
 *   3. Wait SENSOR_MGR_SAMPLE_INTERVAL_MS (~100ms)
 *   4. Read AHT10 (sample #2) — blocking ~80ms
 *   5. Compute average of both samples
 *   6. Call pkt_build() to assemble the 14-byte RF packet
 *   7. Print result to console (compile-time toggle via CONSOLE_PRINT)
 *   8. Populate output pointers
 *
 * Sample failure handling:
 *   - If sample #1 fails: set SENSOR_STATUS_READ_FAIL, skip averaging
 *   - If sample #2 fails: use sample #1 alone with SENSOR_STATUS_STALE flag
 *   - If both fail:      return ERR_READ_FAIL
 *
 * @param  out_data    [out] Pointer to receive averaged sensor_data_t (may be NULL)
 * @param  out_packet  [out] Pointer to receive built rf_packet_t (may be NULL)
 *
 * @return ERR_OK on success (averaged data + RF packet built)
 * @return ERR_READ_FAIL if both sensor samples failed
 * @return ERR_NOT_INIT if sensor_mgr_init() not called
 * @return ERR_INVALID_PARAM if both out pointers are NULL
 *
 * @note  This function blocks for approximately 260ms (2 x 75ms AHT10 wait +
 *        100ms inter-sample delay + ~10ms computation).
 *        For the project's 150-second measurement cycle, this is acceptable.
 *
 * Source: COMMON.md#1.2 Execution Flow
 * Source: aht10_measure [output/modules/2.2_aht10_driver/interface.h#L129]
 * Source: pkt_build [output/modules/2.6_packet_builder/interface.h#L118]
 * Source: power_mgr_is_battery_low [output/modules/2.5_power_manager/interface.h#L202]
 */
int32_t sensor_mgr_run(sensor_data_t *out_data, rf_packet_t *out_packet);

/**
 * @brief  Get a copy of the last measured sensor data
 *
 * Returns a pointer to the internally cached last measurement result.
 * Valid only after at least one successful sensor_mgr_run() call.
 *
 * @return const sensor_data_t*  Pointer to cached data, or NULL if never measured
 */
const sensor_data_t *sensor_mgr_get_last_data(void);

/**
 * @brief  Get the last built RF packet
 *
 * Returns a pointer to the internally cached last RF packet.
 * Valid only after at least one successful sensor_mgr_run() call.
 *
 * @return const rf_packet_t*  Pointer to cached packet, or NULL if never built
 */
const rf_packet_t *sensor_mgr_get_last_packet(void);

/* ====================================================================
 * Status / Diagnostics
 * ==================================================================== */

/**
 * @brief  Get sensor manager diagnostic status
 *
 * @param  status  [out] Pointer to sensor_mgr_status_t to fill
 * @return ERR_OK on success, ERR_INVALID_PARAM if NULL pointer
 */
int32_t sensor_mgr_get_status(sensor_mgr_status_t *status);

/**
 * @brief  Reset the consecutive failure counter
 *
 * After a successful measurement cycle, the failure counter is automatically
 * reset. This function allows manual reset for testing or recovery.
 */
void sensor_mgr_reset_failures(void);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_MGR_INTERFACE_H */
