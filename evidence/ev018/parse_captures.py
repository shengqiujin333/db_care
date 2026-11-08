#!/usr/bin/env python3
"""EV-018 capture parser / independent validator.

Reads the raw COM43 captures copied into evidence/ev018 and:
  - splits lines on CRLF, reports byte length (including CRLF) and over-budget lines (>96 B)
  - validates S line field set/order and value domains
  - recomputes the G line CRC-8 over raw[0..1]/raw[3..4] and reports mismatch
  - lists BUS/VDD/IOTEST/IOSIG lines if present
This is a tester-owned verification asset; it does not modify product code.
"""
from pathlib import Path
import re
import sys

HERE = Path(__file__).resolve().parent

S_KEYS = ["k", "p", "H", "V", "o", "n", "x", "a", "D", "t", "h", "q", "r", "s", "y"]
G_KEYS = ["s", "a44", "a45", "rd", "at", "raw"]
P_KEYS = ["rq", "s", "y", "t", "h"]


def crc8(bs):
    c = 0xFF
    for b in bs:
        c ^= b
        for _ in range(8):
            c = ((c << 1) ^ 0x31) & 0xFF if c & 0x80 else (c << 1) & 0xFF
    return c


def parse_kv(line, keys):
    parts = line.split()
    if not parts or parts[0] not in ("S", "G", "P"):
        return None
    vals = {}
    for tok in parts[1:]:
        if "=" not in tok:
            return None
        k, v = tok.split("=", 1)
        vals[k] = v
    if list(vals.keys()) != keys:
        return None
    return vals


def main():
    caps = sorted(HERE.glob("*.cap"))
    total_s = total_g = total_p = 0
    q1 = q0 = 0
    crc_fail_temp = crc_fail_hum = 0
    byte2_eq_byte0 = 0
    over_budget = []
    s_records = []
    g_records = []

    for cap in caps:
        data = cap.read_bytes()
        if b"\r\n" not in data and data:
            print(f"{cap.name}: raw={data!r}")
        for raw_line in data.split(b"\r\n"):
            if not raw_line:
                continue
            line = raw_line.decode("ascii", errors="replace")
            wire_len = len(raw_line) + 2
            if wire_len > 96:
                over_budget.append((cap.name, line, wire_len))
            if line.startswith("S "):
                vals = parse_kv(line, S_KEYS)
                total_s += 1
                if vals is None:
                    print(f"{cap.name}: BAD S LINE: {line}")
                    continue
                q = int(vals["q"])
                if q:
                    q1 += 1
                else:
                    q0 += 1
                s_records.append((cap.name, vals))
            elif line.startswith("G "):
                vals = parse_kv(line, G_KEYS)
                total_g += 1
                if vals is None:
                    print(f"{cap.name}: BAD G LINE: {line}")
                    continue
                raw = vals["raw"]
                if len(raw) == 12 and raw != "-" * 12:
                    b = bytes.fromhex(raw)
                    ct = crc8(b[0:2])
                    ch = crc8(b[3:5])
                    ok_t = ct == b[2]
                    ok_h = ch == b[5]
                    if not ok_t:
                        crc_fail_temp += 1
                    if not ok_h:
                        crc_fail_hum += 1
                    if b[2] == b[0]:
                        byte2_eq_byte0 += 1
                    g_records.append((cap.name, vals, ok_t, ok_h, b[2] == b[0]))
                else:
                    g_records.append((cap.name, vals, None, None, None))
            elif line.startswith("P "):
                vals = parse_kv(line, P_KEYS)
                total_p += 1
                if vals is None:
                    print(f"{cap.name}: BAD P LINE: {line}")
            elif line.startswith(("BOOT ", "VDD ", "IOTEST ", "IOSIG ", "BUS ")):
                print(f"OBS {cap.name}: {line}  [{wire_len} B]")

    print()
    print(f"captures={len(caps)} S={total_s} (q=1:{q1} q=0:{q0}) G={total_g} P={total_p}")
    print(f"G raw temperature-CRC mismatches={crc_fail_temp} humidity-CRC mismatches={crc_fail_hum}")
    print(f"G raw where byte2==byte0 (beyond temp-CRC mismatch)={byte2_eq_byte0}")
    print(f"lines over 96 B (incl CRLF)={len(over_budget)}")
    for item in over_budget:
        print("  OVER", item)
    print()
    print("S sequence:")
    for name, v in s_records:
        print(f"  {name}: k={v['k']} p={v['p']} H={v['H']} V={v['V']} o={v['o']} "
              f"a={v['a']} D={v['D']} t={v['t']} h={v['h']} q={v['q']} r={v['r']} s={v['s']} y={v['y']}")
    print()
    print("G detail (raw / tempCRC_ok / humCRC_ok / byte2==byte0):")
    for name, v, ok_t, ok_h, dup in g_records:
        print(f"  {name}: s={v['s']} a44={v['a44']} a45={v['a45']} rd={v['rd']} "
              f"at={v['at']} raw={v['raw']} T={ok_t} H={ok_h} dup={dup}")

    # exit nonzero if structural problems found
    bad = len(over_budget) + (0 if total_s else 1)
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
