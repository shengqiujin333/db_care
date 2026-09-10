/**
 * @file    interface.h
 * @brief   Phase 2.5 — Power Manager — Exported API for Upper Layers
 *
 * MCU:     CW32L010Y8M6 (TSSOP-20)
 * Module:  power_manager (layer=service)
 * Bus:     POWER (CR2032 coin cell management)
 *
 * This module provides:
 *   - Power initialization (clock sources: LSI for RTC, HSIOSC for system)
 *   - DeepSleep entry with RTC Auto-Wake Timer (AWT) for ~150s periodic wakeup
 *   - IWDT (Independent Watchdog) safety supervision during DeepSleep
 *   - Battery voltage monitoring via LVD
 *   - Wakeup reason query (POR, IWDT, RTC, external)
 *
 * == Wake Cycle ==
 *   RTC AWT alarm -> RTC_IRQn -> wake from DeepSleep
 *   -> user's sensor/RF task -> power_mgr_sleep_150s() -> DeepSleep
 *
 * Sources:
 *   - cw32l010_pwr.h#L104-L106    (PWR_Config, PWR_GotoLpmMode)
 *   - cw32l010_rtc.h#L459-L477    (RTC_Init, RTC_AWTConfig, RTC_ITConfig)
 *   - cw32l010_iwdt.h#L112-L128   (IWDT_Init, IWDT_Refresh, IWDT_SetPeriod)
 *   - cw32l010_sysctrl.h#L460-L491 (SYSCTRL_LSI_Enable, SYSCTRL_GotoDeepSleep)
 *   - cw32l010_lvd.h#L102-L109    (LVD thresholds for battery monitoring)
 *   - cw32l010.h#L68              (RTC_IRQn = 2)
 *   - connectivity.json#buses[0]   (POWER bus with nets: BAT, GND, N12331)
 *   - COMMON.md#1.2 (150s execution flow)
 */

#ifndef POWER_MANAGER_INTERFACE_H
#define POWER_MANAGER_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>
#include "common_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* Constants                                                                  */
/* ========================================================================== */

/** @brief Default wake interval in seconds (AWT auto-reload value) */
#define POWER_MGR_DEFAULT_WAKE_INTERVAL_S    150U

/** @brief IWDT timeout in seconds (safety watchdog) */
#define POWER_MGR_IWDT_TIMEOUT_S             2U

/** @brief LVD threshold for low battery indication (2.6V for CR2032) */
#define POWER_MGR_LVD_THRESHOLD_MV           2600U

/* ========================================================================== */
/* Wakeup Reason Enum                                                         */
/* ========================================================================== */

/** @brief Reason for MCU wakeup from DeepSleep */
typedef enum {
    POWER_WAKE_REASON_POR       = 0,    /**< Power-on reset (cold boot) */
    POWER_WAKE_REASON_RTC       = 1,    /**< RTC AWT alarm (normal wake) */
    POWER_WAKE_REASON_IWDT      = 2,    /**< IWDT timeout reset */
    POWER_WAKE_REASON_EXT       = 3,    /**< External pin / NRST */
    POWER_WAKE_REASON_LVD       = 4,    /**< LVD reset */
    POWER_WAKE_REASON_UNKNOWN   = 5     /**< Other / unknown */
} power_wake_reason_t;

/* ========================================================================== */
/* Power Status Structure                                                     */
/* ========================================================================== */

/** @brief Current power status snapshot */
typedef struct {
    power_wake_reason_t wake_reason;    /**< Why we woke up */
    uint32_t            uptime_ms;      /**< Uptime since last cold boot (ms) */
    uint16_t            vbat_mv;        /**< Estimated battery voltage (mV), 0 if unknown */
    bool                battery_low;    /**< LVD triggered flag */
    uint32_t            deepsleep_count;/**< Number of DeepSleep entries since boot */
} power_status_t;

/* ========================================================================== */
/* Initialization & Configuration                                             */
/* ========================================================================== */

/**
 * @brief  Initialize the power management subsystem
 *
 * Performs:
 *   1. Read and store wakeup reason from reset flags
 *   2. Enable LSI clock for RTC (SYSCTRL_LSI_Enable)
 *   3. Initialize RTC with LSI source (RTC_RTCCLK_FROM_LSI)
 *   4. Configure IWDT (Independent Watchdog) for safety supervision
 *   5. Optionally configure LVD for battery monitoring (~2.6V threshold)
 *
 * @param  enable_lvd  If true, enable LVD with default threshold (2.6V)
 * @return ERR_OK on success, or negative error code
 *
 * @note   Must be called once at boot, before any power_mgr_sleep_*() call.
 * @note   Returns ERR_BUS if RTC or IWDT init fails.
 */
