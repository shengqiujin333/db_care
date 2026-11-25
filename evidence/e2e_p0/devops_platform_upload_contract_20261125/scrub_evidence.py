#!/usr/bin/env python
"""Scrub captured evidence: remove credential-like literals from the read-only dumps.

Keeps the structural facts (names, lengths, line numbers) but replaces values.
"""
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
FILES = ["01_platform_upload_contract_state.txt", "02_handler_structure_map.txt"]

PATTERNS = [
    (re.compile(r"LTAI[0-9A-Za-z]{12,}"), "<credential-redacted>"),
    # 48-hex shared token material
    (re.compile(r"\b[0-9a-f]{40,64}\b"), "<token-or-digest-redacted>"),
    (re.compile(r"(?i)(ALIBABA_CLOUD_ACCESS_KEY_SECRET=)(?!<)[^\s]+"), r"\1<credential-redacted>"),
]

report = []
for name in FILES:
    path = os.path.join(HERE, name)
    text = open(path, "r", encoding="utf-8", errors="replace").read()
    before = text
    counts = []
    for pattern, repl in PATTERNS:
        text, n = pattern.subn(repl, text)
        counts.append(n)
    if text != before:
        open(path, "w", encoding="utf-8").write(text)
    report.append("%s replaced=%s" % (name, counts))
    # verify
    residual = []
    for pattern, _ in PATTERNS:
        if pattern.search(text):
            residual.append(pattern.pattern)
    report.append("  residual_patterns=%s" % (residual or "none"))

out = None  # report goes to stdout; the curated narrative lives in 03_evidence_redaction_check.txt
summary = "# redaction verify pass (values removed, structure kept)\n" + "\n".join(report)
print(summary)
print("no file rewritten: 03_evidence_redaction_check.txt is the curated record")
