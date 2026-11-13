# Android 构建产物（AB-003）

状态：候选构建（`CANDIDATE_READY`）；**仅供安装与独立测试**，不代表真实 BLE/平台链路或产品总验收通过。
责任角色：Android 工程
产物类型：Android 安装包（APK，debug 变体）
交付源码：Git `548cc3301bdb87e00084136a0cff5fe8c63005e1`；Android 产品源码基线为实现提交 `a12d12b570d541a9d348a1b6d6846275d871c7d4`（`git diff a12d12b..HEAD -- dengbei_care` 仅新增测试侧独立验证用例，无产品源码差异）

---

## 1. 候选身份（本次声明的交付对象）

| 项 | 值 |
|---|---|
| 包名 | `com.jinyuni.dengbei_care`（应用名“爱守护”） |
| 版本 | `versionCode 10` / `versionName 1.9` |
| 变体 | `debug`（可调试；支持测试 `run-as` 数据导出） |
| SDK | `minSdk 26` / `targetSdk 34` / `compileSdk 34` |
| 构建命令 | `cd D:\mypro\beiwo2\AIP\dengbei_care && ./gradlew :app:assembleDebug --offline` |
| 构建工具 | Gradle 8.4（wrapper）/ AGP 8.3.2 / Kotlin 1.9.0 / JDK 21.0.2（Windows amd64） |
| 产物路径 | `dengbei_care/app/build/outputs/apk/debug/app-debug.apk` |
| 大小 | 10,370,964 B |
| SHA-256 | `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b` |
| MD5 | `a97ed9bef658ee89cdf6e2b49447ce69` |
| 签名 | APK Signature Scheme v2 验证通过；`C=US, O=Android, CN=Android Debug`，证书 SHA-256 `773e4ef63f06b85b2a1d20fd65a8df6b8dfafce1a41fcb2adddfd513b08d9255`（项目既有 debug keystore） |
| 交付字节与验收绑定 | **该 APK 即独立验证实际安装的字节**：`evidence/aa006_software_test_20261113/ST-AA006.md` 记录本地/设备 `base.apk` SHA-256 均为 `80232c50…`，结论 `TEST_PASS`（限定 AA-006 增量与已执行范围） |

## 2. 候选范围（明确）

包含（AA-006 N-1/N-2/N-3，独立验证 TEST_PASS 限定范围）：
- N-1 扫描权限由 fragment `ActivityResultLauncher` 发起/消费；授权后自动开始扫描；拒绝/蓝牙关闭/定位关闭提示真实原因；关闭对话框清除挂起动作；
- N-2 读取终态链路状态：`DATA_OK` / `GATEWAY_NO_DATA` / `GATEWAY_NO_DEVICE` / `READ_INCOMPLETE` / `READ_INVALID`，真实 `devCount=0` 显示“网关已连接，暂无设备（N=0）”，不再残留 `CONNECTING`；
- N-3 报告“重新生成”进度/禁用/恢复与零设备/无样本/失败提示，串行触发；
- 既有 AA-005 七项、D-AA005-1、D-01…D-04a 修复保持不变（已由前序独立验证覆盖）。

不包含 / 未验证（不得写成通过）：
- 真实有效批次 `DATA_OK`、全占位 `GATEWAY_NO_DATA`、真实多设备/MTU 边界、9 台快照：当前网关 `devCount=0`，联调阶段；
- 平台 `/upload_data` 写入/幂等/401/403/回读、API 31+ 权限分支、真实升温/超温/光强/功耗：外部条件/后续阶段；
- 有真实绑定数据时的传感器列表与报告刷新、注册/登录、真实告警下发：外部数据；
- `FW-LIGHT-DEF2-ENABLE`：人工 `APPROVED_DEFERRED`/NOT EXECUTED，属固件/需求域，**不在本候选范围**；
- `release` 变体未产出：离线缓存缺少 lint vital 依赖（`com.android.tools.lint:lint*:31.3.2` 等），非离线因本机代理 `127.0.0.1:10808` 拒绝连接；未改构建配置、未扩权；
- 生产签名/上架：无生产 keystore；本候选不发布商店、不面向最终用户分发。

## 3. 构建与自检（本能力复核）

