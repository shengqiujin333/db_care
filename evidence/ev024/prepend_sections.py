#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Prepend the EV-024 section to evidence/test.md and the prototype record.

Re-runnable: if the marker "EV-024" already heads the target file, it does nothing.
"""
import os

BASE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def prepend(section_path, target_path):
    section = open(section_path, "r", encoding="utf-8").read().rstrip("\n")
    old = open(target_path, "r", encoding="utf-8").read()
    if old.lstrip().startswith("# 嵌入式验证证据（EV-024") or "EV-024" in old[:400]:
        print(f"{target_path}: already headed by EV-024, skipped")
        return
    new = section + "\n\n---\n\n" + old
    open(target_path, "w", encoding="utf-8", newline="\n").write(new)
    print(f"{target_path}: EV-024 section prepended ({len(section)} chars)")


def main():
    prepend(os.path.join(BASE, "evidence", "ev024", "EV-024_section.md"),
            os.path.join(BASE, "evidence", "test.md"))

    proto = os.path.join(BASE, "evidence", "ev024", "EV-024_prototype_section.md")
    if os.path.exists(proto):
        prepend(proto, os.path.join(BASE, "evidence", "programmed_prototype.md"))


if __name__ == "__main__":
    main()
