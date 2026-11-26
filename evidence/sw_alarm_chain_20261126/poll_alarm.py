#!/usr/bin/env python3
"""Poll vibrator/audio state via adb until a vibration appears or timeout.

Test-evidence helper for the alarm-chain run. Read-only dumpsys queries.
"""
import subprocess
import sys
import time
from datetime import datetime, timezone

ADB = r"D:/software/platform-tools-latest-windows/platform-tools/adb.exe"
SERIAL = "12f046690506"
OUT = r"D:/mypro/beiwo2/AIP/evidence/sw_alarm_chain_20261126/39_alarm_poll.txt"


def adb(*args: str) -> str:
    return subprocess.run(
        [ADB, "-s", SERIAL, *args], capture_output=True, text=True, timeout=60
    ).stdout


def main() -> int:
    deadline = time.time() + float(sys.argv[1]) if len(sys.argv) > 1 else time.time() + 180
    with open(OUT, "a", encoding="utf-8") as f:
        while time.time() < deadline:
            vib = adb("shell", "dumpsys", "vibrator")
            line = ""
            for candidate in vib.splitlines():
                if "mCurrentVibration" in candidate:
                    line = candidate.strip()
                    break
            now = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
            f.write(f"{now} | {line}\n")
            f.flush()
            if "mCurrentVibration=null" not in line:
                f.write(f"{now} | ALARM_VIBRATION_ACTIVE\n")
                f.flush()
                return 0
            time.sleep(12)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
