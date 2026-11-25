#!/usr/bin/env bash
# Restore the pre-change POST /upload_data gate on the platform host.
#
# Restores the newest (or an explicitly named) /root/abr.py.bak_<ts>, restarts the unit
# and verifies the defect baseline is back (401 unauthorized without the header).
# Rollback restores a KNOWN-FAILING state: App uploads will 401 again. It must never be
# recorded as "healthy".
set -uo pipefail

FILE="/root/abr.py"
UNIT="user_script.service"
BACKUP=""
BASE="http://127.0.0.1:5000"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

while [ $# -gt 0 ]; do
  case "$1" in
    --file) FILE="$2"; shift 2 ;;
    --unit) UNIT="$2"; shift 2 ;;
    --backup) BACKUP="$2"; shift 2 ;;
    --base) BASE="$2"; shift 2 ;;
    *) echo "unknown arg: $1"; exit 64 ;;
  esac
done

if [ -z "$BACKUP" ]; then
  BACKUP=$(ls -1t "${FILE}".bak_* 2>/dev/null | head -1)
fi
[ -n "$BACKUP" ] && [ -f "$BACKUP" ] || { echo "RESULT=FAIL reason=backup_not_found"; exit 1; }

echo "restoring from $BACKUP"
sha256sum "$BACKUP"
cp -a "$BACKUP" "$FILE" || { echo "RESULT=FAIL reason=restore_failed"; exit 1; }
systemctl restart "$UNIT" || { echo "RESULT=FAIL reason=restart_failed"; exit 1; }

for i in $(seq 1 30); do
  CODE=$(curl -sS -o /dev/null -w '%{http_code}' -X OPTIONS --max-time 3 "$BASE/upload_data" 2>/dev/null)
  [ "$CODE" = "200" ] && break
  sleep 1
done

bash "$HERE/verify_upload_contract.sh" --phase pre-fix --base "$BASE"
RC=$?
echo "file_sha256=$(sha256sum "$FILE" | awk '{print $1}')"
if [ "$RC" -eq 0 ]; then
  echo "RESULT=ROLLED_BACK note=restored_known_failing_baseline_app_uploads_will_401"
else
  echo "RESULT=ROLLBACK_UNVERIFIED"
  exit 3
fi
