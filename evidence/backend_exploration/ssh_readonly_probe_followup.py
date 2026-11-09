#!/usr/bin/env python
"""Follow-up bounded read-only SSH inspection (same authorization as probe 1).

Adds: journal retention/history, EMQX listener + broker stats, EMQX file log
tail, port->process mapping, MySQL image identity. No mutation.
"""
import base64
import hashlib
import os
import sys
import time

import paramiko

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ssh_readonly_probe import CMD_TIMEOUT, CONNECT_TIMEOUT, read_connection, redact  # noqa: E402

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "ssh_readonly_inspection_2.txt")
TOTAL_BUDGET = 150

COMMANDS = [
    ("journal_usage", "journalctl --disk-usage 2>&1; journalctl -u user_script --no-pager -n 6 --reverse 2>&1 | head -20"),
    ("service_log_30d", "journalctl -u user_script --since '-30 days' --no-pager 2>&1 | wc -l; journalctl -u user_script --since '-30 days' --no-pager -n 25 2>&1 | tail -25"),
    ("port_process_map", "ss -lntp 2>&1 | head -30"),
    ("emqx_listeners", "emqx ctl listeners 2>&1 | head -60"),
    ("emqx_broker_stats", "emqx ctl stats 2>&1 | head -30"),
    ("emqx_log_files", "ls -l /var/log/emqx 2>&1 | tail -6"),
    ("emqx_log_tail", "tail -40 /var/log/emqx/emqx.log 2>&1"),
    ("mysql_image", "docker image inspect mysql --format 'id={{.Id}} digests={{.RepoDigests}} created={{.Created}}' 2>&1"),
    ("mysql_uptime_state", "docker inspect --format 'health={{if .State.Health}}{{.State.Health.Status}}{{else}}none{{end}} restart_policy={{.HostConfig.RestartPolicy.Name}}' mysql 2>&1"),
    ("os_release", "cat /etc/os-release 2>&1 | head -4; python3 --version 2>&1"),
]


def main():
    host, user, password = read_connection()
    started = time.time()
    lines = [
        "# follow-up bounded read-only SSH inspection of existing service",
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
    lines.append("connect_result=OK after=%.1fs" % (time.time() - started))
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
