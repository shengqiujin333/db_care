# Android 软件失败归因：AA-005 G-6 上传阻塞未全局生效（D-AA005-1）

状态：软件失败归因证据（负责角色：软件测试）
分析对象：`artifacts/android_implementation.md`（Android 实现记录，AA-005 七项增量；产品树提交 `7848b99`）
原始失败：`evidence/software_test.md` **ST-AA005**（`TEST_FAIL`，缺陷 **D-AA005-1**，`failure_cause=MEASURED_DEFECT`）
代码索引：`artifacts/codegraph_android_index.md`（源码基线 `7848b997466091b4eb9ab2c22e8543a9736ee953`，projectPath `D:\mypro\beiwo2\AIP\dengbei_care`）
本轮结论：**需 Android 实现修复**（`REPAIR_REQUIRED`）

> 本文件只做失败归因，不是复验通过，也不代表产品已修复。本轮未修改产品源码、未修改测试期望、未安装/启动 App、未访问真实服务器、未修改权限或环境配置。

---

## 1. 结论摘要

1. **原始失败可稳定复现**：在当前工作区（HEAD `eee975c`，产品树相对 `7848b99` 未变）重跑独立用例 `Aa005SevenItemVerificationTest`，结果为 **20 tests / 1 failed**；唯一失败是 `g6_authRejected_mustPauseAllAutomaticRequests_untilManualRetry`，断言原文为 `expected:<[empty]> but was:<[upload_blocked]>`（`Aa005SevenItemVerificationTest.kt:550/570`）。同一套件的其余 19 项通过。
2. **独立静态核查给出同一结论**：`evidence/software_verify_aa005_android_check.py` 本次复跑为 **53 项 PASS + 1 项 FINDING E6**（exit 0）；E6 原文为“`CloudUploadRepository`/`UploadOutbox` 未读取全局阻塞状态：401 后仅 park 本批行，其余到期行仍会在下一次 `triggerUpload` 自动上传”。
3. **故障机制已定位到具体可修改对象**：`CloudUploadRepository` 在 `AuthRejected`/`RequestRejected` 分支只把**当前批次**的行 `park`（`attempts=maxAttempts`）并调用 `gate.markUploadBlocked(...)` 写入门控状态；但 `isGateOpen()` 与 `uploadPendingOnce()` **从不读取**该阻塞状态。因此下一次自动触发（冷启动 / 每轮 BLE / 入队后 / broker 连接成功）仍会从待发箱取出**其余到期行**并再次发起 HTTP 请求。
4. **边界清楚**：这不是外部设备、网络、服务器 401 根因或测试夹具问题；即使服务器返回真实的 401/403，客户端也应在被拒后停止自动请求，直到用户手动重试。当前实现只做到了“本批行不自动重试”，没有做到验收文字与 AA-005 §6.5 要求的“**全局暂停自动请求**”。
5. **影响**：401/403/其他 4xx 后仍会产生自动 HTTP 请求（请求次数与待发行数、单批上限相关）；设置页显示“上传被拒绝……重试上传”，但后台仍在上传，用户可见状态与实际行为不一致。行被保留、不删除，**不构成数据丢失**；BLE/MQTT/告警/入库不受影响。
6. **证据分类**：本轮缺陷是**实测缺陷**（独立宿主机用例 + 静态核查 + 源码阅读），不依赖真实网络/设备；原始失败 §6 中的真实 BLE/MQTT、真实 `/upload_data`、API 31+ 等未执行项仍未执行，不能写成通过，也不是本次 `TEST_FAIL` 的原因。
7. **结果判断**：`REPAIR_REQUIRED`。修复对象为 Android 实现（`CloudUploadRepository` 及其与 `UploadOutbox`/`UploadGate` 的阻塞状态接线），本能力不修改产品源码或测试期望；修复后须回到原受影响测试重新验证。

---

## 2. 对象身份、版本与证据来源

