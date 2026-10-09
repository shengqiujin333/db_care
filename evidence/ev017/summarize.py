#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
evidence/ev017/summarize.py

把本轮 COM42 原始捕获按「复位段 / 采样周期」整理成可直接引用的表格。
只做逐行归类与字段抽取, 不做任何判定 (判定在 embedded_verify_gxht40_item002.py)。

用法: python summarize.py <prefix1> [prefix2 ...]   (读取 <prefix>.cap 与 <prefix>.cap.json)
"""
import json
import os
import sys

BOOT_TAGS = ("BOOT", "VDD", "IOTEST", "IOSIG", "BUS")


def main(prefixes):
    cycles = []
    for p in prefixes:
        with open(p + ".cap", "rb") as f:
            raw = f.read()
        with open(p + ".cap.json", "r", encoding="utf-8") as f:
            meta = json.load(f)
        lines = [l for l in raw.replace(b"\r\n", b"\n").split(b"\n") if l.strip()]
        order = [l.split(b" ")[0].decode("ascii", "replace") for l in lines]
        print("===== %s  %s .. %s  (%d B) ====="
              % (p, meta["started_at"], meta["finished_at"], len(raw)))
        print("  line order: %s" % ",".join(order))
        for l in lines:
            tag = l.split(b" ")[0].decode("ascii", "replace")
            if tag in BOOT_TAGS or tag == "G":
                print("  %s" % l.decode("ascii", "replace"))
            if tag == "S":
                d = {}
                for kv in l.split(b" ")[1:]:
                    k, v = kv.split(b"=", 1)
                    d[k.decode()] = v.decode()
                cycles.append((p, d))
        print()

    print("| 捕获 | k | q | t (x10) | h (x10) | p | H | V | o | n | x | a | D | r | s | y |")
    print("|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|")
    for p, d in cycles:
        print("| %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s |"
              % (p, d["k"], d["q"], d["t"], d["h"], d["p"], d["H"], d["V"], d["o"],
                 d["n"], d["x"], d["a"], d["D"], d["r"], d["s"], d["y"]))
    ks = [int(d["k"]) for _, d in cycles]
    print()
    print("cycles=%d  q=1 count=%d  k values=%s"
          % (len(cycles), sum(1 for _, d in cycles if d["q"] == "1"), ks))


if __name__ == "__main__":
    main(sys.argv[1:])
