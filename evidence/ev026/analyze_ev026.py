#!/usr/bin/env python3
"""EV-026 independent analysis of raw COM43 captures and the I2C logic-analyser decode.

No product constants are imported; CRC-8 and x10 conversion are re-derived from the
datasheet parameters (poly 0x31, init 0xFF; temp = -450 + round(1750*S/65536);
hum = -60 + round(1250*S/65536)).
"""
import os
import re
import sys

BASE = os.path.dirname(os.path.abspath(__file__))

CAP_FILES = [
    "cap00_preflash_old_image.cap",
    "cap01_postflash_bus_k0.cap",
    "cap02_k6_k9.cap",
    "cap03_k12.cap",
    "cap04_k18.cap",
    "cap05_k21_k24.cap",
    "cap06_k27_k30.cap",
    "cap07_k33.cap",
    "cap08_k45.cap",
    "cap09_k48_k51.cap",
    "cap10_k54_k57.cap",
]

S_RE = re.compile(r"^S (.+)$")
G_RE = re.compile(r"^G (.+)$")


def crc8(data):
    crc = 0xFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = ((crc << 1) ^ 0x31) & 0xFF if (crc & 0x80) else (crc << 1) & 0xFF
    return crc


def t_x10(raw):
    return -450 + ((1750 * raw + 32768) >> 16)


def h_x10(raw):
    v = -60 + ((1250 * raw + 32768) >> 16)
    return max(0, min(1000, v))


def kv(text):
    out = {}
    for tok in text.split():
        if "=" in tok:
            k, v = tok.split("=", 1)
            out[k] = v
    return out


def analyze_captures():
    print("=" * 78)
    print("1. COM43 raw capture analysis (bytes, lines, S/G fields, line budget)")
    print("=" * 78)
    all_s = []
    g_rows = []
    for name in CAP_FILES:
        path = os.path.join(BASE, name)
        with open(path, "rb") as f:
            data = f.read()
        print("\n-- %s: %d bytes, md5=%s" % (name, len(data), "see md5_list.txt"))
        # split on CRLF; every transcript byte must be inside a CRLF-terminated line
        parts = data.split(b"\r\n")
        tail = parts.pop()
        if tail:
            print("   WARNING: trailing bytes without CRLF: %r" % tail)
        for raw in parts:
            text = raw.decode("ascii", "replace")
            print("   [%3d B incl CRLF] %s" % (len(raw) + 2, text))
            if len(raw) + 2 > 96:
                print("   FAIL: line exceeds 96 B budget")
            m = S_RE.match(text)
            if m:
                f = kv(m.group(1))
                all_s.append((name, f))
                t = int(f["t"]); h = int(f["h"])
                if not (-400 <= t <= 1250):
                    print("   FAIL: t out of valid range")
                if not (0 <= h <= 1000):
                    print("   FAIL: h out of valid range")
                if f["q"] != "1":
                    print("   NOTE: q=%s in %s" % (f["q"], name))
            m = G_RE.match(text)
            if m:
                g_rows.append((name, kv(m.group(1))))
    print("\n-- S-row summary")
    print("   count=%d" % len(all_s))
    for name, f in all_s:
        print("   %-28s k=%-3s t=%-5s h=%-4s q=%s r=%s s=%s y=%s V=%s D=%s"
              % (name, f["k"], f["t"], f["h"], f["q"], f["r"], f["s"], f["y"],
                 f["V"], f["D"]))
    ks = [int(f["k"]) for _, f in all_s]
    qs = [f["q"] for _, f in all_s]
    print("   k sequence (observed): %s" % ks)
    print("   q=1: %d / %d" % (qs.count("1"), len(qs)))
    print("   G rows: %d" % len(g_rows))
    for name, f in g_rows:
        print("   %s G %s" % (name, f))
    bad_chunks = [(k, q) for k, q in zip(ks, qs) if q != "1"]
    print("   non-q=1 cycles: %s" % (bad_chunks if bad_chunks else "none"))
    # k deltas where consecutive observations are 3 apart
    deltas = [b - a for a, b in zip(ks, ks[1:]) if b > a]
    print("   observed k deltas: %s (all multiples of 3: %s)"
          % (deltas, all(d % 3 == 0 for d in deltas)))
    return all_s, g_rows


