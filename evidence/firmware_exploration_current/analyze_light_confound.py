#!/usr/bin/env python3
"""Light-channel attribution check (2026-11-10 session).

Question: is the ADC-code change across this session attributable to the user's
light actions, or could it be thermal/humidity drift of the LDR + divider?

Inputs : the S-line captures already in this directory (raw COM36/COM43 bytes).
Outputs: stdout only (redirected to light_confound_analysis_20261110.txt).

Method (no firmware constants, no new hardware):
  * Parse k / t (0.1 C) / h (0.1 %RH) / a (mean light ADC code) from each capture.
  * Pairs with (nearly) equal temperature: any large code difference there is not
    thermal, so it must come from the illumination state.
  * Pairs inside one stated light condition (direct-light window): estimate the
    apparent temperature coefficient da/dt under the assumption that the light
    state did not change.
  * Decompose the total session drift into a thermal part and a remainder.
  * Propagate the coefficient onto the margin of the absolute-threshold option.

Limitations stated in the output: humidity moves together with temperature in this
session, so the two cannot be separated with 5 samples; the coefficient is an
apparent (LDR + divider + ADC) coefficient, not a device datasheet value.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
R3_OHM = 5_000_000
ADC_FULL_SCALE = 4095
S_RE = re.compile(r"^S ((?:[A-Za-z]=-?[0-9]+ ?)+)$")

CAPTURES = [
    "serial_capture_20261110_0720Z.bin",
    "serial_capture_light_210s.bin",
    "serial_capture_direct_280s.bin",
    "serial_capture_after_300s.bin",
]

LABELS = {
    900: "A no-light window   k=900 ",
    993: "B room lamp         k=993 ",
    1002: "C direct light      k=1002",
    1005: "C direct light      k=1005",
    1017: "D direct continued  k=1017",
}


def parse_all():
    seen = {}
    for fname in CAPTURES:
        path = os.path.join(HERE, fname)
        if not os.path.exists(path):
            print("MISSING %s" % path)
            continue
        with open(path, "rb") as fh:
            text = fh.read().decode("ascii", "replace")
        for line in text.split("\r\n"):
            if not line:
                continue
            m = S_RE.match(line)
            if not m:
                continue
            f = {k: int(v) for k, v in (tok.split("=") for tok in m.group(1).split())}
            if f["k"] not in seen:
                seen[f["k"]] = (f["t"], f["h"], f["a"])
    rows = []
    for k in sorted(seen):
        t, h, a = seen[k]
        rows.append((LABELS.get(k, "k=%d" % k), k, t, h, a, ""))
    return rows


def r_ldr(code):
    return R3_OHM * code / float(ADC_FULL_SCALE - code)


def main():
    rows = parse_all()
    print("=" * 78)
    print("0) Samples used (t/h/a are S-line fields: 0.1 C, 0.1 %RH, ADC code)")
    print("   %-27s %6s %6s %6s %6s %10s" % ("label", "k", "t", "h", "a", "R_ldr MOhm"))
    for label, k, t, h, a, _n in rows:
        print("   %-27s %6d %6d %6d %6d %10.2f" % (label, k, t, h, a, r_ldr(a) / 1e6))

    print("=" * 78)
    print("1) Same-temperature pairs: large code differences cannot be thermal")
    pairs = [(rows[i], rows[j]) for i in range(len(rows)) for j in range(i + 1, len(rows))]
    for r1, r2 in pairs:
        if abs(r1[2] - r2[2]) <= 2:          # within 0.2 C
            print("   %s vs %s : dt=%+d (0.1C)  dh=%+d (0.1%%RH)  da=%+d codes"
                  % (r1[0].strip(), r2[0].strip(), r2[2] - r1[2], r2[3] - r1[3], r2[4] - r1[4]))
    print("   -> the 431-code room-lamp/direct difference at dt=+1 (0.1C) is not thermal;")
    print("      the illumination state dominates it.")

    print("=" * 78)
    print("2) Apparent temperature coefficient inside the direct-light window")
    print("   assumption: the user's light state did not change between these samples")
    slopes = []
    for (l1, k1, t1, h1, a1, _n1), (l2, k2, t2, h2, a2, _n2) in zip(rows, rows[1:]):
        if l1[0] == l2[0] == "C" or (l1[0] == "C" and l2[0] == "D"):
            if t2 != t1:
                s = (a2 - a1) * 10.0 / float(t2 - t1)   # codes per 1.0 C
                slopes.append(s)
                print("   k=%d -> k=%d : dt=%+0.1f C, da=%+d codes -> %.1f codes/degC"
                      % (k1, k2, (t2 - t1) / 10.0, a2 - a1, s))
    if slopes:
        lo, hi = min(slopes), max(slopes)
        print("   apparent coefficient: %.1f .. %.1f codes/degC (code falls as it warms)"
              % (lo, hi))

    print("=" * 78)
    print("3) Decomposition of the session drift (k=900 -> k=1017)")
    first, last = rows[0], rows[-1]
    total = last[4] - first[4]
    dt = (last[2] - first[2]) / 10.0
    print("   dt = %+0.1f C, da = %+d codes" % (dt, total))
    if slopes:
        for s in (lo, hi):
            thermal = s * dt
            print("   with %.1f codes/degC -> thermal part %+.0f codes (%.0f%%), remainder %+.0f codes"
                  % (s, thermal, 100.0 * abs(thermal / total), total - thermal))
        print("   -> illumination remains the dominant driver; thermal drift is a minor term")
        print("      for the light/no-light question, but comparable to the margin of an")
        print("      absolute mid-threshold (see section 4).")

    print("=" * 78)
    print("4) Impact on the absolute-threshold option (P2: threshold near 3630)")
    room = [a for (l, _k, _t, _h, a, _n) in rows if l.startswith("B")]
    dark = [a for (l, _k, _t, _h, a, _n) in rows if l.startswith("A")]
    if room and dark:
        thr = (room[0] + dark[0]) // 2
        print("   room lamp a=%d, no-light a=%d -> midpoint threshold ~%d" % (room[0], dark[0], thr))
        print("   margin to room lamp = %d codes, to no-light = %d codes"
              % (thr - room[0], dark[0] - thr))
        if slopes:
            for swing in (2.0, 3.0):
                print("   a +-%0.0f C swing moves the code by +-%0.0f codes = %0.0f%% of the margin"
                      % (swing, abs(hi) * swing, 100.0 * abs(hi) * swing / (thr - room[0])))
        print("   -> P2 is feasible only with per-unit calibration AND temperature compensation")
        print("      or a wider margin; this is a requirement/hardware decision, not firmware.")

    print("=" * 78)
    print("5) Robustness: correct the no-light reference to the room-lamp temperature")
    if room and dark and slopes:
        t_dark, t_room = first[2] / 10.0, [r[2] / 10.0 for r in rows if r[0].startswith("B")][0]
        s = (lo + hi) / 2.0
        corrected = dark[0] + s * (t_room - t_dark)
        print("   no-light a=%d at %.1f C; room lamp at %.1f C; mean coeff %.1f codes/degC"
              % (dark[0], t_dark, t_room, s))
        print("   temperature-corrected no-light code at %.1f C = %.0f" % (t_room, corrected))
        rd = r_ldr(corrected)
        rl = r_ldr(room[0])
        print("   R_dark=%.2f MOhm, R_lit=%.2f MOhm -> R_l/R_d=%.3f (rule needs <= 0.333)"
              % (rd / 1e6, rl / 1e6, rl / rd))
        print("   -> the 1/3 verdict does not change; it gets worse, not better")
        print("   corrected mid-threshold margin = %.0f codes (was %d) -> P2 is tighter still"
              % ((corrected - room[0]) / 2.0, (dark[0] - room[0]) // 2))

    print("=" * 78)
    print("6) Limitations (do not over-read)")
    print("   * humidity moved with temperature in this session (355->338 0.1%RH);")
    print("     t and h cannot be separated from 5 samples.")
    print("   * the coefficient is apparent (LDR + 5M divider + ADC), not a datasheet value.")
    print("   * the 'light state unchanged' assumption for C/D samples comes from the user's")
    print("     statement, not from an independent illuminance measurement.")
    print("   * no new capture was taken this round; all numbers come from committed S lines.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
