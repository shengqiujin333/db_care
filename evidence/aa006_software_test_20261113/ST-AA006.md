# 软件独立验证报告（ST-AA006）

- 能力：`software_tester.software_verification`（软件验证）
- 被测对象：Android App `com.jinyuni.dengbei_care` 当前候选（AA-006 N-1/N-2/N-3 三项增量）
- 仓库/分支/提交：`D:\mypro\beiwo2\AIP`，`main`，`e11b72ab456b68bc7c5827bf882b91b44890e55b`
- Android 产品源码基线（实现提交）：`a12d12b570d541a9d348a1b6d6846275d871c7d4`；本轮仅新增测试侧独立验证用例，未修改产品源码
- 执行时间：2026-11-13（Asia/Shanghai）
- 依据：`artifacts/software_test_design.md`（TD-SW-005 v5.0）、`artifacts/android_architecture.md`（AA-006 rev 5.0）、`artifacts/android_tasks.yaml`
- 结论：**TEST_PASS（限定本轮 AA-006 Android 增量与已执行范围）**；阶段通过 ≠ 完整产品通过，未执行项见 §6。

---

## 1. 被测版本与安装身份

| 项 | 值 | 证据 |
|---|---|---|
| 构建命令 | `./gradlew --offline clean :app:assembleDebug --console=plain` | `build.log`（BUILD SUCCESSFUL） |
| APK | `dengbei_care/app/build/outputs/apk/debug/app-debug.apk` | — |
| APK 大小 | 10,370,964 B | `identity.txt` |
| APK SHA-256 | `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b` | `identity.txt` |
| APK MD5 | `a97ed9bef658ee89cdf6e2b49447ce69` | `identity.txt` |
| 版本 | versionCode 10 / versionName 1.9；minSdk 26 / targetSdk 34 | `apk_badging.txt` |
| 签名 | APK Signature v2；Android Debug 证书（SHA-256 `773e4ef6…`） | `apksigner_verify.txt` |
| 设备 | Redmi 8A / Android 10 / API 29 / serial `12f046690506` | `device_state_pre.txt` |
| 安装 | `android_test install` 校验成功，versionCode 10 / versionName 1.9 | 工具返回（§7） |
| 设备 base.apk SHA-256 | `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b`（与主机候选逐字节一致） | `device_apk_hash.txt` |

> 实现侧自检曾在 12:26 构建同尺寸 APK（SHA-256 `5e5465d8…`）。本轮独立 clean 重建产物字节不同属 debug 打包/签名差异；**本轮安装与验证的是 `80232c50…`**，设备侧摘要与之一致。

---

## 2. 宿主机与静态验证

| 检查 | 命令/脚本 | 结果 | 证据 |
|---|---|---|---|
| 全量宿主机单测 | `./gradlew --offline :app:testDebugUnitTest --rerun-tasks` | **39 套件 / 300 项 / 0 失败 / 0 错误 / 0 跳过** | `unit_test.log`、`unit_test_summary.txt` |
| 独立验证（N-1） | `verification/NotificationsPermissionWiringItem001VerificationTest`（本轮新增） | **6/6 PASS** | JUnit XML / 汇总 |
| 独立验证（N-2） | `verification/GatewayReadStateItem002VerificationTest`（本轮新增） | **6/6 PASS** | 同上 |
| 独立验证（N-3） | `verification/ReportRegenerateFeedbackItem003VerificationTest`（本轮新增） | **9/9 PASS** | 同上 |
| 既有独立静态核查 | `evidence/software_verify_aa005_android_check.py` | **54/54 PASS，0 观察** | `static_aa005.log` |
| 既有独立门禁 | `evidence/aa005_android_check/run_gates.py` | **12/12 EXIT=0** | `gates.log` |
| 布局结构核查 | `evidence/product_fix_20261111/static_layout_check.py` | **OK（15 项）** | `static_layout_check.log` |

