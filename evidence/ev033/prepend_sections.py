#!/usr/bin/env python3
"""Prepend the EV-033 sections to the shared evidence files (idempotent)."""
import io

PAIRS = (
    ("D:/mypro/beiwo2/AIP/evidence/test.md",
     "D:/mypro/beiwo2/AIP/evidence/ev033/EV-033_section.md"),
    ("D:/mypro/beiwo2/AIP/evidence/programmed_prototype.md",
     "D:/mypro/beiwo2/AIP/evidence/ev033/EV-033_prototype_section.md"),
)

for target, section in PAIRS:
    old = io.open(target, encoding="utf-8").read()
    if old.split("\n")[0].find("EV-033") >= 0:
        print("already prepended:", target)
        continue
    new = io.open(section, encoding="utf-8").read()
    io.open(target, "w", encoding="utf-8", newline="\n").write(new + "\n" + old)
    print("prepended:", target, "chars:", len(new) + len(old))
