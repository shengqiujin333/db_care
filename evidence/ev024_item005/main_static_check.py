#!/usr/bin/env python3
"""ITEM-005 source-order/static checks for USER/src/main.c (implementation-side)."""
from pathlib import Path

p = Path(__file__).resolve().parents[2] / "CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/src/main.c"
s = p.read_text(encoding="utf-8")
checks = []

boot = s.index("debug_trace_boot")
uid = s.index("if (k == 0u)")
checks.append(("BOOT before UID fatal", boot < uid))

start = s.index("if (k == 0u)")
end = s.index("k = 10000;", start)
block = s[start:end]
checks += [
    ("UID block clears report_req", "report_req = 0u;" in block),
    ("UID block clears sample_flag", "sample_flag = 0u;" in block),
    ("UID block calls light_init", "light_init();" in block),
    ("UID block closes UART", "debug_trace_flush_close();" in block),
    ("UID block deep-sleeps", "SYSCTRL_GotoDeepSleep();" in block),
    ("UID block does not sample", "temperature_process" not in block),
    ("UID block does not send", "send_data_to_gateway" not in block),
]

loop = s[s.index("while(1)\n    {"):]
checks += [
    ("loop is gated by sample_flag", "if (sample_flag != 0u)" in loop),
    ("loop calls temperature_process", "temperature_process();" in loop),
    ("loop calls send_data_to_gateway", "send_data_to_gateway();" in loop),
    ("loop fetches trace", "sensor_trace_fetch" in loop),
    ("loop closes UART", "debug_trace_flush_close();" in loop),
    ("loop calls go_to_sleep", "go_to_sleep();" in loop),
]
checks.append(("old UID busy loop absent", "while(1){\n\t\t\tgo_to_sleep();" not in s))
checks.append(("no GPIOA reset", "SYSCTRL_AHBPeriphReset" not in s))

bad = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(("PASS  " if ok else "FAIL  ") + name)
print(f"==== result: {len(checks) - len(bad)} passed, {len(bad)} failed ====")
raise SystemExit(1 if bad else 0)
