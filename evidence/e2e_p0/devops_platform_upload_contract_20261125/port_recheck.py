#!/usr/bin/env python
"""Bounded read-only TCP reachability recheck of the platform ports (no payload, no auth)."""
import os
import socket
import time

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "06_service_port_recheck.txt")
HOST = "8.140.23.253"
PORTS = [(1883, "MQTT tcp"), (8883, "MQTT TLS"), (8083, "WS"), (8084, "WSS"),
         (18083, "dashboard"), (5000, "flask"), (3306, "mysql")]

lines = ["# platform TCP reachability recheck (connect only, no protocol payload)",
         "started_utc=%s host=%s" % (time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()), HOST)]
open_count = 0
for port, label in PORTS:
    started = time.time()
    sock = socket.socket()
    sock.settimeout(6)
    try:
        sock.connect((HOST, port))
        state = "OPEN"
        open_count += 1
    except Exception as exc:  # noqa: BLE001
        state = "CLOSED/ERROR %s" % type(exc).__name__
    finally:
        sock.close()
    lines.append("%-5d %-10s %-22s %.0f ms" % (port, label, state, (time.time() - started) * 1000))
lines.append("open=%d/%d" % (open_count, len(PORTS)))
with open(OUT, "w", encoding="utf-8") as fh:
    fh.write("\n".join(lines) + "\n")
print("\n".join(lines))
