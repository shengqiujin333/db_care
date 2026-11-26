#!/usr/bin/env python3
"""Verify the new automatic reading (05:49:05 CST) is already persisted.

The App enqueued and uploaded reading (6A002C00, 1795643345, 21.2, 35.5) at
2026-11-26 05:49:06 CST (CloudUpload: uploaded 1 reading(s)). Replaying the same
(dev_id, time) through the deployed INSERT IGNORE handler must return inserted=0
if the row is present. No new row is created; no credentials are printed.
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
        {"devId": "6A002C00", "time": 1795643345, "temperature": 21.2, "humidity": 35.5},
    ],
}

if __name__ == "__main__":
    body = json.dumps(PAYLOAD)
    conn = http.client.HTTPConnection(HOST, PORT, timeout=15)
    t0 = time.time()
    conn.request("POST", "/upload_data", body=body,
                 headers={"Content-Type": "application/json"})
    resp = conn.getresponse()
    data = resp.read().decode("utf-8", "replace")
    elapsed = int((time.time() - t0) * 1000)
    conn.close()
    print("# replay of the 05:49:06 CST automatic reading")
    print("[replay] utc=%s status=%s elapsed_ms=%s body=%s"
          % (time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
             resp.status, elapsed, data))
