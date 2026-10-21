#!/usr/bin/env python3
"""ITEM-001 门控纯逻辑独立参考与比对（嵌入式测试能力自有证据脚本）。

独立性：
  - 期望模型直接按 IC-003 IF02 / FDS-003 DD-01 的契约重新表述，不复用产品 C 代码。
  - 只读取 harness 输出的原始结果行，逐点比对；不修改产品源码。
用法：
  python evidence/ev020_item001/verify_gate_item001.py <current.out> [old.out]
"""

import sys


def ref_gate(sv, prev, hp, cur, lv, dark):
    """report = sv && (cur>350 || (hp && lv && dark && int32(cur)-int32(prev)>9))"""
    if not sv:
        return 0
    if cur > 350:
        return 1
    if not hp:
        return 0
    if not (lv and dark):
        return 0
    return 1 if (cur - prev) > 9 else 0


def ref_rep4(prev, hp, cur, dark):
    """4 参兼容签名：dark 已由调用方折算 light_valid&&dark。"""
    if cur > 350:
        return 1
    if not hp:
        return 0
    if not dark:
        return 0
    return 1 if (cur - prev) > 9 else 0


def main():
    if len(sys.argv) < 2:
        print("usage: verify_gate_item001.py <current.out> [old.out]")
        return 2

    current_path = sys.argv[1]
    old_path = sys.argv[2] if len(sys.argv) > 2 else None

    vectors = []
    grid = []
    boundaries = []
    constants = None
    self_line = None

    with open(current_path, "r", encoding="utf-8", errors="replace") as fh:
        for raw in fh:
            line = raw.strip()
            if not line:
                continue
            parts = line.split()
            if parts[0] == "V":
                idx, sv, hp, prev, cur, lv, dark, got, exp = map(int, parts[1:10])
                vectors.append((idx, sv, hp, prev, cur, lv, dark, got, exp))
            elif parts[0] == "G":
                sv, hp, prev, cur, lv, dark, gate, rep4 = map(int, parts[1:9])
                grid.append((sv, hp, prev, cur, lv, dark, gate, rep4))
            elif parts[0] == "B":
                boundaries.append((parts[1], int(parts[2]), int(parts[3])))
            elif parts[0] == "C":
                constants = (int(parts[1]), int(parts[2]))
            elif parts[0] == "SELF":
                self_line = tuple(map(int, parts[1:4]))

    fails = []

    # 1. 常量
    if constants != (9, 350):
        fails.append("constants: got %r expected (9, 350)" % (constants,))

    # 2. 14 规范向量
    if len(vectors) != 14:
        fails.append("vectors: got %d rows, expected 14" % len(vectors))
    for (idx, sv, hp, prev, cur, lv, dark, got, exp) in vectors:
        want = ref_gate(sv, prev, hp, cur, lv, dark)
        if want != exp:
            fails.append("V%d spec/ref mismatch: ref=%d expected=%d" % (idx, want, exp))
        if got != want:
            fails.append("V%d executed=%d ref=%d (sv=%d hp=%d prev=%d cur=%d lv=%d dark=%d)"
                         % (idx, got, want, sv, hp, prev, cur, lv, dark))

    # 旧实现负向对照：必须且仅在 v3/v6/v7 与规范不符，且旧实现输出为 0/1/0
    if old_path:
        old_got = {}
        with open(old_path, "r", encoding="utf-8", errors="replace") as fh:
            for raw in fh:
                parts = raw.split()
                if len(parts) >= 3 and parts[0] == "O":
                    old_got[int(parts[1])] = int(parts[2])
        spec = {idx: exp for (idx, _sv, _hp, _p, _c, _lv, _d, _g, exp) in vectors}
        bad = []
        for idx in sorted(spec):
            if old_got.get(idx) != spec[idx]:
                bad.append(idx)
        if bad != [3, 6, 7]:
            fails.append("old-impl mismatch set %r, expected exactly [3, 6, 7]" % (bad,))
        if old_got.get(3) != 0 or old_got.get(6) != 1 or old_got.get(7) != 0:
            fails.append("old-impl v3/v6/v7 values %r, expected 0/1/0"
                         % ([old_got.get(3), old_got.get(6), old_got.get(7)],))

    # 3. 全网格：门控与独立参考、4 参兼容
    if len(grid) != 2 * 2 * 2 * 2 * 11 * 11:
        fails.append("grid: got %d rows, expected %d" % (len(grid), 2 * 2 * 2 * 2 * 11 * 11))
    grid_bad = 0
    equiv_bad = 0
    for (sv, hp, prev, cur, lv, dark, gate, rep4) in grid:
        want = ref_gate(sv, prev, hp, cur, lv, dark)
        if gate != want:
            grid_bad += 1
            if grid_bad <= 20:
                fails.append("G sv=%d hp=%d prev=%d cur=%d lv=%d dark=%d executed=%d ref=%d"
                             % (sv, hp, prev, cur, lv, dark, gate, want))
        if sv == 1:
            want4 = ref_rep4(prev, hp, cur, lv and dark)
            if rep4 != want4 or rep4 != gate:
                equiv_bad += 1
                if equiv_bad <= 20:
                    fails.append("E sv=1 hp=%d prev=%d cur=%d dark_folded=%d gate=%d rep4=%d ref4=%d"
                                 % (hp, prev, cur, lv and dark, gate, rep4, want4))
    if grid_bad:
        fails.append("grid mismatches: %d" % grid_bad)
    if equiv_bad:
        fails.append("4-param equivalence mismatches: %d" % equiv_bad)

    # 4. 边界
    for (name, got, exp) in boundaries:
        if got != exp:
            fails.append("B %s executed=%d expected=%d" % (name, got, exp))

    # 5. harness 自检汇总
    if self_line != (0, 0, 0):
        fails.append("harness self summary %r != (0,0,0)" % (self_line,))

    print("== ITEM-001 gate independent verification ==")
    print("vectors: %d, grid: %d, boundaries: %d, constants: %r"
          % (len(vectors), len(grid), len(boundaries), constants))
    if old_path:
        print("old implementation negative control: mismatch set %r (expected [3, 6, 7])"
              % ([i for i in range(1, 15) if old_got.get(i) != spec[i]],))
    if fails:
        print("RESULT: FAIL (%d findings)" % len(fails))
        for item in fails:
            print("  - " + item)
        return 1
    print("RESULT: PASS (14/14 vectors, %d grid points, 4-param equivalence, boundaries)"
          % len(grid))
    return 0


if __name__ == "__main__":
    sys.exit(main())
