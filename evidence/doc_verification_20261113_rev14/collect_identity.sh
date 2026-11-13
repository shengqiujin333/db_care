#!/bin/sh
# DOC-VERIFY-014 identity collection (read-only)
set -u
REPO=/d/mypro/beiwo2/AIP
OUT="$REPO/evidence/doc_verification_20261113_rev14"
cd "$REPO" || exit 1
{
  echo "DOC-VERIFY-014 identity checks"
  echo "time_local: $(date '+%Y-%m-%d %H:%M:%S %z')"
  echo "time_utc: $(date -u '+%Y-%m-%dT%H:%M:%SZ')"
  echo "repo: D:/mypro/beiwo2/AIP"
  echo "HEAD: $(git rev-parse HEAD)"
  echo "HEAD_subject: $(git log -1 --pretty=%s)"
  echo "branch: $(git rev-parse --abbrev-ref HEAD)"
  echo "--- git status --porcelain (start) ---"
  git status --porcelain
  echo "--- doc version headers ---"
  for f in docs/README.md docs/01_产品说明与版本变更.md docs/02_传感器固件安装与烧录.md docs/03_传感器调试串口手册.md docs/04_安卓App安装与使用.md docs/05_平台服务器配置与联调前提.md docs/06_维护排障与已知限制.md; do
    echo "$f :: $(sed -n '/文档包：DOC-PKG-001/p' "$f" | head -1)"
  done
  echo "docs/README.md title :: $(sed -n '1p' docs/README.md)"
  echo "user manual version :: $(sed -n '3p' dengbei_care/用户使用说明书.md)"
  echo "--- firmware release package ---"
  ls -l fw_release/fw_sensor_FD-002r8.zip
  md5sum fw_release/fw_sensor_FD-002r8.zip
  echo "--- APK candidate ---"
  ls -l dengbei_care/app/build/outputs/apk/debug/app-debug.apk
  sha256sum dengbei_care/app/build/outputs/apk/debug/app-debug.apk
  md5sum dengbei_care/app/build/outputs/apk/debug/app-debug.apk
  echo "--- TLS CA ---"
  md5sum dengbei_care/app/src/main/assets/jd-ca.crt
  echo "--- Android identity evidence ---"
  sed -n '1,12p' evidence/android_build_20261113/build_identity.txt
  sed -n '1,6p' evidence/android_build_20261113/apksigner_verify.txt
  sed -n '1,3p' evidence/android_build_20261113/apk_badging.txt
  cat evidence/android_build_20261113/test_summary.txt
  echo "--- firmware build evidence ---"
  sed -n '1,20p' evidence/fw_release_20261111/package_verify.txt
  echo "--- source diff a12d12b..HEAD -- dengbei_care ---"
  git diff --name-status a12d12b..HEAD -- dengbei_care
  echo "--- product source diff (should be empty) ---"
  git diff --stat a12d12b..HEAD -- dengbei_care/app/src/main
  echo "--- firmware source diff 82a7e74..HEAD (should be empty) ---"
  git diff --stat 82a7e74..HEAD -- CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/EWARM CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/UM2005C CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/COMMON
} > "$OUT/identity_checks.txt" 2>&1
