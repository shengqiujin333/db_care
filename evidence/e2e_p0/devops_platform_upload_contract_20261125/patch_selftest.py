#!/usr/bin/env python
"""Offline self-test for deploy/platform_upload_contract/patch_upload_gate.py.

Builds a synthetic /root/abr.py-shaped file in a temp dir, points a copy of the
patcher at its digest, and exercises: dry-run, apply, idempotency, syntax,
removal completeness, drift guard and structure guard.

No platform access, no network, no credentials.
"""
import hashlib
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(HERE)))
SCRIPT_SRC = os.path.join(ROOT, "deploy", "platform_upload_contract", "patch_upload_gate.py")
BASELINE_RE = re.compile(r'BASELINE_SHA256 = "[0-9a-f]{64}"')

SAMPLE = '''#!/usr/bin/env python
import json
from flask import Flask, request, jsonify

app = Flask(__name__)

# test-fixture comment (placeholder for the deployment comment line)
UPLOAD_TOKEN = "TESTFIXTUREVALUE0000000000000000000000000000000"

@app.route('/login', methods=['POST'])
def login():
    return jsonify({"message": "Login successful"}), 200

@app.route('/upload_data', methods=['POST'])
def upload_data():
    data = request.get_json()
    if not data:
        return jsonify({"error": "No json data"}), 400

    if request.headers.get('X-Upload-Token') != UPLOAD_TOKEN:
        return jsonify({"error": "unauthorized"}), 401

    phone = data.get('phone')
    readings = data.get('readings')
    if not phone:
        return jsonify({"error": "phone is required"}), 400
    if not readings or not isinstance(readings, list) or len(readings) == 0:
        return jsonify({"error": "readings is required"}), 400
    return jsonify({"message": "ok", "inserted": 0}), 200

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
'''
# the fixture comment must carry the header literal so the patcher's comment rule matches
SAMPLE = SAMPLE.replace("# test-fixture comment (placeholder for the deployment comment line)",
                        "# sensor upload shared key (App must send X-Upload-Token)")

results = []


def record(name, ok, detail):
    results.append((name, ok, detail))
    print(("PASS " if ok else "FAIL ") + name + " :: " + str(detail)[:200])


def main():
    tmpdir = tempfile.mkdtemp(prefix="abr_fixture_")
    target = os.path.join(tmpdir, "abr_sample.py")
    patcher = os.path.join(tmpdir, "patch_upload_gate.py")
    with open(target, "w", encoding="utf-8", newline="") as fh:
        fh.write(SAMPLE)
    digest = hashlib.sha256(open(target, "rb").read()).hexdigest()

    src = open(SCRIPT_SRC, "r", encoding="utf-8").read()
    asserted = BASELINE_RE.sub('BASELINE_SHA256 = "%s"' % digest, src, count=1)
    with open(patcher, "w", encoding="utf-8") as fh:
        fh.write(asserted)

    def run(path, *extra):
        p = subprocess.run([sys.executable, patcher, "--file", path,
                            "--python", sys.executable] + list(extra),
                           capture_output=True, text=True)
        return p.returncode, (p.stdout + p.stderr).strip()

    rc, out = run(target, "--dry-run")
    record("dry_run_reports_4_removals", rc == 0 and "removed=4" in out, out)

    rc, out = run(target)
    record("apply_patched", rc == 0 and "RESULT=PATCHED" in out, out)

    body = open(target, encoding="utf-8").read()
    record("no_token_reference_left",
           "UPLOAD_TOKEN" not in body and "X-Upload-Token" not in body, "residual check")
    record("body_validation_kept", "phone is required" in body and "readings is required" in body,
           "gate removal did not disturb validation")
    record("line_delta_4", len(SAMPLE.splitlines()) - len(body.splitlines()) == 4,
           "%d -> %d" % (len(SAMPLE.splitlines()), len(body.splitlines())))
    p = subprocess.run([sys.executable, "-c",
                        "import ast,sys;ast.parse(open(sys.argv[1],encoding='utf-8').read())", target],
                       capture_output=True, text=True)
    record("syntax_ok", p.returncode == 0, p.stderr.strip())

    rc, out = run(target)
    record("idempotent_second_run", rc == 0 and "ALREADY_APPLIED" in out, out)

    drift = os.path.join(tmpdir, "drift.py")
    with open(drift, "w", encoding="utf-8") as fh:
        fh.write(SAMPLE.replace("Login successful", "Login OK"))
    rc, out = run(drift, "--dry-run")
    record("drift_guard", rc == 3 and "ABORT_SHA_MISMATCH" in out, out)

    struct_target = os.path.join(tmpdir, "struct.py")
    altered = SAMPLE.replace("request.headers.get('X-Upload-Token')",
                             "request.headers.get('X-Other-Token')")
    with open(struct_target, "w", encoding="utf-8") as fh:
        fh.write(altered)
    alt = hashlib.sha256(open(struct_target, "rb").read()).hexdigest()
    with open(patcher, "w", encoding="utf-8") as fh:
        fh.write(BASELINE_RE.sub('BASELINE_SHA256 = "%s"' % alt, asserted, count=1))
    rc, out = run(struct_target, "--dry-run")
    record("structure_guard", rc == 4 and "ABORT_STRUCTURE" in out, out)

    failed = [r for r in results if not r[1]]
    out_path = os.path.join(HERE, "05_patch_selftest.txt")
    with open(out_path, "w", encoding="utf-8") as fh:
        fh.write("# offline self-test of the upload-gate patch (synthetic fixture, no platform access)\n")
        fh.write("fixture=%s\n" % "<temp dir, synthetic abr.py shape>" )
        for name, ok, detail in results:
            fh.write("%s | %s | %s\n" % ("PASS" if ok else "FAIL", name, str(detail)[:300]))
        fh.write("OVERALL=%s\n" % ("PASS" if not failed else "FAIL"))
    print("OVERALL=%s wrote=%s" % ("PASS" if not failed else "FAIL", out_path))
    return 0 if not failed else 1


if __name__ == "__main__":
    sys.exit(main())
