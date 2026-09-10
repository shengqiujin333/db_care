/**
 * @file    aht10_driver.c
 * @brief   AHT10 Temperature/Humidity Sensor Driver Implementation
 * @author  Auto-generated (EmbedDevOps)
 * @date    2025-07-17
 *
 * == Dependencies ==
 *   - Module 2.1 (i2c_driver): Provides bit-bang I2C master on PA03/PA04
 *   - SDK: cw32l010_systick.h (SysTickDelay, GetTick)
 *
 * == Pin Mapping (from connectivity.json) ==
 *   I2C SCL = PA04 (net N05845), MCU pin 14
 *   I2C SDA = PA03 (net N06024), MCU pin 13
 *   External pull-ups: R1 = 4.7k (SCL), R2 = 4.7k (SDA)
 *
 * == Protocol ==
 *   I2C addr: 0x38 (7-bit)
 *   Trigger:  START -> SLA+W -> 0xAC 0x33 0x00 -> STOP
 *   Wait:     >=75 ms (sensor measures)
 *   Read:     START -> SLA+R -> [6 bytes] -> STOP
 *   Response: [HUM_MSB][HUM_LSB][HUM_XLSB:4|TEMP_MSB:4][TEMP_LSB][TEMP_XLSB][STATUS]
 *
 * == Conversion Formulas ==
 *   raw_hum = (buf[0] << 12) | (buf[1] << 4) | (buf[2] >> 4)
 *   hum_x10 = (raw_hum * 1000) / 1048576
 *   raw_temp = ((buf[2] & 0x0F) << 16) | (buf[3] << 8) | buf[4]
 *   temp_x10 = (raw_temp * 2000) / 1048576 - 500
 *
 * == Open Issues ==
 *   - No official AHT10 datasheet in inputs/. Command sequence based on
 *     industry-standard implementation. Verify on real hardware.
 *     [output/modules/2.1_i2c_driver/protocol_notes.md#L128-L130]
 *
 * Sources:
 *   - I2C API:     output/modules/2.1_i2c_driver/interface.h
 *   - GPIO check:  inputs/mcu_sdk/Libraries/inc/cw32l010_gpio.h#L352-L368
 *   - Connectivity: meta/connectivity.json#buses[I2C]
 */

/* ========================================================================== */
/* Includes                                                                   */
/* ========================================================================== */

#include "aht10_driver.h"           /* Public + private defines */

/* I2C driver from module 2.1 */
#include "./../output/modules/2.1_i2c_driver/interface.h"  /* i2c_drv_* API */

/* SDK includes */
#include "cw32l010_systick.h"       /* SysTickDelay, GetTick */

/* ========================================================================== */
/* Local State                                                                */
/* ========================================================================== */

/** Driver initialized flag */
static aht10_driver_state_t s_state = { .initialized = 0 };

/* ========================================================================== */
/* Local Helper: Wait for milliseconds                                        */
/* ========================================================================== */

/**
 * @brief  Busy-wait for the specified number of milliseconds
 * @param  ms  Milliseconds to wait
 *
 * Uses SysTickDelay from the CW32L010 SDK.
 * [inputs/mcu_sdk/Libraries/inc/cw32l010_systick.h#L65]
 */
static void aht10_delay_ms(uint32_t ms)
{
    SysTickDelay(ms);
}

/* ========================================================================== */
/* Local Helper: Raw data conversion                                          */
/* ========================================================================== */

