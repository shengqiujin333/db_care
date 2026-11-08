#!/usr/bin/env python3
"""EV-023 independent parse of COM43 captures taken after EV-022.

Reads evidence/ev023/capA.cap .. capC.cap, decodes S/G lines, recomputes the
GXHT40 CRC-8 (poly 0x31, init 0xFF, MSB-first) without importing firmware
constants, and reports the failure signature for the live target.

No product source is used; this is a test-side decoder only.
"""
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
CAPS = ["capA.cap", "capB.cap", "capC.cap"]


def crc8(data: bytes) -> int:
    crc = 0xFF
    for b in data:
        crc ^= b
        for _ in range(8):
            if crc & 0x80:
                crc = ((crc << 1) ^ 0x31) & 0xFF
            else:
                crc = (crc << 1) & 0xFF
    return crc


def temp_x10(word: int) -> int:
    return -450 + round(1750 * word / 65536)


def hum_x10(word: int) -> int:
    v = -60 + round(1250 * word / 65536)
    return max(0, min(1000, v))


def parse(path: str):
    raw = open(path, "rb").read()
    text = raw.decode("ascii", errors="replace")
    lines = [ln for ln in text.replace("\r", "").split("\n") if ln]
    return raw, lines


def main():
    total_cycles = 0
    fails = 0
    rows = []
    for name in CAPS:
        path = os.path.join(HERE, name)
        raw, lines = parse(path)
        print("== %s  %d bytes" % (name, len(raw)))
        last_s = None
        for ln in lines:
            print("  %3d B  %s" % (len(ln.encode()) + 2, ln))
            if ln.startswith("S "):
                total_cycles += 1
                fields = dict(re.findall(r"(\w+)=(-?\d+)", ln))
                last_s = fields
                if fields.get("q") == "0":
                    fails += 1
            elif ln.startswith("G "):
                fields = dict(re.findall(r"(\w+)=([0-9A-Fa-f-]+)", ln))
                rawhex = fields.get("raw", "-" * 12)
                if set(rawhex) == {"-"}:
                    continue
                b = bytes.fromhex(rawhex)
                t_word = (b[0] << 8) | b[1]
                h_word = (b[3] << 8) | b[4]
                tcrc = crc8(b[0:2])
                hcrc = crc8(b[3:5])
                rows.append({
                    "k": last_s.get("k") if last_s else "?",
                    "raw": rawhex,
                    "t_word": t_word,
                    "t": temp_x10(t_word),
                    "tcrc_calc": tcrc,
                    "tcrc_rx": b[2],
                    "eq0": b[2] == b[0],
                    "delta_msb": b[2] - b[0],
                    "h_word": h_word,
                    "h": hum_x10(h_word),
                    "hcrc_calc": hcrc,
                    "hcrc_rx": b[5],
                    "h_ok": hcrc == b[5],
                    "s": fields.get("s"),
                    "a44": fields.get("a44"),
                    "a45": fields.get("a45"),
                    "rd": fields.get("rd"),
                    "at": fields.get("at"),
                })
        print()

    print("=== G-frame independent decode (this EV-023 capture) ===")
    print("k   raw          T_word  T(0.1C)  Tcrc_calc Tcrc_rx  eq_raw0  dMSB  H_word  H(0.1%%)  Hcrc_ok  s a44 a45 rd at")
    for r in rows:
        print("%-3s %s  0x%04X  %6d  0x%02X      0x%02X     %-7s %+4d  0x%04X  %6d   %-7s %s %s  %s  %s  %s" % (
            r["k"], r["raw"], r["t_word"], r["t"], r["tcrc_calc"], r["tcrc_rx"],
            "YES" if r["eq0"] else "NO", r["delta_msb"], r["h_word"], r["h"],
            "YES" if r["h_ok"] else "NO", r["s"], r["a44"], r["a45"], r["rd"], r["at"]))
    print()
    print("cycles=%d  q=0 cycles=%d  q=1 cycles=%d" % (total_cycles, fails, total_cycles - fails))
    print("G frames=%d  temp CRC bad=%d  hum CRC bad=%d  raw[2]==raw[0]=%d" % (
        len(rows),
        sum(1 for r in rows if r["tcrc_calc"] != r["tcrc_rx"]),
        sum(1 for r in rows if not r["h_ok"]),
        sum(1 for r in rows if r["eq0"])))


if __name__ == "__main__":
    main()
