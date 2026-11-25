#!/usr/bin/env python
"""CERT8883-REG-03 / E2E-JOINT-P0: read-only platform service baseline.

Reads connection info from the project migration record (never echoed).
Only status/list/log commands; no sudo, no writes, no service control,
no database connection, no write endpoint.
"""
import os
import re
import time
import paramiko

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "17_platform_baseline_ssh.txt")
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
    ("identity", "date -u +%Y-%m-%dT%H:%M:%SZ; hostname; uname -srm"),
    ("units", "for u in emqx user_script; do echo \"$u active=$(systemctl is-active $u 2>&1) enabled=$(systemctl is-enabled $u 2>&1)\"; done; "
              "systemctl show emqx -p ActiveEnterTimestamp -p NRestarts 2>&1"),
    ("emqx_status", "emqx ctl status 2>&1 | head -8"),
    ("emqx_broker", "emqx ctl broker 2>&1 | head -10"),
    ("emqx_listeners", "emqx ctl listeners 2>&1 | head -60"),
    ("emqx_recent_log", "journalctl -u emqx --since '2026-11-25 14:40:00' --no-pager -n 15 2>&1 | tail -15"),
    ("flask_recent_log", "journalctl -u user_script --since '2026-11-25 14:40:00' --no-pager -n 20 2>&1 | tail -20"),
    ("local_8883_check", "timeout 20 openssl s_client -connect 127.0.0.1:8883 -verify_ip 8.140.23.253 "
                         "-CAfile /var/lib/emqx/certs/listeners/ssl/default/cacert-613B9023E1E698F0 </dev/null 2>&1 "
                         "| grep -E 'subject=|issuer=|Verify return code' | head -5"),
]

lines = [
    "# independent read-only platform baseline (software verification)",
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
        _in, out, err = client.exec_command(command, timeout=20)
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
