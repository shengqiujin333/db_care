#!/usr/bin/env python
"""Read back this round's manifest entries against the real workspace."""
import io
import os

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
MANIFEST = os.path.join(ROOT, "file_manifest.txt")
BASE = "evidence/e2e_p0/devops_reliability_upload_contract_20261126/"
PREFIXES = (BASE, "artifacts/monitoring_reliability_upload_contract.yaml",
            "artifacts/health_evidence_reliability_upload_contract.yaml")


def main():
    with io.open(MANIFEST, encoding="utf-8") as fh:
        rows = [l for l in fh.read().splitlines() if l.startswith(PREFIXES)]
    missing = [r.split(" | ")[0] for r in rows
               if not os.path.exists(os.path.join(ROOT, r.split(" | ")[0]))]
    present = {r.split(" | ")[0] for r in rows}
    on_disk = []
    for name in sorted(os.listdir(HERE)):
        if name.endswith(".pyc") or name == "__pycache__":
            continue
        rel = BASE + name
        on_disk.append(rel)
    unregistered = [p for p in on_disk if p not in present]
    print("manifest_entries=%d" % len(rows))
    print("missing_on_disk=%s" % (missing or "none"))
    print("unregistered_files=%s" % (unregistered or "none"))


if __name__ == "__main__":
    raise SystemExit(main())
