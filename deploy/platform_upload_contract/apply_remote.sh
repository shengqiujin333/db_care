#!/usr/bin/env bash
# Apply the reviewed POST /upload_data contract alignment on the platform host.
#
# Default is DRY RUN. A real change requires --confirm, which is only to be passed by
# an explicitly authorized deployment step with a maintenance window.
#
# Steps: preflight -> verify defect baseline -> backup -> minimal patch -> restart ->
# post-fix verification -> automatic rollback if verification fails.
# Never prints credentials or the removed token value.
set -uo pipefail

FILE="/root/abr.py"
UNIT="user_script.service"
PYTHON="/root/venv/bin/python"
WORKDIR=""
CONFIRM=0
BASE="http://127.0.0.1:5000"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

while [ $# -gt 0 ]; do
  case "$1" in
    --file) FILE="$2"; shift 2 ;;
    --unit) UNIT="$2"; shift 2 ;;
    --python) PYTHON="$2"; shift 2 ;;
    --workdir) WORKDIR="$2"; shift 2 ;;
    --base) BASE="$2"; shift 2 ;;
    --confirm) CONFIRM=1; shift ;;
    *) echo "unknown arg: $1"; exit 64 ;;
  esac
done

TS=$(date -u +%Y%m%dT%H%M%SZ)
WORKDIR=${WORKDIR:-/root/rd_upload_contract_$TS}
mkdir -p "$WORKDIR"
LOG="$WORKDIR/apply.log"
exec > >(tee -a "$LOG") 2>&1

fail() { echo "RESULT=FAIL reason=$*"; exit 1; }

echo "== identity =="
date -u +%Y-%m-%dT%H:%M:%SZ; hostname
echo "file=$FILE unit=$UNIT workdir=$WORKDIR confirm=$CONFIRM"

echo "== preflight =="
[ -f "$FILE" ] || fail "file_missing"
[ -x "$PYTHON" ] || fail "interpreter_missing"
systemctl is-active --quiet "$UNIT" || fail "unit_not_active"
BEFORE_SHA=$(sha256sum "$FILE" | awk '{print $1}')
BEFORE_SIZE=$(stat -c %s "$FILE")
echo "before_sha256=$BEFORE_SHA before_size=$BEFORE_SIZE"

echo "== baseline behaviour (expects 401 without header) =="
bash "$HERE/verify_upload_contract.sh" --phase pre-fix --base "$BASE"
PRE_RC=$?
if [ "$PRE_RC" -ne 0 ]; then
  echo "note: pre-fix verification did not report the expected 401 baseline (rc=$PRE_RC)."
  echo "      If the file already has no gate, the patch script will report ALREADY_APPLIED."
fi

echo "== patch dry run =="
DRY_OUT=$("$PYTHON" "$HERE/patch_upload_gate.py" --file "$FILE" --python "$PYTHON" --dry-run 2>&1)
DRY_RC=$?
echo "$DRY_OUT"
if [ "$DRY_RC" -ne 0 ]; then
  fail "dry_run_or_structure_rc_$DRY_RC"
fi
case "$DRY_OUT" in
  *ALREADY_APPLIED*)
    echo "RESULT=NO_CHANGE_NEEDED reason=gate_already_absent"
    exit 0 ;;
esac

if [ "$CONFIRM" -ne 1 ]; then
  echo "RESULT=DRY_RUN_ONLY no change applied; re-run with --confirm under an authorized window"
  exit 0
fi

echo "== backup =="
BACKUP="${FILE}.bak_${TS}"
cp -a "$FILE" "$BACKUP" || fail "backup_failed"
chmod 600 "$BACKUP"
echo "$BEFORE_SHA  $BACKUP" > "$WORKDIR/BACKUP_SHA256SUMS.txt"
echo "backup=$BACKUP backup_sha256=$(sha256sum "$BACKUP" | awk '{print $1}')"
cp -a "$HERE/patch_upload_gate.py" "$HERE/verify_upload_contract.sh" "$WORKDIR/" 2>/dev/null || true

echo "== apply minimal patch =="
"$PYTHON" "$WORKDIR/patch_upload_gate.py" --file "$FILE" --python "$PYTHON"
PATCH_RC=$?
echo "patch_rc=$PATCH_RC"
if [ "$PATCH_RC" -ne 0 ]; then
  echo "restoring backup because patch did not complete"
  cp -a "$BACKUP" "$FILE"
  fail "patch_rc_$PATCH_RC"
fi

echo "== restart service =="
systemctl restart "$UNIT" || { cp -a "$BACKUP" "$FILE"; systemctl restart "$UNIT"; fail "restart_failed"; }
UP=0
for i in $(seq 1 30); do
  CODE=$(curl -sS -o /dev/null -w '%{http_code}' -X OPTIONS --max-time 3 "$BASE/upload_data" 2>/dev/null)
  if [ "$CODE" = "200" ] && systemctl is-active --quiet "$UNIT"; then UP=1; break; fi
  sleep 1
done
[ "$UP" -eq 1 ] || {
  echo "service did not come back within 30s"
  cp -a "$BACKUP" "$FILE"; systemctl restart "$UNIT";
  fail "service_not_ready"
}
echo "service_ready pid=$(systemctl show "$UNIT" -p ExecMainPID --value) nrestarts=$(systemctl show "$UNIT" -p NRestarts --value)"

echo "== post-fix verification =="
bash "$WORKDIR/verify_upload_contract.sh" --phase post-fix --base "$BASE"
POST_RC=$?

echo "== certificate / listener regression (unchanged by this change) =="
echo | openssl s_client -connect 127.0.0.1:8883 -servername 8.140.23.253 2>/dev/null \
  | openssl x509 -noout -serial -ext subjectAltName 2>/dev/null

if [ "$POST_RC" -ne 0 ]; then
  echo "post-fix verification failed -> automatic rollback"
  cp -a "$BACKUP" "$FILE"
  systemctl restart "$UNIT"
  sleep 3
  bash "$WORKDIR/verify_upload_contract.sh" --phase pre-fix --base "$BASE" | tail -5
  echo "RESULT=ROLLED_BACK backup=$BACKUP"
  exit 2
fi

AFTER_SHA=$(sha256sum "$FILE" | awk '{print $1}')
echo "after_sha256=$AFTER_SHA after_size=$(stat -c %s "$FILE")"
echo "RESULT=APPLIED backup=$BACKUP"
echo "next: real App upload retest must confirm 2xx and persistence readback; this script does not write sensor_data"
exit 0
