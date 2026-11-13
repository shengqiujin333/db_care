# 产品文档独立校核报告（DOC-VERIFY-014）

- 负责角色：软件测试
- 能力：`software_tester.documentation_verification`（产品文档独立校核）
- 时点：2026-11-13（Asia/Shanghai；设备/工具时间戳按本地时间记录）
- 被测文档：DOC-PKG-001 **rev 14**（`docs/README.md`、`docs/01`–`docs/06`、`dengbei_care/用户使用说明书.md`）
- 被测候选：
  - 传感器固件 FD-002 rev 8 发布包 `fw_release/fw_sensor_FD-002r8.zip`（FW-REL-001 rev 3）
  - 安卓 App 1.9（versionCode 10，AB-003，SHA-256 `80232c50…`）
  - 既有网关（本轮未修改、未烧录）与既有平台服务（沿用 2026-11-10 只读核验结论）
- 仓库：`D:\mypro\beiwo2\AIP`，HEAD `d3408014bc6d5d7841f4203756f61067c44167c0`
- 设备：Redmi 8A（`olivelite`），Android 10 / API 29，serial `12f046690506`
- **结论：`TEST_PASS`（限 DOC-PKG-001 rev 14 与本轮已执行范围；不替代产品总功能验收）**

> 本报告只记录本轮实际执行、静态核对与可复核原始证据。烧录、串口采集、调温/改光/功耗、跨链路与平台写入未执行，逐项列明，不写成通过。人工已确认跳过的 `FW-LIGHT-DEF2-ENABLE` 仅按 NOT EXECUTED 披露，不作为返工依据，也不计入本轮缺陷。

---

## 0. 结论与判定依据

| 项 | 结果 |
|---|---|
| DOC-VERIFY-013 的 D-01（06 #26 引用章节） | **已修复并独立复核**：现为 `ST-AA006.md` §7 观察项，原文 §7 含 O-AA006-1..4 |
| DOC-VERIFY-013 的 D-02（源码差异声明） | **已修复并独立复核**：`git diff a12d12b..HEAD -- dengbei_care` 恰为 3 个测试用例 + 用户说明书；`app/src/main` 无差异 |
| DOC-VERIFY-013 的 O-1（EV-039 数字简写） | 已修正为“宿主 222/0、测试侧 609/0、静态 21/21”，与 EV-039 原文一致 |
| 文档身份/版本头/路径引用 | 通过（7 份文档 + 用户说明书均为 rev 14；128 个可解析仓库路径引用、0 缺失） |
| 固件包身份/成员/构建与串口字段 | 静态通过（独立复算包与成员摘要；对照源码字段与 EV-040 原始证据；本能力未烧录/未采集） |
| Android 包身份、安装与设备在装字节 | 通过（候选摘要一致；覆盖安装实测成功；设备 `base.apk` 逐字节一致；数据保留） |
| Android 关键界面与用户步骤 | 通过（首页/我的/滚动/网关对话框/报告页/传感器列表；文字、bounds 与文档一致） |
| 平台配置静态一致性 | 通过（未连接、未写入；沿用 BE-EXP-004 只读结论并保留其时点） |
| 维护文档限制/证据索引 | 结构通过（#1–#26 连续；引用目录/文件存在；未执行清单完整） |
| **本轮新增文档缺陷** | **0 项阻断**；4 项非阻断观察（OBS-1..4，见 §6） |
| **最终判定** | **`TEST_PASS`**（rev 14 文档与当前候选/实现/环境一致，必需读者步骤在可执行范围内复现通过） |

`failure_cause`：不适用（未发现文档或产品实测缺陷）。

---

## 1. 本轮必需读者任务与操作步骤

