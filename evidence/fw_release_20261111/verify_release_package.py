#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Verify the release zip against the expected release identity.

Checks: zip MD5, CRC of every member, member MD5 against the expected table, and
(if present) the Keil build output against the same identity.

Loose .axf/.hex copies are intentionally not kept in fw_release/ because the
repository .gitignore excludes those extensions; the zip carries the binaries.
The zip is packed with fixed member timestamps, so its MD5 is reproducible.
"""
import hashlib
import os
import zipfile

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
ZIP = os.path.join(ROOT, "fw_release", "fw_sensor_FD-002r8.zip")
KEIL = os.path.join(
    ROOT,
    "CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/output/exe",
)

EXPECTED = {
    "fw_sensor_FD-002r8_b6a15fed.axf": "b6a15fed2ed5e3d0c4adc51ac87a4830",
    "fw_sensor_FD-002r8_4d728dec.hex": "4d728dec9b0d28a352c46436b7586c72",
    "PACKAGE.txt": "a58f67fcf5b760767ff5b65888c73dfb",
}
ZIP_MD5 = "5df0c6d45ecddc8aae8bddbbcfc9cfdf"


def md5_bytes(data):
    return hashlib.md5(data).hexdigest()


def md5_file(path):
    with open(path, "rb") as fh:
        return md5_bytes(fh.read())


def main():
    ok = True
    print("zip        : %s (%d bytes)" % (os.path.relpath(ZIP, ROOT), os.path.getsize(ZIP)))
    actual_zip = md5_file(ZIP)
    print("zip md5    : %s (expected %s) %s"
          % (actual_zip, ZIP_MD5, "OK" if actual_zip == ZIP_MD5 else "MISMATCH"))
    ok = ok and actual_zip == ZIP_MD5

    with zipfile.ZipFile(ZIP) as zf:
        bad = zf.testzip()
        print("crc test   : %s" % ("OK" if bad is None else "BAD " + str(bad)))
        ok = ok and bad is None
        names = [i.filename for i in zf.infolist()]
        print("members    : %s" % names)
        if set(names) != set(EXPECTED):
            print("member set differs from expected")
            ok = False
        for name in sorted(names):
            data = zf.read(name)
            got = md5_bytes(data)
            exp = EXPECTED.get(name)
            print("  %-46s %8d  %s  %s" % (name, len(data), got, "OK" if got == exp else "MISMATCH"))
            ok = ok and got == exp

    for name, exp in (("Project.axf", EXPECTED["fw_sensor_FD-002r8_b6a15fed.axf"]),
                      ("Project.hex", EXPECTED["fw_sensor_FD-002r8_4d728dec.hex"])):
        path = os.path.join(KEIL, name)
        if os.path.exists(path):
            got = md5_file(path)
            print("keil %-9s : %s  %s" % (name, got, "OK" if got == exp else "MISMATCH"))
            ok = ok and got == exp
        else:
            print("keil %-9s : not present (run mdk_build rebuild to reproduce)" % name)

    print("RESULT: %s" % ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
