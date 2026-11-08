#!/usr/bin/env python3
"""EV-021 fresh-capture parser / independent validator (embedded_tester).

Parses the COM43 captures taken in this failure-analysis run
(evidence/ev021/run3_a.cap, run3_b.cap) and, for a combined view, the
EV-018 captures (evidence/ev018/*.cap).  For every S/G line it:

  * validates the frozen field set/order and domain where applicable
  * recomputes CRC-8 (poly 0x31, init 0xFF) over raw[0..1] and raw[3..4]
  * reports whether raw[2] equals raw[0] (the byte-duplication signature)
  * reports line length including CRLF

Tester-owned read-only asset; it does not modify product code.
"""
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
EV018 = HERE.parent / "ev018"

S_KEYS = ["k", "p", "H", "V", "o", "n", "x", "a", "D", "t", "h", "q", "r", "s", "y"]
G_KEYS = ["s", "a44", "a45", "rd", "at", "raw"]


def crc8(bs):
    c = 0xFF
    for b in bs:
        c ^= b
        for _ in range(8):
            c = ((c << 1) ^ 0x31) & 0xFF if c & 0x80 else (c << 1) & 0xFF
    return c


def parse_kv(line, keys):
    parts = line.split()
    if not parts or parts[0] not in ("S", "G"):
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


def scan(paths, label):
    caps = 0
    s_lines = g_lines = 0
    q1 = q0 = 0
    temp_bad = hum_bad = 0
    dup = 0
    over = 0
    bad_lines = []
    seq = []
    grecs = []
    for p in paths:
        if not p.exists():
            print(f"{label}: MISSING {p}")
            continue
        caps += 1
        data = p.read_bytes()
        for raw_line in data.split(b"\r\n"):
            if not raw_line:
                continue
            line = raw_line.decode("ascii", errors="replace")
            wire = len(raw_line) + 2
            if wire > 96:
                over += 1
            if line.startswith("S "):
                s_lines += 1
                v = parse_kv(line, S_KEYS)
                if v is None:
                    bad_lines.append((p.name, line))
                    continue
                q1 += int(v["q"])
                q0 += 1 - int(v["q"])
                seq.append((p.name, v))
            elif line.startswith("G "):
                g_lines += 1
                v = parse_kv(line, G_KEYS)
                if v is None:
                    bad_lines.append((p.name, line))
                    continue
                r = v["raw"]
                if len(r) == 12 and r != "-" * 12:
                    b = bytes.fromhex(r)
                    ok_t = crc8(b[0:2]) == b[2]
                    ok_h = crc8(b[3:5]) == b[5]
                    temp_bad += 0 if ok_t else 1
                    hum_bad += 0 if ok_h else 1
                    dup += 1 if b[2] == b[0] else 0
                    grecs.append((p.name, v, ok_t, ok_h, b[2] == b[0], b))
                else:
                    grecs.append((p.name, v, None, None, None, None))
    print(f"== {label}: captures={caps} S={s_lines} (q=1:{q1} q=0:{q0}) "
          f"G={g_lines} lines>96B={over} bad_lines={len(bad_lines)}")
    print(f"   G temp-CRC mismatches={temp_bad} hum-CRC mismatches={hum_bad} "
          f"raw[2]==raw[0]={dup}")
    for name, v in seq:
        print(f"   S {name}: k={v['k']} p={v['p']} t={v['t']} h={v['h']} "
              f"q={v['q']} r={v['r']} s={v['s']} y={v['y']}")
    for name, v, ok_t, ok_h, d, b in grecs:
        print(f"   G {name}: s={v['s']} a44={v['a44']} a45={v['a45']} rd={v['rd']} "
              f"at={v['at']} raw={v['raw']} T_ok={ok_t} H_ok={ok_h} dup={d}"
              + (f" crcT={crc8(b[0:2]):02X}" if b else ""))
    for item in bad_lines:
        print(f"   BAD {item[0]}: {item[1]}")
    return temp_bad, hum_bad, dup, g_lines


def main():
    scan(sorted(HERE.glob("run3_*.cap")), "EV-021 fresh")
    print()
    scan(sorted(EV018.glob("*.cap")), "EV-018 (reference)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
