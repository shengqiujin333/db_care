#!/usr/bin/env python3
"""Independent static checks for ITEM-006 (embedded_tester).

Scope: the real product sources under USER/ (read-only). No build, no device.
These checks complement (and never replace) the compiled harness and the real
target capture:
  * calibration gate present and expressed at compile time;
  * build identifier is the single configuration constant and BOOT uses it;
  * light path keeps 100 ms / 8 samples / integer mean and the valid/dark rules;
  * the product path passes light.valid/dark into the rise gate.

Exit code 0 = every check passed; 1 = at least one mismatch.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
FW = os.path.join(REPO, "CW32L010_StandardPeripheralLib_V1.0.5", "Examples",
                  "sensor", "gpio_input_output")
USER = os.path.join(FW, "USER")

FILES = {
    "sensor_config.h": os.path.join(USER, "inc", "sensor_config.h"),
    "debug_trace.c": os.path.join(USER, "src", "debug_trace.c"),
    "light.c": os.path.join(USER, "src", "light.c"),
    "fw_core.c": os.path.join(USER, "src", "fw_core.c"),
    "measure.c": os.path.join(USER, "src", "measure.c"),
}

pass_n = fail_n = 0


def check(cond, msg):
    global pass_n, fail_n
    if cond:
        pass_n += 1
        print("PASS  %s" % msg)
    else:
        fail_n += 1
        print("FAIL  %s" % msg)


def read(name):
    with open(FILES[name], "r", encoding="utf-8-sig") as f:
        return f.read()


def all_user_text():
    chunks = []
    for root, dirs, names in os.walk(USER):
        for n in names:
            if n.endswith((".c", ".h")):
                with open(os.path.join(root, n), "r", encoding="utf-8-sig",
                          errors="replace") as f:
                    chunks.append((n, f.read()))
    return chunks


def main():
    cfg = read("sensor_config.h")
    tr = read("debug_trace.c")
    light = read("light.c")
    core = read("fw_core.c")
    meas = read("measure.c")

    print("==== ITEM-006 independent static checks ====")
    print("root: %s" % USER)

    # --- calibration gate ---
    check("#if (LIGHT_DARK_CALIBRATED != 0) && (LIGHT_DARK_CALIBRATED != 1)" in cfg,
          "compile-time check rejects any CALIBRATED value other than 0/1")
    check('#error "LIGHT_DARK_CALIBRATED must be 0 or 1"' in cfg,
          "distinct #error text for an illegal LIGHT_DARK_CALIBRATED")
    check("#if (LIGHT_DARK_REF_CODE < 1u) || (LIGHT_DARK_REF_CODE > 4095u)" in cfg,
          "compile-time range check for LIGHT_DARK_REF_CODE 1..4095")
    check('#error "LIGHT_DARK_REF_CODE must be 1..4095 when LIGHT_DARK_CALIBRATED=1"' in cfg,
          "distinct #error text for an out-of-range calibrated reference")
    check(re.search(r"#if\s+LIGHT_DARK_CALIBRATED\s*\n\s*#if", cfg) is not None,
          "reference range check is nested under 'when calibrated'")
    check("#define LIGHT_DARK_CALIBRATED           0" in cfg,
          "delivery configuration remains uncalibrated (CALIBRATED=0)")
    check("#define LIGHT_DARK_REF_CODE             0u" in cfg,
          "delivery configuration keeps the 0 placeholder reference")
    check("LIGHT_SETTLE_MS                 100u" in cfg,
          "settle window unchanged (LIGHT_SETTLE_MS=100)")
    check("LIGHT_ADC_SAMPLES               8u" in cfg,
          "burst length unchanged (LIGHT_ADC_SAMPLES=8)")

    # --- build identifier ---
    check(re.search(r'#define\s+SENSOR_FW_BUILD_ID\s+"RUN10-FDS003"', cfg) is not None,
          "SENSOR_FW_BUILD_ID defined as RUN10-FDS003 in the single config point")
    defs = []
    for name, text in all_user_text():
        for m in re.finditer(r"#define\s+SENSOR_FW_BUILD_ID\b", text):
            defs.append((name, text[:m.start()].count("\n") + 1))
    check(len(defs) == 1 and defs[0][0] == "sensor_config.h",
          "SENSOR_FW_BUILD_ID has exactly one definition (sensor_config.h): %s" % defs)
    hard = [(n, i + 1) for n, text in all_user_text()
            for i, line in enumerate(text.splitlines()) if "FD-002r4" in line]
    check(not hard, "no FD-002r4 remains in any USER source/header: %s" % hard)
    check(re.search(r'trace_puts\("BOOT fw="\);\s*\n\s*trace_puts\(SENSOR_FW_BUILD_ID\);', tr)
          is not None,
          "debug_trace_boot renders 'BOOT fw=' + SENSOR_FW_BUILD_ID")
    check(re.search(r'trace_puts\("BOOT fw=[^"]+"\)', tr) is None,
          "no literal firmware id is inlined in the banner")

    # --- light path semantics ---
    check(re.search(r"r\.valid\s*=\s*r\.adc_ok\s*&&\s*\(LIGHT_DARK_CALIBRATED\s*!=\s*0\);", light)
          is not None,
          "valid = adc_ok && LIGHT_DARK_CALIBRATED (uncalibrated -> false)")
    check(re.search(r"r\.dark\s*=\s*light_is_dark\(r\.mean_adc_code,\s*r\.valid,\s*"
                    r"\(uint16_t\)LIGHT_DARK_REF_CODE\);", light) is not None,
          "dark comes from light_is_dark(mean, valid, LIGHT_DARK_REF_CODE)")
    check(re.search(r"r\.mean_adc_code\s*=\s*\(uint16_t\)\(sum\s*/\s*\(uint32_t\)ok\);", light)
          is not None,
          "mean is an integer division of the successful-sample sum")
    check(re.search(r"\b(float|double)\b|%f", light) is None,
          "no floating point in the light path")
    check("LIGHT_ADC_FULL_SCALE" not in light and "full scale" not in light.lower(),
          "no full-scale fallback constant/path in light.c")
    check(re.search(r"if\s*\(r\.adc_ok\)\s*\{[^}]*mean_adc_code", light, re.S) is not None,
          "mean/min/max are only assigned when adc_ok (failure keeps 0)")

    # --- pure decision / product wiring ---
    check(re.search(r"if\s*\(!valid\)\s*\{\s*return\s+false;", core) is not None,
          "light_is_dark returns false when valid=false")
    check("((uint32_t)3u * (uint32_t)mean_adc_code) >= (uint32_t)c_dark" in core,
          "light_is_dark uses uint32 3*mean >= C (equality = dark)")
    check(re.search(r"sensor_gate_decide\(true,\s*prev_for_decision,\s*"
                    r"have_prev_for_decision,\s*tempvalue,\s*s_light\.valid,\s*"
                    r"s_light\.dark\)", meas) is not None,
          "product path passes light.valid and light.dark into sensor_gate_decide")

    print("result: %d passed, %d failed" % (pass_n, fail_n))
    return 0 if fail_n == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
