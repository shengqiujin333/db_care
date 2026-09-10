/**
 * @file    interface.h
 * @brief   Phase 2.8 — Schedule Module — Exported API for System Entry
 *
 * MCU:     CW32L010Y8M6 (TSSOP-20)
 * Module:  schedule (layer=application)
 * Bus:     POWER (CR2032)
 *
 * This module is the top-level application scheduler / system entry point.
 * It contains main() and orchestrates the 150-second measurement cycle:
 *
 *   Power ON / Reset
 *     ↓
 *   schedule_init()
 *     - System clock (HSIOSC 48 MHz)
 *     - SysTick (1 ms tick)
 *     - UART console (if enabled)
 *     - I2C + AHT10
 *     - RF TWI GPIO
 *     - Power manager (RTC, IWDT, LVD)
 *     - Sensor manager (AHT10 init, packet builder init)
 *     ↓
 *   main loop (infinite):
 *     ┌──────────────────────────────────────┐
 *     │  sensor_mgr_run()  ← blocking ~260ms │
 *     │  rf_twi_transmit() ← blocking ~20ms  │
 *     │  power_mgr_sleep_150s() ← ~150s      │
 *     └──────────────────────────────────────┘
 *     ↓ RTC AWT fires → wake → loop
 *
 * Sources:
 *   - power_mgr_init:       output/modules/2.5_power_manager/interface.h#L102
 *   - power_mgr_sleep_150s: output/modules/2.5_power_manager/interface.h#L135
 *   - sensor_mgr_init:      output/modules/2.7_sensor_mgr/interface.h#L94
 *   - sensor_mgr_run:       output/modules/2.7_sensor_mgr/interface.h#L146
 *   - rf_twi_init:          output/modules/2.4_rf_twi_driver/interface.h#L81
 *   - rf_twi_transmit:      output/modules/2.4_rf_twi_driver/interface.h#L207
 *   - uart_console_init:    output/modules/2.3_uart_console/interface.h#L62
 *   - common_types.h:       output/design/common_types.h
 *   - COMMON.md#1.2 (150s execution flow)
 *   - CONN_SLICE (POWER bus)
 *   - cw32l010_sysctrl.h#L458 (SYSCTRL_HSI_Enable)
 *   - cw32l010_systick.h#L58  (InitTick)
 */

#ifndef SCHEDULE_INTERFACE_H
#define SCHEDULE_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>
#include "./../output/design/common_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* Constants                                                                  */
/* ========================================================================== */

/** @brief Default system clock frequency (HSIOSC = 48 MHz) */
#define SCHEDULE_SYSCLK_HZ              48000000UL

/** @brief Default HCLK prescaler: divide by 1 (48 MHz) */
#define SCHEDULE_HCLK_PRS               SYSCTRL_HCLK_DIV1

/** @brief Default PCLK prescaler: divide by 1 (48 MHz) */
#define SCHEDULE_PCLK_PRS               SYSCTRL_PCLK_DIV1

/** @brief DeepSleep wake interval (seconds) — matches power_manager default */
#define SCHEDULE_WAKE_INTERVAL_S        150U

/** @brief Maximum number of consecutive sensor failures before skipping TX */
#define SCHEDULE_MAX_FAILURES_BEFORE_SKIP  5U

/* ========================================================================== */
/* Initialization                                                             */
/* ========================================================================== */

/**
 * @brief  Initialize the entire system at cold boot
 *
 * Performs, in order:
 *   1. SystemInit() — default CMSIS system init
 *   2. Configure HSIOSC as system clock @ 48 MHz via SYSCTRL_HSI_Enable()
 *      + SYSCTRL_SYSCLKSRC_Config() + SYSCTRL_HCLKPRS_Config()
 *   3. SystemCoreClockUpdate()
 *   4. InitTick(SystemCoreClock) — 1 ms SysTick
 *   5. power_mgr_init(true) — RTC + LSI + IWDT + LVD (~2.6V)
 *   6. uart_console_init(SystemCoreClock) — if CONFIG_CONSOLE_ENABLE
 *   7. rf_twi_init() — PB02/PB03 as TWI GPIO
 *   8. sensor_mgr_init() — AHT10 + packet builder
 *
 * @return ERR_OK on success, or negative error code from first failure
 */
int32_t schedule_init(void);

/* ========================================================================== */
/* Main Loop Control                                                          */
/* ========================================================================== */

/**
 * @brief  Execute one complete measurement + RF TX + sleep cycle
 *
 * Flow:
 *   1. Call sensor_mgr_run() — measure AHT10 twice, average, build RF packet
 *   2. Call rf_twi_transmit() — send packet via UM2005C TWI
 *   3. Call power_mgr_sleep_150s() — enter DeepSleep for ~150s
 *
 * @return ERR_OK on successful completion of full cycle
 * @return negative error code if sensor or RF failed
 *
 * @note  This function blocks for the full cycle duration (~150s).
 *        On error, the system still enters DeepSleep (error data may be sent).
 */
int32_t schedule_run_cycle(void);

/* ========================================================================== */
/* System Control (for test / debug)                                          */
/* ========================================================================== */

/**
 * @brief  Get the number of completed measurement cycles since boot
 * @return uint32_t cycle count
 */
uint32_t schedule_get_cycle_count(void);

/**
 * @brief  Get the number of consecutive sensor failures
 * @return uint32_t failure count
 */
uint32_t schedule_get_failure_count(void);

/**
 * @brief  Check if the system was cold-booted (POR) vs RTC wake
 * @return true if POR, false if normal RTC wake
 */
bool schedule_is_cold_boot(void);

/**
 * @brief  Reset all internal counters (for testing)
 */
void schedule_reset_counters(void);

#ifdef __cplusplus
}
#endif

#endif /* SCHEDULE_INTERFACE_H */
