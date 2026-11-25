#!/usr/bin/env python
"""Read-only check that the rollback point for the 8883 certificate is still intact."""
import os
import re

import paramiko

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
OUT = os.path.join(HERE, "06_rollback_point.txt")
BACKUP = "/root/emqx-8883-cert-backup-20261125T025805Z"
BASELINE_CERT_SHA256 = "a91de074d8ca09549abafc7787019ab2e68b4c8042e04130e9c27f610a0855f9"
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


def main():
    host, user, password = read_connection()
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(host, port=22, username=user, password=password, timeout=20,
                   banner_timeout=20, auth_timeout=20, look_for_keys=False, allow_agent=False)
    cmd = ("stat -c 'backup_dir=%n mode=%a owner=%U:%G mtime=%y' " + BACKUP + " 2>&1; "
           "ls -l --full-time " + BACKUP + " 2>&1; sha256sum " + BACKUP + "/*.crt* " + BACKUP + "/*.key* 2>&1; "
           "cat " + BACKUP + "/SHA256SUMS.txt 2>&1")
    _in, out, err = client.exec_command(cmd, timeout=25)
    text = out.read().decode("utf-8", "replace") + err.read().decode("utf-8", "replace")
    out.channel.recv_exit_status()
    client.close()

    lines = ["# rollback point check (read-only)", "backup_dir=%s" % BACKUP, "", text.rstrip(), "",
             "## machine checks",
             "RESULT baseline_cert_in_backup=%s" % ("YES" if BASELINE_CERT_SHA256 in text else "NO"),
             "RESULT backup_dir_present=%s" % ("YES" if "backup_dir=" in text and "No such file" not in text else "NO")]
    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(redact("\n".join(lines)) + "\n")
    print(redact("\n".join(lines)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
