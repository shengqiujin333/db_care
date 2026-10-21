#!/usr/bin/env python3
"""Real-board check for ITEM-002 on COM43 raw captures.

Scope: the sensor board currently reports GXHT40 no-ACK, so every sample cycle is a
FAILURE cycle (q=0).  On such cycles this script verifies the observable part of the
ITEM-002 acceptance:

  * q=0 (no valid sample -> no decision, no report, no send);
  * S.p / S.H are unchanged from the previous cycle (prev is not advanced);
  * S.t / S.h keep the last valid values (0, because no valid sample has ever existed);
  * the same cycle appends exactly one G diagnostic line;
  * line framing/length constraints (CRLF, <=96 bytes).

It cannot verify the success-cycle requirement (p = old prev, t = current) because
U01 (GXHT40 address ACK) is not closed on this board.  That is recorded, not masked.

Usage: verify_com43_item002.py <capture.bin> [<capture.bin> ...]
Exit code 0 = all applicable checks passed; 1 = mismatch.
"""
import re
import sys

FIELDS = ["k", "p", "H", "V", "o", "n", "x", "a", "D", "t", "h", "q", "r", "s", "y"]
CHECK_PASS = 0
CHECK_FAIL = 0
FAIL = []


def check(cond, label, detail=""):
    global CHECK_PASS, CHECK_FAIL
    if cond:
        CHECK_PASS += 1
    else:
        CHECK_FAIL += 1
        FAIL.append("%s %s" % (label, detail))


def parse_s(line):
    m = re.match(r"^S ((?:[a-zA-Z0-9]+=-?\d+ ?)+)\r\n$", line)
    if not m:
        return None
    pairs = re.findall(r"([a-zA-Z0-9]+)=(-?\d+)", m.group(1))
    keys = [p[0] for p in pairs]
    if keys != FIELDS:
        return None
    return {k: int(v) for k, v in pairs}


def parse_g(line):
    m = re.match(r"^G s=(\d+) a44=(\d+) a45=(\d+) rd=(\d+) at=(\d+) raw=([-0-9A-F]+)\r\n$", line)
    if not m:
        return None
    return {
        "s": int(m.group(1)), "a44": int(m.group(2)), "a45": int(m.group(3)),
        "rd": int(m.group(4)), "at": int(m.group(5)), "raw": m.group(6),
    }


def main():
    paths = sys.argv[1:]
    if not paths:
        print("usage: verify_com43_item002.py <capture.bin> [...]")
        return 2

    cycles = []          # list of dicts with 's' and optional 'g'
    pending = None
    line_no = 0
    for path in paths:
        data = open(path, "rb").read()
        check(len(data) % 2 == 0 or b"\r\n" in data, path + " raw bytes", "len=%d" % len(data))
        for raw in data.split(b"\r\n")[:-1]:
            line = raw.decode("ascii", errors="replace") + "\r\n"
            line_no += 1
            check(len(line) <= 96, "line %d <= 96 bytes" % line_no, "actual=%d" % len(line))
            if line.startswith("S "):
                s = parse_s(line)
                check(s is not None, "line %d S-line schema" % line_no, repr(line))
                if s is None:
                    continue
                if pending is not None:
                    check(pending.get("g") is not None,
                          "cycle k=%d has one G failure line" % pending["k"], "missing")
                pending = dict(s, g=None)
                cycles.append(pending)
            elif line.startswith("G "):
                g = parse_g(line)
                check(g is not None, "line %d G-line schema" % line_no, repr(line))
                if pending is None:
                    check(False, "G line without preceding S", repr(line))
                else:
                    check(pending["g"] is None, "cycle k=%d has only one G line" % pending["k"], "duplicate")
                    pending["g"] = g
                    pending = None
            else:
                # BOOT/VDD/IOTEST/IOSIG/BUS or fragmented boot output may be present;
                # they are not part of this item's judgement.
                pass
    if pending is not None:
        check(pending.get("g") is not None, "last cycle k=%d has one G failure line" % pending["k"], "missing")

    check(len(cycles) >= 3, "at least 3 sample cycles observed", "actual=%d" % len(cycles))

    prev_p = None
    prev_H = None
    prev_t = None
    prev_h = None
    last_k = None
    for c in cycles:
        k = c["k"]
        tag = "k=%d" % k
        if last_k is not None:
            check(k - last_k == 3, tag + " cadence", "delta=%d expected=3 (3 x 1 min)" % (k - last_k))
        last_k = k

        check(c["q"] == 0, tag + " q (failure cycle, no valid sample)", "actual=%d" % c["q"])
        check(c["r"] == 0, tag + " r (no report decision)", "actual=%d" % c["r"])
        check(c["s"] == 0, tag + " s (no send)", "actual=%d" % c["s"])
        check(c["V"] == 0, tag + " V (light not calibrated -> invalid)", "actual=%d" % c["V"])
        check(c["D"] == 0, tag + " D (no dark verdict without valid light)", "actual=%d" % c["D"])

        if prev_p is not None:
            check(c["p"] == prev_p, tag + " p unchanged across failure", "actual=%d previous=%d" % (c["p"], prev_p))
            check(c["H"] == prev_H, tag + " H unchanged across failure", "actual=%d previous=%d" % (c["H"], prev_H))
            check(c["t"] == prev_t, tag + " t keeps last valid value", "actual=%d previous=%d" % (c["t"], prev_t))
            check(c["h"] == prev_h, tag + " h keeps last valid value", "actual=%d previous=%d" % (c["h"], prev_h))
        prev_p, prev_H, prev_t, prev_h = c["p"], c["H"], c["t"], c["h"]

        g = c.get("g")
        check(g is not None, tag + " G line present on failure", "")
        if g is not None:
            check(g["s"] == 2, tag + " G s=2 (no device / acquisition failed)", "actual=%d" % g["s"])
            check(g["a44"] == 0 and g["a45"] == 0, tag + " G a44/a45 show no GXHT40 ACK",
                  "actual=(%d,%d)" % (g["a44"], g["a45"]))
            check(re.match(r"^[-0-9A-F]{12}$", g["raw"]) is not None, tag + " G raw field width", g["raw"])

    # item-002 relevant aggregate: no cycle may show an advanced prev on a failure
    check(all(c["q"] == 0 for c in cycles), "no valid sample in this window (U01 open)", "")
    print("CYCLES %s" % ",".join("k=%d:p=%d:H=%d:t=%d:h=%d:q=%d:r=%d:s=%d" %
                                  (c["k"], c["p"], c["H"], c["t"], c["h"], c["q"], c["r"], c["s"])
                                  for c in cycles))
    print("RESULT checks_passed=%d checks_failed=%d" % (CHECK_PASS, CHECK_FAIL))
    for f in FAIL:
        print("MISMATCH " + f)
    print("LIMIT success-cycle p/t observation NOT exercised (all cycles q=0, GXHT40 ack=none)")
    return 0 if CHECK_FAIL == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
