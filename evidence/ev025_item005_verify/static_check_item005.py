#!/usr/bin/env python3
"""Static and boundary checks for ITEM-005 (main single transaction / BOOT / UID zero).

Scope: the current sensor firmware sources (no execution).
Usage: python static_check_item005.py [gpio_input_output_dir]
Exit code 0 only if every check passes.
"""
import os
import re
import sys

root = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "..", "..", "CW32L010_StandardPeripheralLib_V1.0.5", "Examples", "sensor", "gpio_input_output")
root = os.path.abspath(root)


def read(rel):
    with open(os.path.join(root, rel), encoding="utf-8", errors="replace") as f:
        return f.read()


main_c = read("USER/src/main.c")
measure_c = read("USER/src/measure.c")
trace_c = read("USER/src/debug_trace.c")
light_c = read("USER/src/light.c")
cfg_h = read("USER/inc/sensor_config.h")
all_user = main_c + measure_c + trace_c + light_c

checks = []


def chk(cond, msg):
    checks.append((bool(cond), msg))


# ---------------- main.c: BOOT before UID-zero fatal branch ----------------
uid_pos = main_c.find("if (k == 0u)")
boot_pos = main_c.find("debug_trace_boot(")
chk(uid_pos > 0, "main.c contains the UID-zero fatal branch")
chk(boot_pos > 0 and boot_pos < uid_pos, "BOOT call precedes the UID-zero branch")
for name in ("debug_trace_vdd(", "debug_trace_iotest(", "debug_trace_iosig(", "debug_trace_bus("):
    p = main_c.find(name)
    chk(p > 0 and p < uid_pos, f"{name} precedes the UID-zero branch")

# ---------------- UID-zero block contents ----------------
uid_block = main_c[uid_pos:main_c.find("k = 10000", uid_pos)]
chk("report_req = 0u" in uid_block, "UID-zero: report_req cleared")
chk("sample_flag = 0u" in uid_block, "UID-zero: sample_flag cleared")
chk("light_init()" in uid_block, "UID-zero: light_init() called (PB05 low / ADC off)")
chk("debug_trace_flush_close()" in uid_block, "UID-zero: UART closed")
chk(re.search(r"while\s*\(\s*1\s*\)\s*\{?\s*SYSCTRL_GotoDeepSleep\s*\(\s*\)", uid_block) is not None,
    "UID-zero: direct SYSCTRL_GotoDeepSleep() loop")
for bad in ("temperature_process", "send_data_to_gateway", "sensor_trace_fetch", "go_to_sleep"):
    chk(bad not in uid_block, f"UID-zero: no {bad} in the fatal branch")
chk(re.search(r"while\s*\(\s*1\s*\)\s*\{?\s*go_to_sleep\s*\(\s*\)", main_c) is None,
    "old UID-zero go_to_sleep spin loop is absent")

# ---------------- main loop: one transaction per opportunity ----------------
loop_pos = main_c.find("while(1)", main_c.find("k = 10000"))
chk(loop_pos > 0, "main loop exists after the UID-zero branch")
gate_pos = main_c.find("if (sample_flag != 0u)", loop_pos)
chk(gate_pos > loop_pos, "main loop is gated by sample_flag != 0")
tx = main_c[gate_pos:main_c.find("go_to_sleep();", gate_pos)]
t_temp = tx.find("temperature_process()")
t_send = tx.find("send_data_to_gateway()")
t_fetch = tx.find("sensor_trace_fetch(&trace)")
t_s = tx.find("debug_trace_sample(&trace)")
t_close = tx.find("debug_trace_flush_close()")
chk(t_temp > 0 and t_send > t_temp, "transaction: temperature_process -> send_data_to_gateway")
chk(t_fetch > t_send and t_s > t_fetch, "transaction: trace fetch -> S line after send")
chk(t_close > t_s, "transaction: UART close after S")
chk(main_c.count("debug_trace_sample(") == 1, "exactly one S emission point in main.c")
chk(main_c.count("temperature_process()") == 1, "exactly one temperature_process() call in main.c")
chk(main_c.count("send_data_to_gateway()") == 1, "exactly one send_data_to_gateway() call in main.c")
# close + sleep are outside the sample_flag block (every loop round)
after = main_c[main_c.find("}", t_s):]
chk(after.find("debug_trace_flush_close()") >= 0 and after.find("go_to_sleep()") >= 0,
    "close + sleep run on every loop round, after the gated transaction")