| # | 读者任务 | 文档位置 | 本轮处理 |
|---|---|---|---|
| T1 | 校验传感器发布包并识别版本 | docs/02 §2、§9 | 静态执行（独立复算 zip/成员摘要） |
| T2 | 按工程保存配置构建与下载，核对 `Verify OK` | docs/02 §4–§6 | 静态核对工程/构建/下载声明 + 复用 EV-040/EV-039 真实板证据；**本能力未烧录** |
| T3 | 连接 UART1（COM43/9600 8N1）并按五行/`S`/`G`/`P` 判读 | docs/03 §2–§8 | 静态核对源码/格式/常量 + `usb_list_devices` 核对 COM43/COM26；**本能力未采集串口** |
| T4 | 安装/覆盖升级 Android 候选并核对包身份 | docs/04 §1–§2 | **真机覆盖安装实测**；设备在装字节与候选逐字节一致 |
| T5 | 首次启动权限、扫描网关、读取终态、报告再生成 | docs/04 §3–§4、用户说明书 §2/§4/§9 | 真机走查“我的”页、网关对话框、报告页与重新生成；扫描/保存未执行（避免改配置） |
| T6 | 配置平台地址/端口/证书/topic 并确认联调前置 | docs/05 §1–§5 | 静态核对 `CloudConfig.kt`、`network_security_config.xml`、`jd-ca.crt`、`MqtttService.kt`；**未连接/未写入** |
| T7 | 按维护文档排障并识别未执行项 | docs/06 §2–§5 | 静态核对限制编号、证据索引与未执行清单 |

用户明确当前不能调温、不能改变光强、不能测功耗；这些条件的测试未安排，也未以模拟替代。

---

## 2. 身份核对（原始结果见 `identity_checks.txt`）

