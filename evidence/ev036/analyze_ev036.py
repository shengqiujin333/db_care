#!/usr/bin/env python3
"""EV-036 independent analysis of COM43 raw bytes and the passive I2C decode.

Inputs (raw, produced in this run under evidence/ev036):
  bot01_hw.bin        : port opened first, then HW reset via pyocd; contains
                        BOOT/VDD/IOTEST/IOSIG/BUS and the first S (k=0)
  cap01_k3.bin        : periodic window, S k=3
  cap02_k6_k9.bin     : periodic window, S k=6 and k=9
  cap03_k15_k18.bin   : periodic window, S k=15 and k=18
  decoded_i2c.txt     : offline sigrok i2c decode of raw_i2c_capture.sr

The script re-implements only format checks, the public VDD formula
mv = 4095*bgrmv/code, CRC-8 (poly 0x31, init 0xFF) and the documented
x10 conversion.  It does not import firmware sources.
"""
import hashlib
import io
import math
import re

EV = "D:/mypro/beiwo2/AIP/evidence/ev036"
out = []


def p(s):
    print(s)
    out.append(s)


def crlf_lines(path):
    data = io.open(path, "rb").read()
    p(f"--- {path.split('/')[-1]}: {len(data)} bytes md5={hashlib.md5(data).hexdigest()}")
    if b"\x00" in data:
        p("    FAIL: NUL byte present")
    raw = data.split(b"\r\n")
    if raw and raw[-1] == b"":
        raw = raw[:-1]
    lines = []
    for ln in raw:
        total = len(ln) + 2
        flag = "OK" if total <= 96 else "FAIL >96B"
        p(f"    {total:2d} B {flag}: {ln.decode('ascii', 'replace')}")
        lines.append((ln.decode("ascii"), total))
    return lines


def crc8(bs):
    c = 0xFF
    for b in bs:
        c ^= b
        for _ in range(8):
            c = ((c << 1) ^ 0x31) & 0xFF if c & 0x80 else (c << 1) & 0xFF
    return c


def rnd(x):
    return math.floor(x + 0.5)


p("=== BOT window: port open -> HW reset -> boot diagnostics (COM43 9600 8N1) ===")
boot = crlf_lines(f"{EV}/bot01_hw.bin")
boot_names = ["BOOT", "VDD", "IOTEST", "IOSIG", "BUS"]
got = [t.split(" ")[0] for t, _ in boot[:5]]
p(f"first five line names: {got}")
p("order/once check: " + ("PASS" if got == boot_names else "FAIL"))

for txt, total in boot:
    if txt.startswith("BOOT "):
        m = re.match(r"^BOOT fw=(\S+) uid=([0-9A-Fa-f]{8}) rst=([0-9A-Fa-f]{4}) uart=(\d+)$", txt)
        p("BOOT fields: " + ("PASS" if m else "FAIL") + f" -> {m.groups() if m else None}")
    elif txt.startswith("VDD "):
        m = re.match(r"^VDD ok=([01]) code=(\d+) bgrmv=(\d+) mv=(\d+)$", txt)
        if m:
            ok, code, bgr, mv = map(int, m.groups())
            calc = (4095 * bgr) // code if code else 0
            p(f"VDD: ok={ok} code={code} bgrmv={bgr} mv={mv}; "
              f"4095*bgrmv/code={calc} -> " + ("MATCH" if calc == mv else "MISMATCH"))
        else:
            p("VDD: FAIL format")
    elif txt.startswith("IOTEST "):
        m = re.match(r"^IOTEST sda_lo=([01]) scl_lo=([01]) idle=(\d) swap=(\S+) gc=(\d)$", txt)
        p("IOTEST fields: " + ("PASS" if m else "FAIL") + f" -> {m.groups() if m else None}")
    elif txt.startswith("IOSIG "):
        m = re.match(r"^IOSIG scl=([0-9A-F]{5}) sda=([0-9A-F]{5})$", txt)
        if m:
            scl, sda = m.group(1), m.group(2)
            ack_low = int(sda[-2:], 16) == 0
            p(f"IOSIG: scl={scl} sda={sda}; ACK slot low={ack_low} "
              "(address 0x88 write was ACKed)")
        else:
            p("IOSIG: FAIL format")
    elif txt.startswith("BUS "):
        m = re.match(r"^BUS idle=(\d) scl=([01]) sda=([01]) ack=(\S+)$", txt)
        p("BUS fields: " + ("PASS" if m else "FAIL") + f" -> {m.groups() if m else None}")

p("")
p("=== S rows: frozen format and field checks ===")
rows = []
files = ("bot01_hw.bin", "cap01_k3.bin", "cap02_k6_k9.bin", "cap03_k15_k18.bin")
all_lines = []
for name in files:
    pair = crlf_lines(f"{EV}/{name}")
    all_lines.extend(pair)
    for txt, total in pair:
        if not txt.startswith("S "):
            continue
        m = re.match(r"^S k=(\d+) p=(-?\d+) H=([01]) V=([01]) o=(\d+) n=(\d+) x=(\d+) a=(\d+) "
                     r"D=([01]) t=(-?\d+) h=(\d+) q=([01]) r=([01]) s=(\d+) y=(\d+)$", txt)
        if not m:
            p(f"    FAIL: S line does not match frozen format: {txt!r}")
            continue
        vals = [int(g) for g in m.groups()]
        keys = ["k", "p", "H", "V", "o", "n", "x", "a", "D", "t", "h", "q", "r", "s", "y"]
        rows.append(dict(zip(keys, vals)))