/**
 * @brief  Convert AHT10 6-byte raw data to temperature/humidity x10 values
 *
 * AHT10 raw data format (6 bytes):
 *   Byte 0: Humidity[19:12]  (MSB of 20-bit humidity)
 *   Byte 1: Humidity[11:4]
 *   Byte 2: [Humidity[3:0] : Temperature[19:16]] (nibble split)
 *   Byte 3: Temperature[15:8]
 *   Byte 4: Temperature[7:0]
 *   Byte 5: Status byte (bit 7 = busy)
 *
 * Conversion:
 *   RH = (raw_hum * 100) / 2^20  [%]
 *   T  = (raw_temp * 200) / 2^20 - 50  [degC]
 *
 * We scale by 10x for one decimal place:
 *   hum_x10 = (raw_hum * 1000) / 1048576   [%RH x10]
 *   temp_x10 = (raw_temp * 2000) / 1048576 - 500  [degC x10]
 *
 * @param  raw  6-byte input buffer
 * @param  out  Output structure
 * @return AHT10_OK or AHT10_ERR_PARAM
 *
 * Source: output/modules/2.1_i2c_driver/protocol_notes.md#L149-L161
 */
static aht10_error_t raw_to_data(const uint8_t raw[6], aht10_data_t *out)
{
    uint32_t raw_hum;
    uint32_t raw_temp;

    if (!raw || !out) {
        return AHT10_ERR_PARAM;
    }

    /* Extract 20-bit humidity value */
    raw_hum = ((uint32_t)raw[0] << 12) |
              ((uint32_t)raw[1] << 4)  |
              ((uint32_t)raw[2] >> 4);

    /* Extract 20-bit temperature value */
    raw_temp = ((uint32_t)(raw[2] & 0x0F) << 16) |
               ((uint32_t)raw[3] << 8)           |
               ((uint32_t)raw[4]);

    /* Convert to x10 fixed-point */
    out->humidity_x10 = (uint16_t)((raw_hum * AHT10_HUM_MULTIPLIER) / AHT10_RAW_MAX);

    int32_t temp_val = (int32_t)((raw_temp * AHT10_TEMP_MULTIPLIER) / AHT10_RAW_MAX);
    temp_val -= AHT10_TEMP_OFFSET;
    out->temperature_x10 = (int16_t)temp_val;

    /* Preserve status byte */
    out->status_byte = raw[5];

    /* Record timestamp at conversion time */
    out->timestamp_ms = GetTick();

    return AHT10_OK;
}

/* ========================================================================== */
/* Public API Implementation                                                  */
/* ========================================================================== */

aht10_error_t aht10_init(void)
{
    aht10_error_t err;
    i2c_drv_error_t i2c_err;

    /* Initialize I2C bus (safe to call multiple times) */
    i2c_err = i2c_drv_init();
    if (i2c_err != I2C_DRV_OK) {
        /* I2C driver init failed */
        s_state.initialized = 0;
        return AHT10_ERR_I2C;
    }

    /* Probe AHT10 on the bus */
    if (!i2c_drv_probe(AHT10_I2C_ADDR)) {
        /* No ACK from sensor */
        s_state.initialized = 0;
        return AHT10_ERR_I2C;
    }

    /* Send initialization command (0xBE) */
    /* Some AHT10 variants need this to load calibration coefficients */
    const uint8_t init_cmd = AHT10_CMD_INIT;
    i2c_err = i2c_drv_master_write(AHT10_I2C_ADDR, &init_cmd, 1);
    if (i2c_err != I2C_DRV_OK) {
        /* Init command NACKed — sensor may be pre-initialized */
        /* Continue anyway; try measuring */
    }

    /* Wait for sensor to initialize */
    aht10_delay_ms(AHT10_INIT_DELAY_MS);

    s_state.initialized = 1;
    return AHT10_OK;
}

void aht10_deinit(void)
{
    s_state.initialized = 0;
}

aht10_error_t aht10_measure(aht10_data_t *out)
{
    aht10_error_t err;

    if (!out) {
        return AHT10_ERR_PARAM;
    }

    if (!s_state.initialized) {
        return AHT10_ERR_NOT_INIT;
    }

    /* Step 1: Send trigger command */
    err = aht10_trigger();
    if (err != AHT10_OK) {
        return err;
    }

    /* Step 2: Wait for measurement to complete */
    aht10_delay_ms(AHT10_MEAS_DELAY_MS);

    /* Step 3: Read result */
    err = aht10_read(out);
    return err;
}

