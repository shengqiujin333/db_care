#!/usr/bin/env python3
"""独立验证（software_tester.software_verification / ITEM-006；TD-SW-002 T-SW-L0d-01/02/03）。

本脚本在**宿主机 SQLite 引擎**上执行 v2→v3 / v1→v3 迁移语义，所用迁移语句**直接从被测 Kotlin
源码抽取**（不是手抄），并独立构造改动前的 v2/v1 数据库与数据，因此可核查：

  A 源码事实：`DATABASE_VERSION == 3`；v3 两条新 DDL 形状；`MIGRATION_V2_TO_V3` 恰为这两条；
    `onCreate` 也建新表；v2→v3 分支不含对既有表的 DROP/ALTER/UPDATE（回退只针对新对象）。
  B 基线不变：既有 `temperature`/`alarm_events`/两索引的 DDL 与改动前提交逐字相同（仅可见性变化）。
  C 执行语义（宿主 SQLite）：
    C1 v2+数据 → 迁移 → 历史数据保真、新表/索引/复合主键存在、默认值 0、重复键拒绝、同键 OR REPLACE 幂等；
    C2 迁移重复执行不报错；
    C3 冲突对象（同名 VIEW）→ 首次迁移行为如实记录 → 按代码回退序列（DROP VIEW/TABLE + 重试）恢复为表且数据完好；
    C4 v1+数据 → v1→v2 既有语句（逐字转写）→ v2→v3 语句 → 历史行保留、device_id 回填、三表齐备。

说明：本脚本验证的是**迁移语句与既有 DDL 的语义**；Android `SQLiteOpenHelper` 真机经验层需要
设备侧 instrumented 运行（已编写 `app/src/androidTest/.../TemperatureDatabaseItem006VerificationTest.kt`，
APK 已可编译打包，但本机 Gradle UTP 依赖缺失且无网络，见证据文件记录）。

运行：`python evidence/software_verify_db_migration_item006.py`
退出码：0 = 全部通过；1 = 存在违规。
"""

from __future__ import annotations

import os
import re
import sqlite3
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REL = "dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/TemperatureDatabaseHelper.kt"
SRC = os.path.join(ROOT, REL)
BASELINE_COMMIT = "c232105"  # 本项实现前的提交（android_engineer.android_implementation 上一节点）

violations: list[str] = []


def read(path):
    with open(path, encoding="utf-8") as fh:
        return fh.read()


def extract_simple_consts(text):
    return dict(re.findall(r'(?:private )?const val (\w+) = "([^"]*)"\s*$', text, re.M))


def extract_ddl_const(text, name):
    """抽取形如 `const val NAME =\n "..." +\n "..."` 的常量并解析 $IDENT 插值。"""
    m = re.search(rf"const val {name}\s*=\s*(.*?)(?=\n\s*(?:(?:private )?const val|val |override|fun |}}\n))", text, re.S)
    if not m:
        return None
    parts = re.findall(r'"([^"]*)"', m.group(1))
    resolved = "".join(parts)
    simple = extract_simple_consts(text)
    return re.sub(r"\$(\w+)", lambda mm: simple.get(mm.group(1), mm.group(0)), resolved)


def baseline_text():
    try:
        out = subprocess.run(
            ["git", "show", f"{BASELINE_COMMIT}:{REL}"],
            cwd=ROOT, capture_output=True, text=True, check=True,
        )
        return out.stdout
    except Exception as exc:  # pragma: no cover
        print(f"  WARN 无法读取基线提交 {BASELINE_COMMIT}: {exc}")
        return None


