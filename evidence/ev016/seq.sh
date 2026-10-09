#!/bin/sh
# EV-016 capture sequence: one flash window immediately followed by
# back-to-back capture windows, so the whole 3-minute k grid after the
# download reset is covered WITHOUT gaps.
#
# usage: seq.sh <flash-prefix> <chain-prefix-1> <chain-prefix-2> ...
# Run from the role work directory (the .cap.json files land there).
set -u
D="D:/mylearn/restlessdream_vnext_r4_60_runtime_manifest_codegraph_rl_20260911_DELIVERY"
PY="C:/Users/kason/anaconda3/python.exe"
CLI="$D/company_runtime/hardware_verification/tool_cli.py"
CTX="$D/projects/dengbei_run8/runtime/calls/CALL-e71e0e66-be6a-4c19-b745-c022238fe6a1/context.md"
WS="$D/projects/dengbei_run8/roles/embedded_tester"

# window 1: capture starts, flash fires inside it -> boot banner lands in capture
FLASH="$1"; shift
"$PY" "$CLI" --context "$CTX" --role-workspace "$WS" --tool serial_capture \
    --arguments-file args_cap300.json > "$FLASH.cap.json" 2> "$FLASH.cap.err" &
CPID=$!
sleep 4
"$PY" "$CLI" --context "$CTX" --role-workspace "$WS" --tool mdk_flash \
    --arguments-file args_flash.json > "$FLASH.flash.json" 2> "$FLASH.flash.err"
echo "flash rc=$? at $(date -u +%H:%M:%S)"
wait $CPID
echo "$FLASH done rc=$? at $(date -u +%H:%M:%S)"

# remaining windows: back-to-back with no idle gap
for n in "$@"; do
  "$PY" "$CLI" --context "$CTX" --role-workspace "$WS" --tool serial_capture \
      --arguments-file args_cap300.json > "$n.cap.json" 2> "$n.cap.err"
  echo "$n done rc=$? at $(date -u +%H:%M:%S)"
done
echo "ALL DONE"
