#!/usr/bin/env python
"""Read-only upload-endpoint outcome summary for the third reliability window."""
import os
import re
import time

import paramiko

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
OUT = os.path.join(HERE, "18_resume3_upload_outcomes.txt")


def main():
    rec = None
    for name in os.listdir(ROOT):
        if name.startswith("\u670d\u52a1\u5668"):
            rec = os.path.join(ROOT, name)
    with open(rec, "r", encoding="utf-8", errors="replace") as fh:
        text = fh.read()
    ip = [f[-1] for f in (re.findall(r"[0-9]{1,3}(?:\.[0-9]{1,3}){3}", r)
                          for r in text.splitlines() if "服务器 IP" in r) if f][0]
    m = re.search(r"目标服务器：\s*([^\s/]+)\s*/\s*([^\s`]+)", text)
    user, pwd = m.group(1), m.group(2)

    cmd = ("journalctl -u user_script --since '2026-11-25 16:00:00' --no-pager 2>/dev/null "
           "| grep -E '\"POST /upload_data HTTP/1.1\"' "
           "| sed -E 's/.*python\\[[0-9]+\\]: //; s/ - - \\[/ /; s/\\].*\"POST \\/upload_data HTTP\\/1.1\" / /' "
           "| awk '{print $1, $2, $3}' | sort | uniq -c | head -40; "
           "echo '---success_total_since_1559Z---'; "
           "journalctl -u user_script --since '2026-11-25 21:59:03' --no-pager 2>/dev/null "
           "| grep -cE '\"POST /upload_data HTTP/1.1\" 2[0-9][0-9]'; "
           "echo '---401_total_window---'; "
           "journalctl -u user_script --since '2026-11-25 16:00:00' --no-pager 2>/dev/null "
           "| grep -cE '\"POST /upload_data HTTP/1.1\" 401'; "
           "echo '---last_401---'; "
           "journalctl -u user_script --since '2026-11-25 16:00:00' --no-pager 2>/dev/null "
           "| grep -E '\"POST /upload_data HTTP/1.1\" 401' | tail -2")

    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(ip, port=22, username=user, password=pwd, timeout=20,
                   banner_timeout=20, auth_timeout=20, look_for_keys=False, allow_agent=False)
    _in, out, err = client.exec_command(cmd, timeout=40)
    body = out.read().decode("utf-8", "replace") + err.read().decode("utf-8", "replace")
    client.close()
    body = body.replace(pwd, "<secret-redacted>")
    header = ["# upload endpoint outcomes 16:00-22:45 CST (read-only journalctl)",
              "started_utc=%s" % time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
              "# each row: count  date  time  status  (POST /upload_data only)", ""]
    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write("\n".join(header) + body.rstrip() + "\n")
    print(body)


if __name__ == "__main__":
    raise SystemExit(main())
