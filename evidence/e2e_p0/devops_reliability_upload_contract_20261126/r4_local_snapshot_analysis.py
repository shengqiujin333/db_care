#!/usr/bin/env python
"""Read-only analysis of the App's local snapshot (copied via adb run-as cat).

Answers one reliability question: after the 22:00:24 crash/restart, does the App
still persist new measurements locally, or only raise threshold alarms? Uses the
snapshot copied from the device; nothing is written back to the phone.
"""
import datetime
import io
import os
import sqlite3

HERE = os.path.dirname(os.path.abspath(__file__))
DB = os.path.join(HERE, "24_r4_local_db_snapshot.db")
OUT = os.path.join(HERE, "26_r4_phone_collection_gap.txt")
CST = datetime.timezone(datetime.timedelta(hours=8))


def ts(value):
    try:
        return datetime.datetime.fromtimestamp(int(value), CST).strftime("%Y-%m-%d %H:%M:%S")
    except Exception:  # noqa: BLE001
        return str(value)


def main():
    con = sqlite3.connect(DB)
    lines = [
        "# App local snapshot analysis (read-only copy: databases/bwtemperature_database1)",
        "snapshot_taken_cst=2026-11-26 05:27:00",
        "source=adb exec-out run-as com.jinyuni.dengbei_care cat (read-only)",
        "",
    ]
    total, tmin, tmax = list(con.execute("select count(*), min(time), max(time) from temperature"))[0]
    lines.append("--- [temperature]")
    lines.append("rows=%d first_cst=%s last_cst=%s" % (total, ts(tmin), ts(tmax)))
    for row in con.execute(
            "select strftime('%Y-%m-%d %H', datetime(time,'unixepoch','+8 hours')) h, count(*) "
            "from temperature group by h order by h desc limit 8"):
        lines.append("per_hour %s rows=%d" % row)
    lines.append("")
    p_total, p_min, p_max = list(
        con.execute("select count(*), min(time), max(time) from pending_uploads"))[0]
    lines.append("--- [pending_uploads]")
    lines.append("rows=%d first_cst=%s last_cst=%s" % (p_total, ts(p_min), ts(p_max)))
    for row in con.execute("select device_id, time, temperature, humidity, attempts, next_attempt_at "
                           "from pending_uploads order by time"):
        lines.append("row device=%s time_cst=%s temp=%.1f hum=%.1f attempts=%d next_attempt=%s"
                     % (row[0], ts(row[1]), row[2], row[3], row[4], ts(row[5]) if row[5] else "-"))
    lines.append("")
    a_total, a_min, a_max = list(
        con.execute("select count(*), min(time), max(time) from alarm_events"))[0]
    lines.append("--- [alarm_events]")
    lines.append("rows=%d first_cst=%s last_cst=%s" % (a_total, ts(a_min), ts(a_max)))
    for row in con.execute("select time, device_id, type, message from alarm_events "
                           "order by time desc limit 6"):
        lines.append("row time_cst=%s device=%s kind=%s msg=%s" % (ts(row[0]), row[1], row[2], row[3]))
    lines.append("")
    lines.append("## machine checks")
    lines.append("RESULT temperature_rows=%d" % total)
    lines.append("RESULT last_temperature_row_cst=%s" % ts(tmax))
    lines.append("RESULT pending_rows=%d" % p_total)
    lines.append("RESULT pending_last_cst=%s" % ts(p_max))
    lines.append("RESULT pending_attempts_sum=%d"
                 % list(con.execute("select sum(attempts) from pending_uploads"))[0][0])
    lines.append("RESULT alarm_rows=%d last_alarm_cst=%s" % (a_total, ts(a_max)))
    lines.append("RESULT measurements_continue_after_crash=%s"
                 % ("YES" if tmax > 1795615000 + 600 else "NO"))
    lines.append("RESULT alarms_continue_after_crash=%s"
                 % ("YES" if a_max > 1795615000 + 600 else "NO"))

    with io.open(OUT, "w", encoding="utf-8", newline="\n") as fh:
        fh.write("\n".join(lines) + "\n")
    print("\n".join(lines[-8:]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
