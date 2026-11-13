#!/bin/sh
# EV-039 RPT-13: build the sensor USER translation units with SENSOR_DEBUG_UART=0.
# Test-side verification asset only; does not modify product sources.
# Run from the sensor project root:
#   sh evidence/ev039/build_debug_off.sh
set -u
OUT="D:/mypro/beiwo2/AIP/evidence/ev039"
OBJ="gcc/obj_debugoff_ev039"
INC="-IUSER/inc -ICOMMON -IUM2005C -I../../../Libraries/inc -IMDK/RTE -I/c/Users/kason/AppData/Local/Arm/Packs/ARM/CMSIS/5.9.0/CMSIS/Core/Include"
mkdir -p "$OBJ"
FAIL=0
echo "== compile USER sources with -DSENSOR_DEBUG_UART=0 =="
for f in USER/src/debug_trace.c USER/src/measure.c USER/src/main.c USER/src/light.c USER/src/gxht40.c USER/src/fw_core.c USER/src/sf_i2c.c USER/src/encrytogate.c; do
  base=$(basename "$f" .c)
  arm-none-eabi-gcc -mcpu=cortex-m0plus -mthumb -O1 -Wall -Wextra -DSENSOR_DEBUG_UART=0 -c $INC "$f" -o "$OBJ/$base.o" 2>&1
  rc=$?
  echo "== $f: exit=$rc"
  if [ "$rc" -ne 0 ]; then FAIL=1; fi
done
echo "== symbol check on debug-off objects =="
echo "-- debug_trace.o defined symbols:"
arm-none-eabi-nm --defined-only "$OBJ/debug_trace.o" 2>&1
echo "-- undefined UART symbols referenced by measure.o/main.o/debug_trace.o:"
arm-none-eabi-nm -u "$OBJ/measure.o" "$OBJ/main.o" "$OBJ/debug_trace.o" 2>&1
echo "== RESULT: FAIL=$FAIL =="
exit "$FAIL"
