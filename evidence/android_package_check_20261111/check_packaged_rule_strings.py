#!/usr/bin/env python3
"""Read-only APK package check: confirm key user-facing rule strings are packaged.

Searches all .dex entries inside the built APK for UTF-8 substrings that the
approved Android requirement (rising >0.9C and no light, or >35.0C) must expose.
No device or network access.
"""
import os
import sys
import zipfile

BASE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(BASE, "..", ".."))
APK = os.path.join(REPO, "dengbei_care", "app", "build", "outputs", "apk",
                   "debug", "app-debug.apk")

NEEDLES = [
    "采样周期 3 分钟".encode("utf-8"),
    "升高超过 0.9".encode("utf-8"),
    "且无光".encode("utf-8"),
    "35.0℃".encode("utf-8"),
    "按条件上报".encode("utf-8"),
]

found = {n.decode("utf-8"): False for n in NEEDLES}
with zipfile.ZipFile(APK) as zf:
    dex_names = [n for n in zf.namelist() if n.endswith(".dex")]
    for name in dex_names:
        data = zf.read(name)
        for needle in NEEDLES:
            if needle in data:
                found[needle.decode("utf-8")] = True

print(f"apk={APK}")
print(f"dex_entries={dex_names}")
for text, ok in found.items():
    print(f"{'FOUND' if ok else 'MISSING'}  {text}")
missing = [t for t, ok in found.items() if not ok]
print("RESULT:", "OK" if not missing else f"MISSING {missing}")
sys.exit(0 if not missing else 1)
