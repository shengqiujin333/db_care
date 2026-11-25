#!/usr/bin/env python
"""Read-only deployment-state verification of the existing platform service.

Purpose: confirm, after the 8883 certificate change, that the *deployed* artifact
still matches the approved change record (no drift), that the listener serves the
new leaf, that the rollback point is intact, and that unrelated services did not
regress.

Boundary:
  * host/account/password are read from the project material (服务器迁移记录.md);
    never echoed, never written to evidence;
  * commands are read-only: status/list/digest/openssl handshake/curl GET;
  * no sudo, no file or config write, no service start/stop/restart, no deploy,
    no database connection, no write endpoint, no private key output;
  * every command is time-bounded and the session has a total budget;
  * output is redacted before being written.

This script does not replace deployment; it re-verifies the already-applied
change so the deployment operations result has a reproducible record.
"""
import base64
import hashlib
import os
import re
import time

import paramiko

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
OUT = os.path.join(HERE, "08_deploy_state.txt")

CONNECT_TIMEOUT = 20
CMD_TIMEOUT = 20
TOTAL_BUDGET = 150

# approved change record (deploy/emqx_8883_tls/container.yaml)
EXPECT_CERT_SHA256 = "27964ca8ed1c15964450a01240ebf0185e813332214c6f6f61cc4239cb62ccaa"
EXPECT_SERIAL = "77ADCFD924BA0F87F845C81934D4A3F346D4BB83"
BASELINE_CERT_SHA256 = "a91de074d8ca09549abafc7787019ab2e68b4c8042e04130e9c27f610a0855f9"
BASELINE_CA_SHA256 = "db910d3c363aa3c090a6e74adb115bcf17f780e849575de60e864aa11f213200"
BACKUP_DIR = "/root/emqx-8883-cert-backup-20261125T025805Z"
CERTFILE = "/var/lib/emqx/certs/listeners/ssl/default/cert-2D2ADC29A9BE14AA"
KEYFILE = "/var/lib/emqx/certs/listeners/ssl/default/key-6E6F6ED56285A948"
CACERT = "/var/lib/emqx/certs/listeners/ssl/default/cacert-613B9023E1E698F0"
CONF = "/var/lib/emqx/configs/cluster.hocon"

COMMANDS = [
    ("identity", "date -u +%Y-%m-%dT%H:%M:%SZ; date +%Y-%m-%dT%H:%M:%S%z; hostname; uname -srm"),
    ("units",
     "for u in emqx user_script; do echo \"$u active=$(systemctl is-active $u 2>&1) enabled=$(systemctl is-enabled $u 2>&1)\"; done"),
    ("unit_detail",
     "systemctl show emqx -p ActiveState -p SubState -p ExecMainStartTimestamp -p NRestarts 2>&1"),
    ("emqx_status", "emqx ctl status 2>&1 | head -6"),
    ("emqx_package",
     "dpkg-query -W -f='${Package} ${Version} ${Status}\\n' emqx 2>&1; "
     "stat -c 'cluster.hocon mtime=%y size=%s' " + CONF + " 2>&1"),
    ("emqx_broker", "emqx ctl broker 2>&1 | head -8"),
    ("emqx_listeners", "emqx ctl listeners 2>&1 | grep -E '^(tcp|ssl|ws|wss):default|listen_on|running|current_conn' | head -40"),
    ("deployed_files",
     "ls -l --full-time %s %s %s 2>&1" % (CERTFILE, KEYFILE, CACERT)),
    ("deployed_digests", "sha256sum %s %s %s 2>&1" % (CERTFILE, KEYFILE, CACERT)),
    ("deployed_leaf",
     "openssl x509 -in %s -noout -subject -issuer -serial -dates -fingerprint -sha256 -ext subjectAltName 2>&1" % CERTFILE),
    ("keypair_match",
     "echo -n 'cert_pub_sha256='; openssl x509 -in %s -noout -pubkey 2>/dev/null | openssl sha256 2>/dev/null; "
     "echo -n 'key_pub_sha256='; openssl pkey -in %s -pubout 2>/dev/null | openssl sha256 2>/dev/null" % (CERTFILE, KEYFILE)),
    ("listener_config",
     "grep -n -A5 'ssl_options' %s 2>&1 | head -20" % CONF),
    ("backup_state",
     "stat -c 'backup_dir=%n mode=%a owner=%U:%G mtime=%y' " + BACKUP_DIR + " 2>&1; ls -l --full-time "
     + BACKUP_DIR + " 2>&1; cat " + BACKUP_DIR + "/SHA256SUMS.txt 2>&1"),
    ("local_handshake",
     "timeout 20 openssl s_client -connect 127.0.0.1:8883 -verify_ip 8.140.23.253 -CAfile %s </dev/null 2>&1 "
     "| grep -E 'subject=|issuer=|Verify return code' | head -5" % CACERT),
    ("local_handshake_serial",
     "timeout 20 openssl s_client -connect 127.0.0.1:8883 -CAfile %s </dev/null 2>/dev/null "
     "| openssl x509 -noout -serial 2>/dev/null" % CACERT),
    ("ports",
     "for p in 1883 8883 8083 8084 18083 5000 3306; do "
     "if timeout 5 bash -c \"exec 3<>/dev/tcp/127.0.0.1/$p\" 2>/dev/null; then echo \"PORT $p OPEN\"; "
     "else echo \"PORT $p CLOSED\"; fi; done; "
     "timeout 10 curl -s -o /dev/null -w 'flask_login_get_http=%{http_code}\\n' http://127.0.0.1:5000/login; "
     "timeout 10 curl -s -o /dev/null -w 'dashboard_http=%{http_code}\\n' http://127.0.0.1:18083/status"),
    ("emqx_recent_log",
     "journalctl -u emqx --since '2026-11-25 10:50:00' --no-pager -n 20 2>&1 | tail -20"),
]