独立验证用例使用独立期望表，覆盖：
- N-1：API29/API31 原因与文案矩阵（蓝牙关、定位关、定位权限缺失、扫描权限缺失互不混同）、Fragment launcher 接线、对话框无 Activity 级请求、`stopScan` 清 `pendingScan`、保存/清除/启动路径不请求权限、API 分层请求集合。
- N-2：`success/rejected/lengthRefused/unaligned` 全映射、枚举不含 `CONNECTING`、无 Android import、同输入结果稳定。
- N-3：零设备/无样本/空内容 → `NO_DATA`；成功有内容 → `NONE`；失败 → `FAILED`；任何终态恢复按钮；未知样本数语义；无 Android import；文案常量稳定。

---

## 3. 真机功能验证（API 29，真实系统与真实网关）

### 3.1 ITEM-001（N-1 扫描权限结果与自动续跑）

| 用例 | 刺激 | 实际观察 | 判定 | 证据 |
|---|---|---|---|---|
| N1-D-01 | 撤销定位权限后点击「搜索附近网关」→ 系统弹窗选「仅在使用中允许」 | 授权结果 `12:54:08.671`；`GatewayDiscovery scan started filterByFfe0=true` `12:54:08.734`（+63 ms），**无需再次点击**；找到真实候选 `xxx 84:C2:E4:03:02:02 RSSI=-50 FFE0` | PASS | `logcat_scan_grant_full.txt`、`ui_08/09` |
| N1-D-02 | 保持扫描至超时 | `scan started 12:54:08.734` → `scan stopped 12:54:18.749`（10.015 s）；无重复启动/迟到回调 | PASS | 同上 |
| N1-D-03 | 关闭系统蓝牙后点击搜索 | Toast/状态：**“蓝牙未开启（去开启）”**；logcat 无 `scan started`；未请求权限 | PASS | `screen_scan_bt_off_toast.png`、`logcat_scan_bt_off.txt` |
| N1-D-04 | 定位总开关关闭（权限已授予）后点击搜索 | Toast：**“搜索网关需要开启定位（去开启）”**；未扫描；原因非“权限缺失” | PASS | `screen_scan_location_off_toast.png`、`logcat_scan_location_off.txt` |
| N1-D-05 | 撤销权限后点击搜索 → 系统弹窗选「拒绝」 | 状态栏与 Toast：**“搜索网关需要定位权限（去授权）：缺少 ACCESS_FINE_LOCATION”**；无 `scan started`；仅一次请求 | PASS | `screen_scan_permission_denied_toast.png`、`logcat_scan_denied.txt` |
| N1-D-06 | 再次点击 →「拒绝且不再询问」；随后再次点击搜索 | 首次结果 `result=7`，无扫描；再次点击时系统权限 Activity 立即返回 `result=2`，**无可见弹窗**，仅显示具体缺失提示，无扫描；无自动重复请求 | PASS | `logcat_scan_dont_ask.txt`、`logcat_scan_after_dont_ask.txt`、`screen_scan_after_dont_ask_toast.png` |
| N1-D-07 | ①权限弹窗显示期间按返回；②扫描进行中关闭对话框 | ① MIUI 权限对话框**不响应返回键**（两次 BACK 后仍显示，无可见关闭控件），该刺激在本机不可执行；② 扫描中点击「关闭」后 `scan stopped`（`13:21:52.689 → 13:21:56.257`），无迟到扫描、无 `FATAL/ANR`，页面正常返回 | PASS（①为平台限制，见 §6/§8） | `screen_after_prompt_dismiss_2.png`、`logcat_scan_dismiss_dialog.txt` |
| N1-D-08 | 授权状态下 0.5 s 间隔连续点击搜索 3 次 | `start 21.731 → stop 23.029 → start 23.055 → stop 24.318 → start 24.348 → stop 34.361`；每次重启前先停旧扫描，同一时刻最多一个扫描，最终 10.013 s 有界停止 | PASS | `logcat_rapid_clicks.txt` |
| N1-D-09 | 冷启动（无定位权限）/ 保存地址 / 清除地址 | 冷启动无权限请求、无崩溃、首页正常；保存与清除均不请求定位；清除后对话框显示“未配置网关地址” | PASS | `logcat_startup_no_location.txt`、`screen_startup_no_location.png`、`logcat_gateway_clear.txt`、`screen_gateway_cleared.png` |
| N1-D-10 | 撤销权限后点击「扩大搜索（不过滤）」→ 授权 | 授权 `13:10:42.324` → `scan started filterByFfe0=false` `13:10:42.381`（+57 ms），`scan stopped 13:10:52.390` | PASS | `logcat_expand_search_grant.txt` |

