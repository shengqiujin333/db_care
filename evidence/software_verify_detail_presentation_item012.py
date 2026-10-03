#!/usr/bin/env python3
"""独立静态验证（software_tester.software_verification / ITEM-012；TD-SW-002 T-SW-L3-04/05/06）。

  A 规则文案：从源码抽取 `REPORT_RULE_TEXT`，与独立写出的 readme 规则逐字比对；含 3 分钟/0.9℃/无光/35.0℃/才上报；
    不含 光照/照度/lux；不含 App 推断结论词；且“无光”全详情 UI 仅出现 1 次（就在规则原文里）。
  B 图表范围：上限 24 h（`MAX_CHART_RANGE_HOURS = 24`、`MAX_CHART_RANGE_MS` 由 `HOUR_MS` 派生）；
    详情页不再出现 `49 * 60`/“50 分钟”；提示文案取自 `rangeLimitMessage()`，且全 `app/src/main` 无“50 分钟”残留
    （基线中存在，可证明已改）。
  C 高温强调：严格 `>`（`isHighTemperature` 用 `>`，详情页调用该函数；无 `>= 35` 写法）；`HIGH_TEMP_C = 35.0`。
  D 无推断结论：详情 UI（Fragment/Layout/DetailPresentation）不出现 光照/照度/lux/推测/“因…上报”类文案；
    Fragment 不重算传感器门控（无 0.9 / 35 之外的门控比较）。
  E 布局与只读性：两个 TextView 存在；`reportRuleText` 不绑定点击监听（只读）。
  F 呈现层不参与判定：`DetailPresentation.isHighTemperature` 仅被详情页使用（`MqtttService`/固件路径不引用）。

运行：`python evidence/software_verify_detail_presentation_item012.py`；退出码 0/1。
"""

from __future__ import annotations

import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APP = "dengbei_care/app/src/main"
DP_REL = f"{APP}/java/com/jinyuni/dengbei_care/ui/detail/DetailPresentation.kt"
FRAG_REL = f"{APP}/java/com/jinyuni/dengbei_care/ui/detail/DeviceDetailFragment.kt"
LAYOUT_REL = f"{APP}/res/layout/fragment_device_detail.xml"
BASELINE = "f6d26dc"  # 本项实现前的提交

EXPECTED_RULE = (
    "采样周期 3 分钟。仅当温度较上次下降超过 0.9℃且无光，或温度超过 35.0℃ 时才上报；"
    "未满足条件时不会上报，因此可能长时间没有新数据。"
)

violations: list[str] = []


def read(rel):
    with open(os.path.join(ROOT, rel), encoding="utf-8") as fh:
        return fh.read().replace("\r\n", "\n")


def baseline(rel):
    out = subprocess.run(["git", "show", f"{BASELINE}:{rel}"], cwd=ROOT, capture_output=True, text=True)
    return out.stdout.replace("\r\n", "\n") if out.returncode == 0 else None


def strip_comments(text: str) -> str:
    """去掉整行注释（// * /* <!--），保留代码与字符串字面量。"""
    out = []
    for line in text.splitlines():
        t = line.strip()
        if t.startswith(("//", "*", "/*", "<!--")):
            continue
        out.append(line.split("//")[0])
    return "\n".join(out)


