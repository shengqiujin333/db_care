import json, time, urllib.request, urllib.error, os

out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "14_idempotency_replay.txt")
payload = {
    "phone": "14000000001",
    "mac": "0200000000A1",
    "readings": [
        {"devId": "6A002C00", "time": 1795666245, "temperature": 21.1, "humidity": 37.6}
    ],
}
body = json.dumps(payload).encode("utf-8")
req = urllib.request.Request(
    "http://8.140.23.253:5000/upload_data",
    data=body,
    headers={"Content-Type": "application/json"},
    method="POST",
)
lines = []
lines.append("# same-key idempotency replay (supplementary; NOT row-count readback)")
lines.append("# utc=%s" % time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()))
lines.append("# payload: phone=14000000001 mac=0200000000A1 readings=[{devId:6A002C00,time:1795666245,temperature:21.1,humidity:37.6}]")
lines.append("# this is the 12:10:45 real BLE reading already uploaded by the App")
t0 = time.time()
try:
    with urllib.request.urlopen(req, timeout=20) as resp:
        code = resp.getcode()
        rbody = resp.read().decode("utf-8", "replace")
except urllib.error.HTTPError as e:
    code = e.code
    rbody = e.read().decode("utf-8", "replace")
except Exception as e:  # noqa
    code = "ERROR:%s" % type(e).__name__
    rbody = str(e)
elapsed = int((time.time() - t0) * 1000)
lines.append("http_status=%s elapsed_ms=%s" % (code, elapsed))
lines.append("body=%s" % rbody)
with open(out, "w", encoding="utf-8") as f:
    f.write("\n".join(lines) + "\n")
print("\n".join(lines))
