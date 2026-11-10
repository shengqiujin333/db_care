# Android 当前构建包独立核验（android_package_check）

执行角色：软件测试（`software_tester.software_verification`）
执行日期：2026-11-10
被测对象：`artifacts/android_build.md`（AB-001，候选构建）
被测源码：Git `28b78b0`（构建时工作区干净）；`git diff f516a73..HEAD -- dengbei_care` 为空，产品树与此前独立复验通过的 `f516a73` 一致
被测包：`dengbei_care/app/build/outputs/apk/debug/app-debug.apk`
版本：`versionCode 10 / versionName 1.9`，`minSdk 26 / targetSdk 34`
APK：10,365,460 B，SHA-256 `d20542219faeaf4be0d73ce4650c57e3ea25bc54183ff0117a3f61c0e41bd8b6`，MD5 `58de16b8fef549f004b92ffcec0104d9`
设备：Redmi 8A / Android 10 / API 29 / `12f046690506`；蓝牙开、定位总开关开（mode=3）、**无默认网络**（`Active default network: none`）
结论：**TEST_PASS（限定范围：包身份、安装/升级保留数据、启动与资源装载、可执行关键用户路径、宿主机回归）**

---

## 1. 包身份与签名（独立核查）

| 检查 | 命令/来源 | 结果 |
|---|---|---|
| 本地 APK 摘要 | `sha256sum` / `stat` | 10,365,460 B；SHA-256 `d2054221…8b6`（与交付声明一致） |
| 包名/版本/SDK | `aapt dump badging app-debug.apk` | `package=com.jinyuni.dengbei_care versionCode=10 versionName=1.9`；`sdkVersion 26` / `targetSdkVersion 34`；launchable `MainActivity` |
| 签名 | `apksigner verify --print-certs --verbose` | `Verifies`（v2），单签名者 `C=US, O=Android, CN=Android Debug` |
| 正式构建入口 | `./gradlew :app:assembleDebug --offline` | `BUILD SUCCESSFUL`（35 tasks，全部 up-to-date）；重跑后 APK SHA-256 不变 |
| 安装身份 | `android_test install` | `installed_version_verified`：包名、`version_code=10`、`version_name=1.9`、`sha256=d2054221…` 全部匹配；安装确认点击 0 次 |

原始：`apk_badging.txt`、`apksigner_verify.txt`、`assemble_debug.log`、`install_result.json`。

## 2. 安装/升级与数据保留

- 安装前设备已装同签名 `10/1.9`（`firstInstallTime=2026-11-10 19:25:25`，`lastUpdateTime=2026-11-10 21:51:03`）。
- 本次 `install` 覆盖升级后：`versionCode=10 / versionName=1.9`，`lastUpdateTime=2026-11-10 22:15:13`，`firstInstallTime` 保持 19:25:25 → **覆盖升级，应用数据保留**。
- 数据保留实测：`MyPrefs` 中 `privacy_policy_accepted=true`、`gateway_ble_addr_cleared=true`、`gateway_ble_addr` 为空、`measure_state=starting` 均保留；未卸载、未清数据。

原始：`device_state.txt`、`package_dump_final.txt`、`myprefs_after_stop.xml`。

## 3. 启动、资源装载与关键用户路径（真机实测）

| 步骤 | 实际观察 | 判定 |
|---|---|---|
| 启动 | `android_test launch` 成功；首页渲染“爱守护”、空态“还没有添加设备 / 前往「我的」→「传感器列表」添加设备”、底部三页签图标与文字 | PASS |
| 无崩溃 | 整段会话 logcat 无 `FATAL EXCEPTION` / `ANR in com.jinyuni.dengbei_care`；进程存活（PID 29852） | PASS |
| 前台服务 | `MainActivity.onCreate` 按既有设计启动 `MqtttService` 前台服务（`startMqttService()`，`MainActivity.kt:65`），通知“爱守护正在运行” | 正常基线行为 |
| 守护启动 | 点“启动”→ 按钮变“停止”，红色；日志显示服务周期触发 | PASS |
| 守护停止 | 点“停止”→ 按钮回“启动”；`measure_state=starting`；再次触发时 `BLE connect blocked: gateway address cleared/unset`、`link_state=GATEWAY_UNSET`（22:18:23） | PASS（清除后不回退默认网关） |
| 设定页 | 渲染“选择设备 / 全局默认值(所有设备) / 温度湿度下降报警 / 阈值报警”及默认值 | PASS（资源装载） |
| 我的页 | 渲染用户、网关编码、传感器编码/别称、注册传感器、网关与上传设置、查看今日报告等入口 | PASS（见 §6 观察项） |
| 网关与上传设置 | 对话框渲染：“未配置网关地址 / 平台身份（mac_addr）：空 / 云端上传状态：无（待发 0 条）/ 重试上传 / 搜索附近网关 / 保存 / 清除 / 关闭”；未触发写入 | PASS（呈现与清除态一致） |

