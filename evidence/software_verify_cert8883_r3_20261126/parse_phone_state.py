#!/usr/bin/env python3
"""Parse bounded phone wifi dump + local snapshot for CERT8883 round-3 verification.
Read-only local analysis; no target mutation.
"""
import re
import sqlite3
import os

OUT = os.path.dirname(os.path.abspath(__file__))
txt = open(os.path.join(OUT, "03_wifi_dump.txt"), encoding="utf-8", errors="replace").read()
ssids = sorted(set(re.findall(r'SSID: "([^"]+)"', txt)))
cur = re.findall(r'SSID: ([^,\n]+)', txt)[:5]
print("SSID_candidates=", ssids[:10])
print("current_candidates=", cur)
for line in txt.splitlines():
    if "mNetworkInfo" in line and "WIFI" in line:
        print("NET:", line.strip())
        break

print("--- db")
con = sqlite3.connect(os.path.join(OUT, "05_local_db_before.db"))
cur = con.cursor()
for t in ("temperature", "pending_uploads", "alarm_events"):
    try:
        n = cur.execute("select count(*) from %s" % t).fetchone()[0]
        print(t, "rows=", n)
    except Exception as e:  # noqa: BLE001
        print(t, "ERR", e)
print("pending rows:")
for r in cur.execute(
        "select device_id,time,temperature,humidity,attempts,next_attempt_at "
        "from pending_uploads order by time"):
    print(r)
print("last temperature rows:")
for r in cur.execute(
        "select device_id,time,temperature,humidity from temperature order by time desc limit 3"):
    print(r)
con.close()
