#!/usr/bin/env python3
"""EV-041 parser/assertions for the P0 COM43 capture (no data is sent)."""
import hashlib
import re
import sys

CAP = sys.argv[1] if len(sys.argv) > 1 else "evidence/ev041/ev041_boot_capture.cap"

raw = open(CAP, "rb").read()
print(f"CAP {CAP} size={len(raw)} md5={hashlib.md5(raw).hexdigest()}")
text = raw.decode("latin-1")
lines = [ln for ln in text.split("\r\n") if ln != ""]
print(f"lines={len(lines)}")
for i, ln in enumerate(lines):
    print(f"  [{i}] ({len(ln)}B) {ln}")

failures = []


def check(cond, msg):
    print(("PASS " if cond else "FAIL ") + msg)
    if not cond:
        failures.append(msg)


# A debugger download resets the core once at connect and again after programming.
# The first boot can be cut mid-line by the programmer halt; split on every
# "BOOT fw=" occurrence so a truncated prefix cannot merge two groups.
segments = text.split("BOOT fw=")
boot_groups = ["BOOT fw=" + seg for seg in segments[1:]]
print(f"boot_groups={len(boot_groups)}")
for gi, seg in enumerate(boot_groups):
    head = seg.split("\r\n")[0]
    terminated = seg.split("\r\n", 1)[1] if "\r\n" in seg else ""
    print(f"  group[{gi}] head={head!r} terminated={bool(terminated)}")

if not boot_groups:
    check(False, "at least one BOOT line present")
else:
    # Last BOOT group must be the complete ordered power-on diagnostic sequence.
    seg = boot_groups[-1]
    group = [ln for ln in seg.split("\r\n") if ln != ""]
    if group and group[-1].startswith("S "):
        group = group[:-1]
    print("last_boot_group=", group)
    names = [ln.split()[0] for ln in group]
    check(names[:5] == ["BOOT", "VDD", "IOTEST", "IOSIG", "BUS"],
          f"ordered five boot lines BOOT<VDD<IOTEST<IOSIG<BUS (got {names[:5]})")
    check(sum(1 for n in names[:5] if n == "BOOT") == 1, "exactly one BOOT in final group")
    for want in ("VDD", "IOTEST", "IOSIG", "BUS"):
        check(names[:5].count(want) == 1, f"exactly one {want} line in final group")

    boot = group[0]
    m = re.match(r"BOOT fw=(\S+) uid=([0-9A-Fa-f]{8}) rst=([0-9A-Fa-f]{4}) uart=(\d+)$", boot)
    check(m is not None, "BOOT field format")
    if m:
        check(m.group(1) == "FD-002r4", f"fw id {m.group(1)} == FD-002r4")
        check(m.group(2).upper() == "6A002C00", f"uid4 {m.group(2)} == 6A002C00")
        check(m.group(4) == "9600", f"boot uart {m.group(4)} == 9600")

    vdd = next((ln for ln in group if ln.startswith("VDD ")), "")
    vm = re.match(r"VDD ok=(\d) code=(\d+) bgrmv=(\d+) mv=(\d+)$", vdd)
    check(vm is not None, "VDD field format")
    if vm:
        ok, code, bgr, mv = (int(x) for x in vm.groups())
        check(ok == 1, "VDD ok=1")
        calc = 4095 * bgr // code if code else -1
        check(calc == mv, f"VDD mv==4095*bgrmv//code ({calc}=={mv})")

    bus = next((ln for ln in group if ln.startswith("BUS ")), "")
    check("ack=44" in bus, "BUS ack=44 (GXHT40 address present)")

# Any S lines and their gate fields.
s_lines = [ln for ln in lines if ln.startswith("S ")]
print(f"S_lines={len(s_lines)}")
check(len(s_lines) >= 1, "at least one S sample line")
for ln in s_lines:
    d = dict(re.findall(r"(\w+)=(-?\d+)", ln))
    if "q" not in d:
        check(False, f"S parse {ln}")
        continue
    q, r, s, y = (int(d.get(k, 999)) for k in ("q", "r", "s", "y"))
    print(f"  S k={d.get('k')} t={d.get('t')} h={d.get('h')} q={q} r={r} s={s} y={y}")
    check(q == 1, f"S q=1 (k={d.get('k')})")
    check(r == 0, f"S r=0 (k={d.get('k')})")
    check(s == 0, f"S s=0 (k={d.get('k')})")

# No report/failure lines.
p_lines = [ln for ln in lines if ln.startswith("P ")]
g_lines = [ln for ln in lines if ln.startswith("G ")]
print(f"P_lines={len(p_lines)} G_lines={len(g_lines)}")
check(len(p_lines) == 0, "no P (report/freeze) lines")
check(len(g_lines) == 0, "no G (failure) lines")

# Line length bound: <=96 bytes + CRLF.
longest = max((len(ln) for ln in lines), default=0)
check(longest <= 96, f"longest line {longest}B <= 96B")

print(f"\nRESULT failures={len(failures)}")
sys.exit(1 if failures else 0)
