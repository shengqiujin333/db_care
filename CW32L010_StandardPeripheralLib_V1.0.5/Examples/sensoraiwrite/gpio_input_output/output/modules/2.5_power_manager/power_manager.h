/**
 * @file    power_manager.h
 * @brief   Phase 2.5 — Power Manager — Internal Module Header
 *
 * MCU: CW32L010Y8M6 (TSSOP-20)
 * Module: power_manager (layer=service)
 *
 * Internal types, constants, and static function declarations for
 * the power management subsystem. Not intended for direct inclusion
 * by upper-layer modules; use interface.h instead.
 *
 * Sources:
 *   - cw32l010_pwr.h#L88-L93       (PWR_InitTypeDef)
 *   - cw32l010_rtc.h#L48-L54       (RTC_InitTypeDef)
 *   - cw32l010_rtc.h#L63-L67       (RTC_AWTTypeDef)
 *   - cw32l010_rtc.h#L129          (RTC_RTCCLK_FROM_LSI)
 *   - cw32l010_rtc.h#L199          (RTC_AWTSOURCE_FROM_RTC1HZ_1)
 *   - cw32l010_rtc.h#L141          (RTC_IT_AWTIMER)
 *   - cw32l010_iwdt.h#L31-L44      (IWDT_InitTypeDef)
 *   - cw32l010_iwdt.h#L54          (IWDT_Prescaler_DIV64)
 *   - cw32l010_iwdt.h#L84          (IWDT_OVERFLOW_ACTION_RESET)
 *   - cw32l010_iwdt.h#L89          (IWDT_SLEEP_CONTINUE)
 *   - cw32l010_lvd.h#L181-L188     (LVD_InitTypeDef)
 *   - cw32l010_lvd.h#L104          (LVD_Threshold_2p6V)
 *   - cw32l010_sysctrl.h#L490-L491 (SYSCTRL_GotoDeepSleep, GotoSleep)
 *   - cw32l010_sysctrl.h#L460      (SYSCTRL_LSI_Enable)
 *   - connectivity.json#buses[0]   (POWER bus)
 */

#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include "interface.h"
#include "cw32l010.h"
#include "cw32l010_pwr.h"
#include "cw32l010_rtc.h"
#include "cw32l010_iwdt.h"
#include "cw32l010_lvd.h"
#include "cw32l010_sysctrl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* Internal Constants                                                         */
/* ========================================================================== */

/** @brief IWDT LSI frequency (Hz) — from cw32l010_iwdt.h#L48 */
#define PM_IWDT_LSI_FREQ_HZ         10000UL

/** @brief LSI frequency (Hz) — from cw32l010_sysctrl.h#L7 */
#define PM_LSI_FREQ_HZ              32800UL

/** @brief RTC AWT prescaler: use RTC1HZ (1 Hz) source for second-level timing */
#define PM_AWT_CLK_SRC              RTC_AWTSOURCE_FROM_RTC1HZ_1

/** @brief IRQ priority for RTC (Cortex-M0+ has 2-bit priority, range 0-3) */
#define PM_RTC_IRQ_PRIORITY         3U

/** @brief IRQ priority for LVD (lowest = 3) */
#define PM_LVD_IRQ_PRIORITY         3U

/* ========================================================================== */
/* Internal State                                                             */
/* ========================================================================== */

/** @brief Power manager internal state singleton */
typedef struct {
    bool                initialized;        /**< power_mgr_init() called */
    bool                lvd_enabled;        /**< LVD was configured */
    power_wake_reason_t wake_reason;        /**< Stored wakeup reason from boot */
    uint32_t            deepsleep_count;    /**< DeepSleep entries count */
    bool                battery_low_flag;   /**< LVD triggered flag */
} pm_state_t;

/* ========================================================================== */
/* Internal Function Declarations                                             */
/* ========================================================================== */

