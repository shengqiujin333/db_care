#!/usr/bin/env python3
"""Independent compile-time gate check for ITEM-006 (embedded_tester).

Compiles the REAL USER/inc/sensor_config.h (SENSOR_CONFIG_NO_MCU branch) with
different -D overrides and checks:
  * illegal combinations MUST fail to compile, with the specific #error text;
  * legal combinations MUST compile.
This does not use the implementation's own check_calib_config.sh.

Exit code 0 = all cases behaved as required; 1 = at least one mismatch.
"""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
FW = os.path.join(REPO, "CW32L010_StandardPeripheralLib_V1.0.5", "Examples",
                  "sensor", "gpio_input_output")
INC = os.path.join(FW, "USER", "inc")

TU = '#include "sensor_config.h"\nint item006_gate_tu;\n'

ERR_CAL = b"LIGHT_DARK_CALIBRATED must be 0 or 1"
ERR_REF = b"LIGHT_DARK_REF_CODE must be 1..4095 when LIGHT_DARK_CALIBRATED=1"

# (label, [(macro, value), ...], expect_pass, expected_error)
CASES = [
    ("delivery default CAL=0 REF=0", [], True, None),
    ("CAL=0 REF=0 explicit", [("LIGHT_DARK_CALIBRATED", "0"), ("LIGHT_DARK_REF_CODE", "0")], True, None),
    ("CAL=0 REF=4096 (gate only applies when calibrated)", [
        ("LIGHT_DARK_CALIBRATED", "0"), ("LIGHT_DARK_REF_CODE", "4096")], True, None),
    ("CAL=1 REF=1 lower bound", [("LIGHT_DARK_CALIBRATED", "1"), ("LIGHT_DARK_REF_CODE", "1")], True, None),
    ("CAL=1 REF=4095 upper bound", [
        ("LIGHT_DARK_CALIBRATED", "1"), ("LIGHT_DARK_REF_CODE", "4095")], True, None),
    ("CAL=1 REF=3000 representative", [
        ("LIGHT_DARK_CALIBRATED", "1"), ("LIGHT_DARK_REF_CODE", "3000")], True, None),
    ("CAL=2 rejected", [("LIGHT_DARK_CALIBRATED", "2")], False, ERR_CAL),
    ("CAL=-1 rejected", [("LIGHT_DARK_CALIBRATED", "-1")], False, ERR_CAL),
    ("CAL=1 REF=0 rejected", [("LIGHT_DARK_CALIBRATED", "1"), ("LIGHT_DARK_REF_CODE", "0")], False, ERR_REF),
    ("CAL=1 REF=4096 rejected", [
        ("LIGHT_DARK_CALIBRATED", "1"), ("LIGHT_DARK_REF_CODE", "4096")], False, ERR_REF),
    ("CAL=1 REF=-1 rejected", [
        ("LIGHT_DARK_CALIBRATED", "1"), ("LIGHT_DARK_REF_CODE", "-1")], False, ERR_REF),
]


def run_case(defs):
    cmd = ["gcc", "-std=c11", "-Wall", "-Wextra", "-fsyntax-only",
           "-DSENSOR_CONFIG_NO_MCU", "-I", INC, "-x", "c", "-"]
    for name, value in defs:
        cmd.append("-D%s=%s" % (name, value))
    return subprocess.run(cmd, input=TU.encode("ascii"), stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, cwd=FW, timeout=60)


def main():
    print("==== ITEM-006 independent compile-time gate check ====")
    print("header: %s" % os.path.join(INC, "sensor_config.h"))
    print("gcc: %s" % subprocess.run(["gcc", "--version"], stdout=subprocess.PIPE,
                                     stderr=subprocess.STDOUT, timeout=30)
          .stdout.decode("utf-8", "replace").splitlines()[0])
    passed = failed = 0
    for label, defs, expect_pass, expected_err in CASES:
        proc = run_case(defs)
        ok = (proc.returncode == 0) if expect_pass else (proc.returncode != 0)
        err_text = proc.stderr.decode("utf-8", "replace")
        if not expect_pass and ok and expected_err is not None:
            ok = expected_err.decode("ascii") in err_text
        status = "PASS" if ok else "FAIL"
        if ok:
            passed += 1
        else:
            failed += 1
        print("%s  %-52s exit=%d expected=%s" %
              (status, label, proc.returncode,
               "compile" if expect_pass else "reject"))
        if not ok or not expect_pass:
            first = [ln for ln in err_text.splitlines() if "error" in ln.lower()]
            print("      stderr: %s" % (first[0] if first else "<none>"))
    print("result: %d passed, %d failed" % (passed, failed))
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
