#!/usr/bin/env python
"""Bounded TCP reachability probe for the existing service host.

Reads the host address from project material (服务器迁移记录.md) instead of
hard-coding it, and only opens/closes TCP sockets (no application payload).
"""
import os
import re
import socket
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
MIGRATION = os.path.join(ROOT, "服务器迁移记录.md")

PORTS = [22, 1883, 3306, 5000, 8083, 8084, 8883, 18083]


def read_host():
    with open(MIGRATION, "r", encoding="utf-8") as fh:
        text = fh.read()
    m = re.search(r"服务器 IP.*?\|\s*([0-9]{1,3}(?:\.[0-9]{1,3}){3})\s*\|\s*\*{0,2}([0-9]{1,3}(?:\.[0-9]{1,3}){3})", text)
    if not m:
        raise SystemExit("host not found in project material")
    return m.group(2)


def main():
    host = read_host()
    print(f"probe_start_utc={time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())}")
    print(f"host={host}")
    for port in PORTS:
        t0 = time.time()
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5.0)
        try:
            sock.connect((host, port))
            print(f"port={port:<5} state=TCP_OPEN elapsed_ms={(time.time()-t0)*1000:.0f}")
        except Exception as exc:  # noqa: BLE001 - record everything
            print(f"port={port:<5} state=NO_CONNECT elapsed_ms={(time.time()-t0)*1000:.0f} error={type(exc).__name__}:{exc}")
        finally:
            sock.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
