/**
 * @file    i2c_driver.h
 * @brief   Bit-bang I2C Master Driver for CW32L010 (PA03=SDA, PA04=SCL)
 * @author  Auto-generated (EmbedDevOps)
 * @date    2025-07-17
 *
 * == Architecture Note ==
 * The AHT10 is wired to PA03 (SDA) and PA04 (SCL) per connectivity.json.
 * Per the CW32L010 GPIO alternate function register definitions
 * [cw32l010_gpio.h#L352-L368], neither PA03 nor PA04 supports a hardware
 * I2C alternate function. Hence this driver uses GPIO bit-banging to
 * implement standard I2C master protocol at ~100 kHz.
 *
 * Pin Configuration:
 *   - PA04 (SCL, pin 14): GPIO open-drain output, pulled high by external 4.7kΩ
 *   - PA03 (SDA, pin 13): GPIO open-drain output, pulled high by external 4.7kΩ
 *
 * Timing (at HCLK = 48 MHz / PCLK = 48 MHz):
 *   - SCL half-period delay: ~5 us -> 100 kHz
 *   - SDA setup before SCL rising: ~1 us
 *   - SDA hold after SCL falling: ~1 us
 *   - Timeout: 10 ms (max SCL low stretch)
 *
 * Sources:
 *   - Pin mux:    meta/connectivity.json#components[0].pins[13,14]
 *   - AF check:   inputs/mcu_sdk/Libraries/inc/cw32l010_gpio.h#L352-L368
 *   - GPIO init:  inputs/mcu_sdk/Libraries/inc/cw32l010_gpio.h
 *   - Delay API:  inputs/mcu_sdk/Libraries/inc/cw32l010_systick.h
 *   - I2C spec:   UM10204 (NXP), Standard-mode 100 kHz
 */

#ifndef I2C_DRIVER_H
#define I2C_DRIVER_H

#include "interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* Hardware Mapping (Private to this module, exposed for debug)               */
/* ========================================================================== */

/** I2C SCL pin: PA04 (net N05845), MCU pin 14 */
#define I2C_SCL_PORT       CW_GPIOA
#define I2C_SCL_PIN        GPIO_PIN_4

/** I2C SDA pin: PA03 (net N06024), MCU pin 13 */
#define I2C_SDA_PORT       CW_GPIOA
#define I2C_SDA_PIN        GPIO_PIN_3

/* ========================================================================== */
/* Timing Constants (@ 48 MHz PCLK)                                          */
/* ========================================================================== */

/** SCL half-period in microseconds (5 us -> 100 kHz) */
#define I2C_HALF_PERIOD_US    5U

/** SDA setup time before SCL rising (min 250 ns, use 1 us) */
#define I2C_SDA_SETUP_US      1U

/** SDA hold time after SCL falling (min 0 ns, use 1 us) */
#define I2C_SDA_HOLD_US       1U

/** Bus timeout in milliseconds */
#define I2C_BUS_TIMEOUT_MS    10U

/** Maximum number of SCL pulses for bus clear */
#define I2C_BUS_CLEAR_PULSES  9U

/* ========================================================================== */
/* Debug Assert (can be overridden at build time)                             */
/* ========================================================================== */
#ifndef I2C_ASSERT
    #if defined(DEBUG) || defined(I2C_DRV_DEBUG)
        #include <assert.h>
        #define I2C_ASSERT(x)  assert(x)
    #else
        #define I2C_ASSERT(x)  ((void)0)
    #endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* I2C_DRIVER_H */
