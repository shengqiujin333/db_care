#!/usr/bin/env python3
"""Independent analysis for firmware feasibility exploration (2026-11-09).

Inputs (all produced in this exploration window):
  serial_capture_200s.bin, serial_capture_220s.bin  - raw COM43 bytes
  decoded_i2c.txt                                    - sigrok i2c annotations
  raw_i2c_capture.sr metadata                        - sampling parameters

This script does NOT import product firmware constants or source. It parses
raw bytes/annotations and recomputes CRC-8, conversion, and timing from the
GXHT40 datasheet rules stated in the project requirements.

Output: capture_analysis.txt (written next to this script).
"""
import math
import re
import statistics
from pathlib import Path

HERE = Path(__file__).resolve().parent


def crc8_gxht(data: bytes) -> int:
    crc = 0xFF
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = ((crc << 1) ^ 0x31) & 0xFF if (crc & 0x80) else ((crc << 1) & 0xFF)
    return crc


def temp_x10(raw: int) -> int:
    return -450 + (1750 * raw + 32768) // 65536


def hum_x10(raw: int) -> int:
    v = -60 + (1250 * raw + 32768) // 65536
    return max(0, min(1000, v))


def parse_s_records(bin_path: Path):
    data = bin_path.read_bytes()
    text = data.decode("ascii", errors="strict")
    lines = text.split("\r\n")
    if lines and lines[-1] == "":
        lines.pop()
    records = []
    for line in lines:
        if not line:
            continue
        fields = {}
        parts = line.split()
        tag = parts[0]
        for p in parts[1:]:
            if "=" in p:
                k, v = p.split("=", 1)
                fields[k] = v
        records.append((len(line) + 2, tag, fields, line))
    return data, records


def parse_i2c(path: Path):
    annotations = []
    pat = re.compile(r"^(\d+)-(\d+) i2c-1: (.*)$")
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        m = pat.match(line.strip())
        if m:
            annotations.append((int(m.group(1)), int(m.group(2)), m.group(3)))
    return annotations