| 项 | 值 |
|---|---|
| 分析对象 | `artifacts/android_implementation.md`（Android 实现记录，AA-005 七项增量） |
| 被测产品 | Android App `dengbei_care`（Kotlin / AGP 8.3.2 / Gradle 8.4） |
| 产品树提交 | `7848b99`（`android_engineer` 实现提交；`artifacts/codegraph_android_index.md` 声明源码基线同为此提交） |
| 分析时 HEAD | `eee975c`（`git log --oneline -3`：`eee975c` / `f77873d` / `7848b99`） |
| 产品树差异 | `git diff 7848b99..HEAD -- dengbei_care` 仅新增 `app/src/test/.../verification/Aa005SevenItemVerificationTest.kt`（618 行），**无产品源码改动**；分析时工作区 `git status --short` 为空 |
| 版本 | `versionCode 10 / versionName 1.9`（实现记录；静态核查复跑 F1 再次确认） |
| APK | `app-debug.apk` 10,365,236 B，SHA-256 `bddc698a89482e3d6f4752bb0d0fd6a58e34f9c63e93bc84307b7d41ef755b47`（静态核查 F2 本次复跑读出同一摘要；仅为构建产物身份，不是功能通过证据） |
| 原始失败证据 | `evidence/software_test.md` §ST-AA005（`TEST_FAIL`，D-AA005-1，`failure_cause=MEASURED_DEFECT`） |
| 原始失败原始文件 | `evidence/aa005_android_check/TEST-com.jinyuni.dengbei_care.verification.Aa005SevenItemVerificationTest.xml`、`evidence/aa005_android_check/continuation_rerun_20261110.txt` |
| 代码索引 | `artifacts/codegraph_android_index.md`（本次仅用于确认 Android 源码基线与路径；缺陷结论来自直接读取当前源码，不是图推断） |

**版本一致性说明**：`git diff 7848b99..HEAD -- dengbei_care` 只有测试资产新增，说明本次复现的源产品与原始失败对象是同一版本；这不是“用旧结论替代新证据”，而是同一对象的稳定复现。

---

## 3. 原始失败与本轮复现

### 3.1 原始失败（引用）

`evidence/software_test.md` ST-AA005 记录：

- 缺陷 **D-AA005-1**：`CloudUploadRepository` 在 `AuthRejected`/`RequestRejected` 时只对本批行 `park` 并写 `upload_blocked_reason`，但 `isGateOpen()`/`uploadPendingOnce()` 从不读取阻塞状态。
- 最小复现：内存待发箱 2 行均到期；`Config(batchLimit=1)`；伪 API 恒返回 `AuthRejected(401)`。第一次 `uploadPendingOnce()` → `upload_blocked`；第二次（模拟下一次自动触发）→ **再次发出请求**（调用数 2）。
- 失败断言原文：`org.junit.ComparisonFailure: 401 后不得再产生自动上传请求（其余到期行必须等待手动重试） expected:<[empty]> but was:<[upload_blocked]>`。

### 3.2 本轮复现（当前工作区，第一手）

```text
$ cd D:/mypro/beiwo2/AIP/dengbei_care
$ ./gradlew :app:testDebugUnitTest --offline \
    --tests "com.jinyuni.dengbei_care.verification.Aa005SevenItemVerificationTest"

> Task :app:testDebugUnitTest FAILED
com.jinyuni.dengbei_care.verification.Aa005SevenItemVerificationTest >
  g6_authRejected_mustPauseAllAutomaticRequests_untilManualRetry FAILED
    org.junit.ComparisonFailure at Aa005SevenItemVerificationTest.kt:550

20 tests completed, 1 failed
BUILD FAILED in 1s
GRADLE_EXIT=1
```

复跑生成的 JUnit XML（`dengbei_care/app/build/test-results/testDebugUnitTest/TEST-...xml`，timestamp `2026-11-10T13:29:14`）确认：`tests="20" skipped="0" failures="1" errors="0"`，失败消息与原始证据逐字一致：

```text
org.junit.ComparisonFailure: 401 后不得再产生自动上传请求（其余到期行必须等待手动重试）
expected:<[empty]> but was:<[upload_blocked]>
    at ...Aa005SevenItemVerificationTest.kt:570
    at ...Aa005SevenItemVerificationTest.kt:550
```

同一套件的 19 项通过，包括 `g6_authRejected_blocksTheRejectedBatch_andKeepsAllRows`、`g6_manualRetry_restoresParkedRows_andSucceeds`、`g6_uploadResultClassificationMatrix`。这说明：

- 401 分类、单批 park、手动重试恢复本身是工作的；
- 失败被精确隔离在“**被拒后其余到期行 / 后续自动触发是否仍发请求**”这一全局语义上；
- 失败不是“所有 401 相关断言都失败”的夹具问题。

