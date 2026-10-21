#!/usr/bin/env python3
"""
verify_com43_item004.py - 实板 COM43 原始字节独立解析（embedded_tester.embedded_verification）

判据（ITEM-004 实板可见部分）:
  - 每个采样周期恰一条 S 行；失败周期紧随一条 G 行；
  - 每行以 CRLF 结束、线上字节数 <= 96；
  - S/G 字段数量、键名与顺序固定，值为十进制整数/大写 HEX；
  - 失败周期 s=0（未发送）、y=0（无残留重试）、q=0、r=0；G 的 a44/a45/rd/at/raw 自洽；
  - 相邻周期 k 增量 = 3（1 min RTC × 3 拍 = 180 s），与配置一致；
  - 捕获窗口之外没有半行/悬挂字节。

用法: python verify_com43_item004.py com43_capture_a.dat com43_capture_b.dat
"""
import re
import sys

MAXLINE = 96
passed = 0
failed = 0


def check(cond, msg):
    global passed, failed
    if cond:
        passed += 1
        print("PASS  " + msg)
    else:
        failed += 1
        print("FAIL  " + msg)


S_KEYS = ["k", "p", "H", "V", "o", "n", "x", "a", "D", "t", "h", "q", "r", "s", "y"]
G_KEYS = ["s", "a44", "a45", "rd", "at", "raw"]


def parse_lines(data, name):
    check(data.endswith(b"\r\n"), "%s: 最后一行以 CRLF 结束（无悬挂半字节）" % name)
    raw = data.split(b"\r\n")
    if data.endswith(b"\r\n"):
        raw = raw[:-1]
    lines = [x + b"\r\n" for x in raw]
    for i, ln in enumerate(lines):
        check(len(ln) <= MAXLINE, "%s 行%d: %d B <= %d" % (name, i, len(ln), MAXLINE))
        check(ln.endswith(b"\r\n"), "%s 行%d: 含 CRLF" % (name, i))
        check(all(32 <= b < 127 for b in ln[:-2]), "%s 行%d: 仅可打印 ASCII" % (name, i))
    return [x[:-2].decode("ascii") for x in lines]


def parse_s(line, name, idx):
    parts = line.split(" ")
    check(parts[0] == "S", "%s S%d: 行首为 S" % (name, idx))
    fields = dict(p.split("=", 1) for p in parts[1:])
    check(list(fields.keys()) == S_KEYS, "%s S%d: 字段键名与顺序冻结" % (name, idx))
    for k, v in fields.items():
        if k == "raw":
            continue
        check(re.fullmatch(r"-?\d+", v) is not None, "%s S%d: %s 为十进制整数（%s）" % (name, idx, k, v))
    return fields


def parse_g(line, name, idx):
    parts = line.split(" ")
    check(parts[0] == "G", "%s G%d: 行首为 G" % (name, idx))
    fields = dict(p.split("=", 1) for p in parts[1:])
    check(list(fields.keys()) == G_KEYS, "%s G%d: 字段键名与顺序冻结" % (name, idx))
    check(re.fullmatch(r"[0-9A-F]{12}|-{12}", fields["raw"]) is not None,
          "%s G%d: raw 为 12 位大写 HEX 或 12 个 '-'" % (name, idx))
    for k in ["s", "a44", "a45", "rd", "at"]:
        check(re.fullmatch(r"\d+", fields[k]) is not None, "%s G%d: %s 为整数" % (name, idx, k))
    return fields


def main():
    if len(sys.argv) < 3:
        print("usage: verify_com43_item004.py <a.dat> <b.dat>")
        return 2

    cycles = []
    for path in sys.argv[1:3]:
        name = path.split("/")[-1].split("\\")[-1]
        data = open(path, "rb").read()
        print("-- %s: %d bytes" % (name, len(data)))
        lines = parse_lines(data, name)
        check(len(lines) > 0, "%s: 捕获非空" % name)
        i = 0
        while i < len(lines):
            s = parse_s(lines[i], name, i)
            check(s["q"] == "0", "%s S%d: 当前实板 q=0（GXHT40 无 ACK，U01 未闭合）" % (name, i))
            check(s["r"] == "0", "%s S%d: 无触发上报 r=0" % (name, i))
            check(s["s"] == "0", "%s S%d: 未发送 s=0（无残留发送状态）" % (name, i))
            check(s["y"] == "0", "%s S%d: 无重试残留 y=0" % (name, i))
            check(s["H"] in ("0", "1") and s["V"] in ("0", "1") and s["D"] in ("0", "1"),
                  "%s S%d: H/V/D 为 0/1" % (name, i))
            check(0 <= int(s["o"]) <= 8, "%s S%d: 光照成功样本数 o 在 0..8" % (name, i))
            check(0 <= int(s["n"]) <= int(s["x"]) <= 4095,
                  "%s S%d: 0 <= n <= x <= 4095" % (name, i))
            check(int(s["n"]) <= int(s["a"]) <= int(s["x"]),
                  "%s S%d: n <= a <= x（均值落在最小/最大之间）" % (name, i))
            cycle = {"k": int(s["k"]), "name": name, "g": None}
            i += 1
            if i < len(lines) and lines[i].startswith("G "):
                g = parse_g(lines[i], name, i)
                cycle["g"] = g
                check(g["a44"] == "0" and g["a45"] == "0",
                      "%s G%d: 0x44/0x45 均无 ACK（与 S.q=0 一致）" % (name, i))
                check(g["raw"] == "------------", "%s G%d: raw 未读成功时为 12 个 '-'" % (name, i))
                check(g["s"] == "2", "%s G%d: 状态 2（无器件）" % (name, i))
                i += 1
            else:
                check(False, "%s S%d: 失败周期后必须紧跟 G 行" % (name, i - 1))
            cycles.append(cycle)

    ks = [c["k"] for c in cycles]
    print("observed sample ticks: %s" % ks)
    check(len(cycles) >= 3, "两次捕获合计 >= 3 个周期（至少一个窗口内两个周期）")
    check(ks == sorted(ks), "k 单调递增")
    for a, b in zip(ks, ks[1:]):
        check(b - a == 3, "相邻周期 k 增量 = 3（1 min × 3 = 180 s）: %d -> %d" % (a, b))
    check(cycles[0]["k"] >= 3, "复位后首个观测周期 k >= 3（3 分钟采样节拍）")

    print("\nchecks: %d passed / %d failed" % (passed, failed))
    print("RESULT " + ("PASS" if failed == 0 else "FAIL"))
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
