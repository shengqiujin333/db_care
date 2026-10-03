#!/usr/bin/env python3
"""独立静态/语义验证（software_tester.software_verification / ITEM-010）。

  A 读取周期对齐：`READ_INTERVAL_MS = SensorCadence.SAMPLE_PERIOD_MS`；节拍常量 = 180 s / 180000 ms；
    手动立即读取通道（`wakeReadNow`）保留并在轮询 `select` 中使用。
  B 编排顺序（`processTemperatureHumidityData`）：真实时间 → 参数 → 库基准 → AlarmEvaluator →
    级别映射 → 紧急直接通知 → 卡片告警 → 报警事件落库 → 单条真实时间戳入库 → 入队+上传触发 → 卡片读数 →
    900 s 去抖。逐项按源码位置排序断言。
  C 兼容面保留：三条文案字面值、`ALARM_TYPE_*`、`900 * 1000` 去抖、`>65℃`（AlarmEvaluator）、
    `AlarmEmergent`、`_bkp_all_time`、旧内存窗口字段清零。
  D 上传触发点×4：onCreate（冷启动）、每轮 BLE 开始、broker 连接成功（网络恢复）、入库后入队。
  E 基准来源：`loadBaselineSamples` → `getSamplesInRange`，范围 = `now - 15min - 回溯容差`，查询失败返回空表；
    并在宿主 SQLite 上执行该查询的 SQL（过滤 + 升序）。
  F **步骤隔离**（本项验收文字："任何一步失败都不影响其他步骤"）：断言每个副作用步骤各自受保护。
    若整个函数只有一层 try/catch 且通知/落库/入库/上传共享同一 try，则判 FAIL（并列出未加保护的调用点）。

运行：`python evidence/software_verify_orchestration_item010.py`；退出码 0/1。
"""

from __future__ import annotations

import os
import re
import sqlite3
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APP = "dengbei_care/app/src/main/java/com/jinyuni/dengbei_care"
MQTT_REL = f"{APP}/MqtttService.kt"
CADENCE_REL = f"{APP}/telemetry/SensorCadence.kt"
HELPER_REL = f"{APP}/TemperatureDatabaseHelper.kt"
RINGTONE_REL = f"{APP}/RingtonePlayer.kt"
VIBRATION_REL = f"{APP}/VibrationPlayer.kt"

violations: list[str] = []


def read(rel):
    with open(os.path.join(ROOT, rel), encoding="utf-8") as fh:
        return fh.read().replace("\r\n", "\n")


def function_body(text, signature_regex):
    m = re.search(signature_regex, text, re.S)
    if not m:
        return None
    start = m.end()
    # 以“行首 4 空格 + }”作为函数结束（本工程风格）
    end = re.search(r"\n    \}", text[start:])
    return text[start:start + (end.start() if end else len(text) - start)]


def order_check(body, markers, label):
    print(f"  [{label}] 顺序：")
    pos, ok = -1, True
    for name, needle in markers:
        i = body.find(needle)
        print(f"    {'PASS' if i > pos else 'FAIL'}  {name} @{i}")
        if i <= pos:
            ok = False
        pos = i
    if not ok:
        violations.append(f"B: 顺序异常 {label}")
    return ok


