#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Independent negative controls for ITEM-007 self-check assets.

Each control mutates a *copy* of the current product source (never the product
tree) to reproduce one previously-disproved implementation behavior, then
compiles the actual implementation-side self-check harness against that copy.
A self-check that cannot fail on a wrong implementation is not a valid gate.

All sources/harnesses are read from the working tree at the recorded git HEAD.
"""
import os
import re
import shutil
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
PROJ = os.path.join(ROOT, "CW32L010_StandardPeripheralLib_V1.0.5", "Examples",
                    "sensor", "gpio_input_output")
TEST = os.path.join(PROJ, "test")
SRC = os.path.join(PROJ, "USER", "src")
INC = os.path.join(PROJ, "USER", "inc")
COMMON = os.path.join(PROJ, "COMMON")
UM2005C = os.path.join(PROJ, "UM2005C")
OUT = os.path.dirname(os.path.abspath(__file__))
NEG = os.path.join(OUT, "negctl")
CC = "gcc"

os.makedirs(NEG, exist_ok=True)


def read_norm(path):
    with open(path, "r", encoding="utf-8", errors="replace", newline="") as f:
        return f.read().replace("\r\n", "\n")


def must_replace(text, old, new):
    n = text.count(old)
    if n != 1:
        raise SystemExit("MUTATION ANCHOR NOT UNIQUE (%d): %r" % (n, old[:80]))
    return text.replace(old, new)


def run(cmd, cwd, timeout=120):
    p = subprocess.run(cmd, cwd=cwd, stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT, timeout=timeout)
    return p.returncode, p.stdout.decode("utf-8", "replace")


def compile_and_run(name, harness_cmd, exe_name, log):
    exe = os.path.join(NEG, exe_name)
    cmd = [CC] + harness_cmd + ["-o", exe]
    rc, out = run(cmd, TEST)
    with open(os.path.join(NEG, name + ".compile.log"), "w", encoding="utf-8") as f:
        f.write("CMD: %s\nCWD: %s\nRC: %d\n%s" % (" ".join(cmd), TEST, rc, out))
    if rc != 0:
        return ("COMPILE_FAIL", rc, out)
    try:
        rrc, rout = run([exe], NEG, timeout=120)
    except subprocess.TimeoutExpired:
        return ("RUN_TIMEOUT", -1, "")
    with open(os.path.join(NEG, name + ".run.log"), "w", encoding="utf-8") as f:
        f.write(rout)
    fails = [ln.strip() for ln in rout.splitlines() if "FAIL" in ln or "failed" in ln]
    return ("OK", rrc, "\n".join(fails))


def fail_lines(text):
    return [ln.strip() for ln in text.splitlines() if "FAIL" in ln]


variants = []

# ---------------------------------------------------------------- N1: legacy gate
fw = read_norm(os.path.join(SRC, "fw_core.c"))
fw1 = must_replace(
    fw,
    "    if (!have_prev) {\n        return false;                   /* 无前一有效温度: 升温分支恒假 */\n    }",
    "    if (cur_temp_x10 == SENSOR_REPORT_HIGH_X10) {\n        return false;                   /* legacy: 35.0C 整体排除 */\n    }\n    if (!have_prev) {\n        return false;\n    }")
fw1 = must_replace(
    fw1,
    "    return ((int32_t)cur_temp_x10 - (int32_t)prev_temp_x10)\n           > (int32_t)SENSOR_REPORT_RISE_X10;",
    "    return ((int32_t)prev_temp_x10 - (int32_t)cur_temp_x10)\n           > (int32_t)SENSOR_REPORT_RISE_X10;   /* legacy: 降温方向 */")
p_fw1 = os.path.join(NEG, "fw_core_legacy_drop.c")
open(p_fw1, "w", encoding="utf-8", newline="\n").write(fw1)
variants.append(("N1a_host_sensor_core_vs_legacy_gate",
                 ["-std=c11", "-Wall", "-Wextra", "-I" + INC,
                  os.path.join(TEST, "host_sensor_core_test.c"), p_fw1, "-lm"],
                 "n1a.exe", "must FAIL v3/v6/v7"))
variants.append(("N1b_host_pure_check_vs_legacy_gate",
                 ["-std=c11", "-Wall", "-Wextra", "-I" + INC,
                  os.path.join(TEST, "host_fw_core_pure_check.c"), p_fw1, "-lm"],
                 "n1b.exe", "must FAIL v3/v6/v7"))

# ---------------------------------------------------------------- N2: trace post-advance prev
ms = read_norm(os.path.join(SRC, "measure.c"))
ms2 = must_replace(ms,
                   "    s_trace_prev_temp_x10 = prev_for_decision;",
                   "    s_trace_prev_temp_x10 = s_prev_temp_x10;   /* legacy: 推进后的值 */")
p_ms2 = os.path.join(NEG, "measure_trace_post_advance.c")
open(p_ms2, "w", encoding="utf-8", newline="\n").write(ms2)
variants.append(("N2_measure_flow_vs_post_advance_prev",
                 ["-std=c11", "-Wall", "-Wextra", "-DSENSOR_CONFIG_NO_MCU", "-DSENSOR_DEBUG_UART=1",
                  "-I" + os.path.join(TEST, "mock_measure_mcu"),
                  "-I" + INC, "-I" + COMMON,
                  "-I" + UM2005C,
                  os.path.join(TEST, "host_measure_flow_check.c"), p_ms2,
                  os.path.join(SRC, "sf_i2c.c"), os.path.join(SRC, "fw_core.c"), "-lm"],
                 "n2.exe", "must FAIL A/C p snapshot"))

# ---------------------------------------------------------------- N3: re-encode per retry
ms3 = must_replace(
    ms,
    "    for (attempt = 0u; attempt < (uint8_t)SENSOR_RF_TX_RETRY; attempt++) {\n        if (app_um2005C_send_data_timeout(send_data, SENSOR_RF_FRAME_LEN,",
    "    for (attempt = 0u; attempt < (uint8_t)SENSOR_RF_TX_RETRY; attempt++) {\n        encode_frame10(mcu_uid, tempvalue, huminityvalue, send_data);   /* legacy: 每次重编码 */\n        if (app_um2005C_send_data_timeout(send_data, SENSOR_RF_FRAME_LEN,")
p_ms3 = os.path.join(NEG, "measure_reencode_each_retry.c")
open(p_ms3, "w", encoding="utf-8", newline="\n").write(ms3)
variants.append(("N3_measure_flow_vs_reencode_retry",
                 ["-std=c11", "-Wall", "-Wextra", "-DSENSOR_CONFIG_NO_MCU", "-DSENSOR_DEBUG_UART=1",
                  "-I" + os.path.join(TEST, "mock_measure_mcu"),
                  "-I" + INC, "-I" + COMMON,
                  "-I" + UM2005C,
                  os.path.join(TEST, "host_measure_flow_check.c"), p_ms3,
                  os.path.join(SRC, "sf_i2c.c"), os.path.join(SRC, "fw_core.c"), "-lm"],
                 "n3.exe", "must FAIL single-encode / frozen-frame"))

# ---------------------------------------------------------------- N4: unbounded UART waits
dt = read_norm(os.path.join(SRC, "debug_trace.c"))
dt4 = must_replace(dt, "    s_tx_guard = (uint32_t)DEBUG_UART_LINE_GUARD;",
                   "    s_tx_guard = 1000000u;   /* legacy: 实际无界 */")
dt4 = must_replace(dt4, "        uint32_t guard = (uint32_t)DEBUG_UART_CLOSE_GUARD;",
                   "        uint32_t guard = 1000000u;   /* legacy: 实际无界 */")
dt4 = must_replace(dt4, "        guard = (uint32_t)DEBUG_UART_CLOSE_GUARD;",
                   "        guard = 1000000u;   /* legacy: 实际无界 */")
p_dt4 = os.path.join(NEG, "debug_trace_unbounded.c")
open(p_dt4, "w", encoding="utf-8", newline="\n").write(dt4)
variants.append(("N4_debug_trace_vs_unbounded_wait",
                 ["-std=c11", "-Wall", "-Wextra", "-DSENSOR_DEBUG_UART=1",
                  "-I" + os.path.join(TEST, "mock_trace"),
                  "-I" + INC,
                  os.path.join(TEST, "host_debug_trace_check.c"), p_dt4],
                 "n4.exe", "must FAIL T14/T15 guard"))

# ---------------------------------------------------------------- N5: uncalibrated claims valid/dark
lt = read_norm(os.path.join(SRC, "light.c"))
lt5 = must_replace(lt, "    r.valid = r.adc_ok && (LIGHT_DARK_CALIBRATED != 0);",
                   "    r.valid = r.adc_ok;   /* legacy: 未标定也宣称有效 */")
p_lt5 = os.path.join(NEG, "light_uncalibrated_valid.c")
open(p_lt5, "w", encoding="utf-8", newline="\n").write(lt5)
variants.append(("N5_light_check_vs_uncalibrated_valid",
                 ["-std=c11", "-Wall", "-Wextra", "-I" + os.path.join(TEST, "mock_light"),
                  "-DLIGHT_BGR_TRIM_MV=1200u",
                  "-I" + INC, "-I" + COMMON,
                  os.path.join(TEST, "host_light_check.c"), p_lt5,
                  os.path.join(SRC, "fw_core.c")],
                 "n5.exe", "must FAIL uncalibrated valid/dark=false"))

print("== ITEM-007 negative controls: wrong implementation must be caught ==")
all_ok = True
report = []
for name, cmd, exe, expect in variants:
    state, rc, detail = compile_and_run(name, cmd, exe, None)
    caught = (state == "OK" and rc != 0)
    all_ok = all_ok and caught
    report.append((name, expect, state, rc, caught, detail))
    print("[%s] %s expect=%s compile/run=%s run_rc=%d caught=%s"
          % ("PASS" if caught else "FAIL", name, expect, state, rc, caught))
    if detail:
        for ln in detail.splitlines()[:6]:
            print("        " + ln)

with open(os.path.join(OUT, "negative_controls.out"), "w", encoding="utf-8") as f:
    f.write("== ITEM-007 negative controls (mutated copies; product tree untouched) ==\n")
    for name, expect, state, rc, caught, detail in report:
        f.write("\n[%s] %s\n  expect: %s\n  state: %s run_rc=%d caught=%s\n"
                % ("PASS" if caught else "FAIL", name, expect, state, rc, caught))
        for ln in detail.splitlines():
            f.write("  " + ln + "\n")
    f.write("\nRESULT: %s\n" % ("ALL_WRONG_IMPLEMENTATIONS_CAUGHT" if all_ok else "GAP_FOUND"))
print("\nRESULT: %s" % ("ALL_WRONG_IMPLEMENTATIONS_CAUGHT" if all_ok else "GAP_FOUND"))
sys.exit(0 if all_ok else 1)
