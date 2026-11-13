# Android 候选包独立核验报告（android_package_check · ST-APK-007）

- 负责角色：软件测试（`software_tester.software_verification`）
- 执行时间：2026-11-13 13:43–14:17（Asia/Shanghai；设备/工具时间戳为本地时间）
- 被测对象：`artifacts/android_build.md`（AB-003 / BUILD-007）声明的当前候选 APK
- 源码身份：工作区 HEAD `952670b9d9274f42ebf0fb48d6fbe2ec4a2d1103`；产品源码基线 `a12d12b570d541a9d348a1b6d6846275d871c7d4`。`git diff a12d12b..HEAD -- dengbei_care` 仅 3 个测试侧独立验证用例文件（`evidence/android_package_check_20261113/source_diff_stat.txt`），产品源码无差异。
- 设备：Redmi 8A（`olivelite`），Android 10 / API 29，serial `12f046690506`
- 结论：**TEST_PASS（限定范围：当前 APK 的包身份、构建入口、安装/覆盖升级、启动与资源装载、已执行关键用户路径；不含真实有效批次/平台写入/API 31+ 等未执行项）**
- 阶段边界：本结论只针对当前 Android 候选包与本次已执行范围，**不等于产品总验收通过**；跨链路与平台项见 §7。

---

## 1. 被测包身份（本轮实测）

| 项 | 实测值 | 证据 |
|---|---|---|
| APK 路径 | `dengbei_care/app/build/outputs/apk/debug/app-debug.apk` | `apk_identity.txt` |
| 大小 | 10,370,964 B | 同上 |
| SHA-256 | `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b` | 同上 |
| MD5 | `a97ed9bef658ee89cdf6e2b49447ce69` | 同上 |
| 包名 / 版本 | `com.jinyuni.dengbei_care` / `versionCode 10`、`versionName 1.9` | `apk_badging.txt`、`device_package_post.txt` |
| SDK | `minSdk 26` / `targetSdk 34` / `compileSdk 34` | `apk_badging.txt` |
| 启动组件 | `com.jinyuni.dengbei_care.MainActivity` | `apk_badging.txt` |
| 签名 | APK Signature Scheme v2 `Verifies`；单签名者 `C=US, O=Android, CN=Android Debug`，证书 SHA-256 `773e4ef6…9255`（项目既有 debug keystore） | `apksigner_verify.txt` |
| 设备侧 `base.apk` | SHA-256 `80232c50…de4b`，与主机候选**逐字节一致** | `device_apk_hash.txt`、`device_apk_path.txt` |
| 包内容检查 | 1130 个条目；无 `verification`/`androidTest`/`/test/` 条目，无 instrumentation；无测试专用组件 | `apk_content_check.txt` |

交付包为**debug 候选**（`flags` 含 `DEBUGGABLE`），与构建声明一致；未声称正式发布签名或上架。`release` 变体本轮仍未产出（离线缓存缺 lint 依赖），不在本候选范围。

## 2. 构建入口与宿主机回归（本轮重新执行）

| 检查 | 命令 | 结果 | 证据 |
|---|---|---|---|
| 构建入口 | `cd dengbei_care && ./gradlew --offline :app:assembleDebug --console=plain` | `BUILD SUCCESSFUL`（35 tasks all up-to-date）；构建后 APK SHA-256 仍为 `80232c50…` | `assemble_debug.log`、`apk_identity.txt` |
| 全量宿主机单测 | `./gradlew --offline :app:testDebugUnitTest --rerun-tasks --console=plain` | `BUILD SUCCESSFUL`；**39 套件 / 300 项 / 0 失败 / 0 错误 / 0 跳过** | `unit_test_rerun.log`、`unit_test_summary.txt` |
| 独立静态核查 | `python evidence/software_verify_aa005_android_check.py` | **54/54 PASS，0 观察**（EXIT=0，含清单/周期/MTU/codec 项） | `static_aa005.log` |
| 独立门禁 | `python evidence/aa005_android_check/run_gates.py` | **12/12 gates passed**（EXIT=0） | `gates.log` |
| 布局/文案结构 | `python evidence/product_fix_20261111/static_layout_check.py` | `RESULT: OK`（版本 10/1.9、底部导航、D-01/D-02 入口结构） | `static_layout_check.log` |
| 包内规则文案 | `python evidence/android_package_check_20261111/check_packaged_rule_strings.py` | `RESULT: OK`：13 个 dex 均命中“采样周期 3 分钟”“升高超过 0.9”“且无光”“35.0℃”“按条件上报” | `packaged_rule_strings.txt` |

