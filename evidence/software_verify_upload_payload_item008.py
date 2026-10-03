#!/usr/bin/env python3
"""独立静态验证（software_tester.software_verification / ITEM-008；TD-SW-002 T-SW-L0-11 / T-SW-L0d-07）。

  A 既有端点/数据类未被修改：把改动前提交的 `ApiService.kt` 逐行作为**有序子序列**在现文件中断言全部存在
    （任何既有行被删/改都会失败）；并核对端点清单恰为 5 个旧端点 + `/upload_data`，每个均为
    `suspend fun ... : Response<...>`。
  B 新增 DTO 字段名/类型（从源码文本抽取，独立于反射）。
  C **单位只换算一次**：线上 ×10 → ℃/%RH 的除法只允许出现在 `GatewayFrameCodec`（2 处，
    温度与湿度各一）；`ReadingAttribution`、`UploadPayload`、`MqtttService` 中不得再出现 `/10.0`、
    `* 0.1`、`/ 10` 等二次缩放。
  D `UploadPayload` 为纯逻辑且空集不产生请求体：不得读写 SharedPreferences/网络；两个入口均有空集返回 null。

运行：`python evidence/software_verify_upload_payload_item008.py`；退出码 0/1。
"""

from __future__ import annotations

import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APP = "dengbei_care/app/src/main/java/com/jinyuni/dengbei_care"
API_REL = f"{APP}/ui/zhuce/ApiService.kt"
PAYLOAD_REL = f"{APP}/cloud/UploadPayload.kt"
CODEC_REL = f"{APP}/protocol/GatewayFrameCodec.kt"
ATTRIB_REL = f"{APP}/telemetry/ReadingAttribution.kt"
MQTT_REL = f"{APP}/MqtttService.kt"
BASELINE_COMMIT = "cec8e01"  # 本项实现前的提交

violations: list[str] = []


def read(rel):
    with open(os.path.join(ROOT, rel), encoding="utf-8") as fh:
        return fh.read().replace("\r\n", "\n")


def baseline(rel):
    out = subprocess.run(["git", "show", f"{BASELINE_COMMIT}:{rel}"], cwd=ROOT, capture_output=True, text=True)
    return out.stdout.replace("\r\n", "\n") if out.returncode == 0 else None


def check_api():
    print("== A 既有端点/数据类未被修改 ==")
    base = baseline(API_REL)
    cur = read(API_REL)
    if base is None:
        print("  FAIL  无法读取基线提交")
        violations.append("A: baseline unreadable")
        return
    base_lines = [l for l in base.split("\n")]
    cur_lines = cur.split("\n")
    i, missing = 0, []
    for ln in base_lines:
        try:
            i = cur_lines.index(ln, i) + 1
        except ValueError:
            missing.append(ln)
    ok = not missing
    print(f"  {'PASS' if ok else 'FAIL'}  基线 {len(base_lines)} 行全部按序保留（缺失/被改={len(missing)}）")
    for m in missing[:5]:
        print("    MISSING:", repr(m))
    if not ok:
        violations.append(f"A: 基线行缺失/被改 {missing[:3]}")

    paths = re.findall(r'@POST\("([^"]+)"\)\s*\n\s*suspend fun (\w+)\(', cur)
    expected = [
        ("/sendVerificationCode", "sendVerificationCode"),
        ("/verifyCode", "verifyCode"),
        ("/register", "register"),
        ("/login", "login"),
        ("/setmac", "setMacAddress"),
        ("/upload_data", "uploadData"),
    ]
    ok = paths == expected
    print(f"  {'PASS' if ok else 'FAIL'}  端点清单 = {paths}")
    if not ok:
        violations.append(f"A: 端点清单异常 {paths}")
    suspend_returns = re.findall(r"suspend fun \w+\([^)]*\): (Response<[^>]+>)", cur, re.S)
    ok = len(suspend_returns) == 6 and all(r.startswith("Response<") for r in suspend_returns)
    print(f"  {'PASS' if ok else 'FAIL'}  6 个端点均返回 Response（{suspend_returns}）")
    if not ok:
        violations.append("A: 返回类型异常")


