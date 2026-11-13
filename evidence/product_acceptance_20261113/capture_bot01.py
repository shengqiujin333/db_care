#!/usr/bin/env python3
"""BOT-01 test-side orchestration: open COM43, request CW-DAPLink HW reset, capture boot lines.

Test-side only; not part of the firmware release package. No flash, no memory write.
"""
import datetime
import subprocess
import sys
import time

import serial

PORT = 'COM43'
BAUD = 9600
CAPTURE_SEC = 30.0
OUT_CAP = 'evidence/product_acceptance_20261113/bot01_boot.cap'
OUT_LOG = 'evidence/product_acceptance_20261113/bot01_boot.log'
OUT_RUN = 'evidence/product_acceptance_20261113/bot01_run.txt'

def stamp():
    return datetime.datetime.now(datetime.timezone.utc).strftime('%Y-%m-%dT%H:%M:%S.%fZ')

def main():
    lines = []
    lines.append('window_open=%s' % stamp())
    ser = serial.Serial(PORT, BAUD, bytesize=8, parity='N', stopbits=1, timeout=0.2)
    raw = bytearray()
    try:
        time.sleep(1.5)
        lines.append('reset_start=%s' % stamp())
        p = subprocess.run(
            ['pyocd', 'reset', '-t', 'cortex_m', '-M', 'attach', '-m', 'hw'],
            capture_output=True, text=True, timeout=30)
        lines.append('reset_rc=%d' % p.returncode)
        lines.append('reset_stdout=%s' % p.stdout.strip().replace('\n', ' | '))
        lines.append('reset_stderr=%s' % p.stderr.strip().replace('\n', ' | '))
        lines.append('reset_end=%s' % stamp())
        end = time.time() + CAPTURE_SEC
        chunk_times = []
        while time.time() < end:
            data = ser.read(256)
            if data:
                raw.extend(data)
                chunk_times.append((stamp(), len(data)))
        lines.append('chunks=%s' % chunk_times)
    finally:
        ser.close()
    lines.append('window_close=%s' % stamp())
    with open(OUT_CAP, 'wb') as f:
        f.write(raw)
    text = raw.decode('ascii', errors='replace').replace('\r\n', '\n')
    with open(OUT_LOG, 'w', encoding='utf-8') as f:
        f.write(text)
    received = [ln for ln in text.replace('\r', '\n').split('\n') if ln.strip()]
    found = [ln.split()[0] for ln in received[:8]]
    expected = ['BOOT', 'VDD', 'IOTEST', 'IOSIG', 'BUS']
    ok = False
    detail = ''
    try:
        idx = [found.index(x) for x in expected]
        ok = idx == sorted(idx) and found[:5] == expected
        detail = 'found=%s' % found
    except ValueError:
        detail = 'found=%s missing_one_of=%s' % (found, expected)
    s_first = next((ln for ln in received if ln.startswith('S ')), None)
    lines.append('bytes=%d' % len(raw))
    lines.append('lines=%s' % received)
    lines.append('%s' % detail)
    lines.append('BOOT_LINES_OK=%s' % ('PASS' if ok else 'FAIL'))
    lines.append('first_S=%s' % s_first)
    lines.append('BOT01_RESULT=%s' % ('PASS' if ok else 'FAIL'))
    with open(OUT_RUN, 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines) + '\n')
    print('\n'.join(lines))
    return 0 if ok else 1

if __name__ == '__main__':
    sys.exit(main())
