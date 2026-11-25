#!/usr/bin/env python
"""Fourth reliability window: platform observation after the upload-contract change.

Objects: existing platform 8.140.23.253 (EMQX 5.8.0 ssl:default 0.0.0.0:8883 and
Flask user_script.service on 0.0.0.0:5000). Window covers ~7h20m after the
2026-11-25 21:59:02 CST redeploy of /root/abr.py (upload gate removal).

Read-only: systemctl show / status probes / emqx ctl / openssl s_client / ss /
curl GET / journalctl / stat / sha256sum. No sudo, no restart, no deploy, no
config or data write, no database, no MQTT CONNECT, no POST to any endpoint.
Connection data comes from project material and is redacted.
"""
import os
import re
import time

import paramiko

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
OUT = os.path.join(HERE, "19_r4_platform_window.txt")

CONNECT_TIMEOUT = 20
CMD_TIMEOUT = 45
TOTAL_BUDGET = 300
SAMPLE_GAP = 120

CERTFILE = "/var/lib/emqx/certs/listeners/ssl/default/cert-2D2ADC29A9BE14AA"
CACERT = "/var/lib/emqx/certs/listeners/ssl/default/cacert-613B9023E1E698F0"
CERT_BACKUP = "/root/emqx-8883-cert-backup-20261125T025805Z"
APPROVED_SERIAL = "77ADCFD924BA0F87F845C81934D4A3F346D4BB83"
APPROVED_ABR_SHA = "a062b05f524dc8f6bce596e4571d339f697ef909b318503ad418ee0add24017c"
BASELINE_ABR_SHA = "534dd40aac40c00597b06710069cbcc70533650e01318e0637cd924851740c63"

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
             "| tail -6")

