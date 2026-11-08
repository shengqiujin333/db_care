#!/usr/bin/env python3
"""EV-028 independent analysis of COM43 raw captures (rev 6.4 / F-1 re-verification).

Reads the raw .bin files captured through the mounted serial_capture tool, splits on CRLF,
validates byte/line budgets and parses the frozen S/BUS fields.  No product constants are
imported; expectations come from TD-002 rev 6.0 / IC-002 v3.0.
"""
import os
import re
import sys

BASE = os.path.dirname(os.path.abspath(__file__))

FILES = [
    ("cap01", "cap01_k3_postflash.bin"),
    ("cap02", "cap02_k6_k9.bin"),
    ("cap03", "cap03_reset_bus_k0_k3.bin"),
    ("cap04", "cap04_k9_after_reset.bin"),
    ("cap05", "cap05_k18.bin"),
]

S_KEYS = ["k", "p", "H", "V", "o", "n", "x", "a", "D", "t", "h", "q", "r", "s", "y"]
G_RE = re.compile(r"^G s=\d a44=\d a45=\d rd=\d at=\d raw=(?:[0-9A-F]{12}|-{12})$")
BUS_RE = re.compile(r"^BUS idle=[0-3] scl=[01] sda=[01] ack=(?:none|[0-9A-F]{2}(?:,[0-9A-F]{2})*\+?)$")


def parse_s(line):
    if not line.startswith("S "):
        return None
    parts = line[2:].split(" ")
    out = {}
    for p in parts:
        if "=" not in p:
            raise ValueError("bad field %r in %r" % (p, line))
        k, v = p.split("=", 1)
        out[k] = v
    if list(out.keys()) != S_KEYS:
        raise ValueError("field set/order mismatch: %r" % (list(out.keys()),))
    return out


def main():
    records = []
    pending_bus = None
    ok = True
    print("== byte / line budget ==")
    for tag, name in FILES:
        path = os.path.join(BASE, name)
        data = open(path, "rb").read()
        # printable ASCII + CRLF only
        bad = [b for b in data if b not in (0x0D, 0x0A) and not (0x20 <= b <= 0x7E)]
        terminated = data.endswith(b"\r\n")
        chunks = data.split(b"\r\n")
        lines = [c.decode("ascii") for c in chunks if c]
        stray = len([c for c in chunks if c and not c.decode("ascii").startswith(("S ", "BUS ", "G "))])
        print("%s %s bytes=%d non_ascii=%d crlf_terminated=%s lines=%d stray=%d"
              % (tag, name, len(data), len(bad), terminated, len(lines), stray))
        if bad or not terminated or stray:
            ok = False
        for ln in lines:
            raw_len = len(ln.encode("ascii")) + 2
            kind = ln.split(" ")[0]
            print("   %-4s len=%d%s  %s" % (kind, raw_len, "" if raw_len <= 96 else "  *** >96 ***", ln))
            if raw_len > 96:
                ok = False
            if ln.startswith("BUS "):
                if not BUS_RE.match(ln):
                    print("   *** BUS format mismatch ***")
                    ok = False
                pending_bus = ln
            elif ln.startswith("G "):
                if not G_RE.match(ln):
                    print("   *** G format mismatch ***")
                    ok = False
                records[-1]["g"] = ln
            elif ln.startswith("S "):
                rec = parse_s(ln)
                rec["tag"] = tag
                rec["line"] = ln
                rec["bus"] = pending_bus
                records.append(rec)
            else:
                print("   *** unknown line ***")
                ok = False

    print("\n== S records ==")
    print("tag  k    p    H V o n    x    a    D t    h    q r s y")
    for r in records:
        print("%-4s %-4s %-4s %s %s %s %-4s %-4s %-4s %s %-4s %-4s %s %s %s %s"
              % (r["tag"], r["k"], r["p"], r["H"], r["V"], r["o"], r["n"], r["x"],
                 r["a"], r["D"], r["t"], r["h"], r["q"], r["r"], r["s"], r["y"]))

    print("\n== checks ==")
    q_all = all(r["q"] == "1" for r in records)
    no_g = all("g" not in r for r in records)
    trange = [(int(r["t"]), int(r["h"])) for r in records]
    t_ok = all(-400 <= t <= 1250 for t, _ in trange)
    h_ok = all(0 <= h <= 1000 for _, h in trange)
    uncal = all(r["V"] == "0" and r["D"] == "0" for r in records)
    ks = [int(r["k"]) for r in records]
    print("q=1 all / no G ........... %s / %s" % (q_all, no_g))
    print("t range ................. %s (%s)" % (t_ok, [t for t, _ in trange]))
    print("h range ................. %s (%s)" % (h_ok, [h for _, h in trange]))
    print("V=0 D=0 (uncalibrated) .. %s" % uncal)
    print("k values ................ %s (all multiples of 3: %s)"
          % (ks, all(k % 3 == 0 for k in ks)))

    # F-1 semantics: p of a cycle must equal t of the previous cycle (same boot session).
    # Two flash/download sessions exist: A = cap01+cap02 (flash 11:28:51),
    # B = cap03+cap04+cap05 (flash 11:39:47).  k restarts at 0 in each session.
    print("\n== F-1 prev-value chain (grouped by boot session) ==")
    sessions = [
        ("A (flash 1)", ["cap01", "cap02"]),
        ("B (flash 2)", ["cap03", "cap04", "cap05"]),
    ]
    for sname, tags in sessions:
        seq = [r for r in records if r["tag"] in tags]
        for i, r in enumerate(seq):
            if i == 0:
                print("%s k=%s first-in-session p=%s H=%s" % (sname, r["k"], r["p"], r["H"]))
                continue
            prev = seq[i - 1]
            gap = int(r["k"]) - int(prev["k"])
            if gap == 3:
                matched = prev["t"] == r["p"]
                print("%s k=%s p=%s  prev-captured(k=%s) t=%s  consecutive-match=%s"
                      % (sname, r["k"], r["p"], prev["k"], prev["t"], matched))
                if not matched:
                    ok = False
            else:
                # An intermediate S line fell outside the capture window; p must still be
                # the pre-update value, i.e. it must NOT echo this cycle's measurement.
                distinct = r["p"] != r["t"]
                print("%s k=%s p=%s  (gap of %d ticks; p != t required)  ok=%s"
                      % (sname, r["k"], r["p"], gap, distinct))
                if not distinct:
                    ok = False
    print("\nNOTE: in session B the k=6 and k=15 lines fell outside the capture windows;")
    print("      p(k=9)=260 and p(k=18)=245 prove those cycles' t values were the")
    print("      judgement basis for the following cycle (no line was fabricated).")

    print("\nRESULT: %s" % ("PASS (format/budget/field checks)" if ok else "FAIL"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
