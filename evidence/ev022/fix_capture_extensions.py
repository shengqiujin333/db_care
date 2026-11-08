#!/usr/bin/env python3
"""Rename EV-022 capture references from .bin to .cap.

The repository .gitignore ignores *.bin (build artifacts), so raw serial captures
must use the .cap extension used by earlier rounds. Only ev022 capture names are
rewritten; historical EV-012 .bin names are left untouched.
"""
import os

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
FILES = [
    os.path.join(ROOT, "evidence", "test.md"),
    os.path.join(ROOT, "evidence", "programmed_prototype.md"),
    os.path.join(ROOT, "evidence", "ev022", "EV-022_section.md"),
    os.path.join(ROOT, "evidence", "ev022", "programmed_prototype_section.md"),
    os.path.join(ROOT, "evidence", "ev022", "tool_results.txt"),
    os.path.join(ROOT, "evidence", "ev022", "analyze_ev022.py"),
    os.path.join(ROOT, "file_manifest.txt"),
]


def main():
    total = 0
    for path in FILES:
        with open(path, "r", encoding="utf-8", newline="") as f:
            text = f.read()
        changed = 0
        for i in range(1, 14):
            old = "cap%02d.bin" % i
            new = "cap%02d.cap" % i
            changed += text.count(old)
            text = text.replace(old, new)
        if changed:
            with open(path, "w", encoding="utf-8", newline="") as f:
                f.write(text)
        total += changed
        print("%-70s %d replacement(s)" % (os.path.relpath(path, ROOT), changed))
    print("total", total)


if __name__ == "__main__":
    main()
