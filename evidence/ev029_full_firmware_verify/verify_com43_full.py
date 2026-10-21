#!/usr/bin/env python3
"""Complete-delivery COM43 parser/judge (embedded_tester.embedded_verification).

Reads the raw UART1 byte windows captured from the real CW32L010 sensor board
(9600 8N1) and separates:

  * STRUCTURAL checks  - line set, field order/domains, CRLF, <=96 B, one S per
                         opportunity, G only after a failing S, no fabricated
                         values, k cadence (RTC 1 min x 3);
  * FUNCTIONAL checks  - the product acceptance items that need real sampled
                         data (PA-001: q=1 over >=3 consecutive cycles).

Exit code 0 only if every check passes; functional failure is reported as FAIL
with the raw evidence, not converted into a pass by structural correctness.
"""
import sys

MAXLINE = 96
S_KEYS = ["k", "p", "H", "V", "o", "n", "x", "a", "D", "t", "h", "q", "r", "s", "y"]
G_KEYS = ["s", "a44", "a45", "rd", "at", "raw"]

passed = 0
failed = 0
fail_msgs = []
info = []


def check(cond, name, detail=""):
    global passed, failed
    if cond:
        passed += 1
        print("PASS  %s%s" % (name, ("  [" + detail + "]") if detail else ""))
    else:
        failed += 1
        fail_msgs.append(name + ((" [" + detail + "]") if detail else ""))
        print("FAIL  %s%s" % (name, ("  [" + detail + "]") if detail else ""))


def kv(line):
    d = {}
    for tok in line.split(" ")[1:]:
        if "=" in tok:
            k, v = tok.split("=", 1)
            d[k] = v
    return d


def parse_window(path):
    raw = open(path, "rb").read()
    print("\n== window %s (%d B) ==" % (path, len(raw)))
    check(len(raw) > 0, "capture non-empty", "%d B" % len(raw))
    check(b"\n" not in raw.replace(b"\r\n", b""), "no bare LF")
    check(b"\r" not in raw.replace(b"\r\n", b""), "no stray CR")
    try:
        text = raw.decode("ascii")
        ascii_ok = True
    except UnicodeDecodeError:
        text = raw.decode("latin-1")
        ascii_ok = False
    check(ascii_ok, "capture is pure ASCII", "%d chars" % len(text))
    lines = [ln for ln in text.split("\r\n") if ln != ""]
    check(text.endswith("\r\n"), "capture ends with CRLF")
    check(all(len((ln + "\r\n").encode("ascii")) <= MAXLINE for ln in lines),
          "every line <= 96 B including CRLF",
          ", ".join("%dB" % (len(ln) + 2) for ln in lines))
    return lines


def judge_s(line, prev_k, window_tag):
    parts = line.split(" ")
    check(parts[0] == "S" and len(parts) == 1 + len(S_KEYS),
          "%s S line has fixed 15-field order" % window_tag, line)
    keys = [p.split("=", 1)[0] for p in parts[1:] if "=" in p]
    check(keys == S_KEYS, "%s S key order" % window_tag, ",".join(keys))
    d = kv(line)
    try:
        v = {k: int(x, 10) for k, x in d.items()}
    except ValueError:
        check(False, "%s S values are decimal integers" % window_tag, line)
        return None
    check(0 <= v["o"] <= 8 and v["n"] <= v["x"] and v["a"] <= v["x"] and 0 <= v["a"] <= 4095,
          "%s ADC fields in domain and ordered" % window_tag,
          "o=%d n=%d x=%d a=%d" % (v["o"], v["n"], v["x"], v["a"]))
    check((v["a"] == 0) == (v["o"] == 0), "%s a==0 iff o==0" % window_tag,
          "o=%d a=%d" % (v["o"], v["a"]))
    check(v["V"] == 0 or v["D"] in (0, 1), "%s dark field defined for valid light" % window_tag,
          "V=%d D=%d" % (v["V"], v["D"]))
    check(v["V"] == 0 and v["D"] == 0,
          "%s uncalibrated delivery: V=0/D=0" % window_tag,
          "V=%d D=%d" % (v["V"], v["D"]))
    check(v["q"] == 0 or (v["q"] == 1 and -400 <= v["t"] <= 1250 and 0 <= v["h"] <= 1000),
          "%s sampled values within valid domain" % window_tag,
          "q=%d t=%d h=%d" % (v["q"], v["t"], v["h"]))
    if v["q"] == 0:
        check(v["t"] == 0 and v["h"] == 0,
              "%s failed sample does not fabricate values" % window_tag,
              "t=%d h=%d" % (v["t"], v["h"]))
    check(v["r"] == 0 and v["s"] == 0 and v["y"] == 0,
          "%s failed sample requests no report / sends nothing" % window_tag,
          "r=%d s=%d y=%d" % (v["r"], v["s"], v["y"]))
    check(v["k"] % 3 == 0, "%s k is a multiple of the 3-tick sample period" % window_tag,
          "k=%d" % v["k"])
    if prev_k is not None:
        check(v["k"] > prev_k and (v["k"] - prev_k) % 3 == 0,
              "%s k advances by 3-minute ticks without reset" % window_tag,
              "%d -> %d" % (prev_k, v["k"]))
    return v


