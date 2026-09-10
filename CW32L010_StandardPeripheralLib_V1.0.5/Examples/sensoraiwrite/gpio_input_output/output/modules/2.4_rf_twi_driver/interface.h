/**
 * @file    interface.h
 * @brief   Phase 2.4 — rf_twi_driver: Exported API for UM2005C TWI bit-bang driver
 *
 * MCU:  CW32L010Y8M6 (TSSOP-20)
 * Bus:  GPIO bit-banged TWI on PB02(CLK)/PB03(DATA) for UM2005C RF transmitter
 * Instance: GPIO_IN_INTERRUPT (connectivity.json bus kind for nets RF_CLK/RF_DATA)
 *
 * Sources:
 *   - UM2005C_数据手册_V1.2_V1.2.pdf#page=12 (TWI protocol, 16-cycle transactions)
 *   - UM2005C_数据手册_V1.2_V1.2.pdf#page=7  (timing: tCH/tCL >= 500ns, FSCL <= 1MHz)
 *   - UM2005C_数据手册_V1.2_V1.2.pdf#page=14 (state diagram: TWI_ON -> config -> TWI_OFF)
 *   - connectivity.json#buses[GPIO_IN_INTERRUPT] (RF_CLK=PB02, RF_DATA=PB03)
 *   - CW32L010_DataSheet_CN_V1.0.pdf#page=24  (PB02/PB03 as GPIO)
 *   - cw32l010_gpio.h (GPIO_Init, PB02_DIR_OUTPUT/INPUT, PB03_SETHIGH/SETLOW macros)
 *
 * Consumers: rf_mgr (application layer), packet_builder (service layer)
 */

#ifndef RF_TWI_DRIVER_INTERFACE_H
#define RF_TWI_DRIVER_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>
#include "./../output/design/common_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ====================================================================
 * Configuration Constants
 * ==================================================================== */

/** @brief TWI CLK half-period in NOP counts for ~500ns @ 48MHz HCLK */
#define RF_TWI_HALF_PERIOD_NOP     24U

/** @brief Number of CLK cycles for TWI_ON command */
#define RF_TWI_ON_CYCLES           32U

/** @brief Default TWI address for UM2005C register access */
#define RF_TWI_ADDR_DEFAULT        0x00U

/** @brief Special address for TWI commands (0x3F in A[5:0]) */
#define RF_TWI_ADDR_SPECIAL        0x3FU

/** @brief Timeout for TWI operations in busy-wait loop iterations */
#define RF_TWI_TIMEOUT_LOOPS       10000U

/** @brief Total RF packet size in bytes (from rf_packet_t in common_types.h) */
#define RF_TWI_PACKET_SIZE         14U

/**
 * @brief Delay after TWI_ON for programming mode entry (us)
 * Source: UM2005C_数据手册_V1.2_V1.2.pdf#page=6 (tSLP-TX ~1.2ms total wake)
 */
#define RF_TWI_WAKE_DELAY_US       1200U

/**
 * @brief Minimum CLK low time after TWI_OFF to ensure sleep entry
 * Source: UM2005C_数据手册_V1.2_V1.2.pdf#page=14 (state diagram)
 */
#define RF_TWI_SLEEP_HOLD_MS       15U

/* ====================================================================
 * Initialization and Control
 * ==================================================================== */

/**
 * @brief Initialize TWI GPIO pins (PB02=CLK, PB03=DATA)
 *
 * Configures PB02 as push-pull output, PB03 as push-pull output initially
 * (for write mode). Both pins are set to digital mode, no pull-up, no filter.
 * CLK starts low, DATA starts low.
 *
 * Source: cw32l010_gpio.h#L425-L442 (PB02_AFx_GPIO, PB03_AFx_GPIO macros)
 * Source: connectivity.json#buses[GPIO_IN_INTERRUPT] (RF_CLK, RF_DATA nets)
 *
 * @return ERR_OK on success, or negative error code
 */
int32_t rf_twi_init(void);

/**
 * @brief De-initialize TWI GPIO pins and enter low-power state
 *
 * Sets both pins to input mode (high-Z) to minimize leakage during DeepSleep.
 *
 * @return ERR_OK on success
 */
int32_t rf_twi_deinit(void);

/* ====================================================================
 * TWI Special Commands
 * ==================================================================== */

/**
 * @brief Send TWI_ON -- 32 CLK cycles with DATA=0
 *
 * Enters programming mode and resets TWI circuit.
 * Must be called before any TWI read/write transactions.
 *
 * Source: UM2005C_数据手册_V1.2_V1.2.pdf#page=12
 *
 * @return ERR_OK on success
 */
