#!/usr/bin/env python3
"""独立验证（software_tester.software_verification / ITEM-014；TD-SW-002 T-SW-L1-09）。

  A 版本递增（源码 + 构建产物三级核实）：`build.gradle.kts` 为 9 / "1.8" 且严格大于基线（8 / "1.7"）；
    AGP 产物 `output-metadata.json`、合并清单 `AndroidManifest.xml`、APK 二进制清单（AXML 字符串池）一致。
  B 文档与实现一致（**交叉核对**，文档说法 vs 代码事实）：3 分钟采样、条件上报（0.9℃/无光/>35.0℃）、
    卡片不再“离线”、下降报警 5/10/15 分钟时间窗、>65℃ 与 900 s、24 小时图表、>35.0℃ 高温强调、
    云同步（/upload_data、待发箱、幂等、守护/手机号门控）、服务器地址、待发箱表名、早报“昨日无上报/非等间隔”。
  C 旧口径清零：文档不得再出现 50 分钟、1.7 版本、不上传云端、固定节拍（每 3 分钟上报/一条、288）；
    “离线”只允许出现在否定/说明语境（不再显示/不能/不代表）。
  D `file_manifest.txt` 维护：本项涉及文件条目已更新；整条工作流的全部产品变更文件均在清单中
    （精确路径或粗粒度父目录条目）。

运行：`python evidence/software_verify_release_item014.py`；退出码 0/1。
"""

from __future__ import annotations

import os
import re
import subprocess
import sys
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APP = "dengbei_care/app/src/main"
GRADLE_REL = "dengbei_care/app/build.gradle.kts"
README_REL = "dengbei_care/readme.txt"
MANUAL_REL = "dengbei_care/用户使用说明书.md"
MANIFEST_REL = "file_manifest.txt"
BASELINE = "ccc59d9"      # 本项实现前的提交
FLOW_START = "fe9a81c"    # 本工作流开始前（软件测试设计提交）

violations: list[str] = []


def read(rel):
    with open(os.path.join(ROOT, rel), encoding="utf-8") as fh:
        return fh.read().replace("\r\n", "\n")


def baseline(rel):
    out = subprocess.run(["git", "show", f"{BASELINE}:{rel}"], cwd=ROOT, capture_output=True, text=True)
    return out.stdout.replace("\r\n", "\n") if out.returncode == 0 else None


def code(rel):
    """源码常量抽取结果（注释不计）。"""
    text = read(rel)
    return "\n".join(
        l.split("//")[0] for l in text.splitlines()
        if not l.strip().startswith(("//", "*", "/*"))
    )


