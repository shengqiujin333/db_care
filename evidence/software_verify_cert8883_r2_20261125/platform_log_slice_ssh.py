#!/usr/bin/env python
"""CERT8883-UPLOAD-04 round2: bounded read-only slice of platform HTTP-service logs.

Only reads recent journal lines for the project HTTP service; no sudo, no writes,
no service control, no database connection, no write endpoint, no credential echo.
"""
import os
import re
import time
import paramiko

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "27_platform_http_log_slice.txt")
PASSWORDS = []


def read_connection():
    path = None
    for name in os.listdir(ROOT):
        if name.startswith("服务器迁移记录"):
            path = os.path.join(ROOT, name)
            break
    if not path:
        raise SystemExit("migration record not found")
    text = open(path, "r", encoding="utf-8", errors="replace").read()
    ip = None
    for row in text.splitlines():
        if "服务器 IP" in row:
            found = re.findall(r"[0-9]{1,3}(?:\.[0-9]{1,3}){3}", row)
            if found:
                ip = found[-1]
                break
    cred = re.search(r"目标服务器：\s*([^\s/]+)\s*/\s*([^\s`]+)", text)
    if not ip or not cred:
        raise SystemExit("connection info not found")
    PASSWORDS.append(cred.group(2))
    return ip, cred.group(1), cred.group(2)


def redact(text):
    for v in PASSWORDS:
        if v:
            text = text.replace(v, "<secret-redacted>")
    return re.sub(r"LTAI[0-9A-Za-z]{12,}", "<secret-redacted>", text)


COMMANDS = [
    ("identity", "date -u +%Y-%m-%dT%H:%M:%SZ; date '+%Y-%m-%d %H:%M:%S %Z'; hostname"),
    ("http_log_1614_1625",
     "journalctl -u user_script --since '2026-11-25 16:14:00' --until '2026-11-25 16:26:00' "
     "--no-pager -o short-iso 2>&1 | tail -120"),
    ("http_log_post_only",
     "journalctl -u user_script --since '2026-11-25 16:00:00' --no-pager -o short-iso 2>&1 "
     "| grep -E '\"(POST|GET|OPTIONS) ' | tail -60"),
]

lines = [
    "# bounded read-only platform HTTP log slice (software verification, round 2)",
    "started_utc=%s" % time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
    "forbidden=mutation,sudo,restart,deploy,database,write-endpoint,private-key-output",
    "",
]
host, user, password = read_connection()
client = paramiko.SSHClient()
client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
try:
    client.connect(host, port=22, username=user, password=password, timeout=20,
                   banner_timeout=20, auth_timeout=20, look_for_keys=False, allow_agent=False)
    lines.append("connect_result=OK")
except Exception as exc:  # noqa: BLE001
    lines.append("connect_result=FAILED error=%s: %s" % (type(exc).__name__, exc))
    open(OUT, "w", encoding="utf-8").write(redact("\n".join(lines)) + "\n")
    print(redact("\n".join(lines)))
    raise SystemExit(2)

for label, command in COMMANDS:
    lines.append("--- [%s]" % label)
    try:
        _in, out, err = client.exec_command(command, timeout=25)
        text = out.read().decode("utf-8", "replace")
        errtext = err.read().decode("utf-8", "replace")
        code = out.channel.recv_exit_status()
        lines.append("exit=%s" % code)
        if text.strip():
            lines.append(text.rstrip())
        if errtext.strip():
            lines.append("stderr: " + errtext.rstrip())
    except Exception as exc:  # noqa: BLE001
        lines.append("exec_error=%s: %s" % (type(exc).__name__, exc))
    lines.append("")
client.close()

result = redact("\n".join(lines)) + "\n"
open(OUT, "w", encoding="utf-8").write(result)
print(result)
