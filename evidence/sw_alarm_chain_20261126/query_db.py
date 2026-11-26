#!/usr/bin/env python3
"""Read-only query of the app local DB copy (test evidence helper).

Usage: python query_db.py <sqlite_copy> [--schema] [--temperature N] [--alarms N]
"""
import sqlite3
import sys


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: query_db.py <sqlite_copy> [--schema] [--temperature N] [--alarms N]")
        return 2
    path = sys.argv[1]
    n_temp = 20
    n_alarm = 50
    if "--temperature" in sys.argv:
        n_temp = int(sys.argv[sys.argv.index("--temperature") + 1])
    if "--alarms" in sys.argv:
        n_alarm = int(sys.argv[sys.argv.index("--alarms") + 1])
    con = sqlite3.connect(f"file:{path}?mode=ro", uri=True)
    cur = con.cursor()
    tables = [r[0] for r in cur.execute(
        "select name from sqlite_master where type='table' order by name").fetchall()]
    print("tables:", tables)
    if "--schema" in sys.argv:
        for t in tables:
            print(f"-- schema {t}:",
                  cur.execute("select sql from sqlite_master where name=?", (t,)).fetchone())
    for t in ("temperature", "alarm_events"):
        if t not in tables:
            print(f"-- {t}: MISSING")
            continue
        cnt = cur.execute(f"select count(*) from {t}").fetchone()[0]
        print(f"-- {t} count: {cnt}")
        lim = n_temp if t == "temperature" else n_alarm
        cols = [d[1] for d in cur.execute(f"pragma table_info({t})").fetchall()]
        print(f"-- {t} columns: {cols}")
        for row in cur.execute(f"select * from {t} order by rowid desc limit {lim}").fetchall():
            print("  ", row)
    con.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
