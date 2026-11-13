#!/usr/bin/env python3
"""EV-040 test-side orchestration: open COM43, HW-reset the sensor through CW-DAPLink,
then keep the same serial window open to observe boot diagnostics and sampling cadence.

Verification asset, not product firmware.  pyocd is used only as an external reset
source (generic Cortex-M target, attach mode); no flash programming and no memory
writes are performed by pyocd.  pyserial owns COM43 for the whole window.

Usage:
  python boot_and_passive_capture.py <port> <pre_reset_s> <total_s> <outfile> [<logfile>]
"""
import datetime
import subprocess
import sys
import time

import serial

PORT = sys.argv[1]
PRE_RESET_S = float(sys.argv[2])
TOTAL_S = float(sys.argv[3])
OUT = sys.argv[4]
LOG = sys.argv[5] if len(sys.argv) > 5 else OUT + ".log"

PROBE = "87094109484987710672FF50"
PYOCD = r"C:\Users\kason\AppData\Roaming\Python\Python311\Scripts\pyocd.exe"

logf = open(LOG, "w", encoding="utf-8", newline="\n")


def log(msg):
    print(msg, flush=True)
    logf.write(msg + "\n")
    logf.flush()


def stamp():
    return datetime.datetime.now(datetime.timezone.utc).strftime("%H:%M:%S.%f")[:-3]


log("%s OPEN %s 9600 8N1 total=%.1fs" % (stamp(), PORT, TOTAL_S))
ser = serial.Serial(PORT, 9600, bytesize=8, parity="N", stopbits=1, timeout=0.05)
ser.reset_input_buffer()
t_open = time.monotonic()
time.sleep(PRE_RESET_S)

log("%s RESET_START method=hw" % stamp())
cmd = [PYOCD, "reset", "-v", "-t", "cortex_m", "-u", PROBE, "-M", "attach", "-m", "hw"]
r = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
for line in (r.stdout or "").splitlines():
    log("  pyocd| " + line)
for line in (r.stderr or "").splitlines():
    log("  pyocd! " + line)
log("%s RESET_DONE rc=%d" % (stamp(), r.returncode))

buf = bytearray()
chunks = []
end = t_open + TOTAL_S
while time.monotonic() < end:
    data = ser.read(4096)
    if data:
        ts = time.monotonic() - t_open
        chunks.append((ts, bytes(data)))
        buf.extend(data)
ser.close()

with open(OUT, "wb") as f:
    f.write(buf)

log("%s CAPTURE_DONE bytes=%d chunks=%d file=%s" % (stamp(), len(buf), len(chunks), OUT))
for ts, data in chunks:
    log("  t+%.3fs %d bytes: %r" % (ts, len(data), data))
logf.close()