def main():
    out = []
    out.append("== raw serial captures ==")
    all_means = []
    for name in ("serial_capture_200s.bin", "serial_capture_220s.bin"):
        p = HERE / name
        data, records = parse_s_records(p)
        out.append(f"{name}: bytes={len(data)} md5={__import__('hashlib').md5(data).hexdigest()}")
        for length, tag, f, line in records:
            out.append(f"  len={length} tag={tag} {line}")
            if tag == "S":
                out.append(
                    "    fields: k={k} p={p} H={H} V={V} o={o} n={n} x={x} a={a} D={D} "
                    "t={t} h={h} q={q} r={r} s={s} y={y}".format(**f)
                )
                out.append(
                    f"    checks: CRLF=yes len<=96={length <= 96} V/D=({f['V']},{f['D']}) "
                    f"light_valid_gate_expect_V=0 -> {'OK' if f['V'] == '0' else 'UNEXPECTED'}"
                )
                all_means.append(int(f["a"]))
    out.append("")

    out.append("== I2C decode (sigrok, D0=SCL D1=SDA, 4 MS/s, 5.0 s) ==")
    ann = parse_i2c(HERE / "decoded_i2c.txt")
    out.append(f"annotations={len(ann)}")
    for s, e, text in ann:
        if any(x in text for x in ("Address", "Data write", "Data read", "Start", "Stop", "ACK", "NACK")):
            out.append(f"  {s:>7}-{e:<7} {text}")
    writes = [t.split(": ", 1)[1] for _, _, t in ann if t.startswith("Data write:")]
    reads = [t.split(": ", 1)[1] for _, _, t in ann if t.startswith("Data read:")]
    aw = [t.split(": ", 1)[1] for _, _, t in ann if t.startswith("Address write:")]
    ar = [t.split(": ", 1)[1] for _, _, t in ann if t.startswith("Address read:")]
    out.append(f"address_write={aw} data_write={writes} address_read={ar} data_read={reads}")
    if len(reads) == 6 and writes == ["FD"] and aw == ["44"] and ar == ["44"]:
        raw = bytes(int(x, 16) for x in reads)
        t_raw = (raw[0] << 8) | raw[1]
        h_raw = (raw[3] << 8) | raw[4]
        t_crc = crc8_gxht(raw[0:2])
        h_crc = crc8_gxht(raw[3:5])
        out.append(f"raw=({' '.join(f'{b:02X}' for b in raw)})")
        out.append(f"temp: raw={t_raw} calc_crc=0x{t_crc:02X} recv_crc=0x{raw[2]:02X} match={t_crc == raw[2]} -> t_x10={temp_x10(t_raw)}")
        out.append(f"hum : raw={h_raw} calc_crc=0x{h_crc:02X} recv_crc=0x{raw[5]:02X} match={h_crc == raw[5]} -> h_x10={hum_x10(h_raw)}")
    out.append("")

    out.append("== timing (sample index / 4 MS/s; nominal, not host timestamps) ==")
    starts = [s for s, _, t in ann if t == "Start"]
    stops = [s for s, _, t in ann if t == "Stop"]
    if len(starts) >= 2 and len(stops) >= 2:
        out.append(f"write START={starts[0]} STOP={stops[0]} duration={(stops[0]-starts[0])*0.25/1000:.3f} ms")
        out.append(f"read  START={starts[1]} STOP={stops[1]} duration={(stops[1]-starts[1])*0.25/1000:.3f} ms")
        out.append(f"command STOP -> read START = {(starts[1]-stops[0])*0.25/1000:.3f} ms")
    # bit period from consecutive data-bit end samples (write FD and read bytes)
    bit_ends = []
    for s, e, t in ann:
        if re.match(r"^[01]$", t.strip()):
            bit_ends.append(e)
    if len(bit_ends) > 2:
        diffs = [b - a for a, b in zip(bit_ends, bit_ends[1:]) if 0 < b - a < 10000]
        if diffs:
            out.append(f"bit-period samples: median={statistics.median(diffs):.1f} -> {statistics.median(diffs)*0.25:.3f} us "
                       f"(SCL ~{1e6/(statistics.median(diffs)*0.25):.0f} Hz)")
    out.append("")

    out.append("== light reading and the 1/3 dark criterion (hypothetical C_dark) ==")
    out.append("Current firmware has LIGHT_DARK_CALIBRATED=0, so all V=0/D=0 regardless of C_dark.")
    out.append("Table is arithmetic sensitivity analysis, not a calibration result.")
    out.append("mean_adc_code | C_dark | threshold=ceil(C/3) | dark=(3*mean>=C)")
    for c in (4095, 4000, 3915, 3500):
        thr = math.ceil(c / 3)
        for m in all_means:
            out.append(f"{m:13d} | {c:6d} | {thr:19d} | {3*m >= c}")
    out.append("")
    out.append("Maximum C_dark representable by the 12-bit ADC is 4095.")
    for m in all_means:
        out.append(f"mean={m}: not-dark requires C_dark > 3*{m} = {3*m}; "
                   f"possible_with_12bit_C_dark={3*m < 4095}")
    out.append("")
    out.append("Historical light means from prior evidence (same firmware polarity; not re-measured here):")
    out.append("  EV-026 17:43-18:46 local: 3166,3183,3184,3186,3188,3192,3194,3195,3196,3197,3198,3199")
    out.append("  EV-028 19:28-19:59 local: 3249,3304,3279,3299,3298,3221,3335")
    out.append("  EV-029 20:04 local: 3735,3732")
    out.append("  this run 14:33/14:41 local: " + ",".join(str(x) for x in all_means))
    out.append("")
    out.append("NOTE: the current 3914/3915 reading is at the dark end of the designed polarity.")
    out.append("Without a controlled light/dark stimulus and C_dark, the cause (sensor in darkness,")
    out.append("open/unpopulated light input, or ADC/divider fault) cannot be distinguished.")

    text = "\n".join(out) + "\n"
    (HERE / "capture_analysis.txt").write_text(text, encoding="utf-8")
    print(text)


if __name__ == "__main__":
    main()
