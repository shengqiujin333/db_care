# Android 集成调试修复（FIX-AA005-1）

状态：修复已提交（`FIX_SUBMITTED`）；待独立测试按原受影响用例复验。**不代表**已 TEST_PASS，也不代表真实链路通过。
责任角色：Android 工程（Android 集成调试）
缺陷：**D-AA005-1** —— 401/403/其他 4xx 被拒后未全局暂停自动上传
原始失败：`evidence/software_test.md` §ST-AA005（`TEST_FAIL`，`failure_cause=MEASURED_DEFECT`）
失败归因：`evidence/software_triage.md`（`REPAIR_REQUIRED`，对象/机制/复验目标已定位）
修复对象：Android App `dengbei_care`（仅客户端上传编排；不含固件/网关/服务端/iOS/硬件）
被测基线：失败产品树 `7848b99`（AA-005 七项增量）；本次修复前 APK sha256 `bddc698a89482e3d6f4752bb0d0fd6a58e34f9c63e93bc84307b7d41ef755b47`
本次修复后产物：APK 10,365,496 B，sha256 `db183867632b4a1a3655413ca03481e9b0b4928cb9d425386089d629fde0fd67`，md5 `8e3a1b1e77997fc0aa45b4ec32e2102a`；`versionCode 10 / versionName 1.9`（未改版本，仍为同一交付候选）
修复工作区基线：HEAD `eff48ed`（接手时工作区干净）；产品变更仅 `cloud/CloudUploadRepository.kt` 与本实现侧回归用例，由 Runtime 完成交接提交。

---

## 1. 原始失败（保持原文）

| 项 | 内容 |
|---|---|
| 用例 | `verification/Aa005SevenItemVerificationTest.g6_authRejected_mustPauseAllAutomaticRequests_untilManualRetry` |
| 断言原文 | `401 后不得再产生自动上传请求（其余到期行必须等待手动重试） expected:<[empty]> but was:<[upload_blocked]>` |
| 复现 | 内存待发箱 2 行均到期；`Config(batchLimit=1)`；伪 API 恒返回 `AuthRejected(401)`；第 1 次 `uploadPendingOnce()` 返回 `upload_blocked`（1 次请求）；第 2 次（模拟下一次自动触发）仍发出请求（调用数 2） |
| 验收依据 | `artifacts/android_tasks.yaml` ITEM-005（G-6）“401/403 与其他 4xx **不产生自动重试请求**”；`artifacts/android_architecture.md` §6.5“置**全局阻塞状态**”；设置页向用户展示“上传被拒绝 / 重试上传” |
| 静态佐证 | `evidence/software_verify_aa005_android_check.py` FINDING E6：`CloudUploadRepository`/`UploadOutbox` 不读取阻塞状态 |

## 2. 根因（源码级）

`CloudUploadRepository` 在 `AuthRejected`/`RequestRejected` 分支只：
1) 对**当前批次**执行 `outbox.park(...)`（`attempts = maxAttempts`）；
2) 调用 `gate.markUploadBlocked(...)` 写入 `MyPrefs`（仅设置页读取展示）。

但 `isGateOpen()` 只检查“守护中 + 已注册手机号”，`uploadPendingOnce()` 在取批前**没有**读取阻塞状态；`enqueueAndUpload()`/`triggerUpload()` 也照常触发。因此单批上限之外的到期行（以及稍后到期的行、新入队触发的行）会在下一次自动触发时继续发出 HTTP 请求，与“全局暂停”语义和用户可见状态不一致。数据未丢失（行被保留）。

## 3. 修复内容

仅修改 `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/cloud/CloudUploadRepository.kt`（+ 回归用例），共 31 行产品源码变更：

| 变更点 | 内容 |
|---|---|
| `UploadGate` 新增只读查询 | `fun isUploadBlocked(): Boolean = false`（默认 false，既有伪实现/FakeGate 无需修改，兼容独立测试） |
| 会话级阻塞标志 | `@Volatile private var globalBlocked: Boolean = false`；`fun isUploadBlocked(): Boolean = globalBlocked \|\| gate.isUploadBlocked()` |
| 被拒时置位 | `AuthRejected`/`RequestRejected` 分支：`globalBlocked = true`，仍 `park` 本批并 `gate.markUploadBlocked(...)`（保留原有可观测状态） |
| 自动路径前置检查 | `uploadPendingOnce()` 在 `isGateOpen()` 之后、`mutex.tryLock()`/`dueBatch()`/API 调用**之前**：若 `isUploadBlocked()` → 直接返回 `UploadOutcome(0,0,0,"empty")`（**不取行、不发请求**）。与独立用例期望的 `"empty"` 一致 |
| 入队不丢数据 | `enqueueAndUpload()` 在阻塞期间仍幂等入队，但**不** `triggerUpload()`；新行等待手动重试 |
| 解除阻塞 | `retryBlockedNow()`：`globalBlocked = false` → `gate.clearUploadBlocked()` → `resetForManualRetry(...)` → 立即触发一次上传 |
| 成功清除 | `Success` 分支：`globalBlocked = false`；沿用 `gate.markUploadSucceeded(...)` 清除持久化原因 |
| 生产门控实现 | `PrefsUploadGate.isUploadBlocked()` 读取 `upload_blocked_reason` 非空（跨进程/冷启动保持暂停语义；手动重试后清除） |

