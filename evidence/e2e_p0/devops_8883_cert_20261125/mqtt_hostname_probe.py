#!/usr/bin/env python
"""MQTT-layer probe against the platform 8883 listener with hostname verification ON.

Read-only in effect: opens TLS, sends one MQTT CONNECT with deliberately invalid
credentials (no publish, no subscribe, no state change), records CONNACK, closes.
Also runs a negative control with a wrong server name to prove hostname checking
is actually enabled in this probe (not bypassed with an insecure context).

No real device credential is used or printed.
"""
import os
import socket
import ssl
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
CA = os.path.join(ROOT, "dengbei_care", "app", "src", "main", "assets", "jd-ca.crt")
OUT = os.path.join(HERE, "10_mqtt_hostname_probe.txt")
HOST = "8.140.23.253"
PORT = 8883


def mqtt_connect_packet():
    client_id = b"rd-ops-probe"
    username = b"000000000000"
    password = b"invalid-probe"
    payload = len(client_id).to_bytes(2, "big") + client_id
    payload += len(username).to_bytes(2, "big") + username
    payload += len(password).to_bytes(2, "big") + password
    var = b"\x00\x04MQTT\x04\xc2\x00\x3c"
    body = var + payload
    return b"\x10" + bytes([len(body)]) + body


def recv_connack(sock):
    head = sock.recv(2)
    if len(head) < 2:
        return None, b""
    remaining = head[1]
    body = b""
    while len(body) < remaining:
        chunk = sock.recv(remaining - len(body))
        if not chunk:
            break
        body += chunk
    return head, body


def main():
    lines = ["# MQTT 8883 probe with TLS hostname verification enabled"]
    lines.append("host=%s port=%s ca=assets/jd-ca.crt" % (HOST, PORT))

    ctx = ssl.create_default_context(cafile=CA)
    ctx.check_hostname = True
    ctx.verify_mode = ssl.CERT_REQUIRED

    # positive: correct IP as server name
    try:
        with socket.create_connection((HOST, PORT), timeout=15) as raw:
            with ctx.wrap_socket(raw, server_hostname=HOST) as tls:
                cert = tls.getpeercert()
                lines.append("tls_handshake=OK hostname_verify=OK")
                lines.append("tls_version=%s cipher=%s" % (tls.version(), tls.cipher()[0]))
                subject = dict(x[0] for x in cert.get("subject", ()))
                lines.append("peer_cn=%s" % subject.get("commonName", "?"))
                lines.append("peer_san=%s" % ",".join(
                    v for k, v in cert.get("subjectAltName", ()) if k == "IP Address"))
                tls.sendall(mqtt_connect_packet())
                head, body = recv_connack(tls)
                if head and head[0] == 0x20 and len(body) >= 2:
                    lines.append("CONNACK_RC=%d" % body[1])
                    lines.append("RESULT mqtt_connect_reached=YES")
                else:
                    lines.append("CONNACK_RC=unexpected head=%r body=%r" % (head, body))
    except Exception as exc:  # noqa: BLE001
        lines.append("tls_handshake=FAIL %s: %s" % (type(exc).__name__, exc))
        lines.append("RESULT mqtt_connect_reached=NO")

    # negative control: wrong server name must fail
    try:
        with socket.create_connection((HOST, PORT), timeout=15) as raw:
            with ctx.wrap_socket(raw, server_hostname="8.140.23.254"):
                lines.append("RESULT negative_hostname_control=FAIL (wrong name accepted)")
    except ssl.SSLCertVerificationError as exc:
        lines.append("RESULT negative_hostname_control=OK (%s)" % exc.verify_message)
    except Exception as exc:  # noqa: BLE001
        lines.append("RESULT negative_hostname_control=ERROR %s: %s" % (type(exc).__name__, exc))

    ok = any("CONNACK_RC=5" in l for l in lines) and any("negative_hostname_control=OK" in l for l in lines)
    lines.append("RESULT overall=%s" % ("PASS" if ok else "FAIL"))
    with open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write("\n".join(lines) + "\n")
    print("\n".join(lines))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
