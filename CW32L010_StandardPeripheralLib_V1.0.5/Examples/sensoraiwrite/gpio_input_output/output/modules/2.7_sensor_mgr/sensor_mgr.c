/**
 * @file    sensor_mgr.c
 * @brief   Phase 2.7 — sensor_mgr: Implementation
 *
 * MCU:     CW32L010Y8M6 (TSSOP-20)
 * Module:  sensor_mgr (layer=application)
 *
 * == Design ==
 * This module implements the application-layer orchestration of sensor
 * measurement, averaging, packet building, and console output. It sits
 * above the driver layer and is called by the schedule module (2.8)
 * on each 150-second RTC wake cycle.
 *
 * == Execution Flow (sensor_mgr_run) ==
 *   1. Check battery low via power_mgr_is_battery_low()
 *   2. aht10_measure(&sample1) — blocking ~80ms
 *   3. SysTickDelay(100ms) — inter-sample gap
 *   4. aht10_measure(&sample2) — blocking ~80ms
 *   5. Average valid samples
 *   6. Call pkt_build() for RF packet
 *   7. CONSOLE_PRINT the result
 *   8. Store in internal cache
 *   9. Return pointers to caller
 *
 * == Error Handling ==
 *   - Sample #1 fail:  try sample #2 alone (mark STALE if #2 succeeds)
 *   - Sample #2 fail:  use sample #1 alone (mark STALE)
 *   - Both fail:       set READ_FAIL, return ERR_READ_FAIL
 *   - Consecutive failures > SENSOR_MGR_MAX_FAILURES: still attempts
 *
 * Sources:
 *   - aht10_init:        output/modules/2.2_aht10_driver/interface.h#L96
 *   - aht10_measure:     output/modules/2.2_aht10_driver/interface.h#L129
 *   - aht10_data_t:      output/modules/2.2_aht10_driver/interface.h#L72-L77
 *   - AHT10_MEAS_DELAY_MS: output/modules/2.2_aht10_driver/aht10_driver.h#L72
 *   - pkt_builder_init:  output/modules/2.6_packet_builder/interface.h#L81
 *   - pkt_build:         output/modules/2.6_packet_builder/interface.h#L118
 *   - pkt_status_byte:   output/modules/2.6_packet_builder/interface.h#L217
 *   - sensor_data_t:     output/design/common_types.h#L57-L62
 *   - rf_packet_t:       output/design/common_types.h#L94-L103
 *   - SENSOR_STATUS_*:   output/design/common_types.h#L65-L68
 *   - SYS_STATUS_*:      output/design/common_types.h#L110-L112
 *   - CONSOLE_PRINT:     output/modules/2.3_uart_console/interface.h#L129
 *   - power_mgr_is_battery_low: output/modules/2.5_power_manager/interface.h#L202
 *   - SysTickDelay:      inputs/mcu_sdk/Libraries/inc/cw32l010_systick.h#L89
 *   - GetTick:           inputs/mcu_sdk/Libraries/inc/cw32l010_systick.h#L90
 *   - COMMON.md#1.2 Execution Flow
 */

#include "sensor_mgr.h"

/* ====================================================================
 * Module Instance (private)
 * ==================================================================== */

sensor_mgr_inst_t g_sensor_mgr;

/* ====================================================================
 * Static Helper: Validate Sensor Data
 * ==================================================================== */

static bool sensor_mgr_validate_data(const sensor_data_t *data)
{
    if (data == NULL) {
        return false;
    }

    /* Temperature: -40.0 to +85.0 degC in 0.1 units */
    if ((data->temperature_x10 < SENSOR_MGR_TEMP_MIN_X10) ||
        (data->temperature_x10 > SENSOR_MGR_TEMP_MAX_X10)) {
        return false;
    }

    /* Humidity: 0.0% to 100.0% in 0.1 units */
    if ((data->humidity_x10 < SENSOR_MGR_HUM_MIN_X10) ||
        (data->humidity_x10 > SENSOR_MGR_HUM_MAX_X10)) {
        return false;
    }

    return true;
}

