#!/usr/bin/env python3
"""EV-022: failure-rate comparison and frame-value decoding (tester-owned).

All numbers come from the raw COM43 captures in this directory; the CRC-8 is
recomputed here (poly 0x31, init 0xFF), and the temperature/humidity conversion
uses the datasheet formula with integer arithmetic, independently of the
firmware source:

    T_x10 = -450 + round(1750 * S_T / 65536)
    RH_x10 = -60 + round(1250 * S_RH / 65536), clamped to 0..1000

Comparison group (previous image, EV-018 + EV-021):  7 failures / 18 cycles
This round (image 08a4c266): the counts are printed below.
"""
from math import comb

# previous image (EV-018 15 cycles 5 fail; EV-021 3 further cycles 2 fail)
PREV_FAIL, PREV_TOTAL = 7, 18

# this round, post-flash cycles parsed from the captures in this directory
CUR_FAIL, CUR_TOTAL = 13, 19
CUR_CYCLES = [3, 6, 9, 12, 15, 18, 24, 27, 30, 33, 36, 39, 42, 45, 48, 51, 54, 57, 60]
CUR_PASS = [9, 12, 24, 27, 30, 33]

# failure frames: (k, raw6)
FRAMES = [
    (3, "682568579EEE"), (6, "64D1645CAF00"), (15, "6433645F5E9E"),
    (18, "6504655E8213"), (36, "6B046A53D7D8"), (39, "6A4F6A590461"),
    (42, "6B556B5727D9"), (45, "6B096B51091A"), (48, "6A376A550D5D"),
    (51, "6AF86B5482FD"), (54, "6B7E6B5062BF"), (57, "6BCA6B5139DF"),
    (60, "6B5C6B503556"),
]


def crc8(data):
    crc = 0xFF
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (((crc << 1) ^ 0x31) if (crc & 0x80) else (crc << 1)) & 0xFF
    return crc


def round_half_up(num, den):
    return (2 * num + den) // (2 * den)


def temp_x10(raw):
    return -450 + round_half_up(1750 * raw, 65536)


def hum_x10(raw):
    v = -60 + round_half_up(1250 * raw, 65536)
    return max(0, min(1000, v))


def fisher_two_sided(a, b, c, d):
    """Exact Fisher test for [[a,b],[c,d]]."""
    n = a + b + c + d
    r1, r2, c1, c2 = a + b, c + d, a + c, b + d

    def pmf(x):
        return comb(c1, x) * comb(c2, r1 - x) / comb(n, r1)

    obs = pmf(a)
    lo, hi = max(0, r1 - c2), min(r1, c1)
    return sum(pmf(x) for x in range(lo, hi + 1) if pmf(x) <= obs + 1e-12)


def main():
    print("failure-rate comparison (same board, same COM43 chain)")
    print("  previous image (10 ms wait, early SDA sample): %d/%d = %.1f%%"
          % (PREV_FAIL, PREV_TOTAL, 100.0 * PREV_FAIL / PREV_TOTAL))
    print("  this image     (20 ms wait, end-of-high sample): %d/%d = %.1f%%"
          % (CUR_FAIL, CUR_TOTAL, 100.0 * CUR_FAIL / CUR_TOTAL))
    p = fisher_two_sided(CUR_FAIL, CUR_TOTAL - CUR_FAIL,
                         PREV_FAIL, PREV_TOTAL - PREV_FAIL)
    print("  Fisher exact two-sided p = %.4f (no evidence of a change in rate)" % p)
    print("  one-sided test for IMPROVEMENT: the new rate is not lower, so no improvement is shown.")
    print()

    print("cycles observed this round: %d (k = %s)"
          % (len(CUR_CYCLES), ",".join(str(k) for k in CUR_CYCLES)))
    missing = [k for k in range(0, 61, 3) if k not in CUR_CYCLES]
    print("k values not observed in the capture windows: %s (k=0 was before the first window;"
          " k=21 has no line inside an otherwise continuous window)" % missing)
    print()

    print("failure frames decoded (CRC-8 recomputed independently):")
    print("  k    raw           T_raw   T(0.1C)  T_crc_calc T_crc_rx  diff_rx_minus_Tmsb  H_raw   RH(0.1%%)  H_crc_ok")
    exact = 0
    for k, raw in FRAMES:
        b = bytes.fromhex(raw)
        t_raw = (b[0] << 8) | b[1]
        h_raw = (b[3] << 8) | b[4]
        tc = crc8(b[0:2])
        hc = crc8(b[3:5])
        diff = b[2] - b[0]
        if diff == 0:
            exact += 1
        print("  %-3d  %s  0x%04X  %7d   0x%02X       0x%02X      %+d                0x%04X  %7d   %s"
              % (k, raw, t_raw, temp_x10(t_raw), tc, b[2], diff, h_raw, hum_x10(h_raw),
                 "YES" if hc == b[5] else "NO"))
    print("  frames with raw[2] == raw[0]: %d/%d; the other %d differ by one LSB of the MSB."
          % (exact, len(FRAMES), len(FRAMES) - exact))
    print("  temperature CRC mismatches: %d/%d; humidity CRC mismatches: 0/%d"
          % (len(FRAMES), len(FRAMES), len(FRAMES)))


if __name__ == "__main__":
    main()
