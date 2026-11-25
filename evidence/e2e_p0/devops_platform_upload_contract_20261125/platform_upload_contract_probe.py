#!/usr/bin/env python
"""CERT8883 loop repair node: bounded READ-ONLY inspection of the existing platform
that serves POST /upload_data (user_script.service / /root/abr.py).

Scope: identity, unit state, deployed object metadata/digest, upload handler
gate presence (values redacted), existing backups, cert/listener identity,
listening ports, HTTP route existence (GET/OPTIONS only, no POST), and the
recent read-only journal window.

No sudo, no writes, no restart, no service control, no DB, no POST, no credential echo.
"""
import os
import re
import time

import paramiko

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
OUT = os.path.join(HERE, "01_platform_upload_contract_state.txt")
SECRETS = []


def read_connection():
    path = None
    for name in os.listdir(ROOT):
        if name.startswith("服务器迁移记录"):
            path = os.path.join(ROOT, name)
            break
    if not path:
        raise SystemExit("migration record not found")
    text = open(path, "r", encoding="utf-8", errors="replace").read()
    ip = None
    for row in text.splitlines():
        if "服务器 IP" in row:
            found = re.findall(r"[0-9]{1,3}(?:\.[0-9]{1,3}){3}", row)
            if found:
                ip = found[-1]
                break
    cred = re.search(r"目标服务器：\s*([^\s/]+)\s*/\s*([^\s`]+)", text)
    if not ip or not cred:
        raise SystemExit("connection info not found")
    SECRETS.append(cred.group(2))
    return ip, cred.group(1), cred.group(2)


def redact(text):
    for value in SECRETS:
        if value:
            text = text.replace(value, "<secret-redacted>")
    text = re.sub(r"LTAI[0-9A-Za-z]{12,}", "<secret-redacted>", text)
    # mask any 32+ hex/opaque token literal that may appear in source excerpts
    text = re.sub(r"(?<=UPLOAD_TOKEN = )'[^']*'", "'<token-value-redacted>'", text)
    text = re.sub(r"(?<=UPLOAD_TOKEN = )\"[^\"]*\"", "\"<token-value-redacted>\"", text)
    return text


COMMANDS = [
    ("identity", "date -u +%Y-%m-%dT%H:%M:%SZ; date '+%Y-%m-%d %H:%M:%S %Z'; hostname; uptime"),
    ("unit_state",
     "systemctl show user_script -p Id -p Description -p ActiveState -p SubState "
     "-p ExecMainPID -p ExecMainStartTimestamp -p NRestarts -p ExecStart -p FragmentPath "
     "-p DropInPaths -p Restart -p Environment 2>&1"),
    ("deployed_object",
     "stat -c 'path=%n size=%s mode=%a uid=%u gid=%g inode=%i mtime=%y' /root/abr.py; "
     "sha256sum /root/abr.py; wc -l /root/abr.py; "
     "readlink -f /root/abr.py; ls -la /root/ | grep -i 'abr'"),
    ("handler_excerpt",
     "grep -n \"upload_data\" -A 40 /root/abr.py | head -80"),
    ("token_metadata",
     "grep -n 'UPLOAD_TOKEN' /root/abr.py | sed -e \"s/'[^']*'/\\'<token-value-redacted>\\'/g\"; "
     "grep -c 'X-Upload-Token' /root/abr.py; "
     "python3 - <<'PY'\n"
     "import re\n"
     "src=open('/root/abr.py',encoding='utf-8',errors='replace').read()\n"
     "m=re.search(r\"UPLOAD_TOKEN\\s*=\\s*(['\\\"])(.*?)\\1\",src)\n"
     "print('token_defined=%s' % bool(m))\n"
     "print('token_len=%s' % (len(m.group(2)) if m else 0))\n"
     "PY"),
    ("route_behaviour",
     "curl -sS -o /dev/null -w 'GET /upload_data -> %{http_code}\\n' --max-time 8 http://127.0.0.1:5000/upload_data; "
     "curl -sS -o /dev/null -w 'OPTIONS /upload_data -> %{http_code}\\n' -X OPTIONS --max-time 8 http://127.0.0.1:5000/upload_data; "
     "curl -sS -o /dev/null -w 'GET / -> %{http_code}\\n' --max-time 8 http://127.0.0.1:5000/; "
     "curl -sS -o /dev/null -w 'GET /login -> %{http_code}\\n' --max-time 8 http://127.0.0.1:5000/login"),
    ("listeners",
     "ss -ltnp 2>/dev/null | awk 'NR==1 || /:(1883|8883|8083|8084|18083|5000|3306)\\b/'"),
    ("emqx_state",
     "systemctl show emqx -p ActiveState -p SubState -p NRestarts -p ExecMainStartTimestamp 2>&1; "
     "emqx ctl listeners 2>&1 | sed -n '1,80p'"),
    ("tls_leaf",
     "echo | openssl s_client -connect 127.0.0.1:8883 -servername 8.140.23.253 2>/dev/null "
     "| openssl x509 -noout -subject -issuer -serial -dates -ext subjectAltName"),
    ("journal_today",
     "journalctl -u user_script --since '2026-11-25 00:00:00' --no-pager -o short-iso 2>&1 | tail -120"),
    ("journal_upload_codes",
     "journalctl -u user_script --since '2026-11-25 00:00:00' --no-pager 2>&1 "
     "| grep -c 'POST /upload_data' || true; "
     "journalctl -u user_script --since '2026-11-25 00:00:00' --no-pager 2>&1 "
     "| grep 'POST /upload_data' | awk '{print $NF}' | sort | uniq -c || true"),
    ("cert_files_mtime",
     "ls -la /var/lib/emqx/certs/listeners/ssl/default/ 2>/dev/null"),
]


def main():
    ip, user, password = read_connection()
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(ip, username=user, password=password, timeout=15,
                   allow_agent=False, look_for_keys=False)
    lines = [
        "# read-only platform inspection for POST /upload_data deployment object",
        "started_utc=%s" % time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "mode=READ_ONLY",
        "forbidden=write,sudo,restart,deploy,database,post-request,secret-output",
        "",
    ]
    try:
        for name, cmd in COMMANDS:
            transport = client.get_transport()
            channel = transport.open_session()
            channel.settimeout(25)
            channel.exec_command(cmd)
            chunks = []
            deadline = time.time() + 25
            while time.time() < deadline:
                if channel.recv_ready():
                    chunks.append(channel.recv(65536))
                elif channel.exit_status_ready():
                    while channel.recv_ready():
                        chunks.append(channel.recv(65536))
                    break
                else:
                    time.sleep(0.15)
            out = b"".join(chunks).decode("utf-8", "replace")
            lines.append("=" * 70)
            lines.append("## " + name)
            lines.append(out.rstrip() or "<empty>")
            lines.append("")
    finally:
        client.close()
    body = redact("\n".join(lines))
    with open(OUT, "w", encoding="utf-8") as fh:
        fh.write(body + "\n")
    print("wrote", OUT, len(body), "bytes")


if __name__ == "__main__":
    main()
