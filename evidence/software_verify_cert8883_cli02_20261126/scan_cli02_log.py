#!/usr/bin/env python3
"""CERT8883-CLI-02 bounded log scanner.

Reads a threadtime logcat text file and reports, for the declared window:
  - TLS/hostname/chain error signatures (must be 0)
  - Paho broker connect attempts and CONNACK outcomes
  - BLE link terminal states
  - CloudUpload activity
No device access; pure offline parsing of captured evidence.
"""
import argparse
import re
import sys

TLS_PATTERNS = [
    r"SSLHandshakeException",
    r"No subjectAltNames on the certificate match",
    r"Trust anchor for certification path not found",
    r"PKIX path building failed",
    r"CertPathValidatorException",
    r"SSLPeerUnverifiedException",
]
BROKER_PATTERNS = [
    r"Attempting to reconnect to broker",
    r"Connected to broker",
    r"Failed to connect to broker",
    r"MQTT connection exception",
    r"无权连接",
]
BLE_TERMINAL = [
    r"link_state=DATA_OK",
    r"link_state=CONNECT_FAILED",
    r"link_state=GATEWAY_NO_DATA",
    r"link_state=BINDING_CAPACITY_EXCEEDED",
    r"Max attempts reached",
    r"Successfully connected and read data",
    r"BLE batch:",
]
UPLOAD = [r"CloudUpload: uploaded", r"uploaded \d+ reading"]


def scan(path, tls_pats, broker_pats, ble_pats, upload_pats):
    counts = {p: 0 for p in tls_pats + broker_pats + ble_pats + upload_pats}
    hits = []
    rx = [(p, re.compile(p)) for p in counts]
    with open(path, "r", encoding="utf-8", errors="replace") as fh:
        for line in fh:
            for pat, cre in rx:
                if cre.search(line):
                    counts[pat] += 1
                    hits.append(line.rstrip("\n"))
    return counts, hits


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("logfile")
    ap.add_argument("--label", default="window")
    args = ap.parse_args()

    counts, hits = scan(args.logfile, TLS_PATTERNS, BROKER_PATTERNS, BLE_TERMINAL, UPLOAD)
    tls_total = sum(counts[p] for p in TLS_PATTERNS)
    print(f"# CERT8883-CLI-02 scanner label={args.label} file={args.logfile}")
    print(f"tls_error_signatures_total={tls_total}")
    for p in TLS_PATTERNS:
        print(f"  tls[{p}]={counts[p]}")
    for p in BROKER_PATTERNS:
        print(f"  broker[{p}]={counts[p]}")
    for p in BLE_TERMINAL:
        print(f"  ble[{p}]={counts[p]}")
    for p in UPLOAD:
        print(f"  upload[{p}]={counts[p]}")
    print("--- matched lines (deduplicated, chronological) ---")
    seen = set()
    for line in hits:
        if line in seen:
            continue
        seen.add(line)
        print(line)
    return 0


if __name__ == "__main__":
    sys.exit(main())
