#!/usr/bin/env python3
"""EV-032 independent analysis of the COM43 cycles and the bit-level I2C decode.

Reads only committed raw evidence:
  cap01_k3.cap, cap02_k6_k9.cap, decoded_i2c.txt
Does not reference firmware constants: CRC-8 (poly 0x31, init 0xFF) and the
GXHT40 conversion formulas are re-implemented here from the datasheet.
Sample numbers / nominal 4 MS/s give nominal relative time only.
"""
import io, re, sys, hashlib

EV = "D:/mypro/beiwo2/AIP/evidence/ev032"

def crc8(bs):
    c = 0xFF
    for b in bs:
        c ^= b
        for _ in range(8):
            c = ((c << 1) ^ 0x31) & 0xFF if c & 0x80 else (c << 1) & 0xFF
    return c

def temp_x10(raw):
    # -45 + 175 * S / 65536, x10, round half up (firmware integer form)
    return -450 + (1750 * raw + 32768) // 65536

def hum_x10(raw):
    v = -60 + (1250 * raw + 32768) // 65536
    return max(0, min(1000, v))

out = []
def p(s):
    print(s); out.append(s)

# ---------- serial captures ----------
p("=== COM43 cycles (raw bytes, 9600 8N1) ===")
rows = []
for name in ("cap01_k3.cap", "cap02_k6_k9.cap", "cap03_k15_k18.cap"):
    data = io.open(f"{EV}/{name}", "rb").read()
    p(f"{name}: {len(data)} bytes md5={hashlib.md5(data).hexdigest()}")
    p(f"  hex: {data.hex()}")
    if b"\x00" in data:
        p("  FAIL: NUL byte present")
    lines = data.split(b"\r\n")
    if lines and lines[-1] == b"":
        lines = lines[:-1]
    for ln in lines:
        if len(ln) + 2 > 96:
            p(f"  FAIL: line {len(ln)+2} B > 96")
        txt = ln.decode("ascii", "replace")
        m = re.match(r"^S k=(\d+) p=(-?\d+) H=([01]) V=([01]) o=(\d+) n=(\d+) x=(\d+) a=(\d+) "
                     r"D=([01]) t=(-?\d+) h=(\d+) q=([01]) r=([01]) s=(\d+) y=(\d+)$", txt)
        if not m:
            p(f"  FAIL: S line does not match frozen format: {txt!r}")
            continue
        f = [int(g) for g in m.groups()]
        keys = ["k","p","H","V","o","n","x","a","D","t","h","q","r","s","y"]
        rows.append(dict(zip(keys, f)))
        p(f"  OK  line {len(ln)+2} B: {txt}")

p("")
p("=== per-cycle checks ===")
for r in rows:
    checks = {
        "q==1": r["q"] == 1,
        "V==0 (criterion disabled)": r["V"] == 0,
        "D==0 (criterion disabled)": r["D"] == 0,
        "o==8": r["o"] == 8,
        "a in [n,x]": r["n"] <= r["a"] <= r["x"],
        "t in [-400,1250]": -400 <= r["t"] <= 1250,
        "h in [0,1000]": 0 <= r["h"] <= 1000,
        "r==0/s==0/y==0 (no report)": r["r"] == 0 and r["s"] == 0 and r["y"] == 0,
        "k multiple of 3": r["k"] % 3 == 0,
    }
    bad = [k for k, v in checks.items() if not v]
    p(f"k={r['k']}: " + ("PASS " + ", ".join(checks) if not bad else "FAIL " + ", ".join(bad)))

p("")
p("=== cadence / previous-sample chain ===")
ks = [r["k"] for r in rows]
p(f"k sequence: {ks}")
for a, b in zip(rows, rows[1:]):
    if b["k"] == a["k"] + 3:
        p(f"p(k={b['k']})={b['p']} vs t(k={a['k']})={a['t']}: "
          + ("MATCH" if b["p"] == a["t"] else "MISMATCH"))
        p(f"p(k={b['k']})={b['p']} vs t(k={b['k']})={b['t']}: "
          + ("differs (F-1)" if b["p"] != b["t"] else "equal (F-1 violated)"))