def check_dto():
    print("== B 新增 DTO 字段名/类型 ==")
    cur = read(API_REL)
    m = re.search(r"data class SensorReading\((.*?)\)", cur, re.S)
    fields = re.findall(r"val (\w+): ([\w<>?]+)", m.group(1)) if m else []
    ok = fields == [("devId", "String"), ("time", "Long"), ("temperature", "Double"), ("humidity", "Double")]
    print(f"  {'PASS' if ok else 'FAIL'}  SensorReading = {fields}")
    if not ok:
        violations.append(f"B: SensorReading 字段异常 {fields}")
    m2 = re.search(r"data class SensorUploadData\((.*?)\)", cur, re.S)
    fields2 = re.findall(r"val (\w+): ([\w<>?]+)", m2.group(1)) if m2 else []
    ok = fields2 == [("phone", "String"), ("mac", "String"), ("readings", "List<SensorReading>")]
    print(f"  {'PASS' if ok else 'FAIL'}  SensorUploadData = {fields2}")
    if not ok:
        violations.append(f"B: SensorUploadData 字段异常 {fields2}")


def strip_comments(text):
    """去掉整行注释与行尾 `//` 注释（保留代码）。"""
    out = []
    for line in text.split("\n"):
        s = line.strip()
        if s.startswith("//") or s.startswith("*") or s.startswith("/*"):
            continue
        out.append(line.split("//")[0])
    return "\n".join(out)


def count_conversions(text):
    return len(re.findall(r"/\s*10\.0|/\s*10\b|\*\s*0\.1\b", strip_comments(text)))


def check_single_conversion():
    print("== C 单位只换算一次（×10 → ℃/%RH） ==")
    counts = {name: count_conversions(read(rel)) for name, rel in (
        ("GatewayFrameCodec", CODEC_REL), ("ReadingAttribution", ATTRIB_REL),
        ("UploadPayload", PAYLOAD_REL), ("MqtttService(全部非注释)", MQTT_REL))}
    ok = counts["GatewayFrameCodec"] == 2 and counts["ReadingAttribution"] == 0 and counts["UploadPayload"] == 0
    print(f"  {'PASS' if ok else 'FAIL'}  非注释换算计数 = {counts}（期望 GatewayFrameCodec=2、ReadingAttribution=0、UploadPayload=0）")
    if not ok:
        violations.append(f"C: 换算次数异常 {counts}")

    # MqtttService 中仅允许出现于**无调用点**的旧解析族 interpretPayloadV3（本项范围外，已登记）
    mqtt = read(MQTT_REL)
    m = re.search(r"fun interpretPayloadV3\(.*?\n    \}", mqtt, re.S)
    dead_body = m.group(0) if m else ""
    dead_count = count_conversions(dead_body)
    live_count = counts["MqtttService(全部非注释)"] - dead_count
    callers = len(re.findall(r"interpretPayloadV3\(", strip_comments(mqtt))) - 0
    def_count = len(re.findall(r"fun interpretPayloadV3\(", mqtt))
    ok = dead_count == 2 and live_count == 0 and callers == def_count
    print(f"  {'PASS' if ok else 'FAIL'}  MqtttService: 旧解析族内换算={dead_count}、存活路径换算={live_count}、interpretPayloadV3 调用点={callers - def_count}（无调用点）")
    if not ok:
        violations.append(f"C: 存活路径仍存在换算 dead={dead_count} live={live_count} calls={callers - def_count}")


def check_payload_pure():
    print("== D UploadPayload 纯逻辑与空集门控 ==")
    text = read(PAYLOAD_REL)
    forbidden = [k for k in ("getSharedPreferences", "Retrofit", "OkHttp", "HttpURLConnection", "writableDatabase") if k in text]
    ok = not forbidden
    print(f"  {'PASS' if ok else 'FAIL'}  无副作用依赖（命中={forbidden}）")
    if not ok:
        violations.append(f"D: UploadPayload 含副作用依赖 {forbidden}")
    guards = len(re.findall(r"if \(readings\.isEmpty\(\)\) return null", text))
    ok = guards == 2
    print(f"  {'PASS' if ok else 'FAIL'}  两个入口均有空集返回 null（{guards} 处）")
    if not ok:
        violations.append(f"D: 空集门控数量 {guards}")


def main() -> int:
    check_api()
    check_dto()
    check_single_conversion()
    check_payload_pure()
    print("== RESULT ==")
    if violations:
        for v in violations:
            print("  VIOLATION:", v)
        return 1
    print("  OK: 既有端点/数据类未变、新 DTO 契约正确、单位仅一次换算、UploadPayload 纯逻辑且空集不请求")
    return 0


if __name__ == "__main__":
    sys.exit(main())
