/**
 * @file    power_manager.c
 * @brief   Phase 2.5 — Power Manager Implementation
 *
 * MCU: CW32L010Y8M6 (TSSOP-20)
 * Module: power_manager (layer=service)
 * Bus:    POWER (CR2032 coin cell)
 *
 * == Architecture ==
 *
 * This module implements the power management for a CR2032 battery-powered
 * sensor node. The system spends ~99.9% of its time in DeepSleep, waking
 * every ~150 seconds for a brief sensor read + RF transmit cycle.
 *
 * == Wake Cycle (normal operation) ==
 *   1. RTC AWT (Auto Wake Timer) counts down from 150 (1 Hz tick)
 *   2. AWT underflow -> RTC_IT_AWTIMER interrupt -> wakes CPU from DeepSleep
 *   3. RTC_IRQHandler calls power_mgr_rtc_irq_handler()
 *   4. Handler clears AWTIMER flag via CW_RTC->ICR_f.AWTIMER = 0
 *   5. Execution returns to power_mgr_sleep_150s() after WFI
 *   6. The caller (scheduler) performs sensor read + RF TX
 *   7. Caller calls power_mgr_sleep_150s() again -> enter DeepSleep
 *
 * == IWDT Safety ==
 *   IWDT runs continuously, even during DeepSleep (IWDT_SLEEP_CONTINUE).
 *   Timeout: ~2 seconds. Refreshed just before entering DeepSleep.
 *   If system hangs or fails to wake, IWDT resets the MCU.
 *
 * == Battery Monitoring ==
 *   LVD (Low Voltage Detector) is optional. When enabled with threshold
 *   2.6V (LVD_Threshold_2p6V), it triggers an interrupt when VDD drops
 *   below ~2.6V, setting the battery_low flag.
 *
 * Sources:
 *   - cw32l010_pwr.h#L104-L106       (PWR_Config, PWR_GotoLpmMode)
 *   - cw32l010_pwr.c#L78-L91         (PWR_Config implementation: SCB_SCR)
 *   - cw32l010_rtc.h#L310-L313       (RTC_UNLOCK/RTC_LOCK macros)
 *   - cw32l010_rtc.h#L459-L477       (RTC API functions)
 *   - cw32l010_rtc.h#L129            (RTC_RTCCLK_FROM_LSI)
 *   - cw32l010_rtc.h#L199            (RTC_AWTSOURCE_FROM_RTC1HZ_1)
 *   - cw32l010_rtc.h#L141            (RTC_IT_AWTIMER)
 *   - cw32l010_iwdt.h#L50-L81        (IWDT prescalers, keys, init)
 *   - cw32l010_iwdt.c#L41-L73        (IWDT_Init implementation)
 *   - cw32l010_lvd.h#L102-L109       (LVD_Threshold_2p6V etc.)
 *   - cw32l010_sysctrl.h#L460        (SYSCTRL_LSI_Enable)
 *   - cw32l010_sysctrl.h#L490-L491   (SYSCTRL_GotoDeepSleep, SYSCTRL_GotoSleep)
 *   - cw32l010_sysctrl.h#L331-L338   (Reset flag macros)
 *   - cw32l010.h#L68                 (RTC_IRQn = 2)
 *   - cw32l010.h#L67                 (LVD_IRQn = 1)
 *   - connectivity.json#buses[0]     (POWER bus: BAT, GND, N12331)
 *   - connectivity.json#components[U7].pins[9]  (MCU pin 9 = VDD = BAT)
 *   - COMMON.md#1.2 (150s execution flow)
 */

#include "power_manager.h"
#include "cw32l010_sysctrl.h"
#include <stddef.h>     /* NULL */

/* ========================================================================== */
/* Module State                                                               */
/* ========================================================================== */

/** @brief Internal state singleton — initialized at compile time */
static pm_state_t s_state = {
    .initialized       = false,
    .lvd_enabled       = false,
    .wake_reason       = POWER_WAKE_REASON_UNKNOWN,
    .deepsleep_count   = 0,
    .battery_low_flag  = false
};

