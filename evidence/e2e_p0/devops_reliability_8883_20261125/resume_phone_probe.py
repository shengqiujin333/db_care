#!/usr/bin/env python
"""Bounded, read-only phone observation for the post-fix MQTT window.

Reads device identity, the installed App version, the App's own MqttService log
buffer (recent pre-fix + post-fix entries) and a short live window, then
classifies the failure signature by device-local time:
  * before 11:00 CST: expected pre-fix TLS hostname failures
  * from 11:00 CST (certificate reload): TLS vs MQTT CONNACK/auth outcome
No app interaction, no install, no data clear, no settings change, no reboot.
"""
import os
import re
import subprocess
import time

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "09_resume_phone_window.txt")
ADB = r"D:\software\platform-tools-latest-windows\platform-tools\adb.exe"
SERIAL = "12f046690506"
CUTOFF = "11:00:00"  # device-local CST; certificate reloaded on the platform before this
TLS_SIG = (r"No subjectAltNames|SSLHandshakeException|Trust anchor for certification path "
           r"not found|PKIX path building failed")


def adb(args, timeout=60):
    try:
        proc = subprocess.run([ADB, "-s", SERIAL] + args, capture_output=True, timeout=timeout)
        return (proc.stdout + proc.stderr).decode("utf-8", "replace")
    except subprocess.TimeoutExpired as exc:
        return "TIMEOUT after %ss: %r" % (timeout, exc)


def stamped(text):
    rows = []
    for row in text.splitlines():
        m = re.match(r"^\d\d-\d\d (\d\d:\d\d:\d\d)\.\d+", row.strip())
        if m:
            rows.append((m.group(1), row.rstrip()))
    return rows


def classify(rows):
    joined = "\n".join(r[1] for r in rows)
    tls_rows = [r for r in rows if re.search(TLS_SIG, r[1])]
    auth_rows = [r for r in rows if re.search(r"\u65e0\u6743\u8fde\u63a5 \(5\)|Not authorized", r[1])]
    return {
        "entries": len(rows),
        "reconnects": len(re.findall(r"Attempting to reconnect to broker", joined)),
        "tls_lines": len(tls_rows),
        "tls_events": len(set(r[0] for r in tls_rows)),
        "tls_first": tls_rows[0][0] if tls_rows else "-",
        "tls_last": tls_rows[-1][0] if tls_rows else "-",
        "auth_lines": len(auth_rows),
        "auth_events": len(set(r[0] for r in auth_rows)),
        "auth_first": auth_rows[0][0] if auth_rows else "-",
        "auth_last": auth_rows[-1][0] if auth_rows else "-",
        "connected": len(re.findall(r"Connected to broker", joined)),
        "placeholder": len(re.findall(r"Saved MAC address: 000000000000", joined)),
    }


def main():
    lines = ["# phone-side MQTT window after the 8883 certificate fix (read-only)",
             "started_utc=%s" % time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
             "adb=platform-tools serial=%s" % SERIAL,
             "forbidden=install,uninstall,clear-data,settings-change,input,reboot", ""]

    lines.append("--- [device]")
    lines.append(adb(["shell", "date -u +%Y-%m-%dT%H:%M:%SZ; getprop ro.product.model; "
                              "getprop ro.build.version.release; getprop ro.build.version.sdk"]).rstrip())
    lines.append("")
    lines.append("--- [app_identity]")
    lines.append(adb(["shell", "dumpsys package com.jinyuni.dengbei_care | grep -E "
                              "'versionName|versionCode|lastUpdateTime|firstInstallTime'"]).rstrip())
    lines.append("")
    lines.append("--- [app_process]")
    pid = adb(["shell", "pidof com.jinyuni.dengbei_care"]).strip()
    lines.append("pid=%s" % (pid or "not-running"))
    if pid:
        lines.append(adb(["shell", "ps -o PID,ETIME,ARGS -p %s" % pid]).rstrip())
    lines.append("")

    # full retained buffer of this tag (read-only), then a short live window
    lines.append("--- [logcat_buffer_MqttService]")
    snap = adb(["shell", "logcat -d -v time -t 20000 -s MqttService:*"], timeout=90)
    lines.append("buffer_lines=%d" % len(snap.splitlines()))
    lines.append("")

    lines.append("--- [logcat_live_window_20s]")
    t0 = time.time()
    proc = subprocess.Popen([ADB, "-s", SERIAL, "logcat", "-v", "time", "-s", "MqttService:*"],
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    try:
        time.sleep(20)
    finally:
        proc.terminate()
        try:
            out, _ = proc.communicate(timeout=8)
            live = (out or b"").decode("utf-8", "replace")
        except Exception:  # noqa: BLE001
            proc.kill()
            live = ""
    lines.append("window_start=%s window_end=%s"
                 % (time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime(t0)),
                    time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())))
    lines.append("")

    all_rows = stamped(snap) + stamped(live)
    pre_rows = [r for r in all_rows if r[0] < CUTOFF]
    post_rows = [r for r in all_rows if r[0] >= CUTOFF]
    pre_c, post_c = classify(pre_rows), classify(post_rows)

    lines.append("--- [pre_fix_entries] entries=%d oldest=%s newest=%s"
                 % (len(pre_rows), pre_rows[0][0] if pre_rows else "-", pre_rows[-1][0] if pre_rows else "-"))
    lines.append("\n".join(r[1] for r in pre_rows[-25:]))
    lines.append("")
    lines.append("--- [post_fix_entries] entries=%d oldest=%s newest=%s"
                 % (len(post_rows), post_rows[0][0] if post_rows else "-", post_rows[-1][0] if post_rows else "-"))
    post_show = [r[1] for r in post_rows if re.search(TLS_SIG, r[1]) is None]
    lines.append("\n".join(post_show[-45:]))
    lines.append("")

    lines.append("## machine checks (device-local time, cutoff %s CST = certificate reload)" % CUTOFF)
    lines.append("RESULT pre_fix_entries=%d" % pre_c["entries"])
    lines.append("RESULT pre_fix_tls_error_events=%d first=%s last=%s" % (pre_c["tls_events"], pre_c["tls_first"], pre_c["tls_last"]))
    lines.append("RESULT pre_fix_connack_not_authorized_events=%d" % pre_c["auth_events"])
    lines.append("RESULT post_fix_entries=%d" % post_c["entries"])
    lines.append("RESULT post_fix_tls_error_events=%d" % post_c["tls_events"])
    lines.append("RESULT post_fix_connack_not_authorized_events=%d first=%s last=%s"
                 % (post_c["auth_events"], post_c["auth_first"], post_c["auth_last"]))
    lines.append("RESULT post_fix_reconnect_attempts=%d" % post_c["reconnects"])
    lines.append("RESULT post_fix_connected_to_broker=%d" % post_c["connected"])
    lines.append("RESULT post_fix_saved_mac_placeholder=%d" % post_c["placeholder"])

    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write("\n".join(lines) + "\n")
    print("\n".join(lines[-25:]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

