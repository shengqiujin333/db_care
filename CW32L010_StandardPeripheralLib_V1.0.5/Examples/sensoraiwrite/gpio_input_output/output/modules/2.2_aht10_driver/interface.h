/**
 * @file    interface.h
 * @brief   AHT10 Driver — Exported API for Upper Layers
 * @author  Auto-generated (EmbedDevOps)
 * @date    2025-07-17
 *
 * MCU:     CW32L010Y8M6 (TSSOP-20)
 * Bus:     Bit-bang I2C master via i2c_drv (module 2.1) on PA03(SDA)/PA04(SCL)
 * Device:  AHT10 Temperature/Humidity Sensor @ I2C addr 0x38 (7-bit)
 *
 * == Architecture ==
 * This driver depends on module 2.1 (i2c_driver) for low-level I2C transport.
 * The i2c_driver uses GPIO bit-banging because PA03/PA04 do NOT have hardware
 * I2C alternate functions on the TSSOP-20 package.
 *   [inputs/mcu_sdk/Libraries/inc/cw32l010_gpio.h#L352-L368]
 *
 * == Protocol Summary ==
 *   Trigger:  0xAC 0x33 0x00  -> wait >=75 ms
 *   Read:     6 bytes (hum_msb, hum_lsb, hum_xlsb|temp_msb, temp_lsb, temp_xlsb, status)
 *   Addr:     0x38 (7-bit) -> write byte 0x70, read byte 0x71
 *
 * Sources:
 *   - I2C bus assignment: meta/connectivity.json#buses[I2C]
 *   - i2c_drv API:        output/modules/2.1_i2c_driver/interface.h
 *   - AHT10 protocol:     output/modules/2.1_i2c_driver/protocol_notes.md
 *   - SDK GPIO header:    inputs/mcu_sdk/Libraries/inc/cw32l010_gpio.h
 */

#ifndef AHT10_INTERFACE_H
#define AHT10_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* Error Codes                                                                */
/* ========================================================================== */

/**
 * @brief AHT10 driver error codes
 *
 * Positive values = success/warnings.
 * Negative values = errors.
 */
typedef enum {
    AHT10_OK              =  0,  /**< Operation succeeded */
    AHT10_ERR_I2C         = -1,  /**< I2C transport error (NACK, timeout, etc.) */
    AHT10_ERR_BUSY        = -2,  /**< Sensor busy (status bit 7 set after max wait) */
    AHT10_ERR_PARAM       = -3,  /**< Invalid parameter (NULL pointer, etc.) */
    AHT10_ERR_NOT_INIT    = -4,  /**< Driver not initialized */
    AHT10_ERR_CRC         = -5,  /**< CRC mismatch (if CRC byte present, AHT21B mode) */
    AHT10_ERR_TIMEOUT     = -6   /**< Measurement timeout (sensor did not finish) */
} aht10_error_t;

/* ========================================================================== */
/* Data Structures                                                            */
/* ========================================================================== */

/**
 * @brief AHT10 measurement result
 *
 * All values use x10 fixed-point for one decimal place.
 * Example: temperature_x10 = 253 means 25.3 degC
 *          humidity_x10    = 624 means 62.4 %RH
 *
 * The status byte from the sensor is preserved for diagnostics.
 */
typedef struct {
    int16_t  temperature_x10;   /**< Temperature in degC x10 (-400 to +850) */
    uint16_t humidity_x10;      /**< Relative humidity in %RH x10 (0 to 1000) */
    uint8_t  status_byte;       /**< Raw status byte from sensor (bit 7 = busy) */
    uint32_t timestamp_ms;      /**< Local timestamp at read completion (from GetTick) */
} aht10_data_t;

/* ========================================================================== */
/* Initialization & Configuration                                             */
/* ========================================================================== */

