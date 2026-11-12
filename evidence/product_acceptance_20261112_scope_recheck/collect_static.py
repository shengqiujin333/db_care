#!/usr/bin/env python3
"""Read-only static requirement collector for the product acceptance recheck.

Reads only files under the shared project repository and prints the exact lines
that the acceptance report cites. No device access, no writes.
"""
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SENSOR = os.path.join(
    ROOT,
    "CW32L010_StandardPeripheralLib_V1.0.5",
    "Examples",
    "sensor",
    "gpio_input_output",
)
ANDROID = os.path.join(ROOT, "dengbei_care", "app", "src", "main", "java", "com", "jinyuni", "dengbei_care")


def show(path, pattern, label, limit=40):
    print("== %s ==" % label)
    print("file: %s" % os.path.relpath(path, ROOT))
    rx = re.compile(pattern)
    n = 0
    try:
        with open(path, encoding="utf-8", errors="replace") as f:
            for i, line in enumerate(f, 1):
                if rx.search(line):
                    print("%4d: %s" % (i, line.rstrip()))
                    n += 1
                    if n >= limit:
                        break
    except FileNotFoundError:
        print("MISSING")
    if n == 0:
        print("(no match)")
    print()


show(os.path.join(SENSOR, "USER", "inc", "sensor_config.h"),
     r"SENSOR_RTC_TICK_PERIOD_MIN|SENSOR_SAMPLE_TICKS|LIGHT_DARK_THRESHOLD_CODE|SENSOR_REPORT_RISE_X10|SENSOR_REPORT_HIGH_X10|DEBUG_UART_TX_GPIO_PIN|DEBUG_UART_RX_GPIO_PIN|DEBUG_UART_BaudRate",
     "sensor_config.h cadence/report/light/UART1 constants")

show(os.path.join(SENSOR, "USER", "src", "fw_core.c"),
     r"light_is_dark|threshold_code == 0u|mean_adc_code >=|cur_temp_x10 >|SENSOR_REPORT_RISE_X10|!dark",
     "fw_core.c criterion and report logic")

show(os.path.join(SENSOR, "USER", "src", "light.c"),
     r"r\.valid|r\.dark|LIGHT_DARK_THRESHOLD_CODE",
     "light.c valid/dark wiring")

show(os.path.join(SENSOR, "USER", "src", "debug_trace.c"),
     r"BOOT fw=|VDD ok=|IOTEST |IOSIG |BUS idle=|S k=",
     "debug_trace.c banner/line formats")

show(os.path.join(SENSOR, "USER", "src", "interrupts_cw32l010.c"),
     r"PB04|霍尔|EXTI",
     "interrupts_cw32l010.c hall retirement")

print("== hall/OPTCFG residue scan (USER/) ==")
hits = 0
for dirpath, _dirs, files in os.walk(os.path.join(SENSOR, "USER")):
    for name in files:
        if not name.endswith((".c", ".h")):
            continue
        p = os.path.join(dirpath, name)
        with open(p, encoding="utf-8", errors="replace") as f:
            for i, line in enumerate(f, 1):
                if re.search(r"hall|HALL|OPTCFG|optcfg|霍尔", line):
                    hits += 1
                    print("%s:%d: %s" % (os.path.relpath(p, ROOT), i, line.rstrip()))
print("residue_hits=%d (all are retirement comments if only *.c/*.h comments appear)" % hits)
print()

show(os.path.join(ANDROID, "cloud", "CloudConfig.kt"),
     r"SERVER_HOST|MQTT_BROKER_PORT|HTTP_PORT|MQTT_BROKER_URI|HTTP_BASE_URL",
     "Android CloudConfig.kt server constants")

show(os.path.join(ANDROID, "telemetry", "SensorCadence.kt"),
     r"180|3|0\.9|35",
     "Android SensorCadence.kt cadence/wording")

show(os.path.join(ROOT, "docs", "README.md"),
     r"rev|版本|DOC-PKG|验收",
     "docs/README.md version header", limit=12)