原始：`ui_01_after_launch.xml`、`ui_02_after_launch_retry.xml`、`ui_03_guard_started.xml`、`ui_04_settings_tab.xml`、`ui_05_my_tab.xml`、`ui_06_gateway_dialog.xml`、`ui_07_guard_stopped.xml`、`ui_08_my_tab_scrolled.xml`、`ui_09_after_report_click.xml`、`home_guard_started.png`、`my_tab_overlap_observation.png`、`logcat_full_session.txt`、`services_pre.txt`、`services_after_stop.txt`。

## 4. 宿主机独立回归（当前源码树）

| 检查 | 命令 | 结果 |
|---|---|---|
| 全量宿主机单测（含独立验证资产） | `./gradlew :app:testDebugUnitTest --offline --rerun-tasks` | `BUILD SUCCESSFUL`；**32 套件 / 257 项 / 0 失败 / 0 错误 / 0 跳过** |
| 独立 AA-005 静态核查 | `python evidence/software_verify_aa005_android_check.py` | **54/54 PASS，0 FINDING** |
| 历史独立门禁 | `python evidence/aa005_android_check/run_gates.py` | **12/12 EXIT=0**（含版本 10/1.9、文档与清单一致性门禁） |

原始：`unit_test.log`、`unit_test_summary.txt`、`static_check_aa005.log`、`gates.log`。

## 5. 版本/文档/清单一致性

- 源码、AGP 输出、APK badging、安装后 `dumpsys` 四级均为 `10 / 1.9`。
- `software_verify_release_item014.py`（版本递增、文档与实现交叉核对、`file_manifest.txt` 覆盖与路径有效性）EXIT=0。

## 6. 观察项（如实记录；未计入本轮交付失败，依据见各条）

| ID | 观察 | 证据 | 性质/建议 |
|---|---|---|---|
| O-PKG-1 | “我的”页在 Redmi 8A（720×1520）上，本轮新增的“网关与上传设置”按钮 `[32,1168][688,1264]` 与既有“传感器别称”输入框 `[33,1104][533,1224]`、“注册传感器”按钮 `[452,1128][688,1224]` 在底部约 56 px 重叠；两控件上部仍可见可点，注册路径未失效 | `ui_05_my_tab.xml`、`my_tab_overlap_observation.png` | 本轮 `7848b99` 新增该按钮引入的布局观感问题，非声明验收项；建议将页面改为可滚动容器或把新按钮纳入顶部流式约束，避免遮挡 |
| O-PKG-2 | 既有“查看今日报告”按钮 `[32,1280][688,1376]` 整体位于底部导航 `[0,1264][720,1424]` 之下；实际点击被导航栏拦截（点击后页签变为“设定”） | `ui_05_my_tab.xml`、`ui_09_after_report_click.xml`、`my_tab_overlap_observation.png`；按钮在基线 `fe9a81c` 已存在 | **基线既有布局问题**，非本轮引入；报告内容仍可由 `DailyReportWorker` 的早报通知打开 `ReportActivity`（代码 81–86 行）；建议后续布局修复时一并处理 |

## 7. 未执行项与外部条件（不得写成通过）

| 项 | 缺失条件/依据 | 状态 |
|---|---|---|
| 真实网关 BLE → App → MQTT → `/upload_data` → 服务持久化与回读（DLV-03） | 用户明确“网关与手机跨链路验证安排在后续联调阶段”（Project Goal 补充条件 4）；手机无默认网络；无隔离测试身份/服务写读授权 | NOT EXECUTED |
| 真实 MTU/`value.size`/≤9 台与第 10 台拒绝（DLV-06） | 无真实多设备与可控源；网关本次未配置（`GATEWAY_UNSET`） | NOT EXECUTED |
| 真实 401/403 与持久化回读 | 无默认网络；无隔离账号与服务写/回读授权 | NOT EXECUTED |
| 真实传感器读数下的卡片/详情/24 h 图表与高温强调截图 | 无真实读数源（无绑定设备、无网关数据） | NOT EXECUTED |
| API 31+ 权限分支 | 无 API 31+ 真机/AVD | NOT EXECUTED |
| 调温/改变光强/遮光/功耗 | 用户明确禁止安排 | NOT APPLICABLE |

## 8. 设备收尾

- 守护已停止（按钮“启动”，`measure_state=starting`）；网关地址保持“未配置”；App 保持安装（`10/1.9`）。
- 蓝牙保持开启（`bluetooth_on=1`）、定位总开关保持 mode=3、无默认网络；未修改系统设置；未产生用户数据。
- 本能力启动的命令/工具均已结束，无后台任务遗留。
