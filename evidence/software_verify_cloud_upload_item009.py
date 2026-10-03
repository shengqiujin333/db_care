#!/usr/bin/env python3
"""独立静态验证（software_tester.software_verification / ITEM-009；TD-SW-002 T-SW-L0-12）。

  A 参数集中：`CloudUploadRepository.Config` 默认值 = 单批 50 / 退避 60→1800 / 最多 20，且这些字面量
    只在常量声明处出现（不在业务逻辑里散落）。
  B SQLite 适配器语义：`INSERT OR IGNORE`（幂等键）+ 到期过滤（`next_attempt_at <= ?` 且
    `attempts < ?`）+ 排序（`next_attempt_at, time, device_id`）+ `LIMIT`；删除/更新按 `(device_id, time)`；
    全部包在 `runCatching`（表缺失/异常降级不抛）。
  C 门控顺序：`enqueueAndUpload` 先判门控再入队再触发；`uploadPendingOnce` 先判门控再 `tryLock` 再取批。
  D 失败与并发：`api.upload` 包 try/catch；`Mutex.tryLock` + `finally unlock`；上传在独立
    `CoroutineScope(SupervisorJob() + Dispatchers.IO)`（不阻塞调用方）。
  E 生产接线未启用（属后续队列项）：`MqtttService` 不得引用本模块（本项不改变运行行为）。
  F 生产客户端：`CloudApiClient` 基址取自 `CloudConfig.HTTP_BASE_URL`；`RetrofitUploadApi` 用
    `ApiService.uploadData` 且以 `isSuccessful` 判成功；门控键为 `measure_state=="stopping"`/`registerphone`/`mac_addr`。

运行：`python evidence/software_verify_cloud_upload_item009.py`；退出码 0/1。
"""

from __future__ import annotations

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
APP = "dengbei_care/app/src/main/java/com/jinyuni/dengbei_care"
OUTBOX = f"{APP}/cloud/UploadOutbox.kt"
REPO = f"{APP}/cloud/CloudUploadRepository.kt"
MQTT = f"{APP}/MqtttService.kt"

violations: list[str] = []


def read(rel):
    with open(os.path.join(ROOT, rel), encoding="utf-8") as fh:
        return fh.read().replace("\r\n", "\n")