/* ========================================================================== */
/* Internal Helper: Detect Wake Reason                                        */
/* ========================================================================== */

power_wake_reason_t pm_detect_wake_reason(void)
{
    /*
     * Read all reset flags via SYSCTRL_GetAllRstFlag().
     * Source: cw32l010_sysctrl.h#L481
     */
    uint32_t rst_flags = SYSCTRL_GetAllRstFlag();

    /* Check reset flags in priority order */
    if (rst_flags & SYSCTRL_RESETFLAG_POR) {
        return POWER_WAKE_REASON_POR;
    }
    if (rst_flags & SYSCTRL_RESETFLAG_IWDT) {
        return POWER_WAKE_REASON_IWDT;
    }
    if (rst_flags & SYSCTRL_RESETFLAG_LVD) {
        return POWER_WAKE_REASON_LVD;
    }
    if (rst_flags & SYSCTRL_RESETFLAG_PIN) {
        return POWER_WAKE_REASON_EXT;
    }

    /*
     * If no reset flag is set but we just woke, check the RTC AWTIMER
     * status flag. Access the RTC registers via CW_RTC peripheral pointer.
     * Source: cw32l010.h RTC register map, cw32l010_rtc.h#L310-L313
     */
    RTC_UNLOCK();
    if (CW_RTC->ISR_f.AWTIMER) {
        CW_RTC->ICR_f.AWTIMER = 0;   /* Clear flag by writing 0 */
        RTC_LOCK();
        return POWER_WAKE_REASON_RTC;
    }
    RTC_LOCK();

    return POWER_WAKE_REASON_UNKNOWN;
}

/* ========================================================================== */
/* Internal Helper: RTC Initialization                                        */
/* ========================================================================== */

int32_t pm_rtc_init(void)
{
    ErrorStatus ret;
    RTC_InitTypeDef rtc_init;

    /*
     * Enable LSI clock source for RTC.
     * Source: cw32l010_sysctrl.h#L460
     */
    if (SYSCTRL_LSI_Enable() != 0) {
        LOG_ERROR("LSI enable failed");
        return ERR_BUS;
    }

    /*
     * Configure RTC init structure with LSI clock source.
     * Source: cw32l010_rtc.h#L48-L54, L129
     */
    rtc_init.RTC_ClockSource = RTC_RTCCLK_FROM_LSI;

    /* Set initial time: 00:00:00 (24-hour mode) */
    rtc_init.TimeStruct.Hour   = 0;
    rtc_init.TimeStruct.Minute = 0;
    rtc_init.TimeStruct.Second = 0;
    rtc_init.TimeStruct.AMPM   = 0;     /* Not used in 24h mode */
    rtc_init.TimeStruct.H24    = 1;     /* 24-hour mode */

    /* Set initial date: 2025-01-01, Monday */
    rtc_init.DateStruct.Year  = 25;     /* BCD: 2025 */
    rtc_init.DateStruct.Month = 1;      /* January */
    rtc_init.DateStruct.Day   = 1;      /* 1st */
    rtc_init.DateStruct.Week  = 1;      /* Monday */

    /*
     * Initialize RTC with configured parameters.
     * Source: cw32l010_rtc.h#L459
     */
    ret = RTC_Init(&rtc_init);

    return (ret == SUCCESS) ? ERR_OK : ERR_BUS;
}

/* ========================================================================== */
/* Internal Helper: RTC Auto-Wake Timer Configuration                         */
/* ========================================================================== */

void pm_awt_config(uint16_t seconds)
{
    RTC_AWTTypeDef awt_cfg;

    /*
     * AWT clock source: RTC1HZ (1 Hz) so ARR value = seconds directly.
     * Source: cw32l010_rtc.h#L63-L67, L199, L476
     */
    awt_cfg.AWT_ClockSource = PM_AWT_CLK_SRC;
    awt_cfg.AWT_ARRValue    = seconds;

    RTC_UNLOCK();
    RTC_AWTConfig(&awt_cfg);
    RTC_LOCK();
}

/* ========================================================================== */
/* Internal Helper: IWDT Initialization                                       */
/* ========================================================================== */

