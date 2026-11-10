#!/usr/bin/env python3
"""Prepend the EV-032 sections to evidence/test.md and evidence/programmed_prototype.md.
Idempotent: refuses to prepend twice.
"""
import io

ROOT = "D:/mypro/beiwo2/AIP"
def prepend(target, section, marker):
    body = io.open(target, encoding="utf-8").read()
    if body.startswith(marker):
        print(f"skip (already present): {target}"); return
    sec = io.open(section, encoding="utf-8").read()
    io.open(target, "w", encoding="utf-8", newline="\n").write(sec + "\n" + body)
    print(f"prepended: {target}")

prepend(f"{ROOT}/evidence/test.md", f"{ROOT}/evidence/ev032/EV-032_section.md",
        "# 嵌入式验证证据（EV-032")
prepend(f"{ROOT}/evidence/programmed_prototype.md", f"{ROOT}/evidence/ev032/EV-032_prototype_section.md",
        "# ProgrammedPrototypeArtifact（EV-032")
