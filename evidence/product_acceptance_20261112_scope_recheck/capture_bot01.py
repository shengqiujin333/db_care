#!/usr/bin/env python3
"""BOT-01 automated reassessment harness (test-side, not product firmware).

Purpose: obtain the power-on banner/lines from the current sensor firmware
automatically, without flashing or writing target memory.

Method:
  1. open COM43 (9600 8N1) first so the boot bytes are not missed;
  2. invoke pyocd as an external reset source through the connected CW-DAPLink
     (generic cortex_m target, attach mode, HW reset);
  3. capture the serial stream for a bounded post-reset window;
  4. write raw bytes and a timestamped chunk log, then validate the expected
     five boot lines (BOOT/VDD/IOTEST/IOSIG/BUS) in order.

pyocd cannot identify a CW32-specific target in this environment (probe Target
= n/a), so the generic cortex_m target is used only for the reset request.
No flash programming, no memory write, no option-byte change is performed.

Usage:
  python capture_bot01.py <port> <delay_s> <post_s> <method> <outfile>
Exit code: 0 only when reset rc=0 and all five boot lines are present in order.
"""
import datetime
import subprocess
import sys

import serial

port = sys.argv[1]
delay_s = float(sys.argv[2])
post_s = float(sys.argv[3])
method = sys.argv[4]
outfile = sys.argv[5]
PROBE = "87094109484987710672FF50"
EXPECTED = ["BOOT", "VDD", "IOTEST", "IOSIG", "BUS"]


def stamp():
    return datetime.datetime.now(datetime.timezone.utc).strftime("%H:%M:%S.%f")[:-3]


ser = serial.Serial(port, 9600, bytesize=8, parity="N", stopbits=1, timeout=0.05)
ser.reset_input_buffer()
t_open = __import__("time").monotonic()
print("%s OPEN %s 9600 8N1" % (stamp(), port), flush=True)
__import__("time").sleep(delay_s)
print("%s RESET_START method=%s" % (stamp(), method), flush=True)
cmd = ["pyocd", "reset", "-v", "-t", "cortex_m", "-u", PROBE,
       "-M", "attach", "-m", method]
r = subprocess.run(cmd, capture_output=True, text=True)
print(r.stdout, end="", flush=True)
if r.stderr:
    print(r.stderr, end="", file=sys.stderr, flush=True)
print("%s RESET_DONE rc=%d" % (stamp(), r.returncode), flush=True)

buf = bytearray()
chunks = []
end = __import__("time").monotonic() + post_s
while __import__("time").monotonic() < end:
    data = ser.read(4096)
    if data:
        ts = __import__("time").monotonic() - t_open
        chunks.append((ts, data))
        buf.extend(data)
ser.close()
with open(outfile, "wb") as f:
    f.write(buf)
print("%s CAPTURE_DONE bytes=%d chunks=%d file=%s" %
      (stamp(), len(buf), len(chunks), outfile), flush=True)
for ts, data in chunks:
    print("  t+%.3fs %d bytes: %r" % (ts, len(data), bytes(data)), flush=True)

# Validate the five boot lines appear once each, in order.
text = bytes(buf).decode("ascii", errors="replace")
found = []
for line in text.replace("\r\n", "\n").split("\n"):
    key = line.split(" ", 1)[0].strip()
    if key in EXPECTED and key not in found:
        found.append(key)
print("BOOT_LINES_FOUND=%s" % ",".join(found), flush=True)
ok = r.returncode == 0 and found == EXPECTED
print("BOT01_RESULT=%s" % ("PASS" if ok else "FAIL"), flush=True)
sys.exit(0 if ok else 1)