int32_t pm_iwdt_init(void)
{
    IWDT_InitTypeDef iwdt_init;
    int ret;

    /*
     * IWDT clock = LSI = ~10 kHz (IWDT_FREQ = 10000, cw32l010_iwdt.h#L48)
     * Prescaler DIV64: tick = 10000 / 64 = 156.25 Hz -> period ~6.4 ms
     * Reload for ~2s timeout: 2.0 / 0.0064 = 312.5 -> use 312
     * Window = 0 (no window constraint)
     *
     * Sources:
     *   - cw32l010_iwdt.h#L54  (IWDT_Prescaler_DIV64)
     *   - cw32l010_iwdt.h#L84  (IWDT_OVERFLOW_ACTION_RESET)
     *   - cw32l010_iwdt.h#L89  (IWDT_SLEEP_CONTINUE)
     *   - cw32l010_iwdt.h#L112 (IWDT_Init)
     */
    iwdt_init.IWDT_Prescaler      = IWDT_Prescaler_DIV64;
    iwdt_init.IWDT_OverFlowAction = IWDT_OVERFLOW_ACTION_RESET;
    iwdt_init.IWDT_ITState        = DISABLE;   /* No interrupt, just reset */
    iwdt_init.IWDT_Pause          = IWDT_SLEEP_CONTINUE;  /* Keep running in DeepSleep */
    iwdt_init.IWDT_ReloadValue    = 312;       /* ~2 seconds @ DIV64 */
    iwdt_init.IWDT_WindowValue    = 0;         /* No window (0 = disabled) */

    ret = IWDT_Init(&iwdt_init);
    if (ret != 0) {
        LOG_ERROR("IWDT init failed: %d", ret);
        return ERR_BUS;
    }

    /* Start the watchdog counter */
    IWDT_Cmd();

    return ERR_OK;
}

/* ========================================================================== */
/* Internal Helper: LVD Initialization                                        */
/* ========================================================================== */

void pm_lvd_init(void)
{
    LVD_InitTypeDef lvd_init;

    /*
     * Configure LVD:
     *   - Source: LVD_Source_VDDA (VDD supply) [cw32l010_lvd.h#L92]
     *   - Threshold: LVD_Threshold_2p6V (2.6V for CR2032) [cw32l010_lvd.h#L104]
     *   - Action: LVD_Action_Irq (interrupt, not reset) [cw32l010_lvd.h#L83]
     *   - Filter: LSI clock [cw32l010_lvd.h#L133], 2N6 filter time [cw32l010_lvd.h#L146]
     */
    lvd_init.LVD_Action       = LVD_Action_Irq;
    lvd_init.LVD_Source       = LVD_Source_VDDA;
    lvd_init.LVD_Threshold    = LVD_Threshold_2p6V;
    lvd_init.LVD_FilterClk    = LVD_FilterClk_LSI;
    lvd_init.LVD_FilterTime   = LVD_FilterTime_2N6;

    LVD_Init(&lvd_init);

    /* Configure trigger: level detection (battery low sustained) */
    LVD_TrigConfig(LVD_TRIG_LEVEL, ENABLE);

    /* Enable NVIC interrupt for LVD */
    LVD_EnableIrq(PM_LVD_IRQ_PRIORITY);

    /* Enable the LVD peripheral */
    LVD_Enable();
}

/* ========================================================================== */
/* Internal Helper: Enter DeepSleep                                           */
/* ========================================================================== */

