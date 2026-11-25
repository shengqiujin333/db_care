#!/usr/bin/env python
"""Append/refresh the manifest entry of this maintenance helper, then verify the block."""
import io
import os

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
BASE = "evidence/e2e_p0/devops_reliability_8883_20261125/"
MANIFEST = os.path.join(ROOT, "file_manifest.txt")
SELF = "maintain_manifest_r3.py"

with io.open(MANIFEST, "r", encoding="utf-8") as fh:
    lines = [l for l in fh.read().splitlines()
             if not l.startswith(BASE + SELF)]
idx = next(i for i, l in enumerate(lines) if l.startswith(BASE + "resume3_upload_outcomes.py"))
lines.insert(idx + 1, "%s%s | active | 第三轮 file_manifest 维护脚本（按 path 分隔的三段式追加/更新本轮证据条目）" % (BASE, SELF))
with io.open(MANIFEST, "w", encoding="utf-8", newline="\n") as fh:
    fh.write("\n".join(lines) + "\n")

# read back: every entry under this evidence dir must exist in the workspace
entries = [l.split("|")[0].strip() for l in lines if l.startswith(BASE)]
missing = [p for p in entries if not os.path.exists(os.path.join(ROOT, p))]
print("entries=%d missing=%d" % (len(entries), len(missing)))
for path in missing:
    print("MISSING", path)
unlisted = sorted(f for f in os.listdir(os.path.join(ROOT, BASE))
                  if BASE + f not in entries)
print("unlisted_files=%s" % unlisted)
