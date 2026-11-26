#!/usr/bin/env python3
"""Round-4 CERT8883: replay the freshly uploaded reading once (idempotency).

The real App uploaded (dev_id=6A002C00, time=1795660987, temperature=21.1,
humidity=36.3) at 2026-11-26 10:43:07 CST with phone=14000000001,
mac=0200000000A1. Replaying the same key must not insert a new row; the
handler returns inserted=0. No credentials are printed.
"""
import http.client
import json
import time

HOST = "8.140.23.253"
PORT = 5000
PAYLOAD = {
    "phone": "14000000001",
    "mac": "0200000000A1",
    "readings": [
        {"devId": "6A002C00", "time": 1795660987, "temperature": 21.1, "humidity": 36.3},
    ],
}

print("# replay of the 10:43:07 CST reading already uploaded by the real App")
print("# rows=%d phone=%s mac=%s" % (len(PAYLOAD["readings"]), PAYLOAD["phone"], PAYLOAD["mac"]))
body = json.dumps(PAYLOAD, ensure_ascii=False)
conn = http.client.HTTPConnection(HOST, PORT, timeout=15)
t0 = time.time()
conn.request("POST", "/upload_data", body=body, headers={"Content-Type": "application/json"})
resp = conn.getresponse()
data = resp.read().decode("utf-8", "replace")
elapsed = int((time.time() - t0) * 1000)
conn.close()
print("utc=%s status=%s elapsed_ms=%s body=%s"
      % (time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()), resp.status, elapsed, data))
