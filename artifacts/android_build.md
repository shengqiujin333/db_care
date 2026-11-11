# Android 构建产物（AB-002）

状态：候选构建（`CANDIDATE_READY`）；**仅供安装与独立测试**，不代表真实 BLE/平台链路或产品总验收通过。
责任角色：Android 工程
产物类型：Android 安装包（APK，debug 变体）
交付源码：Git `ab72db91db2c1100de44f5df6a9a9e173d2e44d7`；产品树与修复提交 `dacf888` 一致（`git diff dacf888..HEAD -- dengbei_care` 为空）

---

## 1. 候选身份（本文件声明的交付对象）

| 项 | 值 |
|---|---|
| 包名 | `com.jinyuni.dengbei_care`（应用名“爱守护”） |
| 版本 | `versionCode 10` / `versionName 1.9` |
| 变体 | `debug`（`assembleDebug`；可调试，支持测试 `run-as` 数据导出） |
| SDK | `minSdk 26` / `targetSdk 34` / `compileSdk 34` |
| 构建命令 | `cd D:\mypro\beiwo2\AIP\dengbei_care && ./gradlew :app:assembleDebug --offline` |
| 构建工具 | Gradle 8.4（wrapper）/ AGP 8.3.2 / Kotlin 1.9.0 / JDK 21.0.2（Windows amd64） |
| 产物路径 | `dengbei_care/app/build/outputs/apk/debug/app-debug.apk` |
| 大小 | 10,365,464 B |
| SHA-256 | `71a041d322368c20a181d5716c44ac4d1b2c8a3869926b0047cd96b61e2c10d8` |
| MD5 | `1e17e0c281715fddd26d308246d6424c` |
| 签名 | APK Signature Scheme v2 验证通过；`C=US, O=Android, CN=Android Debug`，证书 SHA-256 `773e4ef63f06b85b2a1d20fd65a8df6b8dfafce1a41fcb2adddfd513b08d9255`（项目既有 debug keystore） |
| 交付字节与验收绑定 | **该 APK 即独立复验实际安装的字节**：`evidence/android_check_20261111/SOFTWARE-TEST-ANDROID-RECHECK-001.md` 记录本地/设备 `base.apk` SHA-256 均为 `71a041d3…`，结论 `TEST_PASS`（限定范围） |

## 2. 候选范围（明确）

包含（与 `artifacts/android_tasks.yaml` ITEM-001…007 及后续修复对应，均已独立复验/回归）：
- BLE 权限策略与请求流程；网关 BLE 身份发现/保存/清除（`gateway_ble_addr` 与 `mac_addr` 分离）；
- BLE 批次完整转发；全零占位跳过与合法 `0.0/0.0`；
- 上传结果分类、401/403/4xx 全局暂停与手动重试；升温文案/时间注释；
- 绑定容量 ≤9 与 MTU/返回长度读取完整性；
- 产品验收 D-01/D-02（底部入口可达）、D-03（控件无重叠）、D-04a（首页路径文案）。

不包含/未验证（不得写成通过）：
- 真实 BLE/GATT/MTU、网关发现与配对、真实 MQTT/`/upload_data`、401/403、持久化回读、API 31+ 权限、9 台真实快照、传感器列表“有数据时”删除、调温/光强/功耗：由后续联调/测试阶段执行，当前保持 NOT EXECUTED；
- `docs/` 随包手册 D-04b（“无 UI 路径”等旧表述）：技术文档角色负责，不在本构建范围；
- 生产发布签名/上架：无生产 keystore，本候选不发布商店、不分发用户；
- `release` 变体未产出：离线缓存缺 lint vital 依赖（`com.android.tools.lint:lint*:31.3.2` 等），非离线因本机代理 `127.0.0.1:10808` 拒绝连接无法下载；未改构建配置、未扩权。

## 3. 构建与自检（本能力复核）

| 检查 | 命令 | 结果 |
|---|---|---|
| 构建 | `./gradlew :app:assembleDebug --offline` | `BUILD SUCCESSFUL`（35 tasks，全部 up-to-date：产物未变化，保持被测字节） |
| 全量宿主机 | `./gradlew :app:testDebugUnitTest --offline` | `BUILD SUCCESSFUL`（up-to-date）；结果集 **33 套件 / 261 项 / 0 失败 / 0 错误 / 0 跳过** |
| 包身份 | `aapt dump badging` | `com.jinyuni.dengbei_care`，`versionCode 10 / versionName 1.9`，`sdkVersion 26 / targetSdk 34`，launcher `MainActivity` |
| 签名 | `apksigner verify --print-certs --verbose` | `Verifies`；v2=true；单签名者 `Android Debug` |
| 独立真机复验 | `evidence/android_check_20261111/SOFTWARE-TEST-ANDROID-RECHECK-001.md` | **TEST_PASS（限定范围）**；安装字节与本地一致，D-01/D-02/D-03/D-04a 回归通过，无 FATAL/ANR |

原始证据：`evidence/android_build_20261111/`（`build.log`、`unit_test.log`、`build_identity.txt`、`apk_badging.txt`、`apksigner_verify.txt`、`build_evidence.md`）；既有重建证据：`evidence/product_android_rebuild_20261111/`。

## 4. 安装/升级说明（交独立测试能力）

前置：Redmi 8A（`12f046690506`）在线；使用已登记 ADB / `android_test` 入口；本候选为 debug 签名。

- **覆盖升级（保留数据，推荐）**：`adb -s 12f046690506 install -r <app-debug.apk>`（或测试工具 install）。
  同签名、同 `versionCode 10` 允许 `-r` 覆盖；实测 `firstInstallTime` 保留、`lastUpdateTime` 更新，应用数据未清除。
- **全新安装**：`adb -s 12f046690506 install <app-debug.apk>`；首次启动需同意隐私政策，权限按需请求。
- **签名不一致**：若设备已有不同签名同包名应用，`-r` 会失败；只能卸载安装（**清除应用数据**），须由测试能力确认数据可丢并记录。
- **回退**：保留旧同签名 APK 可 `-r` 回退（版本相同）。
- 安装后核对 `dumpsys package` 的 `versionCode=10/versionName=1.9` 与 `base.apk` SHA-256 `71a041d3…`；不得以“安装成功”代替业务验收。

## 5. 限制与交接

1. 构建字节可能随构建时间变化；本候选刻意保持为**独立复验已安装的同一字节**，请勿 clean 重建后混用旧验收结论。
2. 真实跨端链路、平台写入/回读与需外部数据的项目保持 NOT EXECUTED；本候选不改变这些结论。
3. `docs/` 手册陈旧表述（D-04b）仍在技术文档域；Android 侧用户说明书已同步修复后行为，有数据时删除仍标注“尚待实机验证”。
4. 本能力未使用 `android_test`、未安装/操作手机；安装与独立验收由测试能力承担。
