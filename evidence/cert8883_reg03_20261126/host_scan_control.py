#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Control probe: demonstrates whether host TCP connect-only results are trustworthy.

If definitely-unused ports also report OPEN at ~0.00s, this Windows host sits behind a
transparent TUN/proxy that answers connects locally; the host connect-only scan must not
be used as reachability evidence.
"""
import socket
import time

HOST = "8.140.23.253"
for p in [1, 9, 9999, 65000, 8883, 18083]:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(3.0)
    t0 = time.time()
    try:
        rc = s.connect_ex((HOST, p))
        print("CONTROL port=%-6d rc=%-4d dt=%.3fs result=%s" % (p, rc, time.time() - t0, "OPEN" if rc == 0 else "CLOSED"))
    except Exception as e:
        print("CONTROL port=%-6d exc=%s" % (p, e))
    finally:
        s.close()
