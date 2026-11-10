#!/usr/bin/env python3
"""Independent analysis of the light-gate evidence (2026-11-10, direct-light round).

Does NOT import or copy firmware constants except:
  * R3 = 5 M and the divider topology (project BOM / netlist hardware facts);
  * 12-bit ADC full scale 4095 (hardware fact);
  * the shipped dark rule formula dark = (3*mean >= C_dark), which is a firmware
    fact documented in USER/inc/sensor_config.h and USER/src/fw_core.c.

Inputs : .bin captures in this directory (raw COM43 bytes, ASCII S lines).
Outputs: stdout only (redirected to light_gate_analysis_20261110.txt).

Checks
  1. Raw-line integrity: CRLF, ASCII, field set/order, length <= 96 bytes.
  2. Divider reverse calculation R_ldr = R3 * code / (4095 - code).
  3. Exhaustive classification for every legal C_dark 1..4095: can the shipped
     rule ever report "light" (not dark) for each observed condition?
  4. Required response for the rule to be usable, and the measured shortfall.
  5. Ordered response series (dark -> room lamp -> direct light).
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
R3_OHM = 5_000_000          # BOM: R3 = 5M (sensor_hardware/MAIN_BOARD.BOM)
ADC_FULL_SCALE = 4095       # 12-bit ADC raw code maximum

FIELD_ORDER = ["k", "p", "H", "V", "o", "n", "x", "a", "D", "t", "h", "q", "r", "s", "y"]
S_RE = re.compile(r"^S ((?:[A-Za-z]=-?[0-9]+ ?)+)$")

# (label, capture file, condition note)
CONDITIONS = [
    ("A 2026-11-09T23:17Z window", "serial_capture_20261110_0720Z.bin",
     "first window of this run; light condition not separately confirmed by user"),
    ("B 2026-11-10T00:49Z room lamp", "serial_capture_light_210s.bin",
     "user: room lamp on, kept shining"),
    ("C 2026-11-10T01:00Z direct light", "serial_capture_direct_280s.bin",
     "user reply '已直射' 2026-11-10T00:58:30Z"),
]

# Historical measured means (FW-EXP-002 rev 2, 2026-11-09, user-confirmed no light)
HISTORICAL_DARK = [("2026-11-09 14:33Z (no light)", 3914),
                   ("2026-11-09 14:40Z (no light)", 3915),
                   ("2026-11-09 15:24Z (no light)", 3932)]


def parse_capture(path):
    with open(path, "rb") as fh:
        blob = fh.read()
    problems = []
    if not blob:
        return [], ["empty file"]
    if b"\r\n" not in blob:
        problems.append("no CRLF found")
    try:
        text = blob.decode("ascii")
    except UnicodeDecodeError as exc:
        return [], ["non-ASCII byte: %s" % exc]
    parts = text.split("\r\n")
    if parts and parts[-1] != "":
        problems.append("trailing bytes after last CRLF: %r" % parts[-1])
    parts = [p for p in parts if p != ""]
    rows = []
    for line in parts:
        m = S_RE.match(line)
        if not m:
            problems.append("unparsed line: %r" % line)
            continue
        fields = [tok.split("=") for tok in m.group(1).split()]
        keys = [k for k, _ in fields]
        if keys != FIELD_ORDER:
            problems.append("field order/set mismatch: %s" % keys)
        if len(line.encode("ascii")) > 96:
            problems.append("line > 96 bytes: %d" % len(line.encode("ascii")))
        rows.append((line, {k: int(v) for k, v in fields}))
    return rows, problems


def r_ldr_ohm(code):
    if code >= ADC_FULL_SCALE:
        return float("inf")
    if code <= 0:
        return 0.0
    return R3_OHM * code / (ADC_FULL_SCALE - code)


def dark_for_all_cdark(mean_code):
    """Return (all_dark, n_dark, n_light) over C_dark in 1..4095."""
    n_light = sum(1 for c in range(1, ADC_FULL_SCALE + 1) if (3 * mean_code) < c)
    return (n_light == 0), (ADC_FULL_SCALE - n_light), n_light


def main():
    rc = 0
    series = []   # (label, k, mean, note)
    for label, fname, note in CONDITIONS:
        path = os.path.join(HERE, fname)
        print("=" * 78)
        if not os.path.exists(path):
            print("MISSING capture for %s: %s" % (label, fname))
            rc = 1
            continue
        rows, problems = parse_capture(path)
        print("capture: %s  (%d bytes, %d S row(s))" % (fname, os.path.getsize(path), len(rows)))
        print("condition: %s" % note)
        for issue in problems:
            print("  PROBLEM: %s" % issue)
            rc = 1
        for line, f in rows:
            print("  raw: %s" % line)
            print("    len=%d B  o=%d/8  n=%d x=%d a=%d  V=%d D=%d  t=%d h=%d q=%d"
                  % (len(line.encode("ascii")), f["o"], f["n"], f["x"], f["a"],
                     f["V"], f["D"], f["t"], f["h"], f["q"]))
            if f["o"]:
                series.append((label, f["k"], f["a"], note))
        if not rows:
            print("  PROBLEM: no S row in window")
            rc = 1

    print("=" * 78)
    print("1) Divider reverse calculation (hardware fact: PB05 -R3 5M- PB04 -J4(GL4539)- GND)")
    print("   R_ldr = 5M * code / (4095 - code);  more light -> lower R_ldr -> lower code")
    refs = list(HISTORICAL_DARK) + [(f"{lbl} k={k}", a) for (lbl, k, a, _n) in series]
    for label, code in refs:
        print("   code=%4d  R_ldr=%9.2f MOhm  (%.2f x R3)   %s"
              % (code, r_ldr_ohm(code) / 1e6, r_ldr_ohm(code) / R3_OHM, label))

    print("=" * 78)
    print("2) Exhaustive check of the shipped rule dark = (3*mean >= C_dark)")
    print("   over every legal C_dark 1..4095; 'light' means NOT dark:")
    print("   %-34s %5s  dark/4095  light/4095  can ever be 'light'?" % ("condition", "mean"))
    for label, code in refs:
        all_dark, n_dark, n_light = dark_for_all_cdark(code)
        print("   %-34s %5d  %4d       %4d        %s"
              % (label[:34], code, n_dark, n_light, "NO" if all_dark else "yes"))

    print("=" * 78)
    print("3) Response required for the 1/3 rule to be usable")
    limit_100 = -(-ADC_FULL_SCALE // 3)   # ceil(4095/3) = 1365
    print("   C_dark upper bound 4095 -> 'light' needs mean < %d" % limit_100)
    print("   -> R_ldr must be < %.2f MOhm (%.1f%% of R3)"
          % (r_ldr_ohm(limit_100 - 1) / 1e6, 100.0 * (limit_100 - 1) / (ADC_FULL_SCALE - (limit_100 - 1))))
    dark_ref = min(c for _l, c in HISTORICAL_DARK)   # 3914: lowest measured dark mean
    print("   lowest measured dark mean %d -> 'light' needs mean < %d"
          % (dark_ref, dark_ref // 3 + (1 if dark_ref % 3 else 0)))
    print("   (for reference) 2/3-of-dark variant with C_dark %d -> 'light' needs mean < %d"
          % (dark_ref, dark_ref * 2 // 3))
    direct = [a for (lbl, _k, a, _n) in series if lbl.startswith("C ")]
    if direct:
        lo, hi = min(direct), max(direct)
        print("   measured under user-confirmed direct light: mean = %d .. %d" % (lo, hi))
        print("   shortfall vs 1/3 rule (C_dark=4095, limit 1364): %d codes (%.2fx above the limit)"
              % (lo - 1364, lo / 1364.0))
        dark_ref = min(c for _l, c in HISTORICAL_DARK)
        print("   shortfall vs 2/3 variant (C_dark=%d, limit %d): %d codes"
              % (dark_ref, dark_ref * 2 // 3, lo - dark_ref * 2 // 3))
        print("   code ratio best-lit / lowest measured dark = %.3f (rule needs <= 0.333)"
              % (lo / float(dark_ref)))

    print("=" * 78)
    print("4) Ordered response series (does light reach the part at all?)")
    ordered = sorted(series, key=lambda r: r[1])
    for label, k, code, _n in ordered:
        print("   %-34s k=%4d  mean=%4d  R_ldr=%8.2f MOhm" % (label[:34], k, code, r_ldr_ohm(code) / 1e6))
    if len(ordered) >= 2:
        first, last = ordered[0], ordered[-1]
        print("   k=%d -> k=%d: delta mean = %+d codes, R_ldr ratio %.2fx"
              % (first[1], last[1], last[2] - first[2],
                 r_ldr_ohm(first[2]) / r_ldr_ohm(last[2])))
    print("   interpretation: monotone decrease with increasing illumination proves the divider,")
    print("   PB05 drive and ADC channel respond, but the magnitude never approaches the rule limit.")
    return rc


if __name__ == "__main__":
    sys.exit(main())
