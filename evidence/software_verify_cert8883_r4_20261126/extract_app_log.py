#!/usr/bin/env python3
"""Extract the app-relevant logcat window (read-only analysis)."""
import os
import re

OUT = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(OUT, "19_logcat_latest.txt")
DST = os.path.join(OUT, "24_logcat_app_window.txt")

pat = re.compile(
    r"CloudUpload|MqttService|MqtttService|BluetoothGatt|link_state|BLE batch|"
    r"CONNACK|无权连接|FATAL EXCEPTION|subjectAltNames|SSLHandshakeException|"
    r"PKIX|Trust anchor")
keep_time = re.compile(r"11-26 10:(3[6-9]|4[0-9]|5[0-9]):[0-9.]+ +12980")

out = []
for line in open(SRC, encoding="utf-8", errors="replace"):
    if keep_time.search(line) and pat.search(line):
        out.append(line.rstrip())
    elif "CloudUpload" in line or "FATAL EXCEPTION" in line:
        out.append(line.rstrip())
open(DST, "w", encoding="utf-8").write("\n".join(out) + "\n")
print("wrote=%s lines=%d src_bytes=%d"
      % (os.path.basename(DST), len(out), os.path.getsize(SRC)))
