#!/usr/bin/env python3
"""Independent ITEM-008 static audit (embedded_tester.embedded_verification).

Checks the current working tree / HEAD against the pre-change firmware baseline
8ae4a33. This script is maintained by the verification capability; it does not
reuse the implementer's evidence/ev027_item008/static_audit.py.

Scope of this item: final Keil build identity + static boundaries:
  * no effective hall/OPTCFG/params/history call
  * no NVM/Flash parameter write
  * no new RF/BLE/upload field
  * no heartbeat / extra sampling
  * pin and interrupt boundaries unchanged
  * S/G worst line <= 96 B (fed by host_diag_line harness output)
"""
from pathlib import Path
import hashlib
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
FW = ROOT / "CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output"
USER = FW / "USER"
BASE = "8ae4a33"
EV = ROOT / "evidence/ev028_item008_verify"

results = []


def check(name: str, ok: bool, detail: str = "") -> None:
    results.append((name, bool(ok), detail))
    print(f"{'PASS' if ok else 'FAIL'}  {name}" + (f"  [{detail}]" if detail else ""))


def git(*args: str) -> str:
    return subprocess.run(["git", *args], cwd=ROOT, text=True,
                          capture_output=True, check=True).stdout


def strip_c_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    text = re.sub(r"//[^\n]*", " ", text)
    return text


def user_files():
    return sorted(USER.rglob("*.c")) + sorted(USER.rglob("*.h"))


def code_text() -> str:
    return "\n".join(strip_c_comments(p.read_text(encoding="utf-8", errors="replace"))
                     for p in user_files())


# ---------------------------------------------------------------- 0. identity
head = git("rev-parse", "HEAD").strip()
check("git HEAD recorded (working tree is the tested identity)",
      head.startswith("c176d02"), head[:12])

changed = [ln for ln in git("diff", "--name-only", BASE, "--",
                            "CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER")
           .splitlines() if ln.strip()]
expected = {
    "USER/inc/fw_core.h",
    "USER/inc/sensor_config.h",
    "USER/src/debug_trace.c",
    "USER/src/fw_core.c",
    "USER/src/main.c",
    "USER/src/measure.c",
}
got = {p.split("/USER/", 1)[1] and "USER/" + p.split("/USER/", 1)[1] for p in changed}
check("product source change set is exactly the 6 reviewed files", got == expected,
      ", ".join(sorted(got)))

all_scope = git("diff", "--name-only", BASE, "--",
                "CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output",
                "CH592EVT/EVT/EXAM/BLE/beiwov2").splitlines()
non_user = [p for p in all_scope if p.strip() and "/USER/" not in p]
check("only test assets changed outside USER/ (no RF/gateway/light/gxht40/sf_i2c/encrytogate)",
      all(("/test/" in p) for p in non_user), ", ".join(non_user) or "none")

code = code_text()

# ------------------------------------- 1. retired hall/OPTCFG/params/history
retired = sorted({m.group(0) for m in
                  re.finditer(r"\b(hall|OPTCFG|params|history)\b", code, re.I)})
check("no effective hall/OPTCFG/params/history call (comments stripped)", not retired,
      ", ".join(retired) or "0 tokens")

# ------------------------------------------------ 2. NVM / Flash parameter write
nvm_hits = re.findall(r"\b(FLASH_(?:Write|Program|Erase)\w*|FLASH_SetReadOutLevel|NVM\w*)\b", code)
check("no NVM/Flash parameter write in USER code", not nvm_hits,
      ", ".join(sorted(set(nvm_hits))) or "0 calls")

# ------------------------------------------------- 3. RF/BLE/upload new fields
rf_len_base = re.search(r"#define\s+SENSOR_RF_FRAME_LEN\s+(\S+)",
                        git("show", f"{BASE}:CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/"
                            "gpio_input_output/USER/inc/sensor_config.h")).group(1)
