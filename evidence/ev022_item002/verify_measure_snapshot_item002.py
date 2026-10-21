#!/usr/bin/env python3
"""Independent reference model for ITEM-002 (measure.c pre-decision snapshot).

Reads the raw per-cycle dump produced by test/host_measure_snapshot_verify_item002.c
and re-derives the expected behaviour WITHOUT calling any product code:

  * prev/have_prev are the values that existed at the START of the transaction;
  * the report decision uses those pre-decision values and the current sample;
  * prev and last-valid humidity advance ONLY on a successful sample;
  * a failed cycle does not report, does not advance, and does not send;
  * the S-trace p/H fields are the pre-decision values (so a successful cycle may
    legitimately have p != t).

Usage: verify_measure_snapshot_item002.py <harness_output.txt> [--expect-fail]
Exit code 0 = all checks passed; 1 = at least one mismatch (or unexpected pass
when --expect-fail is used for the negative control).
"""
import re
import sys

CHECK_PASS = 0
CHECK_FAIL = 0
FAIL_DETAILS = []


def check(cond, label, detail=""):
    global CHECK_PASS, CHECK_FAIL
    if cond:
        CHECK_PASS += 1
    else:
        CHECK_FAIL += 1
        FAIL_DETAILS.append("%s %s" % (label, detail))


def kv(line):
    return {k: int(v) for k, v in re.findall(r"([A-Za-z_]+)=(-?\d+)", line)}


