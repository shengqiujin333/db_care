#!/usr/bin/env python3
"""Read-only EMQX log slice for the P1 TLS/CONNECT window (credentials redacted)."""
import os
import re
import sys

import paramiko

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def read_connection():
    for name in os.listdir(ROOT):
        if name.startswith("服务器迁移记录"):
            path = os.path.join(ROOT, name)
            break
    else:
        raise SystemExit("migration record not found")
    text = open(path, "r", encoding="utf-8", errors="replace").read()
    ip = None
    for row in text.splitlines():
        if "服务器 IP" in row:
            m = re.findall(r"[0-9]{1,3}(?:\.[0-9]{1,3}){3}", row)
            if m:
                ip = m[-1]
                break
    cred = re.search(r"目标服务器：\s*([^\s/]+)\s*/\s*([^\s`]+)", text)
    return ip, cred.group(1), cred.group(2)


def main():
    out = sys.argv[1]
    since = sys.argv[2] if len(sys.argv) > 2 else "12:53:40"
    host, user, pwd = read_connection()
    secret = pwd
    c = paramiko.SSHClient()
    c.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    c.connect(host, port=22, username=user, password=pwd, timeout=20,
              banner_timeout=20, auth_timeout=20, look_for_keys=False,
              allow_agent=False)
    cmds = [
        "ls -l /var/log/emqx 2>&1 | head -20",
        "for f in /var/log/emqx/emqx.log /var/log/emqx/emqx.log.1; do "
        "if [ -r \"$f\" ]; then echo \"== $f\"; grep -n '2026-11-26T%s' \"$f\" 2>/dev/null | tail -40; fi; done" % since,
        "for f in /var/log/emqx/emqx.log /var/log/emqx/emqx.log.1; do "
        "if [ -r \"$f\" ]; then echo \"== $f authn\"; grep -n 'authentication_failure\\|not_authorized\\|TLS\\|ssl' \"$f\" 2>/dev/null | tail -40; fi; done",
        "journalctl -u emqx --since '2026-11-26 12:53:40' --no-pager -o short-iso 2>&1 | tail -60",
    ]
    lines = ["# read-only emqx window", "since=%s" % since, ""]
    for cmd in cmds:
        lines.append("--- [%s]" % cmd[:60])
        _i, o, e = c.exec_command(cmd, timeout=30)
        t = o.read().decode("utf-8", "replace")
        er = e.read().decode("utf-8", "replace")
        lines.append(t.rstrip())
        if er.strip():
            lines.append("stderr: " + er.rstrip())
        lines.append("")
    c.close()
    text = "\n".join(lines) + "\n"
    text = text.replace(secret, "<secret-redacted>")
    text = re.sub(r"LTAI[0-9A-Za-z]{12,}", "<secret-redacted>", text)
    open(out, "w", encoding="utf-8").write(text)
    print(text)


if __name__ == "__main__":
    main()
