import sqlite3, os, sys

p = sys.argv[1] if len(sys.argv) > 1 else os.path.join('dbsnap_before', 'bwt.db')
print("db=", p)
con = sqlite3.connect(p)
cur = con.cursor()
cur.execute("SELECT name FROM sqlite_master WHERE type='table'")
tables = [r[0] for r in cur.fetchall()]
print("tables:", tables)
for t in ('temperature', 'pending_uploads', 'alarm_events'):
    if t in tables:
        cur.execute("SELECT COUNT(*) FROM %s" % t)
        print(t, "rows=", cur.fetchone()[0])
        cur.execute("PRAGMA table_info(%s)" % t)
        print("   cols=", [c[1] for c in cur.fetchall()])
    else:
        print(t, "MISSING")
if 'temperature' in tables:
    cur.execute("PRAGMA table_info(temperature)")
    cols = [c[1] for c in cur.fetchall()]
    tcol = 'time' if 'time' in cols else cols[1]
    print("--- last 6 temperature (all cols) ---")
    for r in cur.execute("SELECT * FROM temperature ORDER BY %s DESC LIMIT 6" % tcol):
        print(r)
if 'pending_uploads' in tables:
    print("--- pending_uploads rows ---")
    for r in cur.execute("SELECT * FROM pending_uploads"):
        print(r)
con.close()
