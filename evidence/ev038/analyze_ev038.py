#!/usr/bin/env python3
"""EV-038 independent analysis of real-board COM43 captures and bit-level I2C decode.

Independent re-implementation: does not import or call product firmware code and
does not reuse implementation constants.  All numbers are recomputed here.
"""
import hashlib
import os
import re

BASE = os.path.dirname(os.path.abspath(__file__))
CAPS = [
    ("bot01_hw.cap", "boot + first sample"),
    ("cap01_k3.cap", ""),
    ("cap02_k6_k9.cap", ""),
    ("cap03_k12_k15.cap", ""),
    ("cap04_k21.cap", ""),
]

pass_n = 0
fail_n = 0


def check(cond, msg):
    global pass_n, fail_n
    if cond:
        pass_n += 1
        print("PASS  " + msg)
    else:
        fail_n += 1
        print("FAIL  " + msg)


def crc8_gxht40(data):
    """CRC-8, poly 0x31, init 0xFF, no reflection (GXHT40 datasheet)."""
    crc = 0xFF
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = ((crc << 1) ^ 0x31) & 0xFF if crc & 0x80 else (crc << 1) & 0xFF
    return crc


S_KEYS = ["k", "p", "H", "V", "o", "n", "x", "a", "D", "t", "h", "q", "r", "s", "y"]

observed = {}   # tick -> dict
rows = []       # (file, line)
boot = {}
line_lengths = []
stray_notes = []

for fn, tag in CAPS:
    path = os.path.join(BASE, fn)
    raw = open(path, "rb").read()
    print("\n== %s (%d B, md5 %s) %s" %
          (fn, len(raw), hashlib.md5(raw).hexdigest(), tag))
    check(b"\x00" not in raw, "%s: no NUL bytes" % fn)
    check(raw.endswith(b"\r\n"), "%s: capture ends on CRLF boundary" % fn)
    lines = raw.split(b"\r\n")
    check(lines[-1] == b"", "%s: no trailing partial line" % fn)
    for lb in lines[:-1]:
        line_lengths.append(len(lb) + 2)
        check(len(lb) + 2 <= 96, "%s: line length %d B incl CRLF <= 96" % (fn, len(lb) + 2))
        line = lb.decode("ascii", "replace")
        check("\x00" not in line, "%s: printable ASCII line" % fn)
        rows.append((fn, line))
        if line.startswith("BOOT "):
            parts = line.split()
            boot["raw"] = line
            boot["keys"] = [p.split("=", 1)[0] for p in parts[1:]]
        elif line.startswith("VDD "):
            m = re.search(r"ok=(\d+) code=(\d+) bgrmv=(\d+) mv=(\d+)", line)
            if m:
                boot["vdd"] = tuple(int(x) for x in m.groups())
            boot["vdd_raw"] = line
        elif line.startswith("IOTEST "):
            boot["iotest"] = line
        elif line.startswith("IOSIG "):
            boot["iosig"] = line
        elif line.startswith("BUS "):
            boot["bus"] = line
        elif line.startswith("S "):
            parts = line.split()
            keys = [p.split("=", 1)[0] for p in parts[1:]]
            if keys == S_KEYS:
                d = {k: int(v) for k, v in (p.split("=", 1) for p in parts[1:])}
                observed[d["k"]] = d

# --- boot sequence ---
print("\n== boot lines")
seq = [name for name in ("BOOT", "VDD", "IOTEST", "IOSIG", "BUS")]
boot_lines = [r for f, r in rows if r.split()[0] in seq]
check([r.split()[0] for r in boot_lines] == seq,
      "BOOT->VDD->IOTEST->IOSIG->BUS each exactly once, in order")
