#!/usr/bin/env python3
"""Test-side orchestration for BOT-01: open COM43 first, then reset the target.

This script is a verification asset (not product firmware).  It opens the
sensor debug UART, waits until the port is definitely receiving, resets the
CW32L010 through the CW-DAPLink with pyocd, and records every received byte
with wall-clock timestamps.

pyocd is used only as a reset source (builtin generic Cortex-M target; the
CW32L010 DFP is not installed).  No flash programming or memory write.

Usage:
  python capture_with_reset.py <port> <delay_s> <post_s> <method> <outfile>
Example:
  python capture_with_reset.py COM43 2 20 hw evidence/ev034/bot01_cap.bin
"""
import datetime
import subprocess
import sys
import time

import serial

port = sys.argv[1]
delay_s = float(sys.argv[2])
post_s = float(sys.argv[3])
method = sys.argv[4]
outfile = sys.argv[5]
PROBE = "87094109484987710672FF50"


def stamp():
    return datetime.datetime.now(datetime.timezone.utc).strftime("%H:%M:%S.%f")[:-3]


ser = serial.Serial(port, 9600, bytesize=8, parity="N", stopbits=1, timeout=0.05)
ser.reset_input_buffer()
t_open = time.monotonic()
print("%s OPEN %s 9600 8N1" % (stamp(), port), flush=True)
time.sleep(delay_s)
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
end = time.monotonic() + post_s
while time.monotonic() < end:
    data = ser.read(4096)
    if data:
        ts = time.monotonic() - t_open
        chunks.append((ts, data))
        buf.extend(data)
ser.close()
with open(outfile, "wb") as f:
    f.write(buf)
print("%s CAPTURE_DONE bytes=%d chunks=%d file=%s" %
      (stamp(), len(buf), len(chunks), outfile), flush=True)
for ts, data in chunks:
    print("  t+%.3fs %d bytes: %r" % (ts, len(data), bytes(data)), flush=True)
