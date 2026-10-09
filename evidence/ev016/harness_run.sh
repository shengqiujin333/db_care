#!/bin/sh
# EV-016 independent host-harness re-run against the delivered firmware source.
# Only USER/src/* comes from the delivered firmware; harnesses/mocks are tester-owned.
set -u
P="D:/mypro/beiwo2/AIP/CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output"
cd "$P" || exit 1
mkdir -p _ev016bin
CC=${CC:-gcc}
BASE="-std=c11 -Wall -Wextra -Wno-unused-function -Wno-misleading-indentation -Wno-unused-const-variable -Wno-int-to-pointer-cast"
echo "# EV-016 independent host harness re-runs ($($CC --version | head -1))"
echo "# firmware source under test: $(git -C D:/mypro/beiwo2/AIP log -1 --format=%h -- CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER)"

banner() { echo; echo "\$ $1"; }

banner "gcc ... host_diag_probe_verify_ev.c (ITEM-001 BUS/G + probe)"
$CC $BASE -DSENSOR_DEBUG_UART=1 -Itest/mock_measure_ev -IUSER/inc -IUM2005C -ICOMMON \
  test/host_diag_probe_verify_ev.c USER/src/measure.c USER/src/gxht40.c USER/src/sf_i2c.c \
  USER/src/fw_core.c USER/src/encrytogate.c -lm -o _ev016bin/diag_probe_verify 2>&1 | grep -v "^$" | tail -5
./_ev016bin/diag_probe_verify 2>&1 | tail -3; echo "exit=$?"

banner "gcc ... host_gxht40_verify_ev.c (-Wl,--wrap=i2c_bus_recover)"
$CC $BASE -Itest/mock_mcu -IUSER/inc -ICOMMON \
  test/host_gxht40_verify_ev.c USER/src/gxht40.c USER/src/sf_i2c.c USER/src/fw_core.c \
  -Wl,--wrap=i2c_bus_recover -o _ev016bin/gxht40_verify 2>&1 | tail -5
./_ev016bin/gxht40_verify 2>&1 | tail -3; echo "exit=$?"

banner "gcc ... host_sf_i2c_verify_ev.c"
$CC $BASE -IUSER/inc test/host_sf_i2c_verify_ev.c USER/src/sf_i2c.c -o _ev016bin/sfi2c_verify 2>&1 | tail -5
./_ev016bin/sfi2c_verify 2>&1 | tail -3; echo "exit=$?"

banner "gcc ... host_diag_line_verify_ev.c"
$CC $BASE -DSENSOR_DEBUG_UART=1 -Itest/mock_diagline_ev -IUSER/inc \
  test/host_diag_line_verify_ev.c USER/src/debug_trace.c -o _ev016bin/diag_line_verify 2>&1 | tail -5
./_ev016bin/diag_line_verify 2>&1 | tail -3; echo "exit=$?"

banner "gcc ... host_fw_core_verify_ev.c"
$CC $BASE -IUSER/inc -ICOMMON test/host_fw_core_verify_ev.c USER/src/fw_core.c -o _ev016bin/fwcore_verify 2>&1 | tail -5
./_ev016bin/fwcore_verify 2>&1 | tail -3; echo "exit=$?"

# NOTE: gxht40.c is intentionally NOT linked here - this harness supplies its own
# scripted gxht40_* stubs (the flow logic is what is under test).
banner "gcc ... host_measure_flow_verify_ev.c -DSENSOR_DEBUG_UART=0 (delivered default)"
$CC $BASE -DSENSOR_DEBUG_UART=0 -Itest/mock_measure_ev -IUSER/inc -IUM2005C -ICOMMON \
  test/host_measure_flow_verify_ev.c USER/src/measure.c USER/src/sf_i2c.c \
  USER/src/fw_core.c USER/src/encrytogate.c -lm -o _ev016bin/measure_verify 2>&1 | grep -E "error|Error"
./_ev016bin/measure_verify 2>&1 | tail -3; echo "exit=$?"

banner "gcc ... host_measure_flow_verify_ev.c -DSENSOR_DEBUG_UART=1"
$CC $BASE -DSENSOR_DEBUG_UART=1 -Itest/mock_measure_ev -IUSER/inc -IUM2005C -ICOMMON \
  test/host_measure_flow_verify_ev.c USER/src/measure.c USER/src/sf_i2c.c \
  USER/src/fw_core.c USER/src/encrytogate.c -lm -o _ev016bin/measure_verify_dbg 2>&1 | grep -E "error|Error"
./_ev016bin/measure_verify_dbg 2>&1 | tail -3; echo "exit=$?"

echo; echo "ALL HARNESSES DONE"
