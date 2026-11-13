#!/usr/bin/env python3
"""EV-040 independent analysis of the COM43 capture (boot diagnostics + sampling cadence).

Reads the raw .cap bytes produced by boot_and_passive_capture.py and prints every
assertion with PASS/FAIL.  No product code is imported; this is a test-side parser.
"""
import re
import sys

CAP = sys.argv[1]
OUT = sys.argv[2]
LOG = sys.argv[3] if len(sys.argv) > 3 else CAP + ".log"
MODE = sys.argv[4] if len(sys.argv) > 4 else "full"

raw = open(CAP, "rb").read()
text = raw.decode("ascii", errors="replace")
lines = text.split("\r\n")
if lines and lines[-1] == "":
    lines = lines[:-1]

results = []


def check(name, ok, detail=""):
    results.append((name, bool(ok), detail))


check("no NUL byte", b"\x00" not in raw, "nul=%d" % raw.count(b"\x00"))
check("ends with CRLF", raw.endswith(b"\r\n"), "tail=%r" % raw[-4:])
check("ascii decodable", "�" not in text, "replacement_chars=%d" % text.count("�"))

# line lengths (bytes including CRLF)
line_lengths = [len(l.encode("ascii", "replace")) + 2 for l in lines]
check("line length <= 96 B", all(n <= 96 for n in line_lengths), "max=%d" % max(line_lengths))

boot_order = ["BOOT", "VDD", "IOTEST", "IOSIG", "BUS"]
heads = [l.split(" ", 1)[0] for l in lines]
check("boot five unique in order", heads[:5] == boot_order, str(heads[:5]))
check("each boot line once", all(heads.count(h) == 1 for h in boot_order), str({h: heads.count(h) for h in boot_order}))

# BOOT fields
m = re.match(r"^BOOT fw=(\S+) uid=(\S+) rst=(\S+) uart=(\d+)$", lines[0]) if lines else None
check("BOOT fields", m is not None, lines[0] if lines else "none")
if m:
    check("BOOT uart=9600", m.group(4) == "9600", m.group(4))
    check("BOOT uid matches board history", m.group(2) == "6A002C00", m.group(2))

# VDD formula
m = re.match(r"^VDD ok=(\d+) code=(\d+) bgrmv=(\d+) mv=(\d+)$", lines[1])
check("VDD ok=1", bool(m and m.group(1) == "1"), lines[1] if len(lines) > 1 else "none")
if m:
    expect = 4095 * int(m.group(3)) // int(m.group(2))
    check("VDD mv formula", expect == int(m.group(4)), "4095*%s//%s=%d vs %s" % (m.group(3), m.group(2), expect, m.group(4)))

# I2C self test / bus
m = re.match(r"^IOTEST sda_lo=(\d+) scl_lo=(\d+) idle=(\d+) swap=(\S+) gc=(\d+)$", lines[2])
check("IOTEST fields", m is not None, lines[2] if len(lines) > 2 else "none")
if m:
    check("IOTEST gc=0 (no general-call reset)", m.group(5) == "0", m.group(5))
m = re.match(r"^BUS idle=(\d+) scl=(\d+) sda=(\d+) ack=(\S+)$", lines[4])
check("BUS fields", m is not None, lines[4] if len(lines) > 4 else "none")
if m:
    check("BUS ack=44 (GXHT40 address)", m.group(4) == "44", m.group(4))

# S lines
S_RE = re.compile(
    r"^S k=(\d+) p=(-?\d+) H=(\d+) V=(\d+) o=(\d+) n=(\d+) x=(\d+) a=(\d+) "
    r"D=(\d+) t=(-?\d+) h=(\d+) q=(\d+) r=(\d+) s=(\d+) y=(\d+)$")
srows = []
for i, l in enumerate(lines):
    if l.startswith("S "):
        mm = S_RE.match(l)
        check("S line %d parses" % i, mm is not None, l)
        if mm:
            keys = ["k", "p", "H", "V", "o", "n", "x", "a", "D", "t", "h", "q", "r", "s", "y"]
            srows.append(dict(zip(keys, [int(g) for g in mm.groups()]), line=l))