p(f"consecutive k triple: {ks[:3]} -> {'PASS' if ks[:3] == [3,6,9] else 'CHECK'}")
p(f"next captured k: {ks[3:]} (gaps are unobserved windows, not missing samples)")
p(f"light raw a range: {[r['a'] for r in rows]} (recorded only; no light/temperature stimulus applied)")

# ---------- bit-level decode ----------
p("")
p("=== bit-level I2C decode (fx2lafw conn=2.32, D0=SCL, D1=SDA, 4 MS/s) ===")
ann = []
for line in io.open(f"{EV}/decoded_i2c.txt", encoding="utf-8"):
    m = re.match(r"^(\d+)-(\d+) i2c-1: (.*)$", line.strip())
    if m:
        ann.append((int(m.group(1)), int(m.group(2)), m.group(3)))
addr_w = [a for a in ann if a[2].startswith("Address write:")]
data_w = [a for a in ann if a[2].startswith("Data write:")]
addr_r = [a for a in ann if a[2].startswith("Address read:")]
data_r = [a for a in ann if a[2].startswith("Data read:")]
starts = [a for a in ann if a[2] == "Start"]
stops = [a for a in ann if a[2] == "Stop"]
acks = [a for a in ann if a[2] == "ACK"]
nacks = [a for a in ann if a[2] == "NACK"]
p(f"address writes: {[a[2] for a in addr_w]}")
p(f"command writes: {[a[2] for a in data_w]}")
p(f"address reads : {[a[2] for a in addr_r]}")
p(f"data bytes    : {[a[2] for a in data_r]}")
p(f"starts={len(starts)} stops={len(stops)} acks={len(acks)} nacks={len(nacks)}")
if len(data_r) == 6:
    vals = [int(a[2].split(":")[1].strip(), 16) for a in data_r]
    t_raw = (vals[0] << 8) | vals[1]
    h_raw = (vals[3] << 8) | vals[4]
    p("raw: " + " ".join(f"{v:02X}" for v in vals))
    p(f"temp word 0x{t_raw:04X}: crc calc 0x{crc8(vals[0:2]):02X} == received 0x{vals[2]:02X} -> "
      + ("MATCH" if crc8(vals[0:2]) == vals[2] else "MISMATCH")
      + f"; {temp_x10(t_raw)} x10 = {temp_x10(t_raw)/10:.1f} C")
    p(f"hum  word 0x{h_raw:04X}: crc calc 0x{crc8(vals[3:5]):02X} == received 0x{vals[5]:02X} -> "
      + ("MATCH" if crc8(vals[3:5]) == vals[5] else "MISMATCH")
      + f"; {hum_x10(h_raw)} x10 = {hum_x10(h_raw)/10:.1f} %RH")
    # nominal timing (sample index / 4 MS/s)
    ns = 1.0 / 4_000_000.0
    p(f"write START->STOP      : {(stops[0][1]-starts[0][1])*ns*1000:.3f} ms")
    p(f"cmd STOP->read START   : {(starts[1][1]-stops[0][1])*ns*1000:.3f} ms")
    p(f"read START->STOP       : {(stops[1][1]-starts[1][1])*ns*1000:.3f} ms")
    # SCL half period: consecutive bit annotation boundaries in the first data byte
    bits = [a for a in ann if re.match(r"^[01]$", a[2])]
    if len(bits) > 2:
        p(f"SCL bit interval       : {abs(bits[1][0]-bits[0][0])*ns*1e6:.2f} us (nominal)")
else:
    p("FAIL: expected 6 data bytes")

io.open(f"{EV}/capture_analysis.txt", "w", encoding="utf-8", newline="\n").write("\n".join(out) + "\n")
print("\nwritten: capture_analysis.txt")
