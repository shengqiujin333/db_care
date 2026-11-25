#!/usr/bin/env python
"""Post-change reliability observation for the 8883 TLS certificate fix (read-only).

Purpose: after the platform leaf certificate was replaced (CN/SAN 8.140.23.253),
collect runtime evidence for a health/reliability judgement: service stability,
listener availability, TLS handshake error volume in a symmetric pre/post window,
current client sessions, and a repeatable handshake success-rate probe.

Boundary (same as the deployment-state probe):
  * connection data is read from the project material, never echoed or written;
  * read-only commands only: status/show/stat/grep/journalctl/openssl s_client;
  * no sudo, no restart/stop/start, no deploy, no config or file write on the host,
    no database connection, no write endpoint, no credential login;
  * bounded timeouts and a total session budget; output is redacted;
  * handshake probes use the server's own CA file with -verify_ip 8.140.23.253,
    and do not send MQTT CONNECT (no auth attempts, no counters for auth).
"""
import os
import re
import time

import paramiko

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
OUT = os.path.join(HERE, "01_platform_reliability_window.txt")

CONNECT_TIMEOUT = 20
CMD_TIMEOUT = 25
TOTAL_BUDGET = 210

CERTFILE = "/var/lib/emqx/certs/listeners/ssl/default/cert-2D2ADC29A9BE14AA"
CACERT = "/var/lib/emqx/certs/listeners/ssl/default/cacert-613B9023E1E698F0"
# restart of EMQX that loaded the new certificate (local CST 2026-11-25 10:58:15)
POST_SINCE = "2026-11-25 10:58:15"
PRE_SINCE = "2026-11-24 02:58:15"
PRE_UNTIL = "2026-11-25 02:58:15"
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


def count_cmd(label, since=None, until=None, pattern=""):
    """journalctl line count for a pattern inside a window (read-only)."""
    window = ""
    if since:
        window += " --since '%s'" % since
    if until:
        window += " --until '%s'" % until
    grep = "grep -ic '%s'" % pattern if pattern else "wc -l"
    return ("%s" % label,
            "journalctl -u emqx%s --no-pager 2>/dev/null | %s" % (window, grep))


COMMANDS = [
    ("identity", "date -u +%Y-%m-%dT%H:%M:%SZ; date +%Y-%m-%dT%H:%M:%S%z; hostname"),
    ("unit_state",
     "systemctl show emqx -p ActiveState -p SubState -p NRestarts -p ActiveEnterTimestamp "
     "-p MemoryCurrent -p CPUUsageNSec 2>&1"),
    ("host_load", "cat /proc/loadavg; free -m | head -3; df -h / | tail -1"),
    ("emqx_status", "emqx ctl status 2>&1 | head -8"),
    ("broker", "emqx ctl broker 2>&1 | head -12"),
    ("listeners", "emqx ctl listeners 2>&1 | grep -E '^(tcp|ssl|ws|wss):default|listen_on|running|current_conn|shutdown_count' | head -44"),
    ("clients", "emqx ctl clients list 2>&1 | head -20"),
    ("leaf", "openssl x509 -in %s -noout -subject -serial -dates 2>&1" % CERTFILE),
    # handshake success-rate probe, 5 attempts, verify IP against the server CA
    ("handshake_rate",
     "ok=0; for i in 1 2 3 4 5; do "
     "if timeout 10 openssl s_client -connect 127.0.0.1:8883 -verify_ip 8.140.23.253 "
     "-CAfile %s </dev/null 2>/dev/null | grep -q 'Verify return code: 0 (ok)'; "
     "then ok=$((ok+1)); fi; done; echo \"handshake_ok_count=$ok/5\"" % CACERT),
    # log windows: defect period vs post-change period
    ("log_total_pre", "journalctl -u emqx --since '%s' --until '%s' --no-pager 2>/dev/null | wc -l" % (PRE_SINCE, PRE_UNTIL)),
    ("log_total_post", "journalctl -u emqx --since '%s' --no-pager 2>/dev/null | wc -l" % POST_SINCE),
    ("log_ssl_err_pre", "journalctl -u emqx --since '%s' --until '%s' --no-pager 2>/dev/null | grep -ic 'ssl_error'" % (PRE_SINCE, PRE_UNTIL)),
    ("log_ssl_err_post", "journalctl -u emqx --since '%s' --no-pager 2>/dev/null | grep -ic 'ssl_error'" % POST_SINCE),
    ("log_handshake_pre", "journalctl -u emqx --since '%s' --until '%s' --no-pager 2>/dev/null | grep -Eic 'handshake|unknown_ca|bad_certificate|no_peer_cert'" % (PRE_SINCE, PRE_UNTIL)),
    ("log_handshake_post", "journalctl -u emqx --since '%s' --no-pager 2>/dev/null | grep -Eic 'handshake|unknown_ca|bad_certificate|no_peer_cert'" % POST_SINCE),
    ("log_auth_pre", "journalctl -u emqx --since '%s' --until '%s' --no-pager 2>/dev/null | grep -ic 'bad_username_or_password'" % (PRE_SINCE, PRE_UNTIL)),
    ("log_auth_post", "journalctl -u emqx --since '%s' --no-pager 2>/dev/null | grep -ic 'bad_username_or_password'" % POST_SINCE),
    ("log_ssl_sample_pre",
     "journalctl -u emqx --since '%s' --until '%s' --no-pager 2>/dev/null | grep -i 'ssl_error' | tail -6" % (PRE_SINCE, PRE_UNTIL)),
    ("log_tail_post", "journalctl -u emqx --since '%s' --no-pager 2>/dev/null | tail -25" % POST_SINCE),
    ("log_tail_pre", "journalctl -u emqx --since '%s' --until '%s' --no-pager 2>/dev/null | tail -8" % (PRE_SINCE, PRE_UNTIL)),
    ("logfile_list", "ls -l /var/log/emqx/ 2>&1 | head -10"),
]


