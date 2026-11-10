#!/usr/bin/env python3
"""Numerical verification of the decisive claim behind the WORK_FAILED verdict.

Claim (artifact §11.2): with code(R) = 4095*R/(R3+R),
    ratio(R3) = code_lit/code_dark = (R_l/R_d) * (R3+R_d)/(R3+R_l)
is monotonically decreasing in R3 and tends to R_l/R_d; therefore any lit state with
R_l/R_d > 1/3 can never be reported as "light" for ANY divider resistor value.

This script does not assume that; it sweeps R3 over 10 kOhm .. 1 GOhm and checks:
  1. monotonicity of ratio(R3) on the sweep,
  2. the sweep minimum against the analytic limit R_l/R_d,
  3. whether ratio <= 1/3 is reached at all, and if so the smallest R3 that reaches it
     (also solved analytically as a cross-check).

Inputs: the measured ADC means already reported in the artifact; no hardware access.
Output: stdout (redirected to r3_sweep_verification_20261110.txt).
"""
import sys

ADC_FULL_SCALE = 4095
R3_OHM = 5_000_000          # BOM value, used only to reverse-calculate R_ldr


def r_of_code(code):
    return R3_OHM * code / float(ADC_FULL_SCALE - code)


def code_of(r, r3):
    return ADC_FULL_SCALE * r / (r3 + r)


CASES = [
    # label, lit code, dark code (same ADC scale)
    ("room lamp 3514 vs same-session no-light 3750", 3514, 3750),
    ("room lamp 3514 vs 2026-11-09 darkest 3914", 3514, 3914),
    ("direct light 3061 vs same-session no-light 3750", 3061, 3750),
    ("direct light 3061 vs 2026-11-09 darkest 3914", 3061, 3914),
]


def main():
    print("=" * 78)
    print("Numerical verification: does any R3 make a lit state reportable as 'light'?")
    print("rule: light requires code_lit <= code_dark/3  <=>  ratio <= 0.3333")
    print("sweep: R3 = 1e4 .. 1e9 ohm, 61 logarithmic points; analytic check as cross-check")
    print("=" * 78)
    rc = 0
    for label, lit_code, dark_code in CASES:
        rl, rd = r_of_code(lit_code), r_of_code(dark_code)
        print()
        print("case: %s" % label)
        print("   R_l=%.3f MOhm  R_d=%.3f MOhm  R_l/R_d=%.4f  (limit as R3->inf)"
              % (rl / 1e6, rd / 1e6, rl / rd))

        ratios = []
        r3 = 1e4
        while r3 <= 1e9 * 1.0001:
            ratios.append((r3, (rl / rd) * ((r3 + rd) / (r3 + rl))))
            r3 *= 1.0 + (1e9 / 1e4) ** (1.0 / 60.0) - 1.0  # 61 log steps
        monotone = all(ratios[i][1] >= ratios[i + 1][1] - 1e-15 for i in range(len(ratios) - 1))
        print("   monotone decreasing over sweep: %s" % ("YES" if monotone else "NO"))
        print("   ratio(10 kOhm)=%.4f  ratio(1 GOhm)=%.4f  minimum on sweep=%.4f"
              % (ratios[0][1], ratios[-1][1], min(r for _x, r in ratios)))
        print("   gap min_sweep -> analytic floor = %.2e (finite sweep end at 1 GOhm)"
              % abs(min(r for _x, r in ratios) - rl / rd))

        hit = [(x, r) for x, r in ratios if r <= 1.0 / 3.0]
        if hit:
            smallest = min(hit)[0]
            analytic = 2.0 * rl / (1.0 - 3.0 * rl / rd) if rd != 0 else float("inf")
            print("   ratio<=1/3 IS reachable: smallest swept R3 = %.0f MOhm" % (smallest / 1e6))
            print("   analytic threshold R3 = 2*R_l/(1-3*R_l/R_d) = %.1f MOhm" % (analytic / 1e6))
            print("   -> numerically consistent: %s"
                  % ("YES" if 0.2 < smallest / analytic < 5.0 else "NO (check pixelation)"))
            if smallest / 1e6 > 100.0:
                print("   -> but that R3 is impractical: source impedance for a 390 us ADC sample")
                print("      and input leakage must be evaluated by hardware before use.")
        else:
            print("   ratio<=1/3 is NOT reachable for any R3 in 10k..1G:")
            print("   the infimum %.4f already exceeds 0.3333 -> structural impossibility"
                  % (rl / rd))

        if not monotone:
            rc = 1
    print()
    print("=" * 78)
    print("Interpretation")
    print("  * The 'no R3 can help' statement for the room-lamp state is confirmed")
    print("    numerically against the same-session no-light reference (ratio floor 0.556).")
    print("  * Against the darker 2026-11-09 reference the floor is 0.280, i.e. just below")
    print("    1/3, but the R3 needed is hundreds of MOhm - outside a practical divider.")
    print("  * Both facts keep the verdict WORK_FAILED; the requirement owner still has to")
    print("    choose one of the P1..P4 paths (artifact section 11.3 / 13.3).")
    return rc


if __name__ == "__main__":
    sys.exit(main())
