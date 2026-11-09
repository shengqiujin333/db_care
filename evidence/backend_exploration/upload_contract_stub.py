#!/usr/bin/env python3
"""后端可行性探索：`POST /upload_data` 契约与幂等语义的**本地隔离**样板实验。

目的不是证明已部署服务的行为，而是在不触网、不写生产数据的前提下，用可复现的
最小样板固定以下三件事：

  1. 本轮 Android 侧会发出的 JSON 形状/单位/批大小与契约逐字一致；
  2. 文档声明的 `UNIQUE(dev_id,time)` + `INSERT IGNORE` 在“相同关系语义”的引擎上
     会产生哪些可观察结果（重复重试不增行、同秒不同值被静默丢弃、mac 不落库）；
  3. 由轮询节拍推出的云端行数上界与单行体积量级（仅算术 + 本地 SQLite 页统计）。

边界（务必保留）：
  * 本脚本只监听 `127.0.0.1` 的临时端口，不访问 8.140.23.253、不连 EMQX、不连 MySQL；
  * 这里的 Flask 服务端与 SQLite 都是**桩**。真实 `abr.py` 源码不在仓库中，
    真实 MySQL 的 `INSERT IGNORE` 语义（含把截断/NOT NULL 等错误降级为 warning）
    未被建模；SQLite `INSERT OR IGNORE` 只忽略约束冲突，二者不可互相外推；
  * 结论只覆盖“契约兼容性与风险模型”，不覆盖已部署服务的可达性、版本、鉴权、持久化或回读。

运行：`python evidence/backend_exploration/upload_contract_stub.py`
退出码：0 = 全部用例按桩模型执行完毕（不代表产品通过）；1 = 桩自身异常。
"""

from __future__ import annotations

import json
import os
import sqlite3
import sys
import tempfile
import threading
from datetime import datetime, timezone

from flask import Flask, jsonify, request
from werkzeug.serving import make_server

import requests

HOST = "127.0.0.1"
DEV_A = "01020304"
DEV_B = "A1B2C3D4"
PHONE = "13800000000"
GW_MAC = "84C2E4030202"

DDL = """
CREATE TABLE sensor_data (
    id BIGINT PRIMARY KEY,                -- 桩：SQLite 用 INTEGER 自增代替 AUTO_INCREMENT
    phone VARCHAR(20) NOT NULL,
    dev_id VARCHAR(8) NOT NULL,
    time BIGINT NOT NULL,
    temperature DECIMAL(6,2),
    humidity DECIMAL(6,2),
    UNIQUE (dev_id, time)                 -- 文档声明的 uk_dev_time
)
"""

# 文档声明的表没有 mac 列；桩按文档建表，用于验证“mac 不落库”的事实
INSERT = (
    "INSERT OR IGNORE INTO sensor_data (phone, dev_id, time, temperature, humidity) "
    "VALUES (?, ?, ?, ?, ?)"
)


class StubStore:
    def __init__(self) -> None:
        fd, self.path = tempfile.mkstemp(prefix="backend_exploration_", suffix=".sqlite3")
        os.close(fd)
        self.lock = threading.Lock()
        with self._conn() as conn:
            conn.execute(DDL)

    def _conn(self) -> sqlite3.Connection:
        conn = sqlite3.connect(self.path, timeout=5)
        conn.execute("PRAGMA journal_mode=WAL")
        return conn

    def insert_batch(self, phone: str, rows: list[tuple]) -> tuple[int, int]:
        inserted = 0
        with self.lock, self._conn() as conn:
            for r in rows:
                cur = conn.execute(INSERT, (phone, r["devId"], r["time"], r["temperature"], r["humidity"]))
                inserted += cur.rowcount
        return inserted, len(rows) - inserted

    def rows(self) -> list[tuple]:
        with self.lock, self._conn() as conn:
            return list(conn.execute(
                "SELECT phone, dev_id, time, temperature, humidity FROM sensor_data "
                "ORDER BY id"
            ))

    def row_count(self) -> int:
        with self.lock, self._conn() as conn:
            return conn.execute("SELECT COUNT(*) FROM sensor_data").fetchone()[0]

    def file_pages(self) -> tuple[int, int]:
        with self.lock, self._conn() as conn:
            page_count = conn.execute("PRAGMA page_count").fetchone()[0]
            page_size = conn.execute("PRAGMA page_size").fetchone()[0]
        return page_count, page_size


