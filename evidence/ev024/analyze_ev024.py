#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""EV-024 independent capture analysis (embedded_tester).

Parses raw COM43 byte streams captured after downloading the D2 build
(axf md5 e56d8d4b48c14ff29ae376781bbdc915).  Does NOT import or read any
firmware constant; CRC-8 and x10 conversions are re-implemented here from
the GXHT40 datasheet description (poly 0x31, init 0xFF, MSB-first).

Checks:
  * line framing/length (<= 96 B including CRLF) and no stray bytes
  * S line: 15 fields, frozen key order, value domains, q=1 => no G,
    q=0 => exactly one following G
  * k cadence (delta 3 per sampling cycle)
  * G raw: independent CRC-8 recomputation, humidity CRC, byte-3 signature
  * failure rate vs the previous build baseline + Fisher exact test
"""

import math
import os
import sys
import glob

BASE = os.path.dirname(os.path.abspath(__file__))

S_KEYS = ["k", "p", "H", "V", "o", "n", "x", "a", "D", "t", "h", "q", "r", "s", "y"]
G_KEYS = ["s", "a44", "a45", "rd", "at", "raw"]


def crc8_gxht40(data: bytes) -> int:
    """CRC-8, poly 0x31, init 0xFF, no reflection, no final xor."""
    crc = 0xFF
    for b in data:
        crc ^= b
        for _ in range(8):
            if crc & 0x80:
                crc = ((crc << 1) ^ 0x31) & 0xFF
            else:
                crc = (crc << 1) & 0xFF
    return crc


def temp_x10(raw: bytes) -> int:
    """GXHT40: T = -45 + 175 * S_T / 65535, reported in 0.1 C."""
    s = (raw[0] << 8) | raw[1]
    return round((-45.0 + 175.0 * s / 65535.0) * 10.0)


def hum_x10(raw: bytes) -> int:
    s = (raw[3] << 8) | raw[4]
    return round(100.0 * 10.0 * s / 65535.0)


def parse_kv(parts):
    out = {}
    for p in parts:
        if "=" not in p:
            return None
        k, v = p.split("=", 1)
        out[k] = v
    return out


def fisher_two_sided(a, b, c, d):
    """Two-sided Fisher exact test on [[a,b],[c,d]]."""
    n = a + b + c + d
    row1, col1 = a + b, a + c
    def prob(x):
        return (math.comb(row1, x) * math.comb(n - row1, col1 - x)) / math.comb(n, col1)
    p0 = prob(a)
    lo = max(0, col1 - (n - row1))
    hi = min(row1, col1)
    return sum(prob(x) for x in range(lo, hi + 1) if prob(x) <= p0 + 1e-12)


def main():
    files = sorted(glob.glob(os.path.join(BASE, "cap*.cap")))
    if not files:
        print("no capture files found")
        return 1

    cycles = []          # list of dicts, one per S line
    failures = []        # G records that follow a failed S
    problems = []
    total_bytes = 0
    total_line_bytes = 0
    windows = []

    for path in files:
        name = os.path.basename(path)
        data = open(path, "rb").read()
        total_bytes += len(data)
        # strict CRLF line framing
        if data and not data.endswith(b"\r\n"):
            problems.append(f"{name}: does not end with CRLF (truncated?)")
        raw_lines = data.split(b"\r\n")
        if raw_lines and raw_lines[-1] == b"":
            raw_lines = raw_lines[:-1]
        window_cycles = []
        i = 0
        while i < len(raw_lines):
            line = raw_lines[i]
            total_line_bytes += len(line) + 2
            if len(line) + 2 > 96:
                problems.append(f"{name}: line {i+1} is {len(line)+2} B (>96)")
            try:
                text = line.decode("ascii")
            except UnicodeDecodeError:
                problems.append(f"{name}: line {i+1} is not ASCII: {line[:40]!r}")
                i += 1
                continue
            if not text:
                problems.append(f"{name}: empty line")
                i += 1
                continue
            parts = text.split(" ")
            tag = parts[0]
            if tag == "S":
                kv = parse_kv(parts[1:])
                if kv is None or list(kv.keys()) != S_KEYS:
                    problems.append(f"{name}: bad S field set/order: {text}")
                    i += 1
                    continue
                rec = {"file": name, "line": i + 1, "raw": text}
                try:
                    for k in S_KEYS:
                        rec[k] = int(kv[k])
                except ValueError:
                    problems.append(f"{name}: non-integer S field: {text}")
                    i += 1
                    continue
                # domain checks
                for k, lo, hi in (("H", 0, 1), ("V", 0, 1), ("D", 0, 1),
                                  ("q", 0, 1), ("r", 0, 1), ("s", 0, 2), ("y", 0, 3)):
                    if not (lo <= rec[k] <= hi):
                        problems.append(f"{name}: S {k}={rec[k]} out of [{lo},{hi}]")
                if not (0 <= rec["h"] <= 1000):
                    problems.append(f"{name}: h out of range: {rec['h']}")
                if rec["q"] == 1 and not (-400 <= rec["t"] <= 1250):
                    problems.append(f"{name}: q=1 but t out of valid domain: {rec['t']}")
                cycles.append(rec)
                window_cycles.append(rec)
                # pairing: q=0 must be followed by G, q=1 must not
                nxt = raw_lines[i + 1].decode("ascii", "replace") if i + 1 < len(raw_lines) else ""
                follows_g = nxt.startswith("G ")
                if rec["q"] == 0:
                    if not follows_g:
                        problems.append(f"{name}: q=0 at k={rec['k']} not followed by G")
                else:
                    if follows_g:
                        problems.append(f"{name}: q=1 at k={rec['k']} followed by G")
                    if rec["s"] == 0 and rec["r"] == 0 and follows_g:
                        problems.append(f"{name}: unexpected G after clean cycle k={rec['k']}")
                i += 1
                continue
            if tag == "G":
                kv = parse_kv(parts[1:])
                if kv is None or list(kv.keys()) != G_KEYS:
                    problems.append(f"{name}: bad G field set/order: {text}")
                    i += 1
                    continue
                raw_hex = kv["raw"]
                rec = {"file": name, "line": i + 1, "raw": text,
                       "s": int(kv["s"]), "a44": int(kv["a44"]),
                       "a45": int(kv["a45"]), "rd": int(kv["rd"]),
                       "at": int(kv["at"]), "raw_hex": raw_hex}
                if raw_hex != "------------":
                    if len(raw_hex) != 12:
                        problems.append(f"{name}: G raw length {len(raw_hex)}: {text}")
                    else:
                        payload = bytes.fromhex(raw_hex)
                        rec["temp_crc_calc"] = crc8_gxht40(payload[0:2])
                        rec["hum_crc_calc"] = crc8_gxht40(payload[3:5])
                        rec["temp_crc_rx"] = payload[2]
                        rec["hum_crc_rx"] = payload[5]
                        rec["temp_crc_ok"] = payload[2] == rec["temp_crc_calc"]
                        rec["hum_crc_ok"] = payload[5] == rec["hum_crc_calc"]
                        rec["byte2_minus_byte0"] = payload[2] - payload[0]
                        rec["t_x10"] = temp_x10(payload)
                        rec["h_x10"] = hum_x10(payload)
                failures.append(rec)
                i += 1
                continue
            problems.append(f"{name}: unknown line tag {tag!r}: {text}")
            i += 1
        if window_cycles:
            windows.append((name, window_cycles[0]["k"], window_cycles[-1]["k"], len(window_cycles)))

    # cadence / continuity
    ks = [c["k"] for c in cycles]
    gaps = []
    for a, b in zip(ks, ks[1:]):
        if b - a != 3:
            gaps.append((a, b, b - a))
    # prev-value semantics
    prev_semantics = []
    last_valid_t = None
    for c in cycles:
        if c["q"] == 1:
            prev_semantics.append((c["k"], c["t"], c["p"], c["H"]))
            last_valid_t = c["t"]
        else:
            if c["p"] != last_valid_t:
                prev_semantics.append(("MISMATCH", c["k"], c["p"], last_valid_t))

    n_cycles = len(cycles)
    n_ok = sum(1 for c in cycles if c["q"] == 1)
    n_fail = n_cycles - n_ok

    print("=" * 78)
    print("EV-024 independent analysis - D2 build (axf e56d8d4b48c14ff29ae376781bbdc915)")
    print("=" * 78)
    print(f"capture files       : {len(files)}")
    print(f"total bytes         : {total_bytes}")
    print(f"sum of line bytes   : {total_line_bytes}")
    print(f"stray bytes         : {total_bytes - total_line_bytes}")
    print(f"S cycles            : {n_cycles}  (q=1: {n_ok}, q=0: {n_fail})")
    print(f"failure rate        : {100.0 * n_fail / n_cycles:.1f}%")
    print(f"G lines             : {len(failures)}")
    print()
    print("per-window cycles (k range):")
    for w in windows:
        print(f"  {w[0]}: k={w[1]}..{w[2]} ({w[3]} cycles)")
    print()
    if gaps:
        print("k delta anomalies (expected 3):")
        for g in gaps:
            print(f"  k={g[0]} -> {g[1]} delta={g[2]}")
    else:
        print("k delta: every consecutive observed cycle differs by +3  (OK)")
    print()

    if failures:
        print("G frame independent CRC-8 recomputation:")
        print("  k   raw          T word  T calc  T rx  T ok  byte2-byte0  H word  H rx  H ok")
        for f in failures:
            if "temp_crc_calc" in f:
                print(f"  {f.get('k','?')}  {f['raw_hex']}  -- 0x{f['temp_crc_calc']:02X}  0x{f['temp_crc_rx']:02X}  "
                      f"{f['temp_crc_ok']}  {f['byte2_minus_byte0']:+d}   -- 0x{f['hum_crc_rx']:02X}  {f['hum_crc_ok']}")
            else:
                print(f"  no raw: {f['raw']}")
    else:
        print("G frames: none (no CRC/read failures observed)")

    print()
    print("S line samples (first / last):")
    for c in (cycles[:1] + cycles[-1:]):
        print(f"  {c['file']}: {c['raw']}")
    print()
    print("prev-value chain (k, t, p, H) for successful cycles:")
    print("  " + ", ".join(f"({k}:{t}/{p},H={h})" for k, t, p, h in prev_semantics))
    print()

    # baseline comparison
    base_fail, base_n = 16, 25
    p = fisher_two_sided(n_fail, n_ok, base_fail, base_n - base_fail)
    print(f"baseline (rev 6.1 build 08a4c266): {base_fail}/{base_n} failed = 64.0%")
    print(f"this build (D2)                 : {n_fail}/{n_cycles} failed = {100.0*n_fail/n_cycles:.1f}%")
    print(f"Fisher exact two-sided p        : {p:.3g}")
    print()
    print(f"problems: {len(problems)}")
    for p_ in problems:
        print("  -", p_)
    return 0 if not problems else 1


if __name__ == "__main__":
    sys.exit(main())
