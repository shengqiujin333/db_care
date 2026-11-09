#!/usr/bin/env python3
# EV-030 independent analysis of raw COM43 captures and bit-level I2C decode.
# Does NOT import or reference implementation constants; formulas come from the
# GXHT40 datasheet/interface contract (poly 0x31 init 0xFF; x10 conversion).
import hashlib
import os
import re
import sys

BASE = os.path.dirname(os.path.abspath(__file__))

S_FIELDS = ["k", "p", "H", "V", "o", "n", "x", "a", "D", "t", "h", "q", "r", "s", "y"]
G_FIELDS = ["s", "a44", "a45", "rd", "at", "raw"]

CAPTURES = [
    ("cap01_boot_bus_k0.cap", "08:22:36.355", "08:24:36.577"),
    ("cap02_k6.cap", "08:25:02.723", "08:30:02.962"),
    ("cap03_k9_k12.cap", "08:30:17.955", "08:35:18.164"),
    ("cap04_k18.cap", "08:37:28.594", "08:42:28.801"),
]

fail = []


def check(cond, msg):
    print(("PASS  " if cond else "FAIL  ") + msg)
    if not cond:
        fail.append(msg)


def crc8(data, poly=0x31, init=0xFF):
    crc = init
    for b in data:
        crc ^= b
        for _ in range(8):
            if crc & 0x80:
                crc = ((crc << 1) ^ poly) & 0xFF
            else:
                crc = (crc << 1) & 0xFF
    return crc


def parse_kv(tokens):
    out = {}
    order = []
    for t in tokens:
        if "=" not in t:
            continue
        k, v = t.split("=", 1)
        out[k] = v
        order.append(k)
    return out, order


def conv_temp(word):
    return -450 + (1750 * word + 32768) // 65536


def conv_hum(word):
    v = -60 + (1250 * word + 32768) // 65536
    return max(0, min(1000, v))


print("=== EV-030 independent capture analysis ===")
all_s = []
all_g = []
for name, t0, t1 in CAPTURES:
    path = os.path.join(BASE, name)
    raw = open(path, "rb").read()
    print("\n--- %s  window %s..%s  %d bytes  md5=%s" % (
        name, t0, t1, len(raw), hashlib.md5(raw).hexdigest()))
    # Each expected line must end CRLF; split preserving terminator
    parts = raw.split(b"\r\n")
    if parts and parts[-1] == b"":
        parts = parts[:-1]
    for line in parts:
        text = line.decode("ascii", errors="replace")
        nbytes = len(line) + 2
        check(nbytes <= 96, "%s line %dB <= 96B: %s" % (name, nbytes, text[:40]))
        check(b"\x00" not in line, "%s no NUL byte" % name)
        toks = text.split()
        if not toks:
            continue
        if toks[0] == "S":
            kv, order = parse_kv(toks[1:])
            check(order == S_FIELDS, "%s S field set/order %s" % (name, order))
            k = int(kv["k"])
            t = int(kv["t"]); h = int(kv["h"])
            q = int(kv["q"])
            check(-400 <= t <= 1250, "%s S k=%d t=%d in [-400,1250]" % (name, k, t))
            check(0 <= h <= 1000, "%s S k=%d h=%d in [0,1000]" % (name, k, h))
            check(q in (0, 1), "%s S k=%d q in {0,1}" % (name, k))
            all_s.append((name, k, kv))
        elif toks[0] == "G":
            kv, order = parse_kv(toks[1:])
            check(order == G_FIELDS, "%s G field set/order %s" % (name, order))
            all_g.append((name, kv))
        elif toks[0] == "BUS":
            kv, order = parse_kv(toks[1:])
            check(order == ["idle", "scl", "sda", "ack"], "%s BUS field order" % name)
            print("      BUS: %s" % text)
        else:
            check(False, "%s unexpected line: %s" % (name, text[:60]))

print("\n=== S/G pairing and cadence ===")
# pair each S with immediate following G if q=0
seq = []
for name, k, kv in all_s:
    seq.append((k, int(kv["q"]), int(kv["p"]), int(kv["H"]), int(kv["t"]), int(kv["h"])))
seq.sort()
for k, q, p, H, t, h in seq:
    print("  k=%-3d q=%d p=%-4d H=%d t=%-4d h=%d" % (k, q, p, H, t, h))