### 3.3 独立静态核查复跑

```text
$ C:/Users/kason/anaconda3/python.exe evidence/software_verify_aa005_android_check.py
...
PASS  E3  401/403/4xx 进入 park，手动重试恢复
PASS  E5  设置页展示阻塞原因并提供“重试上传”
...
== 观察/FINDING ==
FIND  E6  CloudUploadRepository/UploadOutbox 未读取全局阻塞状态：401 后仅 park 本批行，
          其余到期行仍会在下一次 triggerUpload 自动上传
          （实测见 Kotlin 门禁 g6_authRejected_mustPauseAllAutomaticRequests_untilManualRetry）
== RESULT ==
OK: 53 项检查全部通过；1 项观察
SCRIPT_EXIT=0
```

> 说明：脚本的 `E3` 检查“401/403/4xx 进入 park”本身通过，说明“当前批次 park”存在；`E6` 指出的是“全局阻塞未被读取”。两者不矛盾，正好对应缺陷范围。

---

## 4. 故障机制（源码级）

### 4.1 自动路径的入口

`MqtttService` 的四个自动触发点都调用同一个 `CloudUploadRepository.triggerUpload()`（进而调用 `uploadPendingOnce()`）：

| 触发点 | 位置（当前源码） | 说明 |
|---|---|---|
| 冷启动 | `MqtttService.kt:380` | `onCreate` 中 `uploadRepository.triggerUpload()` |
| 每轮 BLE 开始 | `MqtttService.kt:399` | `startBluetoothTask` 循环中、`connectToBluetoothDevice()` 之前 |
| 入库后 | `MqtttService.kt:1187` | `uploadRepository.enqueueAndUpload(...)`；其内部 `CloudUploadRepository.kt:121` 再 `triggerUpload()` |
| broker 连接成功（网络恢复） | `MqtttService.kt:1553` | `connectToBroker.onSuccess` 中 `triggerUpload()` |
| 用户手动重试 | `MqtttService.kt:1302-1304` | `RETRY_UPLOAD` → `uploadRepository.retryBlockedNow()` |

### 4.2 阻塞状态“只写不读”

```kotlin
// CloudUploadRepository.kt:101
fun isGateOpen(): Boolean = gate.guardActive() && gate.phone().isNotBlank()

// CloudUploadRepository.kt:132-143
suspend fun uploadPendingOnce(): UploadOutcome {
    if (!isGateOpen()) return UploadOutcome(0, 0, 0, "gate_closed")
    if (!mutex.tryLock()) return UploadOutcome(0, 0, 0, "busy")
    try {
        val batch = outbox.dueBatch(config.maxAttempts, config.batchLimit)
        if (batch.isEmpty()) return UploadOutcome(0, 0, 0, "empty")
        ...
```

`isGateOpen()` 只检查“守护中 + 已注册手机号”，**没有**检查 `upload_blocked_reason` 或任何内存/持久化阻塞标志；`uploadPendingOnce()` 在取批前也没有阻塞检查。

```kotlin
// CloudUploadRepository.kt:166-173
is UploadResult.AuthRejected, is UploadResult.RequestRejected -> {
    batch.forEach { outbox.park(it, config.maxAttempts) }
    gate.markUploadBlocked(result, outbox.pendingCount())
    Log.w(TAG, "upload rejected for ${batch.size} reading(s); auto-retry paused")
    UploadOutcome(batch.size, 0, batch.size, "upload_blocked")
}
```

被拒时只把**本批**行 `park`（`attempts=maxAttempts`），并写入门控状态；本批之外的行没有变化。

```kotlin
// UploadOutbox.kt:137-143（SqliteUploadOutboxStore.dueRows）
"SELECT ... FROM $table " +
    "WHERE $colNext <= ? AND $colAttempts < ? " +
    "ORDER BY $colNext ASC, $colTime ASC, $colDevId ASC LIMIT ?"
```

取批条件只排除 `attempts >= maxAttempts` 的行；被 park 的本批行被排除，但**其余到期行**（`attempts=0`）仍会被取出。`MemStore` 的独立测试实现同构，因此测试能在不依赖 SQLite 的宿主机上复现。

