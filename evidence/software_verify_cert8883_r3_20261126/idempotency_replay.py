#!/usr/bin/env python3
"""CERT8883-UPLOAD-04 round-3: bounded idempotency replay.

POSTs the EXACT same 9 readings that the real App uploaded at 2026-11-26 05:46:54 CST
(phone 14000000001, mac 0200000000A1). The deployed handler uses INSERT IGNORE with a
unique (dev_id, time) key, so a replay must return HTTP 200 and inserted=0.
No new row is expected; no credentials are printed.
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
        {"devId": "6A002C00", "time": 1795594889, "temperature": 18.0, "humidity": 35.8},
        {"devId": "6A002C00", "time": 1795595126, "temperature": 18.0, "humidity": 35.8},
        {"devId": "6A002C00", "time": 1795595365, "temperature": 18.0, "humidity": 35.8},
        {"devId": "6A002C00", "time": 1795595603, "temperature": 18.0, "humidity": 35.8},
        {"devId": "6A002C00", "time": 1795595840, "temperature": 18.0, "humidity": 35.8},
        {"devId": "6A002C00", "time": 1795596294, "temperature": 18.0, "humidity": 35.8},
        {"devId": "6A002C00", "time": 1795596532, "temperature": 18.0, "humidity": 35.8},
        {"devId": "6A002C00", "time": 1795596773, "temperature": 18.0, "humidity": 35.8},
        {"devId": "6A002C00", "time": 1795615121, "temperature": 21.7, "humidity": 37.1},
    ],
}


def post_once(label):
    body = json.dumps(PAYLOAD, ensure_ascii=False)
    conn = http.client.HTTPConnection(HOST, PORT, timeout=15)
    t0 = time.time()
    conn.request("POST", "/upload_data", body=body,
                 headers={"Content-Type": "application/json"})
    resp = conn.getresponse()
    data = resp.read().decode("utf-8", "replace")
    elapsed = int((time.time() - t0) * 1000)
    conn.close()
    print("[%s] utc=%s status=%s elapsed_ms=%s body=%s"
          % (label, time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
             resp.status, elapsed, data))


if __name__ == "__main__":
    print("# idempotency replay of the 9 readings already uploaded by the real App")
    print("# payload rows=%d phone=%s mac=%s"
          % (len(PAYLOAD["readings"]), PAYLOAD["phone"], PAYLOAD["mac"]))
    post_once("replay-1")
    time.sleep(2)
    post_once("replay-2")