/**
 * @brief  Initialize the AHT10 sensor
 *
 * This function:
 *   1. Initializes the I2C driver (i2c_drv_init) if not already done
 *   2. Probes the AHT10 on the I2C bus (address 0x38)
 *   3. Sends the initialization command (0xBE) and waits 20 ms
 *
 * @retval AHT10_OK               Sensor initialized and ready
 * @retval AHT10_ERR_I2C          I2C init failed or sensor did not ACK probe
 *
 * @note  Safe to call multiple times; re-initializes if already called.
 */
aht10_error_t aht10_init(void);

/**
 * @brief  De-initialize the AHT10 sensor
 *
 * Releases resources (currently a no-op since i2c_drv may be shared).
 * Marks driver as uninitialized.
 */
void aht10_deinit(void);

/* ========================================================================== */
/* Measurement Functions                                                      */
/* ========================================================================== */

/**
 * @brief  Trigger a measurement and read result (blocking, polling)
 *
 * Sequence:
 *   1. Send trigger command (0xAC 0x33 0x00) via I2C write
 *   2. Wait 75 ms for sensor to complete measurement
 *   3. Read 6 bytes of result data via I2C read
 *   4. Convert raw data to temperature_x10 and humidity_x10
 *
 * @param  out  Pointer to aht10_data_t to receive the result
 * @retval AHT10_OK               Measurement successful
 * @retval AHT10_ERR_I2C          I2C write or read error
 * @retval AHT10_ERR_BUSY         Sensor still busy after max wait time
 * @retval AHT10_ERR_PARAM        NULL pointer passed
 * @retval AHT10_ERR_NOT_INIT     Driver not initialized (aht10_init not called)
 *
 * @note  This function blocks for approximately 75 ms (measurement wait).
 *        For the project's 150-second measurement cycle, this is acceptable.
 */
aht10_error_t aht10_measure(aht10_data_t *out);

/**
 * @brief  Trigger a measurement without waiting (for non-blocking use)
 *
 * Sends the trigger command and returns immediately.
 * Caller must wait at least 75 ms before calling aht10_read().
 *
 * @retval AHT10_OK               Trigger sent
 * @retval AHT10_ERR_I2C          I2C write error
 */
aht10_error_t aht10_trigger(void);

/**
 * @brief  Read measurement result after aht10_trigger() + delay
 *
 * Reads 6 bytes from the sensor and converts to temperature/humidity.
 * Must be called at least 75 ms after aht10_trigger().
 *
 * @param  out  Pointer to aht10_data_t to receive the result
 * @retval AHT10_OK               Read successful
 * @retval AHT10_ERR_I2C          I2C read error
 * @retval AHT10_ERR_BUSY         Sensor status indicates still busy
 * @retval AHT10_ERR_PARAM        NULL pointer
 */
aht10_error_t aht10_read(aht10_data_t *out);

/* ========================================================================== */
/* Utility Functions                                                          */
/* ========================================================================== */

/**
 * @brief  Probe the AHT10 sensor (check if it ACKs its I2C address)
 * @return true if sensor responds, false otherwise
 */
bool aht10_probe(void);

/**
 * @brief  Soft-reset the AHT10 sensor (command 0xBA)
 * @retval AHT10_OK       Reset command sent
 * @retval AHT10_ERR_I2C  I2C write error
 *
 * After reset, wait 20 ms before sending commands.
 */
aht10_error_t aht10_soft_reset(void);

/**
 * @brief  Return a human-readable string for an error code
 * @param  err  Error code from aht10_error_t
 * @return Pointer to a static string
 */
const char *aht10_strerror(aht10_error_t err);

/**
 * @brief  Convert raw AHT10 6-byte buffer to temperature/humidity
 *
 * Utility function for direct conversion without going through the full
 * measure/read flow.
 *
 * @param  raw   Pointer to 6-byte raw data from sensor
 * @param  out   Pointer to output data structure
 * @retval AHT10_OK       Conversion succeeded
 * @retval AHT10_ERR_PARAM NULL pointer
 */
aht10_error_t aht10_raw_to_data(const uint8_t raw[6], aht10_data_t *out);

#ifdef __cplusplus
}
#endif

#endif /* AHT10_INTERFACE_H */
