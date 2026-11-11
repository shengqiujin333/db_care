#!/bin/sh
# EV-035 host-side independent verification rerun (embedded_tester assets).
# Run from the sensor project root:
#   cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output
#   sh <repo>/evidence/ev035/run_host_harnesses.sh
# Same harness set as EV-034 (LIGHT-DEF-2 / FD-002 rev 8), fresh output paths.
CC="C:/ProgramData/chocolatey/lib/mingw/tools/install/mingw64/bin/gcc.exe"
OUT="D:/mypro/beiwo2/AIP/evidence/ev035"
BIN="_hostbin_ev035"
GW="../../../../CH592EVT/EVT/EXAM/BLE/beiwov2/APP"
mkdir -p "$BIN" "$OUT"
FAILED=""

run() { # run <name> <log> <cmd...>
    name="$1"; log="$2"; shift 2
    "$@" > "$OUT/$log" 2>&1
    rc=$?
    echo "== $name: exit=$rc"
    tail -n 6 "$OUT/$log"
    if [ "$rc" -ne 0 ]; then FAILED="$FAILED $name(exit=$rc)"; fi
}

compile() { # compile <name> <log> <gcc args...>
    name="$1"; log="$2"; shift 2
    $CC "$@" > "$OUT/$log" 2>&1
    rc=$?
    echo "== compile $name: exit=$rc"
    if [ "$rc" -ne 0 ]; then tail -n 12 "$OUT/$log"; FAILED="$FAILED compile-$name(exit=$rc)"; fi
    return $rc
}

echo "### 1. implementer host suite test/build_test.sh"
CC="$CC" sh test/build_test.sh > "$OUT/host_build_test.log" 2>&1
rc=$?
echo "== build_test.sh: exit=$rc"
tail -n 8 "$OUT/host_build_test.log"
if [ "$rc" -ne 0 ]; then FAILED="$FAILED build_test(exit=$rc)"; fi

echo "### 2. tester harnesses"
compile fw_core host_fw_core_verify_ev.build.log -std=c11 -Wall -Wextra -IUSER/inc \
    test/host_fw_core_verify_ev.c USER/src/fw_core.c -lm -o "$BIN/fw_core_verify.exe" && \
    run fw_core host_fw_core_verify_ev.log "$BIN/fw_core_verify.exe"

compile gxht40 host_gxht40_verify_ev.build.log -std=c11 -Wall -Wextra -Wno-int-to-pointer-cast \
    -Itest/mock_mcu -IUSER/inc -ICOMMON test/host_gxht40_verify_ev.c \
    USER/src/gxht40.c USER/src/sf_i2c.c USER/src/fw_core.c \
    -Wl,--wrap=i2c_bus_recover -lm -o "$BIN/gxht40_verify.exe" && \
    run gxht40 host_gxht40_verify_ev.log "$BIN/gxht40_verify.exe"

compile sf_i2c host_sf_i2c_verify_ev.build.log -std=c11 -Wall -Wextra -IUSER/inc \
    test/host_sf_i2c_verify_ev.c USER/src/sf_i2c.c -o "$BIN/sf_i2c_verify.exe" && \
    run sf_i2c host_sf_i2c_verify_ev.log "$BIN/sf_i2c_verify.exe"

compile diag_line host_diag_line_verify_ev.build.log -std=c11 -Wall -Wextra \
    -DSENSOR_DEBUG_UART=1 -Itest/mock_diagline_ev -IUSER/inc \
    test/host_diag_line_verify_ev.c USER/src/debug_trace.c -o "$BIN/diag_line_verify.exe" && \
    run diag_line host_diag_line_verify_ev.log "$BIN/diag_line_verify.exe"

compile diag_probe host_diag_probe_verify_ev.build.log -std=c11 -Wall -Wextra -Wno-unused-function \
    -DSENSOR_DEBUG_UART=1 -Itest/mock_measure_ev -IUSER/inc -IUM2005C -ICOMMON \
    test/host_diag_probe_verify_ev.c USER/src/measure.c USER/src/gxht40.c USER/src/sf_i2c.c \
    USER/src/fw_core.c USER/src/encrytogate.c -lm -o "$BIN/diag_probe_verify.exe" && \
    run diag_probe host_diag_probe_verify_ev.log "$BIN/diag_probe_verify.exe"

compile measure_flow host_measure_flow_verify_ev.build.log -std=c11 -Wall -Wextra -Wno-unused-function \
    -Itest/mock_measure_ev -IUSER/inc -IUM2005C -ICOMMON \
    test/host_measure_flow_verify_ev.c USER/src/measure.c USER/src/fw_core.c \
    USER/src/sf_i2c.c USER/src/encrytogate.c -lm -o "$BIN/measure_flow_verify.exe" && \
    run measure_flow host_measure_flow_verify_ev.log "$BIN/measure_flow_verify.exe"

