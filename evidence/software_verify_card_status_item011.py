#!/usr/bin/env python3
"""独立静态验证（software_tester.software_verification / ITEM-011；TD-SW-002 T-SW-L0-14、T-SW-L3-01/02/03）。

  A 旧判定入口清零（只看**代码**，注释中的历史说明不计）：`app/src/main`（排除 vendor paho）不得再出现
    `isOnline` / `OFFLINE_THRESHOLD`；卡片/状态相关文件（DeviceState/DeviceCardAdapter/HomeViewModel/
    item_device_card.xml）的代码或布局中不得出现“离线”文案。其它文件中的“离线”文本若为基线既有
    （如报告建议文案，属后续项），记录为 INFO 且必须未新增。
  B chip 四状态：`DeviceCardAdapter.bindStatusChip` 恰有 未收到/紧急/警告/正常 四个 `chip.text` 赋值，
    且（配色资源 + 字色）与改动前基线逐字一致；相对基线仅允许移除“离线”分支。
  C 提示文案：`reportHint` 两种取值都包含“按条件上报，可能长时间无上报”；未收到时温湿度仍为 `--`。
  D 布局：`reportHint` 位于 `tempValue` 之下，`sparkline` 顶部约束指向 `reportHint`。
  E 配色资源 `colors.xml` 与基线一致。
  F 卡片/状态类不得自行推断连通性（链路状态仍由 NotificationsViewModel 表达）。

运行：`python evidence/software_verify_card_status_item011.py`；退出码 0/1。
"""

from __future__ import annotations

import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APP = "dengbei_care/app/src/main"
DS_REL = f"{APP}/java/com/jinyuni/dengbei_care/DeviceState.kt"
ADAPTER_REL = f"{APP}/java/com/jinyuni/dengbei_care/ui/home/DeviceCardAdapter.kt"
HVM_REL = f"{APP}/java/com/jinyuni/dengbei_care/ui/home/HomeViewModel.kt"
LAYOUT_REL = f"{APP}/res/layout/item_device_card.xml"
COLORS_REL = f"{APP}/res/values/colors.xml"
BASELINE = "6f12fd4"  # 本项实现前的提交

CARD_FILES = {DS_REL, ADAPTER_REL, HVM_REL, LAYOUT_REL}

violations: list[str] = []


def abspath(rel: str) -> str:
    return os.path.join(ROOT, rel.replace("/", os.sep))


def read(rel: str) -> str:
    with open(abspath(rel), encoding="utf-8") as fh:
        return fh.read().replace("\r\n", "\n")


def baseline(rel: str):
    out = subprocess.run(["git", "show", f"{BASELINE}:{rel}"], cwd=ROOT, capture_output=True, text=True)
    return out.stdout.replace("\r\n", "\n") if out.returncode == 0 else None


def strip_comments(text: str) -> str:
    lines = []
    for line in text.split("\n"):
        stripped = line.strip()
        if stripped.startswith(("//", "*", "/*", "<!--")):
            continue
        lines.append(line.split("//")[0])
    return "\n".join(lines)


def chip_tuples(text: str):
    return dict(
        (m.group(1), (m.group(2), m.group(3)))
        for m in re.finditer(
            r'chip\.text = "([^"]+)"\s*\n\s*chip\.setChipBackgroundColorResource\(R\.color\.(\w+)\)\s*\n\s*chip\.setTextColor\((Color\.\w+)\)',
            text,
        )
    )