check(len(all_g) == 0, "no G lines (all observed cycles succeeded)")
ks = [s[0] for s in seq]
for i in range(1, len(ks)):
    check(ks[i] - ks[i - 1] in (3, 6), "k step %d -> %d is multiple of 3" % (ks[i - 1], ks[i]))
# direct F-1 chain on consecutive captured cycles
for i in range(1, len(seq)):
    if seq[i][0] - seq[i - 1][0] == 3:
        prev_t = seq[i - 1][4]
        cur_p = seq[i][2]
        check(cur_p == prev_t, "F-1 chain k=%d p=%d == t(k=%d)=%d" % (
            seq[i][0], cur_p, seq[i - 1][0], prev_t))
first = seq[0]
check(first[0] == 0 and first[3] == 0 and first[2] == 0,
      "first sample k=0 has H=0 p=0")
# at least three consecutive captured cycles with q=1
run = best = 0
for i, s in enumerate(seq):
    if i > 0 and s[0] - seq[i - 1][0] == 3 and s[1] == 1:
        run += 1
    elif s[1] == 1:
        run = 1
    else:
        run = 0
    best = max(best, run)
check(best >= 3, ">=3 consecutive q=1 cycles (best run=%d)" % best)

print("\n=== Bit-level I2C (independent recompute) ===")
dec = open(os.path.join(BASE, "decoded_i2c.txt"), encoding="utf-8").read()
read_bytes = [int(m, 16) for m in re.findall(r"Data read: ([0-9A-F]{2})", dec)]
print("  bytes read: %s" % " ".join("%02X" % b for b in read_bytes))
check(len(read_bytes) == 6, "6 data bytes read")
if len(read_bytes) == 6:
    tw = (read_bytes[0] << 8) | read_bytes[1]
    hw = (read_bytes[3] << 8) | read_bytes[4]
    t_crc = crc8(read_bytes[0:2])
    h_crc = crc8(read_bytes[3:5])
    print("  temp word=0x%04X crc calc=0x%02X recv=0x%02X -> %d (x10)" % (
        tw, t_crc, read_bytes[2], conv_temp(tw)))
    print("  hum  word=0x%04X crc calc=0x%02X recv=0x%02X -> %d (x10)" % (
        hw, h_crc, read_bytes[5], conv_hum(hw)))
    check(t_crc == read_bytes[2], "temperature CRC-8 matches")
    check(h_crc == read_bytes[5], "humidity CRC-8 matches")
    check(-400 <= conv_temp(tw) <= 1250, "decoded temperature in valid range")
    check(0 <= conv_hum(hw) <= 1000, "decoded humidity in valid range")
check("Address write: 44" in dec, "address write 0x44")
check("Data write: FD" in dec, "measure command 0xFD")
check("Address read: 44" in dec, "address read 0x44")
check("NACK" in dec and dec.rstrip().endswith("Stop"), "read ends NACK + Stop")

# timing from sample indices
ann = []
for line in dec.splitlines():
    m = re.match(r"(\d+)-(\d+) i2c-1: (.*)$", line.strip())
    if m:
        ann.append((int(m.group(1)), int(m.group(2)), m.group(3)))
sr = 4_000_000.0
def find(label):
    for a, b, txt in ann:
        if txt == label:
            return a
    return None
start1 = find("Start")
stop1 = find("Stop")
start2 = None
stops = [a for a, b, txt in ann if txt == "Stop"]
starts = [a for a, b, txt in ann if txt == "Start"]
if len(starts) >= 2:
    start2 = starts[1]
if start1 is not None and stop1 is not None and start2 is not None:
    print("  write START->STOP: %.3f ms" % ((stop1 - start1) / sr * 1000))
    print("  command STOP->read START: %.3f ms" % ((start2 - stop1) / sr * 1000))
    if len(stops) >= 2:
        print("  read START->STOP: %.3f ms" % ((stops[1] - start2) / sr * 1000))
# SCL bit period: first full bit annotation
for a, b, txt in ann:
    if txt in ("0", "1") and b > a:
        print("  SCL bit period: %d samples = %.3f us (~%.3f kHz)" % (
            b - a, (b - a) / sr * 1e6, sr / (b - a) / 1000.0))
        break

print("\n=== RESULT: %s ===" % ("ALL CHECKS PASSED" if not fail else "FAILURES: %d" % len(fail)))
for f in fail:
    print("  - " + f)
sys.exit(0 if not fail else 1)
