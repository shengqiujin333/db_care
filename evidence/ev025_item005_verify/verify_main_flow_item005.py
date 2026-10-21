#!/usr/bin/env python3
"""Independent verifier for ITEM-005 main-flow harness output.

Reads the raw harness stdout (EVENT_SEQUENCE / EVENT_COUNTS lines) and re-derives
the ITEM-005 invariants without trusting the harness's own PASS lines.

Usage: python verify_main_flow_item005.py harness_main_flow.out [--expect-old]
"""
import sys

TEMP = "TEMPERATURE_PROCESS"
SEND = "SEND_DATA_TO_GATEWAY"
FETCH = "SENSOR_TRACE_FETCH"
SLINE = "S_LINE"
CLOSE = "DEBUG_TRACE_FLUSH_CLOSE"
GOSLEEP = "GO_TO_SLEEP"
DEEP = "SYSCTRL_GotoDeepSleep"
LIGHT = "LIGHT_INIT"
BOOT = "BOOT"
POWER = ["BOOT", "VDD", "IOTEST", "IOSIG", "BUS"]


def parse(path):
    seqs = []
    counts = []
    for line in open(path, encoding="utf-8", errors="replace"):
        line = line.strip()
        if line.startswith("EVENT_SEQUENCE"):
            seqs.append(line.split()[1:])
        elif line.startswith("EVENT_COUNTS"):
            d = {}
            for tok in line.split()[1:]:
                k, v = tok.split("=")
                d[k] = int(v)
            counts.append(d)
    if len(seqs) != 2 or len(counts) != 2:
        raise SystemExit(f"FAIL: expected 2 scenario sequences, got {len(seqs)}")
    return seqs[0], counts[0], seqs[1], counts[1]


def first(seq, kind):
    return seq.index(kind) if kind in seq else -1


def main():
    path = sys.argv[1]
    expect_old = "--expect-old" in sys.argv
    uid_seq, uid_cnt, norm_seq, norm_cnt = parse(path)
    checks = []

    def chk(cond, msg):
        checks.append((bool(cond), msg))

    def run(seq, kind, start=0):
        return [i for i, x in enumerate(seq) if x == kind and i >= start]

    if not expect_old:
        # --- UID zero path ---
        boot = first(uid_seq, BOOT)
        light = first(uid_seq, LIGHT)
        close = first(uid_seq, CLOSE)
        deep = run(uid_seq, DEEP)
        chk(POWER == uid_seq[:5], "UID-zero: first five events are BOOT/VDD/IOTEST/IOSIG/BUS")
        chk(boot >= 0 and light > boot, "UID-zero: BOOT precedes light_init()")
        chk(uid_seq.count(BOOT) == 1, "UID-zero: exactly one BOOT")
        chk(uid_cnt.get(LIGHT, 0) == 1, "UID-zero: light_init() once (PB05 low / ADC off)")
        chk(uid_cnt.get(CLOSE, 0) == 1, "UID-zero: UART closed once")
        chk(uid_cnt.get(TEMP, 0) == 0 and uid_cnt.get(SEND, 0) == 0,
            "UID-zero: no sampling and no send")
        chk(uid_cnt.get(FETCH, 0) == 0 and uid_cnt.get(SLINE, 0) == 0,
            "UID-zero: no trace/S line")
        chk(uid_cnt.get(GOSLEEP, 0) == 0, "UID-zero: no go_to_sleep() spin path")
        chk(len(deep) == 5, f"UID-zero: repeated deep sleep (5 observed, got {len(deep)})")
        chk(light < close < deep[0], "UID-zero: light_init -> UART close -> deep sleep order")
        chk(all(x in (LIGHT, CLOSE, DEEP) for x in uid_seq[5:]),
            "UID-zero: only light_init/close/deep-sleep after the diagnostics (no business calls)")
        chk(len(run(uid_seq, DEEP, deep[0])) == len(deep) and deep == list(range(deep[0], deep[0] + len(deep))),
            "UID-zero: deep-sleep entries are consecutive (real sleep loop, no spin)")

        # --- normal path: one complete transaction per opportunity ---
        chk(norm_seq[:5] == POWER, "normal: power-on diagnostics first")
        chk(norm_cnt.get(TEMP, 0) == 2, f"normal: temperature_process per opportunity == 2 (got {norm_cnt.get(TEMP, 0)})")
        chk(norm_cnt.get(SEND, 0) == 2, f"normal: send call per opportunity == 2 (got {norm_cnt.get(SEND, 0)})")
        chk(norm_cnt.get(FETCH, 0) == 2, f"normal: trace fetch per opportunity == 2 (got {norm_cnt.get(FETCH, 0)})")
        chk(norm_cnt.get(SLINE, 0) == 2, f"normal: exactly one S per opportunity == 2 (got {norm_cnt.get(SLINE, 0)})")
        chk(norm_cnt.get(GOSLEEP, 0) == 9 and norm_cnt.get(DEEP, 0) == 8,
            "normal: 9 loop rounds, 8 deep sleeps (the opportunity round does not sleep)")

        s_positions = run(norm_seq, SLINE)
        t_positions = run(norm_seq, TEMP)
        e_positions = run(norm_seq, SEND)
        for n, (sp, tp, ep) in enumerate(zip(s_positions, t_positions, e_positions), 1):
            # each transaction must start with temp then send then one S
            chk(tp < ep < sp, f"normal: transaction {n} order temp < send < S")
            # nothing else from a previous transaction repeats before this S
            chk(sum(1 for i in range(tp, sp) if norm_seq[i] == TEMP) == 1,
                f"normal: transaction {n} has exactly one temperature_process")
            chk(sum(1 for i in range(tp, sp + 1) if norm_seq[i] == SEND) == 1,
                f"normal: transaction {n} has exactly one send call")
        last_s = s_positions[-1]
        chk(norm_seq[last_s:].count(TEMP) == 0 and norm_seq[last_s:].count(SEND) == 0,
            "normal: idle rounds after the last opportunity have no extra sample/send")
        chk(norm_seq.count(TEMP) == norm_seq.count(SEND) == norm_seq.count(SLINE),
            "normal: temp/send/S counts are one-to-one")
    else:
        # Parent commit must FAIL these invariants (negative control).
        chk(uid_cnt.get(BOOT, 0) == 0, "old: BOOT missing before UID-zero branch (expected defect)")
        chk(uid_cnt.get(GOSLEEP, 0) > 0, "old: UID-zero spins in go_to_sleep() (expected defect)")
        chk(uid_cnt.get(DEEP, 0) == 0, "old: UID-zero never reaches deep sleep (expected defect)")
        chk(uid_cnt.get(LIGHT, 0) == 0, "old: light_init() not called on UID-zero (expected defect)")
        chk(norm_cnt.get(TEMP, 0) > 2 and norm_cnt.get(SEND, 0) > 2,
            "old: main loop calls temperature/send every round, not once per opportunity (expected defect)")
        chk(norm_cnt.get(SLINE, 0) > 2, "old: more than one S per opportunity (expected defect)")

    failed = [m for ok, m in checks if not ok]
    for ok, m in checks:
        print(("PASS  " if ok else "FAIL  ") + m)
    if expect_old:
        print("NEGATIVE_CONTROL_OK" if not failed else f"NEGATIVE_CONTROL_UNEXPECTED {failed}")
    print(f"result: {len(checks) - len(failed)} passed, {len(failed)} failed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
