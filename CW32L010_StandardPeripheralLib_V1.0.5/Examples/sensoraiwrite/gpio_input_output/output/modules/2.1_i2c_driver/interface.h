/**
 * @file    interface.h
 * @brief   I2C Driver — Exported API for Upper Layers
 * @author  Auto-generated (EmbedDevOps)
 * @date    2025-07-17
 *
 * MCU:     CW32L010Y8M6 (TSSOP-20)
 * Bus:     Bit-bang I2C master on PA03 (SDA) / PA04 (SCL)
 * Target:  AHT10 temperature/humidity sensor @ 0x38 (7-bit)
 *
 * == ARCHITECTURE NOTE ==
 * The AHT10 I2C bus (N05845=SCL, N06024=SDA) is wired to MCU pins
 * PA04 (pin 14) and PA03 (pin 13) respectively. Per the CW32L010 GPIO
 * alternate function register definitions [cw32l010_gpio.h#L352-L368],
 * PA03 alternate functions are: GPIO(0), UART2_TXD(1), LPTIM_CH1(2),
 * SPI1_MISO(3), BTIM1_ETR(4), IR_OUT(5), GTIM_CH4(6), ATIM_CH3(7).
 * PA04 alternate functions are: GPIO(0), UART2_RXD(1), LPTIM_CH2(2),
 * SPI1_MOSI(3), MCO_OUT(4), VC2_OUT(5), GTIM1_CH3(6), ATIM_CH1N(7).
 * Neither pin supports a hardware I2C alternate function.
 * Therefore this driver implements standard I2C master protocol via
 * GPIO bit-banging with open-drain outputs.
 *
 * Sources:
 *   - Pin assignment:     meta/connectivity.json#components[0].pins[13,14]
 *   - AF verification:    inputs/mcu_sdk/Libraries/inc/cw32l010_gpio.h#L352-L368
 *   - GPIO API:           inputs/mcu_sdk/Libraries/inc/cw32l010_gpio.h
 *   - Delay/Systick:      inputs/mcu_sdk/Libraries/inc/cw32l010_systick.h
 *   - AHT10 protocol:     industry-standard (0xAC trigger, 6-byte read)
 */

#ifndef INTERFACE_H
#define INTERFACE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* Error Codes                                                                */
/* ========================================================================== */
typedef enum {
    I2C_DRV_OK          = 0,      /**< Operation succeeded */
    I2C_DRV_ERR_NACK    = -1,     /**< Address or data NACK received */
    I2C_DRV_ERR_TIMEOUT = -2,     /**< Bus timeout / SCL held low */
    I2C_DRV_ERR_ARBLOST = -3,     /**< Arbitration lost */
    I2C_DRV_ERR_BUSY    = -4,     /**< Bus busy (another transaction in progress) */
    I2C_DRV_ERR_PARAM   = -5      /**< Invalid parameter */
} i2c_drv_error_t;

/* ========================================================================== */
/* Initialization & Configuration                                             */
/* ========================================================================== */

/**
 * @brief  Initialize bit-bang I2C master on PA03(SDA) / PA04(SCL)
 * @retval I2C_DRV_OK on success
 *
 * Configures both pins as open-drain output (high = Hi-Z via output
 * with OPENDRAIN bit set, pulled up by external 4.7kΩ resistors;
 * low = driven low). SCL and SDA start high (bus idle).
 * I2C bus runs at ~100 kHz (standard mode).
 */
i2c_drv_error_t i2c_drv_init(void);

/**
 * @brief  De-initialize I2C pins (set to GPIO input, Hi-Z)
 */
void i2c_drv_deinit(void);

/* ========================================================================== */
/* Master Transfer Functions (blocking, polling)                              */
/* ========================================================================== */

/**
 * @brief  Write data to a slave device
 * @param  slave_addr  7-bit I2C slave address (e.g. 0x38 for AHT10)
 * @param  data        Pointer to data buffer
 * @param  len         Number of bytes to send
 * @retval I2C_DRV_OK on success, negative on error
 *
 * Generates: START -> SLA+W -> data[0..n-1] -> STOP
 * Returns I2C_DRV_ERR_NACK if slave NACKs address or any data byte.
 */
i2c_drv_error_t i2c_drv_master_write(uint8_t slave_addr,
                                     const uint8_t *data,
                                     size_t len);

/**
 * @brief  Read data from a slave device
 * @param  slave_addr  7-bit I2C slave address
 * @param  data        Pointer to receive buffer
 * @param  len         Number of bytes to read
 * @retval I2C_DRV_OK on success, negative on error
 *
 * Generates: START -> SLA+R -> data[0..n-1] (master NACK last) -> STOP
 */
i2c_drv_error_t i2c_drv_master_read(uint8_t slave_addr,
                                    uint8_t *data,
                                    size_t len);

/**
 * @brief  Combined write-then-restart-read (typical for sensors)
 * @param  slave_addr  7-bit I2C slave address
 * @param  wdata       Write data buffer
 * @param  wlen        Write length
 * @param  rdata       Read data buffer
 * @param  rlen        Read length
 * @retval I2C_DRV_OK on success
 *
 * Generates: START -> SLA+W -> wdata[0..wlen-1] -> REPEATED START ->
 *            SLA+R -> rdata[0..rlen-1] -> STOP
 */
i2c_drv_error_t i2c_drv_master_write_read(uint8_t slave_addr,
                                          const uint8_t *wdata,
                                          size_t wlen,
                                          uint8_t *rdata,
                                          size_t rlen);

/* ========================================================================== */
/* Utility / Bus Management                                                   */
/* ========================================================================== */

/**
 * @brief  Check if any device ACKs its address (ping)
 * @param  slave_addr  7-bit address
 * @retval true if device ACKed, false if NACK or bus error
 */
bool i2c_drv_probe(uint8_t slave_addr);

/**
 * @brief  Generate bus clear sequence (up to 9 SCL pulses + STOP)
 *         to recover a hung slave
 */
void i2c_drv_bus_clear(void);

/**
 * @brief  Return a human-readable string for an error code
 */
const char *i2c_drv_strerror(i2c_drv_error_t err);

#ifdef __cplusplus
}
#endif

#endif /* INTERFACE_H */
