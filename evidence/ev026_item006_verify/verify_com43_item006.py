#!/usr/bin/env python3
"""Independent parser for the COM43 (9600 8N1) raw captures of ITEM-006.

Reads raw bytes captured from the real sensor board and checks:
  * framing: printable ASCII, CRLF-terminated, no truncated tail, <= 96 bytes/line;
  * BOOT carry the configured build identifier (when it is inside the window);
  * every S line has the 15 frozen keys in order and self-consistent light fields;
  * the config-dependent dark decision (uncalibrated -> V=0/D=0; calibrated ->
    D = V && 3*a >= C);
  * failed samples do not report/send and do not fabricate t/h;
  * G lines (if any) have the frozen keys and follow their S line.

No device access; raw capture files are the only input.
Exit code 0 = every executed check passed; 1 = at least one failure.
"""
import argparse
import os
import re
import sys

S_KEYS = ["k", "p", "H", "V", "o", "n", "x", "a", "D", "t", "h", "q", "r", "s", "y"]
G_KEYS = ["s", "a44", "a45", "rd", "at", "raw"]

pass_n = fail_n = 0
notes = []


def check(cond, msg):
    global pass_n, fail_n
    if cond:
        pass_n += 1
        print("PASS  %s" % msg)
    else:
        fail_n += 1
        print("FAIL  %s" % msg)


def body(path):
    with open(path, "rb") as f:
        return f.read()


def parse_kv(line):
    parts = line.split(" ")
    out = []
    for p in parts:
        if "=" in p:
            k, v = p.split("=", 1)
            out.append((k, v))
    return out


def analyze(path, calibrated, ref_code, build_id):
    data = body(path)
    name = os.path.basename(path)
    print("---- %s (%d bytes) ----" % (name, len(data)))
    check(all(32 <= b < 127 or b in (13, 10) for b in data),
          "%s: payload is printable ASCII + CRLF only" % name)
    check(data.endswith(b"\r\n"), "%s: last line ends with CRLF (no truncated tail)" % name)
    check(b"\r\n\r\n" not in data, "%s: no empty line / trailing bytes" % name)

    raw_lines = data.split(b"\r\n")
    if raw_lines and raw_lines[-1] == b"":
        raw_lines.pop()
    lines = [ln.decode("ascii") for ln in raw_lines]
    check(len(lines) > 0, "%s: at least one complete line" % name)
    for ln in lines:
        check(len(ln) + 2 <= 96, "%s: line %d bytes <= 96 incl CRLF: %r" %
              (name, len(ln) + 2, ln[:40]))

    boot_seen = False
    s_lines = []
    g_after_s = 0
    pending_s = None

    for ln in lines:
        if ln.startswith("BOOT "):
            boot_seen = True
            m = re.fullmatch(r"BOOT fw=(\S+) uid=([0-9A-F]{8}) rst=([0-9A-F]{4}) uart=9600", ln)
            check(m is not None, "%s: BOOT line matches frozen format: %r" % (name, ln))
            if m:
                check(m.group(1) == build_id,
                      "%s: BOOT fw=%s equals configured build id %s" %
                      (name, m.group(1), build_id))
        elif ln.startswith("S "):
            pending_s = ln
            kv = parse_kv(ln)
            keys = [k for k, _ in kv]
            check(keys == S_KEYS, "%s: S line has the 15 frozen keys in order: %s" %
                  (name, keys))
            if keys != S_KEYS:
                continue
            d = dict(kv)
            try:
                k = int(d["k"]); V = int(d["V"]); o = int(d["o"]); n = int(d["n"])
                x = int(d["x"]); a = int(d["a"]); D = int(d["D"]); t = int(d["t"])
                h = int(d["h"]); q = int(d["q"]); r = int(d["r"]); s = int(d["s"])
                y = int(d["y"]); H = int(d["H"])
            except ValueError:
                check(False, "%s: S line fields are integers: %r" % (name, ln))
                continue
            s_lines.append((k, ln))
            check(0 <= o <= 8, "%s: S k=%d o=%d in 0..8" % (name, k, o))
            check(0 <= n <= a <= x <= 4095, "%s: S k=%d n<=a<=x and 0..4095" % (name, k))
            check((a == 0) == (o == 0), "%s: S k=%d a==0 iff o==0" % (name, k))
            check(V in (0, 1) and D in (0, 1), "%s: S k=%d V/D are 0/1" % (name, k))
            if calibrated == 0:
                check(V == 0, "%s: S k=%d uncalibrated -> V=0" % (name, k))
                check(D == 0, "%s: S k=%d uncalibrated -> D=0 regardless of mean" % (name, k))
            else:
                expect = 1 if (V == 1 and 3 * a >= ref_code) else 0
                check(D == expect,
                      "%s: S k=%d D matches V && 3*a>=C (%d)" % (name, k, expect))
            check(q in (0, 1) and r in (0, 1) and s in (0, 1, 2) and y >= 0,
                  "%s: S k=%d q/r/s/y domains" % (name, k))
            if q == 0:
                check(r == 0 and s == 0,
                      "%s: S k=%d failed sample -> no report/send (r=s=0)" % (name, k))
                check(t == 0 and h == 0,
                      "%s: S k=%d failed sample -> no fabricated t/h" % (name, k))
            if r == 1:
                check(s in (1, 2), "%s: S k=%d report -> send result s in 1/2" % (name, k))
        elif ln.startswith("G "):
            kv = parse_kv(ln)
            keys = [k for k, _ in kv]
            check(keys == G_KEYS, "%s: G line has the 7 frozen keys in order: %s" %
                  (name, keys))
            check(pending_s is not None, "%s: G line follows an S line" % name)
            if pending_s:
                m = re.search(r" q=(\d)", pending_s)
                check(m is not None and m.group(1) == "0",
                      "%s: G only appears after a failed sample (q=0)" % name)
                g_after_s += 1
            pending_s = None
        elif ln.startswith(("VDD ", "IOTEST ", "IOSIG ", "BUS ")):
            pending_s = None
            if ln.startswith("BUS "):
                pass
        else:
            check(False, "%s: line starts with a known key: %r" % (name, ln))

    if not boot_seen:
        notes.append("%s: BOOT line not inside the capture window (reset occurred before capture)" % name)
    check(len(s_lines) >= 1, "%s: at least one complete S line" % name)
    if len(s_lines) >= 2:
        ks = [k for k, _ in s_lines]
        gaps = [b - a for a, b in zip(ks, ks[1:])]
        check(all(g == 3 for g in gaps),
              "%s: sampling opportunities are 3 ticks apart: %s" % (name, ks))
    for k, ln in s_lines:
        print("      S row k=%d: %s" % (k, ln))
    if g_after_s:
        print("      G rows paired with an S row: %d" % g_after_s)
    return len(s_lines)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("captures", nargs="+")
    ap.add_argument("--calibrated", type=int, default=0)
    ap.add_argument("--ref", type=int, default=0)
    ap.add_argument("--build-id", default="RUN10-FDS003")
    args = ap.parse_args()

    print("==== ITEM-006 independent COM43 capture verification ====")
    print("config: CALIBRATED=%d REF=%d BUILD_ID=%s" %
          (args.calibrated, args.ref, args.build_id))
    total = 0
    for p in args.captures:
        total += analyze(p, args.calibrated, args.ref, args.build_id)
    check(total >= 1, "at least one real sampling opportunity observed (got %d)" % total)
    for n in notes:
        print("NOTE  %s" % n)
    print("result: %d passed, %d failed" % (pass_n, fail_n))
    return 0 if fail_n == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
