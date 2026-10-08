#!/bin/sh
# usage: chain.sh <prefix1> <prefix2> ...   (300 s captures back to back, no flash)
set -u
D="D:/mylearn/restlessdream_vnext_r4_60_runtime_manifest_codegraph_rl_20260911_DELIVERY"
PY="C:/Users/kason/anaconda3/python.exe"
CLI="$D/company_runtime/hardware_verification/tool_cli.py"
CTX="$D/projects/dengbei_run8/runtime/calls/CALL-d5e1fec4-433d-4a2e-9450-8285a78185c8/context.md"
WS="$D/projects/dengbei_run8/roles/embedded_tester"
for n in "$@"; do
  "$PY" "$CLI" --context "$CTX" --role-workspace "$WS" --tool serial_capture \
      --arguments-file args_cap300.json > "$n.cap.json" 2> "$n.cap.err"
  echo "$n done rc=$? at $(date -u +%H:%M:%S)"
done
echo "ALL DONE"
