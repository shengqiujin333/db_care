#!/usr/bin/env python3
"""Scan the P0 platform-stage logcat window for TLS / MQTT / BLE markers."""
import re
import sys

path = sys.argv[1] if len(sys.argv) > 1 else "22_logcat_window.txt"
lines = open(path, "r", encoding="utf-8", errors="replace").read().splitlines()

patterns = [
    ("tls[SSLHandshakeException]", r"SSLHandshakeException"),
    ("tls[No subjectAltNames on the certificate match]", r"No subjectAltNames on the certificate match"),
    ("tls[Trust anchor for certification path not found]", r"Trust anchor for certification path not found"),
    ("tls[PKIX path building failed]", r"PKIX path building failed"),
    ("tls[CertPathValidatorException]", r"CertPathValidatorException"),
    ("tls[SSLPeerUnverifiedException]", r"SSLPeerUnverifiedException"),
    ("broker[Attempting to reconnect to broker]", r"Attempting to reconnect to broker"),
    ("broker[Connected to broker]", r"Connected to broker"),
    ("broker[Failed to connect to broker]", r"Failed to connect to broker"),
    ("broker[not_authorized_5]", r"无权连接 \(5\)"),
    ("ble[link_state=DATA_OK]", r"link_state=DATA_OK"),
    ("ble[link_state=CONNECT_FAILED]", r"link_state=CONNECT_FAILED"),
    ("ble[MTU negotiated]", r"MTU negotiated: mtu="),
    ("ble[GwID=]", r"GwID="),
    ("ble[writeCharacteristic bytes 6]", r"writeCharacteristic\(legacy\) ok=true, bytes=6"),
    ("ble[id=uid4]", r"id=6A002C00"),
    ("ble[Successfully connected and read data]", r"Successfully connected and read data"),
    ("upload[CloudUpload uploaded]", r"CloudUpload: uploaded"),
]

print("file=%s" % path)
for label, pat in patterns:
    n = sum(1 for ln in lines if re.search(pat, ln))
    print("%s=%d" % (label, n))

print("\n--- matched lines (chronological, deduplicated) ---")
seen = set()
for ln in lines:
    if any(re.search(p, ln) for _, p in patterns):
        if ln in seen:
            continue
        seen.add(ln)
        print(ln)
