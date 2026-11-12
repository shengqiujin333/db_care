#!/usr/bin/env python3
"""Check file_manifest.txt entries against the working tree and Git index.

Read-only. Reports:
  - manifest lines that do not follow `path | status | purpose`
  - non-directory paths that do not exist in the working tree
  - glob/wildcard entries (not verifiable as concrete files)
  - entries that exist but are not tracked by Git (informational)
  - tracked project files (excluding .git) that have no manifest entry (informational)
"""
import os
import subprocess
import sys

REPO = sys.argv[1] if len(sys.argv) > 1 else r"D:\mypro\beiwo2\AIP"
MANIFEST = os.path.join(REPO, "file_manifest.txt")

bad_format = []
missing = []
globs = []
untracked = []
entries = []

with open(MANIFEST, encoding="utf-8") as fh:
    for lineno, raw in enumerate(fh, 1):
        line = raw.rstrip("\n")
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        parts = [p.strip() for p in line.split("|")]
        if len(parts) < 3:
            bad_format.append((lineno, line))
            continue
        path, status = parts[0], parts[1]
        if "*" in path or "?" in path:
            globs.append((lineno, path))
            continue
        entries.append((lineno, path, status))
        full = os.path.join(REPO, path.replace("/", os.sep))
        if not os.path.exists(full):
            missing.append((lineno, path))

# Git tracked files
out = subprocess.run(
    ["git", "-C", REPO, "ls-files", "-z"],
    capture_output=True, check=True,
).stdout.decode("utf-8", "replace")
tracked = set(p for p in out.split("\0") if p)

entry_paths = set(p.rstrip("/") for _, p, _ in entries)

for lineno, path, status in entries:
    norm = path.rstrip("/")
    if norm in tracked:
        continue
    full = os.path.join(REPO, norm.replace("/", os.sep))
    if os.path.isdir(full):
        continue  # directory entries are allowed to be untracked as a directory
    untracked.append((lineno, path, status))

missing_tracked = sorted(p for p in tracked if p not in entry_paths and not p.startswith(".git/"))

print(f"manifest lines: {len(entries) + len(globs) + len(bad_format)}")
print(f"concrete entries: {len(entries)}")
print(f"glob entries: {len(globs)}")
print(f"bad format: {len(bad_format)}")
print(f"missing on disk: {len(missing)}")
print(f"exists but not tracked (informational): {len(untracked)}")
print(f"tracked but not listed (informational): {len(missing_tracked)}")

print("\n--- GLOB ENTRIES ---")
for lineno, p in globs:
    print(f"{lineno}: {p}")
print("\n--- BAD FORMAT ---")
for lineno, line in bad_format:
    print(f"{lineno}: {line}")
print("\n--- MISSING ON DISK ---")
for lineno, p in missing:
    print(f"{lineno}: {p}")
print("\n--- EXISTS BUT NOT TRACKED ---")
for lineno, p, s in untracked:
    print(f"{lineno}: {p} [{s}]")
print("\n--- TRACKED BUT NOT LISTED ---")
for p in missing_tracked:
    print(p)
