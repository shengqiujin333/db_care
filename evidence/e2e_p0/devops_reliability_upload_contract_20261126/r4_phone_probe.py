#!/usr/bin/env python
"""Fourth bounded, read-only phone observation for the upload-contract change.

Re-reads the installed App, its own log buffers and its persisted upload state
~7h20m after the 21:59:02 CST platform redeploy. Classification keeps TLS, MQTT
auth and HTTP upload outcomes separate so a healthy TLS layer is never mistaken
for a working upload path.

Read-only: dumpsys/logcat/run-as cat. No input, no install, no clear-data, no
settings change, no reboot.
"""
import os
import re
import subprocess
import time

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "20_r4_phone_window.txt")
ADB = r"D:\software\platform-tools-latest-windows\platform-tools\adb.exe"
SERIAL = "12f046690506"
PKG = "com.jinyuni.dengbei_care"
TLS_SIG = (r"No subjectAltNames|SSLHandshakeException|Trust anchor for certification path "
           r"not found|PKIX path building failed|CertPathValidatorException")


def adb(args, timeout=60):
    try:
        proc = subprocess.run([ADB, "-s", SERIAL] + args, capture_output=True, timeout=timeout)
        return (proc.stdout + proc.stderr).decode("utf-8", "replace")
    except subprocess.TimeoutExpired:
        return "TIMEOUT after %ss" % timeout


def stamped(text):
    rows = []
    for row in text.splitlines():
        m = re.match(r"^\d\d-\d\d (\d\d:\d\d:\d\d)\.\d+", row.strip())
        if m:
            rows.append((m.group(1), row.rstrip()))
    return rows


def main():
    lines = ["# phone-side window, fourth reliability round (read-only)",
             "started_utc=%s" % time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
             "adb=platform-tools serial=%s" % SERIAL,
             "forbidden=install,uninstall,clear-data,settings-change,input,reboot", ""]

    lines.append("--- [device]")
    lines.append(adb(["shell", "date -u +%Y-%m-%dT%H:%M:%SZ; date +%Y-%m-%dT%H:%M:%S%z; "
                              "getprop ro.product.model; getprop ro.build.version.release; "
                              "getprop ro.build.version.sdk"]).rstrip())
    lines.append("")
    lines.append("--- [app_identity]")
    lines.append(adb(["shell", "dumpsys package %s | grep -E "
                              "'versionName|versionCode|lastUpdateTime'" % PKG]).rstrip())
    lines.append("")
    lines.append("--- [app_process]")
    pid = adb(["shell", "pidof %s" % PKG]).strip()
    lines.append("pid=%s" % (pid or "not-running"))
    if pid:
        lines.append(adb(["shell", "ps -o PID,ETIME,ARGS -p %s" % pid]).rstrip())
    lines.append("")
    lines.append("--- [network]")
    lines.append(adb(["shell", "dumpsys connectivity | grep -E 'Active default network' | head -3"]).rstrip())
    lines.append("")

    lines.append("--- [prefs_upload_state]")
    prefs = adb(["shell", "run-as %s cat /data/data/%s/shared_prefs/MyPrefs.xml" % (PKG, PKG)])
    if "<map" in prefs:
        keep = [r.strip() for r in prefs.splitlines()
                if re.search(r"upload|mac_addr|registerphone|measure_state", r)]
        lines.append("\n".join(keep) if keep else "no upload/identity keys present")
    else:
        lines.append("run-as unavailable: %s" % prefs.strip()[:120])
    lines.append("")

    lines.append("--- [crash_buffer]")
    crash = adb(["shell", "logcat -d -b crash -v time -t 200"], timeout=60)
    lines.append("crash_buffer_lines=%d" % len(crash.splitlines()))
    lines.append(crash.strip()[-2500:] if crash.strip() else "(empty)")
    lines.append("")

    lines.append("--- [logcat_buffer_app_tags]")
    snap = adb(["shell", "logcat -d -v time -t 40000 -s MqttService:* CloudUpload:* "
                              "CloudLogUpload:* AndroidRuntime:E"], timeout=120)
    rows = stamped(snap)
    lines.append("buffer_lines=%d oldest=%s newest=%s"
                 % (len(rows), rows[0][0] if rows else "-", rows[-1][0] if rows else "-"))
    lines.append("")
    lines.append("--- [app_log_tail_60]")
    lines.append("\n".join(r[1] for r in rows[-60:]))
    lines.append("")

    joined = "\n".join(r[1] for r in rows)
    tls_rows = [r for r in rows if re.search(TLS_SIG, r[1])]
    auth_rows = [r for r in rows if re.search(r"\u65e0\u6743\u8fde\u63a5 \(5\)|CONNACK.*5", r[1])]
    http401_rows = [r for r in rows if re.search(r"401|AUTH_REJECTED|\u4e91\u7aef\u4e0a\u4f20\u88ab\u62d2\u7edd", r[1])]
    upload_ok = [r for r in rows if re.search(r"upload.*\b20[0-9]\b|UPLOAD_OK|upload ok|\u4e0a\u4f20\u6210\u529f", r[1], re.I)]

    lines.append("## machine checks (device-local time)")
    lines.append("RESULT app_running=%s" % ("YES" if pid else "NO"))
    lines.append("RESULT buffer_entries=%d oldest=%s newest=%s"
                 % (len(rows), rows[0][0] if rows else "-", rows[-1][0] if rows else "-"))
    lines.append("RESULT tls_error_events=%d" % len(set(r[0] for r in tls_rows)))
    lines.append("RESULT connack_not_authorized_events=%d last=%s"
                 % (len(set(r[0] for r in auth_rows)), auth_rows[-1][0] if auth_rows else "-"))
    lines.append("RESULT reconnect_attempts_in_buffer=%d"
                 % len(re.findall(r"Attempting to reconnect to broker", joined)))
    lines.append("RESULT http_401_mentions=%d last=%s"
                 % (len(set(r[0] for r in http401_rows)), http401_rows[-1][0] if http401_rows else "-"))
    lines.append("RESULT http_2xx_upload_mentions=%d" % len(upload_ok))
    lines.append("RESULT crash_lines_in_buffer=%d" % len(re.findall(r"FATAL EXCEPTION", joined)))

    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write("\n".join(lines) + "\n")
    print("\n".join(lines[-10:]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
