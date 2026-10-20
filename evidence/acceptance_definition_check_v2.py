#!/usr/bin/env python3
"""Static alignment check for PACC-002 against its bound inputs (read-only).

Not a product test. Verifies:
- input files and digests;
- AD/PA/E2E/FB coverage and SA-X mapping;
- revised cross-cutting semantics (valid zero, BLE prefix, receive vs source,
  same-key idempotency, alarm coverage, dark separability, no cross-cycle queue);
- no Runtime IDs or absolute host paths.

Exit 0 = all checks pass; exit 1 = at least one failure.
"""
from __future__ import annotations

import hashlib
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
PACC = ROOT / "artifacts" / "product_acceptance_v2.md"

INPUTS = {
    "artifacts/product_requirements.md": "7e2da2e889f9342ebc81fc1c88badd2d3d4321b7f3b350eb5fdfafb3ec346d2c",
    "artifacts/product_specification.md": "9bd3119af50a6e45f2705e537ab6cf2dd69f97d4e0867497e9f7dd7f29f0bd68",
    "artifacts/product_scope.md": "26337f9333655290ea20275f498f736a268ad7475e5100af559fdd77544c5819",
    "artifacts/system_analysis.md": "4c9dd46bbc959d29953f418d76e1464a5c574d9b7d47c9c1a69ed94ef8a62327",
    "artifacts/system_architecture.md": "463ad472adf9fca288233af08dc09d9ab9b9172af5248552d580ce82f54c6659",
    "artifacts/system_decomposition.md": "33540eeede9fd3551f52718a657acb3a507a0d43f88437c0dbf911c332aa581a",
    "artifacts/firmware_exploration.md": "cb1c02d073ee339b7aa44a0ce20ff6f5fc138f2b068ae677dc50dd3c4eb10dc6",
    "artifacts/architecture_exploration.md": "848f7fbc240b8d31e939ed1353ca610d81acbeb9414200714da07e7d4f75e713",
}

checks: list[tuple[str, bool, str]] = []


def check(name: str, ok: bool, detail: str = "") -> None:
    checks.append((name, ok, detail))


def sha256(path: pathlib.Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    check("PACC-002 exists", PACC.is_file())
    if not PACC.is_file():
        for n, ok, d in checks:
            print(("PASS " if ok else "FAIL ") + n + (f" :: {d}" if d else ""))
        return 1

    text = PACC.read_text(encoding="utf-8")

    for rel, expected in INPUTS.items():
        p = ROOT / rel
        check(f"input exists:{rel}", p.is_file())
        if p.is_file():
            actual = sha256(p)
            print(f"{rel} sha256={actual}")
            check(f"digest matches:{rel}", actual == expected, f"{actual[:12]}...")

    for prefix, count in (("AD", 10), ("PA", 31), ("E2E", 9), ("FB", 20)):
        got = sorted(set(re.findall(rf"{prefix}-\d{{2,3}}", text)))
        expected = [f"{prefix}-{i:02d}" for i in range(1, count + 1)]
        if prefix == "PA":
            expected = [f"PA-{i:03d}" for i in range(1, 32)]
        missing = [x for x in expected if x not in got]
        check(f"{prefix} IDs 1..{count} present", not missing, ",".join(missing))

    for i in range(1, 13):
        check(f"SA-X{i:02d} mapped", f"SA-X{i:02d}" in text)
    for i in range(1, 15):
        check(f"PR-{i:03d} referenced", f"PR-{i:03d}" in text)
    for i in range(1, 9):
        check(f"readme point {i} traced", re.search(rf"\| {i} ", text) is not None)

    semantics = {
        "valid zero distinguished": "0.0℃/0.0%RH 合法" in text and "失败占位 0" in text,
        "BLE effective prefix": "16*(1+N)" in text and "有效前缀" in text,
        "receive observation vs source sample": "接收观察" in text and "源样本" in text,
        "same-key idempotency scope": "同 `(dev_id,time)`" in text and "同键" in text,
        "alarm coverage limitation": "不保证所有蹬被降温可见" in text,
        "dark separability": "C_dark > 3L" in text and "不可分离" in text,
        "no cross-cycle send queue": "不跨周期" in text,
        "MQTT publisher not guessed": "不猜发布端" in text,
        "no source exactly-once claim": "源样本恰一次" in text,
        "PACC-001 superseded": "替代 PACC-001" in text or "PACC-001 保留为历史" in text,
        "product not accepted": "NOT ACCEPTED" in text,
    }
    for name, ok in semantics.items():
        check(name, ok)

    check("no Runtime ids", re.search(r"CALL-|WF-|producer_node|content_digest", text) is None)
    check("no absolute windows paths", re.search(r"[A-Za-z]:\\", text) is None)

    failed = [c for c in checks if not c[1]]
    for name, ok, detail in checks:
        print(("PASS " if ok else "FAIL ") + name + (f" :: {detail}" if detail else ""))
    print(f"\n{len(checks) - len(failed)}/{len(checks)} checks passed")
    return 0 if not failed else 1


if __name__ == "__main__":
    sys.exit(main())
