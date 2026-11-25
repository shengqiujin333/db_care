#!/usr/bin/env python
"""Resumed regression window for the non-8883 surfaces touched by the certificate change.

Read-only: unit states, TCP listen ports, HTTP GET status codes of the existing web and
API entry points (no POST, no login, no data write, no database). Kept separate from the
earlier window file so the two observation periods stay comparable.
"""
import os
import re
import time

import paramiko

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
OUT = os.path.join(HERE, "10_service_regression_resume.txt")
PASSWORDS = []


def read_connection():
    path = None
    for name in os.listdir(ROOT):
        if name.startswith("\u670d\u52a1\u5668"):
            path = os.path.join(ROOT, name)
            break
    if not path:
        raise SystemExit("migration record not found")
    with open(path, "r", encoding="utf-8", errors="replace") as fh:
        text = fh.read()
    ip = None
    for row in text.splitlines():
        if "服务器 IP" in row:
            found = re.findall(r"[0-9]{1,3}(?:\.[0-9]{1,3}){3}", row)
            if found:
                ip = found[-1]
                break
    cred = re.search(r"目标服务器：\s*([^\s/]+)\s*/\s*([^\s`]+)", text)
    if not ip or not cred:
        raise SystemExit("connection info not found in project material")
    PASSWORDS.append(cred.group(2))
    return ip, cred.group(1), cred.group(2)


def redact(text):
    for value in PASSWORDS:
        if value:
            text = text.replace(value, "<secret-redacted>")
    return re.sub(r"LTAI[0-9A-Za-z]{12,}", "<secret-redacted>", text)


COMMANDS = [
    ("units", "systemctl is-active emqx user_script 2>&1; "
              "systemctl show emqx user_script -p NRestarts -p ActiveEnterTimestamp 2>&1"),
    ("listen_ports", "ss -ltnH 2>/dev/null | awk '{print $4}' | sed 's/.*://' | sort -n | uniq | tr '\\n' ' '"),
    ("listener_states",
     "emqx ctl listeners 2>&1 | grep -E '^(tcp|ssl|ws|wss):default|running ' | head -20"),
    ("http_codes",
     "for u in http://127.0.0.1:5000/login http://127.0.0.1:5000/upload_data "
     "http://127.0.0.1:18083/status; do "
     "printf '%s -> ' \"$u\"; curl -s -o /dev/null -m 6 -w '%{http_code}\\n' \"$u\"; done"),
    ("flask_recent_errors",
     "journalctl -u user_script --since '2026-11-25 11:00:00' --no-pager 2>/dev/null | "
     "grep -icE 'traceback|error|exception'; echo '---tail---'; "
     "journalctl -u user_script --since '2026-11-25 11:00:00' --no-pager 2>/dev/null | tail -12"),
    ("mysql_alive", "systemctl is-active mysql mariadb 2>&1 | head -3; "
                    "ss -ltnH '( sport = :3306 )' 2>/dev/null | wc -l"),
    ("disk_inode", "df -i / | tail -1; df -h / | tail -1"),
]


def main():
    host, user, password = read_connection()
    lines = ["# resumed non-8883 regression window",
             "started_utc=%s" % time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
             "forbidden=mutation,POST,login,db-query,restart,deploy", ""]
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(host, port=22, username=user, password=password, timeout=20,
                   banner_timeout=20, auth_timeout=20, look_for_keys=False, allow_agent=False)
    lines.append("connect_result=OK")
    outputs = {}
    for label, command in COMMANDS:
        lines.append("--- [%s]" % label)
        try:
            _in, out, err = client.exec_command(command, timeout=30)
            outputs[label] = (out.read() + err.read()).decode("utf-8", "replace")
            lines.append(outputs[label].rstrip())
        except Exception as exc:  # noqa: BLE001
            lines.append("exec_error=%s: %s" % (type(exc).__name__, exc))
        lines.append("")
    client.close()

    ports = outputs.get("listen_ports", "")
    lines.append("## machine checks")
    for port in ("1883", "8883", "8083", "8084", "18083", "5000", "3306"):
        lines.append("RESULT port_%s_open=%s" % (port, "YES" if (" %s " % port) in (" %s " % ports) else "NO"))
    http = outputs.get("http_codes", "")
    lines.append("RESULT flask_login_code=%s"
                 % (re.search(r"/login -> (\d+)", http).group(1) if re.search(r"/login -> (\d+)", http) else "unknown"))
    lines.append("RESULT upload_get_code=%s"
                 % (re.search(r"/upload_data -> (\d+)", http).group(1) if re.search(r"/upload_data -> (\d+)", http) else "unknown"))
    lines.append("RESULT dashboard_status_code=%s"
                 % (re.search(r"/status -> (\d+)", http).group(1) if re.search(r"/status -> (\d+)", http) else "unknown"))
    m = re.search(r"^(\d+)", outputs.get("flask_recent_errors", ""))
    lines.append("RESULT flask_error_lines_since_change=%s" % (m.group(1) if m else "unknown"))
    lines.append("RESULT units_active=%s" % outputs.get("units", "").splitlines()[:2])
    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(redact("\n".join(lines)).rstrip() + "\n")
    print(redact("\n".join(lines[-12:])))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