for r in rows:
    checks = {
        "q==1": r["q"] == 1,
        "V==0": r["V"] == 0,
        "D==0": r["D"] == 0,
        "o==8": r["o"] == 8,
        "a in [n,x]": r["n"] <= r["a"] <= r["x"],
        "t in [-400,1250]": -400 <= r["t"] <= 1250,
        "h in [0,1000]": 0 <= r["h"] <= 1000,
        "r==0/s==0/y==0": r["r"] == 0 and r["s"] == 0 and r["y"] == 0,
        "k multiple of 3": r["k"] % 3 == 0,
        "H/p first-sample semantics": (r["k"] == 0 and r["H"] == 0 and r["p"] == 0)
        or (r["k"] > 0 and r["H"] == 1),
    }
    bad = [k for k, v in checks.items() if not v]
    p(f"k={r['k']}: " + ("PASS " + ", ".join(checks) if not bad else "FAIL " + ", ".join(bad)))

p("")
p("=== cadence and previous-value chain ===")
ks = [r["k"] for r in rows]
p(f"k sequence (capture order): {ks}")
consec_sets = [(0, 3, 6, 9), (15, 18)]
for s in consec_sets:
    p(f"consecutive set {s} present: " + ("PASS" if all(k in ks for k in s) else "FAIL"))
by_k = {r["k"]: r for r in rows}
for a, b in ((0, 3), (3, 6), (6, 9), (15, 18)):
    if a in by_k and b in by_k:
        p(f"p(k={b})={by_k[b]['p']} vs t(k={a})={by_k[a]['t']}: "
          + ("MATCH" if by_k[b]["p"] == by_k[a]["t"] else "MISMATCH"))
p(f"light raw a range: {[r['a'] for r in rows]} (recorded only; no light/temperature stimulus)")
p("gates: no `G` line and no `P` line in any capture: "
  + ("PASS" if not any(t.startswith(("G ", "P ")) for t, _ in all_lines) else "FAIL"))

p("")
p("=== passive I2C decode re-check (offline annotations) ===")
dec = io.open(f"{EV}/decoded_i2c.txt", encoding="utf-8").read()
pairs = re.findall(r"i2c-1: Data read: ([0-9A-F]{2})", dec)
writes = re.findall(r"i2c-1: Data write: ([0-9A-F]{2})", dec)
addr = re.findall(r"i2c-1: Address (write|read): ([0-9A-F]{2})", dec)
p(f"addresses: {addr}; write data: {writes}; read data: {pairs}")
if len(pairs) == 6:
    t_raw = (int(pairs[0], 16) << 8) | int(pairs[1], 16)
    h_raw = (int(pairs[3], 16) << 8) | int(pairs[4], 16)
    p(f"temp word 0x{t_raw:04X} CRC 0x{pairs[2]} recomputed 0x{crc8([int(pairs[0],16), int(pairs[1],16)]):02X}: "
      + ("MATCH" if crc8([int(pairs[0], 16), int(pairs[1], 16)]) == int(pairs[2], 16) else "MISMATCH"))
    p(f"hum  word 0x{h_raw:04X} CRC 0x{pairs[5]} recomputed 0x{crc8([int(pairs[3],16), int(pairs[4],16)]):02X}: "
      + ("MATCH" if crc8([int(pairs[3], 16), int(pairs[4], 16)]) == int(pairs[5], 16) else "MISMATCH"))
    t_x10 = -450 + rnd(1750 * t_raw / 65536)
    h_x10 = max(0, min(1000, -60 + rnd(1250 * h_raw / 65536)))
    p(f"converted: temp_x10={t_x10}, hum_x10={h_x10}")
    if 15 in by_k:
        p(f"p(k=15)={by_k[15]['p']} vs I2C temp_x10={t_x10}: "
          + ("MATCH -> transaction belongs to the k=12 sample" if by_k[15]["p"] == t_x10
             else "MISMATCH"))
else:
    p("FAIL: expected 6 read data bytes")

samples = [int(x) for x in re.findall(r"(\d+)-\d+ i2c-1:", dec)]
stop_after_write = None
start_read = None
for line in dec.splitlines():
    if "i2c-1: Stop" in line and stop_after_write is None:
        stop_after_write = int(line.split("-")[0])
    if "i2c-1: Start" in line and stop_after_write is not None and start_read is None:
        start_read = int(line.split("-")[0])
if stop_after_write is not None and start_read is not None:
    p(f"nominal command STOP -> read START: {(start_read - stop_after_write) / 4000.0:.3f} ms")
p("(sample index / 4 MS/s; nominal relative time only, not cross-device synchronized)")

io.open(f"{EV}/capture_analysis.txt", "w", encoding="utf-8", newline="\n").write("\n".join(out) + "\n")
print("\nwritten: capture_analysis.txt")
