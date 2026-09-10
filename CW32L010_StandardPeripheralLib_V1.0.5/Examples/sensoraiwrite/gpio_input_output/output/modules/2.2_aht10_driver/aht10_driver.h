/**
 * @file    aht10_driver.h
 * @brief   AHT10 Driver — Internal Header (Module Private)
 * @author  Auto-generated (EmbedDevOps)
 * @date    2025-07-17
 *
 * == Hardware Mapping ==
 * I2C bus: Bit-bang master via i2c_drv (module 2.1)
 *   SCL: PA04 (net N05845), MCU pin 14
 *   SDA: PA03 (net N06024), MCU pin 13
 *   Pull-ups: R1=4.7k (SCL), R2=4.7k (SDA) to BAT rail
 *
 * Sources:
 *   - Pin assignment:     meta/connectivity.json#buses[I2C]
 *   - AF verification:    inputs/mcu_sdk/Libraries/inc/cw32l010_gpio.h#L352-L368
 *   - I2C driver:         output/modules/2.1_i2c_driver/interface.h
 *   - AHT10 protocol:     output/modules/2.1_i2c_driver/protocol_notes.md
 */

#ifndef AHT10_DRIVER_H
#define AHT10_DRIVER_H

#include "interface.h"   /* Public API types */

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* Device Constants                                                           */
/* ========================================================================== */

/** AHT10 I2C 7-bit address */
#define AHT10_I2C_ADDR              0x38U

/** I2C write byte (addr << 1 | 0) */
#define AHT10_ADDR_WRITE            ((uint8_t)(AHT10_I2C_ADDR << 1))

/** I2C read byte (addr << 1 | 1) */
#define AHT10_ADDR_READ             ((uint8_t)((AHT10_I2C_ADDR << 1) | 0x01))

/* ========================================================================== */
/* Command Definitions                                                        */
/* ========================================================================== */

/** Initialize sensor (sets calibration coeffs) */
#define AHT10_CMD_INIT              0xBEU

/** Trigger measurement */
#define AHT10_CMD_TRIGGER           0xACU

/** Trigger argument byte 1 */
#define AHT10_CMD_TRIGGER_ARG1      0x33U

/** Trigger argument byte 2 */
#define AHT10_CMD_TRIGGER_ARG2      0x00U

/** Soft reset */
#define AHT10_CMD_SOFT_RESET        0xBAU

/* ========================================================================== */
/* Timing Constants                                                           */
/* ========================================================================== */

/** Power-up stabilization time (ms) */
#define AHT10_POWERUP_DELAY_MS      20U

/** Post-init wait time (ms) */
#define AHT10_INIT_DELAY_MS         20U

/** Measurement wait time (ms) - sensor needs >=75 ms */
#define AHT10_MEAS_DELAY_MS         75U

/** Maximum additional wait if sensor busy (ms) */
#define AHT10_BUSY_TIMEOUT_MS       100U

/** Soft reset wait time (ms) */
#define AHT10_RESET_DELAY_MS        20U

/* ========================================================================== */
/* Read Data Format                                                           */
/* ========================================================================== */

/** Number of bytes read from AHT10 after trigger */
#define AHT10_READ_LEN              6U

/** Status byte bit 7: 1 = busy (measuring), 0 = ready */
#define AHT10_STATUS_BUSY_BIT       ((uint8_t)0x80U)

/* ========================================================================== */
/* Conversion Constants                                                       */
/* ========================================================================== */

/** 2^20 = 1048576, used in raw-to-physical conversion */
#define AHT10_RAW_MAX               1048576UL

/** Humidity multiplier: raw * 1000 / RAW_MAX -> %RH x10 */
#define AHT10_HUM_MULTIPLIER        1000UL

/** Temperature multiplier: raw * 2000 / RAW_MAX -> degC x10, then -500 offset */
#define AHT10_TEMP_MULTIPLIER       2000UL

/** Temperature offset (subtract after scaling) for degC x10 */
#define AHT10_TEMP_OFFSET           500

/* ========================================================================== */
/* Driver State                                                               */
/* ========================================================================== */

/**
 * @brief AHT10 driver state structure (private)
 */
typedef struct {
    uint8_t initialized : 1;   /**< 1 after aht10_init() succeeds */
    uint8_t reserved    : 7;   /**< Reserved for future use */
} aht10_driver_state_t;

#ifdef __cplusplus
}
#endif

#endif /* AHT10_DRIVER_H */
