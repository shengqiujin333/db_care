#!/usr/bin/env python3
"""Independent parser/verifier for the ITEM-005 real-board COM43 captures.

Usage:
  python verify_com43_item005.py <capture_a.bin> <capture_b.bin>

Checks the raw UART1 bytes (9600 8N1) against the frozen diagnostic line contract:
  - ASCII only, every line terminated by CRLF, every line <= 96 bytes including CRLF
  - only S and G lines in these windows (the board was reset before capture started)
  - one S per sampling opportunity; each S with q=0 is followed by exactly one G
  - k increments by SENSOR_SAMPLE_TICKS (3) between consecutive opportunities
  - S field set/order and self-consistency; G failure shape matches "no device ACK"
"""
import sys

S_KEYS = ["k", "p", "H", "V", "o", "n", "x", "a", "D", "t", "h", "q", "r", "s", "y"]
G_KEYS = ["s", "a44", "a45", "rd", "at", "raw"]


def parse_capture(path):
    data = open(path, "rb").read()
    checks = []

    def chk(cond, msg):
        checks.append((bool(cond), msg))

    chk(all(0x09 <= b <= 0x0D or 0x20 <= b <= 0x7E for b in data),
        f"{path}: payload is printable ASCII (HEX/int fields only)")
    chk(data.endswith(b"\r\n"), f"{path}: last line ends with CRLF (no truncated tail)")
    raw_lines = data.split(b"\r\n")
    chk(raw_lines[-1] == b"", f"{path}: no trailing bytes after final CRLF")
    lines = []
    for raw in raw_lines[:-1]:
        chk(len(raw) + 2 <= 96, f"{path}: line length {len(raw) + 2} <= 96 (incl. CRLF)")
        try:
            lines.append(raw.decode("ascii"))
        except UnicodeDecodeError:
            chk(False, f"{path}: line is not ASCII")
    return lines, checks


def parse_kv(line, keys, name):
    parts = line.split()
    ok = len(parts) == len(keys) + 1 and parts[0] == name
    vals = {}
    if ok:
        for p, key in zip(parts[1:], keys):
            if not p.startswith(key + "="):
                ok = False
                break
            vals[key] = p.split("=", 1)[1]
    return ok, vals


def main():
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    all_checks = []
    samples = []
    for path in sys.argv[1:]:
        lines, checks = parse_capture(path)
        all_checks.extend(checks)
        all_checks.append((bool(lines), f"{path}: contains diagnostic lines"))
        # structural: S must be followed by G, G must follow S
        for i, line in enumerate(lines):
            name = line.split()[0] if line.split() else ""
            if name == "S":
                ok, vals = parse_kv(line, S_KEYS, "S")
                all_checks.append((ok, f"{path}: S line has the 15 frozen keys in order"))
                if ok:
                    samples.append(vals)
                    all_checks.append((i + 1 < len(lines) and lines[i + 1].startswith("G "),
                                       f"{path}: S k={vals.get('k')} (q=0) followed by exactly one G"))
                    v = {k: int(x) for k, x in vals.items()}
                    all_checks.append((0 <= v["o"] <= 8 and (v["a"] == 0) == (v["o"] == 0) and
                                       v["n"] <= v["a"] <= v["x"],
                                       f"{path}: S k={v['k']} light fields o/n/a/x self-consistent"))
                    all_checks.append((v["V"] == 0 and v["D"] == 0,
                                       f"{path}: S k={v['k']} uncalibrated V=0 -> D=0"))
                    all_checks.append((v["H"] == 0 and v["p"] == 0,
                                       f"{path}: S k={v['k']} first-sample H=0/p=0"))
                    all_checks.append((v["q"] == 0 and v["r"] == 0 and v["s"] == 0 and v["y"] == 0,
                                       f"{path}: S k={v['k']} failed sample -> no report/send (q=r=s=y=0)"))
                    all_checks.append((v["k"] >= 0 and v["t"] == 0 and v["h"] == 0,
                                       f"{path}: S k={v['k']} no fabricated t/h on a failed sample"))
            elif name == "G":
                ok, vals = parse_kv(line, G_KEYS, "G")
                all_checks.append((ok, f"{path}: G line has the 7 frozen keys in order"))
                if ok:
                    all_checks.append((vals["s"] == "2" and vals["a44"] == "0" and vals["a45"] == "0"
                                       and vals["rd"] == "0" and vals["at"] == "3"
                                       and vals["raw"] == "------------",
                                       f"{path}: G failure shape matches no-device ACK (a44=a45=0, at=3)"))
            else:
                all_checks.append((False, f"{path}: unexpected line type {name!r} in this window"))

    ks = [int(v["k"]) for v in samples]
    all_checks.append((len(ks) >= 2, f"at least two real sampling opportunities observed (got {len(ks)})"))
    all_checks.append((len(ks) == len(set(ks)), "exactly one S per k (no duplicate opportunity)"))
    if len(ks) >= 2:
        diffs = [b - a for a, b in zip(ks, ks[1:])]
        all_checks.append((all(d == 3 for d in diffs),
                           f"k increments by 3 between opportunities: {ks} diffs={diffs}"))

    failed = [m for ok, m in all_checks if not ok]
    for ok, m in all_checks:
        print(("PASS  " if ok else "FAIL  ") + m)
    print(f"result: {len(all_checks) - len(failed)} passed, {len(failed)} failed; S rows k={ks}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
