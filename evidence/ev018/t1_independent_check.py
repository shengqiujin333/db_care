#!/usr/bin/env python3
"""Independent reference check for sensor_decide_report() (TD-002 T1-01/T1-02).

Reads the C harness output lines "prev cur dark have actual" and compares with
the frozen rule from readme/IC-002:
    report = (cur > 350) or (have_prev and dark and (cur - prev) > 9)
computed with unbounded integers (no int16 overflow).
"""
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
FW_ROOT = HERE.parents[1] / "CW32L010_StandardPeripheralLib_V1.0.5" / "Examples" / "sensor" / "gpio_input_output"


def ensure_exe():
    exe = HERE / "t1_check.exe"
    if exe.exists():
        return exe
    src = HERE / "t1_independent_check.c"
    cmd = [
        "gcc",
        "-I", str(FW_ROOT / "USER" / "inc"),
        "-o", str(exe),
        str(src),
        str(FW_ROOT / "USER" / "src" / "fw_core.c"),
    ]
    print("building:", " ".join(cmd))
    subprocess.run(cmd, check=True)
    return exe


def expected(prev, cur, dark, have):
    if cur > 350:
        return 1
    if have and dark:
        return 1 if (cur - prev) > 9 else 0
    return 0


def main():
    exe = ensure_exe()
    out = subprocess.run([str(exe)], capture_output=True, text=True, check=True).stdout
    total = 0
    bad = 0
    for line in out.splitlines():
        parts = line.split()
        if len(parts) != 5:
            continue
        prev, cur, dark, have, actual = map(int, parts)
        exp = expected(prev, cur, dark, have)
        total += 1
        if exp != actual:
            bad += 1
            if bad <= 20:
                print(f"MISMATCH prev={prev} cur={cur} dark={dark} have={have} "
                      f"actual={actual} expected={exp}")
    print(f"t1 vectors={total} mismatches={bad}")
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
