#!/bin/sh
# L0 宿主机测试 (TD-002): 纯逻辑 + 采样/上报流程
# 依赖: MinGW-w64 gcc (x86_64-w64-mingw32-gcc) 或系统 gcc
set -e
cd "$(dirname "$0")"
CC=${CC:-gcc}

echo "== [1/2] fw_core pure logic: CRC16/CRC-8/convert/light/report =="
$CC -std=c11 -Wall -Wextra -I../USER/inc host_sensor_core_test.c \
    ../USER/src/fw_core.c -lm -o host_sensor_core_test.exe
./host_sensor_core_test.exe

echo "== [2/2] measure flow (mock MCU): sample/report/prev-not-updated-on-failure =="
$CC -std=c11 -Wall -Wextra -DSENSOR_CONFIG_NO_MCU -Imock_measure_mcu \
    -I../USER/inc -I../COMMON -I../UM2005C \
    host_measure_flow_check.c ../USER/src/measure.c ../USER/src/sf_i2c.c \
    ../USER/src/fw_core.c -lm -o host_measure_flow_check.exe
./host_measure_flow_check.exe
