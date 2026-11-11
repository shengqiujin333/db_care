#!/usr/bin/env python3
"""EV-032 static checks (read-only) for the LIGHT-DEF-2 firmware delivery.

Checks the source tree at the tester's current HEAD. Writes a PASS/FAIL line per
check and exits non-zero if any check fails.
"""
import io, os, re, sys

BASE = "D:/mypro/beiwo2/AIP/CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output"
USER = os.path.join(BASE, "USER")
TEST = os.path.join(BASE, "test")

pass_n = fail_n = 0
def check(cond, msg):
    global pass_n, fail_n
    if cond:
        pass_n += 1; print(f"PASS  {msg}")
    else:
        fail_n += 1; print(f"FAIL  {msg}")

def read(path):
    return io.open(path, encoding="utf-8", errors="replace").read()

def walk(root, exts=(".c", ".h")):
    for d, _, files in os.walk(root):
        for f in files:
            if f.endswith(exts):
                yield os.path.join(d, f)

def usersrc():
    return list(walk(USER))

cfg = read(os.path.join(USER, "inc", "sensor_config.h"))
fwcore = read(os.path.join(USER, "src", "fw_core.c"))
lightc = read(os.path.join(USER, "src", "light.c"))
mainc = read(os.path.join(USER, "src", "main.c"))
tracec = read(os.path.join(USER, "src", "debug_trace.c"))

# 1. delivery threshold is 0 (criterion disabled)
m = re.search(r"#ifndef LIGHT_DARK_THRESHOLD_CODE\s*\n#define LIGHT_DARK_THRESHOLD_CODE\s+(\w+)", cfg)
check(m is not None and m.group(1) in ("0u", "0"), "LIGHT_DARK_THRESHOLD_CODE default is 0 (delivery: disabled)")

# 2. no legacy 1/3 macros anywhere in the sensor project
legacy = []
for path in list(walk(USER)) + list(walk(TEST, (".c", ".h", ".sh"))):
    txt = read(path)
    for mac in ("LIGHT_DARK_REF_CODE", "LIGHT_DARK_CALIBRATED"):
        if mac in txt:
            legacy.append((os.path.relpath(path, BASE), mac))
check(not legacy, f"legacy 1/3 macros absent from USER/ and test/ ({legacy or 'none'})")

# 3. pure threshold comparison in fw_core.c
check("mean_adc_code >= threshold_code" in fwcore and "threshold_code == 0u" in fwcore,
      "fw_core.c: dark = valid && threshold != 0 && mean >= threshold (stateless)")
check("3 *" not in fwcore and "3*" not in fwcore, "fw_core.c: no 1/3 multiplication remains")

# 4. light.c wiring
check("r.valid = r.adc_ok && (LIGHT_DARK_THRESHOLD_CODE != 0);" in lightc,
      "light.c: valid = adc_ok && (T != 0)")
check("r.dark  = light_is_dark(r.mean_adc_code, r.valid, (uint16_t)LIGHT_DARK_THRESHOLD_CODE);" in lightc,
      "light.c: dark = light_is_dark(mean, valid, T)")
check("r.mean_adc_code = 0u" in lightc or "r.mean_adc_code = 0" in lightc,
      "light.c: invalid path leaves mean 0 (no full-scale synthesis)")

# 5. report rule constants and direction
check(re.search(r"#define\s+SENSOR_REPORT_RISE_X10\s+9u?", cfg) is not None,
      "sensor_config.h: SENSOR_REPORT_RISE_X10 present")
check(re.search(r"#define\s+SENSOR_REPORT_HIGH_X10\s+350u?", cfg) is not None,
      "sensor_config.h: SENSOR_REPORT_HIGH_X10 present")
check("SENSOR_REPORT_DROP_X10" not in cfg, "sensor_config.h: no legacy drop constant")
check("(int32_t)cur_temp_x10 > (int32_t)SENSOR_REPORT_HIGH_X10" in fwcore and
      "((int32_t)cur_temp_x10 - (int32_t)prev_temp_x10)" in fwcore,
      "fw_core.c: rising direction with int32 difference and strict >35.0 C")

# 6. cadence / frame constants
check(re.search(r"#define\s+SENSOR_SAMPLE_TICKS\s+3u?", cfg) is not None, "sensor_config.h: 3 ticks per sample")
check(re.search(r"#define\s+SENSOR_RF_FRAME_LEN\s+10u?", cfg) is not None, "sensor_config.h: 10-byte RF frame")

# 7. power-on line order in main.c
order = []
for name in ("debug_trace_boot(", "debug_trace_vdd(", "debug_trace_iotest(", "debug_trace_iosig(", "debug_trace_bus("):
    order.append(mainc.find(name))
check(all(i >= 0 for i in order) and order == sorted(order),
      f"main.c: power-on order BOOT<VDD<IOTEST<IOSIG<BUS ({order})")
check("debug_trace_flush_close()" in mainc, "main.c: UART1 closed after each trace")

# 8. frozen S-line literal order (assembled from trace_puts segments)
start = tracec.find("trace_putc('S');")
labels = [" k=", " p=", " H=", " V=", " o=", " n=", " x=", " a=",
          " D=", " t=", " h=", " q=", " r=", " s=", " y="]
pos = [tracec.find(t, start) if start >= 0 else -1 for t in labels]
check(start >= 0 and all(i >= 0 for i in pos) and pos == sorted(pos),
      f"debug_trace.c: frozen S line field order ({pos})")

# 9. pin ownership (macro single source)
check("LIGHT_POWER_PORT" in cfg and "LIGHT_ADC_PORT" in cfg, "sensor_config.h: light pin macros defined once")
check("DEBUG_UART_TX_PORT" in cfg or "DEBUG_UART" in cfg, "sensor_config.h: debug UART macros defined once")

# 10. no float / dynamic allocation in product sources
bad = []
for p in usersrc():
    txt = read(p)
    if re.search(r"\b(float|double)\b", txt): bad.append((os.path.basename(p), "float/double"))
    if re.search(r"\b(malloc|calloc|realloc|free)\s*\(", txt): bad.append((os.path.basename(p), "dynamic memory"))
check(not bad, f"USER/src + inc: no float/double or dynamic memory ({bad or 'none'})")

# 11. legacy subsystems inactive
refs = []
for p in usersrc():
    txt = read(p)
    for tok in ("optcfg_", "hall_", "params_", "history_"):
        if tok in txt: refs.append((os.path.basename(p), tok))
check(not refs, f"no active legacy hall/optcfg/params/history references ({refs or 'none'})")

# 12. debug UART guard single source
check("SENSOR_DEBUG_TX_GUARD" in cfg and "SENSOR_DEBUG_TX_GUARD" in tracec,
      "bounded UART TX guard is a single configured constant")

print(f"\nSUMMARY: {pass_n} passed, {fail_n} failed")
sys.exit(0 if fail_n == 0 else 1)
