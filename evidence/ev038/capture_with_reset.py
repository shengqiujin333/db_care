#!/usr/bin/env python3
"""Test-side BOT-01 orchestration for EV-038: open COM43 first, then reset the target.

Verification asset, not product firmware.  pyocd is used only as an external
reset source through the CW-DAPLink (generic Cortex-M target, attach mode); no
flash programming and no memory writes are performed by pyocd.

Usage:
  python capture_with_reset.py <port> <delay_s> <post_s> <method> <outfile>
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
