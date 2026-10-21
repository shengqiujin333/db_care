#!/usr/bin/env python3
"""Independent real-board parser for the ITEM-008 final-image COM43 capture.

Validates the raw 300 s UART1 bytes (9600 8N1, no parsing done by the tool):
  * only CRLF line endings, no bare LF/CR
  * exactly ONE sampling cycle in the window (no heartbeat / extra sampling)
  * S line fixed field order, k=3 (3 x 1 min RTC ticks)
  * any G line follows its S line immediately
  * every line <= SENSOR_DEBUG_UART_MAXLINE (96) bytes including CRLF
Does not require the sensor to ACK; q=0/G is the known field state and is
reported, not treated as a line-format failure.
"""
from pathlib import Path
import re
import sys

CAP = Path(__file__).resolve().parent / "com43_capture_a.dat"
MAXLINE = 96
raw = CAP.read_bytes()

passes, fails = [], []


def check(name, ok, detail=""):
    (passes if ok else fails).append(name)
    print(f"{'PASS' if ok else 'FAIL'}  {name}" + (f"  [{detail}]" if detail else ""))


check("capture is non-empty", len(raw) > 0, f"{len(raw)} B")
crlf = raw.count(b"\r\n")
lf = raw.count(b"\n")
cr = raw.count(b"\r")
check("no bare CR/LF (every LF preceded by CR, no stray CR)",
      lf == crlf and cr == crlf,
      f"CRLF={crlf} LF={lf} CR={cr}")

try:
    text = raw.decode("ascii")
    check("capture is pure ASCII", True, f"{len(text)} chars")
except UnicodeDecodeError as e:
    check("capture is pure ASCII", False, str(e))
    text = raw.decode("ascii", errors="replace")

lines = text.split("\r\n")
if lines and lines[-1] == "":
    lines.pop()
check("no blank lines", all(ln != "" for ln in lines), f"{len(lines)} line(s)")

encoded = [ln + "\r\n" for ln in lines]
lens = [len(ln.encode()) for ln in encoded]
check("every line <= 96 B including CRLF", all(n <= MAXLINE for n in lens),
      ", ".join(f"{n}B" for n in lens))

s_lines = [ln for ln in lines if ln.startswith("S ")]
g_lines = [ln for ln in lines if ln.startswith("G ")]
check("exactly one S line in the 300 s window (no heartbeat/extra sampling)",
      len(s_lines) == 1, f"{len(s_lines)} S line(s)")
check("no unexpected line prefixes (only S/G; BOOT is power-on only)",
      len(s_lines) + len(g_lines) == len(lines),
      ", ".join(sorted({ln.split(' ')[0] for ln in lines})))

if s_lines:
    s = s_lines[0]
    pat = (r"^S k=(\d+) p=(-?\d+) H=([01]) V=([01]) o=(\d+) n=(\d+) x=(\d+) "
           r"a=(\d+) D=([01]) t=(-?\d+) h=(\d+) q=([01]) r=([01]) s=([012]) y=(\d+)$")
    m = re.match(pat, s)
    check("S line has the fixed 15-field order and value domains", bool(m), s)
    if m:
        check("S.k is 3 (RTC 1 min x 3 ticks)", m.group(1) == "3", m.group(1))
        check("S.q=0 is consistent with the G diagnostic line",
              (m.group(12) == "0") == bool(g_lines), f"q={m.group(12)} G={len(g_lines)}")
        check("S values are within their documented ranges",
              all(int(m.group(i)) <= 4095 for i in (5, 6, 7, 8)) and int(m.group(15)) <= 3,
              "ADC<=4095, y<=3")

if g_lines:
    g = g_lines[0]
    gp = r"^G s=([1-5]) a44=([01]) a45=([01]) rd=(\d+) at=([1-3]) raw=(?:-{12}|[0-9A-F]{12})$"
    check("G line has the fixed field order and raw placeholder/HEX form",
          bool(re.match(gp, g)), g)
    idx = lines.index(g)
    check("G line immediately follows its S line", idx > 0 and lines[idx - 1].startswith("S "),
          f"index {idx}")

print()
print(f"==== result: {len(passes)} passed, {len(fails)} failed ====")
if fails:
    print("failed: " + ", ".join(fails))
sys.exit(1 if fails else 0)
