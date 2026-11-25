#!/usr/bin/env python
"""Execute the reviewed /upload_data contract-alignment change on the platform host.

Authorization chain (recorded, not assumed):
  * project goal: fix and re-test until the Android->platform upload path works;
  * capability question 2026-11-25 asked for A/B/C; user answered "选择A"
    (A = authorize removal of the X-Upload-Token gate, keep body validation);
  * operator recovery instruction: retry the interrupted check inside the same task.

Modes:
  --stage dry-run|apply   (dry-run teleports the scripts and runs apply_remote.sh without --confirm)
  --stage post|state      (read-only post-fix verification / state re-read)

Safety: scripts run in /root/rd_upload_contract_<ts>/; the executor takes a 600 backup,
restarts only user_script.service, verifies, and auto-rolls back on post-fix failure.
No DB connection, no credential output, token value never printed.
"""
import argparse
import os
import posixpath
import re
import sys
import time

import paramiko

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
SRC = os.path.join(ROOT, "deploy", "platform_upload_contract")
SECRETS = []


def read_connection():
    path = None
    for name in os.listdir(ROOT):
        if name.endswith(".md") and ("服务器迁移" in name or "Ǩ��" in name):
            path = os.path.join(ROOT, name)
            break
    if not path:
        raise SystemExit("migration record not found")
    text = open(path, "r", encoding="utf-8", errors="replace").read()
    ip = None
    for row in text.splitlines():
        if "IP" in row:
            found = re.findall(r"[0-9]{1,3}(?:\.[0-9]{1,3}){3}", row)
            if found:
                ip = found[-1]
                break
    cred = re.search(r"(?:目标服务器|Ŀ���������)\s*[:：]?\s*root\s*/\s*([^\s`]+)", text)
    if not cred:
        cred = re.search(r"\broot\s*/\s*([^\s`]+)", text)
    if not cred:
        raise SystemExit("connection info not found")
    SECRETS.append(cred.group(1))
    return ip, "root", cred.group(1)


def redact(text):
    for value in SECRETS:
        if value:
            text = text.replace(value, "<secret-redacted>")
    text = re.sub(r"LTAI[0-9A-Za-z]{12,}", "<secret-redacted>", text)
    text = re.sub(r"(?<=UPLOAD_TOKEN = )'[^']*'", "'<token-value-redacted>'", text)
    text = re.sub(r"(?<=UPLOAD_TOKEN = )\"[^\"]*\"", "\"<token-value-redacted>\"", text)
    return text


def run(client, cmd, timeout=180):
    channel = client.get_transport().open_session()
    channel.settimeout(timeout)
    channel.exec_command(cmd)
    out, err = [], []
    deadline = time.time() + timeout
    while time.time() < deadline:
        if channel.recv_ready():
            out.append(channel.recv(65536))
        if channel.recv_stderr_ready():
            err.append(channel.recv_stderr(65536))
        if channel.exit_status_ready():
            while channel.recv_ready():
                out.append(channel.recv(65536))
            while channel.recv_stderr_ready():
                err.append(channel.recv_stderr(65536))
            break
        time.sleep(0.1)
    rc = channel.recv_exit_status()
    channel.close()
    return (rc,
            b"".join(out).decode("utf-8", "replace"),
            b"".join(err).decode("utf-8", "replace"))


SCRIPTS = ["patch_upload_gate.py", "verify_upload_contract.sh",
           "apply_remote.sh", "rollback_remote.sh"]

STATE_CMDS = [
    ("unit", "systemctl show user_script -p ActiveState -p SubState -p ExecMainPID "
             "-p ExecMainStartTimestamp -p NRestarts --value 2>&1; "
             "systemctl is-enabled user_script 2>&1"),
    ("deployed_object",
     "stat -c 'path=%n size=%s mode=%a uid=%u gid=%g inode=%i mtime=%y' /root/abr.py; "
     "sha256sum /root/abr.py; wc -l /root/abr.py; "
     "grep -c 'X-Upload-Token\\|UPLOAD_TOKEN' /root/abr.py || true; "
     "ls -la /root/ | grep -i 'abr'"),
    ("handler_shape",
     "python3 - <<'PY'\n"
     "import re\n"
     "src = open('/root/abr.py', encoding='utf-8', errors='replace').read().splitlines()\n"
     "for i, line in enumerate(src, 1):\n"
     "    if any(k in line for k in ('def upload_data', 'request.get_json', 'No json data',\n"
     "            \"data.get('phone')\", \"data.get('readings')\", 'mysql.connect',\n"
     "            'X-Upload-Token', 'UPLOAD_TOKEN')):\n"
     "        s = re.sub(r\"(UPLOAD_TOKEN\\s*=\\s*)(['\\\"]).*?\\2\", r\"\\1\\2<redacted>\\2\", line)\n"
     "        print('%d| %s' % (i, s.rstrip()))\n"
     "PY"),
    ("ports", "ss -ltn 2>/dev/null | awk 'NR==1 || /:(1883|8883|8083|8084|18083|5000|3306)\\b/'"),
    ("emqx", "systemctl is-active emqx 2>&1; emqx ctl listeners 2>&1 | grep -E 'ssl:default|listener|running' | head -12"),
    ("tls_leaf",
     "echo | openssl s_client -connect 127.0.0.1:8883 -servername 8.140.23.253 2>/dev/null "
     "| openssl x509 -noout -subject -issuer -serial -enddate -ext subjectAltName 2>/dev/null"),
    ("upload_log_window",
     "journalctl -u user_script --since '-6 hour' --no-pager 2>&1 "
     "| grep 'POST /upload_data' | tail -12 || true"),
    ("backups", "ls -la /root/ | grep -i 'abr.py.bak' || true"),
]


