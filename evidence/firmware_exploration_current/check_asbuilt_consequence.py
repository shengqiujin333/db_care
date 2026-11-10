#!/usr/bin/env python3
"""As-built consequence check over every S line in this evidence directory.

Claim under test (artifact §8 item 2 / §11.2): with LIGHT_DARK_CALIBRATED=0 the
temperature-rise branch can never fire, so no report decision and no 433
transmission should appear in any observed cycle.

Method: scan every *.bin capture kept in this directory, tally the S-line fields
V (light valid), D (dark), q (sample ok), r (report decision), s (send result),
y (retry count) and the temperature range. No hardware access, no firmware change.

Output: stdout (redirected to asbuilt_consequence_20261110.txt).
"""
import glob
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
S_RE = re.compile(r"^S ((?:[A-Za-z]=-?[0-9]+ ?)+)$")
KEYS = ["k", "V", "D", "q", "r", "s", "y", "t"]


def main():
    files = sorted(glob.glob(os.path.join(HERE, "*.bin")))
    if not files:
        print("no captures")
        return 1
    seen = {}
    print("=" * 78)
    print("Per-capture tally (deduplicated by k across files)")
    print("   %-42s %6s %3s %3s %3s %3s %3s %3s" %
          ("file", "k", "V", "D", "q", "r", "s", "y"))
    for path in files:
        with open(path, "rb") as fh:
            text = fh.read().decode("ascii", "replace")
        for line in text.split("\r\n"):
            if not line:
                continue
            m = S_RE.match(line)
            if not m:
                print("   unparsed line in %s: %r" % (os.path.basename(path), line))
                continue
            f = {k: int(v) for k, v in (tok.split("=") for tok in m.group(1).split())}
            if f["k"] in seen:
                continue
            seen[f["k"]] = f
            print("   %-42s %6d %3d %3d %3d %3d %3d %3d"
                  % (os.path.basename(path), f["k"], f["V"], f["D"], f["q"], f["r"], f["s"], f["y"]))

    rows = [seen[k] for k in sorted(seen)]
    print("=" * 78)
    print("Summary over %d distinct cycles (k=%d..%d)" % (len(rows), rows[0]["k"], rows[-1]["k"]))
    print("   V (light valid) : %s" % sorted(set(r["V"] for r in rows)))
    print("   D (dark)        : %s" % sorted(set(r["D"] for r in rows)))
    print("   q (sample ok)   : %s" % sorted(set(r["q"] for r in rows)))
    print("   r (report)      : %s" % sorted(set(r["r"] for r in rows)))
    print("   s (send result) : %s" % sorted(set(r["s"] for r in rows)))
    print("   y (retry)       : %s" % sorted(set(r["y"] for r in rows)))
    print("   t range (0.1 C) : %d .. %d" % (min(r["t"] for r in rows), max(r["t"] for r in rows)))
    print("-" * 78)
    ok = all(r["r"] == 0 and r["s"] == 0 and r["y"] == 0 for r in rows)
    print("   -> no report decision, no send attempt, no retry in any observed cycle: %s"
          % ("CONFIRMED" if ok else "NOT CONFIRMED"))
    print("   scope: only the cycles actually captured here; an untransmitted state was")
    print("   never observed to be overridden by the >35.0 C branch (all t <= %.1f C)."
          % (max(r["t"] for r in rows) / 10.0))
    print("   limitation: absence of sends in these windows is consistent with the")
    print("   uncalibrated gate, but it is not a proof for cycles that were not captured.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
