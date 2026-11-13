# 产品文档独立校核报告（DOC-VERIFY-013）

- 负责角色：软件测试
- 能力：`software_tester.documentation_verification`（产品文档独立校核）
- 时点：2026-11-13（Asia/Shanghai；设备/工具时间戳为本地或 UTC，按原文记录）
- 被测文档：DOC-PKG-001 **rev 13**（`docs/README.md`、`docs/01`–`docs/06`、`dengbei_care/用户使用说明书.md`）
- 被测候选：
  - 传感器固件 FD-002 rev 8 发布包 `fw_release/fw_sensor_FD-002r8.zip`（FW-REL-001 rev 3）
  - 安卓 App 1.9（versionCode 10，AB-003，SHA-256 `80232c50…`）
  - 既有网关（本轮未修改、未烧录）与既有平台服务（只读核验结论）
- 仓库：`D:\mypro\beiwo2\AIP`，HEAD `a83861fcf4b360520c56223b09446ca9508547bd`；开工时工作树除本轮证据目录外干净
- 设备：Redmi 8A（`olivelite`），Android 10 / API 29，serial `12f046690506`
- **结论：`TEST_FAIL`（文档缺陷 2 项；已执行范围内未发现新的产品实测缺陷）**

> 本报告只记录本轮实际执行、静态核对与可复核原始证据。烧录、串口采集、调温/改光/功耗、跨链路与平台写入未执行，均逐项列明，不写成通过。人工已确认跳过的 `FW-LIGHT-DEF2-ENABLE` 仅按 NOT EXECUTED 披露，不作为返工依据，也不计入本轮缺陷。

---

## 0. 结论与判定依据

| 项 | 结果 |
|---|---|
| 文档身份、版本头、引用路径 | 通过（157 项内联路径引用 0 缺失；7 份文档 + 用户说明书均为 rev 13） |
| 固件包身份/成员/构建与下载预期 | 静态通过（独立复算包与成员摘要；未在本能力烧录） |
| UART1 接线、参数、行格式、节拍 | 静态通过（对照 `sensor_config.h`/`debug_trace.c/h`；未在本能力采集） |
| Android 包身份与设备在装字节 | 通过（设备 `base.apk` 与 AB-003 候选逐字节一致） |
| Android 关键界面与用户步骤 | 通过（首页/我的/账户/设定/网关对话框/传感器列表/报告页；扫描发现真实网关；报告再生成按钮恢复） |
| 平台配置静态一致性 | 通过（未连接、未写入） |
| 维护文档限制/证据索引 | 结构通过；**发现引用章节错误 D-01** |
| 源码差异声明 | **发现表述不准确 D-02** |
| 产品实测缺陷 | 本轮已执行范围内未发现 |
| **最终判定** | **`TEST_FAIL`**：D-01、D-02 为可操作的文档缺陷，需技术文档修正后重建版本再核验 |

失败原因：`MEASURED_DEFECT`（文档事实/引用错误），不是外部条件缺失。未执行项另列，不作为本次文档缺陷的依据。

---

## 1. 本轮必需读者任务与操作步骤

按当前文档任务与验收来源，列出并核对以下读者任务：

| # | 读者任务 | 文档位置 | 本轮处理 |
|---|---|---|---|
| T1 | 校验传感器发布包并识别版本 | docs/02 §2、§9 | 静态执行（独立复算 zip/成员摘要） |
| T2 | 按工程保存配置构建与下载，核对 `Verify OK` | docs/02 §4–§6 | 静态核对 + 复用 EV-040/EV-039 真实板证据；**本能力未烧录** |
| T3 | 连接 UART1（COM43/9600 8N1）并按五行/`S`/`G`/`P` 判读 | docs/03 §2–§8 | 静态核对源码与既有原始捕获；**本能力未采集串口** |
| T4 | 安装/覆盖升级/回退 Android 候选并核对包身份 | docs/04 §1–§2 | 真机核对在装字节与版本；**未重装/未回退**（设备已是同一候选） |
| T5 | 首次启动权限、扫描网关、读取终态、报告再生成 | docs/04 §3–§4、用户说明书 §2/§4/§9 | 真机走查关键界面与扫描/报告步骤（见 §4） |
| T6 | 配置平台地址/端口/证书/topic 并确认联调前置 | docs/05 §1–§5 | 静态核对 `CloudConfig.kt`、`network_security_config.xml`、`jd-ca.crt`、`MqtttService.kt`；**未连接/未写入** |
| T7 | 按维护文档排障并识别未执行项 | docs/06 §2–§5 | 静态核对限制编号、未执行清单与证据索引 |

