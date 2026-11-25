#!/usr/bin/env python
"""External (from the developer host) post-fix probe for the platform upload contract.

Read-only in effect: the only POST bodies are `{}` and `{"probe":1}`, both rejected by
body validation before mysql.connect(), so no sensor_data row can be created.
Also re-checks the 8883 TLS identity against the App trust anchor (jd-ca.crt).

Run:  C:/Users/kason/anaconda3/python.exe .../external_postfix_probe.py
"""
import json
import os
import socket
import ssl
import sys
import time
import urllib.error
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
OUT = os.path.join(HERE, "11_external_postfix_probe.txt")
HOST = "8.140.23.253"
BASE = "http://%s:5000" % HOST
CA = os.path.join(ROOT, "dengbei_care", "app", "src", "main", "assets", "jd-ca.crt")

results = []


def say(line=""):
    results.append(line)
    print(line)


def http(method, path, body=None, headers=None):
    data = None
    hdrs = dict(headers or {})
    if body is not None:
        data = body.encode()
        hdrs.setdefault("Content-Type", "application/json")
    req = urllib.request.Request(BASE + path, data=data, headers=hdrs, method=method)
    try:
        with urllib.request.urlopen(req, timeout=12) as resp:
            return resp.status, resp.read(300).decode("utf-8", "replace")
    except urllib.error.HTTPError as exc:
        return exc.code, exc.read(300).decode("utf-8", "replace")
    except Exception as exc:  # noqa: BLE001
        return None, "%s: %s" % (type(exc).__name__, exc)


def tcp(port):
    s = socket.socket()
    s.settimeout(6)
    try:
        s.connect((HOST, port))
        return "OPEN"
    except Exception as exc:  # noqa: BLE001
        return "CLOSED(%s)" % type(exc).__name__
    finally:
        s.close()


def main():
    say("# external post-fix probe  %s" % time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()))
    say("target=%s" % HOST)
    say()
    say("== port reachability (TCP connect only) ==")
    for port in (1883, 8883, 8083, 8084, 18083, 5000, 3306):
        say("  %-5d %s" % (port, tcp(port)))
    say()
    say("== HTTP /upload_data over the public interface (no DB-reaching body) ==")
    code, body = http("GET", "/upload_data")
    say("  GET            -> %s %s" % (code, body.strip()[:120]))
    code, body = http("POST", "/upload_data", "{}")
    say("  POST {}        -> %s %s" % (code, body.strip()[:120]))
    code, body = http("POST", "/upload_data", '{"probe":1}')
    say("  POST probe     -> %s %s" % (code, body.strip()[:120]))
    code, body = http("POST", "/upload_data", '{"probe":1}',
                      {"X-Upload-Token": "invalid-probe"})
    say("  POST probe+hd  -> %s %s" % (code, body.strip()[:120]))
    code, body = http("GET", "/login")
    say("  GET /login     -> %s %s" % (code, body.strip()[:120]))
    say()
    say("== 8883 TLS identity against the App trust anchor ==")
    ctx = ssl.create_default_context(cafile=CA)
    ctx.check_hostname = True
    try:
        with socket.create_connection((HOST, 8883), timeout=10) as raw:
            with ctx.wrap_socket(raw, server_hostname=HOST) as tls:
                der = tls.getpeercert(binary_form=True)
                cert = tls.getpeercert()
                say("  handshake      : OK (%s)" % tls.version())
                say("  certificate OK : chain verified against %s" % os.path.basename(CA))
                say("  subject        : %s" % json.dumps(
                    dict(x[0] for x in cert["subject"]), ensure_ascii=False))
                say("  issuer         : %s" % json.dumps(
                    dict(x[0] for x in cert["issuer"]), ensure_ascii=False))
                say("  SAN            : %s" % [v for k, v in cert.get("subjectAltName", [])])
                say("  notAfter       : %s" % cert.get("notAfter"))
                say("  sha256         : %s" % __import__("hashlib").sha256(der).hexdigest())
    except Exception as exc:  # noqa: BLE001
        say("  handshake      : FAILED %s: %s" % (type(exc).__name__, exc))
    say()
    say("== negative control (wrong server name must fail) ==")
    try:
        with socket.create_connection((HOST, 8883), timeout=10) as raw:
            with ctx.wrap_socket(raw, server_hostname="wrong.example.invalid"):
                say("  UNEXPECTED: wrong hostname accepted")
    except ssl.SSLCertVerificationError as exc:
        say("  OK rejected: %s (verify_code=%s)" % (exc.reason, exc.verify_code))
    except Exception as exc:  # noqa: BLE001
        say("  rejected: %s: %s" % (type(exc).__name__, exc))
    say()
    say("== interpretation ==")
    say("  400 phone is required  => header gate removed, body validation kept, no DB write")
    say("  8883 SAN/chain         => certificate dimension unchanged by this repair")
    with open(OUT, "w", encoding="utf-8") as fh:
        fh.write("\n".join(results) + "\n")
    print("wrote", OUT)
    return 0


if __name__ == "__main__":
    sys.exit(main())
