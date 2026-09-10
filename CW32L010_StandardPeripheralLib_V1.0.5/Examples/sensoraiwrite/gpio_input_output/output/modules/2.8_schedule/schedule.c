/**
 * @file    schedule.c
 * @brief   Phase 2.8 — Schedule Module Implementation
 *
 * MCU:     CW32L010Y8M6 (TSSOP-20)
 * Module:  schedule (layer=application)
 * Bus:     POWER (CR2032)
 *
 * == Design ==
 *
 * This module is the top-level entry point for the firmware. It implements a
 * bare-metal super-loop driven by RTC periodic wakeup from DeepSleep. The
 * system spends ~99.9% of its time in DeepSleep (~2 µA), waking every 150
 * seconds for a ~260 ms measurement + ~20 ms RF transmission burst.
 *
 * == Execution Flow ==
 *
 *   Power ON (POR):
 *     → SystemInit() + clock config (HSIOSC 48 MHz)
 *     → SysTick (1 ms), power_mgr, UART, TWI GPIO, sensor_mgr init
 *     → main loop:
 *       ┌─────────────────────────────────────────────────────────┐
 *       │ sensor_mgr_run()   — measure AHT10 (×2, avg)  ~260 ms  │
 *       │ rf_twi_transmit()  — send UM2005C packet      ~20 ms   │
 *       │ power_mgr_sleep_150s() — DeepSleep            ~150 s    │
 *       └──────────── RTC AWT fires → wake ──────────────────────┘
 *
 * == Error Handling ==
 *
 *   - Sensor read failures: still builds a packet with error status and
 *     transmits (to indicate a failing sensor at the receiver).
 *   - RF TX failure: logged to console (if enabled), system still sleeps.
 *   - Consecutive failures > SCHEDULE_MAX_FAILURES_BEFORE_SKIP:
 *     skip RF TX (save battery) but still enter DeepSleep.
 *   - IWDT timeout (not refreshed during DeepSleep ≈ 2s timeout): IWDT
 *     stays active during sleep, refreshed before WFI. In DeepSleep the
 *     IWDT continues counting; with the 2-second timeout, the MCU would
 *     reset if the sleep period exceeds ~2s. HOWEVER, the IWDT is
 *     configured with a reload of ~2 seconds only for the *active* cycle.
 *     The power_manager handles this correctly by setting IWDT_SLEEP_CONTINUE
 *     and refreshing before DeepSleep entry. The RTC AWT counts in LSI
 *     clock domain, not the IWDT clock domain. For DeepSleep durations
 *     longer than the IWDT timeout, the power_manager disables IWDT before
 *     DeepSleep or uses a longer IWDT period. See power_manager.c for details.
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
 *   - cw32l010_sysctrl.h#L458 (SYSCTRL_HSI_Enable)
 *   - cw32l010_systick.h#L58  (InitTick)
 *   - system_cw32l010.h#L66   (SystemInit)
 *   - system_cw32l010.h#L67   (SystemCoreClockUpdate)
 */

#include "schedule.h"
#include <stdarg.h>

/* ========================================================================== */
/* log_printf Implementation (forward-declared in common_types.h)            */
/* ========================================================================== */

/*
 * The LOG_* macros in common_types.h call log_printf().  The platform
 * expects each compilation unit to either provide log_printf() or link
 * against an implementation.  Since the uart_console module provides
 * uart_printf() but not log_printf(), we provide a thin wrapper here.
 *
 * Source: output/design/common_types.h#L132 (forward declaration)
 * Source: output/modules/2.3_uart_console/interface.h#L115 (uart_printf)
 */
void log_printf(const char *fmt, ...)
{
#if CONFIG_CONSOLE_ENABLE
    /* Forward format + args to uart_vprintf for actual console output.
     * uart_vprintf uses the same format specifiers: %s, %d, %u, %x, %X, %c, %f.
     * Common_types.h LOG_* macros prepend "[E]"/"[W]"/"[I]"/"[D]" + "\r\n". */
    va_list args;
    va_start(args, fmt);
    uart_vprintf(fmt, args);
    va_end(args);
#else
    (void)fmt;
#endif
}

