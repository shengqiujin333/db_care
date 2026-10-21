#!/usr/bin/env python3
"""
Independent reference model for ITEM-003 (frozen 10-byte RF frame).

Reads ONLY the raw per-cycle output of host_measure_freeze_verify_item003.c and
re-derives every expectation in Python from the printed stimulus. It does not
call, link or import the product code.

Checks (per cycle):
  * gate decision reproduced independently:
        report = sample_ok and (t > 350 or (have_prev and light_valid and dark and t-prev > 9))
    (strictly greater; int arithmetic; prev advanced only on a valid sample)
  * encode_frame10 called exactly once iff the cycle triggers
  * one send_data_to_gateway() call performs attempts until the first scripted
    success, capped at 3, and stops there
  * every attempt carries len=10, timeout=200 ms
  * all attempts carry the SAME 10 bytes, equal to encode(uid, trigger t, trigger h)
    printed by the harness as REF (this is the "no re-encode / no re-sample" check,
    including the cycle whose stimulus mutates tempvalue/huminityvalue mid-retry)
  * S-trace s is the final local result (0 = not sent / no trigger, 1 = success,
    2 = exhausted) and y (retry counter) is 0 in every cycle
  * report_req is clear after the cycle and a further send call reaches the stub
    0 times (no cross-cycle pending state)

Usage:
  python verify_measure_freeze_item003.py harness_current.out
  python verify_measure_freeze_item003.py harness_old.out --expect-fail
"""
import argparse
import re
import sys

LINE_SEQ = re.compile(r"^SEQ cyc=(\d+) st=(-?\d+) t=(-?\d+) h=(\d+) dark=(\d+) lv=(\d+) seq=(\d+) mutate=(\d+)$")
LINE_C = re.compile(
    r"^C cyc=(\d+) enc=(\d+) sends=(\d+) p=(-?\d+) H=(\d+) t=(-?\d+) h=(\d+) "
    r"q=(\d+) r=(\d+) s=(\d+) y=(\d+) req=(\d+) diag=(\d+) fetched=(\d+)$")
LINE_A = re.compile(r"^A cyc=(\d+) call=(\d+) ret=(\d+) len=(\d+) to=(\d+) frame=([0-9A-F]+)$")
LINE_REF = re.compile(r"^REF cyc=(\d+) t=(-?\d+) h=(\d+) frame=([0-9A-F]+)$")
LINE_AFTER = re.compile(r"^AFTER cyc=(\d+) extra_sends=(\d+)$")
LINE_FETCH = re.compile(r"^FETCH_AGAIN ret=(\d+) first_byte_dec=(\d+)$")

SENSOR_REPORT_HIGH_X10 = 350   # SENSOR_REPORT_HIGH_X10
SENSOR_REPORT_RISE_X10 = 9     # SENSOR_REPORT_RISE_X10
RF_FRAME_LEN = 10              # SENSOR_RF_FRAME_LEN
RF_TX_TIMEOUT_MS = 200         # SENSOR_RF_TX_TIMEOUT_MS
RF_TX_RETRY = 3                # SENSOR_RF_TX_RETRY


class Checker:
    def __init__(self):
        self.passed = 0
        self.failed = 0
        self.failures = []

    def check(self, cond, msg):
        if cond:
            self.passed += 1
        else:
            self.failed += 1
            self.failures.append(msg)


def parse(path):
    seq, cyc, attempts, refs, after, fetch = {}, {}, {}, {}, {}, None
    with open(path, "r", encoding="ascii", errors="replace") as fh:
        for raw in fh:
            line = raw.strip()
            m = LINE_SEQ.match(line)
            if m:
                i = int(m.group(1))
                seq[i] = dict(st=int(m.group(2)), t=int(m.group(3)), h=int(m.group(4)),
                              dark=int(m.group(5)), lv=int(m.group(6)),
                              rsp=[int(c) for c in m.group(7)],
                              mutate=int(m.group(8)))
                continue
            m = LINE_C.match(line)
            if m:
                g = [int(x) for x in m.groups()]
                cyc[g[0]] = dict(enc=g[1], sends=g[2], p=g[3], H=g[4], t=g[5], h=g[6],
                                 q=g[7], r=g[8], s=g[9], y=g[10], req=g[11],
                                 diag=g[12], fetched=g[13])
                continue
            m = LINE_A.match(line)
            if m:
                g = m.groups()
                attempts.setdefault(int(g[0]), []).append(
                    dict(call=int(g[1]), ret=int(g[2]), len=int(g[3]),
                         to=int(g[4]), frame=g[5]))
                continue
            m = LINE_REF.match(line)
            if m:
                g = m.groups()
                refs[int(g[0])] = dict(t=int(g[1]), h=int(g[2]), frame=g[3])
                continue
            m = LINE_AFTER.match(line)
            if m:
                after[int(m.group(1))] = int(m.group(2))
                continue
            m = LINE_FETCH.match(line)
            if m:
                fetch = dict(ret=int(m.group(1)), first=int(m.group(2)))
    return seq, cyc, attempts, refs, after, fetch


