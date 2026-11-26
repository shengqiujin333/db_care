#!/usr/bin/env python3
"""Round-4 CERT8883: bounded read-only regression probe of unaffected paths.

Checks listeners and HTTP semantics only; no login, no POST business payload,
no DB authentication, no file/config mutation.
"""
import http.client as httpclient
import socket
import ssl

HOST = "8.140.23.253"
CA = "dengbei_care/app/src/main/assets/jd-ca.crt"


def tcp(port, timeout=6):
    s = socket.socket()
    s.settimeout(timeout)
    try:
        s.connect((HOST, port))
        return "OPEN"
    except Exception as exc:  # noqa: BLE001
        return "CLOSED(%s)" % type(exc).__name__
    finally:
        s.close()


def http(path, port=5000, method="GET", timeout=8):
    conn = httpclient.HTTPConnection(HOST, port, timeout=timeout)
    try:
        conn.request(method, path)
        r = conn.getresponse()
        body = r.read(200).decode("utf-8", "replace").replace("\n", " ")[:160]
        allow = r.getheader("Allow") or ""
        return "status=%s allow=%r body=%r" % (r.status, allow, body)
    except Exception as exc:  # noqa: BLE001
        return "ERROR %s: %s" % (type(exc).__name__, exc)
    finally:
        conn.close()


def tls():
    ctx = ssl.create_default_context(cafile=CA)
    ctx.check_hostname = True
    try:
        with socket.create_connection((HOST, 8883), timeout=8) as raw:
            with ctx.wrap_socket(raw, server_hostname=HOST) as tls_sock:
                return "OK version=%s cipher=%s" % (
                    tls_sock.version(), (tls_sock.cipher() or ("?",))[0])
    except Exception as exc:  # noqa: BLE001
        return "FAIL %s: %s" % (type(exc).__name__, exc)


if __name__ == "__main__":
    for p in (1883, 8883, 8083, 8084, 18083, 5000, 3306):
        print("tcp port=%d %s" % (p, tcp(p)))
    print("tls_8883_with_jd_ca_and_hostname %s" % tls())
    print("http_5000_/ %s" % http("/"))
    print("http_5000_/login %s" % http("/login"))
    print("http_5000_/upload_data_GET %s" % http("/upload_data"))
    print("http_18083_/status %s" % http("/status", port=18083))
    try:
        s = socket.create_connection((HOST, 3306), timeout=6)
        banner = s.recv(16)
        s.close()
        printable = "".join(chr(b) if 32 <= b < 127 else "." for b in banner)
        print("mysql_3306_banner=%r" % printable)
    except Exception as exc:  # noqa: BLE001
        print("mysql_3306_banner_error=%s: %s" % (type(exc).__name__, exc))
