#!/usr/bin/env python
"""Observable facts about the 21:59:02 upload-service restart (read-only, bounded).

The Flask service that serves POST /upload_data was restarted at 21:59:02 CST with a
new /root/abr.py (size 8688 vs 8949 recorded by the test round). This probe records
only observable deployment facts - file identity, start time, startup log lines and
whether a pre-change copy exists - so the artifact can state what changed and whether
a rollback point exists. It does not read or judge implementation code.

Read-only: ls/stat/sha256sum/systemctl/journalctl. No sudo, no restart, no write.
"""
import os
import re
import time

import paramiko

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
OUT = os.path.join(HERE, "15_resume3_upload_service_change.txt")
CONNECT_TIMEOUT = 20
CMD_TIMEOUT = 30
PASSWORDS = []


def read_connection():
    path = None
    for name in os.listdir(ROOT):
        if name.startswith("\u670d\u52a1\u5668"):
            path = os.path.join(ROOT, name)
            break
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
        raise SystemExit("connection info not found")
    PASSWORDS.append(cred.group(2))
    return ip, cred.group(1), cred.group(2)


def redact(text):
    for value in PASSWORDS:
        if value:
            text = text.replace(value, "<secret-redacted>")
    return text


def main():
    host, user, password = read_connection()
    lines = ["# upload service change observed in the third reliability window (read-only)",
             "started_utc=%s" % time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()), ""]
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    try:
        client.connect(host, port=22, username=user, password=password,
                       timeout=CONNECT_TIMEOUT, banner_timeout=CONNECT_TIMEOUT,
                       auth_timeout=CONNECT_TIMEOUT, look_for_keys=False, allow_agent=False)
    except Exception as exc:  # noqa: BLE001
        lines.append("connect_result=FAILED %s: %s" % (type(exc).__name__, exc))
        write(lines)
        return 2
    lines.append("connect_result=OK")

    def run(command, timeout=CMD_TIMEOUT):
        _in, out, err = client.exec_command(command, timeout=timeout)
        return out.read().decode("utf-8", "replace") + err.read().decode("utf-8", "replace")

    blocks = [
        ("app_file_identity", "stat -c '%n size=%s mode=%a owner=%U:%G mtime=%y' /root/abr.py; "
                              "sha256sum /root/abr.py | cut -c1-24"),
        ("pre_change_copies", "ls -l --time-style=+%Y-%m-%dT%H:%M:%S /root/abr*.py /root/*abr*.bak 2>/dev/null; "
                              "find /root -maxdepth 1 -name '*abr*' -newermt '2026-11-20' 2>/dev/null; "
                              "find /root -maxdepth 2 -name '*upload*' -o -maxdepth 2 -name '*deploy*' 2>/dev/null | head -5"),
        ("unit_timeline", "systemctl show user_script -p NRestarts -p ActiveEnterTimestamp -p InactiveEnterTimestamp "
                          "-p ExecMainStartTimestamp -p MainPID"),
        ("startup_log", "journalctl -u user_script --since '2026-11-25 21:58:50' --until '2026-11-25 22:00:30' "
                        "--no-pager 2>/dev/null | head -18"),
        ("recent_root_changes", "ls -lt --time-style=+%Y-%m-%dT%H:%M:%S /root | head -12"),
    ]
    try:
        for label, command in blocks:
            lines.append("--- [%s]" % label)
            try:
                lines.append(run(command).rstrip())
            except Exception as exc:  # noqa: BLE001
                lines.append("exec_error=%s: %s" % (type(exc).__name__, exc))
            lines.append("")
    finally:
        client.close()
    write(lines)
    print(redact("\n".join(lines[-12:])))
    return 0


def write(lines):
    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(redact("\n".join(lines)).rstrip() + "\n")


if __name__ == "__main__":
    raise SystemExit(main())
