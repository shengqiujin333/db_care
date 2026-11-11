#!/usr/bin/env python3
"""EV-037 independent analysis of real-board COM43 captures and bit-level I2C decode.

Independent re-implementation: does not import or call product firmware code and
does not reuse implementation constants.  All numbers are recomputed here.
"""
import hashlib
import os
import re
import sys

BASE = os.path.dirname(os.path.abspath(__file__))
CAPS = [
    ("bot01_hw.bin", "boot+first sample"),
    ("cap01_k3.bin", ""),
    ("cap02_k6_k9.bin", ""),
    ("cap03_k12_k15.bin", ""),
    ("cap04_k21.bin", ""),
]

pass_n = 0
fail_n = 0
notes = []


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
FLASH_FIELDS = ["fw", "uid", "rst", "uart"]

observed = {}  # tick -> dict
all_rows = []
boot = {}
line_lengths = []

for fn, tag in CAPS:
    path = os.path.join(BASE, fn)
    raw = open(path, "rb").read()
    print("\n== %s (%d B, md5 %s) %s" % (fn, len(raw), hashlib.md5(raw).hexdigest(), tag))
    # split on CRLF and verify no stray bytes
    check(b"\x00" not in raw, "%s: no NUL bytes" % fn)
    check(raw.endswith(b"\r\n"), "%s: capture ends on CRLF boundary" % fn)
    lines = raw.split(b"\r\n")
    check(lines[-1] == b"", "%s: no trailing partial line" % fn)
    for lb in lines[:-1]:
        check(len(lb) + 2 <= 96, "%s: line length %d B incl CRLF <= 96" % (fn, len(lb) + 2))
        line = lb.decode("ascii", "replace")
        all_rows.append((fn, line))
        if line.startswith("BOOT "):
            boot["raw"] = line
            parts = line.split()
            keys = [p.split("=", 1)[0] for p in parts[1:]]
            boot["keys"] = keys
        elif line.startswith("VDD "):
            m = re.search(r"ok=(\d+) code=(\d+) bgrmv=(\d+) mv=(\d+)", line)
            if m:
                boot["vdd"] = tuple(int(x) for x in m.groups())
        elif line.startswith("IOTEST "):
            boot["iotest"] = line
        elif line.startswith("IOSIG "):
            boot["iosig"] = line
        elif line.startswith("BUS "):
            boot["bus"] = line
        elif line.startswith("S "):
            parts = line.split()
            keys = [p.split("=", 1)[0] for p in parts[1:]]
            check(keys == S_KEYS, "%s: S field names/order frozen" % fn)
            vals = {p.split("=", 1)[0]: int(p.split("=", 1)[1]) for p in parts[1:]}
            for k in ("H", "V", "D", "q", "r"):
                check(vals[k] in (0, 1), "%s: S %s in {0,1}" % (fn, k))
            check(vals["s"] in (0, 1, 2), "%s: S s in {0,1,2}" % fn)
            check(vals["y"] in (0, 1, 2, 3), "%s: S y in {0..3}" % fn)
            check(vals["o"] in range(0, 9), "%s: S o in 0..8" % fn)
            check(-400 <= vals["t"] <= 1250, "%s: S t in [-400,1250]" % fn)
            check(0 <= vals["h"] <= 1000, "%s: S h in [0,1000]" % fn)
            check(0 <= vals["n"] <= vals["x"] <= 4095, "%s: S n<=x in 0..4095" % fn)
            if vals["o"] == 0:
                check(vals["n"] == vals["x"] == vals["a"] == 0,
                      "%s: S o=0 implies n=x=a=0" % fn)
            else:
                check(vals["n"] <= vals["a"] <= vals["x"],
                      "%s: S a within [n,x] (o=%d)" % (fn, vals["o"]))
            check(vals["q"] == 1, "%s: S k=%d q=1 (sample ok)" % (fn, vals["k"]))
            check(vals["V"] == 0 and vals["D"] == 0,
                  "%s: S k=%d V=0 D=0 (LIGHT-DEF-2 disabled, T=0)" % (fn, vals["k"]))
            check(vals["k"] not in observed, "%s: tick k=%d not duplicated" % (fn, vals["k"]))
            observed[vals["k"]] = vals
        else:
            check(False, "%s: unexpected line kind %r" % (fn, line[:20]))

