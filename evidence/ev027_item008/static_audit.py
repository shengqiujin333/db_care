#!/usr/bin/env python3
"""ITEM-008 static audit: retired paths, NVM/RF/pins/heartbeat, S/G budget."""
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[2]
fw = root / "CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output"
src = sorted((fw / "USER").rglob("*.c")) + sorted((fw / "USER").rglob("*.h"))


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    text = re.sub(r"//[^\n]*", " ", text)
    return text


code = "\n".join(strip_comments(p.read_text(encoding="utf-8", errors="replace")) for p in src)

checks = []
checks.append(("no effective hall/OPTCFG/params/history call",
               not re.search(r"\b(hall|OPTCFG|params|history)\b", code, re.I)))
checks.append(("no NVM/Flash parameter write",
               not re.search(r"\b(FLASH_(Write|Program|Erase)|FLASH_SetReadOutLevel|NVM)\b", code)))
checks.append(("sample_flag=1 only initial/RTC",
               len(re.findall(r"sample_flag\s*=\s*1\s*;", code)) == 2))
checks.append(("temperature_process called once in USER",
               len(re.findall(r"temperature_process\s*\(\s*\)\s*;", code)) == 1))
checks.append(("send_data_to_gateway called once in USER",
               len(re.findall(r"send_data_to_gateway\s*\(\s*\)\s*;", code)) == 1))
checks.append(("no GPIOA reset in USER code",
               "SYSCTRL_AHBPeriphReset" not in code))
checks.append(("no GPIOA clock disable in USER code",
               not re.search(r"SYSCTRL_AHBPeriphClk_Enable\([^;\n]*DISABLE", code)))

u0 = subprocess.run(["git", "diff", "--unified=0", "8ae4a33", "--",
                     "CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER"],
                    cwd=root, text=True, capture_output=True, check=True).stdout
added = [ln[1:] for ln in u0.splitlines() if ln.startswith("+") and not ln.startswith("+++")]
checks.append(("no added GPIO/NVIC/RTC configuration lines",
               not any(re.search(r"GPIO_(Init|WritePin|ReadPin)|NVIC_|RTC_(Init|SetInterval|Cmd|ITConfig)", ln) for ln in added)))

# Product-source diff against the pre-change firmware baseline.
paths = [
    "CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER",
    "CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/UM2005C",
    "CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/inc/encrytogate.h",
    "CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/src/encrytogate.c",
    "CH592EVT/EVT/EXAM/BLE/beiwov2",
]
diff = subprocess.run(["git", "diff", "--name-only", "8ae4a33", "--", *paths],
                      cwd=root, text=True, capture_output=True, check=True).stdout
changed = [line for line in diff.splitlines() if line.strip()]
checks.append(("no gateway firmware change", not any("CH592EVT" in x for x in changed)))
checks.append(("no RF/encrytogate change",
               not any(("encrytogate" in x or "UM2005C" in x) for x in changed)))
checks.append(("no light/gxht40/sf_i2c behavior change",
               not any(("light.c" in x or "gxht40.c" in x or "sf_i2c.c" in x) for x in changed)))

# S/G worst line lengths from the implementation-side debug trace log.
log = fw / "test/_hostbin"
trace_log = root / "evidence/ev027_item008/host_tests_full.log"
m = re.search(r"len\(worst S\)=(\d+) bytes, len\(worst G\)=(\d+) bytes", trace_log.read_text(encoding="utf-8"))
checks.append(("S/G worst <= 96 B", bool(m) and int(m.group(1)) <= 96 and int(m.group(2)) <= 96))

print("changed product-source files vs 8ae4a33:")
for line in changed:
    print("  " + line)
print()
for name, ok in checks:
    print(("PASS  " if ok else "FAIL  ") + name)
bad = [n for n, ok in checks if not ok]
print(f"==== result: {len(checks) - len(bad)} passed, {len(bad)} failed ====")
raise SystemExit(1 if bad else 0)
