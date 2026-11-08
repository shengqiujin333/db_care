#!/usr/bin/env python3
"""Test-asset repair for EV-022: align the tester-owned fw_core harness with the
frozen rising-direction report rule (T1) instead of the old as-built cooling rule.

Only EXPECTATIONS of the tester harness change. No product source is touched and
no assertion is relaxed: the new reference hash was computed by an independent
Python model (evidence/ev022/ref_report_hash.py) from the readme/IC-002 rule.

Reference rule: report = (cur > 350) || (have_prev && dark && (cur - prev) > 9)
Reference FNV-1a hash over the same 13x11x2x2 grid: 0xA524C2E9
"""

PATH = ("D:/mypro/beiwo2/AIP/CW32L010_StandardPeripheralLib_V1.0.5/Examples/"
        "sensor/gpio_input_output/test/host_fw_core_verify_ev.c")

OLD_HASH = b"#define REF_REPORT_HASH 0xF7D66E3Eu\r\n"
NEW_HASH = b"#define REF_REPORT_HASH 0xA524C2E9u   /* EV-022 independent reference (rising direction) */\r\n"

OLD_T8 = (
    b'    /* ---- T8: exhaustive report decision over a structured grid ----\r\n'
    b'     * NOTE: the hash below characterises the AS-BUILT cooling direction (registered\r\n'
    b"     * deviation D-01); the readme's rising direction is firmware task T4 and will\r\n"
    b'     * require a new reference hash here once it lands. */\r\n'
    b'    printf("[T8] exhaustive report decision grid (as-built, D-01)\\n");\r\n'
)

NEW_T8 = (
    b'    /* ---- T8: exhaustive report decision over a structured grid ----\r\n'
    b'     * EV-022: reference re-derived for the frozen rising direction\r\n'
    b'     *   report = (cur > 350) || (have_prev && dark && (cur - prev) > 9)\r\n'
    b'     * Hash computed by the independent Python model\r\n'
    b'     * evidence/ev022/ref_report_hash.py (same 572-vector grid, unbounded ints). */\r\n'
    b'    printf("[T8] exhaustive report decision grid (rising direction, T1)\\n");\r\n'
)

OLD_T8_CHECK = (
    b'      CHECK(h == REF_REPORT_HASH, "all grid decisions match the as-built rule '
    b'report=(cur>350)||(cur<350&&have&&dark&&(prev-cur>9)) [D-01: cooling direction]"); }\r\n'
)
NEW_T8_CHECK = (
    b'      CHECK(h == REF_REPORT_HASH, "all grid decisions match '
    b'report=(cur>350)||(have&&dark&&(cur-prev>9)) [rising direction]"); }\r\n'
)

OLD_T8B = (
    b'    printf("[T8b] report truth table (as-built, D-01; readme\'s rising direction pending T4)\\n");\r\n'
    b'    CHECK(sensor_decide_report(0, false, 200, true) == false, "no prev, cur=200 -> false");\r\n'
    b'    CHECK(sensor_decide_report(0, false, 350, true) == false, "no prev, cur=350 (exactly 35.0C) -> false");\r\n'
    b'    CHECK(sensor_decide_report(0, false, 351, false) == true, "no prev, cur=351 -> true (light-independent)");\r\n'
    b'    CHECK(sensor_decide_report(300, true, 291, true) == false, "drop exactly 9 (0.9C) -> false (strict)");\r\n'
    b'    CHECK(sensor_decide_report(300, true, 290, true) == true, "drop 10 (1.0C) + DARK -> true");\r\n'
    b'    CHECK(sensor_decide_report(300, true, 290, false) == false, "drop 10 but LIT -> false");\r\n'
    b'    CHECK(sensor_decide_report(360, true, 350, true) == false, "cur=350 with large drop -> false");\r\n'
    b'    CHECK(sensor_decide_report(360, true, 351, true) == true, "cur=351 -> true regardless of drop");\r\n'
    b'    CHECK(sensor_decide_report(300, true, 300, true) == false, "prev==cur -> false");\r\n'
    b'    CHECK(sensor_decide_report(300, true, 320, true) == false, "temperature rise -> false");\r\n'
    b'    CHECK(sensor_decide_report(-100, true, -120, true) == true, "negative temps: -10.0 -> -12.0 C drop 2.0C -> true");\r\n'
    b'    CHECK(sensor_decide_report(32767, true, -32768, true) == true, "int16 extremes do not overflow (drop computed in 32-bit)");\r\n'
    b'    CHECK(sensor_decide_report(1000, true, 350, true) == false,\r\n'
    b'          "exactly 35.0C + large drop + DARK -> no report (IC-002 T<35 conjunct)");\r\n'
    b'    CHECK(sensor_decide_report(1000, true, 349, true) == true,\r\n'
    b'          "34.9C + large drop + DARK -> report");\r\n'
    b'    CHECK(sensor_decide_report(-32768, true, -32768, true) == false, "int16 min, no drop -> false");\r\n'
    b'    CHECK(sensor_decide_report(32767, true, 32767, true) == true, "int16 max -> over-temp branch -> true");\r\n'
)

