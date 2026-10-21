#!/usr/bin/env python3
"""Print line-numbered static references for ITEM-003 from the product measure.c."""
import sys

P = r"CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/src/measure.c"
PATS = ["encode_frame10", "app_um2005C_send_data_timeout", "s_pending_frame_valid",
        "report_req", "report_retry", "s_last_send_result", "SENSOR_RF_TX_RETRY",
        "SENSOR_RF_FRAME_LEN", "SENSOR_RF_TX_TIMEOUT_MS"]

with open(P, encoding="utf-8") as fh:
    lines = fh.read().splitlines()
print("FILE %s" % P)
for i, ln in enumerate(lines, 1):
    for pat in PATS:
        if pat in ln:
            print("%4d: %s" % (i, ln.strip()))
            break