/*
 * Override LOG_* macros to call uart_printf directly in this module,
 * bypassing the log_printf forward declaration issue (log_printf provided
 * as a stub above for linking, but we call uart_printf for actual output).
 * We wrap uart_printf with the same prefix convention.
 */
#if CONFIG_CONSOLE_ENABLE
    #undef LOG_ERROR
    #undef LOG_WARN
    #undef LOG_INFO
    #undef LOG_DEBUG
    #define LOG_ERROR(fmt, ...)   uart_printf("[E] " fmt "\r\n", ##__VA_ARGS__)
    #define LOG_WARN(fmt, ...)    uart_printf("[W] " fmt "\r\n", ##__VA_ARGS__)
    #define LOG_INFO(fmt, ...)    uart_printf("[I] " fmt "\r\n", ##__VA_ARGS__)
    #define LOG_DEBUG(fmt, ...)   uart_printf("[D] " fmt "\r\n", ##__VA_ARGS__)
#endif

/* ========================================================================== */
/* Module Instance                                                            */
/* ========================================================================== */

schedule_state_t g_schedule;

/* ========================================================================== */
/* Internal: Clock Initialization                                             */
/* ========================================================================== */

static int32_t sch_clock_init(void)
{
    int32_t ret;

    /*
     * Enable HSIOSC (48 MHz internal RC oscillator).
     * Source: cw32l010_sysctrl.h#L458
     * SYSCTRL_HSI_Enable(0) enables with no additional divider (48 MHz).
     */
    ret = SYSCTRL_HSI_Enable(0);
    if (ret != 0) {
        LOG_ERROR("schedule: HSIOSC enable failed ret=%d", (int)ret);
        return ERR_BUS;
    }

    /*
     * Wait for HSIOSC stable flag.
     * Source: cw32l010_sysctrl.h#L472
     */
    while (SYSCTRL_GetStableFlag(SYSCTRL_FLAG_HSISTABLE) != SET) {
        /* Spin until HSIOSC is stable (~4 µs typical) */
    }

    /*
     * Switch system clock to HSIOSC.
     * Source: cw32l010_sysctrl.h#L449
     */
    SYSCTRL_SYSCLKSRC_Config(SYSCTRL_SYSCLKSRC_HSI);

    /*
     * Configure HCLK and PCLK dividers.
     * Source: cw32l010_sysctrl.h#L447-L448
     */
    SYSCTRL_HCLKPRS_Config(SCHEDULE_HCLK_PRS);
    SYSCTRL_PCLKPRS_Config(SCHEDULE_PCLK_PRS);

    /*
     * Update SystemCoreClock global variable.
     * Source: system_cw32l010.h#L67
     */
    SystemCoreClockUpdate();

    /* Verify the clock frequency is as expected */
    if (SystemCoreClock != SCHEDULE_SYSCLK_HZ) {
        LOG_WARN("schedule: SystemCoreClock=%lu expected=%lu",
                 (unsigned long)SystemCoreClock,
                 (unsigned long)SCHEDULE_SYSCLK_HZ);
        /* Non-fatal: continue with actual clock */
    }

    LOG_INFO("schedule: SystemCoreClock=%lu Hz", (unsigned long)SystemCoreClock);

    return ERR_OK;
}

/* ========================================================================== */
/* Internal: Microsecond Delay (NOP loop calibrated for 48 MHz)              */
/* ========================================================================== */

static void sch_delay_us(uint32_t us)
{
    /*
     * At 48 MHz, one NOP instruction takes ~20.8 ns.
     * A simple loop: 1 compare + 1 branch + 1 NOP = ~3 instructions.
     * Using a calibrated loop: approximately 12 NOPs per μs.
     *
     * For simplicity and reliability, use FirmwareDelay() from system_cw32l010
     * if available. Otherwise use a simple NOP loop.
     * Source: system_cw32l010.h#L68
     */
    FirmwareDelay(us * 12);
}

