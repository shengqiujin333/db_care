/**
 * @file    rf_twi_driver.h
 * @brief   Phase 2.4 — rf_twi_driver: Internal header for UM2005C TWI bit-bang driver
 *
 * This header contains private types, macros, and internal function declarations.
 * Consumers should include "interface.h" for the public API.
 *
 * MCU:  CW32L010Y8M6 (TSSOP-20)
 * Bus:  GPIO bit-banged TWI on PB02(CLK)/PB03(DATA)
 * Instance: GPIO_IN_INTERRUPT (connectivity.json bus kind for nets RF_CLK/RF_DATA)
 *
 * Sources:
 *   - cw32l010_gpio.h (GPIO macros: PB02_AFx_GPIO, PB02_DIR_OUTPUT, PB02_SETHIGH, etc.)
 *   - cw32l010_sysctrl.h (__SYSCTRL_GPIOB_CLK_ENABLE)
 *   - system_cw32l010.h (FirmwareDelay)
 *   - connectivity.json#buses[GPIO_IN_INTERRUPT] (RF_CLK=PB02, RF_DATA=PB03)
 *   - UM2005C_数据手册_V1.2_V1.2.pdf#page=12 (TWI protocol)
 */

#ifndef RF_TWI_DRIVER_H
#define RF_TWI_DRIVER_H

#include "interface.h"
#include "./../output/design/common_types.h"

/* ====================================================================
 * SDK Includes
 * ==================================================================== */

#include "cw32l010.h"
#include "cw32l010_sysctrl.h"
#include "cw32l010_gpio.h"

/* ====================================================================
 * Pin Definitions
 * ==================================================================== */

/** @brief TWI CLK pin -- PB02 (MCU pin 19), net RF_CLK
 *  Source: connectivity.json#buses[GPIO_IN_INTERRUPT]; CW32L010_DataSheet_CN_V1.0.pdf#page=24 */
#define RF_TWI_CLK_PORT         CW_GPIOB
#define RF_TWI_CLK_PIN          GPIO_PIN_2
#define RF_TWI_CLK_SET_HIGH()   PB02_SETHIGH()
#define RF_TWI_CLK_SET_LOW()    PB02_SETLOW()
#define RF_TWI_CLK_READ()       PB02_GETVALUE()
#define RF_TWI_CLK_DIR_OUT()    PB02_DIR_OUTPUT()
#define RF_TWI_CLK_DIR_IN()     PB02_DIR_INPUT()
#define RF_TWI_CLK_AF_GPIO()    PB02_AFx_GPIO()
#define RF_TWI_CLK_DIGITAL()    PB02_DIGTAL_ENABLE()

/** @brief TWI DATA pin -- PB03 (MCU pin 20), net RF_DATA
 *  Source: connectivity.json#buses[GPIO_IN_INTERRUPT]; CW32L010_DataSheet_CN_V1.0.pdf#page=24 */
#define RF_TWI_DATA_PORT        CW_GPIOB
#define RF_TWI_DATA_PIN         GPIO_PIN_3
#define RF_TWI_DATA_SET_HIGH()  PB03_SETHIGH()
#define RF_TWI_DATA_SET_LOW()   PB03_SETLOW()
#define RF_TWI_DATA_READ()      PB03_GETVALUE()
#define RF_TWI_DATA_DIR_OUT()   PB03_DIR_OUTPUT()
#define RF_TWI_DATA_DIR_IN()    PB03_DIR_INPUT()
#define RF_TWI_DATA_AF_GPIO()   PB03_AFx_GPIO()
#define RF_TWI_DATA_DIGITAL()   PB03_DIGTAL_ENABLE()

/* ====================================================================
 * Internal State
 * ==================================================================== */

/**
 * @brief TWI driver state
 */
typedef enum {
    RF_TWI_STATE_UNINIT = 0,    /**< Driver not initialized */
    RF_TWI_STATE_IDLE,          /**< Initialized, pins configured */
    RF_TWI_STATE_ACTIVE,        /**< In programming mode (after TWI_ON) */
    RF_TWI_STATE_TX,            /**< Transmission in progress */
    RF_TWI_STATE_ERROR          /**< Error state */
} rf_twi_state_t;

