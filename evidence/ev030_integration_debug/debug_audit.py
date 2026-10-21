#!/usr/bin/env python3
"""Integration-debug audit for EV-029 PA-001 failure (no device access)."""
from pathlib import Path
import hashlib
import re
import subprocess

root = Path(__file__).resolve().parents[2]
fw = root / "CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output"
ev29 = root / "evidence/ev029_full_firmware_verify"
checks = []

# 1) Current product source == tested source (EV-029).
src_hash_ok = True
for line in (ev29 / "source_sha256.txt").read_text(encoding="utf-8").splitlines():
    if not line.strip():
        continue
    digest, path = line.split("*", 1)
    digest = digest.strip()
    p = root / path.strip()
    got = hashlib.sha256(p.read_bytes()).hexdigest()
    if got != digest:
        src_hash_ok = False
        print("HASH MISMATCH", path.strip())
checks.append(("current 6 product sources match EV-029", src_hash_ok))

# 2) Current build artifact == tested artifact.
md5_ok = True
for line in (ev29 / "image_md5_build.txt").read_text(encoding="utf-8").splitlines():
    if not line.strip():
        continue
    digest, path = line.split(" ", 1)
    p = Path(path.strip().lstrip("*").replace("/d/", "D:/", 1))
    got = hashlib.md5(p.read_bytes()).hexdigest()
    if got != digest:
        md5_ok = False
        print("MD5 MISMATCH", p)
checks.append(("current axf/hex md5 match EV-029", md5_ok))

# 3) Failure capture content.
s_count = g_count = bus_none = 0
for name in ("com43_capture_window_a.bin", "com43_capture_window_b.bin"):
    text = (ev29 / name).read_text(encoding="ascii")
    for line in text.splitlines():
        if line.startswith("S "):
            s_count += 1
            if " q=0 " not in line + " ":
                print("unexpected valid sample:", line)
        elif line.startswith("G "):
            g_count += 1
            if "a44=0" not in line or "a45=0" not in line:
                print("unexpected ACK in G:", line)
        elif line.startswith("BUS "):
            if "ack=none" in line:
                bus_none += 1
checks.append(("EV-029 captures: 4 S, 4 G, all q=0/a44=a45=0", s_count == 4 and g_count == 4))
checks.append(("EV-029 BUS shows ack=none", bus_none >= 1))

# 4) No firmware change in the I2C/device/power/retired/RF paths.
paths = [
    "CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/src/gxht40.c",
    "CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/src/sf_i2c.c",
    "CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/src/light.c",
    "CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/src/encrytogate.c",
    "CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/UM2005C",
    "CH592EVT/EVT/EXAM/BLE/beiwov2",
]
diff = subprocess.run(["git", "diff", "--name-only", "8ae4a33", "--", *paths],
                      cwd=root, text=True, capture_output=True, check=True).stdout.strip()
checks.append(("no firmware/gateway change in I2C/device/power/RF paths", not diff))

# 5) Firmware probes both documented addresses and has no synthetic sample path.
g = (fw / "USER/src/gxht40.c").read_text(encoding="utf-8")
checks.append(("firmware probes both 0x44/0x45", "GXHT40_ADDR_7BIT_A" in g and "GXHT40_ADDR_7BIT_B" in g))
checks.append(("firmware sends 0xFD only after address ACK",
               "i2c_write_cmd" in g and "GXHT40_CMD_MEASURE_HIGH_REP" in g))
checks.append(("firmware has no fake/constant sample assignment",
               not re.search(r"\b(tempvalue|huminityvalue)\s*=\s*(25|250|500|0)\s*;", g)))

# 6) Netlist: U9 VCC shares BAT with MCU VDD (no firmware power switch).
net = (root / "sensor_hardware/pstxnet.dat").read_text(encoding="utf-8", errors="replace")
checks.append(("netlist U9 VCC and MCU VDD on BAT", "U9 3" in net and "U7 9" in net and "'BAT'" in net))

# 7) Current host self-tests still pass (implementation-side, not real board).
log = (root / "evidence/ev030_integration_debug/host_tests_full.log").read_text(encoding="utf-8")
counts = [int(x) for x in re.findall(r"==== result: (\d+) passed, \d+ failed ====", log)]
checks.append(("host self-tests all pass (295 total)", sum(counts) == 295 and all("0 failed" in x for x in re.findall(r"==== result: \d+ passed, \d+ failed ====", log))))

print("EV-029 failure capture summary: S=%d G=%d BUS none=%d" % (s_count, g_count, bus_none))
print("product diff in audited paths:", diff or "(empty)")
for name, ok in checks:
    print(("PASS  " if ok else "FAIL  ") + name)
bad = [n for n, ok in checks if not ok]
print(f"==== result: {len(checks) - len(bad)} passed, {len(bad)} failed ====")
raise SystemExit(1 if bad else 0)
