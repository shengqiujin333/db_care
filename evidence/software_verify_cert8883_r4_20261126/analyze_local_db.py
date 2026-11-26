#!/usr/bin/env python3
"""Round-4 CERT8883: analyze the current local DB snapshot (read-only)."""
import os
import sqlite3

OUT = os.path.dirname(os.path.abspath(__file__))
DB = os.path.join(OUT, "12_local_db_after_relaunch.db")
con = sqlite3.connect(DB)
cur = con.cursor()
print("db=%s" % os.path.basename(DB))
for t in ("temperature", "pending_uploads", "alarm_events"):
    try:
        n = cur.execute("select count(*) from %s" % t).fetchone()[0]
        print("%s rows=%d" % (t, n))
    except Exception as exc:  # noqa: BLE001
        print("%s ERR %s" % (t, exc))
print("--- last 5 temperature rows (device_id, time, temp, humi)")
for r in cur.execute("select device_id,time,temperature,humidity from temperature order by time desc limit 5"):
    print(r)
print("--- pending_uploads rows (device_id, time, temp, humi, attempts)")
for r in cur.execute("select device_id,time,temperature,humidity,attempts from pending_uploads order by time"):
    print(r)
try:
    last = cur.execute("select device_id,time,temperature,humidity from temperature order by time desc limit 1").fetchone()
    print("--- newest_reading=%r" % (last,))
except Exception as exc:  # noqa: BLE001
    print("newest ERR %s" % exc)
con.close()
