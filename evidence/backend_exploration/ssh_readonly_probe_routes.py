#!/usr/bin/env python
"""Fourth bounded read-only probe: HTTP route existence for the documented
client endpoints, using GET only (never the write/POST methods).

Flask/Werkzeug answers 405 + `Allow:` for an existing POST-only route and 404
for an unknown path, so this distinguishes "route deployed" from "route absent"
without invoking the endpoint logic.
"""
import base64
import hashlib
import os
import sys
import time

import paramiko

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from ssh_readonly_probe import CMD_TIMEOUT, CONNECT_TIMEOUT, read_connection, redact  # noqa: E402

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "http_route_existence.txt")
PATHS = ["/login", "/upload_data", "/sendVerificationCode", "/verifyCode", "/register", "/setmac", "/doesNotExist"]


def build_command():
    parts = []
    for path in PATHS:
        parts.append(
            "printf '%s ' '" + path + "'; "
            "curl -s -o /dev/null -D - --max-time 5 -X GET http://127.0.0.1:5000" + path + " "
            "| tr -d '\\r' | awk 'NR==1{code=$2} /^Allow:/{allow=$0} END{printf \"status=%s %s\\n\", code, allow}'"
        )
    return "; ".join(parts)


def main():
    host, user, password = read_connection()
    started = time.time()
    lines = [
        "# HTTP route existence probe (GET only, read-only, loopback on the host)",
        f"started_utc={time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())}",
        f"host={host} port=22 transport=paramiko/{paramiko.__version__}",
        "method_note=no POST executed; write endpoint never invoked",
    ]
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    client.connect(host, 22, username=user, password=password, timeout=CONNECT_TIMEOUT,
                   banner_timeout=CONNECT_TIMEOUT, auth_timeout=CONNECT_TIMEOUT,
                   look_for_keys=False, allow_agent=False)
    key = client.get_transport().get_remote_server_key()
    lines.append("server_host_key=%s fingerprint_sha256=%s" % (
        key.get_name(), base64.b64encode(hashlib.sha256(key.asbytes()).digest()).decode().rstrip("=")))
    lines.append("connect_result=OK")
    lines.append("")
    command = build_command()
    lines.append(f"$ {command}")
    _in, stdout, stderr = client.exec_command(command, timeout=CMD_TIMEOUT)
    out = stdout.read().decode("utf-8", "replace")
    err = stderr.read().decode("utf-8", "replace")
    lines.append(f"exit={stdout.channel.recv_exit_status()} elapsed_ms={(time.time()-started)*1000:.0f}")
    lines.append("stdout:\n" + out.rstrip())
    if err.strip():
        lines.append("stderr:\n" + err.rstrip())
    client.close()
    lines.append(f"finished_utc={time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())} total_s={time.time()-started:.1f}")
    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(redact("\n".join(lines), [password, user]) + "\n")
    print("\n".join(lines))
    return 0


if __name__ == "__main__":
    sys.exit(main())
