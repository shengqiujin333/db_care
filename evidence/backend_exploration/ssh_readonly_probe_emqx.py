#!/usr/bin/env python
"""Third bounded read-only SSH probe: EMQX traffic counters and client table.

Counters show whether any gateway/app ever connected since the broker restart;
`clients list` is empty when nothing is connected. Read-only, no mutation.
"""
import base64
import hashlib
import os
import sys
import time

import paramiko

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ssh_readonly_probe import CMD_TIMEOUT, CONNECT_TIMEOUT, read_connection, redact  # noqa: E402

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "ssh_readonly_inspection_3.txt")
TOTAL_BUDGET = 120

COMMANDS = [
    ("emqx_broker", "emqx ctl broker 2>&1 | head -25"),
    ("emqx_broker_stats", "emqx ctl broker stats 2>&1 | head -40"),
    ("emqx_broker_metrics", "emqx ctl broker metrics 2>&1 | head -60"),
    ("emqx_clients", "emqx ctl clients list 2>&1 | head -20"),
    ("emqx_journal_span", "journalctl -u emqx --no-pager -n 3 --reverse 2>&1; journalctl -u emqx --since '-30 days' --no-pager 2>&1 | wc -l; journalctl -u emqx --since '-30 days' --no-pager -n 12 2>&1 | tail -12"),
    ("emqx_conf_listeners", "emqx ctl conf show listeners 2>&1 | head -40"),
]


def main():
    host, user, password = read_connection()
    started = time.time()
    lines = [
        "# third bounded read-only SSH probe (EMQX traffic counters)",
        f"started_utc={time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())}",
        f"host={host} port=22 transport=paramiko/{paramiko.__version__}",
    ]
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    try:
        client.connect(host, 22, username=user, password=password, timeout=CONNECT_TIMEOUT,
                       banner_timeout=CONNECT_TIMEOUT, auth_timeout=CONNECT_TIMEOUT,
                       look_for_keys=False, allow_agent=False)
    except Exception as exc:  # noqa: BLE001
        lines.append(f"connect_result=FAILED error={type(exc).__name__}: {exc}")
        _dump(lines, [password])
        print("\n".join(lines))
        return 2
    key = client.get_transport().get_remote_server_key()
    lines.append("server_host_key=%s fingerprint_sha256=%s" % (
        key.get_name(), base64.b64encode(hashlib.sha256(key.asbytes()).digest()).decode().rstrip("=")))
    lines.append("connect_result=OK")
    lines.append("")
    for label, command in COMMANDS:
        if time.time() - started > TOTAL_BUDGET:
            lines.append(f"--- [{label}] SKIPPED (total budget exhausted) ---")
            continue
        lines.append(f"--- [{label}] $ {command}")
        t0 = time.time()
        try:
            _in, stdout, stderr = client.exec_command(command, timeout=CMD_TIMEOUT)
            out = stdout.read().decode("utf-8", "replace")
            err = stderr.read().decode("utf-8", "replace")
            code = stdout.channel.recv_exit_status()
            lines.append(f"exit={code} elapsed_ms={(time.time()-t0)*1000:.0f}")
            if out.strip():
                lines.append("stdout:\n" + out.rstrip())
            if err.strip():
                lines.append("stderr:\n" + err.rstrip())
        except Exception as exc:  # noqa: BLE001
            lines.append(f"exec_error={type(exc).__name__}: {exc}")
        lines.append("")
    client.close()
    lines.append(f"finished_utc={time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())} total_s={time.time()-started:.1f}")
    _dump(lines, [password, user])
    print("\n".join(lines))
    return 0


def _dump(lines, secrets):
    secrets = [s for s in secrets if s and len(s) > 2]
    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(redact("\n".join(lines), secrets) + "\n")


if __name__ == "__main__":
    sys.exit(main())