def main() -> int:
    dp = read(DP_REL)
    frag = read(FRAG_REL)
    layout = read(LAYOUT_REL)

    print("== A 规则文案 ==")
    m = re.search(r"val REPORT_RULE_TEXT: String =\s*(.*?)\n\n", dp, re.S)
    rule = "".join(re.findall(r'"([^"]*)"', m.group(1))) if m else None
    ok = rule == EXPECTED_RULE
    print(f"  {'PASS' if ok else 'FAIL'}  规则文案与 readme 逐字一致")
    if not ok:
        print(f"    got = {rule!r}")
        violations.append("A: 规则文案与期望不符")
    for token in ("采样周期 3 分钟", "0.9℃", "无光", "35.0℃", "才上报"):
        ok = rule is not None and token in rule
        print(f"  {'PASS' if ok else 'FAIL'}  含「{token}」")
        if not ok:
            violations.append(f"A: 缺少 {token}")
    dp_code = strip_comments(dp)
    frag_code = strip_comments(frag)
    detail_ui = dp_code + "\n" + frag_code + "\n" + layout
    for bad in ("lux", "照度", "光照", "勒克斯"):
        n = len(re.findall(bad, detail_ui, re.I))
        ok = n == 0
        print(f"  {'PASS' if ok else 'FAIL'}  详情 UI 不含「{bad}」= {n}")
        if not ok:
            violations.append(f"A: 详情 UI 出现 {bad}")
    wuguang_lines = [l for l in detail_ui.split("\n") if "无光" in l]
    ok = len(wuguang_lines) == 2 and all("0.9℃" in l for l in wuguang_lines)
    print(f"  {'PASS' if ok else 'FAIL'}  “无光”出现 {len(wuguang_lines)} 次且均在含“0.9℃”的规则文案内（Kotlin 常量 + 布局预览）")
    if not ok:
        for l in wuguang_lines:
            print("    " + l.strip()[:110])
        violations.append(f"A: “无光”出现位置异常（{len(wuguang_lines)} 处）")
    for inf in ("因下降", "因为温度", "由于温度", "上报原因", "推测", "触发上报的原因"):
        n = len(re.findall(inf, detail_ui))
        ok = n == 0
        print(f"  {'PASS' if ok else 'FAIL'}  无推断结论词「{inf}」= {n}")
        if not ok:
            violations.append(f"A: 出现推断结论词 {inf}")

    print("== B 图表范围 24 h ==")
    checks = {
        "B1 MAX_CHART_RANGE_HOURS = 24": re.search(r"const val MAX_CHART_RANGE_HOURS = 24\b", dp) is not None,
        "B2 MAX_CHART_RANGE_MS 由 HOUR_MS 派生":
            re.search(r"const val MAX_CHART_RANGE_MS = MAX_CHART_RANGE_HOURS \* HOUR_MS", dp) is not None
            and re.search(r"const val HOUR_MS = 3_600_000L", dp) is not None,
        "B3 详情页使用 MAX_CHART_RANGE_MS":
            "endTime - startTime > DetailPresentation.MAX_CHART_RANGE_MS" in frag,
        "B4 详情页无 49 * 60 / 50 分钟":
            ("49 * 60" not in frag) and ("50 分钟" not in frag),
        "B5 提示取自 rangeLimitMessage()": "Toast.makeText(context, DetailPresentation.rangeLimitMessage(), Toast.LENGTH_SHORT)" in frag,
    }
    for k, v in checks.items():
        print(f"  {'PASS' if v else 'FAIL'}  {k}")
        if not v:
            violations.append(f"B: {k}")
    base_frag = baseline(FRAG_REL)
    if base_frag:
        ok = ("49 * 60 * 1000" in base_frag) and ("50 分钟" in base_frag)
        print(f"  {'PASS' if ok else 'INFO'}  基线确实使用 49*60/50 分钟（证明本项已改）")
    # 全 main 不得再有“50 分钟”
    hits = []
    for base_, _d, files in os.walk(os.path.join(ROOT, APP)):
        for f in files:
            if f.endswith((".kt", ".xml")):
                p = os.path.join(base_, f)
                if "50 分钟" in open(p, encoding="utf-8", errors="ignore").read():
                    hits.append(os.path.relpath(p, ROOT).replace("\\", "/"))
    ok = not hits
    print(f"  {'PASS' if ok else 'FAIL'}  全 app/src/main 无“50 分钟”残留 = {hits}")
    if not ok:
        violations.append(f"B: 50 分钟残留 {hits}")

    print("== C 高温强调严格 > 35.0 ==")
    checks_c = {
        "C1 HIGH_TEMP_C = 35.0": re.search(r"const val HIGH_TEMP_C = 35\.0\b", dp) is not None,
        "C2 isHighTemperature 用严格大于": "fun isHighTemperature(temperatureC: Double): Boolean = temperatureC > HIGH_TEMP_C" in dp,
        "C3 详情页调用该函数": "DetailPresentation.isHighTemperature(" in frag,
        "C4 无 >= 35 写法": not re.search(r">=\s*DetailPresentation\.HIGH_TEMP_C|>=\s*35(\.0)?\b", frag + dp),
        "C5 高温后缀用于文本强调": "DetailPresentation.highTempSuffix(" in frag,
    }
    for k, v in checks_c.items():
        print(f"  {'PASS' if v else 'FAIL'}  {k}")
        if not v:
            violations.append(f"C: {k}")

    print("== D 不重算门控 / 无推断 ==")
    frag_wo_const = frag_code.replace("DetailPresentation.HIGH_TEMP_C", "")
    ok = ("0.9" not in frag_code) and ("35.0" not in frag_wo_const)
    print(f"  {'PASS' if ok else 'FAIL'}  详情页未内联门控常量（使用 DetailPresentation 常量）")
    if not ok:
        violations.append("D: 详情页内联门控常量")
    ok = "0.9" not in dp_code.replace("0.9℃", "")  # 0.9 只允许出现在规则文案里
    print(f"  {'PASS' if ok else 'FAIL'}  DetailPresentation 中 0.9 仅出现在规则文案")
    if not ok:
        violations.append("D: 0.9 出现在非文案处")

    print("== E 布局与只读性 ==")
    checks_e = {
        "E1 reportRuleText 存在": 'android:id="@+id/reportRuleText"' in layout,
        "E2 latestReadingText 存在": 'android:id="@+id/latestReadingText"' in layout,
        "E3 规则文本只读（未绑点击）": "reportRuleText.setOnClickListener" not in frag
            and "latestReadingText.setOnClickListener" not in frag,
        "E4 规则文本由常量赋值": "binding.reportRuleText.text = DetailPresentation.REPORT_RULE_TEXT" in frag,
    }
    for k, v in checks_e.items():
        print(f"  {'PASS' if v else 'FAIL'}  {k}")
        if not v:
            violations.append(f"E: {k}")

    print("== F 呈现层不参与判定 ==")
    other_refs = []
    for base_, _d, files in os.walk(os.path.join(ROOT, APP)):
        for f in files:
            if f.endswith(".kt") and f not in ("DetailPresentation.kt", "DeviceDetailFragment.kt"):
                p = os.path.join(base_, f)
                if "DetailPresentation" in open(p, encoding="utf-8", errors="ignore").read():
                    other_refs.append(os.path.relpath(p, ROOT).replace("\\", "/"))
    ok = not other_refs
    print(f"  {'PASS' if ok else 'FAIL'}  DetailPresentation 仅被详情页使用（其它引用={other_refs}）")
    if not ok:
        violations.append(f"F: DetailPresentation 被其它模块引用 {other_refs}")
    ok = "HIGH_TEMP_C" not in read(f"{APP}/java/com/jinyuni/dengbei_care/MqtttService.kt")
    print(f"  {'PASS' if ok else 'FAIL'}  上传/告警主链路未使用 DetailPresentation 的呈现阈值")
    if not ok:
        violations.append("F: 主链路引用了呈现阈值")

    print("== RESULT ==")
    if violations:
        for v in violations:
            print("  VIOLATION:", v)
        return 1
    print("  OK: 规则文案一致且无光照/推断信息、范围 24h、高温强调严格 >35.0、只读呈现且不参与判定")
    return 0


if __name__ == "__main__":
    sys.exit(main())
