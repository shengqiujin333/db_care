#!/usr/bin/env python3
"""EV-027 independent re-analysis of the 2026-11-08 post-fix captures.

Inputs (this directory):
  decoded_i2c.txt   sigrok i2c decode of raw_i2c_capture.sr (sample numbers)
  cap01_k75.bin     raw COM43 bytes, capture 1
  cap02_k84_k87.bin raw COM43 bytes, capture 2

No implementation constants are used: CRC-8 is poly 0x31 / init 0xFF /
MSB-first / no reflection / no final xor (Sensirion datasheet form).
Timing is derived from sample numbers and the declared 4 MS/s sample rate.
"""
import re
import sys

SAMPLE_SEC = 0.25e-6  # 4 MS/s


def crc8(data: bytes) -> int:
    crc = 0xFF
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = ((crc << 1) ^ 0x31) & 0xFF if (crc & 0x80) else ((crc << 1) & 0xFF)
    return crc


def parse_decode(path):
    """Return dict of annotations and raw sample spans for the first write/read."""
    ann = []
    with open(path, "r", encoding="utf-8", errors="replace") as fh:
        for line in fh:
            m = re.match(r"\s*(\d+)-(\d+)\s+i2c-\d+:\s+(.*)$", line.rstrip("\n"))
            if m:
                ann.append((int(m.group(1)), int(m.group(2)), m.group(3)))
    return ann


def main():
    ann = parse_decode("decoded_i2c.txt")

    def find(pred, start=0):
        for idx in range(start, len(ann)):
            if pred(ann[idx][2]):
                return idx
        return -1

    texts = [a[2] for a in ann]
    print("== decoded annotation summary ==")
    for i, (s, e, t) in enumerate(ann):
        if t in ("Start", "Stop") or t.startswith("Address") or t.startswith("Data") \
                or t in ("ACK", "NACK"):
            print(f"{s:>8}-{e:<8} {t}")

    starts = [i for i, t in enumerate(texts) if t == "Start"]
    stops = [i for i, t in enumerate(texts) if t == "Stop"]
    if len(starts) < 2 or len(stops) < 2:
        print("ERROR: expected 2 START and 2 STOP")
        return 1
    write_start, read_start = ann[starts[0]][0], ann[starts[1]][0]
    write_stop, read_stop = ann[stops[0]][0], ann[stops[1]][0]

    print("\n== timing ==")
    print(f"write START..STOP      : {write_stop - write_start:>7} samples = "
          f"{(write_stop - write_start) * SAMPLE_SEC * 1e3:.3f} ms")
    print(f"command STOP->read START: {read_start - write_stop:>6} samples = "
          f"{(read_start - write_stop) * SAMPLE_SEC * 1e3:.3f} ms")
    print(f"read START..STOP       : {read_stop - read_start:>7} samples = "
          f"{(read_stop - read_start) * SAMPLE_SEC * 1e3:.3f} ms")

    # bit-cell period from the data-bit annotations of the read phase
    widths = [e - s for s, e, t in ann if t in ("0", "1")]
    if widths:
        from collections import Counter
        common = Counter(widths).most_common(3)
        print(f"data bit-cell widths   : {common} samples -> "
              f"{common[0][0] * SAMPLE_SEC * 1e6:.1f} us "
              f"(~{1.0 / (common[0][0] * SAMPLE_SEC) / 1e3:.3f} kHz)")

    # Address / data bytes
    addr_w = [t for t in texts if t.startswith("Address write")]
    addr_r = [t for t in texts if t.startswith("Address read")]
    data_w = [t for t in texts if t.startswith("Data write")]
    data_r = [t for t in texts if t.startswith("Data read")]
    print("\n== transaction content ==")
    print("address write:", addr_w, " data write:", data_w)
    print("address read :", addr_r, " data read :", data_r)

    frame = bytes(int(x.split(":")[1].strip(), 16) for x in data_r)
    print("raw frame    :", " ".join(f"{b:02X}" for b in frame))
    if len(frame) == 6:
        t_raw = (frame[0] << 8) | frame[1]
        h_raw = (frame[3] << 8) | frame[4]
        t_crc_calc = crc8(frame[0:2])
        h_crc_calc = crc8(frame[3:5])
        temp_c = -45.0 + 175.0 * t_raw / 65535.0
        hum_pct = 100.0 * h_raw / 65535.0
        print(f"T word 0x{t_raw:04X} crc calc 0x{t_crc_calc:02X} recv 0x{frame[2]:02X} "
              f"{'MATCH' if t_crc_calc == frame[2] else 'MISMATCH'} -> {temp_c:.1f} C")
        print(f"H word 0x{h_raw:04X} crc calc 0x{h_crc_calc:02X} recv 0x{frame[5]:02X} "
              f"{'MATCH' if h_crc_calc == frame[5] else 'MISMATCH'} -> {hum_pct:.1f} %RH")
    else:
        print("ERROR: expected 6 data bytes")

    print("\n== serial captures ==")
    for name in ("cap01_k75.bin", "cap02_k84_k87.bin"):
        raw = open(name, "rb").read()
        print(f"-- {name} ({len(raw)} bytes)")
        for line in raw.decode("ascii", errors="replace").split("\r\n"):
            if line:
                print("   ", line)
                fields = dict(re.findall(r"(\w+)=(-?\d+)", line))
                if "p" in fields and "t" in fields:
                    print(f"    p={fields['p']} t={fields['t']} "
                          f"{'p==t (post-update prev field)' if fields['p'] == fields['t'] else 'p!=t'}"
                          f" H={fields.get('H')} q={fields.get('q')}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
