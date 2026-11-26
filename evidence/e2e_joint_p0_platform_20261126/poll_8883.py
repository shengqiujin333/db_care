#!/usr/bin/env python3
"""Poll the phone TCP tables for established connections to 8.140.23.253:8883.

Read-only: adb shell cat /proc/net/tcp{,6}. Records UTC/CST timestamped samples
when a socket to remote port 0x22B3 (8883) is present.
"""
import datetime
import subprocess
import time

ADB = [r"D:\software\platform-tools-latest-windows\platform-tools\adb.exe",
       "-s", "12f046690506", "shell", "cat"]
TARGET_IP_HEX = "08E2178F"  # 8.140.23.253 little-endian in /proc format is 8FE21708; match loosely
OUT = r"D:\mypro\beiwo2\AIP\evidence\e2e_joint_p0_platform_20261126\25b_proc_poll_8883.txt"


def table(name):
    r = subprocess.run(ADB + [name], capture_output=True, timeout=15)
    return r.stdout.decode("utf-8", "replace").splitlines()


def scan(lines):
    hits = []
    for ln in lines:
        parts = ln.split()
        if len(parts) < 4:
            continue
        st = parts[3]
        rem = parts[2]
        if ":" not in rem:
            continue
        port = rem.rsplit(":", 1)[1]
        if st == "01" and port.upper() == "22B3":
            hits.append(ln.strip())
    return hits


def main():
    deadline = time.time() + 400
    samples = 0
    with open(OUT, "w", encoding="utf-8") as fh:
        fh.write("poll_start_utc=%s\n" % datetime.datetime.utcnow().isoformat() + "Z")
        while time.time() < deadline:
            samples += 1
            lines = table("tcp6") + table("tcp")
            hits = scan(lines)
            if hits:
                stamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
                fh.write("--- %s\n" % stamp)
                for h in hits:
                    fh.write(h + "\n")
                fh.flush()
            time.sleep(0.5)
        fh.write("samples=%d\n" % samples)
        fh.write("poll_end_utc=%s\n" % datetime.datetime.utcnow().isoformat() + "Z")
    print("done samples=%d" % samples)


if __name__ == "__main__":
    main()