## 3. 安装与覆盖升级（真机）

| 项 | 实测 | 证据 |
|---|---|---|
| 安装动作 | `android_test install`（自动处理安装确认，`confirmation_clicks=0`） | `install_result.json` |
| 安装校验 | 工具返回 `installed_version_verified`：包名、`version_code=10`、`version_name=1.9`、`sha256=80232c50…` | 同上 |
| 安装后设备包 | `base.apk` SHA-256 `80232c50…`，与本地候选一致 | `device_apk_hash.txt` |
| 升级语义 | `firstInstallTime=2026-11-10 19:25:25` 保留；`lastUpdateTime=2026-11-13 13:45:30` 更新 | `device_package_pre.txt`、`device_package_post.txt` |
| 数据保留 | 覆盖安装后 `MyPrefs` 仍存在（`privacy_policy_accepted=true` 等），未清除应用数据 | 安装后 `run-as` 读取、最终 `MyPrefs` 导出 |

## 4. 启动、资源装载与真机关键用户路径（本轮实际执行）

| 用例 | 操作/刺激 | 实际观察 | 判定 | 证据 |
|---|---|---|---|---|
| 启动与首页 | `android_test launch` + `ui_dump`/截图 | 首页“爱守护”“启动”、空态“还没有添加设备”“去添加”、三页签渲染正常；前台服务通知“爱守护正在运行” | PASS | `ui_00_launch.xml`、`screen_00_home.png` |
| 我的页 | 点击「我的」 | “用户：未登录”“账户”“查看今日报告”“打开传感器列表”、网关地址输入/保存/扫一扫、传感器注册区均可见 | PASS | `ui_01_my_page.xml` |
| 传感器列表 | 点击「打开传感器列表」 | “已保存的 ID（0）”“点选可多选，删除选中可批量移除” | PASS | `ui_02_sensor_list.xml` |
| 设定页 | 点击「设定」 | 设备范围、下降报警（2 / 0.1）、阈值报警（35/29/75）、默认值不变 | PASS | `ui_03_settings.xml` |
| 网关对话框 | 「我的」→「网关与上传设置」 | 标题、地址输入、搜索/扩大搜索/清除/保存、云端上传状态“无（待发 0 条）”、重试上传、关闭 | PASS | `ui_04_gateway_dialog.xml` |
| **N1-D-01** 首次授权自动扫描 | `pm revoke FINE/COARSE` 后点击「搜索附近网关」→ 系统对话框点「仅在使用中允许」 | 授权结果 `14:15:03.559 result=4`；`GatewayDiscovery scan started filterByFfe0=true` `14:15:03.632`（**+73 ms，无需再次点击**）；`scan stopped` `14:15:13.658`（10.026 s）；发现真实候选 `xxx 84:C2:E4:03:02:02 RSSI=-51 FFE0` | PASS | `ui_15_permission_prompt.xml`、`logcat_grant_autoscan.txt`、`logcat_grant_autoscan_stop.txt`、`ui_20_after_grant_scan.xml` |
| **N1-D-02** 有界停止 | 授权状态点击搜索并观察 12 s | `scan started 13:48:19.773` → `scan stopped 13:48:29.787`（10.014 s）；停止后无新扫描 | PASS | `logcat_scan.txt` |
| **N1-D-03** 蓝牙关闭 | 快捷开关关闭蓝牙（`bluetooth_on=0`）后点击搜索 | Toast“爱守护：蓝牙未开启（去开启）”；无 `scan started` | PASS | `ui_10_scan_bt_off.xml`、`logcat_scan_bt_off.txt`、`ui_09_quick_settings.xml` |
| **N1-D-04** 定位总开关关闭 | `location_mode=0`（权限已授予）后点击搜索 | Toast“爱守护：搜索网关需要开启定位（去开启）”；无 `scan started`；原因非“权限缺失” | PASS | `ui_12_scan_location_off.xml`、`logcat_scan_location_off.txt`、`ui_13_quick_settings_3.xml` |
| **N1-D-05** 权限拒绝 | 撤销定位权限后点击搜索 → 系统对话框点「拒绝」 | `Permission grant result … result=6`（拒绝）；Toast 与对话框状态“搜索网关需要定位权限（去授权）：缺少 ACCESS_FINE_LOCATION”；无 `scan started`；未二次弹窗 | PASS | `ui_15_permission_prompt.xml`、`ui_16_after_deny.xml`、`logcat_permission_denied.txt` |
| **N1-D-09** 启动/保存/清除不请求权限 | 无定位权限冷启动；保存合法地址；清除地址 | 冷启动无权限弹窗、首页正常；保存与清除均未出现权限弹窗（当次权限已授予，请求触发范围另由静态核查 N1-S-03 覆盖） | PASS | `ui_14_cold_start_no_location.xml`、`logcat_scan_location_off.txt`、`ui_17_gateway_cleared.xml` |
| **N2-D-01** 真实网关终态 | 保存真实候选 `84:C2:E4:03:02:02`，触发一次读取 | `BLE target` → `CONNECTING` → Connected → `MTU negotiated: mtu=240 (status=0), expectedDevices=1, requiredMtu=35` → `BLE frame rejected: NO_DEVICE (devCount=0)` → `link_state=GATEWAY_NO_DEVICE`；首页“网关已连接，暂无设备（N=0）”；无残留 `CONNECTING` | PASS | `ui_06_gateway_saved.xml`、`ui_07_after_start.xml`、`logcat_save_trigger.txt` |
| **N2-D-02** 下一 180 s 周期 | 保持后台读取循环，观察第二个周期 | 周期 1 `14:10:01.845`，周期 2 `14:13:04.575`（间隔 **182.7 s**）；两周期均 `GATEWAY_NO_DEVICE`、MTU 240、devCount=0 | PASS | `logcat_cycle_cadence.txt` |
| **N3-D-01** 零设备重新生成 | 报告页“暂无报告”→ 点击「重新生成」 | `ReportScheduler: Enqueued one-shot daily report` → `DailyReportWorker: No saved devices, skipping report` → Worker `SUCCESS`；App 发出 Toast（`13:52:41.926 Toast Show`）；按钮恢复；页面终态“尚未生成过报告/暂无报告” | PASS | `ui_08_report_page.xml`、`logcat_report_and_cycle2.txt` |
| 守护启动/停止 | 点击「启动」→ 运行 → 点击「停止」 | 按钮“启动↔停止”切换正确；`measure_state` 由 `stopping`（运行中）变为 `starting`（停止）；无崩溃 | PASS | `ui_07_after_start.xml`、`ui_18_home_before_stop.xml`、`ui_19_final_home.xml`、`MyPrefs` 导出 |
| 崩溃/ANR | 全部会话 logcat | 本应用 `FATAL EXCEPTION` 0、`ANR in com.jinyuni.dengbei_care` 0 | PASS | 全部 `logcat_*.txt` 扫描（见 §8） |

