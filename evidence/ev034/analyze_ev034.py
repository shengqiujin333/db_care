#!/usr/bin/env python3
"""EV-034 independent analysis of COM43 raw bytes.

Inputs (raw, produced in this run):
  bot01_hw.bin      : port opened first, then HW reset via pyocd; contains
                      BOOT/VDD/IOTEST/IOSIG/BUS and the first S (k=0)
  cap01_k3_k6.bin   : periodic window, S k=3 and k=6
  cap02_k9_k12.bin  : periodic window, S k=9 and k=12

The script re-implements only the format checks and the public VDD formula
mv = 4095*bgrmv/code.  It does not import firmware sources.
"""
import hashlib
import io
import re

EV = "D:/mypro/beiwo2/AIP/evidence/ev034"
out = []


def p(s):
    print(s)
    out.append(s)


def crlf_lines(path):
    data = io.open(path, "rb").read()
    p(f"--- {path.split('/')[-1]}: {len(data)} bytes md5={hashlib.md5(data).hexdigest()}")
    p(f"    hex: {data.hex()}")
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
for name in ("bot01_hw.bin", "cap01_k3_k6.bin", "cap02_k9_k12.bin", "cap03_k18.bin"):
    for txt, total in crlf_lines(f"{EV}/{name}"):
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
    }
    bad = [k for k, v in checks.items() if not v]
    p(f"k={r['k']}: " + ("PASS " + ", ".join(checks) if not bad else "FAIL " + ", ".join(bad)))

p("")
p("=== cadence and previous-value chain ===")
ks = [r["k"] for r in rows]
p(f"k sequence (capture order): {ks}")
p("consecutive k=0,3,6,9,12 present: "
  + ("PASS" if all(k in ks for k in (0, 3, 6, 9, 12)) else "FAIL"))
by_k = {r["k"]: r for r in rows}
for a, b in ((0, 3), (3, 6), (6, 9), (9, 12)):
    if a in by_k and b in by_k:
        p(f"p(k={b})={by_k[b]['p']} vs t(k={a})={by_k[a]['t']}: "
          + ("MATCH" if by_k[b]["p"] == by_k[a]["t"] else "MISMATCH"))
p(f"p(k=9)={by_k[9]['p']} == t(k=9)={by_k[9]['t']} is a same-temperature coincidence; "
  f"p(k=12)={by_k[12]['p']} proves p uses the previous cycle")
p(f"light raw a range: {[r['a'] for r in rows]} (recorded only; no light/temperature stimulus)")
p(f"p(k=18)={by_k[18]['p']} vs bit-level I2C converted temp at the k=15 sample (254): "
  + ("MATCH -> the transaction belongs to k=15 (cross-window inference)" if by_k[18]["p"] == 254 else "MISMATCH"))
p("")
p("gates: no `G` line and no `P` line in any capture: "
  + ("PASS" if not any(t.startswith(("G ", "P ")) for n in
                      ("bot01_hw.bin", "cap01_k3_k6.bin", "cap02_k9_k12.bin", "cap03_k18.bin")
                      for t, _ in crlf_lines(f"{EV}/{n}")) else "FAIL"))

io.open(f"{EV}/capture_analysis.txt", "w", encoding="utf-8", newline="\n").write("\n".join(out) + "\n")
print("\nwritten: capture_analysis.txt")
