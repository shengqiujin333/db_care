#!/usr/bin/env python3
"""CERT8883-REG-03: read-only platform port/HTTP regression probe.

Only GET/TCP-connect; no POST, no login, no DB auth, no file writes on target.
"""
import socket
import ssl
import sys
import time
import http.client

HOST = "8.140.23.253"
results = []


def tcp(port, timeout=6.0):
    t0 = time.time()
    try:
        s = socket.create_connection((HOST, port), timeout=timeout)
        s.close()
        return True, f"OPEN ({int((time.time()-t0)*1000)} ms)"
    except Exception as e:
        return False, f"FAIL {type(e).__name__}: {e}"


def banner3306(timeout=6.0):
    try:
        s = socket.create_connection((HOST, 3306), timeout=timeout)
        s.settimeout(timeout)
        data = s.recv(256)
        s.close()
        if len(data) > 5:
            ver = data[5:].split(b"\x00", 1)[0].decode("latin-1", "replace")
            return True, f"handshake len={len(data)} version={ver}"
        return False, f"short banner len={len(data)}"
    except Exception as e:
        return False, f"FAIL {type(e).__name__}: {e}"


def http_get(path, port=5000, timeout=8.0):
    try:
        c = http.client.HTTPConnection(HOST, port, timeout=timeout)
        c.request("GET", path)
        r = c.getresponse()
        body = r.read(200).decode("latin-1", "replace")
        hdrs = {k.lower(): v for k, v in r.getheaders()}
        c.close()
        return r.status, hdrs.get("allow", ""), body.replace("\n", " ")[:160]
    except Exception as e:
        return None, "", f"FAIL {type(e).__name__}: {e}"


def tls8883():
    ctx = ssl.create_default_context(cafile="dengbei_care/app/src/main/assets/jd-ca.crt")
    try:
        with socket.create_connection((HOST, 8883), timeout=8) as raw:
            with ctx.wrap_socket(raw, server_hostname=HOST) as s:
                cert = s.getpeercert()
                sub = dict(x[0] for x in cert.get("subject", ()))
                return True, f"TLS {s.version()} peer_cn={sub.get('commonName')}"
    except Exception as e:
        return False, f"FAIL {type(e).__name__}: {e}"


print(f"# CERT8883-REG-03 read-only probe  time_utc={time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())} host={HOST}")

for p in (1883, 8883, 8083, 8084, 18083, 5000):
    ok, msg = tcp(p)
    print(f"PORT {p} tcp={'OPEN' if ok else 'FAIL'} detail={msg}")

ok, msg = tls8883()
print(f"TLS 8883 chain(trust_anchor)={'OK' if ok else 'FAIL'} detail={msg}")

for path, expect in [("/", 404), ("/login", 405), ("/upload_data", 405)]:
    st, allow, body = http_get(path)
    print(f"HTTP 5000 GET {path} status={st} allow={allow!r} body={body!r}")

try:
    c = http.client.HTTPConnection(HOST, 18083, timeout=8)
    c.request("GET", "/status")
    r = c.getresponse()
    b = r.read(100).decode("latin-1", "replace")
    c.close()
    print(f"HTTP 18083 GET /status status={r.status} body={b.strip()!r}")
except Exception as e:
    print(f"HTTP 18083 GET /status FAIL {type(e).__name__}: {e}")

ok, msg = banner3306()
print(f"PORT 3306 mysql_handshake={'OK' if ok else 'FAIL'} detail={msg}")
