#!/usr/bin/env python
"""Attribute the 8883 ssl_error counter movement seen in the resumed window (read-only).

07_resume_window.txt observed ssl_error 4 -> 9 inside 97 s with no MQTT client connected.
Before calling that a regression (or dismissing it as scanner noise) this probe separates
the two candidate sources with an idle-vs-probe control:

  1. idle window: no local traffic at all -> does the counter still move?
  2. probe window: 5 local handshakes with the same CA/IP verification as the App
     -> does the counter move when only correct handshakes are made?

It also burst-samples the kernel socket table to record which peer addresses actually
appear on 8883, and how long-lived they are. No sudo, no capture, no restart, no
configuration change, no MQTT CONNECT.
"""
import os
import re
import time

import paramiko

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
OUT = os.path.join(HERE, "08_ssl_error_attribution.txt")

CACERT = "/var/lib/emqx/certs/listeners/ssl/default/cacert-613B9023E1E698F0"
CONNECT_TIMEOUT = 20
CMD_TIMEOUT = 40
IDLE_SECONDS = 120
SAMPLE_SECONDS = 60
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


COUNTER = ("emqx ctl listeners 2>&1 | sed -n '/ssl:default/,/^tcp:default/p' | "
           "grep -E 'shutdown_count|current_conn'")


def pick(text, name):
    m = re.search(name + r",(\d+)", text or "")
    return int(m.group(1)) if m else None


def main():
    host, user, password = read_connection()
    lines = ["# ssl_error attribution control (8883 certificate change follow-up)",
             "started_utc=%s" % time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
             "method=idle window vs local-correct-handshake window, plus peer socket sampling",
             "forbidden=mutation,sudo,restart,deploy,packet-capture,mqtt-connect", ""]
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    try:
        client.connect(host, port=22, username=user, password=password, timeout=CONNECT_TIMEOUT,
                       banner_timeout=CONNECT_TIMEOUT, auth_timeout=CONNECT_TIMEOUT,
                       look_for_keys=False, allow_agent=False)
    except Exception as exc:  # noqa: BLE001
        lines.append("connect_result=FAILED error=%s: %s" % (type(exc).__name__, exc))
        write(lines)
        print(redact("\n".join(lines)))
        return 2
    lines.append("connect_result=OK")

    def run(command, timeout=CMD_TIMEOUT):
        _in, out, err = client.exec_command(command, timeout=timeout)
        return (out.read() + err.read()).decode("utf-8", "replace")

    try:
        c0 = run(COUNTER)
        lines.append("--- [counters_start %s]" % time.strftime("%H:%M:%SZ", time.gmtime()))
        lines.append(c0.rstrip())

        # 1) idle: no traffic from this host at all
        time.sleep(IDLE_SECONDS)
        c1 = run(COUNTER)
        lines.append("--- [counters_after_idle_%ds @%s]" % (IDLE_SECONDS, time.strftime("%H:%M:%SZ", time.gmtime())))
        lines.append(c1.rstrip())

        # 2) peer sampling on 8883 (kernel socket table; no capture, no root)
        lines.append("--- [peer_sample_%ds]" % SAMPLE_SECONDS)
        peer_cmd = ("for i in $(seq 1 %d); do ss -tnH state syn-recv established "
                    "\"( sport = :8883 or dport = :8883 )\" 2>/dev/null; sleep 0.3; done "
                    "| awk '{print $4, $5}' | sort | uniq -c | sort -rn | head -25" % int(SAMPLE_SECONDS / 0.3))
        peers = run(peer_cmd, timeout=SAMPLE_SECONDS + 30)
        lines.append(peers.rstrip() or "(no sockets observed in sampling points)")
        c2 = run(COUNTER)
        lines.append("--- [counters_after_sampling @%s]" % time.strftime("%H:%M:%SZ", time.gmtime()))
        lines.append(c2.rstrip())

        # 3) probe window: only correct, verified handshakes
        hs = run("ok=0; for i in $(seq 1 5); do "
                 "if timeout 10 openssl s_client -connect 127.0.0.1:8883 -verify_ip 8.140.23.253 "
                 "-CAfile %s </dev/null 2>/dev/null | grep -q 'Verify return code: 0 (ok)'; "
                 "then ok=$((ok+1)); fi; done; echo \"handshake_ok_count=$ok/5\"" % CACERT)
        lines.append("--- [local_correct_handshakes]")
        lines.append(hs.rstrip())
        c3 = run(COUNTER)
        lines.append("--- [counters_after_correct_handshakes @%s]" % time.strftime("%H:%M:%SZ", time.gmtime()))
        lines.append(c3.rstrip())
    finally:
        client.close()

    lines.append("")
    lines.append("## machine checks")
    lines.append("RESULT ssl_error_idle_delta=%s" % (pick(c1, "ssl_error") - pick(c0, "ssl_error")))
    lines.append("RESULT ssl_error_sampling_delta=%s" % (pick(c2, "ssl_error") - pick(c1, "ssl_error")))
    lines.append("RESULT ssl_error_local_handshake_delta=%s" % (pick(c3, "ssl_error") - pick(c2, "ssl_error")))
    lines.append("RESULT not_authorized_idle_delta=%s" % (pick(c1, "not_authorized") - pick(c0, "not_authorized")))
    lines.append("RESULT ssl_closed_idle_delta=%s" % (pick(c1, "ssl_closed") - pick(c0, "ssl_closed")))
    write(lines)
    print(redact("\n".join(lines[-16:])))
    return 0


def write(lines):
    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(redact("\n".join(lines)).rstrip() + "\n")


if __name__ == "__main__":
    raise SystemExit(main())