rf_len_now = re.search(r"#define\s+SENSOR_RF_FRAME_LEN\s+(\S+)",
                       (USER / "inc/sensor_config.h").read_text(encoding="utf-8")).group(1)
check("RF frame length unchanged (10 B) and no new RF field",
      rf_len_base.strip() == rf_len_now.strip() and rf_len_now.strip().startswith("10"),
      f"{rf_len_base.strip()} -> {rf_len_now.strip()}")

added = [ln[1:] for ln in git("diff", "--unified=0", BASE, "--",
                              "CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER")
         .splitlines() if ln.startswith("+") and not ln.startswith("+++")]
added_code = strip_c_comments("\n".join(added))
# New RF/BLE/upload *fields* would show up as new defines/offsets or new API use.
# The re-indented existing call app_um2005C_send_data_timeout(...) is not a field.
forbidden = re.findall(r"\b(?:BLE|MQTT|GATT|upload_data|/upload|HTTP|https?://)\b"
                       r"|encrytogate|\b(?:host|port|device_id|server)\s*=", added_code, re.I)
new_rf_define = re.findall(r"#define\s+(?:RF|BLE|UPLOAD)\w*", added_code, re.I)
send_data_decl = re.search(r"uint8_t\s+send_data\s*\[\s*(\d+)\s*\]", code)
check("added source lines introduce no RF/BLE/upload field",
      not forbidden and not new_rf_define
      and send_data_decl is not None and send_data_decl.group(1) == "10",
      ", ".join(sorted(set(forbidden + new_rf_define))) or "none")

# ------------------------------------------------ 4. heartbeat / extra sampling
sf_one = re.findall(r"sample_flag\s*=\s*1\s*;", code)
check("sample_flag set to 1 only at init and RTC tick", len(sf_one) == 2, f"{len(sf_one)} sites")

tp_calls = re.findall(r"\btemperature_process\s*\(\s*\)\s*;", code)
sd_calls = re.findall(r"\bsend_data_to_gateway\s*\(\s*\)\s*;", code)
check("temperature_process called exactly once in USER", len(tp_calls) == 1, f"{len(tp_calls)} calls")
check("send_data_to_gateway called exactly once in USER", len(sd_calls) == 1, f"{len(sd_calls)} calls")

main_txt = (USER / "src/main.c").read_text(encoding="utf-8")
no_new_timer = not re.search(r"\b(?:LPTIM|GTIM1?|ATIM|BTIM\d*|TIM\d*)_IRQHandler"
                             r"|NVIC_EnableIRQ\s*\(\s*(?:LPTIM|GTIM|ATIM|BTIM|TIM)",
                             added_code)
check("sample cadence unchanged: RTC 1 min x 3 ticks, no second timer/callback",
      "RTC_INTERVAL_EVERY_1M" in main_txt
      and re.search(r"#define\s+SENSOR_SAMPLE_TICKS\s+3u", (USER / "inc/sensor_config.h").read_text(encoding="utf-8")) is not None
      and no_new_timer,
      "3 min")

# no extra sampling work in the main loop: only one guarded transaction block
guarded = re.findall(r"if\s*\(\s*sample_flag\s*!=\s*0u\s*\)\s*\{", main_txt)
check("main loop performs at most one transaction per sample opportunity",
      len(guarded) == 1, f"{len(guarded)} guarded block(s)")

# ------------------------------------- 5. pin / interrupt boundaries unchanged
def defines(text: str) -> dict:
    text = strip_c_comments(text)
    out = {}
    for m in re.finditer(r"^[ \t]*#define[ \t]+([A-Za-z_]\w*)[ \t]+(.+)$", text, re.M):
        out[m.group(1)] = " ".join(m.group(2).split())
    return out

cfg_base = defines(git("show", f"{BASE}:CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/"
                       "gpio_input_output/USER/inc/sensor_config.h"))
