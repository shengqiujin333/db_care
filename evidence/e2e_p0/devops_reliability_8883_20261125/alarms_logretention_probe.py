#!/usr/bin/env python
"""Third reliability observation: listener counters, alarms, log retention (read-only).

Resolves the earlier counter parse, checks whether any EMQX alarm is raised after
the certificate change, and records the log-retention configuration that determines
how much pre/post evidence is available for incident correlation.
"""
import os
import re
import time

import paramiko

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
OUT = os.path.join(HERE, "03_alarms_and_log_retention.txt")
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


def main():
    host, user, password = read_connection()
    lines = ["# alarms / log retention / listener counters (read-only)",
             "started_utc=%s" % time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
             "forbidden=mutation,restart,db,credential-login,connect", ""]
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(host, port=22, username=user, password=password, timeout=20,
                   banner_timeout=20, auth_timeout=20, look_for_keys=False, allow_agent=False)
    cmds = [
        ("ssl_listener_block", "emqx ctl listeners 2>&1 | sed -n '/^ssl:default/,/^$/p'"),
        ("tcp_listener_block", "emqx ctl listeners 2>&1 | sed -n '/^tcp:default/,/^$/p'"),
        ("alarms", "emqx ctl alarms list 2>&1 | head -20"),
        ("log_config", "emqx ctl conf show log 2>&1 | head -40"),
        ("log_files", "ls -la /var/log/emqx/ 2>&1; echo '-- size/idx --'; cat /var/log/emqx/emqx.log.siz 2>&1; cat /var/log/emqx/emqx.log.idx 2>&1"),
        ("log_ssl_error_count", "grep -c 'ssl_error\\|handshake_failure\\|unknown_ca' /var/log/emqx/emqx.log.1 2>&1"),
        ("log_authn_count", "grep -c 'authentication_failure' /var/log/emqx/emqx.log.1 2>&1"),
        ("established_now", "ss -tn state established '( sport = :8883 )' 2>/dev/null | tail -n +2 | wc -l"),
        ("running_services", "systemctl list-units --type=service --state=running --no-pager 2>&1 | head -22"),
        ("logrotate", "ls -la /etc/logrotate.d/ 2>&1 | head -15; grep -A6 emqx /etc/logrotate.d/* 2>/dev/null | head -20"),
        ("emqx_version", "emqx ctl status 2>&1 | head -3; dpkg-query -W -f='${Version}\\n' emqx 2>&1"),
    ]
    for label, command in cmds:
        lines.append("--- [%s]" % label)
        try:
            _in, out, err = client.exec_command(command, timeout=25)
            text = out.read().decode("utf-8", "replace")
            errtext = err.read().decode("utf-8", "replace")
            out.channel.recv_exit_status()
            if text.strip():
                lines.append(text.rstrip())
            if errtext.strip():
                lines.append("stderr: " + errtext.rstrip())
        except Exception as exc:  # noqa: BLE001
            lines.append("exec_error=%s: %s" % (type(exc).__name__, exc))
        lines.append("")
    client.close()
    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(redact("\n".join(lines)) + "\n")
    print(redact("\n".join(lines)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