```kotlin
// CloudUploadRepository.kt:221-233（PrefsUploadGate）
override fun markUploadBlocked(result: UploadResult, pendingCount: Int) {
    ...
    prefs.edit()
        .putString(KEY_UPLOAD_BLOCKED_REASON, reason)
        .putLong(KEY_UPLOAD_BLOCKED_AT, System.currentTimeMillis() / 1000)
        .putString(KEY_UPLOAD_LAST_STATUS, status?.toString() ?: "")
        .putInt(KEY_UPLOAD_PENDING_COUNT, pendingCount)
        .apply()
}
```

该状态在生产中只被设置页读取用于展示：

```kotlin
// GatewayBleSettingsDialog.kt:169-184
val pending = prefs.getInt(PrefsUploadGate.KEY_UPLOAD_PENDING_COUNT, 0)
val reason = prefs.getString(PrefsUploadGate.KEY_UPLOAD_BLOCKED_REASON, "").orEmpty()
val status = prefs.getString(PrefsUploadGate.KEY_UPLOAD_LAST_STATUS, "").orEmpty()
val base = when {
    reason == PrefsUploadGate.KEY_REASON_AUTH ->
        "云端上传被拒绝（HTTP ${status.ifBlank { "401/403" }}），数据已保留（待发 $pending 条）"
    ...
}
```

`CloudUploadRepository` / `UploadOutbox` 中没有任何对 `KEY_UPLOAD_BLOCKED_REASON` 的读取，也没有等价的内部阻塞标志检查。

### 4.3 最小复现路径（与独立测试一致）

1. 待发箱放入两行，均为 `attempts=0`、`nextAttemptAt=0`：`(A1B2C3D4, 10)`、`(0C118224, 11)`。
2. `Config(batchLimit=1)`；伪 API 的 `uploadDetailed` 恒返回 `AuthRejected(401)`。
3. 第 1 次 `uploadPendingOnce()`：取到第 1 行，HTTP 401，`park` 第 1 行（`attempts=20`），返回 `upload_blocked`；`api.calls.size=1`。
4. 时钟前进 200 s，模拟下一次自动触发；第 2 次 `uploadPendingOnce()`：`isGateOpen()` 仍为 true（阻塞状态未读），`dueBatch` 取出第 2 行（`attempts=0`），再次调用 API，返回 `upload_blocked`；`api.calls.size=2`。
5. 期望：第 2 次返回 `empty`，`api.calls.size` 保持 1，直到 `retryBlockedNow()` 清除阻塞；实际：第 2 次返回 `upload_blocked`，请求次数为 2。

这不是“第 1 行被重复上传”，而是“**第 2 行（以及任何后续到期行/新入队行）在 401 后仍被自动上传**”。这正是“全局暂停”缺失的表现。

---

## 5. 影响范围

| 维度 | 影响 | 依据 |
|---|---|---|
| 自动 HTTP 请求 | 401/403/其他 4xx 后仍会产生自动请求；请求次数随待发行数与单批上限增加，直到所有到期行被逐批 park | 源码 §4；独立测试第二次调用 `api.calls.size=2` |
| 用户可见状态 | 设置页显示“上传被拒绝……重试上传”，但后台仍自动请求，状态与行为不一致 | `GatewayBleSettingsDialog.kt:169-184` 读取阻塞状态；repository 不读 |
| 数据完整性 | 行被 park 后保留、不删除；手动重试可恢复。**不丢数据、不重复插入**（服务端 `UNIQUE(dev_id,time)` 兜底） | `UploadOutbox.park`/`resetForManualRetry`；测试断言行数保持 |
| 其他链路 | BLE、MQTT、告警、入库、通知不受影响；上传仍在独立作用域执行 | 分层结构；原始失败 §4；19/20 其他用例通过 |
| 误判风险 | 服务器 401 机制未定性前，客户端可能持续请求；但本缺陷是**客户端暂停语义**缺失，不是服务器鉴权根因 | 本缺陷的第二次请求由伪 API 返回 401 触发，与真实服务器行为无关 |
| 安全/权限 | 本缺陷不涉及权限提升、凭据泄露或绕过鉴权；`CloudApiClient` 仍无鉴权拦截器 | 静态核查 E1/E4；实现记录 §1 ITEM-005 |

---

## 6. 竞争假设与排除

