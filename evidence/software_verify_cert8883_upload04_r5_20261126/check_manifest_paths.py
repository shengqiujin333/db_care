import os

repo = r"D:\mypro\beiwo2\AIP"
manifest = os.path.join(repo, "file_manifest.txt")
prefixes = (
    "evidence/software_verify_cert8883_upload04_r5_20261126/",
    "artifacts/software_test_report_cert8883_upload04_r5.md",
)
missing = []
found = 0
with open(manifest, "r", encoding="utf-8", errors="replace") as f:
    for line in f:
        line = line.rstrip("\n")
        if line.startswith("#") or not line.strip():
            continue
        parts = line.split("|")
        path = parts[0].strip()
        if any(path.startswith(p) for p in prefixes) or path in prefixes:
            full = os.path.join(repo, path.replace("/", os.sep))
            exists = os.path.exists(full)
            found += 1
            if not exists:
                missing.append(path)
            print(("OK  " if exists else "MISS"), path)
print("checked=%d missing=%d" % (found, len(missing)))
print("check_file_manifest.py exists:", os.path.exists(os.path.join(repo, "scripts", "check_file_manifest.py")))