def main() -> int:
    gradle = read(GRADLE_REL)
    readme = read(README_REL)
    manual = read(MANUAL_REL)
    manifest = read(MANIFEST_REL)
    docs = readme + "\n" + manual

    print("== A 版本递增（源码 + 产物） ==")
    m_vc = re.search(r"versionCode = (\d+)", gradle)
    m_vn = re.search(r'versionName = "([^"]+)"', gradle)
    base = baseline(GRADLE_REL)
    b_vc = re.search(r"versionCode = (\d+)", base) if base else None
    b_vn = re.search(r'versionName = "([^"]+)"', base) if base else None
    ok = m_vc and m_vn and b_vc and b_vn and int(m_vc.group(1)) > int(b_vc.group(1)) and m_vn.group(1) != b_vn.group(1)
    print(f"  {'PASS' if ok else 'FAIL'}  build.gradle.kts: {b_vc.group(1) if b_vc else '?'}/{b_vn.group(1) if b_vn else '?'} → {m_vc.group(1) if m_vc else '?'}/{m_vn.group(1) if m_vn else '?'}")
    if not ok:
        violations.append("A: 版本未严格递增")

    meta_path = os.path.join(ROOT, "dengbei_care/app/build/outputs/apk/debug/output-metadata.json")
    merged_path = os.path.join(ROOT, "dengbei_care/app/build/intermediates/merged_manifests/debug/processDebugManifest/AndroidManifest.xml")
    if os.path.exists(meta_path):
        meta = read("dengbei_care/app/build/outputs/apk/debug/output-metadata.json")
        ok = '"versionCode": 9' in meta and '"versionName": "1.8"' in meta
        print(f"  {'PASS' if ok else 'FAIL'}  AGP output-metadata.json: versionCode 9 / versionName 1.8")
        if not ok:
            violations.append("A: output-metadata 版本不符")
    else:
        violations.append("A: 缺少 output-metadata.json（未构建）")
    if os.path.exists(merged_path):
        merged = open(merged_path, encoding="utf-8", errors="ignore").read()
        ok = 'versionCode="9"' in merged and 'versionName="1.8"' in merged
        print(f"  {'PASS' if ok else 'FAIL'}  合并清单: versionCode=\"9\" / versionName=\"1.8\"")
        if not ok:
            violations.append("A: 合并清单版本不符")
    else:
        violations.append("A: 缺少合并清单（未构建）")
    apk = os.path.join(ROOT, "dengbei_care/app/build/outputs/apk/debug/app-debug.apk")
    if os.path.exists(apk):
        with zipfile.ZipFile(apk) as z:
            axml = z.read("AndroidManifest.xml")
        has18 = "1.8".encode("utf-16-le") in axml
        has17 = "1.7".encode("utf-16-le") in axml
        ok = has18 and not has17
        print(f"  {'PASS' if ok else 'FAIL'}  APK 二进制清单字符串池：含 1.8={has18}，含 1.7={has17}")
        if not ok:
            violations.append("A: APK 清单版本字符串不符")
    else:
        violations.append("A: 缺少 APK（未构建）")

    print("== B 文档与实现一致（交叉核对） ==")
    cadence = code(f"{APP}/java/com/jinyuni/dengbei_care/telemetry/SensorCadence.kt")
    dp = code(f"{APP}/ui/detail/DetailPresentation.kt".replace(f"{APP}", f"{APP}/java/com/jinyuni/dengbei_care") if False else f"{APP}/java/com/jinyuni/dengbei_care/ui/detail/DetailPresentation.kt")
    alarm = code(f"{APP}/java/com/jinyuni/dengbei_care/telemetry/AlarmEvaluator.kt")
    cloud = code(f"{APP}/java/com/jinyuni/dengbei_care/cloud/CloudConfig.kt")
    outbox = code(f"{APP}/java/com/jinyuni/dengbei_care/cloud/UploadOutbox.kt")
    repo = code(f"{APP}/java/com/jinyuni/dengbei_care/cloud/CloudUploadRepository.kt")
    helper = code(f"{APP}/java/com/jinyuni/dengbei_care/TemperatureDatabaseHelper.kt")
    reports = code(f"{APP}/java/com/jinyuni/dengbei_care/report/ReportGenerator.kt")
    ds = code(f"{APP}/java/com/jinyuni/dengbei_care/DeviceState.kt")

    # 代码事实
    facts = {
        "采样 180s": "SAMPLE_PERIOD_SECONDS" in cadence and "180" in cadence,
        "规则 0.9/35.0": "0.9℃" in dp and "35.0℃" in dp,
        "范围 24h": "MAX_CHART_RANGE_HOURS = 24" in dp,
        "高温阈值 35.0": "HIGH_TEMP_C = 35.0" in dp,
        "下降窗口 5/10/15": "listOf(5, 10, 15)" in alarm,
        "紧急 65.0": "EMERGENCY_TEMP_C = 65.0" in alarm,
        "新服务器": 'const val MQTT_BROKER_URI = "ssl://$SERVER_HOST:$MQTT_BROKER_PORT"' in read(f"{APP}/java/com/jinyuni/dengbei_care/cloud/CloudConfig.kt") and 'const val SERVER_HOST = "8.140.23.253"' in read(f"{APP}/java/com/jinyuni/dengbei_care/cloud/CloudConfig.kt"),
        "待发箱表名": "pending_uploads" in outbox or "TABLE_PENDING_UPLOADS" in outbox,
        "DB v3": "DATABASE_VERSION = 3" in helper,
        "上传门控": "gate.guardActive() && gate.phone().isNotBlank()" in repo,
        "900s 节流": "(900 * 1000)" in code(f"{APP}/java/com/jinyuni/dengbei_care/MqtttService.kt"),
        "早报文案": "昨日无上报" in reports and "条数非等间隔" in reports,
        "卡片相对时间": "lastReportAgeLabel" in ds,
    }
    for k, v in facts.items():
        print(f"  {'PASS' if v else 'FAIL'}  代码事实：{k}")
        if not v:
            violations.append(f"B: 代码事实缺失 {k}")

    doc_checks = {
        "readme 记录 V1.8 条目": "升级为1.8" in readme,
        "readme 说明 3 分钟采样": "每 3 分钟采样" in readme,
        "readme 说明条件上报（0.9/无光/35）": "0.9℃" in readme and "无光" in readme and "35.0℃" in readme,
        "readme 说明卡片不再离线": "不再" in readme and "离线" in readme,
        "readme 说明云同步与幂等": "/upload_data" in readme and "幂等" in readme and "待发箱" in readme,
        "readme 说明新服务器地址": "8.140.23.253" in readme and "8883" in readme and "5000" in readme,
        "readme 说明早报文案": "昨日无上报" in readme and "非等间隔" in readme,
        "说明书版本 1.8": "版本:1.8" in manual or "版本：1.8" in manual,
        "说明书 24 小时": "24 小时" in manual,
        "说明书 卡片不再离线 + 最近上报": "不再显示「离线」" in manual and "最近上报" in manual,
        "说明书 详情规则说明": "0.9℃" in manual and "35.0℃" in manual,
        "说明书 下降报警时间窗": "5/10/15 分钟" in manual,
        "说明书 云同步章节": "数据上传（云端同步）" in manual and "待发箱" in manual and "sensor_data" in manual,
        "说明书 pending_uploads": "pending_uploads" in manual,
        "说明书 关守护不上传": "守护关闭" in manual and "不上传" in manual,
        "说明书 早报昨日无上报": "昨日无上报" in manual,
    }
    for k, v in doc_checks.items():
        print(f"  {'PASS' if v else 'FAIL'}  文档：{k}")
        if not v:
            violations.append(f"B: 文档缺少 {k}")

    print("== C 旧口径清零 ==")
    for bad in ("版本:1.7", "版本：1.7", "不上传到云端"):
        n = len(re.findall(re.escape(bad), docs))
        ok = n == 0
        print(f"  {'PASS' if ok else 'FAIL'}  文档不得出现「{bad}」→ {n}")
        if not ok:
            violations.append(f"C: 文档仍含 {bad}")
    # “50 分钟”只允许出现在变更记录（由 50 分钟放宽到 24 小时）这类历史语境
    bad50 = []
    for name, text in (("readme.txt", readme), ("用户使用说明书.md", manual)):
        for i, line in enumerate(text.splitlines(), 1):
            if "50 分钟" in line and "放宽" not in line and "由" not in line:
                bad50.append(f"{name}:{i}: {line.strip()[:80]}")
    ok = not bad50
    print(f"  {'PASS' if ok else 'FAIL'}  「50 分钟」仅出现在历史变更语境（越界 {len(bad50)} 处）")
    for b in bad50[:5]:
        print("    " + b)
    if not ok:
        violations.append(f"C: 50 分钟用于当前口径 {bad50[:2]}")
    for bad in ("每 3 分钟上报", "每 3 分钟上传", "每 3 分钟一条", "288"):
        n = len(re.findall(re.escape(bad), docs))
        ok = n == 0
        print(f"  {'PASS' if ok else 'FAIL'}  文档不得暗示固定节拍「{bad}」→ {n}")
        if not ok:
            violations.append(f"C: 文档含固定节拍说法 {bad}")
    # 允许“每 3 分钟采样”，但不得用于上报/上传
    for m in re.finditer("每 3 分钟", docs):
        tail = docs[m.end():m.end() + 10]
        ok = "采样" in tail
        if not ok:
            violations.append(f"C: 「每 3 分钟」未接“采样”：{tail!r}")
    print(f"  {'PASS' if not any('每 3 分钟' in v for v in violations) else 'FAIL'}  「每 3 分钟」仅用于“采样”语境")
    # 离线只允许出现在否定/说明语境
    bad_offline = []
    for text, name in ((readme, "readme.txt"), (manual, "用户使用说明书.md")):
        for i, line in enumerate(text.splitlines(), 1):
            if "离线" in line and not any(k in line for k in ("不再", "不显示", "不能", "不代表", "不看")):
                bad_offline.append(f"{name}:{i}: {line.strip()[:80]}")
    ok = not bad_offline
    print(f"  {'PASS' if ok else 'FAIL'}  “离线”仅在否定/说明语境（越界 {len(bad_offline)} 处）")
    for b in bad_offline[:5]:
        print("    " + b)
    if not ok:
        violations.append(f"C: “离线”用于肯定语境 {bad_offline[:2]}")

    print("== D file_manifest.txt 维护 ==")
    for rel, need in ((GRADLE_REL, "versionCode"), (README_REL, "1.8"), (MANUAL_REL, "1.8")):
        entry = [l for l in manifest.splitlines() if l.startswith(rel + " |")]
        ok = bool(entry) and any(need in l or (need == "versionCode" and "versionName" in l) for l in entry)
        print(f"  {'PASS' if ok else 'FAIL'}  清单条目已更新：{rel}（含「{need}」）")
        if not ok:
            violations.append(f"D: 清单条目未更新 {rel}")

    changed = subprocess.run(
        ["git", "-c", "core.quotepath=false", "diff", "--name-only", f"{FLOW_START}", "HEAD"], cwd=ROOT, capture_output=True, text=True
    ).stdout.split()
    product = [c for c in changed if not c.startswith(("artifacts/", "evidence/", "file_manifest"))]
    manifest_lines = set(l.split(" | ")[0] for l in manifest.splitlines() if " | " in l)
    missing = []
    for p in product:
        if p in manifest_lines:
            continue
        # 允许粗粒度父目录条目（以 / 结尾）
        covers = [m for m in manifest_lines if m.endswith("/") and p.startswith(m)]
        if not covers:
            missing.append(p)
    ok = not missing
    print(f"  {'PASS' if ok else 'FAIL'}  工作流全部产品变更文件（{len(product)} 个）均在清单中（缺失 {len(missing)}）")
    for m_ in missing[:8]:
        print("    MISSING:", m_)
    if not ok:
        violations.append(f"D: 清单缺少 {missing[:5]}")

    print("== D3 本项新增/修改的清单条目路径必须真实存在 ==")
    diff_out = subprocess.run(
        ["git", "-c", "core.quotepath=false", "diff", f"{BASELINE}", "HEAD", "--", MANIFEST_REL],
        cwd=ROOT, capture_output=True, text=True,
    ).stdout
    added_lines = [l[1:] for l in diff_out.splitlines() if l.startswith("+") and not l.startswith("+++")]
    bogus = []
    for line in added_lines:
        parts = line.split(" | ")
        if len(parts) < 2:
            continue
        path, status = parts[0], parts[1]
        if status != "active":
            continue
        if path.endswith("/") or "{" in path or "*" in path:
            continue
        if not os.path.exists(os.path.join(ROOT, path)):
            bogus.append(path)
    ok = not bogus
    print(f"  {'PASS' if ok else 'FAIL'}  本项改动清单条目中路径不存在的 active 条目 = {len(bogus)}")
    for b in bogus:
        print("    BOGUS:", b)
    if not ok:
        violations.append(f"D: file_manifest 条目路径不存在 {bogus}")

    print("== RESULT ==")
    if violations:
        for v in violations:
            print("  VIOLATION:", v)
        return 1
    print("  OK: 版本递增已落地到产物；文档与实现交叉一致且旧口径清零；file_manifest 完整")
    return 0


if __name__ == "__main__":
    sys.exit(main())
