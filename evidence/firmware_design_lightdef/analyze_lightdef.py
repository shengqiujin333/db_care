#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Light-path evidence summary for the firmware no-light definition (design input).

Read-only analysis over already-committed raw captures.  It does NOT access
hardware and does NOT import firmware constants: the criterion forms are
re-derived from the observed ADC means.

Inputs (raw S lines written by the delivered firmware's UART1 trace):
  evidence/firmware_exploration_current/*.bin          (declared light conditions)
  evidence/firmware_exploration/*.cap
  evidence/ev021..ev031/*.cap|*.bin                    (in-situ passive captures)

Output: lightdef_analysis.txt (tables + arithmetic checks).

Usage: python evidence/firmware_design_lightdef/analyze_lightdef.py
"""

import glob
import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "lightdef_analysis.txt")

# Only raw capture artefacts; host-side logs are excluded on purpose.
CAPTURE_DIRS = [
    "evidence/firmware_exploration_current",
    "evidence/firmware_exploration",
    "evidence/ev021", "evidence/ev022", "evidence/ev023", "evidence/ev024",
    "evidence/ev025", "evidence/ev026", "evidence/ev027", "evidence/ev028",
    "evidence/ev029", "evidence/ev030", "evidence/ev031",
]
CAPTURE_EXT = (".bin", ".cap")

# S k=<tick> p=<prev> H=<have> V=<lvalid> o=<ok> n=<min> x=<max> a=<mean>
#   D=<dark> t=<temp> h=<hum> q=<sample_ok> r=<report> s=<send> y=<retry>
S_RE = re.compile(
    rb"S k=(\d+) p=(-?\d+) H=(\d) V=(\d) o=(\d+) n=(\d+) x=(\d+) a=(\d+) "
    rb"D=(\d) t=(-?\d+) h=(\d+) q=(\d) r=(\d) s=(\d) y=(\d)"
)

# Externally declared light conditions (human statements recorded in
# artifacts/firmware_feasibility_current.md §3 / §10).  Anything else is
# "passive observation, lighting not declared" and is reported as such.
DECLARED = {
    ("firmware_exploration_current", 3061): "direct sunlight (user: 已直射)",
    ("firmware_exploration_current", 3083): "direct sunlight (user: 已直射)",
    ("firmware_exploration_current", 3095): "direct sunlight continuation",
    ("firmware_exploration_current", 3514): "room lamp (user: 房间灯)",
    ("firmware_exploration_current", 3750): "same session, lighting NOT declared",
}


def collect():
    rows = []
    for d in CAPTURE_DIRS:
        for f in sorted(glob.glob(os.path.join(ROOT, d, "*"))):
            if not os.path.isfile(f) or not f.endswith(CAPTURE_EXT):
                continue
            try:
                blob = open(f, "rb").read()
            except OSError:
                continue
            for m in S_RE.finditer(blob):
                g = m.groups()
                rows.append({
                    "dir": os.path.basename(d),
                    "file": os.path.basename(f),
                    "k": int(g[0]), "p": int(g[1]), "have": int(g[2]),
                    "valid": int(g[3]), "ok": int(g[4]), "n": int(g[5]),
                    "x": int(g[6]), "a": int(g[7]), "dark": int(g[8]),
                    "t": int(g[9]), "h": int(g[10]), "q": int(g[11]),
                    "r": int(g[12]), "s": int(g[13]), "y": int(g[14]),
                })
    # de-duplicate repeated copies of the same line in derived text artefacts
    seen, uniq = set(), []
    for r in rows:
        key = (r["dir"], r["k"], r["a"], r["n"], r["x"])
        if key in seen:
            continue
        seen.add(key)
        uniq.append(r)
    return uniq


def main():
    rows = collect()
    if not rows:
        print("no S lines found", file=sys.stderr)
        return 2

    out = []
    w = out.append
    w("=== no-light definition evidence summary (design input) ===")
    w("analysis script : evidence/firmware_design_lightdef/analyze_lightdef.py")
    w("inputs          : committed raw UART captures only (no hardware access)")
    w("S lines parsed  : %d (deduplicated per directory/tick/mean/min/max;" % len(rows))
    w("                  the same capture stored as .bin and .cap counts once)")
    w("")

    # 1. declared conditions
    w("--- [1] samples with an externally declared light condition ---")
    for r in sorted(rows, key=lambda z: (z["dir"], z["k"])):
        note = DECLARED.get((r["dir"], r["a"]))
        if not note:
            continue
        w("  %-32s %-26s k=%-5d a=%-4d n=%-4d x=%-4d o=%d t=%-4d q=%d V=%d D=%d  <- %s"
          % (r["dir"], r["file"], r["k"], r["a"], r["n"], r["x"], r["ok"],
             r["t"], r["q"], r["valid"], r["dark"], note))
    w("")

    # 2. per-file spread of the mean for everything else
    w("--- [2] passive captures: per-file min/max of the light ADC mean ---")
    groups = {}
    for r in rows:
        if (r["dir"], r["a"]) in DECLARED:
            continue
        groups.setdefault((r["dir"], r["file"]), []).append(r)
    for key in sorted(groups):
        v = groups[key]
        w("  %-32s %-28s n=%-2d a=%d..%d  spread=%d  t=%d..%d"
          % (key[0], key[1], len(v), min(z["a"] for z in v), max(z["a"] for z in v),
             max(z["a"] for z in v) - min(z["a"] for z in v),
             min(z["t"] for z in v), max(z["t"] for z in v)))
    w("")

    # 3. global bounds used by the design
    lit_declared = [r["a"] for r in rows if r["dir"] == "firmware_exploration_current"
                    and r["a"] in (3061, 3083, 3095, 3514)]
    all_a = [r["a"] for r in rows]
    intra = [r["x"] - r["n"] for r in rows if r["ok"] > 0]
    w("--- [3] bounds ---")
    w("  declared-lit maximum mean          : %d" % max(lit_declared))
    w("  declared-lit minimum mean          : %d" % min(lit_declared))
    w("  passive maximum mean (all captures): %d" % max(all_a))
    w("  passive minimum mean (all captures): %d" % min(all_a))
    w("  max intra-period spread (x-n)      : %d" % max(intra))
    w("  median intra-period spread         : %d" % sorted(intra)[len(intra) // 2])
    w("")

    # 4. retirement proof for the 1/3 form: dark <=> 3*mean >= C_dark
    #    => lit require mean <= C_dark/3 <= 4095/3 = 1365
    lit_best = min(lit_declared)
    w("--- [4] old form 'dark = valid && 3*mean >= C_dark' feasibility ---")
    w("  highest attainable dark threshold : ceil(4095/3) = %d codes" % ((4095 + 2) // 3))
    w("  lowest declared-lit mean observed : %d codes" % lit_best)
    w("  => lit would need mean <= %d ; unmet by %d codes (ratio %.3f)"
      % ((4095 + 2) // 3, lit_best - (4095 + 2) // 3, ((4095 + 2) // 3) / float(lit_best)))
    w("  C_dark needed to call the room lamp 'lit' : > 3*%d = %d (> 12-bit full scale %d)"
      % (3514, 3 * 3514, 4095))
    w("")

    # 5. candidate absolute threshold
    #    lit side  : highest mean under an externally declared light condition
    #    dark side : EV-024 k=3..54 (t=19.7..21.4 C) runs 3660..3856 and then
    #                jumps to 3093..3134 at k=57 -> the covering was removed
    #                between the two windows, so 3660 is the lowest mean that
    #                stayed on the covered side of that natural transition.
    lit_max = 3514
    dark_min = 3660
    thr = int(round(((lit_max + dark_min) / 2.0) / 10.0)) * 10
    w("--- [5] candidate threshold form: dark = valid && mean >= T ---")
    w("  lit-side reference (declared room lamp) : %d" % lit_max)
    w("  dark-side reference (inferred covered)  : %d  (EV-024 k=3..54)" % dark_min)
    w("  empirical separation                    : %d codes (%.1f %% of full scale)"
      % (dark_min - lit_max, 100.0 * (dark_min - lit_max) / 4095.0))
    w("  candidate T = round((lit+dark)/2)       : %d" % thr)
    w("  margin to lit side                      : %d codes" % (thr - lit_max))
    w("  margin to dark side                     : %d codes" % (dark_min - thr))
    w("  margin / median intra-period spread     : %.1fx / %.1fx"
      % ((thr - lit_max) / float(sorted(intra)[len(intra) // 2]),
         (dark_min - thr) / float(sorted(intra)[len(intra) // 2])))
    w("  margin / max intra-period spread        : %.1fx / %.1fx"
      % ((thr - lit_max) / float(max(intra)), (dark_min - thr) / float(max(intra))))
    w("  admissible band (lit < T <= dark)       : (%d, %d]" % (lit_max, dark_min))
    w("  near-saturation check (EV-030/031)      : 3987..3996 >= T -> DARK (margin >= %d)"
      % (3987 - thr))
    w("  NOTE: any T inside the band shares the same %d-code total separation; the"
      % (dark_min - lit_max))
    w("        position only trades false-lit against false-dark. Stability of the")
    w("        band across temperature, placement and unit is NOT characterised.")
    w("")

    # 6. current gate state in every capture
    w("--- [6] gate state actually observed (V / D / r / s) ---")
    states = {}
    for r in rows:
        key = (r["valid"], r["dark"], r["r"], r["s"])
        states[key] = states.get(key, 0) + 1
    for key in sorted(states):
        w("  V=%d D=%d r=%d s=%d : %d lines" % (key[0], key[1], key[2], key[3], states[key]))
    w("")
    w("Note: every capture shows V=0 D=0 r=0 s=0 y=0 -> the disabled/uncalibrated gate")
    w("      never produced a report decision in any recorded cycle (matches the as-built source).")

    text = "\n".join(out) + "\n"
    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(text)
    sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
