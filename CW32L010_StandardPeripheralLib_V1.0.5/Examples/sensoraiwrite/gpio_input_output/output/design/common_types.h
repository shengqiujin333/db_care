/**
 * @file    common_types.h
 * @brief   Phase 2.0 — Common types, error codes, and log API for CW32L010 project
 *
 * MCU: CW32L010Y8M6 (TSSOP-20)
 * Project: Battery-powered temp/humidity sensor + 433MHz RF TX
 *
 * Sources:
 *   - CW32L010_DataSheet_CN_V1.0.pdf#page=24 (pin AF)
 *   - connectivity.json (bus/net mapping)
 *   - UM2005C_数据手册_V1.2_V1.2.pdf#page=12 (TWI protocol)
 */

#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/* ====================================================================
 * Error Code Convention
 * ====================================================================
 * All driver/service functions return int32_t:
 *   >= 0   Success (or byte count / status value)
 *   < 0    Error code (from this enum, negated)
 * ==================================================================== */

typedef enum {
    ERR_OK          =  0,   /**< Operation completed successfully */
    ERR_BUSY        = -1,   /**< Resource temporarily busy */
    ERR_TIMEOUT     = -2,   /**< Operation timed out */
    ERR_INVALID_PARAM = -3, /**< Invalid parameter passed */
    ERR_NACK        = -4,   /**< I2C NACK received */
    ERR_CRC         = -5,   /**< CRC mismatch */
    ERR_READ_FAIL   = -6,   /**< Read operation failed */
    ERR_WRITE_FAIL  = -7,   /**< Write operation failed */
    ERR_NOT_INIT    = -8,   /**< Module not initialized */
    ERR_BUS         = -9,   /**< Bus error (I2C/TWI) */
    ERR_INVALID_STATE = -10,/**< Invalid state machine state */
    ERR_OVERRUN     = -11,  /**< Buffer overrun */
} error_code_t;

/* ====================================================================
 * Sensor Data Structures
 * ==================================================================== */

/**
 * @brief Scaled sensor measurement values
 *
 * temperature_x10: 0.1°C resolution, e.g. 254 = 25.4°C
 * humidity_x10:    0.1%  resolution, e.g. 623 = 62.3%
 *
 * Conversion formulas (from SDK measure.c example):
 *   temp = (raw32 * 200.0 / 1048576.0 - 50.0) * 10
 *   hum  = (raw32 * 100.0 / 1048576.0) * 10
 */
typedef struct {
    int16_t  temperature_x10;   /**< Temperature in 0.1°C, signed (-500..+850) */
    uint16_t humidity_x10;      /**< Humidity in 0.1%, unsigned (0..1000) */
    uint8_t  status;            /**< Bitfield status flags (see SENSOR_STATUS_*) */
    uint32_t timestamp_ms;      /**< System tick (ms) at measurement time */
} sensor_data_t;

/* Sensor status bit definitions */
#define SENSOR_STATUS_VALID     0x01    /**< Data is valid */
#define SENSOR_STATUS_STALE     0x02    /**< Data is from previous cycle */
#define SENSOR_STATUS_READ_FAIL 0x08    /**< I2C read failed */
#define SENSOR_STATUS_CRC_FAIL  0x10    /**< Data CRC check failed */

/* ====================================================================
 * RF Packet Types
 * ==================================================================== */

/**
 * @brief RF packet structure for UM2005C transmission
 *
 * Byte layout (12 bytes total):
 *   [0]   Preamble byte 1: 0xAA
 *   [1]   Preamble byte 2: 0x55
 *   [2-7] 6-byte MCU UID (unique ID)
 *   [8]   Temperature MSB (temperature_x10 >> 8)
 *   [9]   Temperature LSB (temperature_x10 & 0xFF)
 *   [10]  Humidity MSB    (humidity_x10 >> 8)
 *   [11]  Humidity LSB    (humidity_x10 & 0xFF)
 *   [12]  System status byte
 *   [13]  CRC8 (Dallas 1-Wire / MAXIM iButton polynomial)
 *
 * Packet length: 14 bytes
 *
 * Sources:
 *   - UM2005C_数据手册_V1.2_V1.2.pdf#page=12 (TWI data write)
 *   - connectivity.json conflicts[0].user_note: "UID用Mcu的"
 */
typedef struct {
    uint8_t  preamble[2];       /**< 0xAA, 0x55 sync pattern */
    uint8_t  uid[6];            /**< MCU unique ID (96-bit, lower 48 used) */
    uint8_t  temp_hi;           /**< temperature_x10 high byte */
    uint8_t  temp_lo;           /**< temperature_x10 low byte */
    uint8_t  hum_hi;            /**< humidity_x10 high byte */
    uint8_t  hum_lo;            /**< humidity_x10 low byte */
    uint8_t  sys_status;        /**< Battery low flags, power, etc. */
    uint8_t  crc8;              /**< CRC-8/MAXIM (poly 0x31) over bytes 0-12 */
} __attribute__((packed)) rf_packet_t;

#define RF_PACKET_SIZE      14      /**< Total packet size in bytes */
#define RF_PREAMBLE_0       0xAA    /**< Preamble byte 0 */
#define RF_PREAMBLE_1       0x55    /**< Preamble byte 1 */

/* System status bits */
#define SYS_STATUS_BAT_LOW      0x01    /**< Battery voltage low */
#define SYS_STATUS_SENSOR_OK    0x02    /**< Sensor data valid */
#define SYS_STATUS_RF_TXDONE    0x04    /**< RF transmission completed */

/* ====================================================================
 * Logging API Macros
 * ====================================================================
 * Compiled out when LOG_LEVEL is set below the message level.
 * Set LOG_LEVEL in build configuration or board header.
 * ==================================================================== */

#ifndef LOG_LEVEL
#define LOG_LEVEL   LOG_LEVEL_INFO   /**< Default: info and above */
#endif

#define LOG_LEVEL_NONE     0
#define LOG_LEVEL_ERROR    1
#define LOG_LEVEL_WARN     2
#define LOG_LEVEL_INFO     3
#define LOG_LEVEL_DEBUG    4

/* Forward declaration: platform provides log_printf */
void log_printf(const char *fmt, ...);

#if LOG_LEVEL >= LOG_LEVEL_ERROR
#define LOG_ERROR(fmt, ...)  log_printf("[E] " fmt "\r\n", ##__VA_ARGS__)
#else
#define LOG_ERROR(fmt, ...)  ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARN
#define LOG_WARN(fmt, ...)   log_printf("[W] " fmt "\r\n", ##__VA_ARGS__)
#else
#define LOG_WARN(fmt, ...)   ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
#define LOG_INFO(fmt, ...)   log_printf("[I] " fmt "\r\n", ##__VA_ARGS__)
#else
#define LOG_INFO(fmt, ...)   ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define LOG_DEBUG(fmt, ...)  log_printf("[D] " fmt "\r\n", ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...)  ((void)0)
#endif

/* ====================================================================
 * I2C Driver Callback Types
 * ==================================================================== */

/** @brief I2C transaction completion callback */
typedef void (*i2c_callback_t)(uint8_t dev_addr, int32_t status, void *user_arg);

/* ====================================================================
 * Utility Macros
 * ==================================================================== */

/** Clamp a value between min and max */
#define CLAMP(x, lo, hi)  (((x) < (lo)) ? (lo) : (((x) > (hi)) ? (hi) : (x)))

/** Array size */
#define ARRAY_SIZE(a)     (sizeof(a) / sizeof((a)[0]))

/** Bit mask */
#define BIT(n)            (1UL << (n))

#endif /* COMMON_TYPES_H */
