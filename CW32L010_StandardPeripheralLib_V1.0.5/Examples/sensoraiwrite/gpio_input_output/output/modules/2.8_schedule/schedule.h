/**
 * @file    schedule.h
 * @brief   Phase 2.8 — Schedule Module — Internal Module Header
 *
 * MCU:     CW32L010Y8M6 (TSSOP-20)
 * Module:  schedule (layer=application)
 *
 * Internal state, constants, and private declarations for the schedule
 * module. External consumers should include "interface.h" only.
 *
 * Sources:
 *   - output/modules/2.5_power_manager/interface.h   (power_mgr_*)
 *   - output/modules/2.7_sensor_mgr/interface.h      (sensor_mgr_*)
 *   - output/modules/2.4_rf_twi_driver/interface.h   (rf_twi_*)
 *   - output/modules/2.3_uart_console/interface.h    (CONSOLE_PRINT)
 *   - output/design/common_types.h                   (sensor_data_t, rf_packet_t)
 *   - cw32l010_sysctrl.h                             (SYSCTRL_HSI_Enable, etc.)
 *   - cw32l010_systick.h                             (InitTick, SysTickDelay)
 *   - system_cw32l010.h                              (SystemInit, SystemCoreClock)
 */

#ifndef SCHEDULE_H
#define SCHEDULE_H

#include "interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* SDK / Platform Includes                                                    */
/* ========================================================================== */

#include "cw32l010.h"
#include "cw32l010_sysctrl.h"
#include "cw32l010_systick.h"
#include "system_cw32l010.h"

/* ========================================================================== */
/* External Module Dependencies                                               */
/* ========================================================================== */

#include "./../output/modules/2.5_power_manager/interface.h"
#include "./../output/modules/2.7_sensor_mgr/interface.h"
#include "./../output/modules/2.4_rf_twi_driver/interface.h"
#include "./../output/modules/2.3_uart_console/interface.h"

/* ========================================================================== */
/* Internal Constants                                                         */
/* ========================================================================== */

/** @brief Delay before first sensor reading after boot (ms) — AHT10 power-up time */
#define SCHEDULE_BOOT_DELAY_MS          50U

/** @brief Maximum temperature value for sanity check (0.1°C, 85°C) */
#define SCHEDULE_TEMP_SANE_MAX          850

/** @brief Minimum temperature value for sanity check (0.1°C, -40°C) */
#define SCHEDULE_TEMP_SANE_MIN          (-400)

/** @brief Maximum humidity value for sanity check (0.1%, 100%) */
#define SCHEDULE_HUM_SANE_MAX           1000U

/* ========================================================================== */
/* Internal State Structure                                                   */
/* ========================================================================== */

/**
 * @brief Schedule module internal state
 */
typedef struct {
    bool        initialized;            /**< schedule_init() completed */
    bool        cold_boot;              /**< True for POR, false for RTC wake */
    uint32_t    cycle_count;            /**< Completed measurement cycles */
    uint32_t    failure_count;          /**< Consecutive sensor failures */
    sensor_data_t last_data;            /**< Last measured sensor data */
    rf_packet_t   last_packet;          /**< Last transmitted RF packet */
    bool        data_valid;             /**< Whether last_data is valid */
    bool        packet_valid;           /**< Whether last_packet is valid */
} schedule_state_t;

/* ========================================================================== */
/* Internal Function Declarations                                             */
/* ========================================================================== */

/**
 * @brief  Configure system clock for 48 MHz from HSIOSC
 *
 * Sequence:
 *   1. Enable HSIOSC (internal 48 MHz RC oscillator)
 *   2. Wait for HSIOSC stable
 *   3. Switch system clock source to HSIOSC
 *   4. Configure HCLK and PCLK dividers
 *   5. Update SystemCoreClock variable
 *
 * Source: cw32l010_sysctrl.h#L458 (SYSCTRL_HSI_Enable)
 * Source: cw32l010_sysctrl.h#L449 (SYSCTRL_SYSCLKSRC_Config)
 * Source: cw32l010_sysctrl.h#L447 (SYSCTRL_HCLKPRS_Config)
 * Source: cw32l010_sysctrl.h#L448 (SYSCTRL_PCLKPRS_Config)
 *
 * @return ERR_OK on success, ERR_BUS if clock switch fails
 */
static int32_t sch_clock_init(void);

/**
 * @brief  Delay microseconds using calibrated NOP loop
 *
 * Calibrated for 48 MHz HCLK.
 * Each NOP ~ 20.8 ns, so 48 NOPs ≈ 1 μs.
 *
 * @param  us  Microseconds to delay (0..65535)
 */
static void sch_delay_us(uint32_t us);

/**
 * @brief  Delay milliseconds using SysTickDelay()
 *
 * Wrapper around SysTickDelay() with early exit if SysTick not running.
 *
 * @param  ms  Milliseconds to delay
 */
static void sch_delay_ms(uint32_t ms);

/* ========================================================================== */
/* Module Instance (defined in schedule.c)                                    */
/* ========================================================================== */

extern schedule_state_t g_schedule;

#ifdef __cplusplus
}
#endif

#endif /* SCHEDULE_H */