aht10_error_t aht10_trigger(void)
{
    i2c_drv_error_t i2c_err;

    if (!s_state.initialized) {
        return AHT10_ERR_NOT_INIT;
    }

    /* Trigger sequence: 0xAC 0x33 0x00 */
    const uint8_t trigger_cmd[3] = {
        AHT10_CMD_TRIGGER,
        AHT10_CMD_TRIGGER_ARG1,
        AHT10_CMD_TRIGGER_ARG2
    };

    i2c_err = i2c_drv_master_write(AHT10_I2C_ADDR, trigger_cmd, 3);
    if (i2c_err != I2C_DRV_OK) {
        return AHT10_ERR_I2C;
    }

    return AHT10_OK;
}

aht10_error_t aht10_read(aht10_data_t *out)
{
    i2c_drv_error_t i2c_err;
    uint8_t raw[AHT10_READ_LEN];
    uint32_t timeout;

    if (!out) {
        return AHT10_ERR_PARAM;
    }

    if (!s_state.initialized) {
        return AHT10_ERR_NOT_INIT;
    }

    /* Read 6 bytes from sensor */
    i2c_err = i2c_drv_master_read(AHT10_I2C_ADDR, raw, AHT10_READ_LEN);
    if (i2c_err != I2C_DRV_OK) {
        return AHT10_ERR_I2C;
    }

    /* Check status byte (byte 5, bit 7) for busy flag */
    timeout = AHT10_BUSY_TIMEOUT_MS;
    while ((raw[5] & AHT10_STATUS_BUSY_BIT) && (timeout > 0)) {
        /* Sensor still measuring; wait and retry */
        aht10_delay_ms(5);
        timeout -= 5;
        i2c_err = i2c_drv_master_read(AHT10_I2C_ADDR, raw, AHT10_READ_LEN);
        if (i2c_err != I2C_DRV_OK) {
            return AHT10_ERR_I2C;
        }
    }

    if (raw[5] & AHT10_STATUS_BUSY_BIT) {
        /* Sensor still busy after timeout */
        out->status_byte = raw[5];
        out->timestamp_ms = GetTick();
        return AHT10_ERR_BUSY;
    }

    /* Convert raw 6-byte data to x10 values */
    return raw_to_data(raw, out);
}

bool aht10_probe(void)
{
    if (!s_state.initialized) {
        return false;
    }

    return i2c_drv_probe(AHT10_I2C_ADDR);
}

aht10_error_t aht10_soft_reset(void)
{
    i2c_drv_error_t i2c_err;

    if (!s_state.initialized) {
        return AHT10_ERR_NOT_INIT;
    }

    const uint8_t reset_cmd = AHT10_CMD_SOFT_RESET;
    i2c_err = i2c_drv_master_write(AHT10_I2C_ADDR, &reset_cmd, 1);
    if (i2c_err != I2C_DRV_OK) {
        return AHT10_ERR_I2C;
    }

    /* Wait for sensor to reset */
    aht10_delay_ms(AHT10_RESET_DELAY_MS);

    return AHT10_OK;
}

aht10_error_t aht10_raw_to_data(const uint8_t raw[6], aht10_data_t *out)
{
    return raw_to_data(raw, out);
}

const char *aht10_strerror(aht10_error_t err)
{
    switch (err) {
        case AHT10_OK:           return "OK";
        case AHT10_ERR_I2C:      return "I2C transport error";
        case AHT10_ERR_BUSY:     return "Sensor busy";
        case AHT10_ERR_PARAM:    return "Invalid parameter";
        case AHT10_ERR_NOT_INIT: return "Driver not initialized";
        case AHT10_ERR_CRC:      return "CRC mismatch";
        case AHT10_ERR_TIMEOUT:  return "Measurement timeout";
        default:                 return "Unknown error";
    }
}

/* ========================================================================== */
/* EOF                                                                        */
/* ========================================================================== */
