#!/usr/bin/env python3
"""EV-022 independent analysis of COM43 raw captures (embedded_tester).

Purpose: verify the CURRENT delivered build (commit 0f8a8fba, axf md5
08a4c266af6a0f058821ec809d38f78d) on the real target after `mdk_flash`.

- Parses every line of every .cap capture produced by the MCP serial_capture
  tool (COM43 @ 9600 8N1, 8N1, raw bytes incl. CRLF).
- Recomputes CRC-8 (poly 0x31, init 0xFF) independently of product constants.
- Checks the EV-018/EV-021 failure signature (raw[2] == raw[0]) and reports
  every deviation.
- Checks line length budget (<= 96 bytes including CRLF) and S field order.
- Does NOT trust any product-side judgement.

Usage: python3 analyze_ev022.py [capture_dir]
"""

import os
import re
import sys
from collections import OrderedDict

S_KEYS = ["k", "p", "H", "V", "o", "n", "x", "a", "D", "t", "h", "q", "r", "s", "y"]
G_KEYS = ["s", "a44", "a45", "rd", "at", "raw"]

CAPTURES = [
    # (file, utc_window, note)
    ("cap01.cap", "05:55:21-05:57:22", "pre-flash old image (7bfd248 build c7ed03b7)"),
    ("cap02.cap", "05:57:45-06:02:46", "after flash 08a4c266"),
    ("cap03.cap", "06:02:53-06:07:53", "after flash 08a4c266"),
    ("cap04.cap", "06:07:59-06:12:59", "after flash 08a4c266"),
    ("cap05.cap", "06:13:04-06:18:04", "after flash 08a4c266"),
    ("cap06.cap", "06:18:09-06:23:09", "after flash 08a4c266"),
    ("cap07.cap", "06:23:30-06:28:30", "after flash 08a4c266"),
    ("cap08.cap", "06:28:36-06:33:36", "after flash 08a4c266"),
    ("cap09.cap", "06:33:43-06:38:44", "after flash 08a4c266"),
    ("cap10.cap", "06:38:49-06:43:49", "after flash 08a4c266"),
    ("cap11.cap", "06:43:55-06:48:55", "after flash 08a4c266"),
    ("cap12.cap", "06:49:12-06:54:12", "after flash 08a4c266"),
    ("cap13.cap", "06:54:19-06:59:19", "after flash 08a4c266"),
]


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


def read_lines(path):
    with open(path, "rb") as f:
        raw = f.read()
    parts = raw.split(b"\r\n")
    return raw, parts


