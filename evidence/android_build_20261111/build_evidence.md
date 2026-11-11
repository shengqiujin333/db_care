# Android 构建证据（BUILD-006）

状态：构建证据（`android_engineer.android_release`）。仅证明本机构建/包身份与可安装性；不证明真实链路或产品总验收。
交付对象：`artifacts/android_build.md`（AB-002）
源码：Git `ab72db91db2c1100de44f5df6a9a9e173d2e44d7`（构建/核对时工作区干净）；产品树与修复提交 `dacf888` 一致（`git diff dacf888..HEAD -- dengbei_care` 为空）
环境：Windows 11 amd64 / Gradle 8.4（wrapper）/ AGP 8.3.2 / Kotlin 1.9.0 / JDK 21.0.2 / `--offline`

---

## 1. 构建

```text
$ cd D:\mypro\beiwo2\AIP\dengbei_care
$ ./gradlew :app:assembleDebug --offline
BUILD SUCCESSFUL in 824ms
35 actionable tasks: 35 up-to-date
```

说明：产物与上一轮重建字节一致（未触发重打包），因此候选保持为独立复验实际安装的同一 APK。
原始输出：`build.log`。

| 项 | 值 |
|---|---|
| 路径 | `dengbei_care/app/build/outputs/apk/debug/app-debug.apk` |
| 大小 | 10,365,464 B |
| SHA-256 | `71a041d322368c20a181d5716c44ac4d1b2c8a3869926b0047cd96b61e2c10d8` |
| MD5 | `1e17e0c281715fddd26d308246d6424c` |
| 版本 | `versionCode 10` / `versionName 1.9` |
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
Signer #1 certificate SHA-256: 773e4ef63f06b85b2a1d20fd65a8df6b8dfafce1a41fcb2adddfd513b08d9255
```

原始：`apksigner_verify.txt`。项目既有 debug keystore；无生产发布签名，不上架。

## 3. 宿主机回归（与候选同源码）

```text
$ ./gradlew :app:testDebugUnitTest --offline
BUILD SUCCESSFUL in 726ms
25 actionable tasks: 25 up-to-date
```

结果集：**33 套件 / 261 项 / 0 失败 / 0 错误 / 0 跳过**（含 AA-005 七项、D-AA005-1、布局结构回归）。
原始：`unit_test.log`、`test_summary.txt`。

## 4. 独立真机复验（复用有效证据）

`evidence/android_check_20261111/SOFTWARE-TEST-ANDROID-RECHECK-001.md`：**TEST_PASS（限定范围）**。

- 本地 APK 与设备 `base.apk` SHA-256 均为 `71a041d3…`（逐字节一致）；
- `android_test install` 覆盖安装：`versionCode 10 / versionName 1.9`，`firstInstallTime` 保留、`lastUpdateTime` 更新；
- D-01/D-02/D-03/D-04a 真机回归通过；按钮 bounds 不与底部导航/彼此相交；首页文案与可达路径一致；
- 冷启动、页签、设定、网关对话框、上传状态/重试、守护启停无回归；无 FATAL/ANR；
- 结构化复核脚本 `verify_ui_checks.py` 13/13 PASS。

该独立证据只覆盖上述真机范围；真实 BLE/GATT/MQTT/`/upload_data`/401/回读等仍未执行。

## 5. 未产出：release 变体

`assembleRelease` 需要 `:app:lintVitalAnalyzeRelease`，而本地 Gradle 缓存缺少 lint 运行依赖（`com.android.tools.lint:lint*:31.3.2`、`intellij-core`、`uast`、`groovy`、`kotlin-reflect/stdlib`、`httpclient`、`commons-codec` 等）。非离线尝试一次：本机代理 `127.0.0.1:10808` 拒绝连接，无法下载。未修改构建配置、未安装系统包、未扩大网络授权。故本次交付 debug 测试候选；release/上架需先准备 lint 依赖与生产签名。

## 6. 安装/升级与边界

- 覆盖升级：`adb -s 12f046690506 install -r app-debug.apk`（同签名同版本，数据保留）；全新安装直接 `install`；签名不一致时只能卸载（丢数据）。
- 安装后核对 `versionCode/versionName` 与 `base.apk` SHA-256。
- 未使用 `android_test` 在本能力安装/操作手机；未访问服务端；无后台任务。
- 交付字节即独立复验安装字节；请勿 clean 重建后混用旧验收结论。

## 7. 证据文件

`build.log`、`unit_test.log`、`build_identity.txt`、`apk_badging.txt`、`apksigner_verify.txt`、`test_summary.txt`、本文件。
