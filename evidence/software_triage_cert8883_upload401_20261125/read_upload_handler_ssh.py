#!/usr/bin/env python
"""CERT8883-UPLOAD-04 failure analysis: bounded read-only inspection of the deployed
upload handler on the existing service.

Purpose: distinguish competing hypotheses for the measured `POST /upload_data` 401
(credential/token requirement vs. gateway identity requirement vs. verification
state). Only READ operations are performed: stat + read of /root/abr.py. No sudo,
no writes, no service control, no database, no write endpoint, no credential login.

The deployed source may contain credentials. This script prints only the
`/upload_data` handler, the helper functions it calls, and nearby auth-related
definitions, and redacts secret-looking literals BEFORE anything is returned.

Connection data comes from the project material; secrets are never echoed.
"""
import os
import re
import time

import paramiko

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
OUT = os.path.join(HERE, "01_abr_upload_handler_redacted.txt")

CONNECT_TIMEOUT = 20
CMD_TIMEOUT = 30

PASSWORDS = []


def read_connection():
    path = None
    for name in os.listdir(ROOT):
        if name.startswith("服务器"):
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
    # collect any additional literal passwords named in the migration record
    for m in re.finditer(r"(?:root 密码|密码)[：: ]+([^\s，,。|]+)", text):
        value = m.group(1).strip("`'\"")
        if len(value) >= 6:
            PASSWORDS.append(value)
    return ip, cred.group(1), cred.group(2)


def redact(text):
    for value in sorted(set(PASSWORDS), key=len, reverse=True):
        if value:
            text = text.replace(value, "<secret-redacted>")
    text = re.sub(r"LTAI[0-9A-Za-z]{8,}", "<secret-redacted>", text)
    text = re.sub(r"(?i)((?:password|passwd|pwd|secret|secret_key|access_key(?:_secret)?)\s*[=:]\s*)"
                  r"(['\"])(?:(?!\2).)*\2", r"\1\2<secret-redacted>\2", text)
    text = re.sub(r"(?i)(mysql(?:\+\w+)?://[^:\s]+:)[^@\s]+@", r"\1<secret-redacted>@", text)
    return text


REMOTE_CODE = r'''
import re

PATH = "/root/abr.py"
src = open(PATH, encoding="utf-8", errors="replace").read()
lines = src.splitlines()

def block_range(start):
    # start may be a decorator line; the function body starts at the next def
    i = start
    while i < len(lines) and lines[i].lstrip().startswith("@"):
        i += 1
    if i >= len(lines):
        return start, start + 1
    base_indent = len(lines[i]) - len(lines[i].lstrip())
    end = i + 1
    while end < len(lines):
        line = lines[end]
        if line.strip() and (len(line) - len(line.lstrip())) <= base_indent:
            break
        end += 1
    return start, end

# locate the /upload_data route
route_idx = None
for i, line in enumerate(lines):
    if "upload_data" in line and ("route" in line or "post" in line):
        route_idx = i
        break
if route_idx is None:
    for i, line in enumerate(lines):
        if "def upload_data" in line:
            route_idx = i
            break

chosen = []
if route_idx is not None:
    start = route_idx
    while start > 0 and lines[start - 1].lstrip().startswith("@"):
        start -= 1
    s, e = block_range(start)
    chosen.append((s, e))
    # helper functions called from the handler
    body = "\n".join(lines[s:e])
    called = set(re.findall(r"([A-Za-z_]\w*)\s*\(", body))
    defined = {}
    for i, line in enumerate(lines):
        m = re.match(r"def\s+([A-Za-z_]\w*)\s*\(", line)
        if m:
            defined[m.group(1)] = i
    for name in sorted(called):
        if name in defined and name not in ("jsonify", "request"):
            hs, he = block_range(defined[name])
            if (hs, he) not in chosen:
                chosen.append((hs, he))

# include auth/verify/token/setmac/login definitions for contract comparison
for i, line in enumerate(lines):
    m = re.match(r"def\s+([A-Za-z_]\w*)\s*\(", line)
    if not m:
        continue
    name = m.group(1).lower()
    if any(k in name for k in ("auth", "verify", "token", "setmac", "login", "register")):
        s, e = block_range(i)
        if (s, e) not in chosen:
            chosen.append((s, e))

print("total_lines=%d total_bytes=%d" % (len(lines), len(src)))
for s, e in chosen:
    print("--- block lines %d-%d" % (s + 1, e))
    for i in range(s, e):
        print("%4d| %s" % (i + 1, lines[i]))
print("--- end")
'''

