"""DOC-VERIFY-014: bounded inline path-reference check for the current doc package.

Scope: docs/*.md plus dengbei_care/用户使用说明书.md.
Only tokens that start with a known repository top-level entry are resolved; other
backticked identifiers (field names, constants, relative firmware names) are counted
separately and are not reported as missing.
"""
from __future__ import annotations

import os
import re
import sys

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
DOC_FILES = [
    "docs/README.md",
    "docs/01_产品说明与版本变更.md",
    "docs/02_传感器固件安装与烧录.md",
    "docs/03_传感器调试串口手册.md",
    "docs/04_安卓App安装与使用.md",
    "docs/05_平台服务器配置与联调前提.md",
    "docs/06_维护排障与已知限制.md",
    "dengbei_care/用户使用说明书.md",
]
TOP = (
    "docs/", "evidence/", "artifacts/", "fw_release/", "dengbei_care/",
    "CW32L010_StandardPeripheralLib_V1.0.5/", "sensor_hardware/", "CH592EVT/",
    "scripts/", "readme.txt", "file_manifest.txt", "gateway_board.xml",
    "服务器迁移记录.md", "gxht40.pdf",
)

backtick = re.compile(r"`([^`\n]+)`")
checked = 0
missing = []
skipped = 0
per_file = {}

for rel in DOC_FILES:
    path = os.path.join(REPO, rel)
    with open(path, encoding="utf-8") as fh:
        text = fh.read()
    n = 0
    for raw in backtick.findall(text):
        tok = raw.strip().strip("'\"")
        tok = tok.split("#", 1)[0].rstrip(".,;:）)")
        if not tok or any(ch in tok for ch in "<>{}*?| \\"):
            skipped += 1
            continue
        # Shorthand used by the user manual: `docs/01` / `docs/06` mean the
        # versioned manuals, not literal files without extension.
        if re.fullmatch(r"docs/\d{2}", tok):
            skipped += 1
            continue
        if not tok.startswith(TOP):
            skipped += 1
            continue
        n += 1
        checked += 1
        if not os.path.exists(os.path.join(REPO, tok)):
            missing.append((rel, tok))
    per_file[rel] = n

print("repo:", REPO)
for rel, n in per_file.items():
    print(f"  {rel}: {n} resolved references")
print(f"checked={checked} missing={len(missing)} skipped_non_path_tokens={skipped}")
for rel, tok in missing:
    print(f"MISSING  {rel}  ->  {tok}")

sys.exit(1 if missing else 0)