def analyze_i2c():
    print("\n" + "=" * 78)
    print("2. I2C bit-level decode (raw annotations, 4 MS/s -> 0.25 us/sample)")
    print("=" * 78)
    path = os.path.join(BASE, "decoded_i2c.txt")
    events = []
    bits = []
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.rstrip("\n")
            m = re.match(r"^(\d+)-(\d+) i2c-1: (.+)$", line)
            if not m:
                continue
            a, b, ann = int(m.group(1)), int(m.group(2)), m.group(3)
            if ann in ("0", "1"):
                bits.append((a, b, ann))
            else:
                events.append((a, ann))
    for s, ann in events:
        print("   sample %7d  %-22s t=%10.3f ms" % (s, ann, s * 0.25 / 1000.0))

    starts = [s for s, ann in events if ann == "Start"]
    stops = [s for s, ann in events if ann == "Stop"]
    write_addr = [ann for _, ann in events if ann.startswith("Address write")]
    read_addr = [ann for _, ann in events if ann.startswith("Address read")]
    data_w = [ann.split(":")[1].strip() for _, ann in events if ann.startswith("Data write")]
    data_r = [ann.split(":")[1].strip() for _, ann in events if ann.startswith("Data read")]
    acks = sum(1 for _, ann in events if ann == "ACK")
    nacks = sum(1 for _, ann in events if ann == "NACK")
    print("\n   transaction: starts=%d stops=%d acks=%d nacks=%d"
          % (len(starts), len(stops), acks, nacks))
    print("   write addr=%s data=%s" % (write_addr, data_w))
    print("   read  addr=%s data=%s" % (read_addr, data_r))

    if len(starts) >= 2 and stops:
        gap = starts[1] - stops[0]
        print("   command STOP sample %d -> read START sample %d = %d samples = %.3f ms"
              % (stops[0], starts[1], gap, gap * 0.25 / 1000.0))
    if len(starts) >= 2:
        wr = stops[0] - starts[0]
        rd = stops[-1] - starts[1]
        print("   write transaction (START->STOP) = %.3f ms"
              % (wr * 0.25 / 1000.0))
        print("   read  transaction (START->STOP) = %.3f ms"
              % (rd * 0.25 / 1000.0))

    # SCL full bit period from consecutive data-bit windows.
    # The decoder emits bit annotations in LIFO order, so sort by start sample first.
    bit_starts = sorted(b[0] for b in bits)
    periods = [b - a for a, b in zip(bit_starts, bit_starts[1:])]
    if periods:
        from collections import Counter
        common = Counter(periods).most_common(5)
        print("   bit windows sorted=%d  period distribution (top5 samples): %s"
              % (len(bit_starts), common))
        dom = common[0][0]
        dom_count = common[0][1]
        print("   dominant SCL bit period = %d samples = %.3f us -> %.1f Hz "
              "(%d/%d of observed periods)" % (dom, dom * 0.25, 1e6 / (dom * 0.25),
                                              dom_count, len(periods)))

    # independent CRC-8 and conversion of the 6 read bytes
    if len(data_r) == 6:
        raw = [int(x, 16) for x in data_r]
        print("\n   raw6 = %s" % " ".join("%02X" % b for b in raw))
        ct = crc8(raw[0:2])
        ch = crc8(raw[3:5])
        print("   temp word=0x%04X crc calc=0x%02X rx=0x%02X -> %s  x10=%d"
              % ((raw[0] << 8) | raw[1], ct, raw[2],
                 "MATCH" if ct == raw[2] else "MISMATCH", t_x10((raw[0] << 8) | raw[1])))
        print("   hum  word=0x%04X crc calc=0x%02X rx=0x%02X -> %s  x10=%d"
              % ((raw[3] << 8) | raw[4], ch, raw[5],
                 "MATCH" if ch == raw[5] else "MISMATCH", h_x10((raw[3] << 8) | raw[4])))
    else:
        print("   FAIL: expected 6 read data bytes, got %d" % len(data_r))


if __name__ == "__main__":
    analyze_captures()
    analyze_i2c()
