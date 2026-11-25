#!/usr/bin/env bash
# Read-mostly verification of the POST /upload_data acceptance path on the platform host.
#
# It never sends a payload that can reach mysql.connect(): the only POST bodies used
# are `{}` (rejected as "No json data") and `{"probe":1}` (rejected as
# "phone is required" once the header gate is gone). No credentials are printed.
#
# usage: verify_upload_contract.sh --phase pre-fix|post-fix [--base http://127.0.0.1:5000]
set -uo pipefail

PHASE=""
BASE="http://127.0.0.1:5000"
while [ $# -gt 0 ]; do
  case "$1" in
    --phase) PHASE="$2"; shift 2 ;;
    --base)  BASE="$2";  shift 2 ;;
    *) echo "unknown arg: $1"; exit 64 ;;
  esac
done
[ "$PHASE" = "pre-fix" ] || [ "$PHASE" = "post-fix" ] || { echo "usage: $0 --phase pre-fix|post-fix"; exit 64; }

fail=0
say() { printf '%s\n' "$*"; }
check() { # name expected actual
  if [ "$2" = "$3" ]; then say "  OK   $1 ($3)"; else say "  FAIL $1 expected=$2 actual=$3"; fail=1; fi
}

say "== unit =="
UNIT_STATE=$(systemctl is-active user_script.service 2>/dev/null)
PID=$(systemctl show user_script -p ExecMainPID --value)
NRESTART=$(systemctl show user_script -p NRestarts --value)
STARTED=$(systemctl show user_script -p ExecMainStartTimestamp --value)
say "unit_active=$UNIT_STATE pid=$PID nrestarts=$NRESTART started=$STARTED"
check "unit active" "active" "$UNIT_STATE"

say "== route shape =="
GET_CODE=$(curl -sS -o /dev/null -w '%{http_code}' --max-time 8 "$BASE/upload_data")
OPT_CODE=$(curl -sS -o /dev/null -w '%{http_code}' -X OPTIONS --max-time 8 "$BASE/upload_data")
LOGIN_CODE=$(curl -sS -o /dev/null -w '%{http_code}' --max-time 8 "$BASE/login")
check "GET /upload_data" "405" "$GET_CODE"
check "OPTIONS /upload_data" "200" "$OPT_CODE"
check "GET /login (regression)" "405" "$LOGIN_CODE"

say "== body validation unchanged (no DB path) =="
EMPTY=$(curl -sS --max-time 8 -H 'Content-Type: application/json' -d '{}' -w '|%{http_code}' "$BASE/upload_data")
EMPTY_BODY=${EMPTY%|*}; EMPTY_CODE=${EMPTY##*|}
say "  empty_json -> $EMPTY_CODE $EMPTY_BODY"
check "empty json status" "400" "$EMPTY_CODE"

say "== acceptance gate probe (payload cannot reach mysql.connect) =="
PROBE=$(curl -sS --max-time 8 -H 'Content-Type: application/json' -d '{"probe":1}' -w '|%{http_code}' "$BASE/upload_data")
PROBE_BODY=${PROBE%|*}; PROBE_CODE=${PROBE##*|}
say "  probe_json -> $PROBE_CODE $PROBE_BODY"

if [ "$PHASE" = "pre-fix" ]; then
  check "gate still rejects without header" "401" "$PROBE_CODE"
  say "  expectation: 401 {\"error\":\"unauthorized\"} = defect baseline present"
else
  check "gate removed (reaches body validation)" "400" "$PROBE_CODE"
  if printf '%s' "$PROBE_BODY" | grep -q 'phone is required'; then
    say "  OK   body advanced to phone validation (no 401, no DB write)"
  else
    say "  FAIL unexpected body after gate removal: $PROBE_BODY"; fail=1
  fi
fi

say "== recent service log =="
journalctl -u user_script --since '-10 min' --no-pager -o short-iso 2>/dev/null | tail -12 | sed 's/^/  /'

if [ "$fail" -eq 0 ]; then
  say "VERDICT=PASS phase=$PHASE"
  exit 0
fi
say "VERDICT=FAIL phase=$PHASE"
exit 1
