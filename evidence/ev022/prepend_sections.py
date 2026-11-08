#!/usr/bin/env python3
"""Prepend the EV-022 sections to the shared evidence files (idempotent).

evidence/test.md                  <- evidence/ev022/EV-022_section.md
evidence/programmed_prototype.md  <- evidence/ev022/programmed_prototype_section.md

Historical content is preserved verbatim below the new section.
"""
import os

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
EV = os.path.join(ROOT, "evidence")

JOBS = [
    (os.path.join(EV, "ev022", "EV-022_section.md"),
     os.path.join(EV, "test.md"),
     "EV-022 / 修复轮复验"),
    (os.path.join(EV, "ev022", "programmed_prototype_section.md"),
     os.path.join(EV, "programmed_prototype.md"),
     "EV-022 / 修复轮复验"),
]


def main():
    for section, target, marker in JOBS:
        with open(section, "r", encoding="utf-8") as f:
            new = f.read()
        with open(target, "r", encoding="utf-8") as f:
            old = f.read()
        if marker in old[:400]:
            print("already prepended: %s" % target)
            continue
        with open(target, "w", encoding="utf-8", newline="") as f:
            f.write(new + old)
        print("prepended %s -> %s" % (os.path.basename(section), target))


if __name__ == "__main__":
    main()
