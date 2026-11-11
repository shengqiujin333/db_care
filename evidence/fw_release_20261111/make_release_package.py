#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Build the sensor firmware release package (deterministic zip, no device access).

Sources the strictly reproduced Keil build output, verifies its MD5 against the
released identity, writes PACKAGE.txt and packs a byte-stable zip (fixed member
timestamps) so the package MD5 is reproducible. Loose .axf/.hex copies are not
kept because the repository .gitignore excludes those extensions; the zip carries
them. No flashing, no device access.
"""
import hashlib
import os
import zipfile

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
KEIL = os.path.join(
    ROOT,
    "CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/output/exe",
)
OUT = os.path.join(ROOT, "fw_release")
EVID = os.path.join(ROOT, "evidence/fw_release_20261111")

TAG = "FD-002r8"
AXF_NAME = "fw_sensor_%s_b6a15fed.axf" % TAG
HEX_NAME = "fw_sensor_%s_4d728dec.hex" % TAG
AXF_MD5 = "b6a15fed2ed5e3d0c4adc51ac87a4830"
HEX_MD5 = "4d728dec9b0d28a352c46436b7586c72"
ZIP_NAME = "fw_sensor_%s.zip" % TAG
FIXED_TIME = (1980, 1, 1, 0, 0, 0)


def md5_bytes(data):
    return hashlib.md5(data).hexdigest()


def main():
    os.makedirs(OUT, exist_ok=True)
    os.makedirs(EVID, exist_ok=True)

    axf_path = os.path.join(KEIL, "Project.axf")
    hex_path = os.path.join(KEIL, "Project.hex")
    axf = open(axf_path, "rb").read()
    hexf = open(hex_path, "rb").read()
    if md5_bytes(axf) != AXF_MD5 or md5_bytes(hexf) != HEX_MD5:
        raise SystemExit("unexpected build identity: %s / %s" % (md5_bytes(axf), md5_bytes(hexf)))

    pkg = "\n".join([
        "Firmware release package - dengbei sensor (CW32L010)",
        "=====================================================",
        "",
        "Package tag       : " + TAG,
        "Built from commit : 82a7e74e5d6bae855a573e1f23f83936ba8db049 (sensor product source)",
        "Repository HEAD   : 9f7817f (sensor USER/MDK/EWARM/gcc unchanged since 82a7e74)",
        "Design baseline   : FD-002 rev 8 - LIGHT-DEF-2 criterion form; delivery default T=0 (criterion disabled)",
        "Firmware banner   : BOOT fw=FD-002r4 uid=6A002C00 rst=.. uart=9600 (fw string intentionally unchanged)",
        "Build tool        : Keil MDK / ARMCLANG V6.24; project MDK/Project.uvprojx (saved settings, unchanged)",
        'Build command     : mdk_build {"action":"rebuild"}',
        "Build result      : 0 Error / 1 Warning (pre-existing main.c(254) empty while body)",
        "Program size      : Code=25964 RO-data=656 RW-data=116 ZI-data=1676",
        "Target device     : CW32L010 (Keil Device CW32L010F8; BOM U7 = CW32L010Y8M6 - documented difference)",
        "",
        "Member files and MD5",
        "--------------------",
        AXF_NAME + " : " + AXF_MD5 + " (" + str(len(axf)) + " bytes)",
        HEX_NAME + " : " + HEX_MD5 + " (" + str(len(hexf)) + " bytes)",
        "",
        "Reproducibility",
        "---------------",
        "Three independent rebuilds of the same source produced this axf/hex MD5:",
        "  - firmware implementation round 2026-11-10 (evidence/fw_impl_20261110/build_evidence.md)",
        "  - embedded test round EV-035 2026-11-11 (evidence/ev035/image_identity.txt)",
        "  - this release round 2026-11-11 (evidence/fw_release_20261111/mdk_build_release.out.json)",
        "The zip is packed with fixed member timestamps so its own MD5 is reproducible.",
        "",
        "Download / programming instructions (NOT performed by the releasing role)",
        "-------------------------------------------------------------------------",
        "1. Use the saved Keil project MDK/Project.uvprojx with the saved CW-DAPLink debugger and the saved",
        '   CW32L010 flash algorithm (mdk_flash {}); the tool compiles then downloads.',
        "   Do not select another chip, debugger or algorithm, and do not change the project file.",
        '2. Require a flash log containing "Erase Done.Programming Done.Verify OK.Application running ...".',
        "   Verify OK only proves read-back of the programmed image, not function.",
        "3. After download, capture COM43 at 9600 8N1: BOOT/VDD/IOTEST/IOSIG/BUS once, then one S line per",
        "   3-minute cycle. With the delivery default (T=0) each S line must show V=0 D=0.",
        "4. Never flash the existing gateway; it is out of this release scope.",
        "",
        "Limitations and open items (full list in artifacts/firmware_release.md)",
        "----------------------------------------------------------------------",
        "- No-light criterion is DISABLED in this delivery (LIGHT_DARK_THRESHOLD_CODE=0): the rise-and-dark",
        "  upload branch cannot fire; only the strict >35.0 C branch is available.",
        "- Enabling the criterion requires an accepted criterion value plus passive lit/dark evidence;",
        "  that test item is human-confirmed deferred (NOT EXECUTED) for this delivery.",
        "- Real temperature rise / >35 C trigger, cross-link (gateway + phone) and power measurement were",
        "  not executed in this stage (user constraints; later integration stage).",
        "- This package is a build candidate with identity and instructions; it is NOT a statement of",
        "  functional acceptance of the product.",
        "",
    ])
    pkg_bytes = pkg.encode("utf-8")
    pkg_path = os.path.join(OUT, "PACKAGE.txt")
    with open(pkg_path, "wb") as fh:
        fh.write(pkg_bytes)

    zip_path = os.path.join(OUT, ZIP_NAME)
    if os.path.exists(zip_path):
        os.remove(zip_path)
    members = [(AXF_NAME, axf), (HEX_NAME, hexf), ("PACKAGE.txt", pkg_bytes)]
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as zf:
        for name, data in members:
            info = zipfile.ZipInfo(name, date_time=FIXED_TIME)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            zf.writestr(info, data)

    zip_md5 = md5_bytes(open(zip_path, "rb").read())
    with open(os.path.join(EVID, "md5_list.txt"), "w", encoding="utf-8", newline="\n") as fh:
        fh.write("%s *fw_release/%s   # 发布包(确定性打包)\n" % (zip_md5, ZIP_NAME))
        fh.write("%s *fw_release/PACKAGE.txt   # 包内自述\n" % md5_bytes(pkg_bytes))
        fh.write("%s *%s   # 重建产物(不入库，可复现)\n"
                 % (AXF_MD5, os.path.relpath(axf_path, ROOT).replace("\\", "/")))
        fh.write("%s *%s   # 重建产物(不入库，可复现)\n"
                 % (HEX_MD5, os.path.relpath(hex_path, ROOT).replace("\\", "/")))

    print("members:")
    for name, data in members:
        print("  %-44s %8d  %s" % (name, len(data), md5_bytes(data)))
    print("zip: %s  %d bytes  md5=%s" % (ZIP_NAME, os.path.getsize(zip_path), zip_md5))


if __name__ == "__main__":
    main()