NEW_T8B = (
    b'    printf("[T8b] report truth table (rising direction, T1)\\n");\r\n'
    b'    CHECK(sensor_decide_report(0, false, 200, true) == false, "no prev, cur=200 -> false");\r\n'
    b'    CHECK(sensor_decide_report(0, false, 350, true) == false, "no prev, cur=350 (exactly 35.0C) -> false");\r\n'
    b'    CHECK(sensor_decide_report(0, false, 351, false) == true, "no prev, cur=351 -> true (light-independent)");\r\n'
    b'    CHECK(sensor_decide_report(300, true, 309, true) == false, "rise exactly 9 (0.9C) + DARK -> false (strict)");\r\n'
    b'    CHECK(sensor_decide_report(300, true, 310, true) == true, "rise 10 (1.0C) + DARK -> true");\r\n'
    b'    CHECK(sensor_decide_report(300, true, 310, false) == false, "rise 10 but LIT -> false");\r\n'
    b'    CHECK(sensor_decide_report(349, true, 350, true) == false, "34.9 -> 35.0 DARK -> false (no rise branch, not over 35.0)");\r\n'
    b'    CHECK(sensor_decide_report(360, true, 350, true) == false, "exactly 35.0C + no rise -> false (strict >35.0)");\r\n'
    b'    CHECK(sensor_decide_report(360, true, 351, true) == true, "cur=351 -> true regardless of light/prev");\r\n'
    b'    CHECK(sensor_decide_report(300, true, 300, true) == false, "prev==cur -> false");\r\n'
    b'    CHECK(sensor_decide_report(300, true, 290, true) == false, "cooling 1.0C + DARK -> false (rising direction only)");\r\n'
    b'    CHECK(sensor_decide_report(300, true, 291, true) == false, "cooling 0.9C + DARK -> false");\r\n'
    b'    CHECK(sensor_decide_report(-100, true, -120, true) == false, "negative temps -10.0 -> -12.0 C cooling -> false");\r\n'
    b'    CHECK(sensor_decide_report(32767, true, -32768, true) == false, "int16 extremes, downward -> false (int32 math)");\r\n'
    b'    CHECK(sensor_decide_report(-32768, true, 32767, true) == true, "int16 extremes, upward over-temp -> true");\r\n'
    b'    CHECK(sensor_decide_report(1000, true, 350, true) == false, "exactly 35.0C, no rise -> no report (strict T>35)");\r\n'
    b'    CHECK(sensor_decide_report(1000, true, 340, true) == false, "34.0C cooling + DARK -> no report");\r\n'
    b'    CHECK(sensor_decide_report(-32768, true, -32768, true) == false, "int16 min, no rise -> false");\r\n'
    b'    CHECK(sensor_decide_report(32767, true, 32767, true) == true, "int16 max -> over-temp branch -> true");\r\n'
)


def main():
    with open(PATH, "rb") as f:
        d = f.read()

    if b"EV-022 independent reference" in d:
        print("already patched")
        return 0

    for name, old, new in (("REF_REPORT_HASH", OLD_HASH, NEW_HASH),
                           ("T8 header", OLD_T8, NEW_T8),
                           ("T8 CHECK", OLD_T8_CHECK, NEW_T8_CHECK),
                           ("T8b block", OLD_T8B, NEW_T8B)):
        n = d.count(old)
        if n != 1:
            print("ERROR: %s matched %d times (expected 1)" % (name, n))
            return 1
        d = d.replace(old, new)

    with open(PATH, "wb") as f:
        f.write(d)
    print("patched %s" % PATH)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
