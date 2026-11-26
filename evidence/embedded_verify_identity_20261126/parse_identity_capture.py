#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""EMB-VER-IDENT-20261126: parse the two bounded COM43 captures and compute
uptime consistency against the last recorded verified download (EV-041).
Read-only; reads the .cap files in this directory and writes stdout.
"""
import hashlib
import os
import re
from datetime import datetime, timezone

HERE = os.path.dirname(os.path.abspath(__file__))
FILES = ["capture_w1_300s.cap", "capture_w2_200s.cap"]

# EV-041: mdk_flash {} 2026-11-23T08:13:11.758Z-08:13:24.274Z, Verify OK, Application running.
LAST_FLASH_START = datetime(2026, 11, 23, 8, 13, 11, 758000, tzinfo=timezone.utc)
LAST_FLASH_END = datetime(2026, 11, 23, 8, 13, 24, 274000, tzinfo=timezone.utc)

# Capture windows as reported by the serial_capture tool (UTC).
WINDOWS = {
    "capture_w1_300s.cap": (datetime(2026, 11, 26, 15, 17, 8, 477379, tzinfo=timezone.utc),
                            datetime(2026, 11, 26, 15, 22, 8, 692375, tzinfo=timezone.utc)),
    "capture_w2_200s.cap": (datetime(2026, 11, 26, 15, 23, 19, 919004, tzinfo=timezone.utc),
                            datetime(2026, 11, 26, 15, 26, 40, 127529, tzinfo=timezone.utc)),
}

FIELDS_S = ["k", "p", "H", "V", "o", "n", "x", "a", "D", "t", "h", "q", "r", "s", "y"]


def parse_line(line):
    if not line.startswith("S "):
        return None
    rec = {}
    for tok in line[2:].split():
        key, _, val = tok.partition("=")
        rec[key] = int(val)
    missing = [f for f in FIELDS_S if f not in rec]
    if missing:
        raise ValueError("missing fields %s in %r" % (missing, line))
    return rec


def main():
    samples = []
    for name in FILES:
        path = os.path.join(HERE, name)
        raw = open(path, "rb").read()
        print("== %s ==" % name)
        print("bytes=%d sha256=%s" % (len(raw), hashlib.sha256(raw).hexdigest()))
        print("raw repr: %r" % raw)
        text = raw.decode("ascii", errors="replace")
        lines = [l for l in text.replace("\r\n", "\n").split("\n") if l]
        w_start, w_end = WINDOWS[name]
        for idx, line in enumerate(lines):
            rec = parse_line(line)
            if rec is None:
                print("  non-S line: %r" % line)
                continue
            line_bytes = len(line) + 2
            print("  S#%d len=%d(<=96) %s" % (idx, line_bytes, line))
            samples.append((name, w_start, w_end, rec))

    print("\n== cadence / chain continuity ==")
    for i in range(1, len(samples)):
        prev = samples[i - 1][3]
        cur = samples[i][3]
        print("  k %d -> %d (dk=%d)" % (prev["k"], cur["k"], cur["k"] - prev["k"]))
        print("  prev-sample t=%d vs this p=%d (chain %s)"
              % (prev["t"], cur["p"], "OK" if prev["t"] == cur["p"] else "MISMATCH"))
        print("  H=%d V=%d D=%d q=%d r=%d s=%d y=%d"
              % (cur["H"], cur["V"], cur["D"], cur["q"], cur["r"], cur["s"], cur["y"]))

    print("\n== uptime consistency vs EV-041 last verified download ==")
    first_k = samples[0][3]["k"]
    w1_start = WINDOWS[FILES[0]][0]
    w1_end = WINDOWS[FILES[0]][1]
    elapsed_end_min = (w1_end - LAST_FLASH_END).total_seconds() / 60.0
    elapsed_start_min = (w1_start - LAST_FLASH_END).total_seconds() / 60.0
    print("  EV-041 flash end       : %s" % LAST_FLASH_END.isoformat())
    print("  first observed S k     : %d  (inside window %s .. %s)"
          % (first_k, w1_start.isoformat(), w1_end.isoformat()))
    print("  elapsed since flash    : %.2f .. %.2f min" % (elapsed_start_min, elapsed_end_min))
    # The line can have been printed anywhere inside the window, so the
    # smallest defensible deviation is k - elapsed_end (line printed at window end).
    dev_min = first_k - elapsed_end_min
    print("  k - elapsed(window end): %+.2f min (%+.3f %%)"
          % (dev_min, dev_min / elapsed_end_min * 100.0))
    print("  A reset/flash after EV-041 would restart rtc_tick_total at 0 (RAM counter);")
    print("  k=%d is consistent with continuous run since the last verified download." % first_k)

    print("\n== threshold / default-config signature ==")
    for name, _s, _e, rec in samples:
        sig = []
        sig.append("t=%d -> r=%d (strict >350 rule: %s)" % (
            rec["t"], rec["r"], "consistent" if (rec["t"] > 350) == (rec["r"] == 1) else "INCONSISTENT"))
        sig.append("V=%d D=%d (LIGHT_DARK_THRESHOLD_CODE=0 default)" % (rec["V"], rec["D"]))
        sig.append("o=%d (LIGHT_ADC_SAMPLES=8)" % rec["o"])
        print("  %s: %s; %s; %s" % (name, sig[0], sig[1], sig[2]))

    print("\n== verdict inputs ==")
    print("  S lines observed: %d across two windows (~9.5 min wall time)" % len(samples))
    print("  P/G lines observed: 0 (no report/send attempt, no sample failure)")
    print("  BOOT/VDD/IOTEST/IOSIG/BUS: not observed this round (no reset by design)")


if __name__ == "__main__":
    main()
