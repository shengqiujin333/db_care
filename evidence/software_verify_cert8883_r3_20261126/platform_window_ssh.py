#!/usr/bin/env python3
"""Round-3 CERT8883: bounded read-only platform observation.

Reads migration-record connection info (never echoes credentials). Only runs
read-only commands: identity, systemctl show, stat/sha256 of the deployed handler,
journal slices. No sudo, no write, no restart, no database connection.
"""
import argparse
import os
import re
import time

import paramiko

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


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
    return ip, cred.group(1), cred.group(2)


SECRETS = []


def redact(text):
    for v in SECRETS:
        if v:
            text = text.replace(v, "<secret-redacted>")
    return re.sub(r"LTAI[0-9A-Za-z]{12,}", "<secret-redacted>", text)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--since", default="2026-11-26 05:30:00")
    ap.add_argument("--out", required=True)
    args = ap.parse_args()

    host, user, password = read_connection()
    SECRETS.append(password)

    lines = [
        "# round-3 bounded read-only platform observation (software verification)",
        "started_utc=%s" % time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "forbidden=mutation,sudo,restart,deploy,database,write-endpoint,private-key-output",
        "",
    ]
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    try:
        client.connect(host, port=22, username=user, password=password, timeout=20,
                       banner_timeout=20, auth_timeout=20, look_for_keys=False,
                       allow_agent=False)
        lines.append("connect_result=OK")
    except Exception as exc:  # noqa: BLE001
        lines.append("connect_result=FAILED error=%s: %s" % (type(exc).__name__, exc))
        open(args.out, "w", encoding="utf-8").write(redact("\n".join(lines)) + "\n")
        print(redact("\n".join(lines)))
        raise SystemExit(2)

    commands = [
        ("identity", "date -u +%Y-%m-%dT%H:%M:%SZ; date '+%Y-%m-%d %H:%M:%S %Z'; hostname"),
        ("upload_service",
         "systemctl show user_script -p ActiveState -p SubState -p ExecMainPID "
         "-p NRestarts -p ExecMainStartTimestamp"),
        ("handler_identity",
         "stat -c '%n size=%s mtime=%y inode=%i' /root/abr.py; sha256sum /root/abr.py"),
        ("upload_requests_since",
         "journalctl -u user_script --since '%s' --no-pager -o short-iso 2>&1 "
         "| grep -E 'POST /upload_data|mysql.connect|MySQLdb|Traceback' | tail -80" % args.since),
        ("tail_since",
         "journalctl -u user_script --since '%s' --no-pager -o short-iso 2>&1 | tail -40"
         % args.since),
        ("service_errors_since",
         "journalctl -u user_script -p err --since '%s' --no-pager 2>&1 | tail -20" % args.since),
        ("listeners", "ss -ltnp 2>/dev/null | grep -E ':(5000|8883|1883|3306|18083)\\b'"),
    ]
    for label, command in commands:
        lines.append("--- [%s]" % label)
        try:
            _in, out, err = client.exec_command(command, timeout=30)
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
    open(args.out, "w", encoding="utf-8").write(result)
    print(result)


if __name__ == "__main__":
    main()
