#!/usr/bin/env python3
"""EV-021 analysis helper (embedded_tester.embedded_failure_analysis).

Tests alternative byte/CRC models against the six failed G-line frames from
EV-018 to determine whether the temperature-CRC mismatch is explainable by a
protocol/layout/CRC convention difference, or is a byte-duplication anomaly.
Read-only analysis; does not touch product code.
"""
import sys

FRAMES = [
    "67A667543C5D",
    "67176753FA12",
    "65356558B51B",
    "6310635E31FF",
    "6454645B5002",
    "636B635DE85E",
]


def crc8(bs):
    c = 0xFF
    for b in bs:
        c ^= b
        for _ in range(8):
            c = ((c << 1) ^ 0x31) & 0xFF if c & 0x80 else (c << 1) & 0xFF
    return c


def bits(bs):
    return "".join(f"{b:08b}" for b in bs)


def main():
    print("raw            b0 b1 b2(recv) crc(b0b1) crc(b1b0) hum  b5 crc(hum) b2==b0")
    for h in FRAMES:
        b = bytes.fromhex(h)
        print(f"{h} {b[0]:02X} {b[1]:02X} {b[2]:02X}      {crc8(b[0:2]):02X}"
              f"       {crc8(bytes([b[1], b[0]])):02X}      {b[3]:02X}{b[4]:02X} {b[5]:02X} {crc8(b[3:5]):02X}"
              f"      {b[2] == b[0]}")

    print("\nslices whose crc8 == b2 (count over 6 frames):")
    cands = {}
    for h in FRAMES:
        b = bytes.fromhex(h)
        for i in range(6):
            for j in range(i + 1, 7):
                if crc8(b[i:j]) == b[2]:
                    cands[(i, j)] = cands.get((i, j), 0) + 1
    print(cands if cands else "  none")

    print("\nbit-level relation b2 vs b0 / b1 / b3:")
    for h in FRAMES:
        b = bytes.fromhex(h)
        print(f"  {h}: b2={b[2]:08b} b0={b[0]:08b} b1={b[1]:08b} b3={b[3]:08b}"
              f"  b2^b0={b[2] ^ b[0]:02X} b2-b0={b[2]-b[0]}")

    print("\nframe-level: is b2 the previous frame's b0? (successive frames)")
    prev = None
    for h in FRAMES:
        b = bytes.fromhex(h)
        print(f"  {h}: b2={b[2]:02X} prev_b0={prev:02X}" if prev is not None
              else f"  {h}: b2={b[2]:02X} prev_b0=-")
        prev = b[0]

    print("\nphysical plausibility of received words (integer x10 conversion):")
    for h in FRAMES:
        b = bytes.fromhex(h)
        rt = (b[0] << 8) | b[1]
        rh = (b[3] << 8) | b[4]
        t = -450 + round(1750 * rt / 65536)
        hum = -60 + round(1250 * rh / 65536)
        print(f"  {h}: raw_t={rt} -> {t/10:.1f} C, raw_h={rh} -> {hum/10:.1f} %RH")
    return 0


if __name__ == "__main__":
    sys.exit(main())
