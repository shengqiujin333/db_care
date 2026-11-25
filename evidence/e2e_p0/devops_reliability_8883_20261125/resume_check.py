#!/usr/bin/env python
"""Resumed reliability check for the 8883 certificate change (read-only, bounded).

The previous run's window closed at CST 11:42 while the client-side MQTT session was
still blocked by a placeholder identity (CONNACK 5). This run re-observes the same
objects ~2h45m later so the MonitoringArtifact can state whether the TLS layer stayed
healthy and whether the client session outcome changed. It never overwrites the
earlier evidence files.

Read-only: status/show/grep/stat/openssl s_client/ss. No sudo, no restart, no deploy,
no config or data write, no database, no MQTT CONNECT (so no auth attempts and no
counter movement caused by this probe). Connection data comes from project material
and is redacted from output.
"""
import os
import re
import time

import paramiko

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
OUT = os.path.join(HERE, "07_resume_window.txt")

CONNECT_TIMEOUT = 20
CMD_TIMEOUT = 30
TOTAL_BUDGET = 300
SAMPLE_GAP = 90

CERTFILE = "/var/lib/emqx/certs/listeners/ssl/default/cert-2D2ADC29A9BE14AA"
CACERT = "/var/lib/emqx/certs/listeners/ssl/default/cacert-613B9023E1E698F0"
BACKUP = "/root/emqx-8883-cert-backup-20261125T025805Z"
APPROVED_SERIAL = "77ADCFD924BA0F87F845C81934D4A3F346D4BB83"

PASSWORDS = []


def read_connection():
    path = None
    for name in os.listdir(ROOT):
        if name.startswith("\u670d\u52a1\u5668"):
            path = os.path.join(ROOT, name)
            break
    if not path:
        raise SystemExit("migration record not found")
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
    if not ip or not cred:
        raise SystemExit("connection info not found in project material")
    PASSWORDS.append(cred.group(2))
    return ip, cred.group(1), cred.group(2)


def redact(text):
    for value in PASSWORDS:
        if value:
            text = text.replace(value, "<secret-redacted>")
    return re.sub(r"LTAI[0-9A-Za-z]{12,}", "<secret-redacted>", text)


COUNTER_CMD = ("emqx ctl listeners 2>&1 | sed -n '/ssl:default/,/^tcp:default/p' | "
               "grep -E 'running|current_conn|shutdown_count'")
AUTHN_CMD = ("grep -hE 'not_authorized|AUTHN' /var/log/emqx/emqx.log.1 /var/log/emqx/emqx.log 2>/dev/null "
             "| tail -12")
SSL_ERR_CMD = ("grep -hcE 'ssl_error|handshake_failure|unknown_ca|no_peer_cert' "
               "/var/log/emqx/emqx.log 2>/dev/null; echo '---'; "
               "ls -l /var/log/emqx/ | head -8")


