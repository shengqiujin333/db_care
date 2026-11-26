#!/usr/bin/env python3
"""Round-4 CERT8883: bounded read-only phone state re-check.

Collects current device/app identity, prefs, network, local DB snapshot and a
logcat window. No install, no uninstall, no data clear, no prefs edit.
"""
import os
import re
import subprocess
import sys
import time

ADB = r"D:/software/platform-tools-latest-windows/platform-tools/adb.exe"
SERIAL = "12f046690506"
PKG = "com.jinyuni.dengbei_care"
OUT = os.path.dirname(os.path.abspath(__file__))
DB_OUT = os.path.join(OUT, "06_local_db_now.db")


def run(args, timeout=45, binary=False):
    p = subprocess.run([ADB, "-s", SERIAL] + args, capture_output=True,
                       timeout=timeout)
    if binary:
        return p.returncode, p.stdout, p.stderr.decode("utf-8", "replace")
    return p.returncode, p.stdout.decode("utf-8", "replace"), p.stderr.decode("utf-8", "replace")


def section(lines, title, args, timeout=45):
    lines.append("--- [%s]" % title)
    try:
        code, out, err = run(args, timeout=timeout)
        lines.append("exit=%s" % code)
        if out.strip():
            lines.append(out.rstrip())
        if err.strip():
            lines.append("stderr: " + err.strip())
    except Exception as exc:  # noqa: BLE001
        lines.append("exec_error=%s: %s" % (type(exc).__name__, exc))
    lines.append("")


def main():
    lines = [
        "# round-4 bounded read-only phone re-check (software verification)",
        "started_utc=%s" % time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "started_local=%s" % time.strftime("%Y-%m-%d %H:%M:%S"),
        "forbidden=install,uninstall,data-clear,prefs-edit,ui-interaction",
        "",
    ]
    section(lines, "devices", ["devices"])
    section(lines, "time_utc", ["shell", "date -u +%Y-%m-%dT%H:%M:%SZ; date '+%Y-%m-%d %H:%M:%S %Z'"])
    section(lines, "package_version", ["shell", "dumpsys package %s | sed -n '/versionCode/p;/versionName/p;/firstInstallTime/p;/lastUpdateTime/p'" % PKG])
    section(lines, "pm_path", ["shell", "pm path %s" % PKG])
    section(lines, "pid", ["shell", "ps -A -o PID,ELAPSED,NAME | sed -n '1p;/%s/p'" % PKG])
    section(lines, "default_network", ["shell", "ip route; ip -o -4 addr show wlan0"])
    section(lines, "prefs", ["shell", "run-as %s cat shared_prefs/myprefs.xml" % PKG])
    section(lines, "db_list", ["shell", "run-as %s ls -la databases" % PKG])
    section(lines, "crash_buffer", ["logcat", "-b", "crash", "-d", "-t", "120", "-v", "threadtime"], timeout=60)

    # Local DB snapshot through run-as cat (read-only copy; nothing written back).
    lines.append("--- [local_db_snapshot]")
    try:
        code, data, err = run(["exec-out", "run-as", PKG, "cat",
                               "databases/bwtemperature_database1.db"], timeout=60, binary=True)
        lines.append("exit=%s bytes=%d" % (code, len(data)))
        if code == 0 and data:
            open(DB_OUT, "wb").write(data)
            lines.append("saved=%s" % os.path.basename(DB_OUT))
        else:
            lines.append("stderr: " + err.strip())
    except Exception as exc:  # noqa: BLE001
        lines.append("exec_error=%s: %s" % (type(exc).__name__, exc))
    lines.append("")

    # App logcat window, filtered by relevance.
    lines.append("--- [logcat_relevant_tail]")
    try:
        code, out, err = run(["logcat", "-d", "-t", "4000", "-v", "threadtime"], timeout=90)
        keep = re.compile(r"CloudUpload|upload|Upload|uploaded|certificate|SSL|TLS|PKIX|subjectAltNames|FATAL|ANR|DATA_OK|CONNECT_FAILED|Mqtt|CONNACK|无权连接|guard", re.I)
        kept = [ln for ln in out.splitlines() if keep.search(ln)]
        lines.append("exit=%s total_lines=%d kept=%d" % (code, len(out.splitlines()), len(kept)))
        lines.append("\n".join(kept[-400:]))
    except Exception as exc:  # noqa: BLE001
        lines.append("exec_error=%s: %s" % (type(exc).__name__, exc))
    lines.append("")

    result = "\n".join(lines) + "\n"
    open(os.path.join(OUT, "02_phone_state_now.txt"), "w", encoding="utf-8").write(result)
    print(result)


if __name__ == "__main__":
    main()
