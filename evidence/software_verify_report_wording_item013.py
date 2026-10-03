#!/usr/bin/env python3
"""独立静态验证（software_tester.software_verification / ITEM-013；TD-SW-002 T-SW-L3-07）。

  A 文案：报告相关文件不得再出现“无数据”/“288”/“共采集”/“离线”/“每 3 分钟”/“预期”；
    必须出现“昨日无上报”“按条件上报”“条数非等间隔”“不一定异常”。
  B 统计结构与阈值不变（与基线逐字比对）：采样/温度/湿度/报警 行格式、标题、建议阈值 1/3/5/24、报警类型常量。
  C `DailyReportWorker.kt` 未改动（与基线逐字一致）——文案随 `ReportGenerator` 生效，查询与通知结构未动。
  D `DailyStats` 字段与 `getDailyStats` SQL 未改动（与基线逐字比对）。
  E 基线确实使用旧文案（证明本项已改）。
  F `activity_report.xml` 仅 `tools:text` 预览行变化。

运行：`python evidence/software_verify_report_wording_item013.py`；退出码 0/1。
"""

from __future__ import annotations

import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APP = "dengbei_care/app/src/main"
RG_REL = f"{APP}/java/com/jinyuni/dengbei_care/report/ReportGenerator.kt"
WORKER_REL = f"{APP}/java/com/jinyuni/dengbei_care/report/DailyReportWorker.kt"
XML_REL = f"{APP}/res/layout/activity_report.xml"
DB_REL = f"{APP}/java/com/jinyuni/dengbei_care/TemperatureDatabaseHelper.kt"
BASELINE = "7e168bd"  # 本项实现前的提交

violations: list[str] = []


def read(rel):
    with open(os.path.join(ROOT, rel), encoding="utf-8") as fh:
        return fh.read().replace("\r\n", "\n")


def baseline(rel):
    out = subprocess.run(["git", "show", f"{BASELINE}:{rel}"], cwd=ROOT, capture_output=True, text=True)
    return out.stdout.replace("\r\n", "\n") if out.returncode == 0 else None


def strip_comments(text: str) -> str:
    """去掉整行注释与行尾 // 注释，只保留代码（含字符串字面量）。"""
    out = []
    for line in text.splitlines():
        t = line.strip()
        if t.startswith(("//", "*", "/*", "<!--")):
            continue
        out.append(line.split("//")[0])
    return "\n".join(out)