# ---------------- RTC cadence and ISR boundary ----------------
chk("RTC_SetInterval(RTC_INTERVAL_EVERY_1M)" in main_c, "RTC interval configured to 1 minute")
chk(re.search(r"#define\s+SENSOR_SAMPLE_TICKS\s+3u", cfg_h) is not None, "SENSOR_SAMPLE_TICKS == 3")
chk(re.search(r"#define\s+SENSOR_RTC_TICK_PERIOD_MIN\s+1u", cfg_h) is not None,
    "SENSOR_RTC_TICK_PERIOD_MIN == 1")
isr = main_c[main_c.find("void RTC_IRQHandlerCallBack"):main_c.find("void RTC_Configuration")]
chk("rtc_tick_total++" in isr and "rtc_set_cnt++" in isr, "ISR only advances tick counters")
chk("rtc_set_cnt >= SENSOR_SAMPLE_TICKS" in isr and "sample_flag = 1" in isr,
    "ISR raises sample_flag every 3 ticks")
for bad in ("GPIO_", "ADC_", "UART_", "bsp_i2c", "gxht40", "light_", "app_um2005"):
    chk(bad not in isr, f"ISR contains no {bad} peripheral/business call")

# ---------------- low-power / pin boundaries ----------------
chk("SYSCTRL_AHBPeriphReset" not in all_user, "no GPIOA peripheral reset in USER sources")
chk(re.search(r"GPIO_PIN_[78]", all_user) is None, "no PA07/PA08 (SWD) pin reference in USER sources")
chk(main_c.count("//\tFLASH_SetReadOutLevel(FLASH_RDLEVEL2);") == 1,
    "Flash read-protection line stays commented")
chk("SYSCTRL_APBPeriphReset1(DEBUG_UART_CLK, ENABLE)" in trace_c and
    "SYSCTRL_APBPeriphClk_Enable1(DEBUG_UART_CLK, DISABLE)" in trace_c,
    "UART close resets/disables UART1 only")
chk("DEBUG_UART_TX_GPIO_PIN" in trace_c and "DEBUG_UART_RX_GPIO_PIN" in trace_c,
    "UART close reclaims only PA05/PA06")
chk(re.search(r"#define\s+DEBUG_UART_TX_GPIO_PIN\s+GPIO_PIN_6", cfg_h) is not None and
    re.search(r"#define\s+DEBUG_UART_RX_GPIO_PIN\s+GPIO_PIN_5", cfg_h) is not None,
    "UART pins are PA06=TX / PA05=RX")

# ---------------- light_init: PB05 low + ADC off ----------------
li = light_c[light_c.find("void light_init(void)"):light_c.find("static bool light_adc_read_once")]
chk("GPIO_WritePin(LIGHT_POWER_PORT, LIGHT_POWER_PIN, GPIO_Pin_RESET)" in li,
    "light_init(): PB05 driven low")
adc_cfg = light_c[light_c.find("static void light_adc_config(void)"):light_c.find("void light_init(void)")]
chk("ADC_Disable()" in adc_cfg and "light_adc_config()" in li,
    "light_init(): ADC configured then disabled (off)")
chk("LIGHT_ADC_PIN" in li and "GPIO_MODE_ANALOG" in li, "light_init(): PB04 configured analog")

# ---------------- go_to_sleep condition ----------------
gos = measure_c[measure_c.find("void go_to_sleep(void)"):]
chk("(report_req == 0u) && (sample_flag == 0u)" in gos,
    "go_to_sleep(): deep sleep only when report_req and sample_flag are clear")

failed = [m for ok, m in checks if not ok]
for ok, m in checks:
    print(("PASS  " if ok else "FAIL  ") + m)
print(f"result: {len(checks) - len(failed)} passed, {len(failed)} failed")
sys.exit(1 if failed else 0)
