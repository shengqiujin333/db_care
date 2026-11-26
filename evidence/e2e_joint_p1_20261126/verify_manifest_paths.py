#!/usr/bin/env python3
"""Verify manifest entries for this round point at real files (read-only)."""
import os
import sys

REPO = 'D:/mypro/beiwo2/AIP'
PREFIXES = ('evidence/e2e_joint_p1_20261126/', 'artifacts/software_test_report_e2e_joint_p1_r1.md')
missing = []
count = 0
for line in open(os.path.join(REPO, 'file_manifest.txt'), encoding='utf-8', errors='replace'):
    if line.startswith('#'):
        continue
    parts = [p.strip() for p in line.split('|')]
    if len(parts) < 2:
        continue
    path = parts[0]
    if not any(path.startswith(p) or p == path for p in PREFIXES):
        continue
    count += 1
    full = os.path.join(REPO, path)
    if not os.path.exists(full):
        missing.append(path)
print('checked_entries=%d' % count)
print('missing=%s' % (missing if missing else 'NONE'))

# reverse check: every actual file in the new evidence dir is listed
listed = set()
for line in open(os.path.join(REPO, 'file_manifest.txt'), encoding='utf-8', errors='replace'):
    if line.startswith('#'):
        continue
    parts = [p.strip() for p in line.split('|')]
    if parts and parts[0].startswith('evidence/e2e_joint_p1_20261126/') and not parts[0].endswith('/'):
        listed.add(parts[0])
actual = set()
base = os.path.join(REPO, 'evidence/e2e_joint_p1_20261126')
for name in sorted(os.listdir(base)):
    actual.add('evidence/e2e_joint_p1_20261126/' + name)
print('actual_files=%d listed_files=%d' % (len(actual), len(listed)))
print('unlisted=%s' % sorted(actual - listed))
print('listed_but_absent=%s' % sorted(listed - actual))
sys.exit(1 if missing or (actual - listed) or (listed - actual) else 0)
