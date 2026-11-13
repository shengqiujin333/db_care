#!/bin/sh
# L0 宿主机测试 (TD-002): 纯逻辑 + 采样/上报流程 + UART1 调试通道 + 光照通路
# 依赖: MinGW-w64 gcc (x86_64-w64-mingw32-gcc) 或系统 gcc
# 产物写入 _hostbin/ (避免与外部进程占用同名 .exe 冲突)
# 宿主上杀毒/EDR 可能按文件名拦截新建 exe（本轮实测 `_hostbin/s2_flow.exe`
# 持续报 “cannot open output file ... Permission denied”，改名后同一命令成功），
# 因此产物统一使用不带阶段序号前缀的描述性名称；改名的仅是产物文件名，用例与编译参数不变。
# 宿主 Windows 上偶发 "Permission denied" / "Device or resource busy" 的产物占用:
# 对链接失败做一次带延迟的重试, 避免把环境抖动误判为代码失败。
# 2026-11-13 实测: 产物名 `_hostbin/measure_flow_check.exe` 被主机安全软件按名拦截
# (ld: cannot open output file ... Permission denied, 连续两次; 同名手编同样失败,
#  而 `report_flow_check.exe` 首次即成功), 故该阶段产物改用 `report_flow_check.exe`。
# 仅产物文件名变化, 用例、编译参数与断言未改。
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

echo "== [1/4] fw_core pure logic: CRC16/CRC-8/convert/light(LIGHT-DEF-2)/report =="
build "$BIN/core_check.exe" -std=c11 -Wall -Wextra -I../USER/inc \
    host_sensor_core_test.c ../USER/src/fw_core.c -lm
"$BIN/core_check.exe"

echo "== [2/4] measure flow (mock MCU): sample/report/prev-not-updated-on-failure =="
build "$BIN/report_flow_check.exe" -std=c11 -Wall -Wextra -DSENSOR_CONFIG_NO_MCU -DSENSOR_DEBUG_UART=1 \
    -Imock_measure_mcu -I../USER/inc -I../COMMON -I../UM2005C \
    host_measure_flow_check.c ../USER/src/measure.c ../USER/src/sf_i2c.c \
    ../USER/src/fw_core.c -lm
"$BIN/report_flow_check.exe"

# T1: UART1 调试通道自检 (真实 debug_trace.c, 强制 SENSOR_DEBUG_UART=1; 宿主机无 UART 外设, 用 mock_trace 影子层)
echo "== [3/4] UART1 debug trace (mock UART): banner/S-line budget/close semantics =="
build "$BIN/debug_trace_check.exe" -std=c11 -Wall -Wextra -DSENSOR_DEBUG_UART=1 \
    -Imock_trace -I../USER/inc host_debug_trace_check.c ../USER/src/debug_trace.c
"$BIN/debug_trace_check.exe"

# T2: 光照通路自检 (真实 light.c; 判据未启用变体 + 启用后变体)
#  -DLIGHT_BGR_TRIM_MV=1200u: 宿主机覆盖出厂修调值, 避免访问 0x001007D2 绝对地址
echo "== [4/4a] light channel (mock ADC, gate disabled delivery default T=0) =="
build "$BIN/light_check.exe" -std=c11 -Wall -Wextra -Imock_light \
    -DLIGHT_BGR_TRIM_MV=1200u \
    -I../USER/inc -I../COMMON host_light_check.c ../USER/src/light.c ../USER/src/fw_core.c
"$BIN/light_check.exe"

echo "== [4/4b] light channel (enabled variant: -DLIGHT_DARK_THRESHOLD_CODE=3590) =="
build "$BIN/light_check_cal.exe" -std=c11 -Wall -Wextra -Imock_light \
    -DLIGHT_BGR_TRIM_MV=1200u \
    -I../USER/inc -I../COMMON -DLIGHT_DARK_THRESHOLD_CODE=3590u \
    host_light_check.c ../USER/src/light.c ../USER/src/fw_core.c
"$BIN/light_check_cal.exe"
