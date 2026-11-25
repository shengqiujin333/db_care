#!/usr/bin/env python
"""Externally observed 8883 TLS sample for the fourth reliability window.

Connects from the test host to the public endpoint using the App's own trust anchor,
so the result reflects a real client view rather than a loopback listener check.
Read-only: one TLS handshake per sample, no MQTT CONNECT, no credentials, no data.
"""
import hashlib
import os
import socket
import ssl
import statistics
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
OUT = os.path.join(HERE, "21_r4_external_tls.txt")
CA = os.path.join(ROOT, "dengbei_care", "app", "src", "main", "assets", "jd-ca.crt")
HOST = "8.140.23.253"
PORT = 8883
SAMPLES = 12


def fp_of(der):
    return hashlib.sha256(der).hexdigest().upper()


def main():
    lines = ["# external 8883 TLS sample (App trust anchor, host name check by IP SAN)",
             "started_utc=%s" % time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
             "ca=%s" % os.path.relpath(CA, ROOT).replace("\\", "/"),
             "target=%s:%d samples=%d" % (HOST, PORT, SAMPLES),
             "forbidden=mqtt-connect,credentials,write", ""]
    ctx = ssl.create_default_context(cafile=CA)
    ok, fail, lat = 0, [], []
    for i in range(SAMPLES):
        t0 = time.time()
        try:
            with socket.create_connection((HOST, PORT), timeout=10) as raw:
                with ctx.wrap_socket(raw, server_hostname=HOST) as tls:
                    lat.append((time.time() - t0) * 1000)
                    ok += 1
                    if i == 0:
                        der = tls.getpeercert(binary_form=True)
                        lines.append("peer_cert_sha256=%s" % fp_of(der))
                        lines.append("tls_version=%s cipher=%s" % (tls.version(), tls.cipher()[0]))
                        cert = tls.getpeercert()
                        lines.append("subject=%s" % (cert.get("subject"),))
                        lines.append("issuer=%s" % (cert.get("issuer"),))
                        lines.append("serial=%s" % cert.get("serialNumber"))
                        lines.append("san_ip=%s"
                                     % [v for k, v in cert.get("subjectAltName", ()) if k == "IP Address"])
                        lines.append("not_after=%s" % (cert.get("notAfter"),))
        except Exception as exc:  # noqa: BLE001
            fail.append("%02d %s: %s" % (i + 1, type(exc).__name__, exc))
        time.sleep(0.4)

    # negative control: same trust anchor, wrong host name must be rejected
    neg = "unexpectedly accepted"
    try:
        with socket.create_connection((HOST, PORT), timeout=10) as raw:
            with ctx.wrap_socket(raw, server_hostname="8.140.23.254"):
                neg = "unexpectedly accepted"
    except ssl.SSLError as exc:
        neg = "rejected (%s)" % exc.reason
    except Exception as exc:  # noqa: BLE001
        neg = "rejected (%s)" % type(exc).__name__

    lines.append("")
    lines.append("## machine checks")
    lines.append("RESULT handshake_ok=%d/%d" % (ok, SAMPLES))
    lines.append("RESULT failures=%d" % len(fail))
    for row in fail:
        lines.append("FAIL %s" % row)
    if lat:
        lines.append("RESULT latency_ms_median=%.1f min=%.1f max=%.1f"
                     % (statistics.median(lat), min(lat), max(lat)))
    lines.append("RESULT negative_control=%s" % neg)
    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write("\n".join(lines) + "\n")
    print("\n".join(lines[-8:]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
