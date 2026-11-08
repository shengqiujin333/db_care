#!/usr/bin/env python3
"""Independent reference for the T8 report-decision grid (EV-022).

Rule taken from the frozen requirement (readme point 4 / IC-002 rev 3.0), NOT
from the implementation source:

    report = (cur > 350)                       # strictly above 35.0 C
          or (have_prev and dark and (cur - prev) > 9)   # rise > 0.9 C in darkness

Integer arithmetic is unbounded here, so the C implementation's int32 widening
cannot hide an overflow. The FNV-1a hash below is the value the C harness must
reproduce; it is compared against the real sensor_decide_report() in
test/host_fw_core_verify_ev.c.
"""

PREVS = [-32768, -1000, -400, -1, 0, 9, 10, 100, 300, 350, 351, 1000, 32767]
CURS = [-32768, -1000, -400, 0, 100, 291, 300, 350, 351, 1000, 32767]

FNV_INIT = 2166136261


def fnv(h, b):
    return ((h ^ b) * 16777619) & 0xFFFFFFFF


def decide(prev, cur, have, dark):
    if cur > 350:
        return 1
    if have and dark and (cur - prev) > 9:
        return 1
    return 0


def main():
    h = FNV_INIT
    n = 0
    for prev in PREVS:
        for cur in CURS:
            for have in (0, 1):
                for dark in (0, 1):
                    h = fnv(h, decide(prev, cur, have, dark))
                    n += 1
    print("reference rule: report = (cur>350) || (have && dark && cur-prev>9)")
    print("vectors: %d" % n)
    print("REF_REPORT_HASH = 0x%08X" % h)

    # explicit boundary vectors (printed for the harness truth table)
    cases = [
        (300, 309, 1, 1, 0, "rise exactly 9 (0.9C) -> no report (strict)"),
        (300, 310, 1, 1, 1, "rise 10 (1.0C) + DARK -> report"),
        (300, 310, 1, 0, 0, "rise 10 but LIT -> no report"),
        (349, 350, 1, 1, 0, "34.9 -> 35.0 DARK -> no report"),
        (300, 291, 1, 1, 0, "cooling 0.9C -> no report"),
        (300, 290, 1, 1, 0, "cooling 1.0C -> no report (rising direction only)"),
        (-100, -120, 1, 1, 0, "negative cooling -> no report"),
        (32767, -32768, 1, 1, 0, "int16 extremes, downward -> no report (int32 math)"),
        (-32768, 32767, 1, 1, 1, "int16 extremes, upward over-temp -> report"),
        (1000, 350, 1, 1, 0, "exactly 35.0C, no rise -> no report"),
    ]
    for prev, cur, have, dark, exp, text in cases:
        got = decide(prev, cur, have, dark)
        print("  %-6d -> %-6d have=%d dark=%d  got=%d exp=%d  %s %s"
              % (prev, cur, have, dark, got, exp, "OK" if got == exp else "MISMATCH", text))


if __name__ == "__main__":
    main()