> 说明：`MqttService` 的 180 s 读取循环在 Service 创建后持续运行（与“守护”按钮状态独立，属既有已登记行为）；本轮通过保存触发的即时读取与下一周期读取分别取得两笔真实证据。

## 5. 观察项（未计入本轮失败）

| ID | 观察 | 说明 |
|---|---|---|
| O-PKG-7-1 | 本次手机无可用外网，App 连接 broker 报 `ENETUNREACH`（`8.140.23.253:8883`） | 属设备网络条件，非 App 侧功能结论；平台写入/回读未执行（§7） |
| O-AA006-1 | `onCharacteristicRead` 的 `status != GATT_SUCCESS` 分支仅记日志，未直接映射 `READ_INVALID`，外层重试后落 `CONNECT_FAILED` | 上一轮已登记；本轮真实读取均为成功路径，未在设备触发该分支 |
| O-AA006-2 | 守护停止后清除网关地址，首页状态文本可能保留上一次读取文本 | 本轮清除后首页显示“未配置网关地址（搜索附近网关）”，未复现误导性文本 |
| O-PKG-7-2 | 进程被系统权限回收杀死后冷启动，首页按 `measure_state` 恢复“停止”按钮 | 属既有守护状态恢复设计；`MqtttService` 由 `MainActivity` 启动，非本轮缺陷 |

