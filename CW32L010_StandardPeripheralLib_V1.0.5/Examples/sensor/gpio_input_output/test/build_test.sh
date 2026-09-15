#!/bin/sh
# L0 宿主机纯逻辑单元测试 (TD-001 6.1.3): 编译并运行
# 依赖: MinGW-w64 gcc (x86_64-w64-mingw32-gcc) 或系统 gcc
set -e
cd "$(dirname "$0")"
CC=${CC:-gcc}
$CC -std=c11 -Wall -Wextra -I../USER/inc host_sensor_core_test.c \
    ../USER/src/fw_core.c ../USER/src/history.c -lm -o host_sensor_core_test.exe
./host_sensor_core_test.exe