| 检查 | 命令 | 结果 |
|---|---|---|
| 构建 | `./gradlew :app:assembleDebug --offline` | `BUILD SUCCESSFUL`（35 tasks up-to-date；产物未变化，保持被测字节） |
| 全量宿主机 | `./gradlew :app:testDebugUnitTest --offline` | `BUILD SUCCESSFUL`；结果集 **39 套件 / 300 项 / 0 失败 / 0 错误 / 0 跳过**（含测试侧独立 AA-006 21 项） |
| 包身份 | `aapt dump badging` | `com.jinyuni.dengbei_care`，`versionCode 10 / versionName 1.9`，`sdkVersion 26 / targetSdk 34`，launcher `MainActivity` |
| 签名 | `apksigner verify --print-certs --verbose` | `Verifies`；v2=true；单签名者 `Android Debug` |
| 独立验证 | `evidence/aa006_software_test_20261113/ST-AA006.md` | **TEST_PASS（限定 AA-006 增量/已执行范围）**；安装字节一致；真机 N1/N2/N3 主路径与异常路径通过，无 FATAL/ANR |
| 既有静态/门禁 | `software_verify_aa005_android_check.py` / `run_gates.py` / 布局核查 | 54/54；12/12；15/15（见独立报告与证据） |

原始证据：`evidence/android_build_20261113/`（`build.log`、`unit_test.log`、`build_identity.txt`、`test_summary.txt`、`apk_badging.txt`、`apksigner_verify.txt`、`build_evidence.md`）。

## 4. 安装/升级说明（交独立测试能力）

前置：Redmi 8A（`12f046690506`）在线；使用已登记 ADB / `android_test` 入口；本候选为 debug 签名。

- **覆盖升级（保留数据，推荐）**：`adb -s 12f046690506 install -r <app-debug.apk>`（或测试工具 install）。同签名同 `versionCode 10` 允许 `-r`；此前实测 `firstInstallTime` 保留、`lastUpdateTime` 更新，应用数据未清除。
- **全新安装**：`adb -s 12f046690506 install <app-debug.apk>`；首次启动需同意隐私政策，权限按需请求。
- **签名不一致**：若设备已有不同签名同包名应用，`-r` 会失败；只能卸载安装（**清除应用数据**），须由测试能力确认数据可丢并记录。
- **回退**：保留旧同签名 APK 可 `-r` 回退（版本相同）。
- 安装后核对 `dumpsys package` 的 `versionCode=10/versionName=1.9` 与 `base.apk` SHA-256 `80232c50…`；不得以“安装成功”代替业务验收。

## 5. 范围核对（人工决定与原失败）

1. **人工决定**：`FW-LIGHT-DEF2-ENABLE` 为 `APPROVED_DEFERRED`/NOT EXECUTED（2026-11-11T03:50:42Z），不重测、不作为返工依据；`BOT-01` 已 REASSESS 并以自动方式执行 PASS，不再重复。两者均不属本 Android 候选范围。
2. **原检查者范围错误**：`PRODUCT-ACCEPTANCE-002` §5 R4 将“无光判据未启用（人工延期）”记为“未满足”并计入整体失败，与同报告 N-1 的 `SKIP_CONFIRMED` 矛盾；`PRODUCT-ACCEPTANCE-RECHECK-002` 已纠正。本构建不把该项计入 Android 缺陷或候选验收。
3. **当前产品级状态**：最新产品级 `TEST_FAIL`（`EXTERNAL_CONDITION_UNEXECUTED`）仍有效；其依据为真实触发、跨链路、平台写入/回读、功耗、真实 BLE/上传/API31+ 等未执行项。本候选的 `TEST_PASS` 仅为 AA-006 Android 增量的限定结论，**不等于产品通过**。
4. **已知观察**（不构成本候选失败）：`O-AA006-1` 读取回调 `status != GATT_SUCCESS` 分支未显式映射 `READ_INVALID`（外层重试后 `CONNECT_FAILED`，本轮真实读取未触发）；`O-AA006-2` 守护停止后清除网关地址时首页状态条保留上一次文本；`O-AA006-3` 进度/禁用状态极短（终态 Toast 正常）；`O-AA006-4` MIUI“不再询问”仍会瞬时拉起权限 Activity 后立即返回（App 未自动重复请求）。后续迭代可评估，不影响本候选安装。

## 6. 限制

1. 构建字节可能随构建时间变化；本候选刻意保持为**独立验证实际安装的同一字节**，请勿 clean 重建后混用旧验收结论。
2. 安装与独立验收由测试能力承担；本能力未使用 `android_test`、未安装/操作手机。
3. `docs/` 随包手册 D-04b 属技术文档角色；Android 侧用户说明书已同步已修复行为。
4. 未执行项保持 NOT EXECUTED；本候选与本次构建不改变其状态。
