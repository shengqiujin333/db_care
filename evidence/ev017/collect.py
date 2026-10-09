#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
evidence/ev017/collect.py

回收本轮的 serial_capture / mdk_flash JSON 结果到仓库:
  python collect.py <work-dir> <repo-evidence-dir> <prefix1> [prefix2 ...]
把每个 <prefix>.cap.json 的 artifact 复制为 <repo>/<prefix>.cap ('.cap' 而非 '.bin':
仓库 .gitignore 含 *.bin), 记录窗口与 flash 日志, 复制后重新核对大小与 md5。
"""
import hashlib
import json
import os
import shutil
import sys


def md5(path):
    h = hashlib.md5()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def main():
    if len(sys.argv) < 4:
        print(__doc__)
        return 2
    work, repo, prefixes = sys.argv[1], sys.argv[2], sys.argv[3:]
    flash_lines, window_lines, bad = [], [], 0

    for p in prefixes:
        cj = os.path.join(work, p + ".cap.json")
        if not os.path.exists(cj) or os.path.getsize(cj) == 0:
            print("%s: MISSING capture json" % p)
            bad += 1
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
        if not os.path.exists(fj) or os.path.getsize(fj) == 0:
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
