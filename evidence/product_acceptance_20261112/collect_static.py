#!/usr/bin/env python3
"""Collect bounded static identity/rule evidence for PRODUCT-ACCEPTANCE-002."""
import subprocess

REPO = r"D:\mypro\beiwo2\AIP"
SENSOR = REPO + r"\CW32L010_StandardPeripheralLib_V1.0.5\Examples\sensor\gpio_input_output"
ANDROID = REPO + r"\dengbei_care\app\src\main\java\com\jinyuni\dengbei_care"


def show(path, start, end):
    with open(path, encoding="utf-8") as fh:
        for i, line in enumerate(fh, 1):
            if start <= i <= end:
                print(f"{path}:{i}: {line.rstrip()}")


def git(*args):
    r = subprocess.run(["git", *args], cwd=REPO, capture_output=True, text=True)
    print("$ git " + " ".join(args))
    print(r.stdout.strip() or "(empty)")
    if r.stderr.strip():
        print("stderr:", r.stderr.strip())
    print()


print("=== sensor_config.h: timing / light / report rules ===")
show(SENSOR + r"\USER\inc\sensor_config.h", 20, 32)
show(SENSOR + r"\USER\inc\sensor_config.h", 105, 142)
print()
print("=== debug_trace.h: UART1 ownership / pins ===")
show(SENSOR + r"\USER\inc\debug_trace.h", 1, 20)
print()
print("=== Android CloudConfig.kt: platform endpoints ===")
show(ANDROID + r"\cloud\CloudConfig.kt", 10, 32)
print()
print("=== hall / OPTCFG / light-comm references in sensor USER (bounded) ===")
import os

hits = 0
for root, dirs, files in os.walk(SENSOR + r"\USER"):
    for name in files:
        if not name.endswith((".c", ".h")):
            continue
        p = os.path.join(root, name)
        try:
            with open(p, encoding="utf-8", errors="replace") as fh:
                for i, line in enumerate(fh, 1):
                    low = line.lower()
                    if "hall" in low or "optcfg" in low or "光通讯" in line or "霍尔" in line:
                        print(f"{p}:{i}: {line.strip()}")
                        hits += 1
        except OSError:
            pass
print(f"(matches={hits}; all shown lines are comments/retirement notes if no call sites appear)")
print()
print("=== git identity / product source drift ===")
git("rev-parse", "HEAD")
git("diff", "--stat", "82a7e74..HEAD", "--",
    "CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER",
    "CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK",
    "CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/EWARM")
git("diff", "--stat", "dacf888..HEAD", "--", "dengbei_care/app")
git("diff", "--stat", "92bfcb2..HEAD", "--", "docs", "dengbei_care/用户使用说明书.md")