def main(base):
    print("EV-022 independent capture analysis")
    print("build under test: commit 0f8a8fba, axf md5 08a4c266af6a0f058821ec809d38f78d")
    print("decoder: CRC-8 poly=0x31 init=0xFF (independent implementation)")
    print()

    all_s = []
    all_g = []
    bad_lines = []

    for name, window, note in CAPTURES:
        path = os.path.join(base, name)
        raw, parts = read_lines(path)
        print("== %s  %s  %d bytes  (%s)" % (name, window, len(raw), note))
        current_s = None
        for part in parts:
            if not part:
                continue
            line = part + b"\r\n"
            try:
                text = part.decode("ascii")
            except UnicodeDecodeError:
                bad_lines.append((name, part, "non-ascii"))
                print("   [BAD non-ascii] %r" % part)
                continue
            if len(line) > 96:
                bad_lines.append((name, part, "len>%d" % len(line)))
            print("   %3d B  %s" % (len(line), text))
            if text.startswith("S "):
                fields = OrderedDict()
                for tok in text.split()[1:]:
                    if "=" in tok:
                        kv = tok.split("=", 1)
                        fields[kv[0]] = kv[1]
                keys = list(fields.keys())
                if keys != S_KEYS:
                    bad_lines.append((name, part, "S field order %s" % keys))
                rec = {"cap": name, "window": window, "raw_line": text, "fields": fields}
                all_s.append(rec)
                current_s = rec
            elif text.startswith("G "):
                fields = OrderedDict()
                for tok in text.split()[1:]:
                    if "=" in tok:
                        kv = tok.split("=", 1)
                        fields[kv[0]] = kv[1]
                keys = list(fields.keys())
                if keys != G_KEYS:
                    bad_lines.append((name, part, "G field order %s" % keys))
                rec = {"cap": name, "window": window, "raw_line": text, "fields": fields}
                rec["k"] = current_s["fields"].get("k", "?") if current_s else "?"
                all_g.append(rec)
                current_s = None
            elif text.startswith(("BOOT", "VDD", "IOTEST", "IOSIG", "BUS", "P ")):
                pass
            else:
                bad_lines.append((name, part, "unknown line"))
        print()

    print("=" * 78)
    print("S lines (current build only, excluding pre-flash cap01):")
    cur = [r for r in all_s if r["cap"] != "cap01.cap"]
    pre = [r for r in all_s if r["cap"] == "cap01.cap"]
    tp = sum(1 for r in cur if r["fields"].get("q") == "1")
    tf = sum(1 for r in cur if r["fields"].get("q") == "0")
    print("  current build: %d cycles, q=1: %d, q=0: %d (%.1f%% fail)"
          % (len(cur), tp, tf, 100.0 * tf / len(cur) if cur else 0))
    print("  pre-flash image: %d cycles, q=1: %d, q=0: %d"
          % (len(pre), sum(1 for r in pre if r["fields"].get("q") == "1"),
             sum(1 for r in pre if r["fields"].get("q") == "0")))
    print()

    print("G lines (current build):")
    print("  cap     k(from prior S)  raw           T_word  T_crc_calc  T_crc_rx  eq_raw0  H_word  H_crc_calc  H_crc_rx  H_ok")
    # map G to prior S by order within captures
    sig_eq = 0
    sig_tot = 0
    hum_bad = 0
    temp_bad = 0
    for r in all_g:
        if r["cap"] == "cap01.cap":
            continue
        raw_hex = r["fields"].get("raw", "")
        k = r.get("k", "?")
        if len(raw_hex) == 12:
            b = bytes.fromhex(raw_hex)
            t_calc = crc8(b[0:2])
            h_calc = crc8(b[3:5])
            t_ok = (t_calc == b[2])
            h_ok = (h_calc == b[5])
            eq = (b[2] == b[0])
            sig_tot += 1
            if eq:
                sig_eq += 1
            if not t_ok:
                temp_bad += 1
            if not h_ok:
                hum_bad += 1
            print("  %-7s %-15s 0x%02X%02X%02X%02X%02X%02X  0x%04X  0x%02X        0x%02X      %-5s   0x%04X  0x%02X        0x%02X      %s"
                  % (r["cap"], k, b[0], b[1], b[2], b[3], b[4],
                     b[5], (b[0] << 8) | b[1], t_calc, b[2], "YES" if eq else "NO",
                     (b[3] << 8) | b[4], h_calc, b[5], "YES" if h_ok else "NO"))
        else:
            print("  %-7s raw=%s (placeholder/no read)" % (r["cap"], raw_hex))
    print()
    print("  G totals (current build): %d frames, temp CRC bad: %d, humidity CRC bad: %d, raw[2]==raw[0]: %d"
          % (sig_tot, temp_bad, hum_bad, sig_eq))
    print()

    print("Line-budget / format problems: %d" % len(bad_lines))
    for item in bad_lines:
        print("   ", item)

    print()
    print("k sequence (current build):", ", ".join(r["fields"].get("k", "?") for r in cur))
    print("V/D/o per cycle (current build):",
          ", ".join("%s:%s/%s/%s" % (r["fields"].get("k"), r["fields"].get("V"),
                                     r["fields"].get("D"), r["fields"].get("o")) for r in cur))


if __name__ == "__main__":
    base = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.abspath(__file__))
    main(base)