static void sch_delay_ms(uint32_t ms)
{
    /*
     * Use SysTickDelay for millisecond delays.
     * Source: cw32l010_systick.h#L59
     */
    SysTickDelay(ms);
}

/* ========================================================================== */
/* Public API: System Initialization                                         */
/* ========================================================================== */

int32_t schedule_init(void)
{
    int32_t ret;

    /* ---- Step 1: SystemInit (CMSIS default) ---- */
    /*
     * Performs default system clock and vector table setup.
     * Source: system_cw32l010.h#L66
     */
    SystemInit();

    /* ---- Step 2: Clock Configuration (HSIOSC 48 MHz) ---- */
    ret = sch_clock_init();
    if (ret != ERR_OK) {
        /* Clock init failure is fatal */
        return ret;
    }

    /* ---- Step 3: SysTick (1 ms tick) ---- */
    /*
     * InitTick configures SysTick for 1 ms interrupts.
     * Source: cw32l010_systick.h#L58
     */
    InitTick(SystemCoreClock);

    /* ---- Step 4: Power Manager (RTC, LSI, IWDT, LVD) ---- */
    /*
     * power_mgr_init(true) initializes:
     *   - LSI clock for RTC
     *   - RTC with LSI source
     *   - IWDT with ~2s timeout (continues in DeepSleep)
     *   - LVD with ~2.6V threshold (interrupt mode)
     * Source: output/modules/2.5_power_manager/interface.h#L102
     */
    ret = power_mgr_init(true);
    if (ret != ERR_OK) {
        LOG_ERROR("schedule: power_mgr_init failed err=%d", (int)ret);
        return ret;
    }

    /* ---- Step 5: UART Console (compile-time toggle) ---- */
    /*
     * When CONFIG_CONSOLE_ENABLE == 0, uart_console_init is still called
     * but does minimal work (pins as high-Z). When enabled, configures
     * UART1 @ 115200 8N1 on PA05/PA06.
     * Source: output/modules/2.3_uart_console/interface.h#L62
     */
    ret = uart_console_init(SystemCoreClock);
    if (ret != ERR_OK) {
        LOG_WARN("schedule: uart_console_init failed err=%d (non-fatal)", (int)ret);
        /* Non-fatal: system can still operate without console */
    }

    CONSOLE_PRINT("\r\n[CW32L010 Sensor Node v1.0]\r\n");

    /* ---- Step 6: RF TWI GPIO Initialization ---- */
    /*
     * rf_twi_init() configures PB02 (CLK) and PB03 (DATA) as push-pull outputs
     * for UM2005C TWI bit-bang protocol.
     * Source: output/modules/2.4_rf_twi_driver/interface.h#L81
     */
    ret = rf_twi_init();
    if (ret != ERR_OK) {
        LOG_ERROR("schedule: rf_twi_init failed err=%d", (int)ret);
        return ret;
    }

    /* ---- Step 7: Sensor Manager (AHT10 + Packet Builder) ---- */
    /*
     * sensor_mgr_init() initializes AHT10 I2C driver and packet builder
     * (which caches MCU UID for RF packets).
     * Source: output/modules/2.7_sensor_mgr/interface.h#L94
     */
    ret = sensor_mgr_init();
    if (ret != ERR_OK) {
        LOG_ERROR("schedule: sensor_mgr_init failed err=%d", (int)ret);
        return ret;
    }

    /* ---- Step 8: Determine boot reason ---- */
    /*
     * power_mgr_get_wake_reason() returns why the MCU booted.
     * On cold boot (POR), we log and proceed normally.
     * Source: output/modules/2.5_power_manager/interface.h#L208
     */
    power_wake_reason_t reason = power_mgr_get_wake_reason();
    g_schedule.cold_boot = (reason == POWER_WAKE_REASON_POR);

    if (g_schedule.cold_boot) {
        CONSOLE_PRINT("[SCHEDULE] Cold boot (POR). Reason=%d\r\n", (int)reason);
        /* Small delay to allow AHT10 to power-up after POR */
        sch_delay_ms(SCHEDULE_BOOT_DELAY_MS);
    } else {
        CONSOLE_PRINT("[SCHEDULE] RTC wake (reason=%d). Cycle=%lu\r\n",
                      (int)reason, (unsigned long)g_schedule.cycle_count);
    }

    /* ---- Mark initialized ---- */
    g_schedule.initialized     = true;
    g_schedule.cycle_count     = 0;
    g_schedule.failure_count   = 0;
    g_schedule.data_valid      = false;
    g_schedule.packet_valid    = false;

    LOG_INFO("schedule: initialized (cold_boot=%d)", g_schedule.cold_boot ? 1 : 0);

    return ERR_OK;
}

