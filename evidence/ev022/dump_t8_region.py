#!/usr/bin/env python3
"""Dump the exact byte region of the T8/T8b block for a precise test-asset patch."""
PATH = ("D:/mypro/beiwo2/AIP/CW32L010_StandardPeripheralLib_V1.0.5/Examples/"
        "sensor/gpio_input_output/test/host_fw_core_verify_ev.c")

with open(PATH, "rb") as f:
    d = f.read()

i = d.find(b"#define REF_REPORT_HASH")
print("HASH DEF:", repr(d[i:i + 45]))
print()
start = d.find(b"    /* ---- T8:")
end = d.find(b'    printf("==== result')
print(repr(d[start:end]))
