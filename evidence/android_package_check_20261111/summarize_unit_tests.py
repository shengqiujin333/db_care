#!/usr/bin/env python3
"""Summarize JUnit XML results for the current Android package check."""
import glob
import os
import xml.etree.ElementTree as ET

BASE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(BASE, "..", ".."))
results = os.path.join(REPO, "dengbei_care", "app", "build",
                       "test-results", "testDebugUnitTest", "TEST-*.xml")

files = sorted(glob.glob(results))
total = fail = err = skip = 0
bad = []
lines = []
for path in files:
    root = ET.parse(path).getroot()
    t = int(root.get("tests", 0))
    f = int(root.get("failures", 0))
    e = int(root.get("errors", 0))
    s = int(root.get("skipped", 0))
    total += t
    fail += f
    err += e
    skip += s
    lines.append(f"{os.path.basename(path)[5:-4]} tests={t} failures={f} errors={e} skipped={s}")
    if f or e:
        bad.append(os.path.basename(path))

summary = (f"suites={len(files)} tests={total} failures={fail} errors={err} skipped={skip}\n"
           f"failing_suites={bad}\n")
out = os.path.join(BASE, "unit_test_summary.txt")
with open(out, "w", encoding="utf-8") as fh:
    fh.write(summary)
    fh.write("\n".join(lines) + "\n")
print(summary)
print("\n".join(lines))
