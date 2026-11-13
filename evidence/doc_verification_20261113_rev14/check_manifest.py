"""DOC-VERIFY-014: bounded file_manifest.txt vs workspace consistency check (read-only)."""
from __future__ import annotations

import os
import sys

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
MANIFEST = os.path.join(REPO, "file_manifest.txt")

total = 0
missing = []
bad_format = []
wildcards = []
status_counts = {}

with open(MANIFEST, encoding="utf-8") as fh:
    for lineno, line in enumerate(fh, 1):
        line = line.rstrip("\n")
        if not line.strip() or line.startswith("#"):
            continue
        # The purpose field may legally contain '|' (e.g. bit-field notation),
        # so only the first two separators delimit path and status.
        parts = [p.strip() for p in line.split("|", 2)]
        if len(parts) != 3:
            bad_format.append((lineno, line[:120]))
            continue
        path, status, _purpose = parts
        total += 1
        status_counts[status] = status_counts.get(status, 0) + 1
        if any(ch in path for ch in "*?<>{} "):
            wildcards.append((lineno, path))
            continue
        if not os.path.exists(os.path.join(REPO, path)):
            missing.append((lineno, path, status))

print("repo:", REPO)
print(f"entries={total} missing={len(missing)} bad_format={len(bad_format)} wildcards={len(wildcards)}")
print("status_counts:", ", ".join(f"{k}={v}" for k, v in sorted(status_counts.items())))
for lineno, path, status in missing:
    print(f"MISSING  line {lineno} [{status}] {path}")
for lineno, row in bad_format:
    print(f"BAD_FORMAT  line {lineno}: {row}")
for lineno, path in wildcards:
    print(f"WILDCARD  line {lineno}: {path}")

sys.exit(1 if missing or bad_format or wildcards else 0)
