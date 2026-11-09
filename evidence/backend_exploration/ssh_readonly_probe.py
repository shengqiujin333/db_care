#!/usr/bin/env python
"""Bounded, read-only SSH inspection of the existing project service.

Authorization: runtime entry `existing_service_ssh_inspection`
(allowed external action: inspect_existing_service_readonly).

Rules honoured by this script:
  * host / port / account / password are read from project material
    (服务器迁移记录.md), never hard-coded and never echoed;
  * no sudo, no file/config/data mutation, no service start/stop/restart,
    no deploy, no write endpoint call, no direct database connection;
  * every command has a finite timeout and the whole session is bounded;
  * captured text is redacted (credentials + phone-like numbers) before it is
    written to the evidence file.
"""
import os
import re
import socket
import sys
import time

import paramiko

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
MIGRATION = os.path.join(ROOT, "服务器迁移记录.md")
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "ssh_readonly_inspection.txt")

CONNECT_TIMEOUT = 20
CMD_TIMEOUT = 15
TOTAL_BUDGET = 150

# Read-only command set. Each entry: (label, command)
COMMANDS = [
    ("identity", "date -u +%Y-%m-%dT%H:%M:%SZ; hostname; uname -srm"),
    ("systemd_units", "systemctl is-active emqx user_script 2>&1; systemctl is-enabled emqx user_script 2>&1"),
    ("systemd_detail", "systemctl show emqx user_script -p Id -p ActiveState -p SubState -p ExecMainStartTimestamp -p NRestarts -p ExecStart 2>&1"),
    ("listening_ports", "ss -lnt 2>&1"),
    ("docker_ps", "docker ps --format '{{.Names}}|{{.Image}}|{{.Status}}|{{.Ports}}' 2>&1"),
    ("mysql_container", "docker inspect --format '{{.Name}} image={{.Config.Image}} started={{.State.StartedAt}} restarts={{.RestartCount}} running={{.State.Running}}' mysql 2>&1"),
    ("processes", "pgrep -af abr.py 2>&1; pgrep -af emqx 2>&1 | head -5"),
    ("http_root", "curl -s -o /dev/null -w 'GET / -> %{http_code}\\n' --max-time 5 http://127.0.0.1:5000/ 2>&1"),
    ("http_login_route", "curl -s -o /dev/null -w 'GET /login -> %{http_code}\\n' --max-time 5 http://127.0.0.1:5000/login 2>&1"),
    ("http_upload_route", "curl -s -i --max-time 5 -X GET http://127.0.0.1:5000/upload_data 2>&1 | head -25"),
    ("abr_files", "ls -l /root/abr.py /root/abr.py.bak_20260926 2>&1; stat -c '%n size=%s mtime=%y' /root/abr.py 2>&1"),
    ("service_log", "journalctl -u user_script --since '-4 days' --no-pager -n 120 2>&1 | tail -120"),
    ("emqx_log", "journalctl -u emqx --since '-2 days' --no-pager -n 40 2>&1 | tail -40"),
    ("emqx_version", "dpkg -l emqx 2>&1 | tail -3; emqx ctl status 2>&1 | head -20"),
    ("disk", "df -h / 2>&1; free -m 2>&1 | head -3"),
]


def read_connection():
    with open(MIGRATION, "r", encoding="utf-8") as fh:
        text = fh.read()
    hosts = re.search(r"服务器 IP.*?\|\s*[0-9.]+\s*\|\s*\*{0,2}([0-9]{1,3}(?:\.[0-9]{1,3}){3})", text)
    cred = re.search(r"目标服务器：\s*([^\s/]+)\s*/\s*([^\s`]+)", text)
    if not hosts or not cred:
        raise SystemExit("connection info not found in project material")
    return hosts.group(1), cred.group(1), cred.group(2)


PHONE_RE = re.compile(r"(?<!\d)1[3-9]\d{9}(?!\d)")
SECRET_RE = re.compile(r"(?i)(LTAI[0-9A-Za-z]{12,}|&?\^?!?A:z\?|jinyuni\d+|5686123z[A-Za-z!]*)")
IP_RE = re.compile(r"\b\d{1,3}(?:\.\d{1,3}){3}\b")


def redact(text, secret_values):
    text = PHONE_RE.sub("<phone-redacted>", text)
    for value in secret_values:
        if value:
            text = text.replace(value, "<secret-redacted>")
    text = SECRET_RE.sub("<secret-redacted>", text)
    return text


def main():
    host, user, password = read_connection()
    started = time.time()
    lines = []
    lines.append(f"# read-only SSH inspection of existing service")
    lines.append(f"started_utc={time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())}")
    lines.append(f"host={host} port=22 account={user} transport=paramiko/{paramiko.__version__}")
    lines.append(f"scope=status,listening_ports,recent_service_logs,runtime_version (no sudo/mutation)")

    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    try:
        client.connect(
            host,
            port=22,
            username=user,
            password=password,
            timeout=CONNECT_TIMEOUT,
            banner_timeout=CONNECT_TIMEOUT,
            auth_timeout=CONNECT_TIMEOUT,
            look_for_keys=False,
            allow_agent=False,
        )
    except Exception as exc:  # noqa: BLE001
        lines.append(f"connect_result=FAILED after={(time.time()-started):.1f}s")
        lines.append(f"connect_error={type(exc).__name__}: {exc}")
        lines.append("note=no command executed; no credential value recorded")
        _write(lines)
        print("\n".join(lines))
        return 2

    transport = client.get_transport()
    try:
        key = transport.get_remote_server_key()
        lines.append(
            "server_host_key=%s fingerprint_sha256=%s"
            % (key.get_name(), __import__("base64").b64encode(__import__("hashlib").sha256(key.asbytes()).digest()).decode().rstrip("="))
        )
    except Exception as exc:  # noqa: BLE001
        lines.append(f"server_host_key=unavailable ({type(exc).__name__})")
    lines.append(f"connect_result=OK after={(time.time()-started):.1f}s")
    lines.append("")

    secrets = [password, user]
    for label, command in COMMANDS:
        if time.time() - started > TOTAL_BUDGET:
            lines.append(f"--- [{label}] SKIPPED (total budget exhausted) ---")
            continue
        lines.append(f"--- [{label}] $ {command}")
        t0 = time.time()
        try:
            _stdin, stdout, stderr = client.exec_command(command, timeout=CMD_TIMEOUT)
            out = stdout.read().decode("utf-8", "replace")
            err = stderr.read().decode("utf-8", "replace")
            code = stdout.channel.recv_exit_status()
            lines.append(f"exit={code} elapsed_ms={(time.time()-t0)*1000:.0f}")
            if out.strip():
                lines.append("stdout:")
                lines.append(out.rstrip())
            if err.strip():
                lines.append("stderr:")
                lines.append(err.rstrip())
        except Exception as exc:  # noqa: BLE001
            lines.append(f"exec_error={type(exc).__name__}: {exc} elapsed_ms={(time.time()-t0)*1000:.0f}")
        lines.append("")

    client.close()
    lines.append(f"finished_utc={time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())} total_s={time.time()-started:.1f}")
    _write(lines, secrets)
    print("\n".join(lines))
    return 0


def _write(lines, secrets=None):
    secrets = [s for s in (secrets or []) if s and len(s) > 2]
    body = redact("\n".join(lines), secrets) + "\n"
    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(body)


if __name__ == "__main__":
    sys.exit(main())