def main() -> int:
    adapter = read(ADAPTER_REL)
    layout = read(LAYOUT_REL)

    print("== A 旧判定/离线文案清零（只看代码，注释不计） ==")
    code_hits, text_hits = [], []
    for base, dirs, files in os.walk(abspath(APP)):
        dirs[:] = [d for d in dirs if d != "paho"]
        for name in files:
            if not name.endswith((".kt", ".java", ".xml")):
                continue
            p = os.path.join(base, name)
            rel = os.path.relpath(p, ROOT).replace("\\", "/")
            if "/org/eclipse/paho/" in rel:
                continue
            raw = open(p, encoding="utf-8", errors="ignore").read().replace("\r\n", "\n")
            for i, line in enumerate(strip_comments(raw).split("\n"), 1):
                if re.search(r"\bisOnline\b|OFFLINE_THRESHOLD", line):
                    code_hits.append(f"{rel}:{i}: {line.strip()[:88]}")
            for i, line in enumerate(raw.split("\n"), 1):
                s = line.strip()
                if "离线" in line and not s.startswith(("//", "*", "/*", "<!--")):
                    text_hits.append(f"{rel}:{i}: {s[:88]}")

    ok = not code_hits
    print(f"  {'PASS' if ok else 'FAIL'}  代码中 isOnline/OFFLINE_THRESHOLD = {len(code_hits)}")
    for h in code_hits[:5]:
        print("    " + h)
    if not ok:
        violations.append(f"A: 旧判定入口残留 {code_hits[:3]}")

    card_hits = [h for h in text_hits if h.split(":")[0] in CARD_FILES]
    ok = not card_hits
    print(f"  {'PASS' if ok else 'FAIL'}  卡片/状态文件的代码或布局中“离线” = {len(card_hits)}")
    for h in card_hits[:5]:
        print("    " + h)
    if not ok:
        violations.append(f"A: 卡片仍含离线文案 {card_hits[:3]}")

    print("  其它文件中的“离线”文本（须为基线既有，不属本项）：")
    others = [h for h in text_hits if h.split(":")[0] not in CARD_FILES]
    newly = []
    for h in others:
        rel, lineno, _ = h.split(":", 2)
        base_raw = baseline(rel)
        pre = bool(base_raw and "离线" in base_raw)
        print(f"    INFO  {rel}:{lineno} 基线同样含“离线” = {pre}")
        if not pre:
            newly.append(h)
    ok = not newly
    print(f"  {'PASS' if ok else 'FAIL'}  未在非卡片文件新增“离线”文案")
    if not ok:
        violations.append(f"A: 新增离线文案 {newly[:3]}")

    ok = not re.search(r"30\s*\*\s*60\s*\*\s*1000|30L?\s*\*\s*60L?\s*\*\s*1000", strip_comments(adapter))
    print(f"  {'PASS' if ok else 'FAIL'}  卡片逻辑无 30 分钟数据龄阈值")
    if not ok:
        violations.append("A: 卡片仍有 30 分钟阈值")

    print("== B chip 四状态与配色（对照基线） ==")
    cur = chip_tuples(adapter)
    print(f"  当前 = {cur}")
    ok = set(cur.keys()) == {"未收到", "紧急", "警告", "正常"}
    print(f"  {'PASS' if ok else 'FAIL'}  chip 文案集合 = {sorted(cur.keys())}")
    if not ok:
        violations.append(f"B: chip 文案集合异常 {sorted(cur.keys())}")
    base_text = baseline(ADAPTER_REL)
    if base_text:
        base_chips = chip_tuples(base_text)
        for k in ("未收到", "紧急", "警告", "正常"):
            same = base_chips.get(k) == cur.get(k)
            print(f"  {'PASS' if same else 'FAIL'}  {k} 配色/字色与基线一致：{cur.get(k)}")
            if not same:
                violations.append(f"B: {k} 配色被改 {base_chips.get(k)} -> {cur.get(k)}")
        removed = set(base_chips.keys()) - set(cur.keys())
        ok = removed == {"离线"}
        print(f"  {'PASS' if ok else 'FAIL'}  相对基线移除的分支 = {sorted(removed)}（期望仅 离线）")
        if not ok:
            violations.append(f"B: 非预期分支增删 {sorted(removed)}")

    print("== C 提示文案与 -- 占位 ==")
    checks = {
        "C1 未收到提示": "尚未收到上报 · 按条件上报，可能长时间无上报" in adapter,
        "C2 已收到提示含相对时间": "最近上报 ${state.lastReportAgeLabel()} · 按条件上报，可能长时间无上报" in adapter,
        "C3 条件上报提示出现 >=2 次": adapter.count("按条件上报，可能长时间无上报") >= 2,
        "C4 未收到时温湿度为 --（基于 isNeverReported）": '"--"' in adapter and "state.isNeverReported()" in adapter,
    }
    for k, v in checks.items():
        print(f"  {'PASS' if v else 'FAIL'}  {k}")
        if not v:
            violations.append(f"C: {k}")

    print("== D 布局接线 ==")
    checks_d = {
        "D1 reportHint 存在": 'android:id="@+id/reportHint"' in layout,
        "D2 reportHint 位于 tempValue 之下":
            'app:layout_constraintTop_toBottomOf="@id/tempValue"' in layout[layout.find('@+id/reportHint'):layout.find('@+id/sparkline')],
        "D3 sparkline 顶部约束指向 reportHint": 'app:layout_constraintTop_toBottomOf="@id/reportHint"' in layout,
    }
    for k, v in checks_d.items():
        print(f"  {'PASS' if v else 'FAIL'}  {k}")
        if not v:
            violations.append(f"D: {k}")

    print("== E 配色资源未变 ==")
    colors = read(COLORS_REL)
    colors_base = baseline(COLORS_REL)
    ok = colors_base is not None and colors.strip() == colors_base.strip()
    print(f"  {'PASS' if ok else 'FAIL'}  colors.xml 与基线一致")
    if not ok:
        violations.append("E: colors.xml 被改动")

    print("== F 连通性不由数据龄推断 ==")
    ds = read(DS_REL)
    ok = ("_mqtt_broker_state" not in adapter) and ("_mqtt_broker_state" not in ds)
    print(f"  {'PASS' if ok else 'FAIL'}  卡片/状态类未自行推断连通性")
    if not ok:
        violations.append("F: 卡片引入连通性推断")

    print("== RESULT ==")
    if violations:
        for v in violations:
            print("  VIOLATION:", v)
        return 1
    print("  OK: 旧离线判定清零、chip 四状态与配色不变、提示与 -- 占位正确、布局接线正确")
    return 0


if __name__ == "__main__":
    sys.exit(main())