def build_app(store: StubStore) -> Flask:
    app = Flask(__name__)

    @app.post("/upload_data")
    def upload_data():
        # 桩校验策略（真实服务校验策略未知，不能外推）
        body = request.get_json(silent=True)
        if not isinstance(body, dict):
            return jsonify(success=False, error="json"), 400
        phone, mac, readings = body.get("phone"), body.get("mac"), body.get("readings")
        if not isinstance(phone, str) or not phone:
            return jsonify(success=False, error="phone"), 400
        if not isinstance(mac, str) or not mac:
            return jsonify(success=False, error="mac"), 400
        if not isinstance(readings, list):
            return jsonify(success=False, error="readings"), 400
        rows = []
        for r in readings:
            if not isinstance(r, dict):
                return jsonify(success=False, error="reading_type"), 400
            for key in ("devId", "time", "temperature", "humidity"):
                if key not in r:
                    return jsonify(success=False, error=key), 400
            if not isinstance(r["devId"], str) or not isinstance(r["time"], int):
                return jsonify(success=False, error="reading_field_type"), 400
            if not isinstance(r["temperature"], (int, float)) or not isinstance(r["humidity"], (int, float)):
                return jsonify(success=False, error="reading_value_type"), 400
            rows.append(r)
        inserted, ignored = store.insert_batch(phone, rows)
        return jsonify(success=True, inserted=inserted, ignored=ignored), 200

    return app


class ServerThread(threading.Thread):
    def __init__(self, app: Flask):
        super().__init__(daemon=True)
        self.httpd = make_server(HOST, 0, app)
        self.port = self.httpd.server_port

    def run(self) -> None:
        self.httpd.serve_forever()

    def stop(self) -> None:
        self.httpd.shutdown()


def post(port: int, payload, headers=None) -> tuple[int, dict, int]:
    """返回 (HTTP 状态, 响应体, 原始请求体字节数)。请求只发往 127.0.0.1。"""
    body = payload if isinstance(payload, (bytes, bytearray)) else json.dumps(
        payload, separators=(",", ":"), ensure_ascii=False
    ).encode("utf-8")
    hdrs = {"Content-Type": "application/json"}
    if headers:
        hdrs.update(headers)
    resp = requests.post(f"http://{HOST}:{port}/upload_data", data=body, headers=hdrs, timeout=5)
    try:
        parsed = resp.json()
    except ValueError:
        parsed = {"raw": resp.text[:200]}
    return resp.status_code, parsed, len(body)


def reading(dev=DEV_A, t=1699999999, temp=25.5, hum=60.0) -> dict:
    return {"devId": dev, "time": t, "temperature": temp, "humidity": hum}


def batch(n: int, start: int = 1699999000, dev=DEV_A) -> list[dict]:
    return [reading(dev=dev, t=start + i * 180, temp=25.0 + i / 10.0, hum=50.0) for i in range(n)]