BLOCKS = [
    ("identity", "date -u +%Y-%m-%dT%H:%M:%SZ; date +%Y-%m-%dT%H:%M:%S%z; hostname; cat /proc/uptime"),
    ("unit_state",
     "systemctl show emqx -p ActiveState -p SubState -p NRestarts -p ActiveEnterTimestamp -p MemoryCurrent; "
     "echo '--'; "
     "systemctl show user_script -p ActiveState -p SubState -p NRestarts -p ActiveEnterTimestamp "
     "-p MemoryCurrent -p MainPID -p ExecMainStartTimestamp"),
    ("host_load", "cat /proc/loadavg; free -m | head -3; df -h / | tail -1; df -i / | tail -1"),
    ("emqx_status", "emqx ctl status 2>&1 | head -4"),
    ("broker", "emqx ctl broker 2>&1 | head -16"),
    ("listener_ssl", COUNTER_CMD),
    ("leaf", "openssl x509 -in %s -noout -subject -serial -dates -ext subjectAltName 2>&1" % CERTFILE),
    ("cert_file_stat", "stat -c 'mtime=%%y size=%%s mode=%%a' %s; sha256sum %s | cut -c1-16"
                       % (CERTFILE, CERTFILE)),
    ("handshake_10",
     "ok=0; t0=$(date +%%s%%N); for i in $(seq 1 10); do "
     "if timeout 10 openssl s_client -connect 127.0.0.1:8883 -verify_ip 8.140.23.253 "
     "-CAfile %s </dev/null 2>/dev/null | grep -q 'Verify return code: 0 (ok)'; "
     "then ok=$((ok+1)); fi; done; t1=$(date +%%s%%N); "
     "echo \"handshake_ok_count=$ok/10 elapsed_ms=$(( (t1-t0)/1000000 ))\"" % CACERT),
    ("established_8883", "ss -tn state established '( sport = :8883 )' 2>/dev/null | tail -n +2 | wc -l"),
    ("authn_recent", AUTHN_CMD),
    ("upload_service_change",
     "stat -c '%n mode=%a owner=%U:%G size=%s mtime=%y' /root/abr.py; "
     "sha256sum /root/abr.py; "
     "grep -c 'X-Upload-Token' /root/abr.py; grep -c 'UPLOAD_TOKEN' /root/abr.py; "
     "grep -n 'def upload_data' /root/abr.py; "
     "ls -l --time-style=+%Y-%m-%dT%H:%M:%S /root/abr.py* 2>/dev/null; "
     "ls -d /root/rd_upload_contract_* 2>/dev/null"),
    ("upload_post_status_counts",
     "echo '---since_20261125_2159---'; "
     "journalctl -u user_script --since '2026-11-25 21:59:03' --no-pager 2>/dev/null "
     "| grep -oE '\"POST /upload_data HTTP/1.1\" [0-9]{3}' | sort | uniq -c; "
     "echo '---whole_day_1126---'; "
     "journalctl -u user_script --since '2026-11-26 00:00:00' --no-pager 2>/dev/null "
     "| grep -oE '\"POST /upload_data HTTP/1.1\" [0-9]{3}' | sort | uniq -c; "
     "echo '---by_hour_since_2159---'; "
     "journalctl -u user_script --since '2026-11-25 21:59:03' --no-pager 2>/dev/null "
     "| grep -E '\"POST /upload_data HTTP/1.1\"' | awk '{print $3}' | cut -d: -f1 | sort | uniq -c"),
    ("upload_requests_recent",
     "journalctl -u user_script --since '2026-11-25 21:59:03' --no-pager 2>/dev/null "
     "| grep -E '\"POST /upload_data HTTP/1.1\"' | tail -30"),
    ("other_business_endpoints",
     "journalctl -u user_script --since '2026-11-25 21:59:03' --no-pager 2>/dev/null "
     "| grep -oE '\"(POST|GET) /[a-z_]+ HTTP/1.1\" [0-9]{3}' | sort | uniq -c | head -20"),
    ("flask_error_scan",
     "journalctl -u user_script --since '2026-11-25 21:59:03' --no-pager 2>/dev/null "
     "| grep -cE 'Traceback|\" 500 |\" 502 |\" 503 '; echo '--err_priority--'; "
     "journalctl -u user_script -p err --since '2026-11-25 21:59:03' --no-pager 2>/dev/null | tail -8"),
    ("db_connect_evidence",
     "journalctl -u user_script --since '2026-11-25 21:59:03' --no-pager 2>/dev/null "
     "| grep -cE 'mysql.connect|MySQLdb|DeprecationWarning:.*mysql'; echo '--mysql_socket--'; "
     "ss -ltn 2>/dev/null | grep -c ':3306 '"),
    ("service_ports",
     "ss -ltn 2>/dev/null | awk 'NR>1{print $4}' | sed 's/.*://' | sort -n | uniq | tr '\\n' ' '; echo"),
    ("http_semantics",
     "for p in 5000; do "
     "echo -n \"/login=\"; curl -s -o /dev/null -w '%{http_code}' --max-time 5 http://127.0.0.1:$p/login; echo; "
     "echo -n \"/upload_data_GET=\"; curl -s -o /dev/null -w '%{http_code}' --max-time 5 http://127.0.0.1:$p/upload_data; echo; "
     "echo -n \"/upload_data_OPTIONS=\"; curl -s -o /dev/null -w '%{http_code}' --max-time 5 -X OPTIONS http://127.0.0.1:$p/upload_data; echo; "
     "done; echo -n 'dashboard_status='; curl -s -o /dev/null -w '%{http_code}' --max-time 5 http://127.0.0.1:18083/status; echo"),
    ("rollback_points",
     "echo '--cert--'; stat -c 'dir=%n mode=%a owner=%U:%G' " + CERT_BACKUP + " 2>&1; "
     "sha256sum " + CERT_BACKUP + "/leaf.crt.bak 2>/dev/null | cut -c1-16; "
     "echo '--upload--'; ls -l --time-style=+%Y-%m-%dT%H:%M:%S /root/abr.py.bak_* 2>/dev/null; "
     "sha256sum /root/abr.py.bak_* 2>/dev/null; "
     "echo '--rollback_scripts--'; ls -l /root/rd_upload_contract_*/rollback_remote.sh "
     "/root/rd_upload_contract_*/verify_upload_contract.sh 2>/dev/null; "
     "cat /root/rd_upload_contract_*/BACKUP_SHA256SUMS.txt 2>/dev/null | cut -c1-16"),
    ("observability_presence",
     "echo '--running_services--'; systemctl list-units --type=service --state=running --no-pager "
     "--no-legend 2>/dev/null | awk '{print $1}' | grep -Ei 'export|prometh|telegraf|filebeat|zabbix|grafana|node|alert' || echo none; "
     "echo '--prometheus_config--'; ls -d /etc/prometheus /etc/telegraf /opt/prometheus 2>/dev/null || echo none; "
     "echo '--emqx_prometheus--'; grep -RiE 'prometheus|pushgateway' /var/lib/emqx/configs/cluster.hocon "
     "/etc/emqx/emqx.conf 2>/dev/null | head -4 || echo none; "
     "echo '--emqx_alarms--'; emqx ctl alarms 2>&1 | head -3; "
     "echo '--journald_retention--'; journalctl --disk-usage 2>/dev/null; "
     "echo '--emqx_log_files--'; ls -l --time-style=+%Y-%m-%dT%H:%M /var/log/emqx 2>/dev/null | head -8"),
]


