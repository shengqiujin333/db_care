#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""可复跑：依次执行本能力既有独立门禁，记录退出码与结果末行（只读/不访问设备）。"""
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
PY = sys.executable
GATES = [
    "evidence/software_static_check_no_little_endian_parser.py",
    "evidence/software_static_check_server_migration.py",
    "evidence/software_verify_db_migration_item006.py",
    "evidence/software_verify_upload_payload_item008.py",
    "evidence/software_verify_cloud_upload_item009.py",
    "evidence/software_verify_timestamp_write_item007.py",
    "evidence/software_verify_orchestration_item010.py",
    "evidence/software_verify_card_status_item011.py",
    "evidence/software_verify_detail_presentation_item012.py",
    "evidence/software_verify_report_wording_item013.py",
    "evidence/software_verify_release_item014.py",
    "evidence/software_verify_aa005_android_check.py",
]
failed = 0
for gate in GATES:
    proc = subprocess.run([PY, gate], cwd=ROOT, capture_output=True, text=True,
                          timeout=120, encoding="utf-8", errors="replace")
    tail = [l for l in (proc.stdout + proc.stderr).splitlines() if l.strip()]
    print("EXIT=%d  %s" % (proc.returncode, gate))
    if tail:
        print("  " + tail[-1])
    print()
    if proc.returncode != 0:
        failed += 1
print("SUMMARY: %d/%d gates passed" % (len(GATES) - failed, len(GATES)))
sys.exit(0 if failed == 0 else 1)
