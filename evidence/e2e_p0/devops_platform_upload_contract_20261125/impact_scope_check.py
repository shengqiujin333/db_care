#!/usr/bin/env python
"""Bounded impact-scope check for the removed header gate (read-only).

Confirms that no other deployed script on the platform still expects or sends
`X-Upload-Token`, i.e. removing the gate does not break another local client.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import apply_driver as d  # noqa: E402
import paramiko  # noqa: E402

CMDS = [
    ("references_outside_abr",
     "grep -rl --include='*.py' --include='*.sh' --include='*.js' --include='*.json' "
     "'X-Upload-Token' /root 2>/dev/null | head -20; echo '-- end --'"),
    ("abr_token_refs", "grep -c 'X-Upload-Token\\|UPLOAD_TOKEN' /root/abr.py || true"),
    ("backup_digest",
     "sha256sum /root/abr.py /root/abr.py.bak_20261125T135902Z; "
     "stat -c '%n %s %a %y' /root/abr.py"),
    ("unit_final",
     "systemctl is-active user_script; systemctl is-enabled user_script; "
     "systemctl show user_script -p NRestarts -p ExecMainPID"),
]


def main():
    ip, user, pwd = d.read_connection()
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(ip, username=user, password=pwd, timeout=15,
                   allow_agent=False, look_for_keys=False)
    try:
        blocks = []
        for name, cmd in CMDS:
            rc, out, err = d.run(client, cmd, timeout=60)
            blocks.append("## %s (rc=%s)\n%s%s" % (name, rc, out, err))
    finally:
        client.close()
    body = d.redact("## impact scope / final state (read-only)\n\n" + "\n".join(blocks))
    target = os.path.join(HERE, "14_impact_scope_and_final_state.txt")
    with open(target, "w", encoding="utf-8") as fh:
        fh.write(body + "\n")
    print(body)
    print("wrote", target)


if __name__ == "__main__":
    main()