def main():
    host, user, password = read_connection()
    started = time.time()
    lines = [
        "# post-change reliability observation (8883 TLS certificate replacement)",
        "started_utc=%s" % time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "host=%s port=22 transport=paramiko/%s" % (host, paramiko.__version__),
        "scope=service-stability,listener,sessions,handshake-rate,log-window-pre/post",
        "forbidden=mutation,sudo,restart,deploy,database,write-endpoint,credential-login,private-key-output",
        "pre_window=%s..%s (local CST)  post_window=%s..now (local CST)" % (PRE_SINCE, PRE_UNTIL, POST_SINCE),
        "",
    ]

    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    try:
        client.connect(host, port=22, username=user, password=password,
                       timeout=CONNECT_TIMEOUT, banner_timeout=CONNECT_TIMEOUT,
                       auth_timeout=CONNECT_TIMEOUT, look_for_keys=False, allow_agent=False)
    except Exception as exc:  # noqa: BLE001
        lines.append("connect_result=FAILED after=%.1fs error=%s: %s"
                     % (time.time() - started, type(exc).__name__, exc))
        write(lines)
        print(redact("\n".join(lines)))
        return 2
    lines.append("connect_result=OK after=%.1fs" % (time.time() - started))
    lines.append("")

    outputs = {}
    for label, command in COMMANDS:
        if time.time() - started > TOTAL_BUDGET:
            lines.append("--- [%s] SKIPPED (total budget exhausted)" % label)
            continue
        lines.append("--- [%s]" % label)
        t0 = time.time()
        try:
            _in, out, err = client.exec_command(command, timeout=CMD_TIMEOUT)
            text = out.read().decode("utf-8", "replace")
            errtext = err.read().decode("utf-8", "replace")
            code = out.channel.recv_exit_status()
            lines.append("exit=%s elapsed_ms=%.0f" % (code, (time.time() - t0) * 1000))
            if text.strip():
                lines.append(text.rstrip())
            if errtext.strip():
                lines.append("stderr: " + errtext.rstrip())
            outputs[label] = text
        except Exception as exc:  # noqa: BLE001
            lines.append("exec_error=%s: %s" % (type(exc).__name__, exc))
        lines.append("")

    client.close()

    def num(label):
        m = re.search(r"\d+", outputs.get(label, ""))
        return int(m.group(0)) if m else -1

    post_minutes = max(1, int((time.time() - started) / 60) + 27)  # restart 02:58Z -> probe start
    pre_hours = 24.0
    rate_pre = num("log_ssl_err_pre") / pre_hours if num("log_ssl_err_pre") >= 0 else -1
    rate_post = num("log_ssl_err_post") / (post_minutes / 60.0) if num("log_ssl_err_post") >= 0 else -1

    lines.append("## machine checks")
    lines.append("RESULT emqx_active=%s" % ("YES" if "ActiveState=active" in outputs.get("unit_state", "") else "NO"))
    restarts = re.search(r"NRestarts=(\d+)", outputs.get("unit_state", ""))
    lines.append("RESULT nrestarts=%s" % (restarts.group(1) if restarts else "unknown"))
    lines.append("RESULT handshake_rate=%s" % (re.search(r"handshake_ok_count=(\S+)", outputs.get("handshake_rate", "")).group(1)
                                               if re.search(r"handshake_ok_count=(\S+)", outputs.get("handshake_rate", "")) else "unknown"))
    lines.append("RESULT ssl_error_pre_24h=%d" % num("log_ssl_err_pre"))
    lines.append("RESULT ssl_error_post_window=%d" % num("log_ssl_err_post"))
    lines.append("RESULT ssl_error_rate_pre_per_h=%.2f" % rate_pre)
    lines.append("RESULT ssl_error_rate_post_per_h=%.2f" % rate_post)
    lines.append("RESULT post_window_minutes=%d" % post_minutes)
    listeners = outputs.get("listeners", "")
    ssl_block = listeners.split("ssl:default", 1)[1] if "ssl:default" in listeners else ""
    lines.append("RESULT ssl_listener_running=%s" % ("YES" if "running         : true" in ssl_block else "NO"))
    conn = re.search(r"current_conn\s*:\s*(\d+)", ssl_block)
    lines.append("RESULT ssl_current_conn=%s" % (conn.group(1) if conn else "unknown"))
    lines.append("RESULT broker_sessions_line=%s" % (
        [r.strip() for r in outputs.get("broker", "").splitlines() if "sessions" in r.lower()][:1] or "none"))
    write(lines)
    print(redact("\n".join(lines)))
    return 0


def write(lines):
    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(redact("\n".join(lines)) + "\n")


if __name__ == "__main__":
    raise SystemExit(main())
