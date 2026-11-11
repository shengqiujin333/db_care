#!/usr/bin/env python3
"""Delayed target reset for BOT-01 capture orchestration.

Waits until the serial capture window is open, then resets the CW32L010 via the
CW-DAPLink using pyocd's builtin generic Cortex-M target (the CW32 DFP is not
installed/registered).  Only a reset is issued: no flash program, no memory
write.  Raw pyocd output is printed for the evidence log.

Usage: python delayed_reset.py [delay_seconds]
"""
import subprocess
import sys
import time

PROBE = "87094109484987710672FF50"
delay = float(sys.argv[1]) if len(sys.argv) > 1 else 3.0
method = sys.argv[2] if len(sys.argv) > 2 else "sysresetreq"
print("delayed_reset: sleeping %.1f s before reset" % delay, flush=True)
time.sleep(delay)
cmd = [
    "pyocd", "reset", "-v", "-t", "cortex_m", "-u", PROBE,
    "-M", "attach", "-m", method,
]
print("delayed_reset: " + " ".join(cmd), flush=True)
r = subprocess.run(cmd, capture_output=True, text=True)
print(r.stdout, end="")
print(r.stderr, end="", file=sys.stderr)
print("delayed_reset: returncode=%d" % r.returncode, flush=True)
sys.exit(r.returncode)