| # | 假设 | 判断 | 依据 |
|---|---|---|---|
| H1 | 测试夹具 `FakeGate` 无法表示阻塞状态，因此测试本身不可能通过 | **不成立** | 阻塞状态可以由 repository 自身维护（设置于 401/4xx 分支、清除于手动重试/成功），无需依赖测试夹具新增方法；测试通过公共 `uploadPendingOnce()` 路径触发，且同套件 19 项通过；该断言在原始证据与本轮复跑中稳定失败 |
| H2 | 设计只要求“本批不重试”，不要求全局暂停 | **不成立** | 原始验收文字（ITEM-005/G-6）为“401/403 与其他 4xx **不产生自动重试请求**”；AA-005 §6.5 明确“**置全局阻塞状态**”；设置页也向用户展示“已暂停/重试上传”。实现记录自身也写了“不自动重试”，但代码未全局执行 |
| H3 | 真实服务器返回 401 才是根因，客户端行为无误 | **不成立** | 本缺陷的判定不依赖服务器：伪 API 恒返回 401，失败点是“第二次仍发出请求”；即使真实 401 重复出现，客户端也应停止自动请求 |
| H4 | 第二次请求由另一个触发点（BLE/MQTT/告警）造成，与 repository 无关 | **不成立** | 测试直接调用 `uploadPendingOnce()`；四个自动触发点全部收敛到该方法；该方法的取批前没有阻塞检查 |
| H5 | 第二行未到期/时钟不对，测试期望错误 | **不成立** | 第二行 `attempts=0`、`nextAttemptAt=0`，时钟前进 200 s；`dueBatch` 返回该行；`api.calls.size` 实测为 2；静态 SQL 过滤条件为 `next_attempt_at <= now AND attempts < maxAttempts` |
| H6 | 阻塞状态已在别处读取，只是未在测试中体现 | **不成立** | 全 `app/src/main` 搜索：`KEY_UPLOAD_BLOCKED_REASON` 只在 `PrefsUploadGate` 写入、在设置页读取；`CloudUploadRepository`/`UploadOutbox` 无读取 |
| H7 | 后续提交回归导致（不是 AA-005 候选的问题） | **不成立** | `git diff 7848b99..HEAD -- dengbei_care` 无产品源码改动；缺陷属于 `7848b99` 交付的 AA-005 候选 |
| H8 | 测试把“全局暂停”解释得过严，属于测试设计问题 | **不成立（不作为免责理由）** | 期望来自已批准任务文字与 AA-005 §6.5；测试只要求“不再自动请求 + 行保留 + 手动重试恢复”，没有要求新的产品字段或放宽既有断言。实现可在不改测试期望的前提下满足 |

---

## 7. 责任边界与结果判断

| 项 | 判断 |
|---|---|
| 故障对象 | Android App `dengbei_care` 的上传编排（`CloudUploadRepository.kt`，以及它与 `UploadOutbox`/`UploadGate` 的阻塞状态契约） |
| 是否可修改 | 是；属于 Android 实现的可修改产品源码，不涉及固件/网关/服务端/iOS/硬件 |
| 是否外部条件缺失 | **否**。缺陷可在宿主机确定性复现，不需要设备、网络、服务器或隔离账号；原始失败中的真实链路未执行项另行列明，不是本缺陷的原因 |
| 是否责任未定 | **否**。对象、版本、机制、影响与复验目标均已定位；没有需要转交的其他对象 |
| 结果 | **`REPAIR_REQUIRED`**：允许流程进入 Android 实现修复，再回到原受影响测试重新验证 |
| 不等于 | 不等于已修复、不等于 `TEST_PASS`、不等于真实 E2E 通过；原始 `TEST_FAIL` 仍然有效 |

---

## 8. 修复边界建议（交 Android 实现；本能力不执行）

### 8.1 必须满足的行为

