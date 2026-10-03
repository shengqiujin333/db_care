#!/usr/bin/env python3
"""独立验证（software_tester.software_verification / ITEM-007；TD-SW-002 T-SW-L0d-04、T-SW-L1-04）。

验证「入库使用真实采样时间」的改动，语句**从被测源码抽取**（不手抄），并做静态 + 宿主 SQLite 执行：

  A 源码事实：
    A1 新 API `storeTemperatureReading(devId, time, temperature, humidity, storeflag)` 存在，
       且**门控在打开数据库之前**（`if (!isGuardActive(storeflag)) return -1L`）；
    A2 `isGuardActive` 语义为 `storeflag == 1`；
    A3 插入语句列序 = 绑定顺序 = (time, device_id, temperature, humidity)；
    A4 旧列表式 `storeTemperatureData` / `averageList0|1` 已从 main 清除；
    A5 入库路径无 `60L` 回填（全 main 仅 AlarmEvaluator 的时间窗口算术含 `60L`）；
    A6 入口时间语义：`currentTime = timeOverride ?: System.currentTimeMillis()/1000`；
       BLE 调用不传 timeOverride；MQTT 调用传 `batch.timeSeconds`（payload 首段）。
  B 宿主 SQLite 执行抽取出的插入语句（v3 temperature 表，复合主键）：
    B1 写入行 `time` 恰为传入值（无 `-60` 偏移）；
    B2 值列位置正确（time/device_id/temperature/humidity 不串位）；
    B3 同 `(time, device_id)` 重复写入 → 1 行且为新值（REPLACE 幂等）；
    B4 不同 `time` → 新增行（不丢数据）。

运行：`python evidence/software_verify_timestamp_write_item007.py`；退出码 0/1。
"""

from __future__ import annotations

import os
import re
import sqlite3
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
HELPER_REL = "dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/TemperatureDatabaseHelper.kt"
MQTT_REL = "dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/MqtttService.kt"
HELPER = os.path.join(ROOT, HELPER_REL)
MQTT = os.path.join(ROOT, MQTT_REL)

violations: list[str] = []


def read(path):
    with open(path, encoding="utf-8") as fh:
        return fh.read()


def simple_consts(text):
    return dict(re.findall(r'(?:private )?const val (\w+) = "([^"]*)"\s*$', text, re.M))


def ddl_const(text, name):
    m = re.search(
        rf"(?:private )?const val {name}\s*=\s*(.*?)(?=\n\s*(?:(?:private )?const val|val |override|fun |\}}\n))",
        text, re.S,
    )
    if not m:
        return None
    resolved = "".join(re.findall(r'"([^"]*)"', m.group(1)))
    sc = simple_consts(text)
    return re.sub(r"\$(\w+)", lambda mm: sc.get(mm.group(1), mm.group(0)), resolved)