def main() -> int:
    stamp = datetime.now(timezone.utc).isoformat()
    store = StubStore()
    app = build_app(store)
    srv = ServerThread(app)
    srv.start()
    try:
        print("== 本地桩实验（仅 127.0.0.1，不触外网） ==")
        print(f"time_utc={stamp}")
        print(f"python={sys.version.split()[0]} flask={__import__('flask').__version__}")
        print("stub=Flask+SQLite  UNIQUE(dev_id,time)  insert=INSERT OR IGNORE")
        print(f"bind={HOST}:{srv.port}")
        print()

        results = []

        def case(name, payload, expect_note, headers=None):
            status, body, nbytes = post(srv.port, payload, headers)
            rows = store.rows()
            print(f"-- {name}")
            print(f"   req_bytes={nbytes}  status={status}  resp={json.dumps(body, ensure_ascii=False)}")
            print(f"   table_rows={len(rows)}  last={rows[-1] if rows else None}")
            print(f"   说明: {expect_note}")
            print()
            results.append((name, status, nbytes, len(rows), body))

        # 1) 契约既有向量：负温
        case("documented_vector_negative",
             {"phone": PHONE, "mac": GW_MAC, "readings": [reading(t=1699999999, temp=-5.0, hum=60.0)]},
             "既有契约的负温向量应被接收；单位 ℃/%RH 不再二次换算")
        # 2) 本轮新增的升温分支值
        case("rise_branch_value",
             {"phone": PHONE, "mac": GW_MAC, "readings": [reading(t=1700000000, temp=31.0, hum=60.0)]},
             "升温 0.9℃ 判据在传感器侧完成；服务端只需存储该读数，不重算判据")
        # 3) 本轮新增的超温分支值
        case("high_temp_branch_value",
             {"phone": PHONE, "mac": GW_MAC, "readings": [reading(t=1700000001, temp=35.1, hum=55.0)]},
             ">35.0℃ 读数与 35.0 严格边界由传感器侧决定；服务端不解析 35.0 语义")
        # 4) 单批上限 50
        b50 = batch(50)
        case("batch_50",
             {"phone": PHONE, "mac": GW_MAC, "readings": b50},
             "Android CloudUploadRepository.DEFAULT_BATCH_LIMIT=50；整批一个 HTTP 请求")
        # 5) 相同批次重试（超时后服务端其实已入库的情形）
        before = store.row_count()
        case("retry_identical_batch",
             {"phone": PHONE, "mac": GW_MAC, "readings": b50},
             f"相同 (dev_id,time) 重试应不增行；重试前 {before} 行")
        # 6) 同一批内同键不同值
        case("same_key_in_one_batch",
             {"phone": PHONE, "mac": GW_MAC,
              "readings": [reading(t=1700000300, temp=30.0, hum=50.0), reading(t=1700000300, temp=30.1, hum=50.1)]},
             "同批内同键先到先得（30.0 保留，30.1 被忽略）；App 仍会删掉已 2xx 的待发箱行")
        # 7) 已存在键被不同值重试（缓存复读/跨路径同秒）
        case("existing_key_different_value",
             {"phone": PHONE, "mac": GW_MAC,
              "readings": [reading(t=1700000000, temp=32.7, hum=44.0)]},
             "已存在的 (01020304,1700000000) 保留原值 31.0；32.7 被静默忽略且无冲突上报")
        existing = [r for r in store.rows() if r[1] == DEV_A and r[2] == 1700000000]
        print(f"   核对: 该键当前行={existing}")
        print()
        # 8) 相邻秒（同设备两笔真实不同读数）
        case("two_rows_one_second_apart",
             {"phone": PHONE, "mac": GW_MAC,
              "readings": [reading(t=1700000600, temp=30.0, hum=50.0), reading(t=1700000601, temp=30.1, hum=50.1)]},
             "秒级粒度不能区分同秒多个真实样本；相邻秒可以共存")
        # 9) 空读数集（App 不会发；服务端策略未知）
        case("empty_readings",
             {"phone": PHONE, "mac": GW_MAC, "readings": []},
             "桩允许空集并返回 inserted=0；真实服务策略未知，不能外推")
        # 10) 缺手机号
        case("missing_phone",
             {"mac": GW_MAC, "readings": [reading(t=1700001200)]},
             "桩返回 400；真实服务对 phone 的校验策略未核验")
        # 11) 未知网关 MAC
        case("unknown_mac_not_persisted",
             {"phone": PHONE, "mac": "FFFFFFFFFFFF", "readings": [reading(t=1700001500)]},
             "文档表无 mac 列，桩中 mac 只被接收不落库；无法据此审计来源网关")
        # 12) 数值精度
        case("numeric_precision",
             {"phone": PHONE, "mac": GW_MAC, "readings": [reading(t=1700001800, temp=25.555, hum=60.004)]},
             "SQLite REAL 保留 25.555/60.004；MySQL DECIMAL(6,2) 会四舍五入为 25.56/60.00，未在本机建模")
        # 13) 非 JSON 请求体
        status, body, nbytes = post(srv.port, b"{not json")
        print("-- malformed_json")
        print(f"   req_bytes={nbytes}  status={status}  resp={json.dumps(body, ensure_ascii=False)}")
        print("   说明: 桩返回 400；真实服务在解析失败时的状态码与响应体未核验")
        print()
        results.append(("malformed_json", status, nbytes, store.row_count(), body))

        # 14) 180 s 轮询一天的行数上界 + 本地页统计（仅 SQLite，非 InnoDB）
        one_day = [reading(t=1700100000 + i * 180) for i in range(480)]
        ins, ign = store.insert_batch(PHONE, one_day)
        pages, page_size = store.file_pages()
        total_rows = store.row_count()
        print("-- volume_model_1_device_1_day")
        print(f"   180 s 节拍 → 480 次/日；本轮插入 {ins} 行，忽略 {ign} 行")
        print(f"   table_rows={total_rows}  sqlite_pages={pages} page_size={page_size} "
              f"db_bytes≈{pages * page_size}")
        print("   说明: 行数上界来自 BLE 轮询 + 网关缓存复读；体积是 SQLite 页统计，"
              "不是 InnoDB 行/索引体积")
        print()

        print("== 统计摘要 ==")
        print(f"cases={len(results)}  http_2xx={sum(1 for r in results if 200 <= r[1] < 300)}  "
              f"http_4xx={sum(1 for r in results if 400 <= r[1] < 500)}")
        print(f"rows_total={store.row_count()}  db_bytes={(pages * page_size)}")
        print()
        print("== 结论边界 ==")
        print("1. 以上仅为本地 Flask+SQLite 桩，未连接 8.140.23.253、EMQX、MySQL 或 abr.py。")
        print("2. 已证实（桩内）：契约字段/单位/50 条批可用；同键重试不增行；同键不同值静默丢弃；")
        print("   mac 不在文档表中；480 行/日/设备的节拍上界可被该表结构容纳。")
        print("3. 未证实：真实服务的端点存在性、响应码/inserted、MySQL IGNORE 全语义、鉴权/绑定、")
        print("   持久化与回读；不得用本脚本结果替代。")
        return 0
    finally:
        srv.stop()
        srv.join(timeout=5)
        try:
            os.remove(store.path)
            for suffix in ("-wal", "-shm"):
                p = store.path + suffix
                if os.path.exists(p):
                    os.remove(p)
        except OSError:
            pass


if __name__ == "__main__":
    raise SystemExit(main())