def expected_attempts(rsp):
    """attempts actually performed: stop at first success, cap at RF_TX_RETRY"""
    for k in range(RF_TX_RETRY):
        if rsp[k] == 1:
            return k + 1, True
    return RF_TX_RETRY, False


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("outfile")
    ap.add_argument("--expect-fail", action="store_true",
                    help="negative control: succeed only if the output is rejected")
    args = ap.parse_args()

    seq, cyc, attempts, refs, after, fetch = parse(args.outfile)
    ck = Checker()

    ck.check(len(seq) > 0, "no SEQ lines parsed")
    ck.check(set(seq) == set(cyc), "SEQ/C cycle sets differ: %s vs %s" % (sorted(seq), sorted(cyc)))

    prev = 0
    have_prev = False
    trigger_cycles = []

    for i in sorted(seq):
        s = seq[i]
        c = cyc.get(i)
        if c is None:
            ck.check(False, "cycle %d has no C line" % i)
            continue

        sample_ok = (s["st"] == 0)
        if sample_ok:
            rise = s["t"] - prev
            report = (s["t"] > SENSOR_REPORT_HIGH_X10) or \
                     (have_prev and s["lv"] == 1 and s["dark"] == 1 and rise > SENSOR_REPORT_RISE_X10)
            exp_p, exp_H = prev, 1 if have_prev else 0
        else:
            report = False
            exp_p, exp_H = prev, 1 if have_prev else 0

        # --- gate/trace reproduction ---
        ck.check(c["q"] == (1 if sample_ok else 0),
                 "cyc %d: q=%d expected %d" % (i, c["q"], 1 if sample_ok else 0))
        ck.check(c["r"] == (1 if report else 0),
                 "cyc %d: r=%d expected %d (t=%d p=%d H=%d lv=%d dark=%d)" %
                 (i, c["r"], 1 if report else 0, s["t"], prev, 1 if have_prev else 0, s["lv"], s["dark"]))
        ck.check(c["p"] == exp_p, "cyc %d: p=%d expected pre-decision prev %d" % (i, c["p"], exp_p))
        ck.check(c["H"] == exp_H, "cyc %d: H=%d expected %d" % (i, c["H"], exp_H))
        ck.check(c["fetched"] == 1, "cyc %d: trace not fetched" % i)

        # --- ITEM-003 core ---
        exp_enc = 1 if report else 0
        if report:
            n_att, ok = expected_attempts(s["rsp"])
            exp_s = 1 if ok else 2
        else:
            n_att, exp_s = 0, 0

        ck.check(c["enc"] == exp_enc,
                 "cyc %d: encode_frame10 called %d times, expected %d" % (i, c["enc"], exp_enc))
        ck.check(c["sends"] == n_att,
                 "cyc %d: %d send attempts, expected %d" % (i, c["sends"], n_att))
        ck.check(c["s"] == exp_s,
                 "cyc %d: S.s=%d, expected final result %d" % (i, c["s"], exp_s))
        ck.check(c["y"] == 0, "cyc %d: S.y=%d, expected 0" % (i, c["y"]))
        ck.check(c["req"] == 0, "cyc %d: report_req=%d after the cycle, expected 0" % (i, c["req"]))
        ck.check(after.get(i) == 0,
                 "cyc %d: extra send after the cycle reached the stub %s times, expected 0 (pending not cleared)"
                 % (i, after.get(i)))

        alist = attempts.get(i, [])
        ck.check(len(alist) == n_att, "cyc %d: %d A lines, expected %d" % (i, len(alist), n_att))
        if report:
            trigger_cycles.append(i)
        for k, a in enumerate(alist):
            ck.check(a["call"] == k, "cyc %d attempt %d: call index %d" % (i, k, a["call"]))
            ck.check(a["len"] == RF_FRAME_LEN,
                     "cyc %d attempt %d: len=%d expected %d" % (i, k, a["len"], RF_FRAME_LEN))
            ck.check(a["to"] == RF_TX_TIMEOUT_MS,
                     "cyc %d attempt %d: timeout=%d expected %d" % (i, k, a["to"], RF_TX_TIMEOUT_MS))
            ck.check(a["ret"] == s["rsp"][k],
                     "cyc %d attempt %d: stub returned %d" % (i, k, a["ret"]))
            if k > 0:
                ck.check(a["frame"] == alist[0]["frame"],
                         "cyc %d attempt %d: bytes differ from attempt 0 (re-encode/re-sample)" % (i, k))
            ref = refs.get(i)
            ck.check(ref is not None, "cyc %d: no REF line" % i)
            if ref is not None:
                ck.check(ref["t"] == s["t"] and ref["h"] == s["h"],
                         "cyc %d: REF stimulus mismatch" % i)
                ck.check(a["frame"] == ref["frame"],
                         "cyc %d attempt %d: frame != encode(uid, trigger %d/%d)"
                         % (i, k, s["t"], s["h"]))

        if sample_ok:
            prev = s["t"]
            have_prev = True

    # trace fetch is consumed exactly once per cycle
    ck.check(fetch is not None and fetch["ret"] == 0 and fetch["first"] == 0x5A,
             "FETCH_AGAIN: expected ret=0 and untouched buffer (first_byte_dec=90)")

    # the stimulus must actually exercise trigger and non-trigger cycles
    ck.check(len(trigger_cycles) >= 6, "stimulus covered only %d trigger cycles" % len(trigger_cycles))
    ck.check(any(seq[i]["mutate"] == 1 for i in trigger_cycles),
             "no trigger cycle mutated temp/hum during retry (freeze not exercised)")

    print("FILE %s" % args.outfile)
    print("cycles=%d trigger_cycles=%d" % (len(seq), len(trigger_cycles)))
    for f in ck.failures:
        print("FAIL %s" % f)
    print("SUMMARY passed=%d failed=%d" % (ck.passed, ck.failed))

    if args.expect_fail:
        if ck.failed > 0:
            print("NEGATIVE_CONTROL_OK (implementation rejected on %d checks)" % ck.failed)
            return 0
        print("NEGATIVE_CONTROL_INVALID (old implementation passed; the test does not discriminate)")
        return 1

    if ck.failed == 0:
        print("RESULT PASS")
        return 0
    print("RESULT FAIL")
    return 1


if __name__ == "__main__":
    sys.exit(main())