def check_source():
    helper = read(HELPER)
    mqtt = read(MQTT)
    print("== A 源码事实 ==")

    # A1 新 API 与门控位置
    m = re.search(r"fun storeTemperatureReading\((.*?)\): Long \{(.*?)\n    \}", helper, re.S)
    ok = m is not None
    params = [p.strip() for p in m.group(1).split(",")] if m else []
    expected_params = ["devId: String", "time: Long", "temperature: Double", "humidity: Double", "storeflag: Int"]
    ok = ok and params == expected_params
    print(f"  {'PASS' if ok else 'FAIL'}  storeTemperatureReading 参数 = {params}")
    if not ok:
        violations.append(f"A1: 参数不符 {params}")

    body = m.group(2) if m else ""
    gate = body.find("if (!isGuardActive(storeflag)) return -1L")
    db_open = body.find("writableDatabase")
    ok = gate >= 0 and db_open > gate
    print(f"  {'PASS' if ok else 'FAIL'}  门控位于打开数据库之前（gate@{gate} < db@{db_open}）")
    if not ok:
        violations.append("A1: 门控不在数据库访问之前")

    # A2 isGuardActive 语义
    m2 = re.search(r"fun isGuardActive\(storeflag: Int\): Boolean = (.+)", helper)
    ok = m2 is not None and m2.group(1).strip() == "storeflag == 1"
    print(f"  {'PASS' if ok else 'FAIL'}  isGuardActive = {m2.group(1).strip() if m2 else None}")
    if not ok:
        violations.append("A2: isGuardActive 语义不符")

    # A3 插入语句列序
    insert_sql = ddl_const(helper, "SQL_INSERT_TEMPERATURE_READING")
    cols = re.search(r"\(([^)]*)\)\s*VALUES", insert_sql or "")
    col_list = [c.strip() for c in cols.group(1).split(",")] if cols else []
    ok = col_list == ["time", "device_id", "temperature", "humidity"] and "INSERT OR REPLACE" in (insert_sql or "")
    print(f"  {'PASS' if ok else 'FAIL'}  插入语句 = {insert_sql!r}, 列序 = {col_list}")
    if not ok:
        violations.append(f"A3: 插入语句/列序异常 {insert_sql!r} {col_list}")

    # A4 旧 API/临时列表清除
    stale = [p for p in ("storeTemperatureData", "averageList0", "averageList1") if p in helper or p in mqtt]
    ok = not stale
    print(f"  {'PASS' if ok else 'FAIL'}  旧列表式 API 清除（残留={stale}）")
    if not ok:
        violations.append(f"A4: 残留 {stale}")

    # A5 入库路径无 60 s 回填：`60L` 只允许出现在时间窗口/退避等非入库语义处
    hits = []
    for base, _dirs, files in os.walk(os.path.join(ROOT, "dengbei_care", "app", "src", "main")):
        for f in files:
            if f.endswith(".kt"):
                p = os.path.join(base, f)
                rel = os.path.relpath(p, ROOT).replace("\\", "/")
                for i, line in enumerate(read(p).split("\n"), 1):
                    if "60L" in line:
                        hits.append((rel, i, line.strip()))

    def allowed(rel, line):
        # 时间窗口算术（AlarmEvaluator / MqtttService.loadBaselineSamples）或退避常量（CloudUploadRepository）
        if rel.endswith("telemetry/AlarmEvaluator.kt"):
            return True
        if rel.endswith("cloud/CloudUploadRepository.kt"):
            return "DEFAULT_INITIAL_BACKOFF_SECONDS" in line
        if rel.endswith("MqtttService.kt"):
            return "widestWindow" in line
        return False

    bad = [(r, i, t[:70]) for (r, i, t) in hits if not allowed(r, t)]
    ok = not bad
    print(f"  {'PASS' if ok else 'FAIL'}  60L 分布 = {[(r.split('/')[-1], i) for (r, i, _t) in hits]}")
    print(f"        （仅允许 AlarmEvaluator 窗口 / CloudUploadRepository 退避常量 / MqtttService 基准窗口算术）")
    if not ok:
        violations.append(f"A5: 60L 非法使用 {bad}")
    # 存储路径不得按固定间隔回填历史时间戳
    helper = read(HELPER)
    ok2 = ("dataSize" not in helper) and ("size - 1 - i" not in helper) and ("* 60" not in helper)
    print(f"  {'PASS' if ok2 else 'FAIL'}  数据库辅助类无固定间隔回填（dataSize/(size-1-i)/* 60）")
    if not ok2:
        violations.append("A5: 仍存在固定间隔回填")

    # A6 入口时间语义
    ok = "val currentTime = timeOverride ?: (System.currentTimeMillis() / 1000)" in mqtt
    print(f"  {'PASS' if ok else 'FAIL'}  currentTime = timeOverride ?: 接收时刻")
    if not ok:
        violations.append("A6: currentTime 计算不符")
    ble_calls = re.findall(r"processTemperatureHumidityData\(devId, r\.temperatureC, r\.humidityPct\)", mqtt)
    ok_ble = len(ble_calls) >= 1
    print(f"  {'PASS' if ok_ble else 'FAIL'}  BLE 调用不传 timeOverride（命中 {len(ble_calls)} 处）")
    if not ok_ble:
        violations.append("A6: BLE 调用点未找到/传了 timeOverride")
    ok_mqtt = "processTemperatureHumidityData(\n                                            r.devId, r.temperature, r.humidity, batch.timeSeconds\n                                        )" in mqtt or \
              re.search(r"processTemperatureHumidityData\(\s*r\.devId,\s*r\.temperature,\s*r\.humidity,\s*batch\.timeSeconds\s*\)", mqtt) is not None
    print(f"  {'PASS' if ok_mqtt else 'FAIL'}  MQTT 调用传 payload 首段时间 batch.timeSeconds")
    if not ok_mqtt:
        violations.append("A6: MQTT 调用点未传 batch.timeSeconds")
    ok_store = re.search(r"storeTemperatureReading\(\s*this@MqtttService,\s*devId,\s*currentTime,\s*temperature,\s*humidity,\s*humistartflag\s*\)", mqtt) is not None
    print(f"  {'PASS' if ok_store else 'FAIL'}  存储调用实参顺序 = (devId, currentTime, temperature, humidity, storeflag)")
    if not ok_store:
        violations.append("A6: 存储调用实参不符")

    create_sql = ddl_const(helper, "SQL_CREATE_TABLE_TEMPERATURE")
    return insert_sql, create_sql


def check_execution(insert_sql, create_sql):
    print("== B 宿主 SQLite 执行抽取出的语句 ==")
    db = sqlite3.connect(":memory:")
    db.execute(create_sql)

    t = 1700000123
    dev = "A1B2C3D4"
    # 绑定顺序 = SQL 列序 = (time, device_id, temperature, humidity)
    db.execute(insert_sql.replace("INSERT OR REPLACE", "INSERT OR REPLACE"), (t, dev, 25.5, 60.0))
    rows = list(db.execute("SELECT time, device_id, temperature, humidity FROM temperature"))
    ok = rows == [(t, dev, 25.5, 60.0)]
    print(f"  {'PASS' if ok else 'FAIL'}  B1/B2 写入行 = {rows}（time 无偏移、列不串位）")
    if not ok:
        violations.append(f"B1/B2: 写入行异常 {rows}")

    db.execute(insert_sql, (t, dev, 26.5, 61.5))
    rows = list(db.execute("SELECT count(*), max(temperature), max(humidity) FROM temperature"))
    ok = rows == [(1, 26.5, 61.5)]
    print(f"  {'PASS' if ok else 'FAIL'}  B3 同 (time, device_id) 重复写入 → {rows}（1 行且为新值）")
    if not ok:
        violations.append(f"B3: 幂等失败 {rows}")

    db.execute(insert_sql, (t + 180, dev, 24.0, 58.0))
    rows = list(db.execute("SELECT time, temperature FROM temperature ORDER BY time"))
    ok = rows == [(t, 26.5), (t + 180, 24.0)]
    print(f"  {'PASS' if ok else 'FAIL'}  B4 不同 time → 两行 {rows}")
    if not ok:
        violations.append(f"B4: 追加写入失败 {rows}")


def main() -> int:
    insert_sql, create_sql = check_source()
    check_execution(insert_sql, create_sql)
    print("== RESULT ==")
    if violations:
        for v in violations:
            print("  VIOLATION:", v)
        return 1
    print("  OK: 单条真实时间戳写入、幂等、门控与入口时间语义均成立（宿主 SQL 执行验证通过）")
    return 0


if __name__ == "__main__":
    sys.exit(main())