check(sum(1 for f, r in rows if r.startswith("BOOT ")) == 1, "exactly one BOOT line")
check(boot.get("keys") == ["fw", "uid", "rst", "uart"], "BOOT fields fw/uid/rst/uart")
check("uart=9600" in boot.get("raw", ""), "BOOT uart=9600")
if "vdd" in boot:
    ok, code, bgrmv, mv = boot["vdd"]
    check(ok == 1, "VDD ok=1")
    check(code != 0 and (4095 * bgrmv) // code == mv,
          "VDD mv == 4095*bgrmv/code (computed %d)" % ((4095 * bgrmv) // code if code else -1))
check("gc=0" in boot.get("iotest", ""), "IOTEST gc=0")
check("ack=44" in boot.get("bus", ""), "BUS ack=44 (0x44 device answered)")

# --- periodic S lines ---
print("\n== periodic S rows")
for k in sorted(observed):
    d = observed[k]
    print("  k=%d p=%d t=%d h=%d V=%d D=%d o=%d n=%d x=%d a=%d q=%d H=%d r=%d s=%d y=%d" %
          (k, d["p"], d["t"], d["h"], d["V"], d["D"], d["o"], d["n"], d["x"],
           d["a"], d["q"], d["H"], d["r"], d["s"], d["y"]))
check(len(observed) >= 4, ">= 4 distinct S rows observed (%d)" % len(observed))
check(min(observed) == 0, "first sample k=0 captured (H/p no-history evidence)")
check(len(set(observed)) == len(observed), "no duplicate k rows")

ticks = sorted(observed)
for a, b in zip(ticks, ticks[1:]):
    gap = b - a
    check(gap % 3 == 0 and gap >= 3, "tick step k=%d->%d is a positive multiple of 3 (=%d)" % (a, b, gap))

for k in ticks:
    d = observed[k]
    check(d["q"] == 1, "k=%d q=1" % k)
    check(d["o"] == 8, "k=%d o=8" % k)
    check(d["r"] == 0 and d["s"] == 0 and d["y"] == 0, "k=%d r=s=y=0" % k)
    check(-400 <= d["t"] <= 1250, "k=%d t=%d inside [-400,1250]" % (k, d["t"]))
    check(0 <= d["h"] <= 1000, "k=%d h=%d inside [0,1000]" % (k, d["h"]))
    check(d["n"] <= d["a"] <= d["x"], "k=%d n<=a<=x" % k)
    check(d["V"] == 0 and d["D"] == 0, "k=%d V=0 D=0 (criterion-disabled equivalence)" % k)

# p chain: every non-first row's p equals the previous row's t
for a, b in zip(ticks, ticks[1:]):
    if b - a == 3:
        check(observed[b]["p"] == observed[a]["t"],
              "p(k=%d)=%d == t(k=%d)=%d" % (b, observed[b]["p"], a, observed[a]["t"]))

check(observed[0]["H"] == 0 and observed[0]["p"] == 0, "first sample H=0 p=0")
for k in ticks[1:]:
    check(observed[k]["H"] == 1, "k=%d H=1 (previous value available)" % k)

# --- ordered line order in capture: S lines strictly increasing k ---
s_ticks = [int(re.match(r"S k=(\d+)", r).group(1)) for f, r in rows if r.startswith("S ")]
check(s_ticks == sorted(s_ticks) and len(set(s_ticks)) == len(s_ticks),
      "S rows appear once each in increasing k order: %s" % s_ticks)

# --- I2C decode independent recomputation ---
print("\n== bit-level I2C decode")
dec = open(os.path.join(BASE, "decoded_i2c.txt")).read()
m_wr = re.search(r"Address write: 44", dec)
m_fd = re.search(r"Data write: FD", dec)
check(m_wr is not None and m_fd is not None, "decode contains 0x44 write and 0xFD command")
check("Address read: 44" in dec, "decode contains 0x44 read address")
reads = re.findall(r"Data read: ([0-9A-F]{2})", dec)
print("  read bytes:", reads)
check(len(reads) == 6, "exactly 6 data bytes read (%d)" % len(reads))
check(dec.count("Start") == 2 and dec.count("Stop") == 2,
      "two START and two STOP conditions")
check("NACK" in dec.rsplit("Data read", 1)[-1], "last byte NACKed by master")
if len(reads) == 6:
    b = [int(x, 16) for x in reads]
    t_raw = (b[0] << 8) | b[1]
    h_raw = (b[3] << 8) | b[4]
    check(crc8_gxht40([b[0], b[1]]) == b[2],
          "temperature CRC-8 0x%02X matches 0x%02X" % (crc8_gxht40([b[0], b[1]]), b[2]))
    check(crc8_gxht40([b[3], b[4]]) == b[5],
          "humidity CRC-8 0x%02X matches 0x%02X" % (crc8_gxht40([b[3], b[4]]), b[5]))
    t_x10 = (1750 * t_raw + 32768) // 65536 - 450
    h_x10 = (1250 * h_raw + 32768) // 65536 - 60
    h_x10 = max(0, min(1000, h_x10))
    print("  t_x10=%d h_x10=%d (raw T=0x%04X H=0x%04X)" % (t_x10, h_x10, t_raw, h_raw))
    check(-400 <= t_x10 <= 1250, "decoded temperature inside valid domain")
    check(0 <= h_x10 <= 1000, "decoded humidity inside valid domain")
    # cross-check with the serial p chain: p(k=21) is the k=18 sample temperature
    if 21 in observed:
        check(observed[21]["p"] == t_x10,
              "p(k=21)=%d equals decoded transaction temperature %d"
              % (observed[21]["p"], t_x10))
    else:
        check(False, "k=21 row available for I2C pairing")

print("\n== line length stats")
print("  min=%d max=%d n=%d" % (min(line_lengths), max(line_lengths), len(line_lengths)))
check(max(line_lengths) <= 96, "all lines <= 96 B including CRLF")

print("\n== RESULT: PASS=%d FAIL=%d" % (pass_n, fail_n))
raise SystemExit(0 if fail_n == 0 else 1)