def main():
    path = sys.argv[1]
    expect_fail = "--expect-fail" in sys.argv[2:]
    lines = open(path, "r", encoding="utf-8", errors="replace").read().splitlines()

    prev = 0          # model previous valid temperature (x10)
    have = 0          # model have_prev
    last_t = 0        # model last valid temperature
    last_h = 0        # model last valid humidity
    cycles = 0
    success_cycles = 0
    fail_cycles = 0
    p_ne_t_success = 0
    expected_sends = 0
    frames = 0
    frame_ok = 0

    for line in lines:
        if line.startswith("C idx="):
            d = kv(line)
            cycles += 1
            idx = d["idx"]
            ok = (d["st"] == 0)
            exp_q = 1 if ok else 0
            exp_p = prev
            exp_H = have
            if ok:
                cur_t = d["stimt"]
                cur_h = d["stimh"]
                exp_r = 1 if (cur_t > 350 or
                              (exp_H == 1 and d["lv"] == 1 and d["dark"] == 1 and
                               (cur_t - exp_p) > 9)) else 0
                exp_t, exp_h = cur_t, cur_h
                prev, have, last_t, last_h = cur_t, 1, cur_t, cur_h
                success_cycles += 1
                if exp_p != exp_t:
                    p_ne_t_success += 1
            else:
                exp_r = 0
                exp_t, exp_h = last_t, last_h
                fail_cycles += 1

            tag = "cycle %d" % idx
            check(d["q"] == exp_q, tag + " q", "actual=%d expected=%d" % (d["q"], exp_q))
            check(d["p"] == exp_p, tag + " p(pre-decision)", "actual=%d expected=%d (t=%d)" % (d["p"], exp_p, exp_t))
            check(d["H"] == exp_H, tag + " H(pre-decision)", "actual=%d expected=%d" % (d["H"], exp_H))
            check(d["t"] == exp_t, tag + " t", "actual=%d expected=%d" % (d["t"], exp_t))
            check(d["h"] == exp_h, tag + " h", "actual=%d expected=%d" % (d["h"], exp_h))
            check(d["r"] == exp_r, tag + " r(gate)", "actual=%d expected=%d" % (d["r"], exp_r))
            check(d["s"] == (1 if exp_r else 0), tag + " s(send)", "actual=%d" % d["s"])
            check(d["reqb"] == 0, tag + " report_req before", "actual=%d" % d["reqb"])
            check(d["req"] == exp_r, tag + " report_req after", "actual=%d expected=%d" % (d["req"], exp_r))
            check(d["sends"] == exp_r, tag + " send calls", "actual=%d expected=%d" % (d["sends"], exp_r))
            check(d["tv"] == last_t and d["hv"] == last_h, tag + " last valid state",
                  "actual=(%d,%d) expected=(%d,%d)" % (d["tv"], d["hv"], last_t, last_h))
            check(d["diag"] == (0 if ok else 1), tag + " diag_valid", "actual=%d" % d["diag"])
            check(d["fetched"] == 1, tag + " trace fetched", "actual=%d" % d["fetched"])
            expected_sends += exp_r

        elif line.startswith("FRAME_AFTER_FAIL"):
            d = kv(line)
            frames += 1
            check(d["eq"] == 1, "frame after failed cycle %d" % d["idx"],
                  "failed-cycle frame differs from pre-failure reference")
            if d["eq"] == 1:
                frame_ok += 1

        elif line.startswith("FETCH_AGAIN"):
            d = kv(line)
            check(d["ret"] == 0, "second fetch returns 0", "actual=%d" % d["ret"])
            check(d["first_byte_dec"] == 0x5A, "second fetch does not touch out", "actual=0x%02X" % d["first_byte_dec"])

        elif line.startswith("GATE_TOUCHED_OUT"):
            d = kv(line)
            check(d["GATE_TOUCHED_OUT"] == 0, "sample_flag=0 does not touch trace out", "")

        elif line.startswith("GATE "):
            d = kv(line)
            check(d["ldelta"] == 0 and d["gdelta"] == 0, "sample_flag=0 performs no sampling",
                  "light=%d gxht=%d" % (d["ldelta"], d["gdelta"]))
            check(d["fetch"] == 0, "sample_flag=0 produces no trace", "actual=%d" % d["fetch"])

        elif line.startswith("COUNT"):
            d = kv(line)
            check(d["light_calls"] == cycles and d["gxht_calls"] == cycles,
                  "one light + one gxht attempt per transaction",
                  "light=%d gxht=%d cycles=%d" % (d["light_calls"], d["gxht_calls"], cycles))
            check(d["gxht_init"] == 1 and d["light_init"] == 1, "lazy init exactly once",
                  "gxht_init=%d light_init=%d" % (d["gxht_init"], d["light_init"]))
            check(d["send_calls"] == expected_sends + frames, "send stub calls = reported cycles + forced frames",
                  "actual=%d expected=%d" % (d["send_calls"], expected_sends + frames))

        elif line.startswith("TEMP_PTR_NULL"):
            d = kv(line)
            check(d["TEMP_PTR_NULL"] == 0, "i2c object bound", "actual=%d" % d["TEMP_PTR_NULL"])

    # aggregate expectations for this scenario
    check(cycles == 16, "cycle count", "actual=%d" % cycles)
    check(success_cycles == 10, "successful cycles exercised", "actual=%d" % success_cycles)
    check(fail_cycles == 6, "failed cycles exercised", "actual=%d" % fail_cycles)
    check(p_ne_t_success > 0, "at least one successful cycle has p != t",
          "count=%d" % p_ne_t_success)
    check(frames == 2 and frame_ok == 2, "pre-failure frame identity after failed cycles",
          "frames=%d ok=%d" % (frames, frame_ok))

    print("RESULT checks_passed=%d checks_failed=%d" % (CHECK_PASS, CHECK_FAIL))
    for f in FAIL_DETAILS:
        print("MISMATCH " + f)

    if expect_fail:
        # negative control: the old implementation must be rejected
        if CHECK_FAIL > 0 and any("p(pre-decision)" in f for f in FAIL_DETAILS):
            print("NEGATIVE_CONTROL_OK: old implementation rejected on p(pre-decision)")
            return 0
        print("NEGATIVE_CONTROL_INVALID: old implementation was not rejected on p(pre-decision)")
        return 1
    return 0 if CHECK_FAIL == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