/* ========================================================================== */
/* Public API: Main Cycle                                                     */
/* ========================================================================== */

int32_t schedule_run_cycle(void)
{
    int32_t  ret;
    bool     skip_tx = false;

    /* ---- Pre-flight: check if we're initialized ---- */
    if (!g_schedule.initialized) {
        LOG_ERROR("schedule: not initialized");
        return ERR_NOT_INIT;
    }

    /* ---- Step 1: Refresh IWDT before starting active period ---- */
    /*
     * IWDT is running with ~2s timeout. Refresh now to give us the full
     * ~2s for the active cycle (~280ms typical).
     * Source: output/modules/2.5_power_manager/interface.h#L169
     */
    power_mgr_iwdt_refresh();

    /* ---- Step 2: Sensor Measurement ---- */
    /*
     * sensor_mgr_run() performs:
     *   1. Check battery status
     *   2. AHT10 sample #1 (~80ms)
     *   3. Wait 100ms
     *   4. AHT10 sample #2 (~80ms)
     *   5. Average temperature and humidity
     *   6. Build RF packet
     *   7. Console output (if enabled)
     *
     * Returns ERR_OK even if one sample fails (uses valid sample).
     * Returns ERR_READ_FAIL only if both samples fail.
     * Source: output/modules/2.7_sensor_mgr/interface.h#L146
     */
    ret = sensor_mgr_run(&g_schedule.last_data, &g_schedule.last_packet);

    if (ret == ERR_OK) {
        g_schedule.data_valid   = true;
        g_schedule.packet_valid = true;
        g_schedule.failure_count = 0;
    } else if (ret == ERR_READ_FAIL) {
        g_schedule.failure_count++;
        g_schedule.data_valid   = false;
        g_schedule.packet_valid = false;

        LOG_WARN("schedule: sensor read failed (consecutive=%lu)",
                 (unsigned long)g_schedule.failure_count);

        /* Still attempt to get packet data (may contain error status) */
        const rf_packet_t *pkt = sensor_mgr_get_last_packet();
        if (pkt != NULL) {
            g_schedule.last_packet = *pkt;
            g_schedule.packet_valid = true;
        }

        const sensor_data_t *data = sensor_mgr_get_last_data();
        if (data != NULL) {
            g_schedule.last_data = *data;
            g_schedule.data_valid = true;
        }

        /* Check if we should skip TX to save battery */
        if (g_schedule.failure_count >= SCHEDULE_MAX_FAILURES_BEFORE_SKIP) {
            skip_tx = true;
            LOG_WARN("schedule: skipping RF TX (failures=%lu)",
                     (unsigned long)g_schedule.failure_count);
        }
    } else {
        /* Other error (e.g., ERR_NOT_INIT) */
        LOG_ERROR("schedule: sensor_mgr_run error=%d", (int)ret);
        /* Still try to sleep — no data to send */
        power_mgr_sleep_150s();
        return ret;
    }

    /* ---- Step 3: RF Transmission ---- */
    if (!skip_tx && g_schedule.packet_valid) {
        /*
         * rf_twi_transmit() performs:
         *   TWI_ON → write 14-byte packet → TWI_OFF → hold CLK low
         * Full sequence: ~20ms including wake + TX + sleep.
         * Source: output/modules/2.4_rf_twi_driver/interface.h#L207
         */
        int32_t rf_ret = rf_twi_transmit((const uint8_t *)&g_schedule.last_packet);

        if (rf_ret == ERR_OK) {
            CONSOLE_PRINT("[SCHEDULE] RF TX OK\r\n");
        } else {
            LOG_WARN("schedule: rf_twi_transmit failed err=%d", (int)rf_ret);
        }
    } else if (skip_tx) {
        CONSOLE_PRINT("[SCHEDULE] TX skipped (sensor failures)\r\n");
    } else {
        LOG_DEBUG("schedule: no valid packet to transmit");
    }

    /* ---- Step 4: Increment cycle counter ---- */
    g_schedule.cycle_count++;

    CONSOLE_PRINT("[SCHEDULE] Cycle %lu complete. Sleeping %u s...\r\n",
                  (unsigned long)g_schedule.cycle_count,
                  (unsigned)SCHEDULE_WAKE_INTERVAL_S);

    /* ---- Step 5: Enter DeepSleep ---- */
    /*
     * power_mgr_sleep_150s() blocks until RTC AWT fires (~150s).
     * Configures AWT, enables interrupt, sets SLEEPDEEP, refreshes IWDT,
     * then executes WFI. On wake, clears AWT interrupt and returns.
     * Source: output/modules/2.5_power_manager/interface.h#L135
     */
    power_mgr_sleep_150s();

    /* ---- Woke up! ---- */
    return ERR_OK;
}

