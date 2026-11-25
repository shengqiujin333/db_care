#!/usr/bin/env python
"""Local-only check: ensure the saved redacted handler excerpt contains no credential
literals from the project migration record. Prints counts only, never values."""
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
HERE = os.path.dirname(os.path.abspath(__file__))
EVIDENCE = os.path.join(HERE, "01_abr_upload_handler_redacted.txt")

record = next(name for name in os.listdir(ROOT) if name.startswith("服务器"))
text = open(os.path.join(ROOT, record), encoding="utf-8", errors="replace").read()

candidates = set()
for m in re.finditer(r"[\x60\"']([A-Za-z0-9!@#$%^&*_.-]{6,})[\x60\"']", text):
    value = m.group(1)
    if (any(c.isdigit() for c in value) and any(c.isalpha() for c in value)
            and not re.match(r"^(?:\d{1,3}\.){3}\d{1,3}$", value)):
        candidates.add(value)
for m in re.finditer(r"LTAI[0-9A-Za-z]{8,}", text):
    candidates.add(m.group(0))

evidence = open(EVIDENCE, encoding="utf-8", errors="replace").read()
hits = [value for value in candidates if value in evidence]
print("candidate_secret_literals=%d leaked_in_evidence=%d" % (len(candidates), len(hits)))
lines = evidence.splitlines()
for value in hits:
    where = [str(i + 1) for i, line in enumerate(lines) if value in line]
    shape = "hex8" if re.fullmatch(r"[0-9A-Fa-f]{8}", value) else "other"
    print("match_masked=%s… len=%d shape=%s lines=%s"
          % (value[:2], len(value), shape, ",".join(where)))
print("redaction_marker_present=%s" % ("<secret-redacted>" in evidence or "redacted" in evidence))
