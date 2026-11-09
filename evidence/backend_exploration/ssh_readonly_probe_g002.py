#!/usr/bin/env python
"""Bounded, read-only SSH re-inspection of the existing project service.

Authorization: runtime entry `existing_service_ssh_inspection`
(allowed external action: inspect_existing_service_readonly).

Scope honoured:
  * host / account / password are read from the project material
    (服务器迁移记录.md); never hard-coded, never echoed;
  * no sudo, no file/config/data mutation, no service start/stop/restart,
    no deploy, no write-endpoint call, no database connection;
  * read-only HTTP probes use GET / OPTIONS only (no request body);
  * every command has a finite timeout and the whole session is bounded;
  * output is redacted (credentials, phone-like numbers, access keys)
    before the evidence file is written.

This script is a NEW evidence producer for the current run. It does not
overwrite the historical BE-EXP-002 evidence files.
"""
import base64
import hashlib
import os
import re
import socket
import sys
import time

import paramiko

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
MIGRATION = os.path.join(ROOT, "服务器迁移记录.md")
OUT = os.path.join(HERE, "ssh_readonly_inspection_g002.txt")

CONNECT_TIMEOUT = 20
CMD_TIMEOUT = 12
TOTAL_BUDGET = 150

ROUTES = [
    "/login",
    "/upload_data",
    "/sendVerificationCode",
    "/verifyCode",
    "/register",
    "/setmac",
    "/doesNotExist",
]

COMMANDS = [
    ("identity", "date -u +%Y-%m-%dT%H:%M:%SZ; hostname; uname -srm; cat /etc/os-release | head -2"),
    ("systemd_units", "for u in emqx user_script; do echo \"$u active=$(systemctl is-active $u 2>&1) enabled=$(systemctl is-enabled $u 2>&1)\"; done"),
    ("systemd_detail", "systemctl show emqx user_script -p Id -p ActiveState -p SubState -p ExecMainStartTimestamp -p ExecMainPID -p NRestarts 2>&1"),
    ("listening_ports", "ss -lnt 2>&1 | head -40"),
    ("process_owner", "pgrep -af 'abr.py' 2>&1; ss -lntp 2>/dev/null | grep -E ':(5000|8883|3306)\\b' 2>&1"),
    ("docker_ps", "docker ps --format '{{.Names}}|{{.Image}}|{{.Status}}|{{.Ports}}' 2>&1"),
    ("abr_metadata", "stat -c '%n size=%s mtime=%y' /root/abr.py /root/abr.py.bak_20260926 2>&1"),
    ("emqx_version", "dpkg -l emqx 2>&1 | tail -2; emqx ctl status 2>&1 | head -12"),
    ("emqx_broker", "emqx ctl broker 2>&1 | head -8"),
    ("emqx_listeners", "emqx ctl listeners 2>&1 | head -80"),
    ("emqx_clients", "emqx ctl clients list 2>&1 | head -20"),
    ("emqx_broker_stats", "emqx ctl broker stats 2>&1 | head -30"),
    ("http_headers_root", "curl -s -i --max-time 5 http://127.0.0.1:5000/ 2>&1 | head -12"),
    ("http_options_upload", "curl -s -i --max-time 5 -X OPTIONS http://127.0.0.1:5000/upload_data 2>&1 | head -20"),
    ("http_journal_counts",
     "echo -n 'request_lines_total='; journalctl -u user_script --no-pager 2>/dev/null | grep -cE '\"(POST|GET|HEAD) ' ; "
     "echo -n 'post_lines_total='; journalctl -u user_script --no-pager 2>/dev/null | grep -cE '\"POST ' ; "
     "echo 'last_post_lines:'; journalctl -u user_script --no-pager 2>/dev/null | grep -E '\"POST ' | tail -5 ; "
     "echo 'last_non_probe_request:'; journalctl -u user_script --no-pager 2>/dev/null | grep -E '\"(POST|GET|HEAD) ' | grep -v '127.0.0.1 - - \[10/Nov/2026 07:0' | tail -4"),
    ("journal_20261022_upload_window",
     "journalctl -u user_script --since '2026-10-22 19:31:00' --until '2026-10-22 19:33:30' --no-pager 2>&1 | head -60"),
    ("recent_service_log", "journalctl -u user_script --since '-3 days' --no-pager -n 30 2>&1 | tail -30"),
    ("resources", "df -h / 2>&1; free -m 2>&1 | head -3"),
]

# GET-only route existence probes built as one server-side shell loop.
ROUTE_PROBE = (
    "for p in " + " ".join(ROUTES) + "; do "
    "code=$(curl -s -o /dev/null -w '%{http_code}' --max-time 5 \"http://127.0.0.1:5000$p\"); "
    "allow=$(curl -s -I -o /dev/null -w '%{http_code}' --max-time 5 -X GET \"http://127.0.0.1:5000$p\" 2>/dev/null); "
    "echo \"GET $p -> $code\"; "
    "curl -s -i --max-time 5 \"http://127.0.0.1:5000$p\" 2>/dev/null | grep -i '^allow:' | tr -d '\\r' | sed \"s|^|  $p |\"; "
    "done"
)

