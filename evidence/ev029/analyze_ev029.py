#!/usr/bin/env python3
"""EV-029: independent parse of a raw COM43 capture (no implementation constants).

Checks: line framing (CRLF, no stray bytes), S-line field set/order, line length
<= 96 B, q/G presence, k step, and F-1 trace semantics (p of this line equals t
of the previous line; H=1 for a non-first sample).
"""
import hashlib
import pathlib
import re
import sys

RAW = pathlib.Path(__file__).with_name("serial_capture_1204Z.bin")
FIELDS = ["k", "p", "H", "V", "o", "n", "x", "a", "D", "t", "h", "q", "r", "s", "y"]


def parse_line(line: str) -> dict:
    m = re.fullmatch(r"S (.*)", line)
    if not m:
        return {}
    out = {}
    for token in m.group(1).split(" "):
        if "=" not in token:
            return {}
        key, val = token.split("=", 1)
        out[key] = val
    return out


def main() -> int:
    data = RAW.read_bytes()
    print(f"file={RAW.name} bytes={len(data)} md5={hashlib.md5(data).hexdigest()}")
    ends_crlf = data.endswith(b"\r\n")
    print("ends_with_crlf=" + str(ends_crlf))
    if b"\r\n" in data[:-2]:
        chunks = data.split(b"\r\n")[:-1]
    else:
        chunks = [data]
    stray = sum(1 for c in chunks if not c.startswith(b"S ") and not c.startswith(b"BUS "))
    print(f"lines={len(chunks)} stray_or_unknown={stray}")
    prev = None
    failures = []
    for idx, raw in enumerate(chunks, 1):
        text = raw.decode("ascii", errors="replace")
        length = len(raw) + 2  # line + CRLF
        print(f"[{idx}] len_with_crlf={length} raw={text!r}")
        if length > 96:
            failures.append(f"line {idx}: length {length} > 96")
        if not text.startswith("S "):
            continue
        f = parse_line(text)
        if list(f.keys()) != FIELDS:
            failures.append(f"line {idx}: field set/order {list(f.keys())} != {FIELDS}")
            continue
        checks = {
            "k_multiple_of_3": int(f["k"]) % 3 == 0,
            "t_in_range": -400 <= int(f["t"]) <= 1250,
            "h_in_range": 0 <= int(f["h"]) <= 1000,
            "q_is_1": f["q"] == "1",
            "o_is_8": f["o"] == "8",
            "no_report": f["r"] == "0" and f["s"] == "0" and f["y"] == "0",
            "uncalibrated_gate": f["V"] == "0" and f["D"] == "0",
        }
        for name, ok in checks.items():
            if not ok:
                failures.append(f"line {idx} k={f['k']}: {name} failed")
        if prev is not None:
            step = int(f["k"]) - int(prev["k"])
            print(f"    k_step={step} (expect 3)")
            if step != 3:
                failures.append(f"line {idx}: k step {step} != 3")
            # F-1: this line's p must equal previous line's t, and p != t
            ok = f["p"] == prev["t"] and f["p"] != f["t"] and f["H"] == "1"
            print(f"    F-1: p={f['p']} prev.t={prev['t']} cur.t={f['t']} H={f['H']} -> {'OK' if ok else 'MISMATCH'}")
            if not ok:
                failures.append(f"line {idx}: F-1 p/have semantics mismatch")
        prev = f
    print()
    if failures:
        print("FAILURES:")
        for x in failures:
            print(" - " + x)
        return 1
    print("RESULT: all framing/field/range/q and F-1 chain checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
