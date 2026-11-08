#!/usr/bin/env python3
"""EV-025 independent analysis of the 2026-11-08 09:23Z fx2lafw/I2C capture.

Inputs are the decoder annotation sample numbers reproduced from
sigrok_decode decoded.txt (samplerate 4 MHz, 8,000,000 samples = 2.0 s).
This script does not read firmware constants: CRC-8 poly 0x31 / init 0xFF
is implemented independently, and conversions use the datasheet formula.
"""

POLY = 0x31


def crc8(data, init=0xFF):
    c = init
    for b in data:
        c ^= b
        for _ in range(8):
            c = ((c << 1) ^ POLY) & 0xFF if (c & 0x80) else ((c << 1) & 0xFF)
    return c


def main():
    raw = bytes.fromhex("5FC189658860")
    t_raw = (raw[0] << 8) | raw[1]
    h_raw = (raw[3] << 8) | raw[4]
    t_calc = crc8(raw[0:2])
    h_calc = crc8(raw[3:5])
    t_x10 = -450 + round(1750 * t_raw / 65536)
    h_x10 = -60 + round(1250 * h_raw / 65536)

    print("raw bytes               :", raw.hex(" ").upper())
    print("temp word / crc calc/recv: 0x%04X / 0x%02X / 0x%02X  match=%s"
          % (t_raw, t_calc, raw[2], t_calc == raw[2]))
    print("hum  word / crc calc/recv: 0x%04X / 0x%02X / 0x%02X  match=%s"
          % (h_raw, h_calc, raw[5], h_calc == raw[5]))
    print("decoded temp_x10 / hum_x10: %d / %d  (%.1f C, %.1f %%RH)"
          % (t_x10, h_x10, t_x10 / 10.0, h_x10 / 10.0))

    fs = 4_000_000.0
    start_w, stop_w, start_r, stop_r = 15716, 63902, 66389, 226993
    print("\nwrite transaction span  : %.3f ms" % ((stop_w - start_w) / fs * 1000))
    print("command STOP -> read START: %.3f ms" % ((start_r - stop_w) / fs * 1000))
    print("read transaction span   : %.3f ms" % ((stop_r - start_r) / fs * 1000))

    # sigrok I2C decoder emits one contiguous annotation per bit period.
    # Windows below are the 8 data-bit annotations of the first read byte 0x5F.
    bit_windows = [(91159, 93617), (93617, 96074), (96074, 98531),
                   (98531, 100988), (100988, 103445), (103445, 105902),
                   (105902, 108359), (108359, 110816)]
    periods = [(b - a) / fs * 1e9 for a, b in bit_windows]
    mean_period = sum(periods) / len(periods)
    print("\nread bit periods (us)   :", [round(x / 1000.0, 1) for x in periods])
    print("mean bit period         : %.1f us -> nominal SCL %.0f Hz"
          % (mean_period / 1000.0, 1e9 / mean_period))
    # ACK for the read address ends 91017; first data bit starts 91159.
    print("address-ACK -> data gap : %.1f us (edge handling, not a bus stall)"
          % ((91159 - 91017) / fs * 1e6))


if __name__ == "__main__":
    main()