/* ====================================================================
 * Static Helper: Take Single Sample
 * ==================================================================== */

static int32_t sensor_mgr_take_sample(sensor_data_t *out)
{
    aht10_data_t   aht10_result;
    aht10_error_t  aht10_ret;

    if (out == NULL) {
        return ERR_INVALID_PARAM;
    }

    /* Initialize output with failure defaults */
    out->temperature_x10 = 0;
    out->humidity_x10    = 0;
    out->status          = SENSOR_STATUS_READ_FAIL;
    out->timestamp_ms    = GetTick();

    /* Take AHT10 measurement (blocking, ~75-80ms) */
    aht10_ret = aht10_measure(&aht10_result);

    if (aht10_ret != AHT10_OK) {
        /* Measurement failed */
        LOG_WARN("sensor_mgr: AHT10 measure failed err=%d", (int)aht10_ret);
        return ERR_READ_FAIL;
    }

    /* Convert aht10_data_t to sensor_data_t */
    out->temperature_x10 = aht10_result.temperature_x10;
    out->humidity_x10    = aht10_result.humidity_x10;
    out->timestamp_ms    = aht10_result.timestamp_ms;

    /* Validate against reasonable ranges */
    if (!sensor_mgr_validate_data(out)) {
        LOG_WARN("sensor_mgr: AHT10 data out of range: T=%d H=%u",
                 (int)out->temperature_x10, (unsigned)out->humidity_x10);
        out->status = SENSOR_STATUS_READ_FAIL;
        return ERR_READ_FAIL;
    }

    out->status = SENSOR_STATUS_VALID;
    return ERR_OK;
}

/* ====================================================================
 * Static Helper: Average Two Samples
 * ==================================================================== */

static int sensor_mgr_average(const sensor_data_t *s1,
                               const sensor_data_t *s2,
                               sensor_data_t *out)
{
    int valid_count = 0;
    int32_t temp_sum = 0;
    uint32_t hum_sum = 0;

    if ((s1 == NULL) || (s2 == NULL) || (out == NULL)) {
        return 0;
    }

    /* Initialize output */
    out->temperature_x10 = 0;
    out->humidity_x10    = 0;
    out->timestamp_ms    = GetTick();

    /* Accumulate sample #1 */
    if (s1->status & SENSOR_STATUS_VALID) {
        temp_sum += (int32_t)s1->temperature_x10;
        hum_sum  += (uint32_t)s1->humidity_x10;
        valid_count++;
    }

    /* Accumulate sample #2 */
    if (s2->status & SENSOR_STATUS_VALID) {
        temp_sum += (int32_t)s2->temperature_x10;
        hum_sum  += (uint32_t)s2->humidity_x10;
        valid_count++;
    }

    /* Compute average based on valid sample count */
    if (valid_count == 0) {
        out->status = SENSOR_STATUS_READ_FAIL;
        return 0;
    }

    if (valid_count == 1) {
        /* Only one valid sample — use it directly, mark as stale */
        if (s1->status & SENSOR_STATUS_VALID) {
            out->temperature_x10 = s1->temperature_x10;
            out->humidity_x10    = s1->humidity_x10;
        } else {
            out->temperature_x10 = s2->temperature_x10;
            out->humidity_x10    = s2->humidity_x10;
        }
        out->status = SENSOR_STATUS_VALID | SENSOR_STATUS_STALE;
    } else {
        /* Two valid samples — compute integer average */
        out->temperature_x10 = (int16_t)(temp_sum / 2);
        out->humidity_x10    = (uint16_t)(hum_sum / 2);
        out->status = SENSOR_STATUS_VALID;
    }

    return valid_count;
}

/* ====================================================================
 * Public API: Initialization
 * ==================================================================== */

