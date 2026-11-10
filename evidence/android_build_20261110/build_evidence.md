# Android 构建证据（BUILD-005）

状态：构建证据（`android_engineer.android_release`）。仅证明本机构建与包身份；不证明安装、业务链路或产品验收。
交付对象：`artifacts/android_build.md`（AB-001）
源码：Git `28b78b0e46050ee4466908d55e211ca63bd18515`（构建时工作区 `git status --short` 为空；产品树与独立复验版本 `f516a73` 一致）
环境：Windows 11 amd64 / Gradle 8.4（wrapper）/ AGP 8.3.2 / Kotlin 1.9.0 / JDK 21.0.2 / `--offline`

---

## 1. 构建

```text
$ cd D:\mypro\beiwo2\AIP\dengbei_care
$ ./gradlew clean :app:assembleDebug --offline
BUILD SUCCESSFUL in ...
36 actionable tasks: 36 executed
```

原始输出：`build_debug.log`（0 error；11 条既有 warning，见日志）。

产物：

| 项 | 值 |
|---|---|
| 路径 | `dengbei_care/app/build/outputs/apk/debug/app-debug.apk` |
| 大小 | 10,365,460 B |
| SHA-256 | `d20542219faeaf4be0d73ce4650c57e3ea25bc54183ff0117a3f61c0e41bd8b6` |
| MD5 | `58de16b8fef549f004b92ffcec0104d9` |
| 包名/版本 | `com.jinyuni.dengbei_care` / `versionCode 10` / `versionName 1.9` |
| SDK | `minSdk 26` / `targetSdk 34` / `compileSdk 34` |
| 启动 Activity | `com.jinyuni.dengbei_care.MainActivity` |

原始：`build_identity.txt`、`apk_badging.txt`。

## 2. 签名核查

```text
$ apksigner verify --print-certs --verbose app-debug.apk
Verifies
Verified using v2 scheme (APK Signature Scheme v2): true
Number of signers: 1
Signer #1 certificate DN: C=US, O=Android, CN=Android Debug
Signer #1 certificate SHA-256 digest: 773e4ef63f06b85b2a1d20fd65a8df6b8dfafce1a41fcb2adddfd513b08d9255
```

原始：`apksigner_verify.txt`。签名使用项目既有 debug keystore（`app/build.gradle.kts` 的 release 也指向该配置）；无生产发布签名。

## 3. 宿主机测试（与候选同源码）

```text
$ ./gradlew :app:testDebugUnitTest --offline --rerun-tasks
BUILD SUCCESSFUL
```

| 项 | 值 |
|---|---|
| 套件 / 用例 | 32 / **257** |
| 失败 / 错误 / 跳过 | 0 / 0 / 0 |
| 含独立用例 | `Aa005SevenItemVerificationTest` 20/20（D-AA005-1 已关闭） |

原始：`unit_test.log`；独立复验记录见 `evidence/software_test.md` ST-AA005-R2（`f516a73`，TEST_PASS 限定范围）。

## 4. 未产出：release 变体（如实记录）

- `./gradlew clean :app:assembleDebug :app:assembleRelease --offline`：debug 成功；release 在 `:app:lintVitalAnalyzeRelease` 失败，原因是离线缓存缺少 lint 运行依赖（`com.android.tools.lint:lint*:31.3.2`、`intellij-core`、`uast`、`groovy`、`kotlin-reflect/stdlib`、`httpclient`、`commons-codec` 等）。
- 非离线重试一次：本机代理 `127.0.0.1:10808` 拒绝连接（`Connection refused`），无法下载；未修改构建配置、未安装系统包、未扩大网络授权。
- 结论：本次交付 debug 测试候选；release/上架构建需先准备 lint 依赖与生产签名，属后续发布条件，不在本候选范围。

原始：`build_offline_debug_release_attempt.log`（离线失败完整日志）。

## 5. 可复现性与差异说明

- 复现命令见 §1；需本机已有 Gradle 缓存（离线）。`clean` 重建的 APK SHA-256 可能因打包时间戳不同而变化；版本、源码树与功能一致。
- 独立测试 ST-AA005-R2 安装/实机验证的 APK（`6383c4a5…`）由同一产品树 `f516a73` 构建；本候选为 `28b78b0`（产品树未变）重建，字节不同。若验收要求字节级同一，须先安装本候选并重跑相应用例。

## 6. 未执行（外部条件/职责边界）

- 未使用 `android_test`/ADB 安装或启动 App，未操作手机/蓝牙/网关；真实安装、升级保留数据、真实 BLE/GATT、真实 `/upload_data` 与 401/回读、API 31+、9 台快照、UI 截图均未执行。
- 未访问服务端、未改权限/环境配置、未上架；无后台任务遗留。

## 7. 交付物

- 构建产物：`dengbei_care/app/build/outputs/apk/debug/app-debug.apk`（按仓库 `.gitignore` 不入库；以本文件摘要识别）。
- 安装/升级说明：`artifacts/android_build.md` §4。
- 证据文件：`build_debug.log`、`unit_test.log`、`build_identity.txt`、`apk_badging.txt`、`apksigner_verify.txt`、`build_offline_debug_release_attempt.log`、本文件。