TOKEN_META_CODE = r'''
import hashlib, re
src = open("/root/abr.py", encoding="utf-8", errors="replace").read()
m = re.search(r"^UPLOAD_TOKEN\s*=\s*(.+)$", src, re.M)
if not m:
    print("upload_token_defined=NO")
else:
    expr = m.group(1).strip()
    dynamic = ("os.environ" in expr) or ("getenv" in expr)
    print("upload_token_defined=YES dynamic=%s" % dynamic)
    lit = re.match(r"^[\"'](.*)[\"']\s*$", expr)
    if lit:
        value = lit.group(1)
        print("upload_token_literal=YES length=%d sha256_8=%s"
              % (len(value), hashlib.sha256(value.encode()).hexdigest()[:8]))
    else:
        print("upload_token_literal=NO shape=%s"
              % re.sub(r"[\"'][^\"']*[\"']", "<redacted>", expr)[:60])
print("x_upload_token_refs=%d" % len(re.findall(r"X-Upload-Token", src)))
for path in ("/root/abr.py", "/root/abr.py.bak_20260926"):
    try:
        text = open(path, encoding="utf-8", errors="replace").read()
    except OSError as exc:
        print("%s read_error=%s" % (path, exc))
        continue
    print("%s lines=%d has_x_upload_token=%s has_upload_token_const=%s"
          % (path, len(text.splitlines()), "X-Upload-Token" in text,
             bool(re.search(r"^UPLOAD_TOKEN\s*=", text, re.M))))
'''

COMMANDS = [
    ("identity", "date -u +%Y-%m-%dT%H:%M:%SZ; date +%Y-%m-%dT%H:%M:%S%z; hostname"),
    ("service_object",
     "systemctl show user_script -p ActiveState -p SubState -p NRestarts -p ExecMainPID "
     "-p ExecMainStartTimestamp 2>&1; stat -c 'path=%n size=%s mtime=%y inode=%i' /root/abr.py 2>&1"),
    ("upload_handler", "python3 - <<'PY'\n" + REMOTE_CODE + "\nPY"),
    ("token_metadata_no_value", "python3 - <<'PY'\n" + TOKEN_META_CODE + "\nPY"),
]


def main():
    host, user, password = read_connection()
    started = time.time()
    lines = [
        "# CERT8883-UPLOAD-04 triage: read-only inspection of the deployed upload handler",
        "started_utc=%s" % time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "scope=read_only: stat+read deployed handler; no sudo, no write, no restart, no DB, no write endpoint",
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

    try:
        for label, command in COMMANDS:
            lines.append("--- [%s]" % label)
            try:
                _in, out, err = client.exec_command(command, timeout=CMD_TIMEOUT)
                text = out.read().decode("utf-8", "replace")
                errtext = err.read().decode("utf-8", "replace")
                code = out.channel.recv_exit_status()
                lines.append("exit=%s" % code)
                if text.strip():
                    lines.append(text.rstrip())
                if errtext.strip():
                    lines.append("stderr: " + errtext.rstrip())
            except Exception as exc:  # noqa: BLE001
                lines.append("exec_error=%s: %s" % (type(exc).__name__, exc))
            lines.append("")
    finally:
        client.close()

    lines.append("finished_utc=%s total_s=%.1f" % (time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
                                                   time.time() - started))
    write(lines)
    print(redact("\n".join(lines)))
    return 0


def write(lines):
    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(redact("\n".join(lines)).rstrip() + "\n")


if __name__ == "__main__":
    raise SystemExit(main())
