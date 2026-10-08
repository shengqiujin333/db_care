#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
evidence/ev013/collect.py

把 run.sh / chain.sh 生成的 serial_capture / mdk_flash JSON 结果收进仓库:

  python collect.py <work-dir> <repo-evidence-dir>

- 每个 <prefix>.cap.json 的 artifacts[0] (serial_capture 写入角色工作目录的原始
  字节流) 复制为 <repo>/<prefix>.cap
  (**刻意不用 .bin 扩展名**: 仓库 .gitignore 忽略 *.bin, 上一轮 EV-012 的原始捕获
   因此从未真正进入 Git, 本轮改用 .cap 并回填 EV-012);
- 每次 mdk_flash 的 build/flash 原始日志与时间戳汇总到 <repo>/mdk_flash_logs.txt;
- 每次 serial_capture 的窗口起止时间汇总到 <repo>/capture_windows.txt;
- 复制后逐字节比对大小与 md5, 不一致则非零退出。
"""
import hashlib
import json
import os
import shutil
import sys

PREFIXES = ["b1", "b2", "c1", "c2"]


def md5(path):
    h = hashlib.md5()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    work, repo = sys.argv[1], sys.argv[2]
    flash_lines, window_lines, bad = [], [], 0

    for p in PREFIXES:
        cj = os.path.join(work, p + ".cap.json")
        if not os.path.exists(cj):
            continue
        with open(cj, "r", encoding="utf-8") as f:
            cap = json.load(f)
        src = cap["artifacts"][0]
        dst = os.path.join(repo, p + ".cap")
        shutil.copyfile(src, dst)
        same = os.path.getsize(src) == os.path.getsize(dst) and md5(src) == md5(dst)
        bad += 0 if same else 1
        print("%s: %d B md5=%s  %s -> %s"
              % (p, os.path.getsize(dst), md5(dst), os.path.basename(src),
                 os.path.basename(dst)))
        window_lines.append("%s  %d B  %s .. %s  %s"
                            % (p, os.path.getsize(dst), cap["started_at"],
                               cap["finished_at"], cap["stdout"].splitlines()[0]))
        fj = os.path.join(work, p + ".flash.json")
        if not os.path.exists(fj):
            continue
        with open(fj, "r", encoding="utf-8") as f:
            fl = json.load(f)
        flash_lines.append("===== %s  success=%s exit_code=%s  %s .. %s\n%s\n"
                           % (p, fl["success"], fl["exit_code"], fl["started_at"],
                              fl["finished_at"], fl["stdout"]))

    if flash_lines:
        with open(os.path.join(repo, "mdk_flash_logs.txt"), "w", encoding="utf-8") as f:
            f.write("".join(flash_lines))
        print("wrote mdk_flash_logs.txt (%d run(s))" % len(flash_lines))
    with open(os.path.join(repo, "capture_windows.txt"), "w", encoding="utf-8") as f:
        f.write("prefix | bytes | UTC window | first line\n" + "\n".join(window_lines) + "\n")
    print("wrote capture_windows.txt")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
