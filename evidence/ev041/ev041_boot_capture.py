#!/usr/bin/env python3
"""Test-side passive fixture: hold COM43 open across a debugger download reset.

Opens the sensor debug UART (9600 8N1) and records raw bytes + per-chunk UTC
timestamps for a bounded duration. It never sends bytes and never touches the
target; the reset itself comes from the registered `mdk_flash` tool running in
parallel (SWD/CMSIS-DAP), so the boot banner and first sample are captured.

Usage: python ev041_boot_capture.py <duration_sec> <out_dir>
"""
import datetime
import os
import sys
import time

import serial

PORT = "COM43"
BAUD = 9600


def main() -> int:
    dur = float(sys.argv[1]) if len(sys.argv) > 1 else 150.0
    outdir = sys.argv[2] if len(sys.argv) > 2 else "."
    os.makedirs(outdir, exist_ok=True)
    raw_path = os.path.join(outdir, "ev041_boot_capture.cap")
    log_path = os.path.join(outdir, "ev041_boot_capture.log")

    started = datetime.datetime.now(datetime.timezone.utc)
    t0 = time.monotonic()
    ser = serial.Serial(PORT, BAUD, bytesize=8, parity="N", stopbits=1, timeout=0.2)
    total = 0
    chunks = 0
    with open(raw_path, "wb") as rf, open(log_path, "w", encoding="utf-8") as lf:
        lf.write(f"OPEN {PORT} {BAUD} 8N1 at {started.isoformat()} dur={dur:.1f}s\n")
        lf.flush()
        try:
            while True:
                el = time.monotonic() - t0
                if el >= dur:
                    break
                data = ser.read(256)
                if data:
                    now = datetime.datetime.now(datetime.timezone.utc)
                    rf.write(data)
                    rf.flush()
                    total += len(data)
                    chunks += 1
                    text = data.decode("latin-1").replace("\r", "\\r").replace("\n", "\\n")
                    lf.write(f"[{el:9.3f}s {now.isoformat()}] {len(data):3d}B: {text}\n")
                    lf.flush()
        finally:
            ser.close()
            lf.write(
                f"CLOSE total={total}B chunks={chunks} elapsed={time.monotonic()-t0:.3f}s "
                f"ended={datetime.datetime.now(datetime.timezone.utc).isoformat()}\n"
            )
    print(f"capture done bytes={total} chunks={chunks} raw={raw_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
