# Android 构建产物（AB-001）

状态：候选构建（`CANDIDATE_READY`）；**仅供安装与独立测试**，不代表实机业务链路或产品验收通过。
责任角色：Android 工程
产物类型：Android 安装包（APK，debug 变体）
被测源码：Git `28b78b0`（工作区干净；`git diff f516a73..HEAD -- dengbei_care` 为空，即产品树与独立复验通过时的 `f516a73` 一致）

---

## 1. 候选身份（本文件声明的交付对象）

| 项 | 值 |
|---|---|
| 包名 | `com.jinyuni.dengbei_care`（应用名“爱守护”） |
| 版本 | `versionCode 10` / `versionName 1.9` |
| 变体 | `debug`（`assembleDebug`） |
| 最低/目标 SDK | `minSdk 26` / `targetSdk 34`（`compileSdk 34`） |
| 构建命令 | `cd D:\mypro\beiwo2\AIP\dengbei_care && ./gradlew clean :app:assembleDebug --offline` |
| 构建工具 | Gradle 8.4（wrapper）/ AGP 8.3.2 / Kotlin 1.9.0 / JDK 21.0.2（Windows amd64） |
| 产物路径 | `dengbei_care/app/build/outputs/apk/debug/app-debug.apk` |
| 大小 | 10,365,460 B |
| SHA-256 | `d20542219faeaf4be0d73ce4650c57e3ea25bc54183ff0117a3f61c0e41bd8b6` |
| MD5 | `58de16b8fef549f004b92ffcec0104d9` |
| 签名 | APK Signature Scheme v2 验证通过；签名证书 `C=US, O=Android, CN=Android Debug`，证书 SHA-256 `773e4ef63f06b85b2a1d20fd65a8df6b8dfafce1a41fcb2adddfd513b08d9255`（项目既有 debug keystore） |
| 可调试 | 是（debug 变体；支持测试设计中的 `run-as` 数据导出） |

> 构建字节摘要随构建时间变化：同一源码树的两次 clean 构建可能得到不同 SHA-256。版本、源码树与功能内容一致；安装验收以包名/版本、签名与行为为准。

## 2. 候选范围（明确）

**包含**（与 `artifacts/android_tasks.yaml` ITEM-001…007 对应，独立测试 ST-AA005-R2 已复验）：
- BLE 权限策略与请求流程；网关 BLE 身份发现/保存/清除（`gateway_ble_addr` 与 `mac_addr` 分离）；
- BLE 批次完整转发 `time:temps:humis`；全零占位跳过与合法 `0.0/0.0`；
- 上传结果分类（2xx/401-403/其他 4xx/可重试）、全局阻塞暂停与手动重试；
- 升温规则文案与时间/节拍注释同步；
- 绑定容量 ≤9 与 MTU/返回长度读取完整性守卫；
- 既有 BLE 布局/AES/topic/`/upload_data`/SQLite v3/告警行为不变。

**不包含/未验证**：
- 真实手机安装与升级、真实 BLE 扫描/GATT/MTU/多设备、真实 `/upload_data` 与 401/403、服务端持久化回读、API 31+ 权限分支、9 台真实快照、UI 截图：由独立测试能力按 TD-SW-004 执行；本能力未安装/操作手机；
- 生产发布签名与上架：无生产 keystore，release 变体使用项目既有 debug 签名配置；本候选不上架、不分发；
- `release` 变体本次**未产出**：离线构建缺少 lint vital 依赖（`com.android.tools.lint:lint-*:31.3.2` 等不在本地缓存），非离线构建因本机代理 `127.0.0.1:10808` 拒绝连接而无法下载；未修改构建配置或扩大网络授权，详见 `evidence/android_build_20261110/build_offline_debug_release_attempt.log`。

## 3. 本机构建自检

| 检查 | 命令 | 结果 |
|---|---|---|
| 干净构建 | `./gradlew clean :app:assembleDebug --offline` | `BUILD SUCCESSFUL`（36 tasks） |
| 编译告警 | 同上 | 11 条基线告警，无编译错误（与既有基线集合一致） |
| 全量宿主机测试 | `./gradlew :app:testDebugUnitTest --offline --rerun-tasks` | **32 套件 / 257 项 / 0 失败 / 0 错误 / 0 跳过**（含独立 `Aa005SevenItemVerificationTest` 20/20） |
| 包身份 | `aapt dump badging` | `package=com.jinyuni.dengbei_care versionCode=10 versionName=1.9`，`sdkVersion 26/targetSdk 34`，launchable `MainActivity` |
| 签名 | `apksigner verify --print-certs --verbose` | `Verifies`；v2=true；单签名者 `Android Debug`（证书摘要见 §1） |

原始证据：`evidence/android_build_20261110/`（`build_debug.log`、`unit_test.log`、`build_identity.txt`、`apk_badging.txt`、`apksigner_verify.txt`、`build_evidence.md`）。

## 4. 安装/升级说明（交独立测试能力执行）

前置：Redmi 8A（`12f046690506`）在线；使用已登记 ADB/`android_test` 入口；本候选为 debug 签名。

- **覆盖升级（保留数据，推荐）**：设备上若已安装同签名（Android Debug）的 `com.jinyuni.dengbei_care`，执行
  `adb -s 12f046690506 install -r <app-debug.apk>`（或测试工具的 install 动作）。
  同 `versionCode 10` + 同签名允许 `-r` 覆盖；应用数据（SQLite、MyPrefs）保留。安装后核对 `dumpsys package` 的 `versionCode=10`、`versionName=1.9` 与 APK SHA-256。
- **全新安装**：`adb -s 12f046690506 install <app-debug.apk>`；首次启动会要求隐私政策同意，权限按需请求（通知 API33+；蓝牙直连 API31+；扫描另请求）。
- **签名不一致时**：若设备上是不同签名的同包名应用，`-r` 会失败；只能卸载后安装（**会清除应用数据**）。执行前须由测试能力确认数据是否可丢并记录。
- **回退**：保留上一构建 APK 即可 `-r` 回退（同签名、versionCode 相同）；不要依赖应用内回退。
- **禁止**：不使用本候选上架/分发；不安装到生产用户；不修改设备签名或系统权限。

## 5. 测试衔接与限制

1. 独立复验 `ST-AA005-R2`（`evidence/software_test.md`）在 `f516a73` 上 TEST_PASS（限定范围），其安装/实机测试使用的 APK SHA-256 为 `6383c4a5…`；本候选为同一产品树在 `28b78b0` 的重建，版本与功能一致但构建字节不同。若验收要求“被安装字节=被测字节”，请安装本候选并重跑相应实机/宿主机用例后再判定。
2. 真实 BLE/服务端链路与未执行项（多设备、占位/0.0、真实 401/回读、API31+、9 台）仍为 NOT EXECUTED，构建成功与本候选均不改变这些结论。
3. 本能力未使用 `android_test`、未安装/启动 App、未访问服务端；真实手机安装与独立验收由测试能力承担。