PHONE_RE = re.compile(r"(?<!\d)1[3-9]\d{9}(?!\d)")
SECRET_RE = re.compile(r"LTAI[0-9A-Za-z]{12,}")


def read_connection():
    with open(MIGRATION, "r", encoding="utf-8") as fh:
        text = fh.read()
    host = re.search(
        r"服务器 IP.*?\|\s*[0-9.]+\s*\|\s*\*{0,2}([0-9]{1,3}(?:\.[0-9]{1,3}){3})", text
    )
    cred = re.search(r"目标服务器：\s*([^\s/]+)\s*/\s*([^\s`]+)", text)
    if not host or not cred:
        raise SystemExit("connection info not found in project material")
    return host.group(1), cred.group(1), cred.group(2)


def redact(text, secrets):
    text = PHONE_RE.sub("<phone-redacted>", text)
    for value in secrets:
        if value and len(value) > 2:
            text = text.replace(value, "<secret-redacted>")
    return SECRET_RE.sub("<secret-redacted>", text)


def tcp_probe(host):
    lines = []
    for port in (22, 1883, 3306, 5000, 8083, 8883, 18083):
        t0 = time.time()
        try:
            with socket.create_connection((host, port), timeout=5):
                lines.append("port=%d TCP_OPEN %.0fms" % (port, (time.time() - t0) * 1000))
        except Exception as exc:  # noqa: BLE001
            lines.append("port=%d TCP_FAIL %s %.0fms" % (port, type(exc).__name__, (time.time() - t0) * 1000))
    return lines


def main():
    host, user, password = read_connection()
    started = time.time()
    lines = [
        "# read-only SSH re-inspection of the existing service (current run)",
        "started_utc=%s" % time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "host=%s port=22 account=<redacted-account> transport=paramiko/%s" % (host, paramiko.__version__),
        "scope=identity,unit_state,listening_ports,process_owner,container_state,service_metadata,"
        "emqx_version/listeners/stats,GET|OPTIONS route probes,recent_logs,resources",
        "forbidden_in_this_run=write_endpoints,database_access,sudo,mutation,start/stop/restart,deploy",
        "",
        "--- [local_tcp_probe] connect/close only, 5 s per port",
    ]
    lines.extend(tcp_probe(host))
    lines.append("")

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
        lines.append("connect_result=FAILED after=%.1fs" % (time.time() - started))
        lines.append("connect_error=%s: %s" % (type(exc).__name__, exc))
        lines.append("note=no command executed; no credential value recorded")
        write(lines, [password])
        print(redact("\n".join(lines), [password]))
        return 2

    transport = client.get_transport()
    try:
        key = transport.get_remote_server_key()
        fp = base64.b64encode(hashlib.sha256(key.asbytes()).digest()).decode().rstrip("=")
        lines.append("server_host_key=%s fingerprint_sha256=%s" % (key.get_name(), fp))
    except Exception as exc:  # noqa: BLE001
        lines.append("server_host_key=unavailable (%s)" % type(exc).__name__)
    lines.append("connect_result=OK after=%.1fs" % (time.time() - started))
    lines.append("")

    commands = list(COMMANDS)
    commands.insert(12, ("route_existence_get_only", ROUTE_PROBE))

    secrets = [password]
    for label, command in commands:
        if time.time() - started > TOTAL_BUDGET:
            lines.append("--- [%s] SKIPPED (total budget exhausted) ---" % label)
            continue
        lines.append("--- [%s] $ %s" % (label, command))
        t0 = time.time()
        try:
            _stdin, stdout, stderr = client.exec_command(command, timeout=CMD_TIMEOUT)
            out = stdout.read().decode("utf-8", "replace")
            err = stderr.read().decode("utf-8", "replace")
            code = stdout.channel.recv_exit_status()
            lines.append("exit=%s elapsed_ms=%.0f" % (code, (time.time() - t0) * 1000))
            if out.strip():
                lines.append("stdout:")
                lines.append(out.rstrip())
            if err.strip():
                lines.append("stderr:")
                lines.append(err.rstrip())
        except Exception as exc:  # noqa: BLE001
            lines.append("exec_error=%s: %s elapsed_ms=%.0f" % (type(exc).__name__, exc, (time.time() - t0) * 1000))
        lines.append("")

    client.close()
    lines.append("finished_utc=%s total_s=%.1f" % (time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()), time.time() - started))
    write(lines, secrets)
    print(redact("\n".join(lines), secrets))
    return 0


def write(lines, secrets):
    body = redact("\n".join(lines), secrets) + "\n"
    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(body)


if __name__ == "__main__":
    sys.exit(main())