| 对象 | 文档声明 | 本轮实测 | 判定 |
|---|---|---|---|
| 文档包 | DOC-PKG-001 rev 14（2026-11-13） | README 标题 rev 14；01–06 头部一致；用户说明书 1.9/AB-003 | 通过 |
| 固件包 | 134,791 B；md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf` | 独立复算一致；zip CRC OK | 通过 |
| 固件成员 | axf `b6a15fed…` 297,380 B；hex `4d728dec…` 75,275 B；PACKAGE `a58f67fc…` 3,202 B | 逐成员复算一致 | 通过 |
| 发布/实现记录 | FW-REL-001 rev 3；FWI-004 rev 9 | `artifacts/firmware_release.md`、`firmware_implementation.md` 头部一致 | 通过 |
| 固件源码 | 产品源提交 `82a7e74` | `git diff 82a7e74..HEAD`（USER/MDK/EWARM/UM2005C/COMMON）为空 | 通过 |
| APK | 10,370,964 B；SHA-256 `80232c50…`；versionCode 10 / 1.9（AB-003） | 主机复算一致；badging/签名一致 | 通过 |
| 设备在装字节 | 与候选逐字节一致 | 重装后 `base.apk` SHA-256 `80232c50…`、MD5 `a97ed9be…`、10,370,964 B | 通过 |
| 设备 | Redmi 8A / API 29 / `12f046690506` | `android_test devices` 一致 | 通过 |
| 串口/调试器 | COM43=Prolific 067B:2303；COM26=C251:F001 CDC；CW-DAPLink SN `87094109484987710672FF50` | `usb_list_devices` 一致 | 通过（仅枚举身份） |
| 平台 | `8.140.23.253`：MQTT 8883、HTTP 5000 | `CloudConfig.kt` 一致；本轮未连接 | 静态通过 |

---

## 3. 静态核对（逐文档）

### 3.1 docs/README.md
- 候选版本、文档清单、环境前提与当前候选/证据一致；rev 14 变更记录与 DOC-VERIFY-013 对应。
- §5 #1 明确 `FW-LIGHT-DEF2-ENABLE` 为人工 `SKIP_CONFIRMED`/NOT EXECUTED，并指出原检查者计入整体失败属范围错误；与 `PRODUCT-ACCEPTANCE-RECHECK-002` 一致。
- §5 #7/#11 的 AA-006 限定验证与 AB-002 整体验收状态与 ST-AA006、PRODUCT-ACCEPTANCE-002/RECHECK-002 原文一致。

### 3.2 docs/01
- 8 项需求逐项表与 `sensor_config.h`/`fw_core.c`/`light.c` 一致：`SENSOR_SAMPLE_TICKS=3`、`SENSOR_REPORT_RISE_X10=9`、`SENSOR_REPORT_HIGH_X10=350`、`LIGHT_DARK_THRESHOLD_CODE=0`。
- 边界语义（差值 9 不触发、35.0 不触发超温、首周期无前值）与实现一致。
- §5 安卓源码差异声明已修正，与 `git diff` 输出一致（D-02 关闭）。
- 实板观测引用 EV-040（`k=0/3/6/9`、179.66/179.59 s、全 `V=0 D=0`）与 `evidence/test.md` 顶部一致；EV-036 `a=2860..2886` 与 `evidence/ev036/capture_analysis.txt` 一致。
- 硬件拓扑（PB05→R3 5M→PB04/AIN11→光敏→GND、J4）与 `sensor_hardware/pstxnet.dat`、BOM 一致。

### 3.3 docs/02
- §2 包/成员摘要独立复算一致；§3 调试器序列号与 USB 枚举一致；§4 构建 0E/1W、Code/RO/RW/ZI、gcc FLASH/RAM 与 EV-039/040、`main.c:254`、`gcc_build_release.log` 一致；§5 下载日志与 EV-040 一致；§6 五行/`S` 行格式与 `debug_trace.c` 一致。
- §8 升级/回退：无 OTA、重新烧录、同工程同算法；与发布记录一致。
- 本轮未执行烧录/回读校验（工具与外部动作不在当前能力），已在 §7 列明。

### 3.4 docs/03
- §2 接线 PA06/PA05/J3、9600 8N1、PCLK 8 MHz 与 `sensor_config.h`、`debug_trace.h` 一致；COM43/COM26 身份经 USB 枚举复核。
- §3–§7 五行与 `S`/`G`/`P` 字段格式、行长预算与 `debug_trace.c` 格式串一致。
- §5.2/§10 交付默认 `T=0 ⇒ V=0 D=0`、升温+无光分支不触发，与 `light.c`/`fw_core.c` 一致。
- 本轮未采集 COM43；复用 EV-040/EV-039 真实板原始证据，未写成新执行。

### 3.5 docs/04
- §1 身份与 AB-003/设备一致；源码差异声明已修正（D-02 关闭）。
- **§2.2 覆盖升级已实测**：同一候选安装成功，`firstInstallTime` 保持、`lastUpdateTime` 更新、`MyPrefs.xml` 保留。
- §3 权限策略与 `BlePermissionPolicy.kt` 一致；§3.5 扫描续跑、原因区分、关闭清除挂起动作与 `GatewayBleSettingsDialog.kt` 一致。
- §4.1/§4.2 的真机文字、bounds、按钮集与本次 UI dump 一致；读取终态表与 `GatewayReadState.kt` 一致。
- §4.4 报告再生成反馈与 `ReportActivity.kt`/`ReportRegenerateFeedback.kt` 一致。
- §5 上传分类/20 次/1800 s 与 `UploadResult.kt`/`CloudUploadRepository.kt` 一致。
- §6/§7/§8 报警、排障与限制与源码/证据一致。

### 3.6 docs/05
- 服务器地址/端口、3 个明文放行域、CA md5、topic/clientId/TLS 版本与源码一致；Kotlin 源码无旧主机 `117.72.84.210`。
- 未复述凭据；检索未发现固定后缀/口令/AccessKey 明文。
- 本轮未连接服务器、未调用写接口、未连数据库；文档保留 2026-11-10 只读核验时点。

### 3.7 docs/06
- §3 限制编号 #1–#26 连续；#17/#18/#20 已修复保留编号；#26 引用已改为 `ST-AA006.md` §7 观察项（D-01 关闭）。
- §4 未执行清单覆盖 API31+、9 台快照、多设备/MTU、`DATA_OK`、`GATEWAY_NO_DATA`、报告真机失败路径、有数据刷新、平台写入/回读/401/403、回退流程。
- §5 证据索引目录/文件均存在；EV-039 数字表述已修正（O-1 关闭）。

### 3.8 随包用户说明书
- 版本行、APK 身份、历史 APK 大小/时间、权限说明、账户页、网关扫描续跑、`GATEWAY_NO_DEVICE` 文案、报告再生成反馈与源码/真机一致。
- 历史 APK `dengbei_care/爱守护.apk` 实测 8,144,628 B、时间 2024-11-06，与文档一致。
- 未发现凭据明文。

### 3.9 清单一致性
- `file_manifest.txt`：1842 条具体路径（含本轮新增 30 条），0 缺失、0 格式错、0 通配（`check_manifest_output.txt`）。
- 本轮新增证据目录已在交付前补入清单（见 §9）。

---

## 4. 真机执行记录（Redmi 8A / API 29）

工具：`android_test`（devices/install/launch/ui_dump/ui_click/ui_swipe/ui_press/ui_screenshot/logcat）+ 只读 ADB（`dumpsys package`、`pm path`、`sha256sum`、`run-as ls`、`logcat -b crash`）。未卸载、未清数据、未保存网关、未启动守护。

| # | 操作 | 时间（设备本地） | 实际观察 | 证据 |
|---|---|---|---|---|
| R1 | `android_test devices` | 16:32:59 | `12f046690506 device product:olivelite model:Redmi_8A` | 工具返回 |
| R2 | 读取在装包 | 16:36 | versionCode 10 / versionName 1.9；`base.apk` 与候选一致 | `device_package_dump.txt`、`device_apk_hash.txt` |
| R3 | 启动 App（升级前） | 16:36:52 | 打开上次页面（传感器列表空态）；无崩溃 | `ui_06_sensor_list_empty.xml`、`logcat_tail.txt` |
| R4 | 返回「我的」并 dump | 16:37 | `用户：未登录`、`账户`、`查看今日报告`、`打开传感器列表`、网关编码/注册传感器区 | `ui_00_my_page.xml` |
| R5 | 上滑后 dump | 16:37 | `网关与上传设置` `[32,1120][688,1216]`；与 `注册传感器` `[32,976][688,1072]` 不相交 | `ui_01_my_scrolled.xml` |
| R6 | 打开「网关与上传设置」 | 16:38 | 标题/状态/输入框/搜索/扩大搜索/清除/保存/上传状态/重试/关闭 与文档一致 | `ui_02_gateway_dialog.xml` |
| R7 | 关闭对话框（未保存） | 16:38:36 | 对话框关闭，配置未改 | 工具返回 |
| R8 | 打开「查看今日报告」 | 16:38:44 | `ReportActivity`：`早起报告`、`尚未生成过报告`、`重新生成`（enabled） | `ui_03_report.xml` |
| R9 | 点「重新生成」并复查 | 16:39:08–16:39:26 | 按钮恢复 enabled；仍“尚未生成过报告”；瞬时 Toast 未捕获（源码常量/AA-006 证据支持） | `ui_04_report_after_regen.xml`、`screen_report_after_regen.png` |
| R10 | 覆盖安装同一候选 | 16:41:37–16:41:44 | `installed_version_verified`；包名/version/SHA-256 一致；`firstInstallTime` 保持、`lastUpdateTime` 更新、`MyPrefs.xml` 保留 | `install_result.json`、`device_state.txt` |
| R11 | 安装后启动 | 16:42 | 首页空态文案、`启动`、三页签；pid 4495；crash buffer 0 行 | `ui_05_home_after_reinstall.xml`、`logcat_crash.txt` |

说明：
- 未执行真实网关扫描/保存、守护启动与真实批次读取，避免改变设备业务配置。
- 未执行全新安装、卸载/回退、权限首次弹窗或拒绝路径；这些在 docs/04 §8、docs/06 §4 已披露为未验证。

---

## 5. 发现（本轮）

**本轮未发现需要文档返工的缺陷。** DOC-VERIFY-013 的 D-01、D-02 均已按要求修复；O-1 亦已修正。rev 14 的版本头、引用、源码差异表述和 EV-039 数字均与当前候选/证据一致。

---

## 6. 观察项（不构成本轮失败依据）

- **OBS-1**：用户说明书 §3.1 写“输入手机号(11 位,1 开头)”，实现校验为 `^1[3-9]\d{9}$`（`ZhuCeFragment.kt:420`）。对真实手机号无影响，但“1 开头”未覆盖实现规则；建议后续写为“11 位、1[3-9] 开头”。
- **OBS-2**：docs/04 §4.1 列出的失败提示未包含两条边缘提示（`请登录...`、`mac 设置失败，请重试...`）；主流程提示与源码一致，不影响文档步骤。
- **OBS-3**：docs/04 §4.2 写“候选优先过滤 FFE0”，实现默认使用 FFE0 `ScanFilter`（仅 FFE0），`扩大搜索`才不过滤；同表已表达差异，建议后续改为“默认仅 FFE0”。
- **OBS-4**：本轮点击“重新生成”未截到瞬时 Toast；源码常量 `TEXT_NO_DATA` 与 AA-006 N3-D-01 真机证据一致，按“未捕获瞬时提示”记录，不判失败。

---

## 7. 未执行 / 未验证（如实披露）

| 项 | 状态 | 原因 |
|---|---|---|
| 传感器烧录、`Verify OK` 回读 | 未执行（本能力） | 当前能力工具集无 `mdk_flash`；`flash_device` 不在授权内；复用 EV-040/EV-039 真实板记录 |
| COM43 串口采集（上电五行/`S` 行） | 未执行（本能力） | `serial_capture` 不在当前能力工具集；复用 EV-040/EV-039/EV-038 原始证据 |
| 真实升温 >0.9 ℃ / >35.0 ℃ 触发与 433 实发 | 未执行 | 用户明确当前不能调温 |
| 改变光强/遮光双态（`FW-LIGHT-DEF2-ENABLE`） | NOT EXECUTED（人工 `APPROVED_DEFERRED`） | 人工已确认跳过；不重测、不重复询问、不作为返工依据 |
| 功耗测量（`E2E-04`） | 未执行 | 无仪器，已批准延期 |
| 网关→手机跨链路（433+BLE） | 未执行 | 后续联调阶段 |
| 平台 `/upload_data` 真实写入/幂等/回读、401/403 | 未执行 | 无写授权/数据库授权；本轮无 SSH 入口授权 |
| Android 全新安装、卸载、签名不一致路径 | 未执行 | 会清除本机数据；文档已披露 |
| Android 回退流程（旧包 `-r` 覆盖回退） | 未执行 | 无上一构建包；文档已披露 |
| 真实 BLE 有效批次 `DATA_OK`、全占位 `GATEWAY_NO_DATA`、多设备/MTU、9 台快照 | 未执行 | 当前网关 `devCount=0`，无真实数据源 |
| 报告有数据成功刷新、真机失败路径 | 未执行 | 0 设备、无当日样本；无非破坏性失败诱导 |
| API 31+ 权限分支 | 未执行 | 无 API 31+ 设备 |
| 产品整体验收 | 未在本 APK 重跑 | AB-002 重跑仍 `TEST_FAIL`（外部条件）；AB-003 仅 AA-006 限定验证 |
| iOS | 未修改、未验证 | 用户需求本轮不改 iOS |

---

## 8. 人工范围决定复核（操作员恢复指令）

- `FW-LIGHT-DEF2-ENABLE` 的人工结论为 `APPROVED_DEFERRED` / NOT EXECUTED（2026-11-11T03:50:42Z，「无法准备，先跳过」）。本轮**未重测、未重复询问、未作为返工依据**。
- 原检查者 `PRODUCT-ACCEPTANCE-002` §5 R4 将该人工延期项与“需求未满足”混同，并计入整体 `TEST_FAIL` 依据；`PRODUCT-ACCEPTANCE-RECHECK-002` §2 已逐条纠正（E-1～E-4）。本轮核对原文，确认 rev 14 文档（README §5 #1、docs/01 §3/§6、docs/06 §4）按 NOT EXECUTED 披露，未把人工跳过项写成缺陷或返工依据。
- 本轮不因该人工跳过项判定文档失败；其余有效目标均已独立核对并给出实际结果。

---

## 9. 证据索引

| 文件 | 内容 |
|---|---|
| `identity_checks.txt` | HEAD、工作树、文档版本头、固件/APK/CA 摘要、构建证据、源码差异 |
| `static_checks.txt` | 逐项静态/实机核对、D-01/D-02/O-1 关闭证据、源码行号、观察项与未执行 |
| `check_doc_refs.py` / `check_doc_refs_output.txt` | 文档内联路径引用检查（128 checked / 0 missing） |
| `check_manifest.py` / `check_manifest_output.txt` | 清单—工作区一致性检查（1842 条、0 缺失/0 格式错/0 通配） |
| `source_diff_files.txt` / `source_diff_stat.txt` / `product_main_diff_stat.txt` | D-02 复算：`git diff a12d12b..HEAD -- dengbei_care` 及 `app/src/main` 空差异 |
| `device_package_dump.txt` / `device_apk_path.txt` / `device_apk_hash.txt` / `device_state.txt` | 设备版本、包路径、在装 `base.apk` 摘要、安装前后状态 |
| `install_result.json` | 覆盖安装结果（包名/版本/SHA-256 已核验） |
| `ui_00_my_page.xml` … `ui_06_sensor_list_empty.xml` | 真机 UI 原始树（我的/滚动/网关对话框/报告/重生成后/安装后首页/传感器列表） |
| `screen_report_after_regen.png` | 报告再生成后截图 |
| `logcat_crash.txt` / `logcat_tail.txt` | crash buffer 0 行；会话尾部无 FATAL/ANR |
| `usb_devices.txt` | COM43/COM26/CW-DAPLink 枚举身份 |
| `head.txt` / `git_status.txt` | 仓库身份与工作树状态 |
| `collect_identity.sh` | 身份采集脚本（只读） |

> 本报告为独立文档校核结论，不替代产品总功能验收；旧版 DOC-VERIFY 结论不适用于 rev 14。