void pm_enter_deepsleep(void)
{
    PWR_InitTypeDef pwr_cfg;

    /*
     * Configure low-power mode via PWR_Config:
     *   - SLEEPDEEP = enabled (enter DeepSleep on WFI) [cw32l010_pwr.h#L70]
     *   - SEVONPEND = disabled [cw32l010_pwr.h#L60]
     *   - SLEEPONEXIT = disabled [cw32l010_pwr.h#L78]
     *
     * Source: cw32l010_pwr.h#L104
     */
    pwr_cfg.PWR_Sevonpend   = PWR_Sevonpend_Disable;
    pwr_cfg.PWR_SleepDeep   = PWR_SleepDeep_Enable;
    pwr_cfg.PWR_SleepOnExit = PWR_SleepOnExit_Disable;

    PWR_Config(&pwr_cfg);

    /*
     * Ensure IWDT is refreshed before sleep so we have the full
     * ~2 second window during DeepSleep. Without this, the IWDT may
     * have already counted down part of its period.
     */
    IWDT_Refresh();

    /*
     * Enter DeepSleep via WFI.
     * SLEEPDEEP bit in SCB_SCR is already set by PWR_Config.
     * SYSCTRL_GotoDeepSleep() executes __WFI().
     *
     * Source: cw32l010_sysctrl.h#L491
     */
    SYSCTRL_GotoDeepSleep();

    /*
     * Execution resumes here after wakeup.
     * HSIOSC restarts (~4 us typ), CPU continues.
     */

    /* Increment DeepSleep counter */
    s_state.deepsleep_count++;
}

/* ========================================================================== */
/* Public API: Initialization                                                 */
/* ========================================================================== */

int32_t power_mgr_init(bool enable_lvd)
{
    int32_t ret;

    /* Detect and store wakeup reason from reset flags */
    s_state.wake_reason = pm_detect_wake_reason();

    LOG_INFO("Power manager init, wake reason: %d", (int)s_state.wake_reason);

    /*
     * Clear reset flags for next boot detection.
     * Source: cw32l010_sysctrl.h#L482
     */
    SYSCTRL_ClearRstFlag(SYSCTRL_RESETFLAG_ALL);

    /* Initialize RTC with LSI clock */
    ret = pm_rtc_init();
    if (ret != ERR_OK) {
        LOG_ERROR("RTC init failed");
        return ret;
    }

    /* Initialize IWDT for safety supervision */
    ret = pm_iwdt_init();
    if (ret != ERR_OK) {
        LOG_ERROR("IWDT init failed");
        return ret;
    }

    /* Initialize LVD if requested */
    if (enable_lvd) {
        pm_lvd_init();
        s_state.lvd_enabled = true;
        LOG_INFO("LVD enabled, threshold 2.6V");
    }

    /*
     * Enable RTC interrupt in NVIC (lowest priority).
     * RTC_IRQn = 2 (cw32l010.h#L68)
     */
    NVIC_SetPriority(RTC_IRQn, PM_RTC_IRQ_PRIORITY);
    NVIC_EnableIRQ(RTC_IRQn);

    s_state.initialized = true;
    LOG_INFO("Power manager ready");

    return ERR_OK;
}

void power_mgr_deinit(void)
{
    if (!s_state.initialized) {
        return;
    }

    /* Disable LVD */
    if (s_state.lvd_enabled) {
        LVD_Disable();
        LVD_DisableIrq();
    }

    /* Disable RTC interrupt in NVIC */
    NVIC_DisableIRQ(RTC_IRQn);

    /* Stop IWDT */
    IWDT_Stop();

    /* Reset state */
    s_state.initialized      = false;
    s_state.lvd_enabled      = false;
    s_state.battery_low_flag = false;

    LOG_INFO("Power manager deinitialized");
}

/* ========================================================================== */
/* Public API: Sleep Functions                                                */
/* ========================================================================== */

void power_mgr_sleep_150s(void)
{
    power_mgr_sleep_seconds(POWER_MGR_DEFAULT_WAKE_INTERVAL_S);
}

void power_mgr_sleep_seconds(uint16_t seconds)
{
    if (!s_state.initialized) {
        LOG_ERROR("power_mgr not initialized!");
        return;
    }

    if (seconds == 0) {
        seconds = 1;   /* Minimum 1 second */
    }

    /*
     * Step 1: Configure AWT for the desired period.
     * AWT source = RTC1HZ (1 Hz) so reload value = seconds.
     */
    pm_awt_config(seconds);

    /*
     * Step 2: Enable AWT interrupt and start the timer.
     * Source: cw32l010_rtc.h#L462 (RTC_ITConfig), L477 (RTC_AWTCmd)
     */
    RTC_UNLOCK();
    RTC_ITConfig(RTC_IT_AWTIMER, ENABLE);
    RTC_AWTCmd(ENABLE);
    RTC_LOCK();

    LOG_DEBUG("DeepSleep for %u s", (unsigned)seconds);

    /*
     * Step 3: Enter DeepSleep via PWR_Config + WFI.
     * Execution pauses here until RTC AWTIMER fires.
     */
    pm_enter_deepsleep();

    /*
     * Step 4: Woke up. Disable AWT to save power until next cycle.
     * The interrupt flag is already cleared by the ISR.
     */
    RTC_UNLOCK();
    RTC_AWTCmd(DISABLE);
    RTC_LOCK();
}