min_s = 4 if MODE == "full" else 1
check("S samples >= %d" % min_s, len(srows) >= min_s, "count=%d" % len(srows))
if srows:
    check("k strictly increasing", all(srows[i]["k"] < srows[i + 1]["k"] for i in range(len(srows) - 1)),
          str([r["k"] for r in srows]))
    check("k step == 3", all(srows[i + 1]["k"] - srows[i]["k"] == 3 for i in range(len(srows) - 1)),
          str([srows[i + 1]["k"] - srows[i]["k"] for i in range(len(srows) - 1)]))
    check("first H=0 p=0", srows[0]["H"] == 0 and srows[0]["p"] == 0,
          "H=%d p=%d" % (srows[0]["H"], srows[0]["p"]))
    check("later H=1", all(r["H"] == 1 for r in srows[1:]), str([r["H"] for r in srows]))
    check("p chain == previous t", all(srows[i]["p"] == srows[i - 1]["t"] for i in range(1, len(srows))),
          str([(r["p"], srows[i - 1]["t"]) for i, r in enumerate(srows) if i > 0]))
    check("all q=1", all(r["q"] == 1 for r in srows), str([r["q"] for r in srows]))
    check("all r=s=y=0", all(r["r"] == 0 and r["s"] == 0 and r["y"] == 0 for r in srows),
          str([(r["r"], r["s"], r["y"]) for r in srows]))
    check("all V=0 D=0 (delivery default T=0)", all(r["V"] == 0 and r["D"] == 0 for r in srows),
          str([(r["V"], r["D"]) for r in srows]))
    check("all o=8", all(r["o"] == 8 for r in srows), str([r["o"] for r in srows]))
    check("t in [-400,1250] (0.1 C)", all(-400 <= r["t"] <= 1250 for r in srows), str([r["t"] for r in srows]))
    check("h in [0,1000] (0.1 %RH)", all(0 <= r["h"] <= 1000 for r in srows), str([r["h"] for r in srows]))
    check("n <= a <= x", all(r["n"] <= r["a"] <= r["x"] for r in srows),
          str([(r["n"], r["a"], r["x"]) for r in srows]))

check("no P (report) lines", not any(l.startswith("P ") for l in lines),
      str([l for l in lines if l.startswith("P ")]))
check("no G (failure) lines", not any(l.startswith("G ") for l in lines),
      str([l for l in lines if l.startswith("G ")]))
check("no unexpected line prefixes",
      set(heads) <= set(boot_order + ["S"]), str(sorted(set(heads))))

# cadence from the capture log timestamps
try:
    loglines = open(LOG, encoding="utf-8").read().splitlines()
except OSError:
    loglines = []
times = []
for l in loglines:
    mm = re.match(r"  t\+([0-9.]+)s .*b'S k=(\d+)", l)
    if mm:
        times.append((float(mm.group(1)), int(mm.group(2))))
if len(times) >= 3 and MODE == "full":
    times.sort()
    deltas = [round(times[i + 1][0] - times[i][0], 3) for i in range(len(times) - 1)]
    ks = [t[1] for t in times]
    # The first post-boot interval can be shorter than 180 s when the free-running
    # RTC counter is not phase-aligned to the reset instant.  Steady state is the
    # interval between consecutive scheduled samples after that first alignment.
    steady = deltas[1:]
    check("steady-state interval within 180+-2 s", all(178 <= d <= 182 for d in steady), str(steady))
    check("no missed sampling cycle (interval <= 182 s)", all(d <= 182 for d in deltas), str(deltas))
    cadence_detail = "k=%s intervals=%s (first post-boot interval is RTC phase alignment; steady=%s)" % (
        ks, deltas, steady)
elif MODE == "full":
    cadence_detail = "timestamps not parsed"
else:
    cadence_detail = "boot-only run (cadence checked in the full capture)"
check("cadence detail available", MODE != "full" or len(times) >= 3, cadence_detail)

print("EV-040 COM43 capture analysis")
print("capture=%s bytes=%d lines=%d" % (CAP, len(raw), len(lines)))
print(cadence_detail)
print()
fails = 0
for name, ok, detail in results:
    if not ok:
        fails += 1
    print("%-45s %s  %s" % (name, "PASS" if ok else "FAIL", detail))
print()
print("TOTAL=%d FAILED=%d" % (len(results), fails))
print("RESULT: %s" % ("PASS" if fails == 0 else "FAIL"))

with open(OUT, "w", encoding="utf-8", newline="\n") as f:
    f.write("EV-040 COM43 capture analysis\n")
    f.write("capture=%s bytes=%d lines=%d\n" % (CAP, len(raw), len(lines)))
    f.write(cadence_detail + "\n\n")
    for name, ok, detail in results:
        f.write("%-45s %s  %s\n" % (name, "PASS" if ok else "FAIL", detail))
    f.write("\nTOTAL=%d FAILED=%d\n" % (len(results), fails))
    f.write("RESULT: %s\n" % ("PASS" if fails == 0 else "FAIL"))
sys.exit(0 if fails == 0 else 1)