### 3.2 ITEM-002（N-2 读取终态链路状态，真实网关）

| 用例 | 刺激 | 实际观察 | 判定 | 证据 |
|---|---|---|---|---|
| N2-D-01 | 保存真实网关 `84:C2:E4:03:02:02`，启动守护，等待一次读取 | `BLE target` `13:16:00.474` → `CONNECTING` → Connected `.866` → `MTU negotiated: mtu=240 requiredMtu=35` `13:16:02.655` → 读取 → `BLE frame rejected: NO_DEVICE (devCount=0)` `13:16:03.340` → `link_state=GATEWAY_NO_DEVICE`；首页显示 **“网关已连接，暂无设备（N=0）”**（读取完成后 ≤3 s） | PASS | `logcat_guard_cycle1_scheduled.txt`、`screen_guard_cycle1_scheduled.png` |
| N2-D-02 | 保持守护，观察第二个 180 s 周期 | 第二次读取 `13:19:03.364` → `GATEWAY_NO_DEVICE` `13:19:06.834`；与首周期间隔 **182.9 s**；未回到长时间 CONNECTING | PASS | `logcat_guard_cycle2.txt` |
| N2-D-05 | 在上述会话观察业务语义 | `devCount=0` 不产生读数：无 ` id=` 处理日志、无入库/入队；`upload_pending_count` 未增加（待发 0）；未触发告警 | PASS（限 GATEWAY_NO_DEVICE） | `myprefs_guard_cycle1.xml`、`myprefs_after_guard_stop.xml` |
| N2-D-06 | 从 CONNECTING 到终态的持续时间 | 周期 1：`13:16:00.475 → 13:16:03.341`（2.9 s）；周期 2：`13:19:03.365 → 13:19:06.834`（3.5 s）；均落到 `GATEWAY_NO_DEVICE`，无残留 CONNECTING | PASS | 同上 |
| N2-D-03 | 真实有效批次 | 当前网关无真实传感器数据 | **NOT EXECUTED**（条件缺失） | — |
| N2-D-04 | 全占位批次 | 无真实源可构造 | **NOT EXECUTED**（条件缺失） | — |

### 3.3 ITEM-003（N-3 报告再生成反馈）

| 用例 | 刺激 | 实际观察 | 判定 | 证据 |
|---|---|---|---|---|
| N3-D-01 | 零设备，点击「重新生成」 | `DailyReportWorker: No saved devices, skipping report` → `SUCCESS` `13:12:08.583`；Toast **“暂无可生成的数据（尚无设备读数）”** `13:12:08.670`；按钮恢复 | PASS | `logcat_report_regenerate.txt`、`screen_report_click_085s.png` |
| N3-D-02 | 连续快速点击 3 次 | 三个 Worker 依次执行（`13:12:36.638 / 37.402 / 38.174`），每次在下一请求前完成；**无并行/重叠**；每次点击各自得到终态提示 | PASS（串行；因完成 <10 ms，未触发并发守卫，见 §8-O3） | `logcat_report_triple_click.txt` |
| N3-D-03 | 点击后立即返回，重复 3 次 | 进程 PID `27753` 未变化；logcat **0 条 `FATAL EXCEPTION` / `ANR in com.jinyuni…`**；Worker 均 SUCCESS | PASS | `logcat_report_back_destroy.txt` |
| N3-D-04 | 真机诱导失败 | 无非破坏性失败诱导方式（不伪造/不破坏数据） | **NOT EXECUTED**（宿主纯逻辑已覆盖 FAILED 分支） | — |
| N3-D-05 | 有数据成功刷新 | 当前 0 已保存设备、无当日数据 | **NOT EXECUTED**（条件缺失） | — |