void power_mgr_sleep_light(void)
{
    if (!s_state.initialized) {
        return;
    }

    /*
     * Sleep mode (not DeepSleep):
     * - CPU clock stops
     * - Peripherals keep running
     * - Wake by any interrupt (SysTick, UART, etc.)
     *
     * Source: cw32l010_sysctrl.h#L490 (SYSCTRL_GotoSleep)
     *
     * Note: SYSCTRL_GotoSleep() clears SLEEPDEEP and executes WFI.
     */
    SYSCTRL_GotoSleep();
}

/* ========================================================================== */
/* Public API: IWDT Support                                                   */
/* ========================================================================== */

void power_mgr_iwdt_refresh(void)
{
    /*
     * Pet the watchdog to prevent reset.
     * Source: cw32l010_iwdt.h#L115
     */
    IWDT_Refresh();
}

bool power_mgr_iwdt_caused_reset(void)
{
    uint32_t rst_flags = SYSCTRL_GetAllRstFlag();
    return (rst_flags & SYSCTRL_RESETFLAG_IWDT) != 0;
}

/* ========================================================================== */
/* Public API: Battery / LVD Monitoring                                       */
/* ========================================================================== */

int32_t power_mgr_get_status(power_status_t *status)
{
    if (status == NULL) {
        return ERR_INVALID_PARAM;
    }

    status->wake_reason     = s_state.wake_reason;
    status->uptime_ms       = 0;   /* TODO: read from SysTick counter if available */
    status->vbat_mv         = 0;   /* TODO: ADC measurement of VDD for mV reading */
    status->battery_low     = s_state.battery_low_flag;
    status->deepsleep_count = s_state.deepsleep_count;

    return ERR_OK;
}

bool power_mgr_is_battery_low(void)
{
    return s_state.battery_low_flag;
}

power_wake_reason_t power_mgr_get_wake_reason(void)
{
    return s_state.wake_reason;
}

/* ========================================================================== */
/* Interrupt Handlers                                                         */
/* ========================================================================== */

void power_mgr_rtc_irq_handler(void)
{
    /*
     * Check and clear AWTIMER interrupt flag.
     * When AWTIMER bit in ISR is 1, writing 0 to ICR_f.AWTIMER clears it.
     * Source: cw32l010_rtc.h#L141, CW_RTC register map in cw32l010.h
     */
    if (CW_RTC->ISR_f.AWTIMER) {
        CW_RTC->ICR_f.AWTIMER = 0;   /* Write 0 to clear (see RTC_IE_AWTIMER_CLEAR) */
    }
}

void power_mgr_lvd_irq_handler(void)
{
    /* Set the battery low flag */
    s_state.battery_low_flag = true;

    /*
     * Clear LVD interrupt flag.
     * Source: cw32l010_lvd.h#L218
     */
    LVD_ClearIrq();
}

/* ========================================================================== */
/* RTC_IRQHandler — Vector table entry                                        */
/* ========================================================================== */

void RTC_IRQHandler(void) __attribute__((interrupt("IRQ")));
void RTC_IRQHandler(void)
{
    power_mgr_rtc_irq_handler();
}

/* ========================================================================== */
/* LVD_IRQHandler — Vector table entry (if LVD enabled)                       */
/* ========================================================================== */

void LVD_IRQHandler(void) __attribute__((interrupt("IRQ")));
void LVD_IRQHandler(void)
{
    power_mgr_lvd_irq_handler();
}
