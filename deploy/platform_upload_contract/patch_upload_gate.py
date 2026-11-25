#!/usr/bin/env python3
"""Remove the undocumented `X-Upload-Token` gate from the deployed /root/abr.py.

Runs ON the platform host. Idempotent, minimal, and reversible:
  * refuses to touch a file whose sha256 differs from the reviewed baseline;
  * removes exactly four lines (comment, constant, gate `if`, gate `return`);
  * never prints the token value or any other credential;
  * byte-compiles the result with the service interpreter before replacing.

Exit codes: 0 patched | 0 already applied | 3 baseline drift | 4 structure unexpected
            5 compile failed (original untouched)
"""
import argparse
import hashlib
import os
import re
import subprocess
import sys
import tempfile

BASELINE_SHA256 = "534dd40aac40c00597b06710069cbcc70533650e01318e0637cd924851740c63"
GATE_IF = "    if request.headers.get('X-Upload-Token') != UPLOAD_TOKEN:\n"
GATE_RETURN = '        return jsonify({"error": "unauthorized"}), 401\n'
CONST_RE = re.compile(r"^UPLOAD_TOKEN\s*=\s*(['\"])(?:(?!\1).)*\1\s*$")


def sha256_bytes(blob):
    return hashlib.sha256(blob).hexdigest()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--file", default="/root/abr.py")
    ap.add_argument("--python", default="/root/venv/bin/python")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    with open(args.file, "rb") as fh:
        raw = fh.read()
    before_sha = sha256_bytes(raw)
    text = raw.decode("utf-8")

    has_gate = "X-Upload-Token" in text or "UPLOAD_TOKEN" in text
    if not has_gate:
        print("RESULT=ALREADY_APPLIED sha256=%s" % before_sha)
        return 0

    if before_sha != BASELINE_SHA256:
        print("RESULT=ABORT_SHA_MISMATCH expected=%s actual=%s"
              % (BASELINE_SHA256, before_sha))
        return 3

    lines = text.splitlines(keepends=True)
    gate_idx = [i for i, l in enumerate(lines) if l in (GATE_IF, GATE_RETURN)]
    const_idx = [i for i, l in enumerate(lines) if CONST_RE.match(l.rstrip("\n"))]
    comment_idx = [i for i, l in enumerate(lines)
                   if l.lstrip().startswith("#") and "X-Upload-Token" in l]

    if len(gate_idx) != 2 or len(const_idx) != 1 or len(comment_idx) != 1:
        print("RESULT=ABORT_STRUCTURE gate=%d const=%d comment=%d"
              % (len(gate_idx), len(const_idx), len(comment_idx)))
        return 4
    if gate_idx != [gate_idx[0], gate_idx[0] + 1]:
        print("RESULT=ABORT_STRUCTURE gate_lines_not_adjacent %s" % gate_idx)
        return 4

    drop = set(gate_idx) | set(const_idx) | set(comment_idx)
    new_lines = [l for i, l in enumerate(lines) if i not in drop]
    new_text = "".join(new_lines)
    if "X-Upload-Token" in new_text or "UPLOAD_TOKEN" in new_text:
        print("RESULT=ABORT_RESIDUAL reference still present after removal")
        return 4
    removed = len(lines) - len(new_lines)
    if removed != 4:
        print("RESULT=ABORT_STRUCTURE removed=%d expected=4" % removed)
        return 4

    if args.dry_run:
        print("RESULT=DRY_RUN_READY removed=%d lines_after=%d sha256=%s"
              % (removed, len(new_lines), before_sha))
        return 0

    directory = os.path.dirname(os.path.abspath(args.file)) or "."
    fd, tmp = tempfile.mkstemp(prefix=".abr_patch_", dir=directory, text=True)
    try:
        with os.fdopen(fd, "w", encoding="utf-8", newline="") as out:
            out.write(new_text)
        st = os.stat(args.file)
        os.chmod(tmp, st.st_mode & 0o777)
        if hasattr(os, "chown"):
            try:
                os.chown(tmp, st.st_uid, st.st_gid)
            except PermissionError:
                pass
        check = subprocess.run([args.python, "-m", "py_compile", tmp],
                               capture_output=True, text=True)
        if check.returncode != 0:
            print("RESULT=ABORT_COMPILE %s" % check.stderr.strip()[:400])
            return 5
        os.replace(tmp, args.file)
        tmp = None
    finally:
        if tmp and os.path.exists(tmp):
            os.unlink(tmp)

    with open(args.file, "rb") as fh:
        after_sha = sha256_bytes(fh.read())
    print("RESULT=PATCHED removed=%d lines_after=%d sha256_before=%s sha256_after=%s"
          % (removed, len(new_lines), before_sha, after_sha))
    return 0


if __name__ == "__main__":
    sys.exit(main())
