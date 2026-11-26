#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""CERT8883-REG-03 read-only probe.

Scope:
  * phone logcat/event scan for crash/ANR/TLS signatures and terminal states (offline files)
  * platform connect-only / GET-only probes: 1883, 8883(TLS), 8083/8084, 18083, 5000, 3306
  * bounded TCP reachability scan for unexpected public diagnostic/free-check ports

No POST, no login, no SSH/DB connection, no target write, no service restart.
Only TCP connect, TLS handshake (verify with project CA), plain GET, and MySQL greeting read.
"""
import datetime
import http.client
import json
import os
import re
import socket
import ssl
import sys
import threading
import time

BASE = os.path.dirname(os.path.abspath(__file__))
HOST = "8.140.23.253"
CA = os.path.normpath(os.path.join(BASE, "..", "..", "dengbei_care", "app", "src", "main", "assets", "jd-ca.crt"))


def utc_now():
    return datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def write(name, text):
    with open(os.path.join(BASE, name), "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    print("wrote", name)


def scan_phone():
    out = []
    out.append("# phone offline scan (input files captured by adb read-only)")
    out.append("scan_utc=" + utc_now())

    log = os.path.join(BASE, "13_logcat_since_1140.txt")
    txt = open(log, encoding="utf-8", errors="replace").read()
    patterns = {
        "AndroidRuntime": r"AndroidRuntime",
        "FATAL_EXCEPTION": r"FATAL EXCEPTION",
        "ANR": r"\bANR\b|am_anr",
        "tls_error_signature": (r"No subjectAltNames|Trust anchor for certification path not found|"
                                r"PKIX path building failed|CertPathValidatorException|"
                                r"SSLPeerUnverifiedException|SSLHandshakeException"),
        "link_state_DATA_OK": r"link_state=DATA_OK",
        "link_state_CONNECT_FAILED": r"link_state=CONNECT_FAILED",
        "link_state_CONNECTING": r"link_state=CONNECTING",
        "CloudUpload_uploaded": r"CloudUpload: uploaded",
        "CONNACK_not_authorized": r"无权连接 \(5\)",
    }
    for name, pat in patterns.items():
        out.append("SCAN %-28s %d" % (name, len(re.findall(pat, txt))))

    # terminal state lines with timestamps
    out.append("")
    out.append("## terminal-state / key lines in window")
    for line in txt.splitlines():
        if re.search(r"link_state=|CloudUpload: uploaded|无权连接 \(5\)|Successfully connected and read data", line):
            out.append(line)

    # event buffer: app process / ANR / crash
    ev = os.path.join(BASE, "15_events_recent.txt")
    ev_lines = []
    if os.path.exists(ev):
        with open(ev, encoding="utf-8", errors="replace") as f:
            for line in f:
                if "com.jinyuni.dengbei_care" in line and re.search(r"am_anr|am_crash|am_proc_died|am_proc_start", line):
                    ev_lines.append(line.rstrip())
    out.append("")
    out.append("## events (app proc/anr/crash)")
    out.append("EVENT_HITS=%d" % len(ev_lines))
    out.extend(ev_lines[-40:])

    # crash buffer summary
    cb = os.path.join(BASE, "14_crash_buffer.txt")
    if os.path.exists(cb):
        ct = open(cb, encoding="utf-8", errors="replace").read()
        out.append("")
        out.append("## crash buffer")
        out.append("CRASH_BUFFER_FATAL_LINES=%d" % len(re.findall(r"FATAL EXCEPTION", ct)))
        for line in ct.splitlines():
            if "FATAL EXCEPTION" in line or "Process: com.jinyuni.dengbei_care" in line:
                out.append(line)
    write("30_phone_scan.txt", "\n".join(out) + "\n")


def tcp_probe(port, timeout=4.0):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(timeout)
    t0 = time.time()
    try:
        rc = s.connect_ex((HOST, port))
        dt = time.time() - t0
        return ("OPEN" if rc == 0 else "ERR_%d" % rc), dt
    except Exception as e:
        return "EXC_%s" % type(e).__name__, time.time() - t0
    finally:
        try:
            s.close()
        except Exception:
            pass


def tls_probe():
    lines = ["# TLS handshake to %s:8883 with project trust anchor, verify_ip semantics" % HOST]
    try:
        ctx = ssl.create_default_context(cafile=CA)
        ctx.check_hostname = True
        ctx.verify_mode = ssl.CERT_REQUIRED
        with socket.create_connection((HOST, 8883), timeout=10) as raw:
            with ctx.wrap_socket(raw, server_hostname=HOST) as ss:
                der = ss.getpeercert(binary_form=True)
                lines.append("handshake=OK")
                lines.append("tls_version=%s" % ss.version())
                lines.append("cipher=%s" % (ss.cipher()[0] if ss.cipher() else "?"))
                lines.append("hostname_verification=OK (check_hostname=True, server_hostname=IP %s)" % HOST)
                lines.append("leaf_sha256=%s" % ssl.cert_fingerprint(der, "sha256"))
                info = ss.getpeercert()
                for k in ("subject", "issuer", "serialNumber", "notBefore", "notAfter", "subjectAltName"):
                    lines.append("%s=%s" % (k, info.get(k)))
    except Exception as e:
        lines.append("handshake=FAIL %s: %s" % (type(e).__name__, e))
    return "\n".join(lines) + "\n"


def http_get(port, path, timeout=8):
    lines = ["== GET http://%s:%d%s ==" % (HOST, port, path)]
    try:
        conn = http.client.HTTPConnection(HOST, port, timeout=timeout)
        conn.request("GET", path, headers={"User-Agent": "cert8883-reg03-readonly/1.0", "Accept": "*/*"})
        r = conn.getresponse()
        body = r.read(600)
        lines.append("status=%d %s" % (r.status, r.reason))
        for k, v in r.getheaders():
            if k.lower() in ("allow", "server", "content-type", "www-authenticate", "access-control-allow-origin", "content-length", "retry-after"):
                lines.append("header %s: %s" % (k, v))
        lines.append("body_first_600=%r" % body)
        conn.close()
    except Exception as e:
        lines.append("error=%s: %s" % (type(e).__name__, e))
    return "\n".join(lines) + "\n"


def mysql_greeting():
    lines = ["# MySQL port greeting read (no auth, no SQL)"]
    try:
        s = socket.create_connection((HOST, 3306), timeout=8)
        s.settimeout(8)
        data = s.recv(256)
        lines.append("bytes=%d" % len(data))
        lines.append("hex_prefix=%s" % data[:16].hex())
        # packet: 3-byte len, 1-byte seq, 1-byte protocol, server version NUL terminated
        if len(data) > 5:
            lines.append("protocol_version=%d" % data[4])
            rest = data[5:]
            end = rest.find(b"\x00")
            lines.append("server_version=%r" % rest[:end].decode("latin1", "replace"))
        s.close()
    except Exception as e:
        lines.append("error=%s: %s" % (type(e).__name__, e))
    return "\n".join(lines) + "\n"


def bounded_scan():
    documented = [1883, 8883, 8083, 8084, 18083, 5000, 3306, 22]
    extra = [21, 23, 25, 53, 80, 443, 2019, 2375, 2376, 3000, 4000, 4370, 5370, 5432,
             5601, 6379, 7474, 8000, 8001, 8080, 8081, 8086, 8088, 8089, 8090, 9000,
             9090, 9091, 9200, 9300, 10000, 11211, 15672, 27017, 50000, 50070, 61616]
    ports = sorted(set(documented + extra))
    results = {}
    lock = threading.Lock()

    def work(p):
        r, dt = tcp_probe(p, timeout=2.0)
        with lock:
            results[p] = (r, dt)

    threads = [threading.Thread(target=work, args=(p,)) for p in ports]
    for t in threads:
        t.start()
    for t in threads:
        t.join()

    lines = ["# bounded TCP reachability scan (connect-only, 2.0s timeout, %s)" % utc_now()]
    lines.append("documented_ports=%s" % documented)
    lines.append("scanned_ports=%s" % ports)
    open_list, closed_list = [], []
    for p in ports:
        r, dt = results.get(p, ("NO_RESULT", 0))
        tag = "DOC" if p in documented else "EX"
        lines.append("PORT %-6d %-5s %-8s %.2fs" % (p, tag, r, dt))
        (open_list if r == "OPEN" else closed_list).append(p)
    lines.append("")
    lines.append("RESULT documented_open=%s" % sorted([p for p in open_list if p in documented]))
    lines.append("RESULT documented_not_open=%s" % sorted([p for p in documented if p not in open_list]))
    lines.append("RESULT extra_open=%s" % sorted([p for p in open_list if p not in documented]))
    return "\n".join(lines) + "\n"


def main():
    started = utc_now()
    write("31_tls_probe.txt", tls_probe())

    http_out = ["# HTTP GET-only probes %s" % started]
    for port, path in [(18083, "/status"), (18083, "/api/v5/status"), (18083, "/api/v5/nodes"),
                       (18083, "/"), (5000, "/"), (5000, "/login"), (5000, "/upload_data")]:
        http_out.append(http_get(port, path))
    write("32_http_probes.txt", "\n".join(http_out) + "\n")

    tcp_out = ["# TCP connect-only probes %s" % started]
    for p in [1883, 8883, 8083, 8084, 5000, 18083, 3306]:
        r, dt = tcp_probe(p, timeout=6.0)
        tcp_out.append("TCP %-6d %-8s %.2fs" % (p, r, dt))
    write("33_tcp_connect.txt", "\n".join(tcp_out) + "\n")

    write("34_mysql_greeting.txt", mysql_greeting())
    write("35_bounded_port_scan.txt", bounded_scan())
    write("36_probe_timestamps.txt", "started_utc=%s\nfinished_utc=%s\n" % (started, utc_now()))
    scan_phone()


if __name__ == "__main__":
    main()
