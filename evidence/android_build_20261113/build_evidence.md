# Android 构建证据（BUILD-007）

状态：构建证据（`android_engineer.android_release`）。仅证明本机构建与包身份；不证明真实链路或产品总验收。
交付对象：`artifacts/android_build.md`（AB-003）
源码：Git `548cc3301bdb87e00084136a0cff5fe8c63005e1`；Android 产品源码基线 `a12d12b570d541a9d348a1b6d6846275d871c7d4`（`git diff a12d12b..HEAD -- dengbei_care` 仅测试侧新增独立验证用例）
环境：Windows 11 amd64 / Gradle 8.4（wrapper）/ AGP 8.3.2 / Kotlin 1.9.0 / JDK 21.0.2 / `--offline`

---

## 1. 构建

```text
$ cd D:\mypro\beiwo2\AIP\dengbei_care
$ ./gradlew :app:assembleDebug --offline
BUILD SUCCESSFUL in 796ms
35 actionable tasks: 35 up-to-date
```

说明：产物未变化，保持为独立验证实际安装的同一字节。
原始输出：`build.log`。

| 项 | 值 |
|---|---|
| 路径 | `dengbei_care/app/build/outputs/apk/debug/app-debug.apk` |
| 大小 | 10,370,964 B |
| SHA-256 | `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b` |
| MD5 | `a97ed9bef658ee89cdf6e2b49447ce69` |
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

原始：`apksigner_verify.txt`。项目既有 debug keystore；无生产签名，不上架。

## 3. 宿主机回归

```text
$ ./gradlew :app:testDebugUnitTest --offline
BUILD SUCCESSFUL in 865ms
25 actionable tasks: 25 up-to-date
```

结果集：**39 套件 / 300 项 / 0 失败 / 0 错误 / 0 跳过**（含测试侧 AA-006 独立验证 21 项）。
原始：`unit_test.log`、`test_summary.txt`。

## 4. 独立验证（复用有效证据）

`evidence/aa006_software_test_20261113/ST-AA006.md`：**TEST_PASS（限定 AA-006 Android 增量与已执行范围）**。

- 本地 APK 与设备 `base.apk` SHA-256 均为 `80232c50…`（逐字节一致）；`android_test install` 校验 versionCode 10 / versionName 1.9；
- N1 真机：授权后 63 ms 自动开始扫描、10.015 s 停止、候选 `xxx 84:C2:E4:03:02:02 RSSI=-50 FFE0`；蓝牙/定位/权限拒绝分别正确提示；连续点击串行；启动/保存/清除不请求权限；
- N2 真机：真实网关 `devCount=0` → `GATEWAY_NO_DEVICE`，首页“网关已连接，暂无设备（N=0）”；两周期 182.9 s，无残留 CONNECTING；不产生读数/入库/入队/告警；
- N3 真机：零设备点击有“暂无可生成的数据（尚无设备读数）”Toast，按钮恢复；连续点击串行；退出无崩溃；
- 既有静态 54/54、12/12 门禁、15/15 布局核查通过；会话无 FATAL/ANR。

未执行项（真实有效批次/占位/9 台/API31+/平台写入回读/真实触发/功耗等）见 ST-AA006 §6，保持 NOT EXECUTED。

## 5. 未产出：release 变体

`assembleRelease` 依赖 `:app:lintVitalAnalyzeRelease`，本地 Gradle 缓存缺少 lint 运行依赖；非离线网络因本机代理 `127.0.0.1:10808` 拒绝连接。未修改构建配置、未安装系统包、未扩大网络授权。本次交付 debug 测试候选；release/上架需 lint 依赖与生产签名。

## 6. 范围核对与人工决定

- 人工 `FW-LIGHT-DEF2-ENABLE = APPROVED_DEFERRED`（NOT EXECUTED），不重测、不作返工依据；`BOT-01 = REASSESS` 已执行 PASS；两者均不属本候选。
- 原 `PRODUCT-ACCEPTANCE-002` R4 的范围错误已由 `PRODUCT-ACCEPTANCE-RECHECK-002` 纠正；本构建不将其计入 Android 缺陷。
- 产品级 `TEST_FAIL=EXTERNAL_CONDITION_UNEXECUTED` 仍有效；本候选限定 TEST_PASS 不等于产品通过。

## 7. 安装/升级与边界

- 覆盖升级：`adb -s 12f046690506 install -r app-debug.apk`（同签名同版本，数据保留）；全新安装直接 `install`；签名不一致只能卸载（丢数据）。
- 安装后核对版本与 `base.apk` SHA-256 `80232c50…`。
- 本能力未使用 `android_test` 安装/操作手机；未访问服务端；无后台任务。
- 交付字节即独立验证字节；请勿 clean 重建后混用旧验收结论。

## 8. 证据文件

`build.log`、`unit_test.log`、`build_identity.txt`、`test_summary.txt`、`apk_badging.txt`、`apksigner_verify.txt`、本文件。
