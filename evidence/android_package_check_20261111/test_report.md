# Android 当前候选包独立核验报告（android_package_check · ST-AA005-R4）

- 负责角色：软件测试
- 时点：2026-11-11 19:16–19:25（Asia/Shanghai；设备与工具时间戳为 UTC）
- 被测对象：`artifacts/android_build.md`（AB-002）声明的当前候选 APK
- 源码身份：Git `a421d0daf52a914781acd4112775d662acfd7312`（工作区干净）；产品树与修复提交 `dacf888` 一致
- 设备：Redmi 8A（`olivelite`），Android 10 / API 29，serial `12f046690506`
- 结论：**TEST_PASS（限定范围：当前 APK 的包身份、正式构建入口、安装/覆盖升级、启动与资源装载、已执行关键用户路径、宿主机回归与独立静态/门禁；不含真实 BLE/MQTT/平台链路）**

## 1. 被测身份

| 项 | 实测 |
|---|---|
| APK 路径 | `dengbei_care/app/build/outputs/apk/debug/app-debug.apk` |
| 大小 | 10,365,464 B |
| SHA-256 | `71a041d322368c20a181d5716c44ac4d1b2c8a3869926b0047cd96b61e2c10d8` |
| MD5 | `1e17e0c281715fddd26d308246d6424c` |
| 包名/版本 | `com.jinyuni.dengbei_care`，`versionCode 10 / versionName 1.9` |
| SDK | `minSdk 26 / targetSdk 34 / compileSdk 34` |
| 启动组件 | `MainActivity` |
| 签名 | APK Signature Scheme v2 验证通过；单签名者（Android Debug 证书，项目既有 debug keystore） |

原始记录：`apk_badging.txt`、`apksigner_verify.txt`、`apk_identity.txt`。

## 2. 正式构建入口

| 检查 | 命令 | 结果 |
|---|---|---|
| 构建 | `cd dengbei_care && ./gradlew :app:assembleDebug --offline` | `BUILD SUCCESSFUL`（35 tasks，全部 up-to-date）；构建后 APK 哈希不变，仍为 `71a041d3…` |

原始记录：`assemble_debug.log`。

## 3. 安装与升级（真机）

| 项 | 实测 | 原始证据 |
|---|---|---|
| 安装动作 | `android_test install`（自动处理安装确认，`confirmation_clicks=0`） | `install_result.json` |
| 安装校验 | 工具返回 `installed_version_verified`：包名、`version_code=10`、`version_name=1.9`、`sha256=71a041d3…` | `install_result.json` |
| 安装前设备包 | `base.apk` SHA-256 `71a041d3…`（同一候选） | `device_path_pre.txt`、`device_apk_hash_pre.txt` |
| 安装后设备包 | `base.apk` SHA-256 `71a041d3…`，与本地候选逐字节一致 | `device_path_post.txt`、`device_apk_hash_post.txt` |
| 升级语义 | `firstInstallTime=2026-11-10 19:25:25` 保留；`lastUpdateTime=2026-11-11 19:18:26` 更新 | `device_package_pre.txt`、`device_package_post.txt` |
| 数据保留 | `privacy_policy_accepted=true`、`gateway_ble_addr` 为空且 `gateway_ble_addr_cleared=true`、`measure_state=starting`、`upload_blocked_reason` 为空 | `myprefs_after_reinstall.xml` |

## 4. 启动、资源装载与关键用户路径

| 项 | 操作 | 实际结果 | 原始证据 |
|---|---|---|---|
| 启动 | `android_test launch` | `MainActivity` 前台；`ui_info` 返回包名/Activity/PID | `ui_info_after_launch.json` |
| 首页 | `ui_dump` + 截图 | “爱守护”“启动”、空态“还没有添加设备”“去添加”、三页签（首页/设定/我的）；NavHost `[0,183][720,1264]` 与底部导航 `[0,1264][720,1424]` 不相交 | `ui_01_after_launch.xml`、`screen_01_home_after_launch.png` |
| 我的页 | 点击「我的」页签 | “用户：未登录”“账户”“查看今日报告”“打开传感器列表”均可见；页面为可滚动 `ScrollView` | `ui_02_my_page.xml` |
| 我的页（滚动后） | 上滑 | 「注册传感器」`[32,976][688,1072]` 与「网关与上传设置」`[32,1120][688,1216]` 不相交，且均在底部导航之上 | `ui_03_my_page_scrolled.xml` |
| 网关与上传设置 | 点击「网关与上传设置」 | 对话框显示“未配置网关地址”“平台身份（mac_addr）：”（空）、“云端上传状态：无（待发 0 条）”“重试上传”，可关闭 | `ui_04_gateway_dialog.xml` |
| D-01 报告入口 | 点击「查看今日报告」 | 打开 `ReportActivity`：“早起报告”“尚未生成过报告”“重新生成”，未误切页签 | `ui_05_report_activity.xml` |
| D-02 传感器列表入口 | 点击「打开传感器列表」 | 进入 `SensorIdListFragment`：“已保存的 ID（0）”“点选可多选，删除选中可批量移除”，“删除选中”禁用 | `ui_06_sensor_list.xml` |
| 守护启动 | 点击「启动」 | 按钮变“停止”；链路状态“未配置网关地址（搜索附近网关）”，清除后不回退默认地址 | `ui_07_guard_started.xml`、`logcat_guard_session.txt` |
| 守护停止 | 再次点击（“停止”） | 按钮恢复“启动”；`finish_guard_time=2026-11-11 19:22:43` | `ui_08_guard_stopped.xml`、`myprefs_after_reinstall.xml` |
| 设定页 | 点击「设定」页签 | 正常渲染：选择设备、下降报警（2 / 0.1）、阈值报警（35/29/75/40），与既有默认值一致 | `ui_09_settings_page.xml`、`screen_02_settings.png` |
| 崩溃检查 | 整段 logcat | 无 `FATAL EXCEPTION`、无本应用 ANR；进程存活 | `logcat_launch_session.txt`、`logcat_guard_session.txt` |