1. **设置全局阻塞**：`AuthRejected`（401/403）与 `RequestRejected`（其他 4xx，408/429 除外）时，除 park 本批行外，必须建立可被自动路径观察到的全局阻塞状态；该状态应与现有 `upload_blocked_reason` 语义一致，并能在同一进程内立即生效。
2. **自动路径先检查阻塞**：`triggerUpload()` / `uploadPendingOnce()` 在取批和调用 API **之前**检查全局阻塞；阻塞时直接返回，不取行、不发请求。独立测试期望第二次调用的 `skipped == "empty"`（不是新增的 `"blocked"` 字符串），且 `api.calls.size` 保持 1。
3. **入队不丢数据**：`enqueueAndUpload()` 在阻塞期间仍可幂等入队（避免新读数丢失），但不得自动触发上传；新行等待手动重试。
4. **手动重试**：`retryBlockedNow()` 清除阻塞、恢复被暂停行并触发一次上传；这是解除阻塞的唯一自动/用户入口，成功后可沿用现有 `markUploadSucceeded` 清除状态。
5. **不改变兼容面**：`UploadResult` 分类、待发箱表结构与 `(dev_id, time)` 幂等键、单批 50、退避 60→1800、最多 20 次、门控（守护中 + 已注册手机号）、BLE/MQTT/告警/入库行为均保持不变。
6. **不得修改或放宽独立测试**：`Aa005SevenItemVerificationTest` 的期望与断言强度保持原样；不得通过改测试、删断言、改 `FakeGate` 期望或把 `"empty"` 改成 `"blocked"` 来“通过”。

### 8.2 建议的最小实现边界

- 在 `CloudUploadRepository` 内维护一个会话级 `blocked` 标志，并在 `AuthRejected`/`RequestRejected` 分支置位；`uploadPendingOnce()` 在 `isGateOpen()` 之后、`dueBatch()` 之前检查；`retryBlockedNow()` 与成功路径清除。
- 若希望跨进程保持阻塞语义，可同时让 `UploadGate` 暴露只读的阻塞查询（默认实现返回 false，保持既有伪实现兼容），生产 `PrefsUploadGate` 读取 `upload_blocked_reason`；但**测试夹具的兼容性不能成为不实现会话级标志的理由**。
- 不新增数据库字段、不改服务端契约、不改鉴权头；阻塞是客户端编排状态，不是服务端协议变更。

> 以上是归因后的边界建议，不是本能力对实现的替代；具体代码由 Android 实现能力编写并自检。

---

## 9. 复验目标

修复提交后，回到**原受影响测试**并保持原测试不改：

| 复验项 | 命令/对象 | 期望 |
|---|---|---|
| 原始失败用例 | `./gradlew :app:testDebugUnitTest --offline --tests "com.jinyuni.dengbei_care.verification.Aa005SevenItemVerificationTest"` | **20/20 PASS，0 failed**；`g6_authRejected_mustPauseAllAutomaticRequests_untilManualRetry` 第二次返回 `empty`、`api.calls.size=1` |
| 全量宿主机回归 | `./gradlew :app:testDebugUnitTest --offline` | 0 failures / 0 errors（以修复提交的实际套件数为准；不得低于当前 255 项，除非有明确的测试资产变更理由） |
| 独立静态核查 | `python evidence/software_verify_aa005_android_check.py` | 53 项 PASS；**FINDING E6 消失**（或改为已修复的陈述），exit 0 |
| 12 个历史门禁 | `python evidence/aa005_android_check/run_gates.py` | 12/12 exit 0；不得放宽任何既有断言 |
| 构建 | `./gradlew :app:assembleDebug --offline` | BUILD SUCCESSFUL；记录 APK 尺寸与 SHA-256 |
| 版本/对象 | `git diff` / `git rev-parse` | 修复提交只触及必要产品文件；测试期望、阈值、表结构、服务端契约未变 |
| 真实链路（另列，不因本修复自动通过） | 真实 401/403、`/upload_data`、BLE/MQTT、设备 UI | 仍需隔离测试环境；未执行即 `NOT EXECUTED`，不得写成 PASS |

**复验判定**：只有原始失败用例和全量回归通过、且 E6 不再成立，才能把本缺陷视为修复；分析完成或代码看起来合理都不算。真实链路未执行项仍保留为后续 E2E 条件，不用模拟成功替代。

---

## 10. 未执行项与证据局限

