#!/usr/bin/env python3
"""
Independent parser/judge for the raw COM43 captures taken after mdk_flash of the
current ITEM-003 image.

It only checks what a failed (q=0) cycle can show about ITEM-003:
  * S.s is the final local result of the cycle (0 = nothing to send) and S.y is 0;
  * no trigger/send state survives: every cycle is independent (no report_req
    visible, but s=0 and one S per cycle);
  * the two capture windows show the 3-minute cadence (k advances by 3);
  * line framing: every line ends CRLF and is <= 96 bytes;
  * S fields are self-consistent (V=0 -> D=0, o/n/x/a domains).

The board currently has no GXHT40 ACK (U01), so no trigger cycle can be produced
on the real target; the frozen-frame/3-attempt semantics are proven on the host
(verify_measure_freeze_item003.py) and this script does NOT claim them.

Usage: python verify_com43_item003.py com43_capture_a.dat com43_capture_b.dat
"""
import re
import sys

LINE_LIMIT = 96

S_RE = re.compile(
    r"^S k=(\d+) p=(-?\d+) H=(\d+) V=(\d+) o=(\d+) n=(\d+) x=(\d+) a=(\d+) "
    r"D=(\d+) t=(-?\d+) h=(\d+) q=(\d+) r=(\d+) s=(\d+) y=(\d+)$")
G_RE = re.compile(
    r"^G s=(\d+) a44=(\d+) a45=(\d+) rd=(\d+) at=(\d+) raw=([0-9A-F-]+)$")


class Checker:
    def __init__(self):
        self.passed = 0
        self.failed = 0
        self.failures = []

    def check(self, cond, msg):
        if cond:
            self.passed += 1
        else:
            self.failed += 1
            self.failures.append(msg)


def main():
    if len(sys.argv) < 3:
        print("usage: verify_com43_item003.py <capture_a.dat> <capture_b.dat>")
        return 2

    ck = Checker()
    all_k = []
    all_s = []

    for path in sys.argv[1:]:
        with open(path, "rb") as fh:
            data = fh.read()
        lines = data.split(b"\r\n")
        trailing_empty = (lines[-1] == b"")
        ck.check(trailing_empty, "%s: last line has no CRLF terminator" % path)
        lines = [b for b in lines if b != b""]
        ck.check(len(lines) > 0, "%s: no lines" % path)

        for raw in lines:
            wire_len = len(raw) + 2
            ck.check(wire_len <= LINE_LIMIT,
                     "%s: line %d bytes > %d: %r" % (path, wire_len, LINE_LIMIT, raw[:40]))
            text = raw.decode("ascii", errors="replace")
            m = S_RE.match(text)
            if m:
                g = m.groups()
                k, p, H, V, o, n, x, a, D, t, h, q, r, s, y = [int(v) for v in g]
                all_k.append(k)
                all_s.append(dict(p=p, H=H, V=V, o=o, n=n, x=x, a=a, D=D,
                                  t=t, h=h, q=q, r=r, s=s, y=y, raw=text))
                # field domains / self-consistency
                ck.check(o <= 8, "%s: o=%d out of 0..8" % (path, o))
                ck.check(n <= a <= x, "%s: expected n<=a<=x, got %d/%d/%d" % (path, n, a, x))
                ck.check((a == 0) == (o == 0), "%s: a==0 must match o==0 (%d/%d)" % (path, a, o))
                ck.check(D == 0 or V == 1, "%s: D=1 with V=0" % path)
                ck.check(-400 <= t <= 1250, "%s: t=%d out of sensor domain" % (path, t))
                ck.check(0 <= h <= 1000, "%s: h=%d out of sensor domain" % (path, h))
                ck.check(s in (0, 1, 2), "%s: S.s=%d out of 0/1/2" % (path, s))
                # ITEM-003 observables on a non-trigger cycle
                ck.check(r == 0, "%s: r=%d but all cycles on this board are q=0" % (path, r))
                ck.check(q == 0, "%s: q=%d unexpected (no GXHT40 ACK on this board)" % (path, q))
                ck.check(s == 0, "%s: S.s=%d on a non-trigger cycle, expected 0" % (path, s))
                ck.check(y == 0, "%s: S.y=%d, expected 0" % (path, y))
                continue
            m = G_RE.match(text)
            if m:
                ck.check(m.group(1) in ("2", "3", "4", "5"),
                         "%s: G.s=%s out of 2..5" % (path, m.group(1)))
                ck.check(m.group(6) == "------------" or len(m.group(6)) == 12,
                         "%s: G.raw length %d" % (path, len(m.group(6))))
                continue
            ck.check(False, "%s: unrecognized line %r" % (path, text[:60]))

    # every S must be followed by exactly one G (failure cycle) -- checked by order
    for path in sys.argv[1:]:
        with open(path, "rb") as fh:
            text = fh.read().decode("ascii", errors="replace")
        seq = [ln.split(" ")[0] for ln in text.split("\r\n") if ln]
        for i, tag in enumerate(seq):
            if tag == "S":
                ck.check(i + 1 < len(seq) and seq[i + 1] == "G",
                         "%s: S at position %d not followed by G" % (path, i))
            elif tag == "G":
                ck.check(i > 0 and seq[i - 1] == "S", "%s: G at %d without S" % (path, i))
        ck.check(seq.count("S") == len([x for x in seq if x == "S"]),
                 "%s: S count" % path)

    # cadence: k must advance by 3 between consecutive captured cycles
    ck.check(len(all_k) >= 3, "only %d cycles captured" % len(all_k))
    for a, b in zip(all_k, all_k[1:]):
        ck.check(b - a == 3, "k advanced %d -> %d (expected +3 = 180 s)" % (a, b))

    # p/H unchanged across the failed cycles (failed cycle must not advance prev)
    if all_s:
        ck.check(all(x["p"] == all_s[0]["p"] and x["H"] == all_s[0]["H"] for x in all_s),
                 "p/H changed across failed cycles")
        ck.check(all(x["t"] == all_s[0]["t"] and x["h"] == all_s[0]["h"] for x in all_s),
                 "t/h changed across failed cycles without a valid sample")

    print("captures=%d cycles=%d k=%s" % (len(sys.argv) - 1, len(all_k), all_k))
    for x in all_s:
        print(x["raw"])
    for f in ck.failures:
        print("FAIL %s" % f)
    print("SUMMARY passed=%d failed=%d" % (ck.passed, ck.failed))
    if ck.failed == 0 and all_s:
        print("RESULT PASS (failed-cycle observables only; trigger path not producible on this board)")
        return 0
    print("RESULT FAIL")
    return 1


if __name__ == "__main__":
    sys.exit(main())
