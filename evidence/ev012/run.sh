#!/bin/sh
# usage: run.sh <out-prefix> [flash-delay-sec]
set -u
D="D:/mylearn/restlessdream_vnext_r4_60_runtime_manifest_codegraph_rl_20260911_DELIVERY"
PY="C:/Users/kason/anaconda3/python.exe"
CLI="$D/company_runtime/hardware_verification/tool_cli.py"
CTX="$D/projects/dengbei_run8/runtime/calls/CALL-59c453eb-f965-4ee8-8e95-3205fad7cec9/context.md"
WS="$D/projects/dengbei_run8/roles/embedded_tester"
OUT="$1"; DELAY="${2:-4}"
date -u +"start %H:%M:%S"
"$PY" "$CLI" --context "$CTX" --role-workspace "$WS" --tool serial_capture --arguments-file args_cap300.json > "$OUT.cap.json" 2> "$OUT.cap.err" &
CPID=$!
sleep "$DELAY"
"$PY" "$CLI" --context "$CTX" --role-workspace "$WS" --tool mdk_flash --arguments-file args_flash.json > "$OUT.flash.json" 2> "$OUT.flash.err"
echo "flash rc=$? at $(date -u +%H:%M:%S)"
wait $CPID
echo "capture rc=$? at $(date -u +%H:%M:%S)"