cfg_now = defines((USER / "inc/sensor_config.h").read_text(encoding="utf-8"))
pin_names = [n for n in cfg_base
             if re.search(r"(?:^|_)(?:GPIO|PIN|PORT|CHANNEL|AFX|AFRX|UARTx?|CLK)(?:_|$)", n)]
pin_changed = [n for n in pin_names if cfg_base.get(n) != cfg_now.get(n)]
check("all pin/peripheral macros keep their baseline values", not pin_changed,
      ", ".join(pin_changed) or f"{len(pin_names)} macros identical")

removed = [n for n in cfg_base if n not in cfg_now]
renamed_ok = removed == ["SENSOR_REPORT_DROP_X10"]
check("only the renamed gate constant was removed from config", renamed_ok,
      ", ".join(removed) or "none")

intr_diff = git("diff", "--name-only", BASE, "--",
                "CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/src/interrupts_cw32l010.c")
check("interrupt service implementation file unchanged", not intr_diff.strip(), intr_diff.strip() or "clean")

cfg_lines = re.findall(r"GPIO_(?:Init|WritePin|ReadPin)|NVIC_|RTC_(?:Init|SetInterval|Cmd|ITConfig)",
                       added_code)
check("added lines contain no new GPIO/NVIC/RTC configuration", not cfg_lines,
      ", ".join(sorted(set(cfg_lines))) or "0 hits")

check("no new IRQ handler added", not re.search(r"\w+_IRQHandler", added_code), "0 handlers")

# --------------------------------------------------- 6. build log consistency
build_log = (EV / "build_rebuild.log").read_text(encoding="utf-8", errors="replace")
comp = re.search(r"Using Compiler '([^']+)'", build_log)
err = re.search(r"(\d+)\s+Error\(s\),\s+(\d+)\s+Warning\(s\)", build_log)
size = re.search(r"Code=(\d+)\s+RO-data=(\d+)\s+RW-data=(\d+)\s+ZI-data=(\d+)", build_log)
warn_sites = re.findall(r"([^\s:]+\.c)\((\d+)\): warning:", build_log)
check("build log: compiler V6.24", bool(comp) and comp.group(1) == "V6.24",
      comp.group(1) if comp else "missing")
check("build log: 0 errors / 1 warning", bool(err) and err.group(1) == "0" and err.group(2) == "1",
      f"{err.group(1)}E/{err.group(2)}W" if err else "missing")
check("sole warning is the pre-existing main.c:252 empty-body loop",
      len(warn_sites) == 1 and Path(warn_sites[0][0]).name == "main.c"
      and warn_sites[0][1] == "252", str(warn_sites))
check("build log size matches Code=19544 RO=644 RW=116 ZI=1684",
      bool(size) and size.groups() == ("19544", "644", "116", "1684"),
      "/".join(size.groups()) if size else "missing")

# -------------------------------------------------- 7. S/G line budget (harness)
trace_out = (EV / "diag_line_verify.out")
if trace_out.exists():
    trace_txt = trace_out.read_text(encoding="utf-8", errors="replace")
    s = re.search(r"worst-case S: (\d+) bytes", trace_txt)
    g = re.search(r"worst-case G line: (\d+) bytes", trace_txt)
    check("S/G worst line <= 96 B (independent harness)",
          bool(s) and bool(g) and int(s.group(1)) <= 96 and int(g.group(1)) <= 96,
          f"S={s.group(1) if s else '?'}B G={g.group(1) if g else '?'}B")
    check("S/G harness reports 0 failed", "0 failed" in trace_txt,
          re.search(r"==== result: (\d+) passed, (\d+) failed ====", trace_txt).group(0)
          if "==== result:" in trace_txt else "missing")
else:
    check("S/G worst line <= 96 B (independent harness)", False, "harness output missing")

print()
bad = [n for n, ok, _ in results if not ok]
print(f"==== result: {len(results) - len(bad)} passed, {len(bad)} failed ====")
sys.exit(1 if bad else 0)
