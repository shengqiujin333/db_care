#!/bin/sh
# L0 宿主机测试 (TD-002): 纯逻辑 + 采样/上报流程 + UART1 调试通道
# 依赖: MinGW-w64 gcc (x86_64-w64-mingw32-gcc) 或系统 gcc
set -e
cd "$(dirname "$0")"
CC=${CC:-gcc}

echo "== [1/3] fw_core pure logic: CRC16/CRC-8/convert/light/report =="
$CC -std=c11 -Wall -Wextra -I../USER/inc host_sensor_core_test.c \
    ../USER/src/fw_core.c -lm -o host_sensor_core_test.exe
./host_sensor_core_test.exe

echo "== [2/3] measure flow (mock MCU): sample/report/prev-not-updated-on-failure =="
$CC -std=c11 -Wall -Wextra -DSENSOR_CONFIG_NO_MCU -Imock_measure_mcu \
    -I../USER/inc -I../COMMON -I../UM2005C \
    host_measure_flow_check.c ../USER/src/measure.c ../USER/src/sf_i2c.c \
    ../USER/src/fw_core.c -lm -o host_measure_flow_check.exe
./host_measure_flow_check.exe

# T1: UART1 调试通道自检 (真实 debug_trace.c, 强制 SENSOR_DEBUG_UART=1; 宿主机无 UART 外设, 用 mock_trace 影子层)
echo "== [3/3] UART1 debug trace (mock UART): banner/S-line budget/close semantics =="
$CC -std=c11 -Wall -Wextra -DSENSOR_DEBUG_UART=1 -Imock_trace -I../USER/inc \
    host_debug_trace_check.c ../USER/src/debug_trace.c -o host_debug_trace_check.exe
./host_debug_trace_check.exe