## 6. 人工范围决定与原失败复评（按运行指令）

- `FW-LIGHT-DEF2-ENABLE`：人工 `SKIP_CONFIRMED` / **经人工确认，未执行**（HR-8A11935AD0A7，2026-11-11T03:50:42Z）。属固件/需求判据，**不在 Android 候选范围**；本轮未重测、未重复询问，也未记为 PASS。
- 原 `PRODUCT-ACCEPTANCE-002` §5 R4 将上述已批准延期项计为整体失败，属范围错误；`PRODUCT-ACCEPTANCE-RECHECK-002` 已纠正，本轮不沿用该错误判据。
- `BOT-01`（REASSESS/自动化）属固件域，不属 Android 包检查范围；本轮未重跑、未据此改变结论。
- 产品级 `TEST_FAIL=EXTERNAL_CONDITION_UNEXECUTED` 仍有效，其依据为真实触发/跨链路/平台写入回读/功耗等未执行项；本报告的限定 `TEST_PASS` 不改变该状态。

## 7. 未执行项（不得写成通过）

| 项 | 状态 | 原因/条件 |
|---|---|---|
| N2-D-03 真实有效批次 `DATA_OK` | NOT EXECUTED | 当前网关 `devCount=0`，无真实传感器在网；联调阶段 |
| N2-D-04 全占位 `GATEWAY_NO_DATA` | NOT EXECUTED | 无真实占位源，不伪造 BLE |
| N2-D-05 状态与本地入库/上传/告警的业务语义 | 部分执行 | 已观察 `devCount=0` 无读数路径；“无新增读数/入库/入队”以既有证据与日志语义为准，未做数据库级断言 |
| N1-D-06 “拒绝且不再询问”重复请求 | 本轮未执行 | 同字节前序独立验证已覆盖（`evidence/aa006_software_test_20261113/`）；本轮未重复该破坏性较低但耗时的路径 |
| N1-D-07 扫描中关闭对话框/权限弹窗返回 | 本轮未执行 | 同字节前序证据覆盖；本轮未重复 |
| N1-D-08 连续快速点击 3 次 | 本轮未执行 | 同字节前序证据覆盖；本轮未重复 |
| N1-D-10 扩大搜索（不过滤）授权后续跑 | 本轮未执行 | 同字节前序证据覆盖；本轮未重复 |
| N3-D-02/03/04/05 连点/退出/失败/有数据刷新 | 本轮未执行 | 同字节前序证据覆盖；有数据项需真实数据 |
| 真实 BLE 有效批次、多设备/MTU 边界、9 台快照 | NOT EXECUTED | 仅 1 台真实网关、0 设备；联调阶段 |
| 平台 `/upload_data` 写入/幂等/401/403/回读、注册/登录、真实告警 | NOT EXECUTED | 手机无外网（`ENETUNREACH`），且无平台写接口/数据库授权；联调阶段 |
| API 31+ 权限分支 | NOT EXECUTED | 设备为 Android 10 / API 29 |
| 真实升温/超温、光强双态/遮光、功耗 | NOT APPLICABLE | 用户明确当前不能调温、不能改变光强、不能测功耗，不安排相关硬件测试 |

## 8. 崩溃/ANR 扫描范围

对本次目录内 12 个 `logcat_*.txt`（`logcat_scan`、`logcat_guard_cycle1`、`logcat_save_trigger`、`logcat_report_and_cycle2`、`logcat_scan_bt_off`、`logcat_scan_location_off`、`logcat_permission_denied`、`logcat_scan_granted_again`、`logcat_cycle_cadence`、`logcat_grant_autoscan`、`logcat_grant_autoscan_stop`、`logcat_final`）扫描 `FATAL EXCEPTION`、`ANR in com.jinyuni.dengbei_care`、进程崩溃标记：**均为 0 命中**。

