#!/usr/bin/env python
"""Remove only the byte-compile leftover created by this change's patch step.

Deletes /root/__pycache__/abr_patch_*.cpython-*.pyc (created by py_compile of the
temporary candidate file). Nothing else is touched; the pre-existing
/root/__pycache__/abr.cpython-312.pyc (2026-10-22) is left in place.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import apply_driver as d  # noqa: E402
import paramiko  # noqa: E402

CMD = ("ls -la /root/__pycache__ 2>/dev/null; "
       "rm -f /root/__pycache__/abr_patch_*.cpython-*.pyc; "
       "echo '-- after --'; ls -la /root/__pycache__ 2>/dev/null; "
       "echo '-- workdir kept --'; ls -la /root/rd_upload_contract_20261125T135855Z")


def main():
    ip, user, pwd = d.read_connection()
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(ip, username=user, password=pwd, timeout=15,
                   allow_agent=False, look_for_keys=False)
    try:
        rc, out, err = d.run(client, CMD, timeout=40)
    finally:
        client.close()
    body = d.redact("## cleanup of patch byte-compile leftover (rc=%s)\n%s%s"
                    % (rc, out, err))
    target = os.path.join(HERE, "13_cleanup.txt")
    with open(target, "w", encoding="utf-8") as fh:
        fh.write(body + "\n")
    print(body)
    print("wrote", target)


if __name__ == "__main__":
    main()
