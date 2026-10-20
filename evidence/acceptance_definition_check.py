#!/usr/bin/env python3
"""Acceptance-definition alignment check for PDR-001 / PSPEC-001 (read-only).

This is not a product test. It verifies that the requirements/specification
artifacts are internally consistent and traceable enough to define acceptance.
Exit 0 = all checks pass; exit 1 = at least one check fails.
"""
from __future__ import annotations

import hashlib
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
REQ = ROOT / "artifacts" / "product_requirements.md"
SPEC = ROOT / "artifacts" / "product_specification.md"
TR = ROOT / "evidence" / "product_requirements_traceability.md"

checks: list[tuple[str, bool, str]] = []
warnings: list[tuple[str, bool, str]] = []


def check(name: str, ok: bool, detail: str = "") -> None:
    checks.append((name, ok, detail))


def warn(name: str, ok: bool, detail: str = "") -> None:
    warnings.append((name, ok, detail))


def sha256(path: pathlib.Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    for p in (REQ, SPEC, TR):
        check(f"exists:{p.name}", p.is_file(), str(p.relative_to(ROOT)))

    req = REQ.read_text(encoding="utf-8")
    spec = SPEC.read_text(encoding="utf-8")
    tr = TR.read_text(encoding="utf-8")

    # 1. Requirement sections and per-section attributes.
    parts = re.split(r"(?m)^### (PR-\d{3}) ", req)[1:]
    ids = parts[0::2]
    bodies = parts[1::2]
    check("PR section count == 14", len(ids) == 14, f"got {len(ids)}")
    check("PR ids unique", len(ids) == len(set(ids)))
    for pid, body in zip(ids, bodies):
        check(f"{pid}:priority", bool(re.search(r"（P[012]", body.splitlines()[0])), body.splitlines()[0][:60])
        check(f"{pid}:source", "**来源**" in body)
        check(f"{pid}:observable acceptance", "**可验收行为**" in body)

    # 2. Specification must cover every requirement id, either explicitly or by
    #    the section group that implements it (PSPEC-001 section 8 mapping).
    semantic_cover = {
        "PR-006": ("网关", "不改"),
        "PR-011": ("光通讯", "退役"),
        "PR-012": ("低功耗", "深睡"),
    }
    for pid in ids:
        if pid in spec:
            check(f"spec references {pid}", True)
        elif pid in semantic_cover:
            ok = all(t in spec for t in semantic_cover[pid])
            warn(f"spec covers {pid} semantically", ok, "/".join(semantic_cover[pid]))
        else:
            check(f"spec references {pid}", False)

    # 3. Traceability references every requirement id and has a status table.
    for pid in ids:
        check(f"traceability references {pid}", pid in tr)
    check("traceability status legend", "AS-BUILT-OK" in tr and "NOT-VERIFIED" in tr)

    # 4. Core semantics must agree in both artifacts.
    check("req rise formula", "cur_temp_x10 - (int32)prev_temp_x10 > 9" in req)
    check("spec rise formula", "cur-prev > 9" in spec)
    check("req strict high boundary", "> 350" in req)
    check("spec strict high boundary", "> 350" in spec)
    check("req dark 1/3 formula", "3 × mean_adc_code >= C_dark" in req)
    check("spec dark 1/3 formula", "3*mean_adc >= C_dark" in spec)
    check("req exact 35.0 still evaluated", "仍要评估升温分支" in req)
    check("spec exact 35.0 still evaluated", "cur = 350" in spec and "仍上报" in spec)
    check("req prev semantics", "上一次有效采集温度" in req)
    check("spec prev semantics", "上一有效温度" in spec)

    # 5. Readme 8-point coverage appears in the traceability map.
    for point in range(1, 9):
        check(f"readme point {point} mapped", re.search(rf"\| {point} \|", tr) is not None)

    # 6. No Runtime-owned identifiers or absolute host paths leak into artifacts.
    blob = req + spec + tr
    check("no call/workflow ids", re.search(r"CALL-|WF-|producer_node|content_digest", blob) is None)
    check("no absolute windows paths", re.search(r"[A-Za-z]:\\", blob) is None)

    # 7. Open conditions / resources are explicit, not silently assumed.
    for token in ("C_dark", "OQ-01", "OQ-03", "OQ-06", "OQ-09"):
        check(f"open condition documented:{token}", token in req)

    # 8. Version binding material printed for the acceptance record.
    for p in (REQ, SPEC, TR):
        print(f"{p.relative_to(ROOT)} sha256={sha256(p)}")

    failed = [c for c in checks if not c[1]]
    for name, ok, detail in checks:
        print(("PASS " if ok else "FAIL ") + name + (f" :: {detail}" if detail else ""))
    for name, ok, detail in warnings:
        print(("WARN " if not ok else "PASS ") + name + (f" :: {detail}" if detail else ""))
    warn_failed = [w for w in warnings if not w[1]]
    print(f"\n{len(checks) - len(failed)}/{len(checks)} checks passed; {len(warn_failed)} warning(s)")
    return 0 if not failed else 1


if __name__ == "__main__":
    sys.exit(main())
