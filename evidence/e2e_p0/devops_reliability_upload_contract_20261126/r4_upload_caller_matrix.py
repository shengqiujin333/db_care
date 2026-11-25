#!/usr/bin/env python
"""Fourth window, third read-only pass: caller/status matrix for POST /upload_data
and a bounded check that no public origin ever reached the write handler.

Read-only: journalctl parsing only.
"""
import os
import re
import time

import paramiko

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
OUT = os.path.join(HERE, "23_r4_upload_caller_matrix.txt")

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
    PASSWORDS.append(cred.group(2))
    return ip, cred.group(1), cred.group(2)


def redact(text):
    for value in PASSWORDS:
        if value:
            text = text.replace(value, "<secret-redacted>")
    return text


CMD = (
    "echo '---caller_status_since_2159---'; "
    "journalctl -u user_script --since '2026-11-25 21:59:03' --no-pager 2>/dev/null "
    "| grep -E '\"POST /upload_data HTTP/1.1\"' "
    "| sed -E 's/.*python\\[[0-9]+\\]: //' | sort | uniq -c; "
    "echo '---last_upload_request---'; "
    "journalctl -u user_script --since '2026-11-25 21:59:03' --no-pager 2>/dev/null "
    "| grep -E '\"POST /upload_data HTTP/1.1\"' | tail -1; "
    "echo '---first_upload_request_after_redeploy---'; "
    "journalctl -u user_script --since '2026-11-25 21:59:03' --no-pager 2>/dev/null "
    "| grep -E '\"POST /upload_data HTTP/1.1\"' | head -1; "
    "echo '---quiet_hours_since_last_request---'; "
    "date '+now=%Y-%m-%d %H:%M:%S %Z'; "
    "echo '---total_requests_per_hour---'; "
    "journalctl -u user_script --since '2026-11-25 21:59:03' --no-pager 2>/dev/null "
    "| grep -cE '\"(GET|POST|HEAD) '; "
)


def main():
    host, user, password = read_connection()
    lines = ["# fourth window pass 3: upload caller/status matrix (read-only)",
             "started_utc=%s" % time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()), ""]
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(host, port=22, username=user, password=password, timeout=20,
                   banner_timeout=20, auth_timeout=20, look_for_keys=False, allow_agent=False)
    try:
        _in, out, err = client.exec_command(CMD, timeout=60)
        body = out.read().decode("utf-8", "replace") + err.read().decode("utf-8", "replace")
    finally:
        client.close()
    lines.append(body.rstrip())
    lines.append("")
    lines.append("## machine checks")
    rows = re.findall(
        r"^\s*(\d+)\s+(\d+\.\d+\.\d+\.\d+) - - \[[^\]]+\] \"POST /upload_data HTTP/1\.1\" (\d{3})",
        body, re.M)
    codes = sorted({r[2] for r in rows})
    lines.append("RESULT upload_requests=%d" % sum(int(r[0]) for r in rows))
    lines.append("RESULT distinct_callers=%s" % sorted({r[1] for r in rows}))
    lines.append("RESULT status_counts=%s"
                 % {code: sum(int(r[0]) for r in rows if r[2] == code) for code in codes})
    lines.append("RESULT upload_2xx_count=%d" % sum(int(r[0]) for r in rows if r[2].startswith("2")))
    lines.append("RESULT upload_5xx_count=%d" % sum(int(r[0]) for r in rows if r[2].startswith("5")))
    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(redact("\n".join(lines)).rstrip() + "\n")
    print(redact("\n".join(lines)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
