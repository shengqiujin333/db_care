#!/bin/sh
# ITEM-006: LIGHT_DARK_* compile-time gate self-check.
# Usage (from test/): sh check_calib_config.sh
set -u
CC=${CC:-gcc}
INC=../USER/inc
TMP=${TMPDIR:-/tmp}/calib_cfg_$$
mkdir -p "$TMP" || exit 1
trap 'rm -rf "$TMP"' EXIT INT TERM
printf '#include "sensor_config.h"\nint main(void) { return 0; }\n' > "$TMP/t.c"

pass=0
fail=0

try_ok() {
    name=$1; shift
    if "$CC" -std=c11 -DSENSOR_CONFIG_NO_MCU -I"$INC" "$@" "$TMP/t.c" -o "$TMP/t.exe" >"$TMP/out" 2>&1; then
        echo "PASS  $name (compiles)"
        pass=$((pass + 1))
    else
        echo "FAIL  $name (unexpected compile failure)"
        cat "$TMP/out"
        fail=$((fail + 1))
    fi
}

try_fail() {
    name=$1; shift
    if "$CC" -std=c11 -DSENSOR_CONFIG_NO_MCU -I"$INC" "$@" "$TMP/t.c" -o "$TMP/t.exe" >"$TMP/out" 2>&1; then
        echo "FAIL  $name (expected compile failure but succeeded)"
        fail=$((fail + 1))
    else
        echo "PASS  $name (rejected at compile time)"
        pass=$((pass + 1))
    fi
}

try_ok   "default uncalibrated (0/0)"
try_ok   "calibrated ref=1"      -DLIGHT_DARK_CALIBRATED=1 -DLIGHT_DARK_REF_CODE=1
try_ok   "calibrated ref=4095"   -DLIGHT_DARK_CALIBRATED=1 -DLIGHT_DARK_REF_CODE=4095
try_fail "CALIBRATED=2"          -DLIGHT_DARK_CALIBRATED=2
try_fail "calibrated ref=0"      -DLIGHT_DARK_CALIBRATED=1 -DLIGHT_DARK_REF_CODE=0
try_fail "calibrated ref=4096"   -DLIGHT_DARK_CALIBRATED=1 -DLIGHT_DARK_REF_CODE=4096

echo "==== result: $pass passed, $fail failed ===="
[ "$fail" -eq 0 ]