---

## 4. 兼容面与回归

- 全量宿主机 300 项、既有 54 项静态、12 门禁、15 布局结构全部通过（§2）。
- 真机会话全程无本应用 `FATAL EXCEPTION` / `ANR`；进程存活。
- 版本仍为 versionCode 10 / versionName 1.9；未新增权限声明（本轮安装包权限集与 AB-002 一致）。
- 未修改产品源码、协议、存储、上传字段、告警阈值与节拍。

---

## 5. 环境与清理

| 项 | 初始 | 测试中 | 结束 |
|---|---|---|---|
| 定位权限 | granted=true | 多次撤销/授予/拒绝/不再询问/询问 | **granted=true**（恢复） |
| 蓝牙 | on | off（验证 N1-D-03）→ on | **on** |
| 定位总开关 | mode=3 | mode=0（验证 N1-D-04）→ mode=3 | **mode=3** |
| 网关地址 | 未配置 | 保存真实候选 `84:C2:E4:03:02:02`（N2）→ 清除 | **未配置（cleared=true）** |
| 守护 | 停止 | 启动（N2 两周期）→ 停止 | **停止（measure_state=starting）** |
| 应用数据 | 隐私已同意、无绑定设备 | 未新增传感器绑定；待发箱 0 条 | 同初始 |

清理证据：`myprefs_final.xml`、`device_package_final.txt`、`device_state_final.txt`、`screen_final_home.png`。

---

## 6. 未执行项与披露（不得记为通过）

| 项 | 原因 | 责任/恢复条件 |
|---|---|---|
| N2-D-03 有效批次 DATA_OK | 无真实传感器在网 | 联调阶段（真实网关+传感器） |
| N2-D-04 全占位 GATEWAY_NO_DATA | 无真实占位源，不伪造 BLE | 联调阶段 |
| N3-D-04 真机失败提示 | 无非破坏性失败诱导 | 宿主机矩阵已覆盖；真机可后续构造 |
| N3-D-05 有数据成功刷新 | 0 设备、无当日样本 | 需真实数据 |
| N1-D-07① 权限弹窗期间按返回 | MIUI 权限对话框不响应 BACK、无可见关闭控件；已用“扫描中关闭对话框”与“请求返回拒绝”两条路径覆盖迟到动作风险 | 设备/系统条件 |
| API 31+ 权限分支 | 无 API 31+ 设备 | 设备条件 |
| 9 台真实快照、真实 BLE 多设备/MTU 边界 | 仅 1 台真实网关、0 设备 | 联调阶段 |
| 433→网关→BLE→手机同笔一致性 | 用户安排后续联调 | 联调阶段 |
| 平台 `/upload_data` 写入/幂等/回读/401/403 | 无写接口与数据库授权 | 平台/联调阶段 |
| 注册/登录、有真实绑定数据列表、真实告警下发 | 无账号/数据 | 外部数据 |
| 真实升温/超温触发、光强双态/遮光、功耗 | 用户明确当前不能调温/改光/测功耗 | 阶段外，不安排 |
| `FW-LIGHT-DEF2-ENABLE` | 人工 `APPROVED_DEFERRED`（HR-8A11935AD0A7，「无法准备，先跳过」），属固件/需求判据 | 保持 **NOT EXECUTED**，不重测、不作为返工依据 |

**阶段通过 ≠ 产品通过**：上述项不因本轮 Android 增量通过而变为通过。

---

## 7. 观察项（不构成本轮失败）

