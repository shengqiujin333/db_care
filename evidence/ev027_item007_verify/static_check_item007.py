#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Independent static checks for ITEM-007 (implementation-side self-check assets).

Checks that the three named assets carry the current rise/14-vector semantics,
that the old drop/35.0-exclusion semantics survive only as an explicit negative
control, and that the required behavior categories are actually asserted.
Read-only: no product or test source is modified.
"""
import os
import re
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
PROJ = os.path.join(ROOT, "CW32L010_StandardPeripheralLib_V1.0.5", "Examples",
                    "sensor", "gpio_input_output")
TEST = os.path.join(PROJ, "test")
SRC = os.path.join(PROJ, "USER", "src")
INC = os.path.join(PROJ, "USER", "inc")

p = 0
f = 0
warn = []

_LOG_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "static_check.out")
_LOG = open(_LOG_PATH, "w", encoding="utf-8")


class _Tee(object):
    def write(self, s):
        _LOG.write(s)
        sys.__stdout__.write(s)

    def flush(self):
        _LOG.flush()
        sys.__stdout__.flush()


sys.stdout = _Tee()


def ok(cond, msg):
    global p, f
    if cond:
        p += 1
        print("  PASS  " + msg)
    else:
        f += 1
        print("  FAIL  " + msg)


def read(path):
    with open(path, "r", encoding="utf-8", errors="replace") as fh:
        return fh.read()


EXPECTED_VEC = [
    (1, 1, 300, 308, 1, 1, 0), (1, 1, 300, 309, 1, 1, 0),
    (1, 1, 300, 310, 1, 1, 1), (1, 1, 300, 310, 1, 0, 0),
    (1, 1, 300, 310, 0, 0, 0), (1, 1, 310, 300, 1, 1, 0),
    (1, 1, 340, 350, 1, 1, 1), (1, 1, 340, 350, 1, 0, 0),
    (1, 0, 0, 351, 0, 0, 1), (1, 0, 0, 350, 1, 1, 0),
    (1, 0, 0, 360, 0, 0, 1), (1, 1, 352, 351, 0, 0, 1),
    (1, 1, 300, 310, 0, 0, 0), (0, 0, 0, 0, 0, 0, 0),
]
vec_re = re.compile(r"\{\s*([01])\s*,\s*([01])\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,"
                    r"\s*([01])\s*,\s*([01])\s*,\s*([01])\s*\}")


def vectors_in(text):
    return [tuple(int(x) for x in m.groups()) for m in vec_re.finditer(text)]


print("[A] 14-vector table identity (implementation self-checks vs frozen EV-020 table)")
for name in ("host_sensor_core_test.c", "host_fw_core_pure_check.c"):
    vs = vectors_in(read(os.path.join(TEST, name)))
    ok(vs[:14] == EXPECTED_VEC and len(vs) >= 14,
       "%s: first 14 vectors exactly match frozen table (%d found)" % (name, len(vs)))
    ok(vs[2] == (1, 1, 300, 310, 1, 1, 1), "%s: v3 rise +1.0C dark -> report" % name)
    ok(vs[5] == (1, 1, 310, 300, 1, 1, 0), "%s: v6 drop -> no report" % name)
    ok(vs[6] == (1, 1, 340, 350, 1, 1, 1),
       "%s: v7 exact 35.0C continues rise evaluation -> report" % name)
    ok(vs[7] == (1, 1, 340, 350, 1, 0, 0), "%s: v8 exact 35.0C but lit -> no report" % name)
    ok(vs[13] == (0, 0, 0, 0, 0, 0, 0), "%s: v14 sample_valid=0 -> no decision" % name)

print("[B] legacy drop / 35.0-exclusion semantics only as explicit negative control")
core = read(os.path.join(TEST, "host_sensor_core_test.c"))
ok("static bool legacy_drop" in core, "host_sensor_core_test: legacy replica is explicit and named")
ok("legacy_bad == 3u" in core, "host_sensor_core_test: replica must fail exactly 3 of 13 vectors")
ok("legacy fails v3 (rise not reported)" in core and
   "legacy fails v6 (drop reported)" in core and
   "legacy fails v7 (35.0 rise short-circuited)" in core,
   "host_sensor_core_test: v3/v6/v7 discriminations asserted")
for name in ("host_fw_core_pure_check.c", "host_measure_flow_check.c"):
    t = read(os.path.join(TEST, name))
    ok("prev_x10 - (int32_t)cur_x10" not in t and "cur_x10 == 350) return false" not in t,
       "%s: no legacy drop/35.0-exclusion expectation" % name)
ok("prev_x10 - (int32_t)cur_x10" not in core.split("static bool legacy_drop")[0],
   "host_sensor_core_test: legacy drop absent outside the negative-control replica")
ok("cur_x10 == 350) return false" not in core.split("static bool legacy_drop")[0],
   "host_sensor_core_test: 35.0 exclusion absent outside the negative-control replica")

print("[C] required behavior categories are asserted")
mf = read(os.path.join(TEST, "host_measure_flow_check.c"))
ok("[14] ITEM-002" in mf, "measure flow: section [14] pre-decision snapshot present")
ok("A 周期 p/H 为判定前值 (160/1)" in mf,
   "measure flow: pre-decision prev asserted (A p=160 while t=170)")
ok("失败周期 p/H 保持失败前值 (170/1)" in mf,
   "measure flow: failure does not advance prev")
ok("3 次尝试逐字节同一冻结帧" in mf, "measure flow: frozen frame byte-identical across attempts")
ok("3 次发送不重新编码" in mf, "measure flow: no re-encode during retries")
ok("用尽后清除待发状态" in mf and "下一周期无残留, 不再发送旧帧" in mf,
   "measure flow: exhaustion clears state, no cross-cycle retry")
ok('tr.send == 2u && tr.retry == 0u' in mf, "measure flow: S.s=2 / S.y=0 on exhaustion")
ok("未触发 -> 不编码不发送" in mf, "measure flow: no pending -> no send")

dt = read(os.path.join(TEST, "host_debug_trace_check.c"))
ok("T14" in dt and "TXE 永不置位" in dt and "DEBUG_UART_LINE_GUARD" in dt,
   "UART bounded: TXE stuck aborts within line guard")
ok("T15" in dt and "DEBUG_UART_CLOSE_GUARD" in dt and "超时后仍执行 UART1 复位/关时钟" in dt,
   "UART bounded: TXBUSY stuck close within guard and teardown still runs")

lt = read(os.path.join(TEST, "host_light_check.c"))
ok("uncalibrated: adc_ok=true but valid=false" in lt,
   "light: uncalibrated cannot claim valid")
ok("uncalibrated: dark=false even at full-scale reading" in lt,
   "light: uncalibrated cannot claim dark")
ok("mean=1000" in lt and "999" in lt,
   "light: calibrated ceil(C_dark/3) boundary 1000/999")

bt = read(os.path.join(TEST, "build_test.sh"))
ok(bt.count("== [") >= 7 and "host_fw_core_pure_check.c" in bt and
   "host_sensor_core_test.c" in bt and "host_measure_flow_check.c" in bt and
   "host_debug_trace_check.c" in bt and "host_light_check.c" in bt and
   "check_calib_config.sh" in bt,
   "build_test.sh: single entry runs all 7 groups including pure check and calib gate")

print("[D] product sources carry the current semantics the assets assert")
fwc = read(os.path.join(SRC, "fw_core.c"))
ok("(int32_t)cur_temp_x10 - (int32_t)prev_temp_x10" in fwc,
   "fw_core.c: rise comparison cur-prev")
ok("> (int32_t)SENSOR_REPORT_RISE_X10" in fwc, "fw_core.c: uses RISE constant")
ok("cur_temp_x10 == SENSOR_REPORT_HIGH_X10" not in fwc,
   "fw_core.c: no exact-35.0 exclusion")
msr = read(os.path.join(SRC, "measure.c"))
ok("prev_for_decision       = s_prev_temp_x10;" in msr,
   "measure.c: pre-decision snapshot")
ok("s_trace_prev_temp_x10 = prev_for_decision;" in msr,
   "measure.c: trace uses pre-decision snapshot")
ok("app_um2005C_send_data_timeout(send_data, SENSOR_RF_FRAME_LEN" in msr and
   "s_pending_frame_valid" in msr,
   "measure.c: retries send the frozen 10B buffer")

print("[E] commit under test did not modify tester-owned *_verify_ev.c")
changed = subprocess.run(["git", "show", "--name-only", "--format=", "baf9d0f"],
                         cwd=ROOT, stdout=subprocess.PIPE).stdout.decode("utf-8", "replace")
ok("_verify_ev.c" not in changed, "commit baf9d0f changed no *_verify_ev.c")
ok("test/build_test.sh" in changed and "test/host_sensor_core_test.c" in changed,
   "commit baf9d0f did change the declared assets")
ok("USER/src/" not in changed, "commit baf9d0f changed no product source")

# Documentation/coverage-label discrepancy observed during review (not a gate failure).
if "成功 A -> 成功 B -> 失败 -> 成功 C" in mf:
    warn.append("host_measure_flow_check.c [14] label says A->B->failure->C, but the "
                "executed sequence is A->failure->C (no B cycle); driver_test.md's "
                "'B p=A / C p=B' wording is not backed by the executed checks. "
                "Pre-decision snapshot coverage itself is present (A p=160, t=170).")

print("\n==== static result: %d passed, %d failed, %d warning ====" % (p, f, len(warn)))
for w in warn:
    print("WARN: " + w)
sys.exit(0 if f == 0 else 1)
