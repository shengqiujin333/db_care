#!/usr/bin/env python
"""Second reliability observation: MQTT auth-rejection rate and its source (read-only).

Motivation: after the certificate replacement the 8883 listener shows nearly no
TLS errors but a growing `not_authorized` count, i.e. some client(s) complete TLS
and are then rejected by authentication. This probe measures whether that traffic
is continuous, captures the source addresses, and reads the broker's own log and
authentication configuration, without authenticating, publishing or changing state.
"""
import os
import re
import time

import paramiko

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
OUT = os.path.join(HERE, "02_auth_rejection_rate.txt")

CONNECT_TIMEOUT = 20
CMD_TIMEOUT = 20
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


def run(client, command):
    _in, out, err = client.exec_command(command, timeout=CMD_TIMEOUT)
    text = out.read().decode("utf-8", "replace")
    errtext = err.read().decode("utf-8", "replace")
    out.channel.recv_exit_status()
    return text + (("stderr:" + errtext) if errtext.strip() else "")


def counters(client):
    text = run(client, "emqx ctl listeners 2>&1 | grep -E '^(ssl|tcp):default|shutdown_count'")
    result = {}
    for block in text.split("tcp:default"):
        pass
    cur = None
    for row in text.splitlines():
        row = row.strip()
        if row.startswith("ssl:default"):
            cur = "ssl"
        elif row.startswith("tcp:default"):
            cur = "tcp"
        elif row.startswith("shutdown_count") and cur:
            result[cur] = row.split(":", 1)[1].strip()
    return result


def main():
    host, user, password = read_connection()
    lines = [
        "# MQTT auth-rejection rate on 8883 (read-only)",
        "started_utc=%s" % time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "scope=listener-counters,established-tcp,broker-log,auth-config",
        "forbidden=connect,publish,subscribe,mutation,restart,db,credential-login",
        "",
    ]
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(host, port=22, username=user, password=password, timeout=CONNECT_TIMEOUT,
                   banner_timeout=CONNECT_TIMEOUT, auth_timeout=CONNECT_TIMEOUT,
                   look_for_keys=False, allow_agent=False)
    lines.append("connect_result=OK")
    lines.append("")
    lines.append("--- [listener_counters_t0] %s" % time.strftime("%H:%M:%SZ", time.gmtime()))
    t0 = counters(client)
    lines.append("ssl=%s" % t0.get("ssl"))
    lines.append("tcp=%s" % t0.get("tcp"))
    lines.append("")

    # sample established connections to 8883 every 15s for 75s
    lines.append("--- [established_8883_samples]")
    lines.append("note: exported columns are remote address/port only; no payload is read")
    for i in range(6):
        sample = run(client, "ss -tn state established '( sport = :8883 )' 2>/dev/null | tail -n +2 | "
                             "awk '{print $4, $5}'")
        stamp = time.strftime("%H:%M:%SZ", time.gmtime())
        rows = [r for r in sample.splitlines() if r.strip()]
        lines.append("t=%s established=%d %s" % (stamp, len(rows), "|".join(rows[:10])))
        if i < 5:
            time.sleep(15)
    lines.append("")

    lines.append("--- [listener_counters_t1] %s" % time.strftime("%H:%M:%SZ", time.gmtime()))
    t1 = counters(client)
    lines.append("ssl=%s" % t1.get("ssl"))
    lines.append("tcp=%s" % t1.get("tcp"))
    lines.append("")

    def delta(name):
        def pick(text):
            m = re.search(name + r",(\d+)", text or "")
            return int(m.group(1)) if m else None
        a, b = pick(t0.get("ssl")), pick(t1.get("ssl"))
        if a is None or b is None:
            return "unknown"
        return "%d (t0=%d t1=%d over 75s)" % (b - a, a, b)

    lines.append("RESULT ssl_not_authorized_delta_75s=%s" % delta("not_authorized"))
    lines.append("RESULT ssl_ssl_error_delta_75s=%s" % delta("ssl_error"))
    lines.append("RESULT ssl_closed_delta_75s=%s" % delta("ssl_closed"))
    lines.append("")

    lines.append("--- [emqx_log_files]")
    lines.append(run(client, "ls -la /var/log/emqx/ 2>&1").rstrip())
    lines.append("")
    lines.append("--- [emqx_log_1_tail]")
    lines.append(run(client, "tail -60 /var/log/emqx/emqx.log.1 2>&1").rstrip())
    lines.append("")
    lines.append("--- [emqx_log_1_auth_and_tls]")
    lines.append(run(client, "grep -iE 'auth|ssl|handshake|certificate|not_authorized' "
                            "/var/log/emqx/emqx.log.1 2>&1 | tail -40").rstrip())
    lines.append("")
    lines.append("--- [auth_config]")
    lines.append(run(client, "emqx ctl conf show authentication 2>&1 | head -40").rstrip())
    lines.append("")
    lines.append("--- [listener_ssl_config]")
    lines.append(run(client, "emqx ctl conf show listeners.ssl.default 2>&1 | head -40").rstrip())
    lines.append("")
    lines.append("--- [journal_auth]")
    lines.append(run(client, "journalctl -u emqx --since '2026-11-25 10:58:15' --no-pager 2>/dev/null "
                            "| grep -iE 'auth|ssl|handshake|connected|client' | tail -20").rstrip())

    client.close()
    write(lines)
    print(redact("\n".join(lines)))
    return 0


def write(lines):
    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(redact("\n".join(lines)) + "\n")


if __name__ == "__main__":
    raise SystemExit(main())