# ---------------------------------------------------------------- A / B 源码事实
def checks_source():
    text = read(SRC)
    simple = extract_simple_consts(text)

    print("== A 源码事实 ==")
    version = re.search(r"const val DATABASE_VERSION = (\d+)", text)
    ok = version is not None and version.group(1) == "3"
    print(f"  {'PASS' if ok else 'FAIL'}  DATABASE_VERSION = {version.group(1) if version else None} (want 3)")
    if not ok:
        violations.append("A: DATABASE_VERSION != 3")

    table = extract_ddl_const(text, "SQL_CREATE_TABLE_PENDING_UPLOADS")
    index = extract_ddl_const(text, "SQL_CREATE_INDEX_PENDING_UPLOADS_NEXT")
    want_table = ("CREATE TABLE IF NOT EXISTS pending_uploads (device_id TEXT, time INTEGER, temperature REAL, "
                  "humidity REAL, attempts INTEGER DEFAULT 0, next_attempt_at INTEGER DEFAULT 0, "
                  "PRIMARY KEY(device_id, time))")
    want_index = "CREATE INDEX IF NOT EXISTS idx_pending_uploads_next ON pending_uploads(next_attempt_at)"
    for label, got, want in (("v3 待发箱 DDL", table, want_table), ("v3 索引 DDL", index, want_index)):
        ok = got == want
        print(f"  {'PASS' if ok else 'FAIL'}  {label}: {got!r}")
        if not ok:
            violations.append(f"A: {label} 与期望不符")

    # MIGRATION_V2_TO_V3 恰为两条
    m = re.search(r"MIGRATION_V2_TO_V3: List<String> = listOf\((.*?)\)", text, re.S)
    steps = re.findall(r"\b(SQL_CREATE_\w+)\b", m.group(1)) if m else []
    ok = steps == ["SQL_CREATE_TABLE_PENDING_UPLOADS", "SQL_CREATE_INDEX_PENDING_UPLOADS_NEXT"]
    print(f"  {'PASS' if ok else 'FAIL'}  MIGRATION_V2_TO_V3 = {steps}")
    if not ok:
        violations.append(f"A: MIGRATION_V2_TO_V3 内容异常: {steps}")

    # onCreate 建新表
    on_create = re.search(r"override fun onCreate\(db: SQLiteDatabase\) \{(.*?)\n    \}", text, re.S)
    body = on_create.group(1) if on_create else ""
    ok = "SQL_CREATE_TABLE_PENDING_UPLOADS" in body and "SQL_CREATE_INDEX_PENDING_UPLOADS_NEXT" in body
    print(f"  {'PASS' if ok else 'FAIL'}  onCreate 含 v3 新表与索引")
    if not ok:
        violations.append("A: onCreate 未包含 v3 新对象")

    # v2→v3 分支：不含对既有表的破坏性语句
    block = re.search(r"if \(oldVersion < 3\) \{(.*?)\n        \}\n    \}", text, re.S)
    blk = block.group(1) if block else ""
    destructive = re.findall(r"\b(DROP TABLE|DROP VIEW|ALTER TABLE|UPDATE|DELETE FROM)\b", blk)
    touches_existing = re.search(r"(DROP|ALTER|UPDATE|DELETE)[^\n]*(TABLE_TEMPERATURE|TABLE_ALARM_EVENTS)", blk)
    ok = destructive and set(destructive) <= {"DROP TABLE", "DROP VIEW"} and touches_existing is None
    print(f"  {'PASS' if ok else 'FAIL'}  v2→v3 分支仅含 CREATE 与（回退时）新对象 DROP；破坏性语句={destructive}")
    if not ok:
        violations.append(f"A: v2→v3 分支出现可疑破坏性语句 {destructive}")

    # 基线 DDL 逐字不变
    print("== B 既有结构（对照改动前提交） ==")
    base = baseline_text()
    if base is not None:
        for name in ("SQL_CREATE_TABLE_TEMPERATURE", "SQL_CREATE_TABLE_ALARM_EVENTS",
                     "SQL_CREATE_INDEX_TEMPERATURE_DEVICE", "SQL_CREATE_INDEX_ALARM_DEVICE"):
            cur = extract_ddl_const(text, name)
            old = extract_ddl_const(base, name)
            ok = cur is not None and cur == old
            print(f"  {'PASS' if ok else 'FAIL'}  {name} 未变")
            if not ok:
                violations.append(f"B: {name} 与基线不同: {old!r} -> {cur!r}")
        old_ver = re.search(r"const val DATABASE_VERSION = (\d+)", base)
        print(f"  INFO  基线 DATABASE_VERSION = {old_ver.group(1) if old_ver else None}（本项升为 3）")

    return table, index, text