def stage_dry(client, host):
    ts = time.strftime("%Y%m%dT%H%M%SZ", time.gmtime())
    workdir = "/root/rd_upload_contract_%s" % ts
    rc, out, err = run(client, "mkdir -p %s && chmod 700 %s" % (workdir, workdir))
    if rc != 0:
        raise SystemExit("mkdir failed: %s" % err)
    sftp = client.open_sftp()
    try:
        for name in SCRIPTS:
            sftp.put(os.path.join(SRC, name), posixpath.join(workdir, name))
        sftp.chmod(posixpath.join(workdir, "patch_upload_gate.py"), 0o755)
        for name in SCRIPTS[1:]:
            sftp.chmod(posixpath.join(workdir, name), 0o755)
    finally:
        sftp.close()
    rc, out, err = run(client, "ls -l %s" % workdir)
    print("== uploaded ==")
    print(out)

    rc, out, err = run(client, "bash %s/apply_remote.sh --workdir %s" % (workdir, workdir),
                       timeout=240)
    print("== dry run rc=%s ==" % rc)
    print(out)
    if err.strip():
        print("== stderr ==")
        print(err)
    return workdir, rc, out, err


def stage_apply(client, workdir):
    rc, out, err = run(client, "bash %s/apply_remote.sh --workdir %s --confirm"
                       % (workdir, workdir), timeout=420)
    print("== apply rc=%s ==" % rc)
    print(out)
    if err.strip():
        print("== stderr ==")
        print(err)
    return rc, out, err


def stage_post(client, workdir):
    lines = []
    rc, out, err = run(client, "bash %s/verify_upload_contract.sh --phase post-fix"
                       % workdir, timeout=120)
    lines.append("## post-fix verify rc=%s" % rc)
    lines.append(out.rstrip())
    if err.strip():
        lines.append("## stderr\n" + err.rstrip())
    for name, cmd in STATE_CMDS:
        rc, out, err = run(client, cmd, timeout=60)
        lines.append("=" * 70)
        lines.append("## %s (rc=%s)" % (name, rc))
        lines.append(out.rstrip() or "<empty>")
    return "\n".join(lines)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--stage", required=True,
                    choices=["dry-run", "apply", "post", "diff"])
    ap.add_argument("--workdir", default="")
    ap.add_argument("--backup", default="")
    ap.add_argument("--out", default="")
    args = ap.parse_args()

    ip, user, password = read_connection()
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(ip, username=user, password=password, timeout=15,
                   allow_agent=False, look_for_keys=False)
    try:
        if args.stage == "dry-run":
            workdir, rc, out, err = stage_dry(client, ip)
            body = redact("# uploaded=%s\n# dry-run rc=%s\n%s" % (workdir, rc, out + err))
            target = args.out or os.path.join(HERE, "07_apply_dryrun.log")
        elif args.stage == "apply":
            if not args.workdir:
                raise SystemExit("--workdir required")
            rc, out, err = stage_apply(client, args.workdir)
            body = redact("# apply rc=%s workdir=%s\n%s" % (rc, args.workdir, out + err))
            target = args.out or os.path.join(HERE, "08_apply_execution.log")
        elif args.stage == "post":
            if not args.workdir:
                raise SystemExit("--workdir required")
            body = redact(stage_post(client, args.workdir))
            target = args.out or os.path.join(HERE, "09_postfix_state.txt")
        else:  # diff
            if not args.backup:
                raise SystemExit("--backup required for diff stage")
            rc, out, err = run(
                client,
                "python3 - <<'PY'\n"
                "import difflib\n"
                "b = open('__BACKUP__', encoding='utf-8', errors='replace').readlines()\n"
                "a = open('/root/abr.py', encoding='utf-8', errors='replace').readlines()\n"
                "d = list(difflib.unified_diff(b, a, 'abr.py.before', 'abr.py.after', n=2))\n"
                "print(''.join(d) if d else '<no textual difference>')\n"
                "print('lines_before={} lines_after={}'.format(len(b), len(a)))\n"
                "PY".replace("__BACKUP__", args.backup), timeout=60)
            body = redact("## structural diff before->after (rc=%s)\n%s%s"
                          % (rc, out, err))
            target = args.out or os.path.join(HERE, "10_apply_diff.txt")
        with open(target, "w", encoding="utf-8") as fh:
            fh.write(body.rstrip() + "\n")
        print("wrote", target)
    finally:
        client.close()


if __name__ == "__main__":
    main()
