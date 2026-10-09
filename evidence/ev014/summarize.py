#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
evidence/ev014/summarize.py

把本轮 COM42 原始捕获按「复位段 / 采样周期」整理成可直接引用的表格。
只做逐行归类与字段抽取, 不做任何判定 (判定在 embedded_verify_gxht40_item002.py)。

用法: python summarize.py <prefix1> [prefix2 ...]
"""
import json
import os
import re
import sys

BOOT = re.compile(rb"^(BOOT|IOTEST|IOSIG|BUS)\b")


def main(prefixes):
    print("| 捕获 | 窗口 (UTC) | 字节 | 行序 |")
    print("|---|---|---|---|")
    rows = []
    cycles = []
    for p in prefixes:
        with open(p + ".cap", "rb") as f:
            raw = f.read()
        with open(p + ".cap.json", "r", encoding="utf-8") as f:
            meta = json.load(f)
        lines = raw.replace(b"\r\n", b"\n").split(b"\n")
        lines = [l for l in lines if l.strip()]
        order = []
        for l in lines:
            tag = l.split(b" ")[0].decode("ascii", "replace")
            order.append(tag)
            if tag == b"S".decode():
                d = dict(kv.split(b"=", 1) for kv in l.split(b" ")[1:])
                d = {k.decode(): v.decode() for k, v in
                     (kv.split(b"=", 1) for kv in l.split(b" ")[1:])}
                cycles.append((p, d, meta["started_at"]))
        rows.append((p, meta["started_at"][11:19] + " .. " + meta["finished_at"][11:19],
                     len(raw), ",".join(order)))
    for r in rows:
        print("| %s | %s | %d | %s |" % r)

    print()
    print("| 捕获 | k | q | t (x10) | h (x10) | p | H | V | o | n | x | a | D | r | s | y |")
    print("|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|")
    for p, d, _ in cycles:
        print("| %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s |"
              % (p, d["k"], d["q"], d["t"], d["h"], d["p"], d["H"], d["V"], d["o"],
                 d["n"], d["x"], d["a"], d["D"], d["r"], d["s"], d["y"]))
    ks = [int(d["k"]) for _, d, _ in cycles]
    print()
    print("cycles=%d  q=1 count=%d  k values=%s" % (len(cycles),
                                                    sum(1 for _, d, _ in cycles if d["q"] == "1"), ks))


if __name__ == "__main__":
    main(sys.argv[1:])