int32_t sensor_mgr_init(void)
{
    int32_t ret;

    /* Reset internal state */
    g_sensor_mgr.state                = SENSOR_MGR_STATE_UNINIT;
    g_sensor_mgr.total_cycles         = 0;
    g_sensor_mgr.failed_cycles        = 0;
    g_sensor_mgr.stale_cycles         = 0;
    g_sensor_mgr.consecutive_failures = 0;
    g_sensor_mgr.data_valid           = false;
    g_sensor_mgr.packet_valid         = false;
    g_sensor_mgr.last_temp_x10        = 0;
    g_sensor_mgr.last_hum_x10         = 0;
    g_sensor_mgr.last_battery_low     = false;

    /* Initialize AHT10 sensor driver */
    ret = aht10_init();
    if (ret != AHT10_OK) {
        LOG_ERROR("sensor_mgr: aht10_init failed err=%d", (int)ret);
        return ERR_NOT_INIT;
    }

    /* Initialize packet builder (caches MCU UID) */
    ret = pkt_builder_init();
    if (ret != ERR_OK) {
        LOG_ERROR("sensor_mgr: pkt_builder_init failed err=%d", (int)ret);
        return ERR_BUS;
    }

    g_sensor_mgr.state = SENSOR_MGR_STATE_READY;
    LOG_INFO("sensor_mgr: initialized");

    return ERR_OK;
}

void sensor_mgr_deinit(void)
{
    g_sensor_mgr.state = SENSOR_MGR_STATE_UNINIT;
    g_sensor_mgr.data_valid   = false;
    g_sensor_mgr.packet_valid = false;

    pkt_builder_deinit();
    aht10_deinit();

    LOG_INFO("sensor_mgr: de-initialized");
}

/* ====================================================================
 * Public API: Measurement Cycle
 * ==================================================================== */

