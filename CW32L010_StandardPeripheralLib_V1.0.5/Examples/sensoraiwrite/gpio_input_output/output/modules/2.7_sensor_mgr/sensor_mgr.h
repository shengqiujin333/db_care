/**
 * @file    sensor_mgr.h
 * @brief   Phase 2.7 — sensor_mgr: Internal Header (Module Private)
 *
 * MCU:     CW32L010Y8M6 (TSSOP-20)
 * Module:  sensor_mgr (layer=application)
 *
 * This header contains the internal state structure and private declarations
 * for the sensor manager. External consumers should include "interface.h".
 *
 * == Data Flow ==
 *   sensor_mgr_run() is called by schedule (2.8) on each RTC wake cycle.
 *   It orchestrates the measurement, averaging, packet building, and console output.
 *
 * Sources:
 *   - output/design/common_types.h        (sensor_data_t, rf_packet_t, error_code_t)
 *   - output/modules/2.2_aht10_driver/interface.h (aht10_data_t, aht10_measure)
 *   - output/modules/2.6_packet_builder/interface.h (pkt_build)
 *   - output/modules/2.5_power_manager/interface.h  (power_mgr_is_battery_low)
 *   - output/modules/2.3_uart_console/interface.h   (CONSOLE_PRINT)
 *   - COMMON.md#1.2  (Execution flow: sample, avg, TX, print, sleep)
 */

#ifndef SENSOR_MGR_H
#define SENSOR_MGR_H

#include "interface.h"
#include "./../output/design/common_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ====================================================================
 * SDK / Platform Includes
 * ==================================================================== */

#include "cw32l010.h"
#include "cw32l010_systick.h"             /* SysTickDelay, GetTick */

/* ====================================================================
 * Module Dependencies (external modules)
 * ==================================================================== */

#include "./../output/modules/2.2_aht10_driver/interface.h"    /* aht10_data_t, aht10_measure */
#include "./../output/modules/2.2_aht10_driver/aht10_driver.h" /* AHT10_MEAS_DELAY_MS */
#include "./../output/modules/2.6_packet_builder/interface.h"  /* pkt_build, pkt_builder_init */
#include "./../output/modules/2.5_power_manager/interface.h"   /* power_mgr_is_battery_low */
#include "./../output/modules/2.3_uart_console/interface.h"    /* CONSOLE_PRINT */

/* ====================================================================
 * Internal Constants
 * ==================================================================== */

/** @brief Maximum reasonable temperature in 0.1\u00b0C (+85\u00b0C for AHT10) */
#define SENSOR_MGR_TEMP_MAX_X10          850

/** @brief Minimum reasonable temperature in 0.1\u00b0C (-40\u00b0C for AHT10) */
#define SENSOR_MGR_TEMP_MIN_X10         (-400)

/** @brief Maximum reasonable humidity in 0.1% (100.0%) */
#define SENSOR_MGR_HUM_MAX_X10          1000U

/** @brief Minimum reasonable humidity in 0.1% (0.0%) */
#define SENSOR_MGR_HUM_MIN_X10          0U

/* ====================================================================
 * Internal State
 * ==================================================================== */

/**
 * @brief Sensor manager instance data (internal)
 */
typedef struct {
    sensor_mgr_state_t  state;                /**< Module initialization state */
    sensor_data_t       last_data;            /**< Cached last measurement result */
    rf_packet_t         last_packet;          /**< Cached last RF packet */
    bool                data_valid;           /**< Whether last_data has valid content */
    bool                packet_valid;         /**< Whether last_packet has valid content */
    uint32_t            total_cycles;         /**< Total measurement cycles */
    uint32_t            failed_cycles;        /**< Cycles where both samples failed */
    uint32_t            stale_cycles;         /**< Cycles using only 1 sample (stale) */
    uint32_t            consecutive_failures; /**< Consecutive failed cycles */
    int16_t             last_temp_x10;        /**< Last valid temperature */
    uint16_t            last_hum_x10;         /**< Last valid humidity */
    bool                last_battery_low;     /**< Last battery status */
} sensor_mgr_inst_t;

/* ====================================================================
 * Internal Function Declarations
 * ==================================================================== */

/**
 * @brief  Take a single AHT10 measurement and convert to sensor_data_t format
 *
 * Calls aht10_measure(), then converts aht10_data_t to sensor_data_t format.
 *
 * @param  out  Pointer to sensor_data_t to fill
 * @return ERR_OK on success, or aht10 error code on failure
 */
static int32_t sensor_mgr_take_sample(sensor_data_t *out);

/**
 * @brief  Average two sensor_data_t values
 *
 * Computes (s1 + s2) / 2 for both temperature and humidity.
 * If a sample has SENSOR_STATUS_READ_FAIL set, it is excluded from averaging.
 *
 * @param  s1     First sample
 * @param  s2     Second sample
 * @param  out    Pointer to averaged result
 * @return number of valid samples used (0, 1, or 2)
 */
static int sensor_mgr_average(const sensor_data_t *s1,
                               const sensor_data_t *s2,
                               sensor_data_t *out);

/**
 * @brief  Validate sensor data against reasonable ranges
 *
 * Checks temperature and humidity against min/max limits.
 *
 * @param  data  Pointer to sensor data to validate
 * @return true if values are within range, false otherwise
 */
static bool sensor_mgr_validate_data(const sensor_data_t *data);

/**
 * @brief  The module instance (private, defined in sensor_mgr.c)
 */
extern sensor_mgr_inst_t g_sensor_mgr;

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_MGR_H */