未改：`UploadResult` 分类、`/upload_data` 契约与鉴权（仍无）、`pending_uploads` 表结构与 `(dev_id,time)` 幂等键、单批 50、退避 60→1800 s、最多 20 次、上传门控条件、BLE/MQTT/告警/入库行为、UI 文案、版本号、所有独立测试期望与门禁脚本。

## 4. 本次自测证据（实现侧）

证据目录：`evidence/android_fix_20261110/`。

| 检查 | 命令/对象 | 结果 |
|---|---|---|
| 原受影响用例（独立测试，未修改） | `./gradlew :app:testDebugUnitTest --offline --tests "com.jinyuni.dengbei_care.verification.Aa005SevenItemVerificationTest"` | **20 tests / 0 failures / 0 errors**（失败用例消失）；XML 副本 `TEST-Aa005SevenItemVerificationTest.xml` |
| 全量宿主机回归 | `./gradlew :app:testDebugUnitTest --offline --rerun-tasks` | **32 套件 / 257 项 / 0 失败 / 0 错误 / 0 跳过**（255 项既有 + 2 项本轮实现侧回归，见下） |
| 独立静态核查 | `python evidence/software_verify_aa005_android_check.py` | **54/54 PASS，0 FINDING**（E6 已变为 `PASS 上传编排读取全局阻塞状态`） |
| 独立门禁 | `python evidence/aa005_android_check/run_gates.py` | **12/12 EXIT=0** |
| 构建 | `./gradlew clean :app:assembleDebug --offline` | `BUILD SUCCESSFUL`；APK 身份见上；`aapt dump badging`：`versionCode 10 / versionName 1.9 / minSdk 26 / targetSdk 34` |

新增实现侧回归（`CloudUploadFailureClassificationTest`，与独立用例同形，不改独立资产）：
1. `globalBlocked_stopsAllAutomaticUploads_untilManualRetry`——单批 1 行 401 后，第二次自动触发返回 `empty`、请求数保持 1、行保留；手动重试解除阻塞。
2. `blockedState_stillEnqueuesNewReadings_withoutAutomaticRequest`——阻塞期间新读数仍入队、不自动请求。

原始文件：`unit_test_full.log`、`static_aa005.log`、`gates.log`、`build.log`、`test_summary.txt`、`apk_identity.txt`、`apk_badging.txt`。

## 5. 复现与复验方法（交独立测试）

宿主机（确定性，本缺陷的主要判定）：
```text
cd D:\mypro\beiwo2\AIP\dengbei_care
./gradlew :app:testDebugUnitTest --offline --tests "com.jinyuni.dengbei_care.verification.Aa005SevenItemVerificationTest"
./gradlew :app:testDebugUnitTest --offline --rerun-tasks
C:/Users/kason/anaconda3/python.exe evidence/software_verify_aa005_android_check.py
C:/Users/kason/anaconda3/python.exe evidence/aa005_android_check/run_gates.py
```
预期：焦点用例 20/20；全量 0 失败（不低于 255 项）；静态 54/54 且 E6 不再出现；12/12 门禁。

真机（本次未执行，需隔离测试环境；不得用模拟替代）：
1. 安装本修复后的 APK（sha256 见上）。
2. 在隔离账号/网络下制造真实 401/403（或经获准测试入口构造），触发一次上传。
3. 观察设置页“上传被拒绝（HTTP 401/403），数据已保留（重试上传）”；此后等待多个 BLE/网络恢复周期，**不得**再出现 `/upload_data` 自动请求（logcat 无 `CloudUpload.*uploaded`，无 HTTP 调用）。
4. 点“重试上传”后清除阻塞并立即尝试一次；成功则待发条数下降、失败重新分类。
5. 真实 `/upload_data` 持久化与回读、真实 BLE/MQTT、API 31+ 权限仍属 NOT EXECUTED（外部条件）。

## 6. 剩余边界与未执行项

1. **真实 401/403 与 `/upload_data` 未执行**：手机无默认网络、无隔离账号；本缺陷由宿主机独立用例判定，不依赖服务器，但真实链路仍未验证。
2. **真实 BLE 扫描/GATT/MTU/多设备批次/占位/9 台边界、设备 UI 截图未执行**：与 D-AA005-1 无关，保持原 ST-AA005 §6 的 NOT EXECUTED 记录。
3. **未修改独立测试期望或门禁脚本**：本轮只改产品代码与本实现侧回归；`Aa005SevenItemVerificationTest` 与 12 个门禁保持原样并已复跑。
4. **观察项 O-AA005-1（扫描权限提示原因）与 O-AA005-3（说明书版本措辞）未在本轮修复范围内**，保持原测试观察记录，不因本修复宣称已解决。
5. 本次未连接手机/网关、未安装 App、未访问服务端；未修改权限/环境配置；无后台任务遗留。
