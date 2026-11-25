#!/usr/bin/env python
"""Post-deployment state drift check for the /upload_data acceptance fix.

Read-only on the platform: file identity, service unit, handler gate absence,
backup/rollback point, listener/port regression, 8883 certificate regression and
the upload request window after the restart. It sends only non-DB-reaching
bodies (`{}` and `{"probe":1}`) and never touches MySQL.

Exit code 0 only when every judged criterion passes; otherwise 1 so a stale
deployment cannot be mistaken for the approved candidate.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import apply_driver as d  # noqa: E402
import paramiko  # noqa: E402

EXPECT_SHA = "a062b05f524dc8f6bce596e4571d339f697ef909b318503ad418ee0add24017c"
EXPECT_SIZE = 8688
EXPECT_LINES = 250
BASELINE_SHA = "534dd40aac40c00597b06710069cbcc70533650e01318e0637cd924851740c63"

CMDS = [
    ("deployed_object",
     "sha256sum /root/abr.py; stat -c 'path=%n size=%s mode=%a uid=%u gid=%g "
     "inode=%i mtime=%y' /root/abr.py; wc -l < /root/abr.py"),
    ("gate_absence",
     "grep -c 'X-Upload-Token' /root/abr.py || true; "
     "grep -c 'UPLOAD_TOKEN' /root/abr.py || true; "
     "grep -n 'def upload_data' /root/abr.py"),
    ("unit",
     "systemctl show user_script -p ActiveState -p SubState -p NRestarts "
     "-p ExecMainPID -p ExecMainStartTimestamp -p ExecStart"),
    ("rollback_point",
     "sha256sum /root/abr.py.bak_20261125T135902Z; "
     "stat -c '%n %s %a %U:%G' /root/abr.py.bak_20261125T135902Z; "
     "ls /root/rd_upload_contract_20261125T135855Z/rollback_remote.sh"),
    ("ports",
     "ss -ltn | awk 'NR==1 || $4 ~ /:(5000|8883|1883|18083)$/'"),
    ("emqx",
     "systemctl is-active emqx; emqx ctl listeners 2>&1 | grep -E 'ssl:default|listen_on|running|acceptors' | head -12"),
    ("tls_leaf",
     "openssl x509 -in /var/lib/emqx/certs/listeners/ssl/default/cert-2D2ADC29A9BE14AA "
     "-noout -serial -subject -dates -ext subjectAltName 2>&1"),
    ("local_http",
     "curl -s -o /dev/null -w 'GET_upload=%{http_code}\\n' "
     "-X GET http://127.0.0.1:5000/upload_data; "
     "curl -s -o /dev/null -w 'OPTIONS_upload=%{http_code}\\n' "
     "-X OPTIONS http://127.0.0.1:5000/upload_data; "
     "curl -s -o /dev/null -w 'GET_login=%{http_code}\\n' -X GET http://127.0.0.1:5000/login; "
     "curl -s -w '\\nEMPTY=%{http_code}\\n' -H 'Content-Type: application/json' "
     "-d '{}' http://127.0.0.1:5000/upload_data; "
     "curl -s -w '\\nPROBE=%{http_code}\\n' -H 'Content-Type: application/json' "
     "-d '{\"probe\":1}' http://127.0.0.1:5000/upload_data; "
     "curl -s -w '\\nPROBE_TOKEN=%{http_code}\\n' -H 'Content-Type: application/json' "
     "-H 'X-Upload-Token: probe-invalid' -d '{\"probe\":1}' "
     "http://127.0.0.1:5000/upload_data"),
    ("upload_window",
     "journalctl -u user_script --since '2026-11-25 21:59:02' --no-pager 2>&1 "
     "| grep 'POST /upload_data' | tail -15; echo '-- end --'"),
    ("service_errors",
     "journalctl -u user_script --since '2026-11-25 21:59:02' --no-pager -p err 2>&1 | tail -5; "
     "echo '-- end --'"),
]


def judge(blocks):
    text = "\n".join(blocks.values())
    checks = [
        ("deployed_sha_matches_candidate", EXPECT_SHA in text),
        ("deployed_size_%d" % EXPECT_SIZE, "size=%d" % EXPECT_SIZE in text),
        ("gate_constant_absent", "gate_absence" in text),
        ("unit_active", "ActiveState=active" in blocks.get("unit", "")),
        ("unit_no_restart_loop", "NRestarts=0" in blocks.get("unit", "")),
        ("backup_baseline_present", BASELINE_SHA in blocks.get("rollback_point", "")),
        ("ports_5000_8883_open", "5000" in blocks.get("ports", "")
         and "8883" in blocks.get("ports", "")),
        ("emqx_ssl_running",
         "active" in blocks.get("emqx", "")
         and "listen_on       : 0.0.0.0:8883" in blocks.get("emqx", "")),
        ("cert_serial_unchanged",
         "77ADCFD924BA0F87F845C81934D4A3F346D4BB83" in blocks.get("tls_leaf", "")),
        ("san_has_target", "8.140.23.253" in blocks.get("tls_leaf", "")),
        ("route_semantics_unchanged",
         "GET_upload=405" in blocks.get("local_http", "")
         and "OPTIONS_upload=200" in blocks.get("local_http", "")
         and "GET_login=405" in blocks.get("local_http", "")),
        ("no_token_401_anymore",
         "PROBE=400" in blocks.get("local_http", "")
         and "PROBE_TOKEN=400" in blocks.get("local_http", "")),
        ("body_validation_kept", "No json data" in blocks.get("local_http", "")),
    ]
    # the gate-absence counters must both be 0
    ga = blocks.get("gate_absence", "").split("\n")
    counters = [ln for ln in ga if ln.strip().isdigit()]
    checks.append(("gate_counters_zero", len(counters) >= 2 and all(c.strip() == "0" for c in counters[:2])))
    return checks


def main():
    ip, user, pwd = d.read_connection()
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(ip, username=user, password=pwd, timeout=15,
                   allow_agent=False, look_for_keys=False)
    blocks = {}
    try:
        for name, cmd in CMDS:
            rc, out, err = d.run(client, cmd, timeout=60)
            blocks[name] = "## %s (rc=%s)\n%s%s" % (name, rc, out, err)
    finally:
        client.close()

    checks = judge(blocks)
    failed = [n for n, ok in checks if not ok]
    summary = "\n".join("  %-36s %s" % (n, "OK" if ok else "FAIL") for n, ok in checks)
    head = ("# post-deploy state drift check (read-only)\n"
            "# judged against candidate sha256 %s\n\n" % EXPECT_SHA)
    tail = ("\n== summary ==\n%s\nVERDICT=%s\n"
            % (summary, "PASS" if not failed else "FAIL " + ",".join(failed)))
    body = d.redact(head + "\n".join(blocks.values()) + tail)

    target = os.path.join(HERE, "16_state_drift_check.txt")
    with open(target, "w", encoding="utf-8") as fh:
        fh.write(body)
    print(summary)
    print("VERDICT=%s" % ("PASS" if not failed else "FAIL"))
    print("wrote", target)
    return 0 if not failed else 1


if __name__ == "__main__":
    sys.exit(main())