- **O-AA006-1**：`MqtttService.onCharacteristicRead` 的 `status != GATT_SUCCESS` 分支只记日志并结束本次尝试，未直接调用 `GatewayReadState`（TD-SW-005 F-01 期望 `READ_INVALID`）。外层重试循环在 5 次尝试后写入 `CONNECT_FAILED`，因此不会永久停留 `CONNECTING`；本轮真实读取均为成功路径，未在设备上触发该分支，结论仅来自静态阅读。
- **O-AA006-2**：守护停止状态下清除网关地址后，首页链路状态条仍保留上一次读取的“网关已连接，暂无设备（N=0）”文本，直到下一次读取/状态更新；配置本身已正确清除（对话框显示“未配置网关地址”）。不属本轮验收判据。
- **O-AA006-3**：零设备时报告 Worker 在点击后约 10–50 ms 内完成，进度/禁用状态短于截图采集间隔（0.35 s 首帧已恢复）；Toast 终态反馈正常，无静默完成。按钮禁用逻辑由静态核查（同步置禁用）与 N3-H 矩阵覆盖。
- **O-AA006-4**：选择“拒绝且不再询问”后，用户再次点击搜索时 MIUI 仍会启动权限 Activity 并立即返回 `result=2`（无可见弹窗）；App 未自动重复请求，提示为具体缺失权限。

---

## 8. 结论

- **ITEM-001 / ITEM-002 / ITEM-003 的正常与异常路径在本轮可执行范围内均通过**；真实网关 `devCount=0` 的终态与下一周期已在 Redmi 8A/API 29 上实测（MTU 240、FFE2 160 B 场景）。
- 条件项与外部条件项已逐条披露为 NOT EXECUTED，未以宿主结果替代；人工延期的 `FW-LIGHT-DEF2-ENABLE` 不计入本轮必需项，也未重测。
- 本轮未测出产品缺陷；判定 **TEST_PASS（限定 AA-006 Android 增量与已执行范围）**。完整产品验收仍受 §6 项约束。

---

## 9. 证据文件索引

| 文件 | 内容 |
|---|---|
| `identity.txt` | 提交、构建、APK 身份与宿主结果摘要 |
| `build.log` / `unit_test.log` / `unit_test_summary.txt` | 构建与 39 套件/300 项结果 |
| `apk_badging.txt` / `apksigner_verify.txt` | 包版本与签名 |
| `device_apk_hash.txt` / `device_state_pre.txt` / `device_package_pre.txt` | 设备与安装字节身份 |
| `logcat_scan_grant_full.txt` / `ui_08_scan_permission_prompt.xml` / `ui_09_after_scan_timeout.xml` | N1-D-01/02 |
| `screen_scan_bt_off_toast.png` / `logcat_scan_bt_off.txt` | N1-D-03 |
| `screen_scan_location_off_toast.png` / `logcat_scan_location_off.txt` | N1-D-04 |
| `screen_scan_permission_denied_toast.png` / `logcat_scan_denied.txt` | N1-D-05 |
| `logcat_scan_dont_ask.txt` / `logcat_scan_after_dont_ask.txt` / `screen_scan_after_dont_ask_toast.png` | N1-D-06 |
| `screen_after_prompt_dismiss_2.png` / `logcat_scan_dismiss_dialog.txt` | N1-D-07 |
| `logcat_rapid_clicks.txt` | N1-D-08 |
| `logcat_startup_no_location.txt` / `screen_startup_no_location.png` / `logcat_gateway_clear.txt` / `screen_gateway_cleared.png` | N1-D-09 |
| `logcat_expand_search_grant.txt` | N1-D-10 |
| `logcat_guard_cycle1_scheduled.txt` / `screen_guard_cycle1_scheduled.png` / `logcat_guard_cycle2.txt` | N2-D-01/02/06 |
| `myprefs_guard_cycle1.xml` / `myprefs_after_guard_stop.xml` | N2-D-05 |
| `logcat_report_regenerate.txt` / `screen_report_click_*.png` | N3-D-01 |
| `logcat_report_triple_click.txt` | N3-D-02 |
| `logcat_report_back_destroy.txt` | N3-D-03 |
| `static_aa005.log` / `gates.log` / `static_layout_check.log` | 静态/门禁/布局 |
| `myprefs_final.xml` / `device_package_final.txt` / `device_state_final.txt` / `screen_final_home.png` | 清理与最终状态 |
