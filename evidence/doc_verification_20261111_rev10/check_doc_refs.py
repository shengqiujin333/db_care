"""Check repository-relative path references in a Markdown document.

Read-only helper for DOC-VERIFY-010: extracts inline-code tokens that look like
repo-relative paths and reports whether each resolves under one of the project
source roots used by this documentation package. Skips URLs, placeholders and
wildcards. It does not expand file lists written inside Chinese parentheses;
those are checked separately by the report.
"""
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
BASES = [
    ROOT,
    os.path.join(ROOT, "CW32L010_StandardPeripheralLib_V1.0.5", "Examples", "sensor",
                 "gpio_input_output"),
    os.path.join(ROOT, "dengbei_care", "app", "src", "main", "java", "com", "jinyuni",
                 "dengbei_care"),
    os.path.join(ROOT, "dengbei_care", "app", "src", "main"),
    os.path.join(ROOT, "dengbei_care", "app", "src", "main", "res"),
    os.path.join(ROOT, "dengbei_care"),
]
TOKEN = re.compile(r"`([A-Za-z0-9_一-鿿][^`\n]*)`")


def looks_like_path(tok):
    if "://" in tok or "<" in tok or "*" in tok or " " in tok:
        return False
    if tok.startswith("..."):
        return False
    if "/" not in tok:
        return False
    if tok.endswith("/"):
        return True
    base = tok.rsplit("/", 1)[-1]
    return "." in base


def resolves(tok):
    rel = tok.rstrip("/").replace("/", os.sep)
    return any(os.path.exists(os.path.join(base, rel)) for base in BASES)


def main(paths):
    missing = []
    checked = 0
    for doc in paths:
        with open(doc, encoding="utf-8") as fh:
            text = fh.read()
        for tok in TOKEN.findall(text):
            if not looks_like_path(tok):
                continue
            tok = tok.split("（")[0].split("(")[0].strip()
            checked += 1
            if not resolves(tok):
                missing.append((doc, tok))
    print(f"bases={len(BASES)} checked={checked} missing={len(missing)}")
    for doc, tok in missing:
        print(f"MISSING {doc}: {tok}")
    return 0 if not missing else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
