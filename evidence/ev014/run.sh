#!/bin/sh
# usage: run.sh <out-prefix> [flash-delay-sec]
# EV-014 (run8 ITEM-002 复测：用户已重新焊接 U9 GXHT40 后授权重做第二项)
# start a 300 s COM42 capture, then run mdk_flash {} inside that window so the
# reset banner + BOOT/IOTEST/IOSIG/BUS lines land in the same raw capture as the
# following sampling periods.
set -u
D="D:/mylearn/restlessdream_vnext_r4_60_runtime_manifest_codegraph_rl_20260911_DELIVERY"
PY="C:/Users/kason/anaconda3/python.exe"
CLI="$D/company_runtime/hardware_verification/tool_cli.py"
CTX="$D/projects/dengbei_run8/runtime/calls/CALL-4aaa5528-9d4a-4844-8930-0edca33b2e65/context.md"
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