## 5. 回归与独立静态检查（本轮重新执行）

| 检查 | 命令 | 结果 | 证据 |
|---|---|---|---|
| 宿主机全量单测 | `./gradlew :app:testDebugUnitTest --offline --rerun-tasks` | `BUILD SUCCESSFUL`；**33 套件 / 261 项 / 0 失败 / 0 错误 / 0 跳过** | `unit_test.log`、`unit_test_summary.txt` |
| AA-005 独立静态核查 | `python evidence/software_verify_aa005_android_check.py` | **54/54 PASS，0 项观察**（EXIT=0） | `static_check_aa005.log` |
| 独立门禁 | `python evidence/aa005_android_check/run_gates.py` | **12/12 gates passed**（EXIT=0） | `gates.log` |
| 布局/文案结构 | `python evidence/product_fix_20261111/static_layout_check.py` | `RESULT: OK`（版本 10/1.9、底部导航保留、D-01/D-02/D-03/D-04a 结构） | `static_layout_check.log` |
| 包内规则文案 | `python evidence/android_package_check_20261111/check_packaged_rule_strings.py` | **RESULT: OK**：APK 的 13 个 dex 中均命中“采样周期 3 分钟”“升高超过 0.9”“且无光”“35.0℃”“按条件上报” | `packaged_rule_strings.txt` |

## 6. 观察项（未计入本轮失败）

| ID | 观察 | 依据 |
|---|---|---|
| O-PKG-3 | 守护停止后 `MqtttService` 仍以前台服务存在（`isForeground=true`），且既有文档已登记“关闭守护后仍可能进行周期读取”的已知行为（P-08～P-13）。本轮未观察到用户可见崩溃或数据异常 | `device_state_final.txt`、`ui_08_guard_stopped.xml` |
| O-PKG-4 | 设备详情页（规则文案/24 h 图/高温强调）需已绑定设备与真实读数才能从 UI 进入；本轮以宿主机用例（`DetailPresentationTest` 等）覆盖逻辑，未做真机详情截图 | `unit_test_summary.txt` |

以上观察不属于本任务声明的验收失败，后续联调/维护阶段跟踪。

## 7. 未执行项（不得写成通过）

| 项 | 状态 | 原因 |
|---|---|---|
| 真实 BLE/GATT/MTU、真实网关发现与配对 | NOT EXECUTED | 无已配置网关/已绑定传感器；项目补充条件明确跨链路属后续联调阶段 |
| 真实 MQTT / `POST /upload_data` 写入与回读、401/403 | NOT EXECUTED | 无隔离测试账号与平台写读授权；本轮仅验证 App 侧状态展示与入口 |
| 传感器列表“有数据时”的列出/删除、详情页真实读数 | NOT EXECUTED | 当前无已绑定数据；未写入用户数据 |
| API 31+ 蓝牙权限分支 | NOT EXECUTED | 设备为 Android 10 / API 29 |
| 9 台真实快照与第 10 台拒绝 | NOT EXECUTED | 无真实多设备源 |
| 调温 / 改变光强 / 遮光 / 功耗测量 | NOT APPLICABLE | 用户明确当前不能调节温度/光强、不能测功耗，不安排相关硬件测试 |
| 固件 `FW-LIGHT-DEF2-ENABLE` 人工跳过项 | 不在本能力范围 | 固件对象；保持既有 `SKIP_CONFIRMED`（NOT EXECUTED），本报告不重复询问 |

## 8. 判定

1. **包身份与正式构建**：当前 APK 由正式构建入口产出，包名/版本/SDK/签名与交付声明完全一致；重建后哈希不变。**通过**
2. **安装/升级**：当前候选覆盖安装成功，工具校验版本与 SHA-256 与本地候选一致；`firstInstallTime` 保留、`lastUpdateTime` 更新、应用数据未被清除。**通过**
3. **启动、资源装载与关键用户路径**：首页、我的、设定、网关对话框、报告页、传感器列表均正常渲染；守护启停语义正确；D-01/D-02/D-03/D-04a 修复在本轮真机操作中未复现；无本应用 FATAL/ANR。**通过**
4. **回归**：261 项宿主机用例、54 项独立静态核查、12 个独立门禁、布局/文案结构检查全部通过。**通过**
5. **未执行项与观察项不因本结论视为通过。** 真实跨端链路、平台写入/回读、有数据详情等保留至后续联调阶段。

**结论：`TEST_PASS`（限定于当前 APK `71a041d3…`、源码 `a421d0d`、本报告已执行范围）。**
设备收尾：守护已停止、网关保持未配置、App 保持安装、蓝牙/定位/网络设置未改；未卸载、未清除应用数据；无遗留后台诊断任务。