## 9. 环境与收尾

| 项 | 开始 | 测试中 | 结束 |
|---|---|---|---|
| 定位权限 | granted=true | 撤销/拒绝/再次授权 | **granted=true** |
| 蓝牙 | on | 关闭（N1-D-03）→ on | **on（`bluetooth_on=1`）** |
| 定位总开关 | mode=3 | mode=0（N1-D-04）→ mode=3 | **mode=3** |
| 网关地址 | 未配置 | 保存真实候选 → 清除 | **未配置（`cleared=true`）** |
| 守护 | 停止 | 启动 → 停止 | **停止（`measure_state=starting`）** |
| 应用数据 | 隐私已同意、0 绑定 | 未新增绑定；待发 0 | 同初始 |

清理证据：`device_state_final.txt`、`device_package_post.txt`、最终 `MyPrefs`、`ui_19_final_home.xml`、`screen_20_final_home_after_grant.png`。未卸载、未清除应用数据、未改系统网络配置；无遗留后台诊断任务。

## 10. 判定

1. **包身份**：APK 大小/SHA-256/MD5/包名/版本/SDK/签名与交付声明完全一致；设备 `base.apk` 与主机候选逐字节一致。**通过**
2. **构建入口**：`assembleDebug` 成功且产物未变化；全量宿主机 39 套件/300 项、54 项静态、12 门禁、布局结构、包内规则文案均通过。**通过**
3. **安装/升级**：覆盖安装成功并校验版本与 SHA-256；`firstInstallTime` 保留、`lastUpdateTime` 更新、数据未清除。**通过**
4. **真机关键路径**：启动/首页/我的/设定/传感器列表/网关对话框/报告页渲染正常；N-1 授权后 73 ms 自动扫描、10 s 有界停止、蓝牙关/定位关/权限拒绝分别给出正确原因；N-2 真实网关两周期均落 `GATEWAY_NO_DEVICE`（间隔 182.7 s，MTU 240，devCount=0）；N-3 零设备重新生成产生 Worker 终态与用户提示；守护启停正确；无本应用 FATAL/ANR。**通过**
5. **未执行项与观察项不因本结论视为通过**；真实跨链路、平台写入/回读、API 31+、真实触发与有数据场景保留至后续阶段。
6. 本轮**未测出 Android 产品缺陷**；未发现正式包中混入测试专用组件或 instrumentation。

**结论：`TEST_PASS`（限定于当前 APK `80232c50…de4b`、HEAD `952670b`、本报告已执行范围）。**

---

## 11. 证据文件索引

| 文件 | 内容 |
|---|---|
| `apk_identity.txt`、`apk_badging.txt`、`apksigner_verify.txt`、`apk_content_check.txt` | 包身份、版本、SDK、签名、包内容 |
| `install_result.json`、`device_apk_path.txt`、`device_apk_hash.txt`、`device_package_pre.txt`、`device_package_post.txt` | 安装、设备包摘要、升级时间语义 |
| `assemble_debug.log`、`unit_test_rerun.log`、`unit_test_summary.txt` | 构建与 39 套件/300 项 |
| `static_aa005.log`、`gates.log`、`static_layout_check.log`、`packaged_rule_strings.txt`、`source_diff_stat.txt` | 静态核查、门禁、布局、包内规则文案、源码差异 |
| `ui_00`–`ui_20`（XML） | 各页面/对话框/权限弹窗的实际 UI 树 |
| `screen_00_home.png`、`screen_19_final_home.png`、`screen_20_final_home_after_grant.png` | 首页与收尾截图 |
| `logcat_scan.txt`、`logcat_grant_autoscan*.txt`、`logcat_scan_bt_off.txt`、`logcat_scan_location_off.txt`、`logcat_permission_denied.txt`、`logcat_scan_granted_again.txt` | N-1 各路径原始日志 |
| `logcat_save_trigger.txt`、`logcat_cycle_cadence.txt` | N-2 真实网关读取与 180 s 周期 |
| `logcat_report_and_cycle2.txt` | N-3 零设备重新生成与 Worker/Toast |
| `device_state_final.txt` | 收尾设备状态 |
