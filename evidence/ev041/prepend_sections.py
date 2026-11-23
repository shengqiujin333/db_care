#!/usr/bin/env python3
"""Prepend EV-041 sections into the cumulative evidence files (idempotent)."""
import os

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))


def prepend(target: str, section_file: str, marker: str) -> None:
    with open(os.path.join(HERE, section_file), encoding="utf-8") as f:
        section = f.read().rstrip() + "\n"
    path = os.path.join(ROOT, target)
    with open(path, encoding="utf-8") as f:
        old = f.read()
    if old.lstrip().startswith(marker) or ("EV-041" in old.split("\n", 1)[0]):
        print(f"skip (already at top): {target}")
        return
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(section + "\n---\n\n" + old)
    print(f"prepended: {target}")


prepend("evidence/test.md", "EV-041_section.md", "# 嵌入式实板验证证据（EV-041")
prepend("evidence/programmed_prototype.md", "EV-041_prototype_section.md", "# ProgrammedPrototypeArtifact（EV-041")