/**
 * @brief  Determine the wakeup reason from SYSCTRL reset flags
 * @return power_wake_reason_t
 *
 * Checks:
 *   - SYSCTRL_RESETFLAG_POR   -> POWER_WAKE_REASON_POR
 *   - SYSCTRL_RESETFLAG_IWDT  -> POWER_WAKE_REASON_IWDT
 *   - SYSCTRL_RESETFLAG_LVD   -> POWER_WAKE_REASON_LVD
 *   - SYSCTRL_RESETFLAG_PIN   -> POWER_WAKE_REASON_EXT
 *   - RTC AWT interrupt flag  -> POWER_WAKE_REASON_RTC (via CW_RTC->ISR_f.AWTIMER)
 *
 * Source: cw32l010_sysctrl.h#L331-L338
 */
power_wake_reason_t pm_detect_wake_reason(void);

/**
 * @brief  Configure RTC with LSI clock source
 * @return ERR_OK on success, ERR_BUS on failure
 *
 * Enables LSI via SYSCTRL_LSI_Enable(), configures RTC_InitTypeDef with
 * RTC_RTCCLK_FROM_LSI, sets initial date/time, calls RTC_Init().
 *
 * Source: cw32l010_rtc.h#L459, cw32l010_sysctrl.h#L460
 */
int32_t pm_rtc_init(void);

/**
 * @brief  Configure the RTC Auto-Wake Timer (AWT) for periodic wakeup
 *
 * @param  seconds  Wake interval in seconds (1..65535)
 *
 * Uses RTC_AWTSOURCE_FROM_RTC1HZ_1 as clock source so ARR value = seconds.
 *
 * Source: cw32l010_rtc.h#L476
 */
void pm_awt_config(uint16_t seconds);

/**
 * @brief  Configure and start the IWDT
 * @return ERR_OK on success, ERR_BUS on IWDT init failure
 *
 * Prescaler: IWDT_Prescaler_DIV64 (10000 / 64 = 156.25 Hz tick)
 * Reload: 312 (~2 second timeout)
 * Window: 0 (disabled)
 * Overflow action: IWDT_OVERFLOW_ACTION_RESET (system reset)
 * Sleep mode: IWDT_SLEEP_CONTINUE (continues during DeepSleep)
 *
 * Source: cw32l010_iwdt.h#L112-L128
 */
int32_t pm_iwdt_init(void);

/**
 * @brief  Configure LVD for battery low monitoring (~2.6V threshold)
 *
 * Threshold: LVD_Threshold_2p6V triggers when VDD drops below ~2.6V
 * Action: LVD_Action_Irq (interrupt, not reset)
 * Source: LVD_Source_VDDA (main supply)
 * Filter: LSI clock, 2N6 filter time
 *
 * Sources:
 *   - cw32l010_lvd.h#L104 (LVD_Threshold_2p6V)
 *   - cw32l010_lvd.h#L83  (LVD_Action_Irq)
 *   - cw32l010_lvd.h#L92  (LVD_Source_VDDA)
 *   - cw32l010_lvd.h#L133 (LVD_FilterClk_LSI)
 *   - cw32l010_lvd.h#L146 (LVD_FilterTime_2N6)
 */
void pm_lvd_init(void);

/**
 * @brief  Enter DeepSleep via PWR_Config + SYSCTRL_GotoDeepSleep
 *
 * Configures SLEEPDEEP=1 via PWR_Config, refreshes IWDT,
 * then calls SYSCTRL_GotoDeepSleep() which executes __WFI().
 *
 * In DeepSleep:
 *   - HSIOSC/HSE off
 *   - LSI keeps running (RTC + IWDT active)
 *   - RAM retention
 *   - RTC AWT continues counting
 *
 * Wake sources: RTC_IRQn (AWTIMER), external NRST, IWDT reset
 *
 * Sources:
 *   - cw32l010_pwr.h#L104
 *   - cw32l010_sysctrl.h#L491
 */
void pm_enter_deepsleep(void);

#ifdef __cplusplus
}
#endif

#endif /* POWER_MANAGER_H */