PASSWORDS = []


def read_connection():
    path = None
    for name in os.listdir(ROOT):
        if name.startswith("服务器迁移记录") or name.startswith("\u670d\u52a1\u5668"):
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


def main():
    host, user, password = read_connection()
    started = time.time()
    lines = [
        "# read-only deployment-state verification (8883 certificate change)",
        "started_utc=%s" % time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "host=%s port=22 transport=paramiko/%s" % (host, paramiko.__version__),
        "scope=listener/deployed-cert/backup/config-reference/ports/HTTP/recent-log",
        "forbidden=mutation,sudo,restart,deploy,database,write-endpoint,private-key-output",
        "",
    ]

    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    try:
        client.connect(host, port=22, username=user, password=password,
                       timeout=CONNECT_TIMEOUT, banner_timeout=CONNECT_TIMEOUT,
                       auth_timeout=CONNECT_TIMEOUT, look_for_keys=False, allow_agent=False)
    except Exception as exc:  # noqa: BLE001
        lines.append("connect_result=FAILED after=%.1fs error=%s: %s" % (time.time() - started, type(exc).__name__, exc))
        write(lines)
        print(redact("\n".join(lines)))
        return 2
    lines.append("connect_result=OK after=%.1fs" % (time.time() - started))
    lines.append("")

    outputs = {}
    for label, command in COMMANDS:
        if time.time() - started > TOTAL_BUDGET:
            lines.append("--- [%s] SKIPPED (total budget exhausted) ---" % label)
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

    # --- machine checks -------------------------------------------------
    digest_text = outputs.get("deployed_digests", "")
    cert_digest = ""
    for row in digest_text.splitlines():
        parts = row.split()
        if len(parts) == 2 and parts[1].endswith("cert-2D2ADC29A9BE14AA"):
            cert_digest = parts[0]
    ca_digest = ""
    for row in digest_text.splitlines():
        parts = row.split()
        if len(parts) == 2 and "cacert-" in parts[1]:
            ca_digest = parts[0]
    leaf_text = outputs.get("deployed_leaf", "")
    serial_match = re.search(r"serial=([0-9A-F]+)", leaf_text)
    served = re.search(r"serial=([0-9A-F]+)", outputs.get("local_handshake_serial", ""))
    backup_text = outputs.get("backup_state", "") + outputs.get("backup_sha", "")

    lines.append("## machine checks")
    lines.append("RESULT deployed_cert_sha256=%s" % cert_digest)
    lines.append("RESULT deployed_cert_matches_approved=%s" % ("YES" if cert_digest == EXPECT_CERT_SHA256 else "NO"))
    lines.append("RESULT deployed_serial=%s" % (serial_match.group(1) if serial_match else "unknown"))
    lines.append("RESULT deployed_serial_matches_approved=%s" % (
        "YES" if serial_match and serial_match.group(1) == EXPECT_SERIAL else "NO"))
    lines.append("RESULT served_serial=%s" % (served.group(1) if served else "unknown"))
    lines.append("RESULT served_serial_matches_deployed=%s" % (
        "YES" if serial_match and served and serial_match.group(1) == served.group(1) else "NO"))
    lines.append("RESULT ca_digest_unchanged=%s" % ("YES" if ca_digest == BASELINE_CA_SHA256 else "NO"))
    lines.append("RESULT backup_contains_baseline_cert=%s" % (
        "YES" if BASELINE_CERT_SHA256 in backup_text else "NO"))
    pub_hashes = re.findall(r"SHA2-256\(stdin\)= ([0-9a-f]{64})", outputs.get("keypair_match", ""))
    keypair_ok = len(pub_hashes) == 2 and pub_hashes[0] == pub_hashes[1]
    lines.append("RESULT keypair_match=%s" % ("YES" if keypair_ok else "NO"))
    ports = outputs.get("ports", "")
    lines.append("RESULT eight883_open=%s" % ("YES" if "PORT 8883 OPEN" in ports else "NO"))
    lines.append("RESULT all_expected_ports_open=%s" % (
        "YES" if all(("PORT %d OPEN" % p) in ports for p in (1883, 8883, 8083, 8084, 18083, 5000, 3306)) else "NO"))
    ssl_running = "ssl:default" in outputs.get("emqx_listeners", "") and "running         : true" in outputs.get("emqx_listeners", "")
    lines.append("RESULT ssl_listener_running=%s" % ("YES" if ssl_running else "NO"))
    overall = all([
        cert_digest == EXPECT_CERT_SHA256,
        bool(serial_match and serial_match.group(1) == EXPECT_SERIAL),
        bool(served and served.group(1) == EXPECT_SERIAL),
        ca_digest == BASELINE_CA_SHA256,
        BASELINE_CERT_SHA256 in backup_text,
        keypair_ok,
        "PORT 8883 OPEN" in ports,
        ssl_running,
    ])
    lines.append("RESULT overall=%s" % ("DEPLOYED_VERIFIED" if overall else "CHECK_FAILED"))
    write(lines)
    print(redact("\n".join(lines)))
    return 0 if overall else 1


def write(lines):
    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(redact("\n".join(lines)) + "\n")


if __name__ == "__main__":
    raise SystemExit(main())