def check(repo, outbox, mqtt):
    print("== A 参数集中 ==")
    expected = {
        "DEFAULT_BATCH_LIMIT": "50",
        "DEFAULT_INITIAL_BACKOFF_SECONDS": "60L",
        "DEFAULT_MAX_BACKOFF_SECONDS": "1800L",
        "DEFAULT_MAX_ATTEMPTS": "20",
    }
    for name, want in expected.items():
        m = re.search(rf"const val {name} = ([^\n]+)$", repo, re.M)
        got = m.group(1).strip() if m else None
        ok = got == want
        print(f"  {'PASS' if ok else 'FAIL'}  {name} = {got} (want {want})")
        if not ok:
            violations.append(f"A: {name}={got}")
    # 业务逻辑中不得再用这些魔数（除常量声明与注释）
    code = "\n".join(
        l.split("//")[0] for l in repo.split("\n") if not l.strip().startswith(("//", "*", "/*"))
    )
    stray = []
    for lit in ("50", "1800L", "20"):
        for m in re.finditer(rf"(?<![\w.]){re.escape(lit)}(?![\w.])", code):
            line = code[:m.start()].count("\n") + 1
            text = code.split("\n")[line - 1]
            if "const val DEFAULT_" in text or "maxAttempts:" in text or "DEFAULT_" in text:
                continue
            stray.append((lit, line, text.strip()[:60]))
    ok = not stray
    print(f"  {'PASS' if ok else 'FAIL'}  业务逻辑中无散落魔数（stray={stray[:3]}）")
    if not ok:
        violations.append(f"A: 散落魔数 {stray[:3]}")

    print("== B SQLite 适配器语义 ==")
    checks = {
        "B1 幂等插入 INSERT OR IGNORE": "INSERT OR IGNORE INTO $table" in outbox,
        "B2 到期过滤 next_attempt_at <= ? AND attempts < ?":
            "$colNext <= ? AND $colAttempts < ?" in outbox,
        "B3 排序 next_attempt_at, time, device_id":
            "ORDER BY $colNext ASC, $colTime ASC, $colDevId ASC" in outbox,
        "B4 LIMIT 绑定": "LIMIT ?" in outbox,
        "B5 删除按 (dev_id, time)": "DELETE FROM $table WHERE $colDevId = ? AND $colTime = ?" in outbox,
        "B6 更新尝试次数/下次时间":
            "UPDATE $table SET $colAttempts = ?, $colNext = ? WHERE $colDevId = ? AND $colTime = ?" in outbox,
        "B7 先查后插（事务内幂等）": "if (exists(db, row.devId, row.time))" in outbox,
    }
    for name, ok in checks.items():
        print(f"  {'PASS' if ok else 'FAIL'}  {name}")
        if not ok:
            violations.append(f"B: {name}")
    degrade = len(re.findall(r"runCatching \{", outbox))
    ok = degrade >= 5 and "getOrElse" in outbox
    print(f"  {'PASS' if ok else 'FAIL'}  降级容错：runCatching×{degrade} + getOrElse")
    if not ok:
        violations.append("B: 适配器缺少降级容错")

    print("== C 门控顺序 ==")
    body = re.search(r"fun enqueueAndUpload\((.*?)\n    \}", repo, re.S)
    b = body.group(0) if body else ""
    gate_idx = b.find("if (!isGateOpen())")
    enq_idx = b.find("outbox.enqueue")
    trig_idx = b.find("triggerUpload()")
    ok = 0 <= gate_idx < enq_idx < trig_idx
    print(f"  {'PASS' if ok else 'FAIL'}  enqueueAndUpload：门控({gate_idx}) → 入队({enq_idx}) → 触发({trig_idx})")
    if not ok:
        violations.append("C: enqueueAndUpload 顺序异常")
    up = re.search(r"suspend fun uploadPendingOnce\(\): UploadOutcome \{(.*?)\n    \}", repo, re.S)
    u = up.group(0) if up else ""
    g2, lock2, batch2 = u.find("if (!isGateOpen())"), u.find("mutex.tryLock()"), u.find("outbox.dueBatch")
    ok = 0 <= g2 < lock2 < batch2
    print(f"  {'PASS' if ok else 'FAIL'}  uploadPendingOnce：门控({g2}) → tryLock({lock2}) → 取批({batch2})")
    if not ok:
        violations.append("C: uploadPendingOnce 顺序异常")
    ok = "gate.guardActive() && gate.phone().isNotBlank()" in repo
    print(f"  {'PASS' if ok else 'FAIL'}  isGateOpen = 守护中 AND 手机号非空")
    if not ok:
        violations.append("C: isGateOpen 语义")

    print("== D 失败与并发 ==")
    checks2 = {
        "D1 api.upload 包 try/catch": "try {\n                api.upload(payload)" in repo,
        "D2 Mutex.tryLock": "mutex.tryLock()" in repo,
        "D3 finally unlock": "finally {\n            mutex.unlock()" in repo,
        "D4 独立作用域 SupervisorJob + Dispatchers.IO":
            "CoroutineScope(SupervisorJob() + Dispatchers.IO)" in repo,
        "D5 触发上传用 scope.launch（不阻塞调用方）": "fun triggerUpload(): Job = scope.launch { uploadPendingOnce() }" in repo,
        "D6 失败逐行 markFailed + backoffSeconds": "outbox.markFailed(it, config.backoffSeconds(it.attempts + 1))" in repo,
        "D7 成功逐行 ack": "batch.forEach { outbox.ack(it) }" in repo,
    }
    for name, ok in checks2.items():
        print(f"  {'PASS' if ok else 'FAIL'}  {name}")
        if not ok:
            violations.append(f"D: {name}")

    print("== E 生产接线未启用（不得改变运行行为） ==")
    refs = [l for l in mqtt.split("\n") if ("CloudUploadRepository" in l or "UploadOutbox" in l)]
    ok = not refs
    print(f"  {'PASS' if ok else 'FAIL'}  MqtttService 引用数 = {len(refs)}（期望 0；接线属后续队列项）")
    if not ok:
        violations.append(f"E: MqtttService 已接线 {refs[:2]}")

    print("== F 生产客户端与门控键 ==")
    checks3 = {
        "F1 基址取自 CloudConfig.HTTP_BASE_URL": "baseUrl(CloudConfig.HTTP_BASE_URL)" in repo,
        "F2 RetrofitUploadApi 用 uploadData + isSuccessful":
            "apiService.uploadData(data).isSuccessful" in repo,
        "F3 门控键 measure_state/stopping": '"measure_state"' in repo and '"stopping"' in repo,
        "F4 手机号键 registerphone": '"registerphone"' in repo,
        "F5 网关 MAC 键 mac_addr": '"mac_addr"' in repo,
    }
    for name, ok in checks3.items():
        print(f"  {'PASS' if ok else 'FAIL'}  {name}")
        if not ok:
            violations.append(f"F: {name}")


def main() -> int:
    check(read(REPO), read(OUTBOX), read(MQTT))
    print("== RESULT ==")
    if violations:
        for v in violations:
            print("  VIOLATION:", v)
        return 1
    print("  OK: 参数集中、SQL 幂等/到期/排序正确、门控优先、失败与并发受控、未接线且客户端契约正确")
    return 0


if __name__ == "__main__":
    sys.exit(main())
