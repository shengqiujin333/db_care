#!/usr/bin/env python
"""Repeatable TLS reachability sample for the fixed 8883 listener (read-only).

20 consecutive TLS handshakes from the operator host to 8.140.23.253:8883 using
the App's trust anchor (assets/jd-ca.crt) with hostname/IP verification enabled.
No MQTT CONNECT is sent, so no authentication attempt and no business data.
One negative control with a wrong IP confirms verification is still enforced.
"""
import os
import socket
import ssl
import statistics
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
CA = os.path.join(ROOT, "dengbei_care", "app", "src", "main", "assets", "jd-ca.crt")
OUT = os.path.join(HERE, "04_connectivity_sample.txt")
HOST = "8.140.23.253"
PORT = 8883
ATTEMPTS = 20


def handshake(ctx, server_name, timeout=12):
    t0 = time.time()
    with socket.create_connection((HOST, PORT), timeout=timeout) as raw:
        with ctx.wrap_socket(raw, server_hostname=server_name) as tls:
            cert = tls.getpeercert()
            elapsed = (time.time() - t0) * 1000
            return elapsed, dict(x[0] for x in cert.get("subject", ())).get("commonName", "?"), tls.version(), tls.cipher()[0]


def main():
    lines = ["# 8883 TLS reachability sample after certificate replacement",
             "started_utc=%s" % time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
             "target=%s:%d ca=%s" % (HOST, PORT, os.path.relpath(CA, ROOT).replace("\\", "/")),
             "attempts=%d hostname_verification=on mqtt_connect=none" % ATTEMPTS, ""]
    ctx = ssl.create_default_context(cafile=CA)
    ctx.check_hostname = True
    ctx.verify_mode = ssl.CERT_REQUIRED

    ok, latencies, errors = 0, [], []
    for i in range(1, ATTEMPTS + 1):
        try:
            ms, cn, ver, cipher = handshake(ctx, HOST)
            ok += 1
            latencies.append(ms)
            lines.append("attempt=%02d result=OK elapsed_ms=%.1f peer_cn=%s tls=%s cipher=%s" % (i, ms, cn, ver, cipher))
        except Exception as exc:  # noqa: BLE001
            errors.append("%s: %s" % (type(exc).__name__, exc))
            lines.append("attempt=%02d result=FAIL %s: %s" % (i, type(exc).__name__, exc))
        time.sleep(1)

    lines.append("")
    lines.append("--- [negative_control_wrong_ip]")
    try:
        handshake(ctx, "8.140.23.254")
        lines.append("result=FAIL (wrong IP accepted)")
        negative_ok = False
    except ssl.SSLCertVerificationError as exc:
        lines.append("result=OK rejected verify_message=%s" % exc.verify_message)
        negative_ok = True
    except Exception as exc:  # noqa: BLE001
        lines.append("result=ERROR %s: %s" % (type(exc).__name__, exc))
        negative_ok = False

    lines.append("")
    lines.append("## machine checks")
    lines.append("RESULT tls_success=%d/%d" % (ok, ATTEMPTS))
    if latencies:
        lines.append("RESULT tls_latency_ms_min=%.1f median=%.1f max=%.1f"
                     % (min(latencies), statistics.median(latencies), max(latencies)))
    lines.append("RESULT negative_control=%s" % ("OK" if negative_ok else "FAIL"))
    lines.append("RESULT overall=%s" % ("PASS" if ok == ATTEMPTS and negative_ok else "FAIL"))
    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write("\n".join(lines) + "\n")
    print("\n".join(lines))
    return 0 if ok == ATTEMPTS and negative_ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
