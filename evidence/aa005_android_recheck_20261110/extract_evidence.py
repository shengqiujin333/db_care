# -*- coding: utf-8 -*-
"""Extract device/scan/GATT evidence lines from raw logcat captures (test-side helper)."""
import os
import re

BASE = os.path.dirname(os.path.abspath(__file__))


def read_lines(name):
    path = os.path.join(BASE, name)
    with open(path, encoding="utf-8", errors="replace") as f:
        return f.readlines()


def extract(src_names, patterns, out_name, header):
    rows = []
    for name in src_names:
        for line in read_lines(name):
            if all(re.search(p, line) for p in patterns):
                rows.append(line.rstrip())
    # de-duplicate while preserving order (overlapping logcat snapshots)
    seen = set()
    unique = []
    for row in rows:
        if row not in seen:
            seen.add(row)
            unique.append(row)
    with open(os.path.join(BASE, out_name), "w", encoding="utf-8", newline="\n") as f:
        f.write(header + "\n")
        f.write("\n".join(unique) + "\n")
    return len(unique)


n1 = extract(
    ["logcat_full3.txt", "logcat_full4.txt"],
    [r"GatewayDiscovery|GatewayBleSettings"],
    "device_scan_evidence.txt",
    "# Real-device BLE discovery evidence (Redmi 8A / Android 10 / API 29)\n"
    "# raw source: logcat captures 2026-11-10 21:55Z (local 21:55)",
)

n2 = extract(
    ["logcat_full4.txt", "logcat_full5.txt", "logcat_full6.txt", "logcat_full7.txt"],
    [r"MqttService", r"21:5[4-9]:|22:0[0-2]:"],
    "device_gatt_evidence.txt",
    "# Real-device BLE/GATT + cadence evidence (Redmi 8A / Android 10 / API 29)\n"
    "# gateway 84:C2:E4:03:02:02 discovered via FFE0; two read cycles",
)

n3 = extract(
    ["logcat_full4.txt", "logcat_full5.txt", "logcat_full6.txt", "logcat_full7.txt"],
    [r"link_state=|BLE connect blocked|BLE target|Attempt [0-9]|MTU negotiated|cipher length|BLE frame rejected|Successfully connected|Saved MAC address|Failed to connect to broker|ENETUNREACH"],
    "device_link_state_evidence.txt",
    "# Link state / permission / network evidence",
)

print("scan_lines=%d gatt_lines=%d state_lines=%d" % (n1, n2, n3))