def main():
    host, user, password = read_connection()
    started = time.time()
    lines = [
        "# resumed reliability window (8883 certificate replacement, same objects)",
        "started_utc=%s" % time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "scope=service-stability,listener,handshake-rate,auth-outcome,drift,rollback-point",
        "forbidden=mutation,sudo,restart,deploy,database,write-endpoint,credential-login,mqtt-connect",
        "",
    ]

    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    try:
        client.connect(host, port=22, username=user, password=password,
                       timeout=CONNECT_TIMEOUT, banner_timeout=CONNECT_TIMEOUT,
                       auth_timeout=CONNECT_TIMEOUT, look_for_keys=False, allow_agent=False)
    except Exception as exc:  # noqa: BLE001
        lines.append("connect_result=FAILED error=%s: %s" % (type(exc).__name__, exc))
        write(lines)
        print(redact("\n".join(lines)))
        return 2
    lines.append("connect_result=OK after=%.1fs" % (time.time() - started))

    def run(client, command, timeout=CMD_TIMEOUT):
        _in, out, err = client.exec_command(command, timeout=timeout)
        text = out.read().decode("utf-8", "replace")
        errtext = err.read().decode("utf-8", "replace")
        return text + errtext

    try:
        blocks = [
            ("identity", "date -u +%Y-%m-%dT%H:%M:%SZ; date +%Y-%m-%dT%H:%M:%S%z; hostname"),
            ("unit_state", "systemctl show emqx -p ActiveState -p SubState -p NRestarts "
                           "-p ActiveEnterTimestamp -p MemoryCurrent"),
            ("host_load", "cat /proc/loadavg; free -m | head -3; df -h / | tail -1; uptime"),
            ("emqx_status", "emqx ctl status 2>&1 | head -6"),
            ("broker", "emqx ctl broker 2>&1 | head -14"),
            ("listener_ssl", COUNTER_CMD),
            ("clients", "emqx ctl clients list 2>&1 | head -12"),
            ("leaf", "openssl x509 -in %s -noout -subject -serial -dates -ext subjectAltName 2>&1" % CERTFILE),
            ("cert_file_stat", "stat -c 'mtime=%%y size=%%s mode=%%a' %s; sha256sum %s | cut -c1-16" % (CERTFILE, CERTFILE)),
            ("handshake_10",
             "ok=0; for i in $(seq 1 10); do "
             "if timeout 10 openssl s_client -connect 127.0.0.1:8883 -verify_ip 8.140.23.253 "
             "-CAfile %s </dev/null 2>/dev/null | grep -q 'Verify return code: 0 (ok)'; "
             "then ok=$((ok+1)); fi; done; echo \"handshake_ok_count=$ok/10\"" % CACERT),
            ("established_8883", "ss -tn state established '( sport = :8883 )' 2>/dev/null | tail -n +2 | wc -l"),
            ("authn_recent", AUTHN_CMD),
            ("ssl_error_logscan", SSL_ERR_CMD),
            ("backup_point", "stat -c 'dir=%%n mode=%%a owner=%%U:%%G' " + BACKUP + " 2>&1; "
                             "sha256sum " + BACKUP + "/leaf.crt.bak 2>/dev/null | cut -c1-16"),
            ("mtime_listener_conf", "ls -l --time-style=+%Y-%m-%dT%H:%M:%S /etc/emqx/*.hocon 2>/dev/null | head -5"),
        ]
        outputs = {}
        for label, command in blocks:
            if time.time() - started > TOTAL_BUDGET:
                lines.append("--- [%s] SKIPPED (budget)" % label)
                continue
            lines.append("--- [%s]" % label)
            try:
                outputs[label] = run(client, command)
                lines.append(outputs[label].rstrip())
            except Exception as exc:  # noqa: BLE001
                lines.append("exec_error=%s: %s" % (type(exc).__name__, exc))
            lines.append("")

        # paired counter sample: real client auth outcomes over a fixed interval
        lines.append("--- [counter_delta_%ss]" % SAMPLE_GAP)
        t0 = run(client, COUNTER_CMD)
        est0 = run(client, "ss -tn state established '( sport = :8883 )' 2>/dev/null | tail -n +2 | wc -l")
        lines.append("t0_utc=%s" % time.strftime("%H:%M:%SZ", time.gmtime()))
        lines.append(t0.rstrip())
        lines.append("t0_established=%s" % est0.strip())
        time.sleep(SAMPLE_GAP)
        t1 = run(client, COUNTER_CMD)
        est1 = run(client, "ss -tn state established '( sport = :8883 )' 2>/dev/null | tail -n +2 | wc -l")
        lines.append("t1_utc=%s elapsed_s=%d" % (time.strftime("%H:%M:%SZ", time.gmtime()), time.time() - started))
        lines.append(t1.rstrip())
        lines.append("t1_established=%s" % est1.strip())
        lines.append("")
    finally:
        client.close()

    def pick(text, name):
        m = re.search(name + r",(\d+)", text or "")
        return int(m.group(1)) if m else None

    lines.append("## machine checks")
    unit = outputs.get("unit_state", "")
    lines.append("RESULT emqx_active=%s" % ("YES" if "ActiveState=active" in unit else "NO"))
    m = re.search(r"NRestarts=(\d+)", unit)
    lines.append("RESULT nrestarts=%s" % (m.group(1) if m else "unknown"))
    m = re.search(r"ActiveEnterTimestamp=(.+)", unit)
    lines.append("RESULT active_since=%s" % (m.group(1).strip() if m else "unknown"))
    leaf = outputs.get("leaf", "")
    lines.append("RESULT served_serial_matches_approved=%s"
                 % ("YES" if APPROVED_SERIAL in leaf.replace("serial=", "").upper() else "NO"))
    lines.append("RESULT san_has_connect_ip=%s" % ("YES" if "8.140.23.253" in leaf else "NO"))
    m = re.search(r"handshake_ok_count=(\S+)", outputs.get("handshake_10", ""))
    lines.append("RESULT handshake_rate=%s" % (m.group(1) if m else "unknown"))
    lines.append("RESULT ssl_listener_running=%s"
                 % ("YES" if re.search(r"running\s*:\s*true", outputs.get("listener_ssl", "")) else "NO"))
    for name in ("not_authorized", "ssl_error", "ssl_closed"):
        a, b = pick(t0, name), pick(t1, name)
        lines.append("RESULT ssl_%s_delta_%ss=%s" % (name, SAMPLE_GAP, (b - a) if None not in (a, b) else "unknown"))
    lines.append("RESULT established_8883_t0=%s t1=%s" % (est0.strip() if "est0" in dir() else "?", est1.strip() if "est1" in dir() else "?"))
    lines.append("RESULT backup_baseline_cert_present=%s"
                 % ("YES" if "a91de074d8ca0954" in outputs.get("backup_point", "") else "NO"))
    lines.append("RESULT authn_recent_lines=%d" % len([r for r in outputs.get("authn_recent", "").splitlines() if r.strip()]))

    write(lines)
    print(redact("\n".join(lines[-30:])))
    return 0


def write(lines):
    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(redact("\n".join(lines)).rstrip() + "\n")


if __name__ == "__main__":
    raise SystemExit(main())
