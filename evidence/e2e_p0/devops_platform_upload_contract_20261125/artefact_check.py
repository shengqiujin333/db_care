#!/usr/bin/env python
"""Small read-only follow-up: leftover artefacts of the patch and service counters."""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import apply_driver as d  # noqa: E402
import paramiko  # noqa: E402

CMDS = [
    ("pycache", "ls -la /root/__pycache__ 2>/dev/null || echo no_pycache"),
    ("workdir", "ls -la /root/rd_upload_contract_20261125T135855Z"),
    ("unit", "systemctl show user_script -p NRestarts -p ActiveState -p SubState -p ExecMainPID"),
    ("tail", "journalctl -u user_script --since '-15 min' --no-pager | tail -5"),
]


def main():
    ip, user, pwd = d.read_connection()
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(ip, username=user, password=pwd, timeout=15,
                   allow_agent=False, look_for_keys=False)
    out_all = []
    try:
        for name, cmd in CMDS:
            rc, out, err = d.run(client, cmd, timeout=40)
            out_all.append("## %s (rc=%s)\n%s%s" % (name, rc, out, err))
    finally:
        client.close()
    body = d.redact("\n".join(out_all))
    target = os.path.join(HERE, "12_artefact_check.txt")
    with open(target, "w", encoding="utf-8") as fh:
        fh.write(body + "\n")
    print(body)
    print("wrote", target)


if __name__ == "__main__":
    main()