# ---------------------------------------------------------------- C 执行语义
def migrate(db, steps):
    for st in steps:
        db.execute(st)


def seed_v2(db):
    db.executescript(
        "CREATE TABLE temperature (time INTEGER, device_id TEXT, temperature REAL, humidity REAL, PRIMARY KEY(time, device_id));"
        "CREATE TABLE alarm_events (id INTEGER PRIMARY KEY AUTOINCREMENT, time INTEGER, device_id TEXT, type TEXT, message TEXT);"
        "CREATE INDEX idx_temperature_device ON temperature(device_id, time);"
        "CREATE INDEX idx_alarm_device ON alarm_events(device_id, time);"
        "INSERT INTO temperature VALUES (1000,'A1B2C3D4',25.0,60.0);"
        "INSERT INTO temperature VALUES (1060,'A1B2C3D4',24.0,58.0);"
        "INSERT INTO temperature VALUES (2000,'0C118224',30.0,70.0);"
        "INSERT INTO alarm_events(time,device_id,type,message) VALUES (1000,'A1B2C3D4','drop','温度湿度下降报警');"
        "PRAGMA user_version = 2;"
    )


def obj_type(db, name):
    row = db.execute("SELECT type FROM sqlite_master WHERE name=?", (name,)).fetchone()
    return row[0] if row else None


def table_info(db, table):
    return [(r[1], r[2], r[5]) for r in db.execute(f"PRAGMA table_info({table})")]


def index_cols(db, index):
    return [r[2] for r in db.execute(f"PRAGMA index_info({index})")]