int32_t sensor_mgr_run(sensor_data_t *out_data, rf_packet_t *out_packet)
{
    sensor_data_t sample1;
    sensor_data_t sample2;
    sensor_data_t averaged;
    int32_t ret_s1, ret_s2;
    int valid_samples;
    bool bat_low;
    int32_t pkt_ret;

    /* Check initialization */
    if (g_sensor_mgr.state != SENSOR_MGR_STATE_READY) {
        LOG_ERROR("sensor_mgr: not initialized");
        return ERR_NOT_INIT;
    }

    /* At least one output pointer must be provided */
    if ((out_data == NULL) && (out_packet == NULL)) {
        return ERR_INVALID_PARAM;
    }

    /* ---- Step 1: Check battery status ---- */
    bat_low = power_mgr_is_battery_low();
    g_sensor_mgr.last_battery_low = bat_low;

    /* ---- Step 2: Sample #1 ---- */
    LOG_DEBUG("sensor_mgr: taking sample #1");
    ret_s1 = sensor_mgr_take_sample(&sample1);

    if (ret_s1 == ERR_OK) {
        LOG_DEBUG("sensor_mgr: sample #1 T=%d H=%u",
                  (int)sample1.temperature_x10, (unsigned)sample1.humidity_x10);
    } else {
        LOG_WARN("sensor_mgr: sample #1 failed");
    }

    /* ---- Step 3: Inter-sample delay ---- */
    SysTickDelay(SENSOR_MGR_SAMPLE_INTERVAL_MS);

    /* ---- Step 4: Sample #2 ---- */
    LOG_DEBUG("sensor_mgr: taking sample #2");
    ret_s2 = sensor_mgr_take_sample(&sample2);

    if (ret_s2 == ERR_OK) {
        LOG_DEBUG("sensor_mgr: sample #2 T=%d H=%u",
                  (int)sample2.temperature_x10, (unsigned)sample2.humidity_x10);
    } else {
        LOG_WARN("sensor_mgr: sample #2 failed");
    }

    /* ---- Step 5: Compute average ---- */
    valid_samples = sensor_mgr_average(&sample1, &sample2, &averaged);

    if (valid_samples == 0) {
        /* Both samples failed */
        g_sensor_mgr.consecutive_failures++;
        g_sensor_mgr.failed_cycles++;
        g_sensor_mgr.total_cycles++;

        averaged.status = SENSOR_STATUS_READ_FAIL;

        /* Still build a packet with failure status */
        if (out_packet != NULL) {
            pkt_ret = pkt_build(&averaged, bat_low, out_packet);
            if (pkt_ret == ERR_OK) {
                g_sensor_mgr.last_packet = *out_packet;
                g_sensor_mgr.packet_valid = true;
            }
        }

        if (out_data != NULL) {
            *out_data = averaged;
            g_sensor_mgr.last_data = averaged;
            g_sensor_mgr.data_valid = true;
        }

        LOG_ERROR("sensor_mgr: both samples failed (cycle %lu)",
                  (unsigned long)g_sensor_mgr.total_cycles);

        return ERR_READ_FAIL;
    }

    /* Success — reset consecutive failure counter */
    g_sensor_mgr.consecutive_failures = 0;

    /* Track stale cycles */
    if (averaged.status & SENSOR_STATUS_STALE) {
        g_sensor_mgr.stale_cycles++;
    }

    g_sensor_mgr.total_cycles++;
    g_sensor_mgr.last_temp_x10 = averaged.temperature_x10;
    g_sensor_mgr.last_hum_x10  = averaged.humidity_x10;

    LOG_INFO("sensor_mgr: avg T=%d H=%u (samples=%d)",
             (int)averaged.temperature_x10,
             (unsigned)averaged.humidity_x10,
             valid_samples);

    /* ---- Step 6: Build RF packet ---- */
    if (out_packet != NULL) {
        pkt_ret = pkt_build(&averaged, bat_low, out_packet);
        if (pkt_ret == ERR_OK) {
            g_sensor_mgr.last_packet = *out_packet;
            g_sensor_mgr.packet_valid = true;
        } else {
            LOG_WARN("sensor_mgr: pkt_build failed err=%d", (int)pkt_ret);
        }
    }

    /* ---- Step 7: Console output ---- */
    CONSOLE_PRINT("[T=%d.%dC H=%u.%u%%]\r\n",
                  (int)(averaged.temperature_x10 / 10),
                  (int)(averaged.temperature_x10 % 10),
                  (unsigned)(averaged.humidity_x10 / 10),
                  (unsigned)(averaged.humidity_x10 % 10));

    /* ---- Step 8: Populate output ---- */
    if (out_data != NULL) {
        *out_data = averaged;
    }

    /* Update internal cache */
    g_sensor_mgr.last_data = averaged;
    g_sensor_mgr.data_valid = true;

    return ERR_OK;
}

/* ====================================================================
 * Public API: Data Retrieval
 * ==================================================================== */

const sensor_data_t *sensor_mgr_get_last_data(void)
{
    if (!g_sensor_mgr.data_valid) {
        return NULL;
    }
    return &g_sensor_mgr.last_data;
}

const rf_packet_t *sensor_mgr_get_last_packet(void)
{
    if (!g_sensor_mgr.packet_valid) {
        return NULL;
    }
    return &g_sensor_mgr.last_packet;
}

/* ====================================================================
 * Public API: Status / Diagnostics
 * ==================================================================== */

int32_t sensor_mgr_get_status(sensor_mgr_status_t *status)
{
    if (status == NULL) {
        return ERR_INVALID_PARAM;
    }

    status->state          = g_sensor_mgr.state;
    status->total_cycles   = g_sensor_mgr.total_cycles;
    status->failed_cycles  = g_sensor_mgr.failed_cycles;
    status->stale_cycles   = g_sensor_mgr.stale_cycles;
    status->last_temp_x10  = g_sensor_mgr.last_temp_x10;
    status->last_hum_x10   = g_sensor_mgr.last_hum_x10;
    status->battery_low    = g_sensor_mgr.last_battery_low;

    return ERR_OK;
}

void sensor_mgr_reset_failures(void)
{
    g_sensor_mgr.consecutive_failures = 0;
}