/* ========================================================================== */
/* Public API: Status / Diagnostics                                           */
/* ========================================================================== */

uint32_t schedule_get_cycle_count(void)
{
    return g_schedule.cycle_count;
}

uint32_t schedule_get_failure_count(void)
{
    return g_schedule.failure_count;
}

bool schedule_is_cold_boot(void)
{
    return g_schedule.cold_boot;
}

void schedule_reset_counters(void)
{
    g_schedule.cycle_count     = 0;
    g_schedule.failure_count   = 0;
    g_schedule.data_valid      = false;
    g_schedule.packet_valid    = false;
}

/* ========================================================================== */
/* Main Entry Point                                                           */
/* ========================================================================== */
#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    while (1)
    {
    }
}
#endif
/**
 * @brief  Firmware entry point — called by reset vector
 *
 * System initialization followed by infinite measurement/sleep loop.
 *
 * Sequence:
 *   1. schedule_init() — clock, power, UART, TWI, sensor init
 *   2. main loop: schedule_run_cycle() infinite
 *
 * The main loop never returns. On critical init failure, the MCU is
 * forced into an infinite error loop with IWDT reset enabled.
 *
 * @return int — never returns under normal operation
 */
int main(void)
{
    int32_t ret;

    /* ---- System Initialization ---- */
    ret = schedule_init();

    if (ret != ERR_OK) {
        /*
         * Critical initialization failure.
         * Log and enter error loop; IWDT will reset the system.
         */
        LOG_ERROR("schedule: init failed err=%d — halting", (int)ret);

        /* Try to get some output via console even in error state */
        CONSOLE_PRINT("[FATAL] Init failed err=%d\r\n", (int)ret);

        /*
         * Error loop — IWDT will fire and reset after ~2s.
         * Refresh IWDT here to give more time for debug, but eventually
         * let it reset to attempt recovery.
         */
        volatile uint32_t timeout = 1000000;
        while (timeout > 0) {
            timeout--;
        }
        /* Let IWDT fire and reset */
        while (1) {
            /* IWDT reset will occur */
            (void)0;
        }
    }

    /* ---- Print welcome banner ---- */
    CONSOLE_PRINT("[SCHEDULE] Entering main loop. Wake interval=%u s\r\n",
                  (unsigned)SCHEDULE_WAKE_INTERVAL_S);

    /* ---- Main Loop (infinite) ---- */
    while (1) {
        /*
         * Execute one complete measurement → TX → sleep cycle.
         * This function blocks for the full ~150s duration.
         * On error, the system still enters DeepSleep and retries.
         */
        (void)schedule_run_cycle();
    }

    /* Unreachable */
}