int32_t power_mgr_init(bool enable_lvd);

/**
 * @brief  De-initialize the power management subsystem
 *
 * Disables LVD, RTC interrupt, stops IWDT.
 * After this call, DeepSleep is not possible.
 */
void power_mgr_deinit(void);

/* ========================================================================== */
/* Sleep Functions                                                            */
/* ========================================================================== */

/**
 * @brief  Enter DeepSleep for the default interval (~150s)
 *
 * Sequence:
 *   1. Configure RTC AWT with reload = POWER_MGR_DEFAULT_WAKE_INTERVAL_S
 *   2. Enable AWT interrupt (RTC_IT_AWTIMER)
 *   3. Set SLEEPDEEP via PWR_Config
 *   4. Refresh IWDT before sleep
 *   5. Execute SYSCTRL_GotoDeepSleep() -> WFI -> enter DeepSleep
 *
 * On wake (RTC_IRQn handler):
 *   - Clear AWTIMER interrupt flag via CW_RTC->ICR_f.AWTIMER = 0
 *   - Disable AWT timer
 *   - Return to caller
 *
 * @note   This function blocks until the RTC AWT fires.
 *         After return, the caller should execute its task and call
 *         this function again for the next cycle.
 */
void power_mgr_sleep_150s(void);

/**
 * @brief  Enter DeepSleep for a custom duration
 *
 * @param  seconds  Duration in seconds (1..65535, limited by AWTARR 16-bit)
 *
 * @note   Same sequence as power_mgr_sleep_150s() but with custom reload.
 */
void power_mgr_sleep_seconds(uint16_t seconds);

/**
 * @brief  Enter Sleep mode (not DeepSleep) — for short waits
 *
 * CPU clock stops via SYSCTRL_GotoSleep(), but peripherals remain clocked.
 * Wake by any interrupt (including SysTick).
 * Lower latency wake than DeepSleep.
 */
void power_mgr_sleep_light(void);

/* ========================================================================== */
/* IWDT (Watchdog) Support                                                    */
/* ========================================================================== */

/**
 * @brief  Refresh / "pet" the IWDT to prevent reset
 *
 * Must be called within the IWDT timeout period (~2s).
 * Safe to call from any context.
 *
 * @note   The power_manager automatically refreshes IWDT before DeepSleep.
 *         Application code must refresh during active periods if IWDT
 *         timeout is less than the active task duration.
 */
void power_mgr_iwdt_refresh(void);

/**
 * @brief  Check if the last reset was caused by IWDT timeout
 * @return true if IWDT timeout occurred, false otherwise
 */
bool power_mgr_iwdt_caused_reset(void);

/* ========================================================================== */
/* Battery / LVD Monitoring                                                   */
/* ========================================================================== */

/**
 * @brief  Get the current power status snapshot
 *
 * @param  status  Pointer to power_status_t to fill
 * @return ERR_OK on success, ERR_INVALID_PARAM if NULL
 *
 * Fills:
 *   - wake_reason from stored reset flags
 *   - uptime_ms from SysTick counter (0 if not available)
 *   - vbat_mv (via LVD rough estimation only; returns 0 without ADC)
 *   - battery_low from LVD flag
 *   - deepsleep_count
 */
int32_t power_mgr_get_status(power_status_t *status);

/**
 * @brief  Check if battery voltage is low (below configured LVD threshold)
 * @return true if battery is low, false otherwise
 *
 * @note   Returns false if LVD was not enabled during init.
 */
bool power_mgr_is_battery_low(void);

/**
 * @brief  Get the wakeup reason for the current boot
 * @return power_wake_reason_t enum value
 */
power_wake_reason_t power_mgr_get_wake_reason(void);

/* ========================================================================== */
/* RTC Interrupt Handler (called from vector table RTC_IRQHandler)            */
/* ========================================================================== */

/**
 * @brief  RTC interrupt service routine
 *
 * Checks and clears AWTIMER interrupt flag via CW_RTC->ICR_f.AWTIMER.
 *
 * @note   This function is called from the ISR defined in power_manager.c.
 *         Users should not call this directly.
 */
void power_mgr_rtc_irq_handler(void);

/* ========================================================================== */
/* LVD Interrupt Handler (if LVD was enabled)                                 */
/* ========================================================================== */

/**
 * @brief  LVD interrupt service routine
 *
 * Sets the battery_low flag and clears LVD interrupt flag.
 */
void power_mgr_lvd_irq_handler(void);

#ifdef __cplusplus
}
#endif

#endif /* POWER_MANAGER_INTERFACE_H */