compile rf_frame host_rf_frame_verify_ev.build.log -std=c11 -Wall -Wextra -Wno-unused-function \
    -Wno-unused-const-variable -Wno-misleading-indentation -IUSER/inc -I"$GW" \
    test/host_rf_frame_verify_ev.c USER/src/encrytogate.c "$GW/feistel_al.c" \
    -o "$BIN/rf_frame_verify.exe" && \
    run rf_frame host_rf_frame_verify_ev.log "$BIN/rf_frame_verify.exe"

compile rf_report host_rf_report_verify_ev.build.log -std=c11 -Wall -Wextra -Wno-unused-function \
    -Itest/mock_measure_ev -IUSER/inc -IUM2005C -ICOMMON \
    test/host_rf_report_verify_ev.c USER/src/measure.c USER/src/fw_core.c \
    USER/src/sf_i2c.c USER/src/encrytogate.c -lm -o "$BIN/rf_report_verify.exe" && \
    run rf_report host_rf_report_verify_ev.log "$BIN/rf_report_verify.exe"

compile rtc_main host_rtc_cadence_verify_ev.main.build.log -c -ffunction-sections -fdata-sections \
    -Dmain=firmware_main_entry -Itest/mock_main_ev -IUSER/inc -IUM2005C -ICOMMON \
    USER/src/main.c -o "$BIN/main_ev.o"
compile rtc_harness host_rtc_cadence_verify_ev.harness.build.log -c -ffunction-sections -fdata-sections \
    -Itest/mock_main_ev -IUSER/inc -IUM2005C -ICOMMON \
    test/host_rtc_cadence_verify_ev.c -o "$BIN/rtc_ev.o"
if [ -f "$BIN/main_ev.o" ] && [ -f "$BIN/rtc_ev.o" ]; then
    $CC -Wl,--gc-sections "$BIN/main_ev.o" "$BIN/rtc_ev.o" -o "$BIN/rtc_verify.exe" \
        > "$OUT/host_rtc_cadence_verify_ev.link.log" 2>&1
    if [ $? -eq 0 ]; then
        run rtc host_rtc_cadence_verify_ev.log "$BIN/rtc_verify.exe"
    else
        FAILED="$FAILED link-rtc"; tail -n 12 "$OUT/host_rtc_cadence_verify_ev.link.log"
    fi
fi

compile gw_compat host_gw_compat_verify_ev.build.log -std=c11 -Wall -Wextra -Wno-unused-function \
    -Wno-unused-const-variable -Wno-misleading-indentation -IUSER/inc -I"$GW" \
    test/host_gw_compat_verify_ev.c USER/src/encrytogate.c "$GW/feistel_al.c" \
    -o "$BIN/gw_compat_verify.exe" && \
    run gw_compat host_gw_compat_verify_ev.log "$BIN/gw_compat_verify.exe"

compile vdd host_vdd_verify_ev.build.log -std=c11 -Wall -Wextra -Itest/mock_vdd_ev -IUSER/inc -ICOMMON \
    -DLIGHT_BGR_TRIM_MV=mock_bgr_trim_value \
    test/host_vdd_verify_ev.c USER/src/light.c USER/src/fw_core.c -o "$BIN/vdd_verify.exe" && \
    run vdd host_vdd_verify_ev.log "$BIN/vdd_verify.exe"

echo "### 3. independent light.c wiring: delivery T=0 and enabled T=3000"
compile light_disabled host_light_verify_ev.build.log -std=c11 -Wall -Wextra \
    -Itest/mock_mcu -IUSER/inc -ICOMMON test/host_light_verify_ev.c \
    USER/src/light.c USER/src/fw_core.c -o "$BIN/light_verify.exe" && \
    run light_disabled host_light_verify_ev.log "$BIN/light_verify.exe"

compile light_enabled host_light_verify_ev_enabled.build.log -std=c11 -Wall -Wextra \
    -DLIGHT_DARK_THRESHOLD_CODE=3000u -Itest/mock_mcu -IUSER/inc -ICOMMON \
    test/host_light_verify_ev.c USER/src/light.c USER/src/fw_core.c -o "$BIN/light_verify_enabled.exe" && \
    run light_enabled host_light_verify_ev_enabled.log "$BIN/light_verify_enabled.exe"

echo "### RESULT: FAILED=[$FAILED]"
