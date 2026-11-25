#!/usr/bin/env python
"""Context of the 21:59 upload-service redeploy (read-only, bounded).

Lists the deploy directory created by the implementation role at 21:58:55 CST and
prints only the tail of its deploy/verification logs, so the reliability artifact can
cite what version was deployed and which checks the deployer itself ran. Secrets are
redacted. No file, service or data change.
"""
import os
import re

import paramiko

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
OUT = os.path.join(HERE, "16_resume3_deploy_context.txt")
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
    text = re.sub(r"(?i)(password|passwd|token)\s*[=:]\s*\S+", r"\1=<redacted>", text)
    text = re.sub(r"eyJ[A-Za-z0-9._-]{10,}", "<jwt-redacted>", text)
    return text


def main():
    host, user, password = read_connection()
    lines = ["# deploy context for the 21:59:02 CST upload-service change (read-only)",
             "note=produced by the implementation role; read here only for the reliability window", ""]
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    try:
        client.connect(host, port=22, username=user, password=password, timeout=20,
                       banner_timeout=20, auth_timeout=20, look_for_keys=False, allow_agent=False)
    except Exception as exc:  # noqa: BLE001
        lines.append("connect_result=FAILED %s: %s" % (type(exc).__name__, exc))
        write(lines)
        return 2
    lines.append("connect_result=OK")
    blocks = [
        ("deploy_dir", "ls -l --time-style=+%Y-%m-%dT%H:%M:%S /root/rd_upload_contract_20261125T135855Z; "
                       "ls -l --time-style=+%Y-%m-%dT%H:%M:%S /root/rd_ops_20261125"),
        ("deploy_log_tail", "tail -25 /root/rd_ops_20261125/deploy_v2.log 2>/dev/null"),
        ("deploy_script_head", "head -20 /root/rd_ops_20261125/deploy_v2.sh 2>/dev/null"),
        ("other_logs", "ls /root/rd_upload_contract_20261125T135855Z | head -12; "
                       "for f in /root/rd_upload_contract_20261125T135855Z/*.log; do echo \"== $f\"; tail -8 \"$f\"; done 2>/dev/null"),
    ]
    try:
        for label, command in blocks:
            lines.append("--- [%s]" % label)
            _in, out, err = client.exec_command(command, timeout=30)
            lines.append((out.read().decode("utf-8", "replace") + err.read().decode("utf-8", "replace")).rstrip())
            lines.append("")
    finally:
        client.close()
    write(lines)
    print(redact("\n".join(lines[-30:])))
    return 0


def write(lines):
    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(redact("\n".join(lines)).rstrip() + "\n")


if __name__ == "__main__":
    raise SystemExit(main())