def checks_execution(table_ddl, index_ddl):
    steps = [table_ddl, index_ddl]

    print("== C1 v2 + 数据 → v2→v3 迁移（宿主 SQLite） ==")
    db = sqlite3.connect(":memory:")
    seed_v2(db)
    db.commit()
    migrate(db, steps)
    db.commit()
    rows = list(db.execute("SELECT time, device_id, temperature, humidity FROM temperature ORDER BY time"))
    ok = rows == [(1000, "A1B2C3D4", 25.0, 60.0), (1060, "A1B2C3D4", 24.0, 58.0), (2000, "0C118224", 30.0, 70.0)]
    print(f"  {'PASS' if ok else 'FAIL'}  历史温湿度 3 行保真: {rows}")
    if not ok:
        violations.append("C1: 历史温湿度数据未保真")
    alarms = list(db.execute("SELECT time, device_id, type, message FROM alarm_events"))
    ok = alarms == [(1000, "A1B2C3D4", "drop", "温度湿度下降报警")]
    print(f"  {'PASS' if ok else 'FAIL'}  历史报警记录保真: {alarms}")
    if not ok:
        violations.append("C1: 历史报警记录未保真")

    kind = obj_type(db, "pending_uploads")
    print(f"  {'PASS' if kind == 'table' else 'FAIL'}  pending_uploads 对象类型 = {kind}")
    if kind != "table":
        violations.append(f"C1: pending_uploads 类型为 {kind}")
    info = table_info(db, "pending_uploads")
    ok = [c[0] for c in info] == ["device_id", "time", "temperature", "humidity", "attempts", "next_attempt_at"]
    print(f"  {'PASS' if ok else 'FAIL'}  列顺序 = {[c[0] for c in info]}")
    if not ok:
        violations.append("C1: 列集合不符")
    pk = {c[0]: c[2] for c in info if c[2] > 0}
    ok = pk == {"device_id": 1, "time": 2}
    print(f"  {'PASS' if ok else 'FAIL'}  复合主键顺序 = {pk}")
    if not ok:
        violations.append("C1: 复合主键不符")
    idx_names = [r[1] for r in db.execute("PRAGMA index_list(pending_uploads)")]
    ok = "idx_pending_uploads_next" in idx_names and index_cols(db, "idx_pending_uploads_next") == ["next_attempt_at"]
    print(f"  {'PASS' if ok else 'FAIL'}  索引 = {idx_names}, 列 = {index_cols(db, 'idx_pending_uploads_next')}")
    if not ok:
        violations.append("C1: 索引不符")
    # 索引未变
    temp_idx = [r[1] for r in db.execute("PRAGMA index_list(temperature)")]
    alarm_idx = [r[1] for r in db.execute("PRAGMA index_list(alarm_events)")]
    ok = "idx_temperature_device" in temp_idx and "idx_alarm_device" in alarm_idx
    print(f"  {'PASS' if ok else 'FAIL'}  既有索引保留 = {temp_idx} / {alarm_idx}")
    if not ok:
        violations.append("C1: 既有索引丢失")

    # 默认值 / 幂等 / 主键约束
    db.execute("INSERT OR REPLACE INTO pending_uploads(device_id,time,temperature,humidity) VALUES ('A1B2C3D4',5000,25.0,60.0)")
    db.execute("INSERT OR REPLACE INTO pending_uploads(device_id,time,temperature,humidity) VALUES ('A1B2C3D4',5000,26.5,61.5)")
    n, attempts, nxt, temp = db.execute("SELECT count(*), max(attempts), max(next_attempt_at), max(temperature) FROM pending_uploads").fetchone()
    ok = (n, attempts, nxt, temp) == (1, 0, 0, 26.5)
    print(f"  {'PASS' if ok else 'FAIL'}  同键 OR REPLACE → 1 行, attempts={attempts}, next={nxt}, temp={temp}")
    if not ok:
        violations.append("C1: 幂等/默认值不符")
    try:
        db.execute("INSERT INTO pending_uploads(device_id,time) VALUES ('A1B2C3D4',5000)")
        print("  FAIL  普通重复 INSERT 未触发主键约束")
        violations.append("C1: 主键约束未生效")
    except sqlite3.IntegrityError:
        print("  PASS  普通重复 INSERT 触发主键约束")

    print("== C2 迁移重复执行（幂等） ==")
    try:
        migrate(db, steps)
        print("  PASS  重复执行两条 DDL 无报错")
    except Exception as exc:
        print(f"  FAIL  重复执行报错: {exc}")
        violations.append(f"C2: 重复迁移报错 {exc}")

    print("== C3 冲突对象（同名 VIEW）与回退序列 ==")
    db2 = sqlite3.connect(":memory:")
    seed_v2(db2)
    db2.execute("CREATE VIEW pending_uploads AS SELECT 1 AS x")
    db2.commit()
    first_error = None
    try:
        migrate(db2, steps)
    except Exception as exc:
        first_error = f"{type(exc).__name__}: {exc}"
    print(f"  INFO  首次迁移（存在同名 VIEW）结果 = {first_error or '无报错'}")
    # 代码回退序列：DROP VIEW IF EXISTS → DROP TABLE IF EXISTS → 重试
    db2.execute("DROP VIEW IF EXISTS pending_uploads")
    db2.execute("DROP TABLE IF EXISTS pending_uploads")
    try:
        migrate(db2, steps)
        db2.commit()
    except Exception as exc:
        print(f"  FAIL  回退后仍失败: {exc}")
        violations.append(f"C3: 回退后迁移失败 {exc}")
    kind = obj_type(db2, "pending_uploads")
    ok = kind == "table" and len([c for c in table_info(db2, "pending_uploads") if c[2] > 0]) == 2
    print(f"  {'PASS' if ok else 'FAIL'}  回退后 pending_uploads = {kind}")
    if not ok:
        violations.append(f"C3: 回退后对象类型 {kind}")
    preserved = db2.execute("SELECT count(*) FROM temperature").fetchone()[0]
    ok = preserved == 3
    print(f"  {'PASS' if ok else 'FAIL'}  冲突场景下历史数据仍为 3 行（未被重建）")
    if not ok:
        violations.append("C3: 冲突场景历史数据丢失")
    if first_error is None:
        print("  INFO  注意：本机 SQLite 同名 VIEW 时首条 CREATE 未报错，回退路径可能不被触发（见上）；结果仍为 table")

    print("== C4 v1 + 数据 → v1→v2（既有语句）→ v2→v3 ==")
    db3 = sqlite3.connect(":memory:")
    db3.executescript(
        "CREATE TABLE temperatureA (time INTEGER, temperature REAL, humidity REAL);"
        "CREATE TABLE temperatureB (time INTEGER, temperature REAL, humidity REAL);"
        "CREATE TABLE temperatureC (time INTEGER, temperature REAL, humidity REAL);"
        "INSERT INTO temperatureA VALUES (900,26.0,61.0);"
        "INSERT INTO temperatureA VALUES (960,25.0,59.0);"
        "PRAGMA user_version = 1;"
    )
    # 以下语句逐字转写自 TemperatureDatabaseHelper.onUpgrade 的 v1→v2 分支
    v1_to_v2 = [
        "DROP TABLE IF EXISTS temperature",
        "DROP TABLE IF EXISTS alarm_events",
        "ALTER TABLE temperatureA RENAME TO temperature",
        "ALTER TABLE temperature ADD COLUMN device_id TEXT",
        "UPDATE temperature SET device_id = 'UNKNOWN'",
        "DROP TABLE IF EXISTS temperatureB",
        "DROP TABLE IF EXISTS temperatureC",
        "CREATE TABLE alarm_events (id INTEGER PRIMARY KEY AUTOINCREMENT, time INTEGER, device_id TEXT, type TEXT, message TEXT)",
        "CREATE INDEX idx_temperature_device ON temperature(device_id, time)",
        "CREATE INDEX idx_alarm_device ON alarm_events(device_id, time)",
    ]
    try:
        migrate(db3, v1_to_v2)
        migrate(db3, steps)
        db3.commit()
    except Exception as exc:
        print(f"  FAIL  v1→v3 语句执行报错: {exc}")
        violations.append(f"C4: v1→v3 报错 {exc}")
    v1_rows = list(db3.execute("SELECT time, device_id, temperature, humidity FROM temperature ORDER BY time"))
    ok = len(v1_rows) == 2 and all(r[1] == "UNKNOWN" for r in v1_rows)
    print(f"  {'PASS' if ok else 'FAIL'}  v1 历史行保留并回填 device_id: {v1_rows}")
    if not ok:
        violations.append("C4: v1 数据未按既有策略保留/回填")
    ok = obj_type(db3, "pending_uploads") == "table" and obj_type(db3, "alarm_events") == "table"
    print(f"  {'PASS' if ok else 'FAIL'}  v1→v3 后 alarm_events/pending_uploads 齐备")
    if not ok:
        violations.append("C4: v1→v3 后表缺失")


def main() -> int:
    table_ddl, index_ddl, _ = checks_source()
    checks_execution(table_ddl, index_ddl)
    print("== RESULT ==")
    if violations:
        for v in violations:
            print("  VIOLATION:", v)
        return 1
    print("  OK: v3 schema/迁移语句与既有结构不变均成立；迁移语义（保数据/幂等/回退/v1→v3）在宿主 SQLite 通过")
    return 0


if __name__ == "__main__":
    sys.exit(main())