/**
 * @brief TWI driver instance structure
 */
typedef struct {
    rf_twi_state_t  state;      /**< Current driver state */
    uint32_t        timeout_us; /**< Operation timeout in microseconds */
    bool            initialized;/**< Flag: init completed */
} rf_twi_driver_t;

/* ====================================================================
 * Internal Function Declarations
 * ==================================================================== */

/**
 * @brief Generate one CLK cycle (high then low) with DATA driven
 *
 * Timing: Each half-cycle = RF_TWI_HALF_PERIOD_NOP NOPs @ 48MHz ~ 500ns
 * Total CLK period ~ 1us -> 1MHz
 *
 * Source: UM2005C_数据手册_V1.2_V1.2.pdf#page=7 (tCH >= 500ns, tCL >= 500ns)
 *
 * @param data_level  Value to output on DATA pin during this cycle
 */
static inline void rf_twi_cycle(uint8_t data_level)
{
    /* Set DATA before CLK rising to ensure setup time */
    if (data_level) {
        RF_TWI_DATA_SET_HIGH();
    } else {
        RF_TWI_DATA_SET_LOW();
    }

    /* CLK high phase (~500ns) */
    RF_TWI_CLK_SET_HIGH();
    for (volatile uint32_t i = 0; i < RF_TWI_HALF_PERIOD_NOP; i++) { __NOP(); }

    /* CLK low phase (~500ns) -- DATA is sampled on falling edge */
    RF_TWI_CLK_SET_LOW();
    for (volatile uint32_t i = 0; i < RF_TWI_HALF_PERIOD_NOP; i++) { __NOP(); }
}

/**
 * @brief Generate CLK cycle for read (DATA released, MCU samples)
 *
 * CLK goes high, MCU reads DATA pin on falling edge.
 *
 * @return uint8_t  0 or 1, the value read from DATA pin
 */
static inline uint8_t rf_twi_sample_cycle(void)
{
    uint8_t value;

    /* CLK high phase */
    RF_TWI_CLK_SET_HIGH();
    for (volatile uint32_t i = 0; i < RF_TWI_HALF_PERIOD_NOP; i++) { __NOP(); }

    /* Read DATA on falling edge */
    RF_TWI_CLK_SET_LOW();
    value = RF_TWI_DATA_READ() ? 1 : 0;
    for (volatile uint32_t i = 0; i < RF_TWI_HALF_PERIOD_NOP; i++) { __NOP(); }

    return value;
}

/* ====================================================================
 * GPIO Initialization Helper Macros
 * ==================================================================== */

/**
 * These use the CW32L010 SDK GPIO register macros from cw32l010_gpio.h:
 *
 * - PB02_AFx_GPIO() / PB03_AFx_GPIO() -- set to GPIO function (AF0)
 * - PB02_DIGTAL_ENABLE() / PB03_DIGTAL_ENABLE() -- digital mode (not analog)
 * - PB02_DIR_OUTPUT() / PB03_DIR_OUTPUT() -- output direction
 * - PB02_PUSHPULL_ENABLE() / PB03_PUSHPULL_ENABLE() -- push-pull
 * - PB02_PUR_DISABLE() / PB03_PUR_DISABLE() -- no internal pull-up
 * - PB02_SETHIGH()/SETLOW(), PB03_SETHIGH()/SETLOW() -- output levels
 *
 * Source: cw32l010_gpio.h#L158-L161 (DIR macros)
 * Source: cw32l010_gpio.h#L195-L198 (PUSHPULL/OPENDRAIN macros)
 * Source: cw32l010_gpio.h#L122-L125 (ANALOG/DIGITAL macros)
 * Source: cw32l010_gpio.h#L232-L235 (PUR pull-up macros)
 * Source: cw32l010_gpio.h#L290-L293 (SETHIGH/SETLOW macros)
 * Source: cw32l010_gpio.h#L425-L442 (AFx_GPIO macros for PB02,PB03)
 */

#endif /* RF_TWI_DRIVER_H */
