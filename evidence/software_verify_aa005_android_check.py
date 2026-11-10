#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
软件测试独立静态核查（software_tester.software_verification / AA-005 七项增量）。

只读 `dengbei_care/app/src/main`、构建脚本、文档与 `file_manifest.txt`；不访问设备、
不修改产品源码、不依赖实现侧自检脚本。退出码 0=全部通过，1=任一检查失败。

覆盖 TD-SW-004 §4.3 的 L1 项（T-SW-L1-25/26/27/28/29/30/31/32/33）中可在源码层
独立断言的要点，以及本轮实测发现的“上传阻塞未全局生效”的静态证据。
"""
import hashlib
import json
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
APP = os.path.join(ROOT, "dengbei_care", "app")
MAIN = os.path.join(APP, "src", "main")
JAVA = os.path.join(MAIN, "java", "com", "jinyuni", "dengbei_care")
RES = os.path.join(MAIN, "res")
KOTLIN = os.path.join(MAIN, "java")
DOC_README = os.path.join(ROOT, "dengbei_care", "readme.txt")
DOC_MANUAL = os.path.join(ROOT, "dengbei_care", "用户使用说明书.md")
MANIFEST = os.path.join(ROOT, "file_manifest.txt")

results = []
findings = []


def ok(cid, msg):
    results.append((True, cid, msg))


def bad(cid, msg):
    results.append((False, cid, msg))


def find(cid, msg):
    findings.append((cid, msg))


def read(path):
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        return f.read()


def walk_kt(base):
    out = []
    for dirpath, _dirs, files in os.walk(base):
        for name in files:
            if name.endswith((".kt", ".java")):
                out.append(os.path.join(dirpath, name))
    return sorted(out)


def collect(pattern, files=None, flags=0):
    rx = re.compile(pattern, flags)
    hits = []
    for path in (files or walk_kt(KOTLIN)):
        text = read(path)
        for m in rx.finditer(text):
            line = text.count("\n", 0, m.start()) + 1
            hits.append((os.path.relpath(path, ROOT), line, m.group(0)))
    return hits


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    return text


# ---------------- A. G-1 权限模型 ----------------
mq = read(os.path.join(JAVA, "MqtttService.kt"))
main_act = read(os.path.join(JAVA, "MainActivity.kt"))
dialog = read(os.path.join(JAVA, "ui", "gateway", "GatewayBleSettingsDialog.kt"))
policy = read(os.path.join(JAVA, "ble", "BlePermissionPolicy.kt"))
manifest_xml = read(os.path.join(MAIN, "AndroidManifest.xml"))

if "Bluetooth permission not granted" not in mq:
    ok("A1", "MqtttService 不再有“仅一行 Bluetooth permission not granted 后返回”的路径")
else:
    bad("A1", "MqtttService 仍存在旧的静默权限返回路径")

if "private fun checkBluetoothPermission(): Boolean = evaluatePermission(needScan = false).canConnect" in mq:
    ok("A2", "checkBluetoothPermission 直接使用权限策略（needScan=false）")
else:
    bad("A2", "checkBluetoothPermission 未使用统一权限策略")

if "private fun isBluetoothBusy(): Boolean {\n        if (!checkBluetoothPermission()) return false" in mq:
    ok("A3", "isBluetoothBusy 在权限缺失时返回 false（不再伪装为 busy）")
else:
    bad("A3", "isBluetoothBusy 的权限缺失分支不符合预期")

if "private suspend fun connectToBluetoothDevice() {\n        val policy = evaluatePermission(needScan = false)" in mq:
    ok("A4", "connectToBluetoothDevice 使用同一策略")
else:
    bad("A4", "connectToBluetoothDevice 未使用统一策略")

for token in ("sdkInt=$", "missing=$", "needScan=false adapterEnabled="):
    if token in mq:
        ok("A5:" + token.strip(), "权限失败日志包含 " + token.strip())
    else:
        bad("A5:" + token.strip(), "权限失败日志缺少 " + token.strip())

if "connectRequestPermissions(Build.VERSION.SDK_INT, granted)" in main_act and \
        "needsNotificationPermission(Build.VERSION.SDK_INT, granted)" in main_act:
    ok("A6", "MainActivity 启动权限请求使用 connectRequestPermissions + 通知权限判定")
else:
    bad("A6", "MainActivity 启动权限请求未按 API 使用策略")

main_act_code = strip_comments(main_act)
fine_lines = [ln.strip() for ln in main_act_code.splitlines() if "ACCESS_FINE_LOCATION" in ln]
allowed_fine = {
    "if (has(Manifest.permission.ACCESS_FINE_LOCATION)) granted += PermissionNeed.FINE_LOCATION",
    "PermissionNeed.FINE_LOCATION -> Manifest.permission.ACCESS_FINE_LOCATION",
}
unexpected = [ln for ln in fine_lines if ln not in allowed_fine]
if not unexpected:
    ok("A7", "MainActivity 不在启动路径直接请求 ACCESS_FINE_LOCATION"
             "（仅收集已授予状态与枚举映射）")
else:
    bad("A7", "MainActivity 存在意外的 ACCESS_FINE_LOCATION 用法: %s" % unexpected)

if "scanRequestPermissions(" in dialog and "needScan = true" in dialog:
    ok("A8", "网关设置页扫描权限使用 scanRequestPermissions + needScan=true 策略")
else:
    bad("A8", "网关设置页未按扫描策略请求权限")

if 'android.permission.ACCESS_FINE_LOCATION' in manifest_xml:
    ok("A9", "Manifest 保留 ACCESS_FINE_LOCATION（既有 Wi-Fi 配网页）")
else:
    bad("A9", "Manifest 丢失 ACCESS_FINE_LOCATION")

if 'android.permission.BLUETOOTH_CONNECT' in manifest_xml and \
        'android.permission.BLUETOOTH_SCAN' in manifest_xml:
    ok("A10", "Manifest 声明 BLUETOOTH_CONNECT / BLUETOOTH_SCAN")
else:
    bad("A10", "Manifest 缺少蓝牙新权限声明")

# ---------------- B. G-2 网关身份 ----------------
hard = collect(re.escape("84:C2:E4:03:02:02"))
hard_rel = [os.path.normpath(h[0]) for h in hard]
if len(hard) == 1 and hard_rel[0].endswith(os.path.normpath("protocol/GatewayIdentity.kt")):
    ok("B1", "硬编码迁移默认 MAC 恰 1 处（GatewayIdentity.kt）")
else:
    bad("B1", "硬编码默认 MAC 分布异常: %s" % hard)

if "GatewayIdentity.resolveAddress(" in mq and "gateway_ble_addr" in mq:
    ok("B2", "连接目标来自 GatewayIdentity.resolveAddress / gateway_ble_addr")
else:
    bad("B2", "连接目标未使用 gateway_ble_addr")

if 'getRemoteDevice(address)' in mq and 'getRemoteDevice("' not in mq:
    ok("B3", "GATT 目标来自变量 address，不是硬编码字面量")
else:
    bad("B3", "GATT 目标可能仍为硬编码")

identity = read(os.path.join(JAVA, "protocol", "GatewayIdentity.kt"))
if "PREF_GATEWAY_BLE_ADDR_CLEARED" in identity and "gateway_ble_addr_cleared" in identity:
    ok("B4", "显式清除标记键存在且隔离于默认值")
else:
    bad("B4", "缺少清除标记")

# ---------------- C. G-3 批次转发 ----------------
legacy_tokens = ["bletime", "bleTempData", "bleHumiData", "bletimebkp"]
legacy_hits = []
for t in legacy_tokens:
    legacy_hits += collect(re.escape(t))
if not legacy_hits:
    ok("C1", "工程内无“只保留最后一笔”的转发状态变量")
else:
    bad("C1", "仍存在旧转发状态: %s" % legacy_hits[:5])

plan_calls = collect(r"BleBatchForwarding\.plan\(")
if len(plan_calls) == 1:
    ok("C2", "批次载荷构造只有一处（BleBatchForwarding.plan 调用 1 次）")
else:
    bad("C2", "BleBatchForwarding.plan 调用次数 = %d" % len(plan_calls))

if "humi" + "," + "%s" not in read(os.path.join(JAVA, "MqtttService.kt")):
    ok("C3", "MqtttService 无湿度在前的旧拼接")
else:
    bad("C3", "存在湿度在前的旧拼接")

if "pendingForwardPayload" in mq and "MQTT offline: keep latest BLE batch (bounded=1)" in mq:
    ok("C4", "未连接 MQTT 时只保留最近一批（有界 1）并在连接成功后发布一次")
else:
    bad("C4", "未连接缓存策略缺失")

if "shouldPublishBle()" in mq and "publishMessage(payload, rtopic0)" in mq:
    ok("C5", "保留既有 topic 选择分支")
else:
    bad("C5", "topic 分支缺失")

# ---------------- D. G-7 容量/完整性 ----------------
for fn in ("capacityVerdict(", "mtuVerdict(", "lengthVerdict("):
    if fn in mq:
        ok("D1:" + fn, "MqtttService 调用 " + fn)
    else:
        bad("D1:" + fn, "MqtttService 未调用 " + fn)

if "val size = value?.size ?: 0" in mq and "size % 16 != 0" in mq:
    ok("D2", "onCharacteristicRead 使用实际 value.size 做 16 B 对齐判定")
else:
    bad("D2", "未使用 value.size 判定")

for state in ("BINDING_CAPACITY_EXCEEDED", "READ_INCOMPLETE", "GATEWAY_NO_DATA"):
    if state in mq:
        ok("D3:" + state, "链路状态 " + state + " 可读")
    else:
        bad("D3:" + state, "缺少链路状态 " + state)

if "maxCount: Int = GatewayReadGuard.MAX_DEVICES" in mq and \
        "all.size > maxCount || all.size > GatewayReadGuard.MAX_DEVICES" in mq:
    ok("D4", "0xA1 写入按 MAX_DEVICES=9 明确拒绝，无 50 台静默截断")
else:
    bad("D4", "0xA1 写入容量拒绝缺失")

if "take(50)" not in mq and "maxCount = 50" not in mq:
    ok("D5", "工程内无 take(50) / maxCount=50")
else:
    bad("D5", "仍存在 50 台截断")

notif = read(os.path.join(JAVA, "ui", "notifications", "NotificationsFragment.kt"))
if "GatewayReadGuard.MAX_DEVICES" in notif and "bound >= GatewayReadGuard.MAX_DEVICES" in notif:
    ok("D6", "绑定入口在达到 9 台时明确拒绝")
else:
    bad("D6", "绑定入口缺少容量拒绝")

if "mtuVerdict(negotiatedMtu, expectedDevices)" in mq and "g.discoverServices()" in mq:
    ok("D7", "MTU 判定通过后才 discoverServices（不足不发起读）")
else:
    bad("D7", "MTU 判定未接入读路径")

if "mtuRetried" in mq and "retrying connect once" in mq:
    ok("D8", "MTU 不足只重连一次")
else:
    bad("D8", "MTU 重试有界性未体现")

# ---------------- E. G-6 上传分类/阻塞 ----------------
code_main = []
for p in walk_kt(KOTLIN):
    txt = strip_comments(read(p))
    if re.search(r"Authorization|Bearer\s", txt):
        code_main.append(os.path.relpath(p, ROOT))
if not code_main:
    ok("E1", "非注释代码中无 Authorization/Bearer")
else:
    bad("E1", "存在未证实鉴权: %s" % code_main)

cloud = read(os.path.join(JAVA, "cloud", "CloudUploadRepository.kt"))
if "api.uploadDetailed(payload)" in cloud:
    ok("E2", "上传使用 UploadResult 详细分类")
else:
    bad("E2", "未使用 uploadDetailed")

if "outbox.park(it, config.maxAttempts)" in cloud and "resetForManualRetry" in cloud:
    ok("E3", "401/403/4xx 进入 park，手动重试恢复")
else:
    bad("E3", "阻塞/恢复逻辑缺失")

if "interceptorInjectionPoint" in cloud and cloud.count("addInterceptor") == 1:
    ok("E4", "仅一个空拦截器注入点")
else:
    bad("E4", "拦截器注入点形状异常（addInterceptor 计数=%d）" % cloud.count("addInterceptor"))

dialog_text = read(os.path.join(JAVA, "ui", "gateway", "GatewayBleSettingsDialog.kt"))
if "KEY_UPLOAD_BLOCKED_REASON" in dialog_text and "重试上传" in dialog_text:
    ok("E5", "设置页展示阻塞原因并提供“重试上传”")
else:
    bad("E5", "设置页缺少阻塞展示/手动重试")

# 静态证据：阻塞状态只被写入与展示，取批/门控未读取该状态
repo_part = strip_comments(cloud.split("class PrefsUploadGate")[0])
outbox_code = strip_comments(read(os.path.join(JAVA, "cloud", "UploadOutbox.kt")))
blocked_read_in_repo = ("KEY_UPLOAD_BLOCKED_REASON" in repo_part or
                        "upload_blocked_reason" in repo_part or
                        "isUploadBlocked" in repo_part or
                        "uploadBlocked(" in repo_part)
if not blocked_read_in_repo and "upload_blocked_reason" not in outbox_code:
    find("E6", "CloudUploadRepository/UploadOutbox 未读取全局阻塞状态：401 后仅 park 本批行，"
                "其余到期行仍会在下一次 triggerUpload 自动上传（实测见 Kotlin 门禁 "
                "g6_authRejected_mustPauseAllAutomaticRequests_untilManualRetry）")
else:
    ok("E6", "上传编排读取全局阻塞状态")

# ---------------- F. 版本与产物 ----------------
gradle = read(os.path.join(APP, "build.gradle.kts"))
m_vc = re.search(r"versionCode\s*=\s*(\d+)", gradle)
m_vn = re.search(r'versionName\s*=\s*"([^"]+)"', gradle)
if m_vc and m_vn and int(m_vc.group(1)) >= 10 and m_vn.group(1) == "1.9":
    ok("F1", "versionCode=%s versionName=%s" % (m_vc.group(1), m_vn.group(1)))
else:
    bad("F1", "版本号不符合 10 / 1.9")

apk = os.path.join(APP, "build", "outputs", "apk", "debug", "app-debug.apk")
if os.path.isfile(apk):
    h = hashlib.sha256(open(apk, "rb").read()).hexdigest()
    ok("F2", "APK 存在，sha256=%s，size=%d" % (h, os.path.getsize(apk)))
else:
    bad("F2", "APK 不存在")

# ---------------- G. G-4 规则文案/文档 ----------------
dp = read(os.path.join(JAVA, "ui", "detail", "DetailPresentation.kt"))
m = re.search(r'val REPORT_RULE_TEXT: String =\s*(.*?)\n\n', dp, flags=re.S)
if not m:
    bad("G1", "无法抽取 REPORT_RULE_TEXT")
else:
    expr = m.group(1)
    parts = re.findall(r'"([^"]*)"', expr)
    actual = "".join(parts)
    frozen = ("采样周期 3 分钟。仅当温度较上次升高超过 0.9℃且无光，或温度超过 35.0℃ 时才上报；"
              "未满足条件时不会上报，因此可能长时间没有新数据。")
    if actual == frozen:
        ok("G1", "REPORT_RULE_TEXT 与冻结文本逐字一致")
    else:
        bad("G1", "规则文案不一致: %r" % actual)

banned_rule = "较上次下降超过 0.9"
scope_files = [os.path.join(JAVA, "ui", "detail", "DetailPresentation.kt"),
               os.path.join(RES, "layout", "fragment_device_detail.xml"),
               DOC_README, DOC_MANUAL]
banned_hits = [(os.path.relpath(p, ROOT), banned_rule) for p in scope_files
               if banned_rule in read(p)]
if not banned_hits:
    ok("G2", "产品源码/布局/readme/说明书均无“较上次下降超过 0.9”")
else:
    bad("G2", "旧下降文案残留: %s" % banned_hits)

readme = read(DOC_README)
if "2026-11 升级为1.9" in readme and "升高超过 0.9℃且无光" in readme:
    ok("G3", "readme V1.9 变更记录已同步升温方向")
else:
    bad("G3", "readme 缺少 V1.9/升温说明")

cad = read(os.path.join(JAVA, "telemetry", "SensorCadence.kt"))
if "升高 >0.9℃" in cad:
    ok("G4", "SensorCadence 注释方向为升温")
else:
    bad("G4", "SensorCadence 注释未更新")

api = read(os.path.join(JAVA, "ui", "zhuce", "ApiService.kt"))
if "不是物理采样时刻" in api:
    ok("G5", "ApiService.SensorReading.time 注释同步接收秒语义")
else:
    bad("G5", "time 注释未同步")

layout = read(os.path.join(RES, "layout", "fragment_device_detail.xml"))
frozen_short = "仅当温度较上次升高超过 0.9℃且无光"
if frozen_short in layout:
    ok("G6", "详情布局静态文本为升温方向")
else:
    bad("G6", "详情布局文本未更新")

dp_code = strip_comments(dp)
for w in ("lux", "照度", "光照", "勒克斯"):
    if w in dp_code:
        bad("G7", "DetailPresentation 非注释代码出现光照词元 " + w)
        break
else:
    ok("G7", "DetailPresentation 非注释代码无 lux/照度/光照/勒克斯")

# App 自身下降报警文案保持不变
alarm_tokens = ["温度湿度下降报警", "温湿度超限报警", "紧急报警，温度超过65度，谨防火灾"]
missing = [t for t in alarm_tokens if t not in mq]
if not missing:
    ok("G8", "App 自身三条报警文案逐字保留")
else:
    bad("G8", "报警文案缺失: %s" % missing)

# ---------------- H. 清单 ----------------
manifest_txt = read(MANIFEST)
bad_entries = []
for raw in manifest_txt.splitlines():
    line = raw.strip()
    if not line or line.startswith("#"):
        continue
    parts = [p.strip() for p in line.split("|")]
    if len(parts) < 2 or parts[1] != "active":
        continue
    path = parts[0]
    if "*" in path:
        # 清单中的通配分组条目（如 software_verify_*_item0NN.py）是文件族描述，
        # 不是单一真实路径；检查同目录下至少存在一个匹配前缀的文件。
        import glob as _glob
        directory = os.path.dirname(path)
        prefix = os.path.basename(path).split("*")[0]
        if not _glob.glob(os.path.join(ROOT, directory, prefix + "*")):
            bad_entries.append(path)
        continue
    if path.endswith("/"):
        candidate = os.path.join(ROOT, path.rstrip("/"))
        if not os.path.isdir(candidate):
            bad_entries.append(path)
    else:
        if not os.path.exists(os.path.join(ROOT, path)):
            bad_entries.append(path)
if not bad_entries:
    ok("H1", "file_manifest.txt 全部 active 路径存在")
else:
    bad("H1", "无效 active 条目 %d 条: %s" % (len(bad_entries), bad_entries[:5]))

new_files = [
    "dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/ble/BlePermissionPolicy.kt",
    "dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/ble/GatewayDiscovery.kt",
    "dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/protocol/GatewayIdentity.kt",
    "dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/protocol/GatewayReadGuard.kt",
    "dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/telemetry/BleBatchForwarding.kt",
    "dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/telemetry/BleObservation.kt",
    "dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/cloud/UploadResult.kt",
    "dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/ui/gateway/GatewayBleSettingsDialog.kt",
]
missing_manifest = [p for p in new_files if p not in manifest_txt]
if not missing_manifest:
    ok("H2", "本轮 8 个新增产品文件均已在清单登记")
else:
    bad("H2", "清单未登记: %s" % missing_manifest)

# ---------------- I. 其它接线 ----------------
if "private val READ_INTERVAL_MS = SensorCadence.SAMPLE_PERIOD_MS" in mq:
    ok("I1", "BLE 读取周期引用 SensorCadence.SAMPLE_PERIOD_MS")
else:
    bad("I1", "读取周期未与 180 s 对齐")

if 'requestMtu(240)' in mq:
    ok("I2", "请求 MTU 240 保留")
else:
    bad("I2", "MTU 请求缺失")

if "samples += decodeDeviceBlock(frame, off)" in read(os.path.join(JAVA, "protocol", "GatewayFrameCodec.kt")):
    ok("I3", "codec 仍按 devCount 消费设备块（尾部不当作设备）")
else:
    bad("I3", "codec 设备块消费逻辑缺失")

# ---------------- 输出 ----------------
print("== AA-005 独立静态核查 ==")
failed = 0
for good, cid, msg in results:
    print(("PASS  " if good else "FAIL  ") + cid + "  " + msg)
    if not good:
        failed += 1
if findings:
    print("== 观察/FINDING ==")
    for cid, msg in findings:
        print("FIND  " + cid + "  " + msg)
print("== RESULT ==")
if failed == 0:
    print("OK: %d 项检查全部通过；%d 项观察" % (len(results), len(findings)))
    sys.exit(0)
else:
    print("FAILED: %d/%d 项检查失败" % (failed, len(results)))
    sys.exit(1)