print("\n== boot sequence")
check(boot.get("keys") == FLASH_FIELDS, "BOOT field order fw,uid,rst,uart")
check(boot.get("raw", "").endswith("uart=9600"), "BOOT uart=9600")
vdd = boot.get("vdd")
check(vdd is not None and vdd[0] == 1, "VDD ok=1")
if vdd:
    ok, code, bgrmv, mv = vdd
    check(4095 * bgrmv // code == mv,
          "VDD recompute 4095*%d//%d = %d == %d" % (bgrmv, code, 4095 * bgrmv // code, mv))
check("ack=44" in boot.get("bus", ""), "BUS ack=44 (0x44 device ACK)")
check(re.search(r"swap=none", boot.get("iotest", "")) is not None, "IOTEST swap=none")
check(re.search(r"gc=0", boot.get("iotest", "")) is not None, "IOTEST gc=0")

# order: BOOT,VDD,IOTEST,IOSIG,BUS are the first five rows, then S k=0
kinds = []
for fn, line in all_rows:
    kinds.append(line.split()[0] if line.split() else "")
first = kinds[:6]
check(first[:5] == ["BOOT", "VDD", "IOTEST", "IOSIG", "BUS"],
      "power-on five lines ordered once each: %s" % first[:5])
check(first[5] == "S", "first sample S follows BUS")
check(all(k.count("BOOT") == 1 for k in [",".join(kinds)]), "exactly one BOOT line")
check(0 in observed, "S k=0 captured after reset")

print("\n== cadence / p chain")
ticks = sorted(observed)
print("observed ticks:", ticks)
check(len(ticks) >= 3, "at least 3 cycles captured (%d)" % len(ticks))
check(ticks[0] == 0, "first observed tick is 0")
gaps = [b - a for a, b in zip(ticks, ticks[1:])]
check(all(g in (3, 6) for g in gaps), "tick gaps are multiples of 3: %s" % gaps)
for k in ticks:
    v = observed[k]
    if k == 0:
        check(v["p"] == 0 and v["H"] == 0, "k=0: p=0 H=0 (no previous sample)")
    elif (k - 3) in observed:
        prev = observed[k - 3]
        check(v["p"] == prev["t"] and v["H"] == 1,
              "k=%d: p=%d == t(k=%d)=%d and H=1" % (k, v["p"], k - 3, prev["t"]))
check(all(observed[k]["r"] == 0 and observed[k]["s"] == 0 and observed[k]["y"] == 0
          for k in ticks), "no report/transmit/retry in observed cycles (r=s=y=0)")

print("\n== bit-level I2C (independent CRC-8)")
dec = open(os.path.join(BASE, "decoded_i2c.txt"), encoding="utf-8", errors="replace").read()
addr_w = "Address write: 44" in dec
addr_r = "Address read: 44" in dec
cmd = "Data write: FD" in dec
check(addr_w and cmd and addr_r, "decode shows 0x44 write 0xFD then 0x44 read")
data = [int(m, 16) for m in re.findall(r"Data read: ([0-9A-F]{2})", dec)]
print("read bytes:", " ".join("%02X" % b for b in data))
check(len(data) == 6, "six data bytes read")
if len(data) == 6:
    t_raw = (data[0] << 8) | data[1]
    h_raw = (data[3] << 8) | data[4]
    t_crc = data[2]
    h_crc = data[5]
    check(crc8_gxht40([data[0], data[1]]) == t_crc,
          "temperature word %02X%02X CRC %02X MATCH" % (data[0], data[1], t_crc))
    check(crc8_gxht40([data[3], data[4]]) == h_crc,
          "humidity word %02X%02X CRC %02X MATCH" % (data[3], data[4], h_crc))
    t_x10 = round(-450 + 1750.0 * t_raw / 65535.0)
    h_x10 = round(1000.0 * h_raw / 65535.0)
    print("converted: temp %d x10, humidity %d x10" % (t_x10, h_x10))
    notes.append("I2C temp x10 = %d, humidity x10 = %d" % (t_x10, h_x10))
    # pairing: p at the next captured tick must equal this temperature
    later = [k for k in ticks if k > 0]
    paired = [k for k in later if observed[k]["p"] == t_x10]
    check(bool(paired),
          "I2C temperature %d x10 equals p at captured tick(s) %s (cross-window pairing)"
          % (t_x10, paired))

# nominal timing from sample indices (4 MS/s, 250 ns/sample)
start_cmd = None
stop_cmd = None
start_rd = None
for line in dec.splitlines():
    m = re.match(r"(\d+)-\d+ i2c-1: Start", line)
    if m:
        if start_cmd is None:
            start_cmd = int(m.group(1))
        else:
            start_rd = int(m.group(1))
    if re.search(r"i2c-1: Stop", line) and stop_cmd is None:
        stop_cmd = int(line.split("-")[0])
if start_cmd is not None and start_rd is not None and stop_cmd is not None:
    gap_us = (start_rd - stop_cmd) / 4.0
    print("nominal command STOP -> read START: %.3f ms" % (gap_us / 1000.0))
    notes.append("command STOP->read START %.3f ms (nominal 4 MS/s)" % (gap_us / 1000.0))
    check(0 < gap_us / 1000.0 < 100, "nominal command-to-read gap is plausible")

print("\n== summary")
print("observed ticks: %s" % ticks)
print("rows: %d; bytes: %d" % (len(all_rows), sum(len(r[1]) + 2 for r in all_rows)))
for n in notes:
    print("NOTE  " + n)
print("\nRESULT: %d passed, %d failed" % (pass_n, fail_n))
sys.exit(0 if fail_n == 0 else 1)
