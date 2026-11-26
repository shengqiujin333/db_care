#!/usr/bin/env python3
"""Round-4 CERT8883: read-only connection-count baseline on the platform.

Uses only `ss -tn` (service/port status). No sudo, no management API, no write,
no database connection, no restart. Credentials are read from the project
migration record and never echoed.
"""
import os
import re

import paramiko

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def read_connection():
    path = None
    for name in os.listdir(ROOT):
        if name.startswith("服务器迁移记录"):
            path = os.path.join(ROOT, name)
            break
    text = open(path, "r", encoding="utf-8", errors="replace").read()
    ip = None
    for row in text.splitlines():
        if "服务器 IP" in row:
            found = re.findall(r"[0-9]{1,3}(?:\.[0-9]{1,3}){3}", row)
            if found:
                ip = found[-1]
                break
    cred = re.search(r"目标服务器：\s*([^\s/]+)\s*/\s*([^\s`]+)", text)
    return ip, cred.group(1), cred.group(2)


def main():
    host, user, password = read_connection()
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    print("# round-4 read-only connection counts (ss -tn)")
    try:
        client.connect(host, port=22, username=user, password=password, timeout=20,
                       banner_timeout=20, auth_timeout=20, look_for_keys=False,
                       allow_agent=False)
        print("connect_result=OK")
    except Exception as exc:  # noqa: BLE001
        print("connect_result=FAILED error=%s: %s" % (type(exc).__name__, exc))
        raise SystemExit(2)

    commands = [
        ("all_established",
         "ss -tn state established 2>/dev/null | awk 'NR>1 {print $4}' | "
         "awk -F: '{print $NF}' | sort | uniq -c | sort -rn | head -20"),
        ("per_port_detail",
         "ss -tn state established 2>/dev/null | head -40"),
    ]
    for label, command in commands:
        print("--- [%s]" % label)
        _in, out, err = client.exec_command(command, timeout=30)
        text = out.read().decode("utf-8", "replace")
        errtext = err.read().decode("utf-8", "replace")
        print("exit=%s" % out.channel.recv_exit_status())
        if text.strip():
            print(text.rstrip())
        if errtext.strip():
            print("stderr: " + errtext.rstrip())
        print("")
    client.close()


if __name__ == "__main__":
    main()
