#!/usr/bin/env python3
"""Prepend the EV-026 sections to evidence/test.md and evidence/programmed_prototype.md.

Idempotent: if the EV-026 header is already at the top, nothing is written.
"""
import os

BASE = "D:/mypro/beiwo2/AIP/evidence"

JOBS = [
    ("ev026/EV-026_section.md", "test.md", "# 嵌入式验证证据（EV-026"),
    ("ev026/EV-026_prototype_section.md", "programmed_prototype.md",
     "# ProgrammedPrototypeArtifact（EV-026"),
]

for section_rel, target_rel, marker in JOBS:
    with open(os.path.join(BASE, section_rel), encoding="utf-8") as f:
        section = f.read().rstrip() + "\n"
    target = os.path.join(BASE, target_rel)
    with open(target, encoding="utf-8") as f:
        old = f.read()
    if old.lstrip().startswith(marker):
        print("skip (already at top):", target_rel)
        continue
    with open(target, "w", encoding="utf-8") as f:
        f.write(section + "\n---\n\n" + old)
    print("prepended EV-026 to", target_rel)