1. **真实服务器 401/403 与 `/upload_data` 未执行**：无隔离测试账号/网络；本缺陷不依赖它们，但真实鉴权机制、服务端持久化与回读仍未验证。
2. **真实 BLE 扫描/GATT/MTU/多设备批次未执行**：网关未确认上电/广播；与本缺陷无关，保留为 E2E 条件。
3. **设备 UI 截图/通知/卡片未执行**：本缺陷可由宿主机逻辑与静态核查判定；未安装/启动 App，避免连接生产 broker 产生副作用。
4. **设备端 SQLite 行级未执行**：`connectedDebugAndroidTest` 仍受 UTP 依赖缺失与无外网阻塞；本缺陷不依赖 SQLite 真实行级行为（测试使用内存待发箱）。
5. **本轮未修改产品源码、测试期望、权限或环境配置**：只新增本归因文件并维护 `file_manifest.txt`；无设备/网络操作，无后台任务遗留。
6. **APK 摘要的时点**：静态核查 F2 本次复跑读出 `bddc698a...`，与原始证据一致；这只是构建产物身份，不是本次复跑重新构建的证明，也不代表功能通过。

---

## 11. 本轮实际改动

- 新增：`evidence/software_triage.md`（本文件）。
- 更新：`file_manifest.txt`（登记本文件）。
- **未修改**：`dengbei_care/**` 产品源码、`app/src/test/**` 测试期望、`runtime/permissions.yaml`、`runtime/environment.yaml`；未安装/卸载 App、未访问设备或服务器。

---

## 12. 原始结果附录

### 12.1 本轮聚焦复跑（关键输出）

```text
> Task :app:testDebugUnitTest FAILED
com.jinyuni.dengbei_care.verification.Aa005SevenItemVerificationTest >
  g6_authRejected_mustPauseAllAutomaticRequests_untilManualRetry FAILED
    org.junit.ComparisonFailure at Aa005SevenItemVerificationTest.kt:550
20 tests completed, 1 failed
BUILD FAILED in 1s
```

### 12.2 本轮静态核查（尾部）

```text
PASS  E3  401/403/4xx 进入 park，手动重试恢复
PASS  E5  设置页展示阻塞原因并提供“重试上传”
== 观察/FINDING ==
FIND  E6  CloudUploadRepository/UploadOutbox 未读取全局阻塞状态：401 后仅 park 本批行，
          其余到期行仍会在下一次 triggerUpload 自动上传
== RESULT ==
OK: 53 项检查全部通过；1 项观察
```

### 12.3 原始失败 JUnit XML（节选）

```text
<testcase name="g6_authRejected_mustPauseAllAutomaticRequests_untilManualRetry" ...>
  <failure message="org.junit.ComparisonFailure: 401 后不得再产生自动上传请求（其余到期行必须等待手动重试） expected:&lt;[empty]&gt; but was:&lt;[upload_blocked]&gt;" ...>
    at ...Aa005SevenItemVerificationTest.kt:570
    at ...Aa005SevenItemVerificationTest.kt:550
  </failure>
</testcase>
```

### 12.4 关键源码位置

| 文件:行 | 内容 | 作用 |
|---|---|---|
| `CloudUploadRepository.kt:101` | `isGateOpen() = guardActive() && phone().isNotBlank()` | 门控只含守护/手机号，无阻塞 |
| `CloudUploadRepository.kt:132-143` | `uploadPendingOnce()`：门控 → 锁 → `dueBatch()` | 取批前无阻塞检查 |
| `CloudUploadRepository.kt:166-173` | `AuthRejected`/`RequestRejected` → `park` 本批 + `markUploadBlocked` | 只暂停本批 |
| `CloudUploadRepository.kt:221-233` | `PrefsUploadGate.markUploadBlocked` 写 `upload_blocked_reason` | 状态被写入 |
| `UploadOutbox.kt:137-143` | `dueRows` 的 `WHERE next_attempt_at <= ? AND attempts < ?` | 本批被 park 后，其余到期行仍可取 |
| `MqtttService.kt:380/399/1187/1553` | 冷启动 / 每轮 BLE / 入库后 / broker 连接成功触发 | 四个自动触发点 |
| `MqtttService.kt:1302-1304` | `RETRY_UPLOAD` → `retryBlockedNow()` | 手动重试入口 |
| `GatewayBleSettingsDialog.kt:169-184` | 展示 `upload_blocked_reason`/待发条数 | 状态被读取用于 UI，不影响自动路径 |
| `Aa005SevenItemVerificationTest.kt:544-573` | 全局暂停用例（`batchLimit=1`，两行均到期） | 本缺陷的最小独立复现 |