用户明确当前不能调温、不能改变光强、不能测功耗；这些条件的测试未安排，也未以模拟替代。

---

## 2. 身份核对（原始结果见 `identity_checks.txt`）

| 对象 | 文档声明 | 本轮实测 | 判定 |
|---|---|---|---|
| 文档包 | DOC-PKG-001 rev 13（2026-11-13） | 7 份文档 + 用户说明书头部一致 | 通过 |
| 固件包 | 134,791 B；md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf` | 独立复算一致 | 通过 |
| 固件成员 | axf `b6a15fed…` 297,380 B；hex `4d728dec…` 75,275 B；PACKAGE `a58f67fc…` 3,202 B | 流式 md5 全部一致 | 通过 |
| 发布/实现记录 | FW-REL-001 rev 3；FWI-004 rev 9 | `artifacts/firmware_release.md`、`artifacts/firmware_implementation.md` 头部一致 | 通过 |
| APK | 10,370,964 B；SHA-256 `80232c50…`；versionCode 10 / 1.9（AB-003） | 主机 APK 摘要一致；设备 `dumpsys` 版本一致 | 通过 |
| 设备在装字节 | 与候选逐字节一致 | 设备 `base.apk` SHA-256 `80232c50…`、MD5 `a97ed9be…`、10,370,964 B | 通过 |
| 设备 | Redmi 8A / API 29 / `12f046690506` | `android_test devices` 一致 | 通过 |
| 平台 | `8.140.23.253`：MQTT 8883、HTTP 5000 | `CloudConfig.kt` 一致；本轮未连接 | 静态通过 |

---

## 3. 静态核对（逐文档）

### 3.1 docs/README.md
- 候选版本表、文档清单、环境前提、安全约束、待核验项 #1–#11 与当前候选/证据一致。
- #1 明确 `FW-LIGHT-DEF2-ENABLE` 为人工 `SKIP_CONFIRMED`/NOT EXECUTED，且说明“原检查者计入未满足/整体失败属范围错误，复核报告已纠正”；与 RECHECK-002 §2 原文一致。
- #11 产品整体验收状态为“AB-002 重跑仍 `TEST_FAIL`（外部条件）、AB-003 未重跑”，与 PRODUCT-ACCEPTANCE-002 / RECHECK-002 一致。
- 内联路径引用 25 项，0 缺失。

### 3.2 docs/01
- 需求逐项表与 `sensor_config.h`/`fw_core.c` 一致：`SENSOR_REPORT_RISE_X10=9`、`SENSOR_REPORT_HIGH_X10=350`、`LIGHT_DARK_THRESHOLD_CODE=0`。
- 边界语义（差值 9 不触发、35.0 不触发超温、首周期无前值）与实现一致。
- 实板观测引用 EV-036/038/039/040 均存在；EV-040 的 `k=0/3/6/9`、179.66/179.59 s、全 `V=0 D=0` 与 `evidence/test.md` 顶部一致。
- **D-02 见 §5**（源码差异表述）。
- 内联路径引用 35 项，0 缺失。

### 3.3 docs/02
- §2 包/成员摘要独立复算一致；§3 调试器序列号与 `usb_list_devices` 的 CW-DAPLink `87094109484987710672FF50` 一致；§4 构建 0E/1W、Code/RO/RW/ZI 与 EV-039/040 一致；§5 下载日志与 EV-040 一致；§6 五行/`S` 行与 EV-040 原始行一致。
- §8 升级/回退：无 OTA、重新烧录、同工程同算法；与发布记录一致。
- 本轮未执行烧录/回读校验（工具与外部动作不在当前能力），已在 §7 列明。

### 3.4 docs/03
- §2 接线 PA06/PA05/J3、9600 8N1 与 `sensor_config.h`、`debug_trace.h` 一致。
- §3–§7 五行与 `S`/`G`/`P` 字段格式与 `debug_trace.c` 的格式串一致；`G` 的 `s=1..5`、`raw` 12 位 hex、`P` 的冻结语义均一致。
- §8 编排顺序“先开串口再复位”与 EV-040/EV-034 记录一致；§10 限制（T=0、串口非数据接口、每次打印后关闭）与实现一致。
- 本轮未采集 COM43（`serial_capture` 不在当前能力工具集），复用 EV-040/EV-039 真实板原始证据；未把复用写成新执行。

### 3.5 docs/04
- §1 身份与 AB-003/设备一致；**D-02 见 §5**（源码差异表述）。
- §2 安装/覆盖升级/回退命令与 AB-003 §4 一致；本轮未重装/未回退，设备在装字节已核对。
- §3 权限策略与 `BlePermissionPolicy.kt` 一致；§3.5 扫描续跑、原因区分、关闭清除挂起动作与 `GatewayBleSettingsDialog.kt` 一致。
- §4.2 读取终态表与 `GatewayReadState.kt`/`MqtttService.kt` 一致；`GATEWAY_NO_DEVICE` 文案逐字一致。
- §4.4 报告再生成反馈与 `ReportActivity.kt`/`ReportRegenerateFeedback.kt` 一致。
- §5 上传分类/20 次/1800 s 与 `UploadResult.kt`/`CloudUploadRepository.kt` 一致。
- §6/§7/§8 报警、排障与限制与源码/证据一致。
- 内联路径引用（含 docs/05、06、用户说明书）合计 97 项，0 缺失。

### 3.6 docs/05
- 服务器地址/端口、3 个明文放行域、CA md5、topic/clientId/TLS 版本与源码一致。
- 未复述凭据；检索未发现固定后缀/口令/AccessKey 明文。
- 本轮未连接服务器、未调用写接口、未连数据库。

### 3.7 docs/06
- §3 限制编号 #1–#26 连续；#17/#18/#20 已修复保留编号；#26 为 AA-006 观察项。
- §4 未执行清单覆盖 API31+、9 台快照、多设备/MTU、DATA_OK、GATEWAY_NO_DATA、报告真机失败路径、有数据刷新、平台写入/回读/401/403、回退流程。
- §5 证据索引目录/文件均存在（ev039、ev040、ev038、fw_release_20261111/20261113、android_build_20261113、aa006_software_test_20261113、android_package_check_20261113、product_acceptance_20261112(_scope_recheck)、backend_exploration）。
- **D-01 见 §5**（#26 引用章节）。

### 3.8 随包用户说明书
- 版本行、APK 身份、历史 APK 大小/时间、权限说明、账户页、网关扫描续跑、`GATEWAY_NO_DEVICE` 文案、报告再生成反馈与源码/真机一致。
- 未发现凭据明文。

### 3.9 清单一致性
- `file_manifest.txt`：交付前检查 1812 条具体路径，0 通配、0 格式错、0 磁盘缺失；72 条存在但未跟踪（信息项，含本轮新增证据在 Runtime 提交前状态）；2594 条厂商跟踪文件未逐条登记（信息项，历史基线同类）。
- 本轮新增证据目录的清单条目已补充（见 §9）。

---

## 4. 真机执行记录（Redmi 8A / API 29）

工具：`android_test`（devices/launch/ui_dump/ui_click/ui_swipe/ui_press/ui_screenshot/logcat）+ 只读 ADB（`dumpsys package`、`pm path`、`sha256sum`、`logcat -d`）。未安装/卸载、未清除数据、未保存网关、未启动守护。

| # | 操作 | 时间（设备本地） | 实际观察 | 证据 |
|---|---|---|---|---|
| R1 | `android_test devices` | 15:52 | `12f046690506 device product:olivelite model:Redmi_8A` | 工具返回 |
| R2 | 读取在装包与摘要 | 15:52–15:53 | versionCode 10 / versionName 1.9；`base.apk` SHA-256 `80232c50…`、10,370,964 B | `device_package_dump.txt`、`device_apk_path.txt`、`device_apk_hash.txt` |
| R3 | 启动 App 并 dump 首页 | 15:53:35–15:53:46 | “爱守护”“启动”、空态“还没有添加设备”“去添加”、空态提示“点击下方「去添加」添加设备；也可在「我的」→「打开传感器列表」查看已保存 ID”、链路条“未配置网关地址（搜索附近网关）”、三页签 | `ui_00_home.xml` |
| R4 | 进入「我的」并 dump | 15:54:07–15:54:19 | “用户：未登录”“账户”“查看今日报告”“打开传感器列表”“请输入网关编码：”“保存”“请输入传感器编码和别称：”“注册传感器” | `ui_01_my_page.xml` |
| R5 | 上滑后 dump | 15:54:32–15:54:46 | 页面可滚动；“注册传感器”`[32,976][688,1072]` 与“网关与上传设置”`[32,1120][688,1216]` 不相交 | `ui_02_my_scrolled.xml` |
| R6 | 打开「网关与上传设置」并 dump | 15:55:13–15:55:25 | 标题、“未配置网关地址 / 平台身份（mac_addr）：”、“网关 BLE 地址”、搜索/扩大搜索/清除/保存、“云端上传状态：无（待发 0 条）”、“重试上传”、“关闭” | `ui_03_gateway_dialog.xml` |
| R7 | 点「搜索附近网关」并观察 | 15:55:57–15:56:10 | 扫描立即开始（权限已授予，无需二次点击）；界面“正在搜索…（最多 10 秒）”；发现真实候选 `xxx 84:C2:E4:03:02:02 RSSI=-50 FFE0`；logcat `scan stopped` 15:56:10.008 | `ui_04_after_scan.xml`、`logcat_session_full.txt` |
| R8 | 点「关闭」 | 15:56:41 | 对话框关闭，未保存网关（业务数据未改） | 工具返回 |
| R9 | 进入「查看今日报告」并 dump | 15:57:04–15:57:18 | `ReportActivity`：“早起报告”“尚未生成过报告”“重新生成”（enabled） | `ui_06_report_activity.xml` |
| R10 | 点「重新生成」并观察 | 15:57:36–15:57:58 | 点击后 9 s 内已恢复 enabled；仍“尚未生成过报告”；禁用/进度态过短，本次 dump 未捕获（源码 `generating` 守卫与 AA-006 O-AA006-3 记录该态 <0.35 s）；瞬时 Toast 未截到（见 O-3） | `ui_07_report_after_regen.xml`、`screen_07_report_after_regen.png` |
| R11 | 返回并打开「账户」 | 16:04:03–16:04:14 | “登录/注册”勾选、手机号、获取验证码、验证码、密码、确认密码、注册/登录/退出/忘记/修改密码/注销 | `ui_09_account_page.xml` |
| R12 | 打开「设定」并 dump | 16:04:51–16:05:03 | “选择设备”“全局默认值(所有设备)”、下降报警 2 / 0.1、阈值报警 35 / 29 / 75 | `ui_10_settings.xml` |
| R13 | 打开「打开传感器列表」并 dump | 16:07:33–16:07:45 | “已保存的 ID（0）”“点选可多选，删除选中可批量移除”“删除选中”（disabled） | `ui_11_sensor_list.xml` |
| R14 | 会话日志检查 | 全程 | 0 条 `FATAL EXCEPTION` / `ANR in com.jinyuni.dengbei_care` | `logcat_session_full.txt` |

说明：
- R7 的“自动开始扫描”在“权限已授予”状态下观察到点击即扫描；权限弹窗后的自动续跑由源码 `onPermissionResult` 与 AA-006 N1-D-01 真机证据共同支持（本轮未再次撤销权限，避免扰动设备状态）。
- R10 的“暂无可生成的数据（尚无设备读数）”文案由源码常量与 AA-006 N3-D-01 真机 Toast 证据支持；本轮复跑确认按钮恢复与报告未生成，瞬时 Toast 未截到。
- 全程未保存网关、未启动守护、未安装/卸载应用、未清除数据。

---

## 5. 发现（文档缺陷）

### D-01（中等，引用错误）：docs/06 §3 #26 的证据章节指向错误

- 位置：`docs/06_维护排障与已知限制.md` §3 限制表 #26。
- 原文：`evidence/aa006_software_test_20261113/ST-AA006.md` **§8**。
- 实际：四条 `O-AA006-1`～`O-AA006-4` 观察项位于 `ST-AA006.md` **§7 观察项**；§8 是“结论”，不含这四条观察的正文。
- 影响：读者按引用跳转会落到结论段，无法直接核对四条观察；属于可复核性缺陷。
- 建议修正：将 `§8` 改为 `§7`（或写“§7 观察项”）。修正后只需复核该行与受影响引用，不需重跑真机。

### D-02（中等，事实表述不准确）：源码差异声明与实际 `git diff` 不符

- 位置：
  - `docs/01_产品说明与版本变更.md` §5 交付物清单“安卓源码”行；
  - `docs/04_安卓App安装与使用.md` §1“源码”行。
- 原文：`git diff a12d12b..HEAD -- dengbei_care` **仅新增测试侧独立验证用例**。
- 实际（本轮独立执行，见 `source_diff_files.txt`、`source_diff_stat.txt`）：
  ```
  dengbei_care/app/src/test/java/.../GatewayReadStateItem002VerificationTest.kt
  dengbei_care/app/src/test/java/.../NotificationsPermissionWiringItem001VerificationTest.kt
  dengbei_care/app/src/test/java/.../ReportRegenerateFeedbackItem003VerificationTest.kt
  dengbei_care/用户使用说明书.md
  ```
  即除 3 个测试侧独立验证用例外，还包含随包用户说明书 rev 13 的更新（10 行变更）。产品源码（`app/src/main`）确无差异，但“仅新增测试侧独立验证用例”作为整条命令的结论不成立。
- 影响：读者按该命令复核时会看到额外文件，误以为文档漏报改动；影响源码身份核对的可信度。
- 建议修正：改为“新增 3 个测试侧独立验证用例并更新随包用户说明书；`dengbei_care/app/src/main` 无产品源码差异”，或把命令限定到产品源码路径并给出实际输出。
- 关联：`artifacts/android_build.md` 头部有同义表述（Android 构建产物，非本文档角色）；建议 Android 构建方同步核对措辞。

---

## 6. 观察项（不构成本轮失败依据）

- **O-1**：`docs/06` §5 将 EV-039 写作“宿主 222/609”，而 `evidence/ev039/EV-039_section.md` 的原始表述为“宿主 222/0 + 测试侧 609/0 + 静态 21/21”。数字本身来自原文，但简写易被误读为单一 222/609 结果；建议改为“宿主 222/0、测试侧 609/0”。不构成本轮失败。
- **O-2**：`docs/04` §3.5 第 5 条“关闭对话框会清除挂起的待扫描动作”已由源码 `stopScan()`/`onDismiss` 静态确认；本轮未重新构造“权限结果迟到”场景，AA-006 覆盖的是“扫描中关闭对话框”。属未重跑路径，不是缺陷。
- **O-3**：本轮 R10 未截到“暂无可生成的数据（尚无设备读数）”的瞬时 Toast；源码常量 `ReportRegenerateFeedback.TEXT_NO_DATA` 与 AA-006 N3-D-01 真机证据一致。记录为“本轮未捕获瞬时提示”，不写成未通过。
- **O-4**：`docs/06` §5 的 EV-040 目录说明为“两次上电五行、`k=0/3/6/9` 与节拍分析”，与 `evidence/ev040/` 实际文件一致；无需修改。

---

## 7. 未执行 / 未验证（如实披露）

| 项 | 状态 | 原因 |
|---|---|---|
| 传感器烧录、`Verify OK` 回读 | 未执行（本能力） | 当前能力工具集无 `mdk_flash`；`flash_device` 外部动作不在授权内；复用 EV-040/EV-039 真实板记录，不写成新执行 |
| COM43 串口采集（上电五行/`S` 行） | 未执行（本能力） | `serial_capture` 不在当前能力工具集；复用 EV-040/EV-039/EV-038 原始证据 |
| 真实升温 >0.9 ℃ / >35.0 ℃ 触发与 433 实发 | 未执行 | 用户明确当前不能调温 |
| 改变光强/遮光双态（`FW-LIGHT-DEF2-ENABLE`） | NOT EXECUTED（人工 `APPROVED_DEFERRED`） | 人工已确认跳过；不重测、不重复询问、不作为返工依据 |
| 功耗测量（`E2E-04`） | 未执行 | 无仪器，已批准延期 |
| 网关→手机跨链路（433+BLE） | 未执行 | 后续联调阶段 |
| 平台 `/upload_data` 真实写入/幂等/回读、401/403 | 未执行 | 无写授权/数据库授权 |
| Android 重装/覆盖升级/回退 | 未重跑 | 设备已安装同一候选字节；复用 AB-003/AA-006 安装与覆盖升级证据 |
| 真实 BLE 有效批次 `DATA_OK`、全占位 `GATEWAY_NO_DATA`、多设备/MTU、9 台快照 | 未执行 | 当前网关 `devCount=0`，无真实数据源 |
| 报告有数据成功刷新、真机失败路径 | 未执行 | 0 设备、无当日样本；无非破坏性失败诱导 |
| API 31+ 权限分支 | 未执行 | 无 API 31+ 设备 |
| 产品整体验收 | 未在本 APK 重跑 | AB-002 重跑仍 `TEST_FAIL`（外部条件）；AB-003 仅 AA-006 限定验证 |
| iOS | 未修改、未验证 | 用户需求本轮不改 iOS |

---

## 8. 人工范围决定复核（操作员恢复指令）

- `FW-LIGHT-DEF2-ENABLE` 的人工结论为 `APPROVED_DEFERRED` / NOT EXECUTED（2026-11-11T03:50:42Z，「无法准备，先跳过」）。本轮**未重测、未重复询问、未作为返工依据**。
- 原检查者 `PRODUCT-ACCEPTANCE-002` §5 R4 将“上传规则”标为“未满足/未执行”，并把“无光分支仍禁用”计入整体 `TEST_FAIL` 依据；`PRODUCT-ACCEPTANCE-RECHECK-002` §2 已逐条纠正（E-1～E-4）。本轮核对原文，确认：
  - R4 的“判定公式实现”与需求 4 一致，属已实现项；
  - “无光判据启用”单列 NOT EXECUTED（人工延期）；
  - “真实升温/超温触发”单列 NOT EXECUTED（外部条件）；
  - 整体验收仍不通过，但依据只保留其余外部条件/后续阶段项。
- rev 13 文档在 README §5 #1、docs/01 §3/§6、docs/06 §4 均已按上述口径披露，未把人工跳过项写成缺陷或返工依据。该范围纠正在文档侧是正确的。

---

## 9. 证据索引

| 文件 | 内容 |
|---|---|
| `identity_checks.txt` | HEAD、文档/固件/APK/设备/平台身份与摘要原始记录 |
| `static_checks.txt` | 逐文档静态核对、源码/格式/常量/路径/清单检查与 D-01/D-02 |
| `check_doc_refs_output.txt` | 路径引用检查结果（157 checked / 0 missing） |
| `check_manifest_output.txt` | 清单—工作区—Git 一致性检查输出（交付前 1812 条、0 缺失/0 通配/0 格式错） |
| `source_diff_files.txt` / `source_diff_stat.txt` | D-02 的 `git diff a12d12b..HEAD -- dengbei_care` 原始输出 |
| `device_package_dump.txt` / `device_apk_path.txt` / `device_apk_hash.txt` | 设备版本、包路径、在装 `base.apk` 摘要（副本核对后删除） |
| `ui_00_home.xml` … `ui_11_sensor_list.xml` | 真机 UI 原始树（首页/我的/滚动/网关对话框/扫描后/报告/账户/设定/传感器列表） |
| `screen_07_report_after_regen.png` | 报告再生成后界面截图 |
| `logcat_session_full.txt` / `logcat_tail.txt` | 本会话日志（0 FATAL/ANR） |
| `head.txt` / `git_status.txt` | 仓库身份与工作树状态 |

> 本报告为独立文档校核结论，不替代产品总功能验收；旧版 DOC-VERIFY 结论不适用于 rev 13。
