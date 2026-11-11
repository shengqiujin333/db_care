#!/usr/bin/env python3
"""EV-034 independent bit-level I2C analysis from sigrok annotations.

Re-implements CRC-8 (poly 0x31, init 0xFF) and the GXHT40 x10 conversion
from the datasheet; does not import firmware sources.  Sample numbers at the
nominal 4 MS/s give relative time only (no cross-device synchronization).
"""
import io
import re

EV = "D:/mypro/beiwo2/AIP/evidence/ev034"
p = []


def emit(s):
    print(s)
    p.append(s)


def crc8(bs):
    c = 0xFF
    for b in bs:
        c ^= b
        for _ in range(8):
            c = ((c << 1) ^ 0x31) & 0xFF if c & 0x80 else (c << 1) & 0xFF
    return c


def temp_x10(raw):
    return -450 + (1750 * raw + 32768) // 65536


def hum_x10(raw):
    v = -60 + (1250 * raw + 32768) // 65536
    return max(0, min(1000, v))


ann = []
for line in io.open(f"{EV}/decoded_i2c.txt", encoding="utf-8"):
    m = re.match(r"^(\d+)-(\d+) i2c-1: (.*)$", line.strip())
    if m:
        ann.append((int(m.group(1)), int(m.group(2)), m.group(3)))

starts = [a for a in ann if a[2] == "Start"]
stops = [a for a in ann if a[2] == "Stop"]
addr_w = [a for a in ann if a[2].startswith("Address write:")]
addr_r = [a for a in ann if a[2].startswith("Address read:")]
data_w = [a for a in ann if a[2].startswith("Data write:")]
data_r = [a for a in ann if a[2].startswith("Data read:")]
acks = [a for a in ann if a[2] == "ACK"]
nacks = [a for a in ann if a[2] == "NACK"]

emit("=== bit-level I2C (fx2lafw conn=2.32, D0=SCL, D1=SDA, 4 MS/s, 8,000,000 samples) ===")
emit(f"starts={len(starts)} stops={len(stops)} acks={len(acks)} nacks={len(nacks)}")
emit(f"address writes: {[a[2] for a in addr_w]}")
emit(f"command writes: {[a[2] for a in data_w]}")
emit(f"address reads : {[a[2] for a in addr_r]}")

if len(data_r) != 6:
    emit(f"FAIL: expected 6 data bytes, got {len(data_r)}")
else:
    vals = [int(a[2].split(":")[1].strip(), 16) for a in data_r]
    t_raw = (vals[0] << 8) | vals[1]
    h_raw = (vals[3] << 8) | vals[4]
    emit("raw: " + " ".join(f"{v:02X}" for v in vals))
    tc = crc8(vals[0:2])
    hc = crc8(vals[3:5])
    emit(f"temp word 0x{t_raw:04X}: crc calc 0x{tc:02X} == received 0x{vals[2]:02X} -> "
         + ("MATCH" if tc == vals[2] else "MISMATCH")
         + f"; {temp_x10(t_raw)} x10 = {temp_x10(t_raw)/10:.1f} C")
    emit(f"hum  word 0x{h_raw:04X}: crc calc 0x{hc:02X} == received 0x{vals[5]:02X} -> "
         + ("MATCH" if hc == vals[5] else "MISMATCH")
         + f"; {hum_x10(h_raw)} x10 = {hum_x10(h_raw)/10:.1f} %RH")
    ns = 1.0 / 4_000_000.0
    if len(starts) >= 2 and len(stops) >= 2:
        emit(f"write START->STOP      : {(stops[0][1]-starts[0][1])*ns*1000:.3f} ms")
        emit(f"cmd STOP->read START   : {(starts[1][1]-stops[0][1])*ns*1000:.3f} ms")
        emit(f"read START->STOP       : {(stops[1][1]-starts[1][1])*ns*1000:.3f} ms")
    bits = sorted([a for a in ann if re.match(r"^[01]$", a[2])], key=lambda x: x[0])
    if len(bits) > 2:
        d = abs(bits[1][0] - bits[0][0]) * ns
        emit(f"SCL bit interval       : {d*1e6:.2f} us (nominal)")
        emit(f"SCL frequency          : {1.0/d:.2f} Hz (nominal)")
    emit("boundary: digital levels only; no analog amplitude/rise-time claim; "
         "nominal sample time is not synchronized to the COM43 wall clock")
    emit("pairing: the converted value is matched against the later S rows in capture_analysis.txt "
         "(inference, not a same-window capture)")

io.open(f"{EV}/i2c_analysis.txt", "w", encoding="utf-8", newline="\n").write("\n".join(p) + "\n")
print("\nwritten: i2c_analysis.txt")