def main() -> int:
    rg = read(RG_REL)
    worker = read(WORKER_REL)
    xml = read(XML_REL)

    print("== A 新文案与禁止位 ==")
    # 禁止位只看用户可见文本（字符串字面量/布局），注释中的历史说明不计
    report_text = strip_comments(rg) + "\n" + strip_comments(worker) + "\n" + xml
    for bad in ("无数据", "288", "共采集", "离线", "每 3 分钟", "预期"):
        n = len(re.findall(re.escape(bad), report_text))
        ok = n == 0
        print(f"  {'PASS' if ok else 'FAIL'}  不得出现「{bad}」→ 命中 {n}")
        if not ok:
            violations.append(f"A: 报告文件仍含 {bad}")
    for good in ("昨日无上报", "按条件上报", "条数非等间隔", "不一定异常"):
        ok = good in rg
        print(f"  {'PASS' if ok else 'FAIL'}  必须包含「{good}」")
        if not ok:
            violations.append(f"A: 缺少 {good}")

    print("== B 统计结构/阈值与基线逐字一致 ==")
    base_rg = baseline(RG_REL)
    if base_rg is None:
        violations.append("B: 无法读取基线 ReportGenerator")
    else:
        keep_patterns = [
            r'sb\.appendLine\("  - 采样 \$\{s\.sampleCount\} 条"\)',
            r'sb\.appendLine\("  - 温度:最低 \$\{fmt\(s\.tempMin\)\}°C,最高 \$\{fmt\(s\.tempMax\)\}°C,平均 \$\{fmt\(s\.tempAvg\)\}°C"\)',
            r'sb\.appendLine\("  - 湿度:最低 \$\{fmt\(s\.humiMin\)\}%,最高 \$\{fmt\(s\.humiMax\)\}%,平均 \$\{fmt\(s\.humiAvg\)\}%"\)',
            r'sb\.appendLine\("  - 报警 \$\{section\.alarms\.size\} 次"\)',
            r'val title = "爱守护 - 早起报告"|title = "爱守护 - 早起报告"',
            r's\.sampleCount < 24',
            r'dropCount >= 3',
            r'thresholdCount >= 5',
            r'emergencyCount > 0',
        ]
        for pat in keep_patterns:
            cur_has = re.search(pat, rg) is not None
            base_has = re.search(pat, base_rg) is not None
            ok = cur_has and base_has
            print(f"  {'PASS' if ok else 'FAIL'}  保留：{pat[:60]}…（当前={cur_has}, 基线={base_has}）")
            if not ok:
                violations.append(f"B: 结构与基线不一致 {pat[:40]}")
        # ALARM_TYPE 映射建议段未变
        for t in ("ALARM_TYPE_EMERGENCY", "ALARM_TYPE_DROP", "ALARM_TYPE_THRESHOLD"):
            ok = t in rg and t in base_rg
            print(f"  {'PASS' if ok else 'FAIL'}  报警类型常量保留 {t}")
            if not ok:
                violations.append(f"B: 缺少 {t}")

    print("== C DailyReportWorker 未改动 ==")
    base_worker = baseline(WORKER_REL)
    ok = base_worker is not None and worker.strip() == base_worker.strip()
    print(f"  {'PASS' if ok else 'FAIL'}  DailyReportWorker.kt 与基线逐字一致")
    if not ok:
        violations.append("C: DailyReportWorker 被改动")
    ok = "ReportGenerator.build(" in worker and ".toText()" in worker
    print(f"  {'PASS' if ok else 'FAIL'}  worker 仍调用 ReportGenerator.build(...).toText()（文案随生成器生效）")
    if not ok:
        violations.append("C: worker 调用链异常")

    print("== D DailyStats / getDailyStats SQL 未改动 ==")
    db = read(DB_REL)
    base_db = baseline(DB_REL)
    m = re.search(r"data class DailyStats\((.*?)\)", db, re.S)
    mb = re.search(r"data class DailyStats\((.*?)\)", base_db, re.S) if base_db else None
    ok = m is not None and mb is not None and m.group(1) == mb.group(1)
    print(f"  {'PASS' if ok else 'FAIL'}  DailyStats 字段与基线一致")
    if not ok:
        violations.append("D: DailyStats 被改动")
    q = re.search(r"fun getDailyStats\(.*?val query = ((?:\s*\"[^\"]*\"\s*\+?)+)", db, re.S)
    qb = re.search(r"fun getDailyStats\(.*?val query = ((?:\s*\"[^\"]*\"\s*\+?)+)", base_db, re.S) if base_db else None
    sql = "".join(re.findall(r'"([^"]*)"', q.group(1))) if q else None
    sqlb = "".join(re.findall(r'"([^"]*)"', qb.group(1))) if qb else None
    ok = sql is not None and sql == sqlb
    print(f"  {'PASS' if ok else 'FAIL'}  getDailyStats SQL 与基线一致（MIN/MAX/AVG/COUNT 结构不变）")
    if not ok:
        violations.append(f"D: getDailyStats SQL 变化 {sqlb!r} -> {sql!r}")

    print("== E 基线旧文案（证明已改） ==")
    for old in ("无数据", "共采集", "288"):
        ok = base_rg is not None and old in base_rg
        print(f"  {'PASS' if ok else 'INFO'}  基线含旧文案「{old}」")
        if not ok:
            violations.append(f"E: 基线未含 {old}")

    print("== F activity_report.xml 仅 tools:text 变化 ==")
    base_xml = baseline(XML_REL)
    if base_xml:
        strip_tools = lambda t: "\n".join(l for l in t.split("\n") if "tools:text" not in l)
        ok = strip_tools(xml) == strip_tools(base_xml)
        print(f"  {'PASS' if ok else 'FAIL'}  除 tools:text 预览行外与基线一致")
        if not ok:
            violations.append("F: activity_report.xml 有非预览改动")
    else:
        print("  INFO  无法读取基线布局")

    print("== RESULT ==")
    if violations:
        for v in violations:
            print("  VIOLATION:", v)
        return 1
    print("  OK: 新文案到位、旧文案清零、统计结构与阈值及 DailyReportWorker/DailyStats/SQL 均未改动")
    return 0


if __name__ == "__main__":
    sys.exit(main())