def main() -> int:
    mqtt = read(MQTT_REL)
    cadence = read(CADENCE_REL)
    helper = read(HELPER_REL)
    code = "\n".join(
        l.split("//")[0] for l in mqtt.split("\n")
        if not l.strip().startswith(("//", "*", "/*"))
    )

    print("== A 读取周期对齐 ==")
    checks = {
        "A1 READ_INTERVAL_MS = SensorCadence.SAMPLE_PERIOD_MS":
            "private val READ_INTERVAL_MS = SensorCadence.SAMPLE_PERIOD_MS" in mqtt,
        "A2 节拍常量 180 s": re.search(r"const val SAMPLE_PERIOD_SECONDS(?:\s*:\s*\w+)?\s*=\s*180L?\b", cadence) is not None,
        "A3 节拍 180000 ms（由秒常量派生）":
            re.search(r"const val SAMPLE_PERIOD_MS(?:\s*:\s*\w+)?\s*=\s*SAMPLE_PERIOD_SECONDS\s*\*\s*1000L", cadence) is not None
            and 180 * 1000 == 180000,
        "A4 手动立即读取通道保留（wakeReadNow 用于 select/唤醒）":
            "wakeReadNow" in mqtt and "wakeReadNow.onReceiveCatching" in mqtt and "wakeReadNow.tryReceive()" in mqtt,
        "A5 读取循环使用 READ_INTERVAL_MS": "onTimeout(READ_INTERVAL_MS)" in mqtt,
    }
    for k, v in checks.items():
        print(f"  {'PASS' if v else 'FAIL'}  {k}")
        if not v:
            violations.append(f"A: {k}")

    print("== B 编排顺序 ==")
    body = function_body(mqtt, r"fun processTemperatureHumidityData\(devId: String, temperature: Double, humidity: Double, timeOverride: Long\? = null\) \{")
    if body is None:
        violations.append("B: 未找到 processTemperatureHumidityData")
        print("  FAIL  未找到 processTemperatureHumidityData")
    else:
        order_check(body, [
            ("真实时间", "val currentTime = timeOverride ?: (System.currentTimeMillis() / 1000)"),
            ("每设备参数", "val params = loadDeviceParams(devId)"),
            ("库基准", "val baselines = loadBaselineSamples(devId, currentTime)"),
            ("AlarmEvaluator 判定", "val verdict = AlarmEvaluator.evaluate("),
            ("级别映射", "verdict.severity.toAppSeverity()"),
            ("紧急直接通知", "if (AlarmEmergent) {"),
            ("卡片告警", "homeViewModel.updateDeviceAlarm(devId, alarmMessage, alarmSeverity)"),
            ("报警事件落库", 'when (alarmMessage) {'),
            ("单条真实时间入库", "storeTemperatureReading("),
            ("入队+上传触发", "uploadRepository.enqueueAndUpload(devId, currentTime, temperature, humidity)"),
            ("卡片读数", "homeViewModel.updateDeviceReading(devId, deviceName, temperature, humidity, currentTime)"),
            ("900 s 去抖", "if (now - history.lastAlarmTime > (900 * 1000))"),
        ], "pipeline")

    print("== C 兼容面保留 ==")
    compat = {
        "C1 文案：温度湿度下降报警": '"温度湿度下降报警" -> storeAlarmEvent(' in mqtt,
        "C2 文案：温湿度超限报警": '"温湿度超限报警" -> storeAlarmEvent(' in mqtt,
        "C3 文案：紧急报警（谨防火灾）": '"紧急报警，温度超过65度，谨防火灾" -> storeAlarmEvent(' in mqtt,
        "C4 ALARM_TYPE_* 三种": all(s in mqtt for s in ("ALARM_TYPE_DROP", "ALARM_TYPE_THRESHOLD", "ALARM_TYPE_EMERGENCY")),
        "C5 900 s 去抖字面值": "(900 * 1000)" in mqtt,
        "C6 去抖仅在守护中": "if (homeViewModel.startData.value == 1) {" in mqtt,
        "C7 >65℃ 比较只在 AlarmEvaluator（Service 无该比较）":
            "EMERGENCY" in mqtt and "> EMERGENCY_TEMP_C" not in code and "65.0" not in code and "65f" not in code,
        "C8 AlarmEmergent 字段保留": "AlarmEmergent" in mqtt,
        "C9 _bkp_all_time 保留": "_bkp_all_time" in mqtt,
        "C10 旧内存窗口清零": not re.search(r"data5MinAgo|data10MinAgo|data15MinAgo|isSignificantChange", mqtt),
        "C11 未再出现 Service 侧阈值比较":
            not re.search(r"[<>]\s*params\.(tempmax|tempmin|humimax|humimin)", code)
            and "params.tempmax" in mqtt  # 仍作为 AlarmEvaluator 的入参传入
            and "AlarmEvaluator.Parameters(" in mqtt,
    }
    for k, v in compat.items():
        print(f"  {'PASS' if v else 'FAIL'}  {k}")
        if not v:
            violations.append(f"C: {k}")

    print("== D 上传触发点 ==")
    triggers = {
        "D1 冷启动 onCreate": re.search(r"uploadRepository\.triggerUpload\(\)\s*\n    \}\n\n    private fun startBluetoothTask", mqtt) is not None
            or "冷启动补传" in mqtt,
        "D2 每轮 BLE 开始": "// 每轮 BLE 开始：补传待发箱" in mqtt,
        "D3 网络恢复（broker 连接成功）": "uploadRepository.triggerUpload()" in mqtt[mqtt.find("connectToBroker"):] if "connectToBroker" in mqtt else False,
        "D4 入库后入队": "uploadRepository.enqueueAndUpload(" in mqtt,
        "D5 入队仅在守护中": "if (humistartflag == 1) {" in mqtt,
    }
    for k, v in triggers.items():
        print(f"  {'PASS' if v else 'FAIL'}  {k}")
        if not v:
            violations.append(f"D: {k}")
    # 触发点计数（triggerUpload 调用应 >=3：冷启动/每轮/网络恢复）
    n_trigger = len(re.findall(r"uploadRepository\.triggerUpload\(\)", mqtt))
    print(f"  {'PASS' if n_trigger >= 3 else 'FAIL'}  triggerUpload 调用数 = {n_trigger}（期望 >=3）")
    if n_trigger < 3:
        violations.append(f"D: triggerUpload 调用数 {n_trigger}")

    print("== E 基准来源（库查询 + 宿主 SQL 执行） ==")
    ok = ("loadBaselineSamples" in mqtt and "getSamplesInRange(devId, from, now)" in mqtt
          and "DEFAULT_LOOKBACK_SLACK_SECONDS" in mqtt and "AlarmEvaluator.DROP_WINDOW_MINUTES.maxOrNull()" in mqtt)
    print(f"  {'PASS' if ok else 'FAIL'}  loadBaselineSamples → getSamplesInRange，范围含最宽窗口 + 回溯容差")
    if not ok:
        violations.append("E: 基准查询接线异常")
    ok = "baseline query failed" in mqtt and "emptyList()" in mqtt
    print(f"  {'PASS' if ok else 'FAIL'}  查询失败 → 空列表（不报警、不伪造）")
    if not ok:
        violations.append("E: 查询失败降级缺失")
    m = re.search(r'val query = ((?:\s*"[^"]*"\s*\+?)+)', helper)
    sql = "".join(re.findall(r'"([^"]*)"', m.group(1))) if m else None
    ok = sql is not None and "WHERE $COLUMN_DEVICE_ID = ? AND $COLUMN_TIME BETWEEN ? AND ?" in sql and "ORDER BY $COLUMN_TIME ASC" in sql
    print(f"  {'PASS' if ok else 'FAIL'}  getSamplesInRange SQL = {sql}")
    if not ok:
        violations.append("E: getSamplesInRange SQL 异常")
    if sql:
        resolved = sql.replace("$TABLE_TEMPERATURE", "temperature").replace("$COLUMN_DEVICE_ID", "device_id") \
            .replace("$COLUMN_TIME", "time").replace("$COLUMN_TEMPERATURE", "temperature").replace("$COLUMN_HUMIDITY", "humidity")
        db = sqlite3.connect(":memory:")
        db.execute("CREATE TABLE temperature (time INTEGER, device_id TEXT, temperature REAL, humidity REAL, PRIMARY KEY(time, device_id))")
        db.executemany("INSERT INTO temperature VALUES (?,?,?,?)",
                       [(100, "A", 1.0, 10.0), (300, "A", 3.0, 30.0), (200, "A", 2.0, 20.0), (250, "B", 9.0, 90.0)])
        rows = list(db.execute(resolved, ("A", 150, 300)))
        ok = rows == [(200, 2.0, 20.0), (300, 3.0, 30.0)]  # 仅取 time/temperature/humidity 三列
        print(f"  {'PASS' if ok else 'FAIL'}  宿主执行：过滤设备+时间范围且按时间升序 → {rows}")
        if not ok:
            violations.append(f"E: 查询语义不符 {rows}")

    print("== F 步骤隔离（验收文字：任何一步失败都不影响其他步骤） ==")
    if body is None:
        violations.append("F: 无法分析步骤隔离")
    else:
        try_blocks = re.findall(r"(?<![\w.])try \{", body)
        print(f"  INFO  函数内 try 块数 = {len(try_blocks)}")
        # 通知/震动/铃声 vs 入库/上传 是否处于同一 try 覆盖域（本函数风格：单层 try 覆盖全函数体）
        notify_idx = body.find("if (AlarmEmergent) {")
        store_idx = body.find("storeTemperatureReading(")
        upload_idx = body.find("uploadRepository.enqueueAndUpload(")
        single_try = len(try_blocks) == 1
        # 紧急通知块与入库/上传之间是否存在独立保护（catch 或局部 try）
        segment = body[notify_idx:store_idx] if 0 <= notify_idx < store_idx else ""
        guarded_between = ("catch" in segment) or ("try {" in segment)
        helpers_in_segment = [
            name for name, pattern in (
                ("RingtonePlayer.startAlarm", r"RingtonePlayer\.startAlarm\("),
                ("VibrationPlayer.vibratePhone", r"VibrationPlayer\.vibratePhone\("),
            ) if re.search(pattern, segment)
        ]
        # 这些调用是否已被局部 try 覆盖（segment 内出现 try）
        helpers_guarded = ("try {" in segment)
        ok = not single_try and guarded_between
        print(f"  {'PASS' if ok else 'FAIL'}  单一全函数 try={single_try}；通知块与入库之间独立保护={guarded_between}")
        if not ok:
            violations.append(
                "F: 步骤隔离不足——函数为单一 try 覆盖，且紧急通知块（" + ", ".join(helpers_in_segment) +
                "）与入库/上传之间无独立保护；其中 RingtonePlayer.startAlarm 内部无异常保护（MediaPlayer.create 可能返回 null）"
            )
        print(f"  INFO  通知块内副作用调用 {helpers_in_segment}；已被局部 try 覆盖={helpers_guarded}")
        # 落库失败也不得跳过入库/上传：检查 storeAlarmEvent 是否独立保护
        alarm_store_seg = body[body.find('when (alarmMessage) {'):store_idx] if store_idx > 0 else ""
        ok2 = ("catch" in alarm_store_seg) or ("try {" in alarm_store_seg)
        print(f"  {'PASS' if ok2 else 'FAIL'}  报警事件落库与入库之间独立保护={ok2}")
        if not ok2:
            violations.append("F: 报警事件落库（storeAlarmEvent，内部无 catch）失败会跳过入库与上传触发")

    print("== F2 逐步保护（每个副作用步骤各自 try/catch）与辅助类不外抛 ==")
    if body is not None:
        steps = [
            ("清 sparkline", "homeViewModel.clearDevice(devId)"),
            ("紧急通知块", "if (AlarmEmergent) {"),
            ("卡片告警", "homeViewModel.updateDeviceAlarm(devId, alarmMessage, alarmSeverity)"),
            ("报警事件落库", "when (alarmMessage) {"),
            ("单条入库", "storeTemperatureReading("),
            ("入队+上传触发", "uploadRepository.enqueueAndUpload("),
            ("卡片读数", "homeViewModel.updateDeviceReading("),
            ("900 s 去抖副作用", "if (now - history.lastAlarmTime > (900 * 1000))"),
        ]
        ok_all = True
        for i, (name, needle) in enumerate(steps):
            start = body.find(needle)
            end = body.find(steps[i + 1][1]) if i + 1 < len(steps) else len(body)
            region = body[start:end] if start >= 0 and end > start else ""
            guarded = ("catch" in region)
            print(f"  {'PASS' if guarded else 'FAIL'}  {name} 之后出现 catch（步骤闭合）")
            if not guarded:
                ok_all = False
                violations.append(f"F2: {name} 未被独立保护")
        n_try = len(re.findall(r"(?<![\w.])try \{", body))
        print(f"  INFO  函数内 try 块数 = {n_try}")
        if not ok_all:
            print("  FAIL  存在未闭合保护的步骤")

        ring = read(RINGTONE_REL)
        vib = read(VIBRATION_REL)
        helper_checks = {
            "RingtonePlayer.startAlarm 有 try/catch": "fun startAlarm(" in ring
                and ring[ring.find("fun startAlarm("):].count("catch") >= 1,
            "RingtonePlayer 对 MediaPlayer.create 判空": "MediaPlayer.create(context, alarmUri) ?:" in ring,
            "RingtonePlayer URI 为空降级": "no default alarm/notification uri available" in ring,
            "RingtonePlayer.stopAlarm 有 try/catch+finally 置空":
                "fun stopAlarm()" in ring and "catch (e: Exception)" in ring and "mediaPlayer = null" in ring,
            "VibrationPlayer.vibratePhone 有 try/catch": "fun vibratePhone(" in vib
                and vib[vib.find("fun vibratePhone("):].count("catch") >= 1,
            "VibrationPlayer 用 as? 判空而非强转": "as? VibratorManager" in vib and "as? Vibrator" in vib
                and "as VibratorManager" not in vib and "as Vibrator" not in vib,
            "VibrationPlayer.stopVibration 有 try/catch+finally 置空":
                "fun stopVibration(" in vib and "catch (e: Exception)" in vib and "vibrator = null" in vib,
            "两个辅助类均不向外抛（无显式 throw）":
                "throw " not in ring and "throw " not in vib,
        }
        for k, v in helper_checks.items():
            print(f"  {'PASS' if v else 'FAIL'}  {k}")
            if not v:
                violations.append(f"F2: {k}")

    print("== RESULT ==")
    if violations:
        for v in violations:
            print("  VIOLATION:", v)
        return 1
    print("  OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