def main():
    host, user, password = read_connection()
    started = time.time()
    lines = [
        "# fourth reliability window (timebase: 2026-11-25 21:59:02 CST redeploy of /root/abr.py)",
        "started_utc=%s" % time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "scope=service-stability,tls-handshake,auth-outcome,upload-endpoint-rate,error-rate,"
        "drift,rollback-point,observability",
        "forbidden=mutation,sudo,restart,deploy,database,write-endpoint,mqtt-connect",
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

    def run(command, timeout=CMD_TIMEOUT):
        _in, out, err = client.exec_command(command, timeout=timeout)
        return out.read().decode("utf-8", "replace") + err.read().decode("utf-8", "replace")

    outputs = {}
    t0 = est0 = t1 = est1 = ""
    try:
        for label, command in BLOCKS:
            if time.time() - started > TOTAL_BUDGET:
                lines.append("--- [%s] SKIPPED (budget)" % label)
                continue
            lines.append("--- [%s]" % label)
            try:
                outputs[label] = run(command)
                lines.append(outputs[label].rstrip())
            except Exception as exc:  # noqa: BLE001
                lines.append("exec_error=%s: %s" % (type(exc).__name__, exc))
            lines.append("")

        lines.append("--- [counter_delta_%ss]" % SAMPLE_GAP)
        t0 = run(COUNTER_CMD)
        est0 = run("ss -tn state established '( sport = :8883 )' 2>/dev/null | tail -n +2 | wc -l")
        lines.append("t0_utc=%s" % time.strftime("%H:%M:%SZ", time.gmtime()))
        lines.append(t0.rstrip())
        lines.append("t0_established=%s" % est0.strip())
        time.sleep(SAMPLE_GAP)
        t1 = run(COUNTER_CMD)
        est1 = run("ss -tn state established '( sport = :8883 )' 2>/dev/null | tail -n +2 | wc -l")
        lines.append("t1_utc=%s elapsed_s=%d" % (time.strftime("%H:%M:%SZ", time.gmtime()),
                                                 time.time() - started))
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
    emqx_block, _, flask_block = unit.partition("--")
    lines.append("RESULT emqx_active=%s" % ("YES" if "ActiveState=active" in emqx_block else "NO"))
    lines.append("RESULT flask_active=%s" % ("YES" if "ActiveState=active" in flask_block else "NO"))
    m = re.search(r"NRestarts=(\d+)", flask_block)
    lines.append("RESULT flask_nrestarts=%s" % (m.group(1) if m else "unknown"))
    m = re.search(r"ExecMainStartTimestamp=(.+)", flask_block)
    lines.append("RESULT flask_started=%s" % (m.group(1).strip() if m else "unknown"))
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
    lines.append("RESULT established_8883_t0=%s t1=%s" % (est0.strip(), est1.strip()))
    change = outputs.get("upload_service_change", "")
    lines.append("RESULT abr_sha_is_approved_candidate=%s" % ("YES" if APPROVED_ABR_SHA in change else "NO"))
    lines.append("RESULT abr_sha_is_defect_baseline=%s" % ("YES" if BASELINE_ABR_SHA in change else "NO"))
    lines.append("RESULT upload_token_gate_present=%s"
                 % ("YES" if ("X-Upload-Token" in change and not re.search(r"^0$", change, re.M)) else "NO"))
    err = outputs.get("flask_error_scan", "").strip().splitlines()
    lines.append("RESULT flask_traceback_or_5xx_lines=%s" % (err[0].strip() if err else "unknown"))
    lines.append("RESULT backup_baseline_cert_present=%s"
                 % ("YES" if "a91de074d8ca0954" in outputs.get("rollback_points", "") else "NO"))
    lines.append("RESULT upload_backup_present=%s"
                 % ("YES" if "534dd40aac40c005" in outputs.get("rollback_points", "") else "NO"))
    counts = outputs.get("upload_post_status_counts", "")
    lines.append("RESULT upload_status_breakdown=%s"
                 % (" | ".join(r.strip() for r in counts.splitlines() if re.match(r"^\s*\d+\s+", r)) or "none"))
    obs = outputs.get("observability_presence", "")
    lines.append("RESULT metrics_or_alerting_agent_found=%s"
                 % ("YES" if re.search(r"export|prometh|telegraf|filebeat|zabbix|alert", obs) else "NO"))

    write(lines)
    print(redact("\n".join(lines[-30:])))
    return 0


def write(lines):
    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(redact("\n".join(lines)).rstrip() + "\n")


if __name__ == "__main__":
    raise SystemExit(main())
