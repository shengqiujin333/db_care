#!/usr/bin/env python
"""Read-only dump of the upload-gate apply log written at 21:59:02 (context only)."""
import os
import re

import paramiko

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
OUT = os.path.join(HERE, "17_resume3_upload_gate_apply_log.txt")


def main():
    rec = None
    for name in os.listdir(ROOT):
        if name.startswith("\u670d\u52a1\u5668"):
            rec = os.path.join(ROOT, name)
    with open(rec, "r", encoding="utf-8", errors="replace") as fh:
        text = fh.read()
    ip = [f[-1] for f in (re.findall(r"[0-9]{1,3}(?:\.[0-9]{1,3}){3}", r)
                          for r in text.splitlines() if "服务器 IP" in r) if f][0]
    m = re.search(r"目标服务器：\s*([^\s/]+)\s*/\s*([^\s`]+)", text)
    user, pwd = m.group(1), m.group(2)

    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(ip, port=22, username=user, password=pwd, timeout=20,
                   banner_timeout=20, auth_timeout=20, look_for_keys=False, allow_agent=False)
    _in, out, err = client.exec_command(
        "cat /root/rd_upload_contract_20261125T135855Z/apply.log; echo '---backup sums---'; "
        "cat /root/rd_upload_contract_20261125T135855Z/BACKUP_SHA256SUMS.txt", timeout=30)
    body = out.read().decode("utf-8", "replace") + err.read().decode("utf-8", "replace")
    client.close()
    body = body.replace(pwd, "<secret-redacted>")
    body = re.sub(r"(?i)(password|passwd|token)\s*[=:]\s*\S+", r"\1=<redacted>", body)
    body = re.sub(r"eyJ[A-Za-z0-9._-]{10,}", "<jwt-redacted>", body)
    header = ("# upload-gate apply log (written by the implementation role at 21:59:02 CST)\n"
              "# quoted for the reliability window only; no judgement of code here\n\n")
    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(header + body.rstrip() + "\n")
    print("\n".join(body.splitlines()[:40]))


if __name__ == "__main__":
    raise SystemExit(main())
