#!/usr/bin/env python
"""Structural (read-only) map of /root/abr.py: routes, functions, header checks,
token usages and 401 branches, with literal values redacted.

Used only to design a precise, minimal, reversible patch. No writes on target.
"""
import os
import re

import paramiko

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
OUT = os.path.join(HERE, "02_handler_structure_map.txt")
SECRETS = []


def read_connection():
    path = None
    for name in os.listdir(ROOT):
        if name.startswith("服务器迁移记录"):
            path = os.path.join(ROOT, name)
            break
    text = open(path, "r", encoding="utf-8", errors="replace").read()
    ip = None
    for row in text.splitlines():
        if "服务器 IP" in row:
            found = re.findall(r"[0-9]{1,3}(?:\.[0-9]{1,3}){3}", row)
            if found:
                ip = found[-1]
                break
    cred = re.search(r"目标服务器：\s*([^\s/]+)\s*/\s*([^\s`]+)", text)
    SECRETS.append(cred.group(2))
    return ip, cred.group(1), cred.group(2)


def redact(text):
    for value in SECRETS:
        if value:
            text = text.replace(value, "<secret-redacted>")
    text = re.sub(r"LTAI[0-9A-Za-z]{12,}", "<secret-redacted>", text)
    # redact assignment RHS for credential-looking names, keep name
    text = re.sub(
        r"(?im)^(\s*[A-Z0-9_]*(?:PASS|PASSWORD|SECRET|TOKEN|KEY|ACCESS)[A-Z0-9_]*\s*=\s*).*$",
        r"\1<value-redacted>", text)
    return text


CMD = r"""
python3 - <<'PY'
import re
p='/root/abr.py'
lines=open(p,encoding='utf-8',errors='replace').read().splitlines()
print('total_lines=%d' % len(lines))
for i,l in enumerate(lines,1):
    s=l.strip()
    if (s.startswith('@app.route') or s.startswith('def ') or 'request.headers' in l
            or 'UPLOAD_TOKEN' in l or 'X-Upload-Token' in l or 'return jsonify' in l
            or s.startswith('#') and ('token' in s.lower() or 'upload' in s.lower())) and len(s)<200:
        print('%4d| %s' % (i, l[:160]))
print('--- count header-name literals ---')
print('lines_with_header_literal=%d' % sum(1 for l in lines if 'X-Upload-Token' in l))
print('lines_with_token_const=%d' % sum(1 for l in lines if 'UPLOAD_TOKEN' in l))
PY
"""


def main():
    ip, user, password = read_connection()
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(ip, username=user, password=password, timeout=15,
                   allow_agent=False, look_for_keys=False)
    try:
        _, stdout, stderr = client.exec_command(CMD, timeout=30)
        out = stdout.read().decode("utf-8", "replace")
        err = stderr.read().decode("utf-8", "replace")
    finally:
        client.close()
    body = redact(out + ("\n[stderr]\n" + err if err.strip() else ""))
    header = ("# read-only structural map of the deployed upload handler\n"
              "mode=READ_ONLY; forbidden=write,restart,deploy,secret-output\n\n")
    with open(OUT, "w", encoding="utf-8") as fh:
        fh.write(header + body + "\n")
    print("wrote", OUT)
    print(body[:4000])


if __name__ == "__main__":
    main()
