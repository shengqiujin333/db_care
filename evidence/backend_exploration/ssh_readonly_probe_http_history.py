#!/usr/bin/env python
"""Fifth bounded read-only probe: quantify HTTP request history in the service log.

Counts request lines and POST occurrences over the whole journalled history of
the unit (bounded by journal retention), so the "no client traffic" statement
has an explicit window instead of a sample.
"""
import base64
import hashlib
import os
import sys
import time

import paramiko

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ssh_readonly_probe import CMD_TIMEOUT, CONNECT_TIMEOUT, read_connection, redact  # noqa: E402

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "http_history_counts.txt")
TOTAL_BUDGET = 120

COMMANDS = [
    ("earliest_entry", "journalctl -u user_script --no-pager -o short-iso 2>&1 | head -2"),
    ("history_counts", "journalctl -u user_script --no-pager 2>&1 | awk 'BEGIN{req=0;post=0;get=0} /HTTP\\/1\\.1/{req++} /\"POST /{post++} /\"GET /{get++} END{printf \"request_lines=%d post_lines=%d get_lines=%d\\n\", req, post, get}'"),
    ("history_lines_total", "journalctl -u user_script --no-pager 2>&1 | wc -l"),
]


def main():
    host, user, password = read_connection()
    started = time.time()
    lines = [
        "# HTTP request history counts (read-only)",
        f"started_utc={time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())}",
        f"host={host} port=22 transport=paramiko/{paramiko.__version__}",
    ]
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(host, 22, username=user, password=password, timeout=CONNECT_TIMEOUT,
                   banner_timeout=CONNECT_TIMEOUT, auth_timeout=CONNECT_TIMEOUT,
                   look_for_keys=False, allow_agent=False)
    key = client.get_transport().get_remote_server_key()
    lines.append("server_host_key=%s fingerprint_sha256=%s" % (
        key.get_name(), base64.b64encode(hashlib.sha256(key.asbytes()).digest()).decode().rstrip("=")))
    lines.append("connect_result=OK")
    lines.append("")
    for label, command in COMMANDS:
        if time.time() - started > TOTAL_BUDGET:
            lines.append(f"--- [{label}] SKIPPED (budget) ---")
            continue
        lines.append(f"--- [{label}] $ {command}")
        _in, stdout, stderr = client.exec_command(command, timeout=CMD_TIMEOUT)
        out = stdout.read().decode("utf-8", "replace")
        err = stderr.read().decode("utf-8", "replace")
        lines.append(f"exit={stdout.channel.recv_exit_status()}")
        if out.strip():
            lines.append("stdout:\n" + out.rstrip())
        if err.strip():
            lines.append("stderr:\n" + err.rstrip())
        lines.append("")
    client.close()
    lines.append(f"finished_utc={time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())} total_s={time.time()-started:.1f}")
    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(redact("\n".join(lines), [password, user]) + "\n")
    print("\n".join(lines))
    return 0


if __name__ == "__main__":
    sys.exit(main())