int32_t rf_twi_on(void);

/**
 * @brief Send TWI_OFF -- command 0xFF02
 *
 * Exits programming mode. After TWI_OFF, the UM2005C enters TX mode
 * if data has been written. Hold CLK low >=15ms after this to enter sleep.
 *
 * Source: UM2005C_数据手册_V1.2_V1.2.pdf#page=12
 *
 * @return ERR_OK on success
 */
int32_t rf_twi_off(void);

/**
 * @brief Send SOFT_RST -- command 0xFF04
 *
 * Resets all UM2005C digital circuits except TWI interface.
 *
 * Source: UM2005C_数据手册_V1.2_V1.2.pdf#page=12
 *
 * @return ERR_OK on success
 */
int32_t rf_twi_soft_reset(void);

/**
 * @brief Send TWI_RST -- command 0xFF01
 *
 * Resets only the TWI interface module on UM2005C.
 * Use if TWI enters an unknown/bad state.
 *
 * Source: UM2005C_数据手册_V1.2_V1.2.pdf#page=12
 *
 * @return ERR_OK on success
 */
int32_t rf_twi_reset(void);

/* ====================================================================
 * TWI Data Transactions
 * ==================================================================== */

/**
 * @brief Write one byte to UM2005C via TWI
 *
 * Performs a 16-cycle TWI write transaction:
 *   Cycles 1-8:  W/R=1, A[5:0]=address, bit8=0
 *   Cycles 9-16: D[7:0] = data byte
 *
 * Source: UM2005C_数据手册_V1.2_V1.2.pdf#page=12
 *
 * @param address   6-bit register address (0-63), placed in A[5:0]
 * @param data      8-bit data byte to write
 * @return ERR_OK on success, ERR_BUS on bus error (DATA stuck)
 */
int32_t rf_twi_write(uint8_t address, uint8_t data);

/**
 * @brief Read one byte from UM2005C via TWI
 *
 * Performs a 16-cycle TWI read transaction:
 *   Cycles 1-8:  W/R=0, A[5:0]=address, bit8=0
 *   Cycles 9-16: DATA driven by UM2005C, MCU samples on falling edge
 *
 * Source: UM2005C_数据手册_V1.2_V1.2.pdf#page=12
 *
 * @param address   6-bit register address (0-63), placed in A[5:0]
 * @param out_byte  Pointer to store the read byte
 * @return ERR_OK on success, ERR_INVALID_PARAM if out_byte is NULL
 */
int32_t rf_twi_read(uint8_t address, uint8_t *out_byte);

/**
 * @brief Write multiple bytes sequentially to UM2005C
 *
 * Calls rf_twi_write() for each byte. All bytes are written
 * to the same register address (FIFO/streaming).
 *
 * @param address   Base register address
 * @param data      Pointer to data buffer
 * @param length    Number of bytes to write
 * @return ERR_OK on success, or first error encountered
 */
int32_t rf_twi_write_buf(uint8_t address, const uint8_t *data, uint16_t length);

/* ====================================================================
 * High-Level RF Transmission
 * ==================================================================== */

/**
 * @brief Perform complete RF transmission sequence
 *
 * Full state machine:
 *   IDLE -> WAKE_RF -> TWI_ON -> DATA_WRITE -> TWI_OFF -> SLEEP -> IDLE
 *
 * Sources:
 *   - UM2005C_数据手册_V1.2_V1.2.pdf#page=14 (state diagram)
 *   - UM2005C_数据手册_V1.2_V1.2.pdf#page=6 (timing)
 *
 * @param packet   Pointer to 14-byte RF packet (rf_packet_t)
 * @return ERR_OK on success, error code on failure
 */
int32_t rf_twi_transmit(const uint8_t *packet);

/**
 * @brief Read LBD (low battery detection) value
 *
 * Reads the LBD register (0x3E) after TWI_ON.
 * Value maps to voltage: Vbat = 2.0 + LBD_value * 0.1 V
 *
 * Source: UM2005C_数据手册_V1.2_V1.2.pdf#page=13
 *
 * @param out_lbd  Pointer to store LBD value
 * @return ERR_OK on success
 */
int32_t rf_twi_read_lbd(uint8_t *out_lbd);

/* ====================================================================
 * Timing Utility (internal, exposed for test access)
 * ==================================================================== */

/** @brief Microsecond-level delay using calibrated NOP loop @ 48MHz */
void rf_twi_delay_us(uint32_t us);

/** @brief Millisecond-level delay using FirmwareDelay() */
void rf_twi_delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* RF_TWI_DRIVER_INTERFACE_H */
