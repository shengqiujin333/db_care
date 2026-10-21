#!/bin/sh
# Independent rerun of the implementation-side self-check entry point.
# Raw stdout/stderr are captured verbatim; exit code is appended.
set +e
cd "$(dirname "$0")/../../CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test" || exit 90
sh build_test.sh > "$OUT_FILE" 2>&1
rc=$?
echo "EXIT_CODE=$rc" >> "$OUT_FILE"
echo "EXIT_CODE=$rc"
exit $rc
