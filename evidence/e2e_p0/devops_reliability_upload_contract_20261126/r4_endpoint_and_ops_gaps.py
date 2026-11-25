#!/usr/bin/env python
"""Fourth window, second read-only pass: upload endpoint callers, error budget,
public-scan noise, systemd restart policy and rollback/observability gaps.

Read-only: journalctl / systemctl show / systemctl cat / ss / grep. No sudo,
no restart, no deploy, no write endpoint, no database.
"""
import os
import re
import time

import paramiko

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
OUT = os.path.join(HERE, "22_r4_endpoint_and_ops_gaps.txt")

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


BLOCKS = [
    ("upload_callers_since_2159",
     "journalctl -u user_script --since '2026-11-25 21:59:03' --no-pager 2>/dev/null "
     "| grep -E '\"POST /upload_data HTTP/1.1\"' "
     "| sed -E 's/.*python\\[[0-9]+\\]: //' "
     "| awk '{print $1, $NF}' | sort | uniq -c"),
    ("all_status_lines_since_2159",
     "journalctl -u user_script --since '2026-11-25 21:59:03' --no-pager 2>/dev/null "
     "| grep -oE '\"(GET|POST|HEAD|PUT|DELETE) [^ ]+ HTTP/1.1\" [0-9]{3}' | sort | uniq -c | sort -rn | head -25"),
    ("public_scan_lines",
     "journalctl -u user_script --since '2026-11-25 21:59:03' --no-pager 2>/dev/null "
     "| grep -E '\"(GET|POST) /(mcp|sse|sdk|actuator|\\\\.env|admin|wp-)' | tail -12"),
    ("restart_policy",
     "systemctl show user_script -p Restart -p RestartUSec -p StartLimitBurst -p StartLimitIntervalUSec "
     "-p WatchdogUSec -p ExecStart"),
    ("unit_file",
     "systemctl cat user_script 2>/dev/null | sed -E 's/(password|passwd|PWD)=.*/\\1=<redacted>/I' | head -40"),
    ("emqx_warn_rate",
     "grep -c 'warning' /var/log/emqx/emqx.log.1 2>/dev/null; echo '--last_warn--'; "
     "grep 'warning' /var/log/emqx/emqx.log.1 2>/dev/null | tail -3"),
    ("log_rotation",
     "ls -l --time-style=+%Y-%m-%dT%H:%M /var/log/emqx 2>/dev/null; "
     "echo '--emqx_log_rules--'; grep -nE 'file|level|rotation' /var/lib/emqx/configs/cluster.hocon 2>/dev/null | head -8"),
    ("cron_and_timers",
     "crontab -l 2>/dev/null | head -10; echo '--timers--'; "
     "systemctl list-timers --all --no-pager --no-legend 2>/dev/null | awk '{print $1, $2, $3, $NF}' | head -8"),
    ("disk_and_inode",
     "df -h / | tail -1; df -i / | tail -1; echo '--top_dirs--'; "
     "du -sh /var/log 2>/dev/null; du -sh /var/lib/mysql 2>/dev/null"),
    ("backup_retention",
     "ls -1 /root/abr.py.bak_* 2>/dev/null; echo '--cert_backups--'; ls -d /root/emqx-8883-cert-backup-* 2>/dev/null"),
]


def main():
    host, user, password = read_connection()
    lines = ["# fourth window pass 2: endpoint callers, error budget, ops gaps (read-only)",
             "started_utc=%s" % time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()), ""]
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(host, port=22, username=user, password=password, timeout=20,
                   banner_timeout=20, auth_timeout=20, look_for_keys=False, allow_agent=False)

    def run(command, timeout=45):
        _in, out, err = client.exec_command(command, timeout=timeout)
        return out.read().decode("utf-8", "replace") + err.read().decode("utf-8", "replace")

    outputs = {}
    try:
        for label, command in BLOCKS:
            lines.append("--- [%s]" % label)
            try:
                outputs[label] = run(command)
                lines.append(outputs[label].rstrip())
            except Exception as exc:  # noqa: BLE001
                lines.append("exec_error=%s: %s" % (type(exc).__name__, exc))
            lines.append("")
    finally:
        client.close()

    callers = outputs.get("upload_callers_since_2159", "")
    ips = re.findall(r"^\s*(\d+)\s+([0-9.]+)\s+(\d{3})$", callers, re.M)
    lines.append("## machine checks")
    lines.append("RESULT upload_callers=%s" % (ips or "none"))
    lines.append("RESULT distinct_caller_ips=%s" % sorted({i[1] for i in ips}))
    lines.append("RESULT non_2xx_only=%s"
                 % ("YES" if ips and all(i[2][0] in "45" for i in ips) else "NO"))
    statuses = outputs.get("all_status_lines_since_2159", "")
    lines.append("RESULT any_2xx_response_since_2159=%s"
                 % ("YES" if re.search(r"\" 2[0-9][0-9]", statuses) else "NO"))
    policy = outputs.get("restart_policy", "")
    m = re.search(r"Restart=(\S+)", policy)
    lines.append("RESULT restart_policy=%s" % (m.group(1) if m else "unknown"))
    warn = outputs.get("emqx_warn_rate", "").strip().splitlines()
    lines.append("RESULT emqx_warning_count_in_current_log=%s" % (warn[0].strip() if warn else "unknown"))
    write(lines)
    print(redact("\n".join(lines)))
    return 0


def write(lines):
    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(redact("\n".join(lines)).rstrip() + "\n")


if __name__ == "__main__":
    raise SystemExit(main())
