#!/usr/bin/env python3
"""Read-only local App DB summary for E2E-JOINT-P1 evidence (no writes)."""
import sqlite3
import sys
import json

p = sys.argv[1]
c = sqlite3.connect('file:%s?mode=ro' % p, uri=True)
cur = c.cursor()
out = {}
out['tables'] = [r[0] for r in cur.execute("select name from sqlite_master where type='table'")]
for t in ('temperature', 'pending_uploads'):
    try:
        out[t + '_count'] = cur.execute("select count(*) from %s" % t).fetchone()[0]
    except Exception as e:
        out[t + '_count'] = 'ERR: %s' % e
try:
    cur.execute("select * from temperature order by time desc limit 10")
    out['latest10'] = [list(r) for r in cur.fetchall()]
    out['cols'] = [d[0] for d in cur.description]
except Exception as e:
    out['latest10'] = 'ERR: %s' % e
try:
    cur.execute("select * from temperature where temperature between 36.6 and 36.8")
    out['rows_36_7'] = [list(r) for r in cur.fetchall()]
except Exception as e:
    out['rows_36_7'] = 'ERR: %s' % e
try:
    cur.execute("select * from pending_uploads")
    out['pending'] = [list(r) for r in cur.fetchall()][:10]
except Exception as e:
    out['pending'] = 'ERR: %s' % e
print(json.dumps(out, ensure_ascii=False, indent=2))