def judge_g(line, s_d, window_tag):
    parts = line.split(" ")
    check(parts[0] == "G" and len(parts) == 1 + len(G_KEYS),
          "%s G line has fixed field order" % window_tag, line)
    d = kv(line)
    check(d.get("s") == "2" and d.get("a44") == "0" and d.get("a45") == "0" and
          d.get("rd") == "0" and d.get("at") == "3",
          "%s G facts consistent with no device ACK" % window_tag, line)
    raw = d.get("raw", "")
    check(raw == "------------" or
          (len(raw) == 12 and all(c in "0123456789ABCDEF" for c in raw)),
          "%s G raw is 12 uppercase hex or placeholder" % window_tag, raw)
    if s_d is not None:
        check(s_d["q"] == 0, "%s G only emitted for a failing sample" % window_tag,
              "q=%d" % s_d["q"])


def main():
    if len(sys.argv) < 2:
        raise SystemExit("usage: verify_com43_full.py <windowA.bin> [windowB.bin ...]")

    all_s = []
    prev_k = None
    for path in sys.argv[1:]:
        lines = parse_window(path)
        expect_g = False
        for ln in lines:
            tag = "w%d" % (len(all_s) // 2 + 1)
            if ln.startswith("S "):
                check(not expect_g, "%s previous failing S is followed by G" % tag)
                d = judge_s(ln, prev_k, tag)
                if d is not None:
                    prev_k = d["k"]
                    all_s.append(d)
                expect_g = bool(d is not None and d["q"] == 0)
            elif ln.startswith("G "):
                judge_g(ln, all_s[-1] if all_s else None, tag)
                expect_g = False
            else:
                info.append("non-cycle line present: %s" % ln.split(" ")[0])
        check(not expect_g, "window %s ends without an unmatched failing S" % path)

    print("\n== aggregate ==")
    check(len(all_s) >= 2, "multiple sampling opportunities observed",
          "cycles=%d" % len(all_s))
    check(len({d["k"] for d in all_s}) == len(all_s), "no duplicate k",
          ",".join(str(d["k"]) for d in all_s))
    q_ok = sum(1 for d in all_s if d["q"] == 1)
    info.append("cycles=%d q1=%d q0=%d k=%s" %
                (len(all_s), q_ok, len(all_s) - q_ok,
                 ",".join(str(d["k"]) for d in all_s)))
    print("INFO  " + info[-1])
    if info:
        print("INFO  " + "; ".join(i for i in info[:-1]))

    # Functional acceptance (PA-001): q=1 over >=3 consecutive cycles.
    check(q_ok >= 3, "PA-001 real sampling: >=3 consecutive cycles with q=1",
          "observed q=1 in %d/%d cycles" % (q_ok, len(all_s)))
    # PA-017 diagnostic line set: BOOT/VDD/IOTEST/IOSIG/BUS power-on once.
    has_boot = any(x.startswith("non-cycle line present: BOOT") for x in info)
    has_vdd = any(x.startswith("non-cycle line present: VDD") for x in info)
    has_iotest = any(x.startswith("non-cycle line present: IOTEST") for x in info)
    has_iosig = any(x.startswith("non-cycle line present: IOSIG") for x in info)
    check(has_boot and has_vdd and has_iotest and has_iosig,
          "PA-017 power-on BOOT/VDD/IOTEST/IOSIG observed in these windows",
          "boot=%d vdd=%d iotest=%d iosig=%d" %
          (has_boot, has_vdd, has_iotest, has_iosig))

    print("\n==== result: %d passed, %d failed ====" % (passed, failed))
    for m in fail_msgs:
        print("MISMATCH " + m)
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
