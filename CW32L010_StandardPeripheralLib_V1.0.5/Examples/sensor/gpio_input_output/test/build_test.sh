#!/bin/sh
# L0 宿主机测试 (TD-002): 纯逻辑 + 采样/上报流程 + UART1 调试通道 + 光照通路
# 依赖: MinGW-w64 gcc (x86_64-w64-mingw32-gcc) 或系统 gcc
# 产物写入 _hostbin/ (避免与外部进程占用同名 .exe 冲突)
# 宿主 Windows 上偶发 "Permission denied" / "Device or resource busy" 的产物占用:
# 对链接失败做一次带延迟的重试, 避免把环境抖动误判为代码失败。
set -e
cd "$(dirname "$0")"
CC=${CC:-gcc}
BIN=_hostbin
rm -rf "$BIN" 2>/dev/null || true
mkdir -p "$BIN"

build() {
    out="$1"; shift
    $CC "$@" -o "$out" 2>/tmp/bt_cc_err.log || {
        echo "  (编译产物被占用, 2 s 后重试: $out)"
        sleep 2
        rm -f "$out" 2>/dev/null || true
        $CC "$@" -o "$out"
    }
}

echo "== [1/4] fw_core pure logic: CRC16/CRC-8/convert/light(1/3)/report =="
build "$BIN/s1_core.exe" -std=c11 -Wall -Wextra -I../USER/inc \
    host_sensor_core_test.c ../USER/src/fw_core.c -lm
"$BIN/s1_core.exe"

echo "== [2/4] measure flow (mock MCU): sample/report/prev-not-updated-on-failure =="
build "$BIN/s2_flow.exe" -std=c11 -Wall -Wextra -DSENSOR_CONFIG_NO_MCU \
    -Imock_measure_mcu -I../USER/inc -I../COMMON -I../UM2005C \
    host_measure_flow_check.c ../USER/src/measure.c ../USER/src/sf_i2c.c \
    ../USER/src/fw_core.c -lm
"$BIN/s2_flow.exe"

# T1: UART1 调试通道自检 (真实 debug_trace.c, 强制 SENSOR_DEBUG_UART=1; 宿主机无 UART 外设, 用 mock_trace 影子层)
echo "== [3/4] UART1 debug trace (mock UART): banner/S-line budget/close semantics =="
build "$BIN/s3_trace.exe" -std=c11 -Wall -Wextra -DSENSOR_DEBUG_UART=1 \
    -Imock_trace -I../USER/inc host_debug_trace_check.c ../USER/src/debug_trace.c
"$BIN/s3_trace.exe"

# T2: 光照通路自检 (真实 light.c; 未标定变体 + 标定后变体)
echo "== [4/4a] light channel (mock ADC, uncalibrated delivery default) =="
build "$BIN/s4_light.exe" -std=c11 -Wall -Wextra -Imock_light \
    -I../USER/inc -I../COMMON host_light_check.c ../USER/src/light.c ../USER/src/fw_core.c
"$BIN/s4_light.exe"

echo "== [4/4b] light channel (calibrated variant: -DLIGHT_DARK_CALIBRATED=1 -DLIGHT_DARK_REF_CODE=3000) =="
build "$BIN/s4_light_cal.exe" -std=c11 -Wall -Wextra -Imock_light \
    -I../USER/inc -I../COMMON -DLIGHT_DARK_CALIBRATED=1 -DLIGHT_DARK_REF_CODE=3000u \
    host_light_check.c ../USER/src/light.c ../USER/src/fw_core.c
"$BIN/s4_light_cal.exe"
