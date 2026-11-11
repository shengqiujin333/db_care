# 软件测试证据 · 产品验收 D-01/D-02/D-03/D-04a 修复后独立复验（android_check）

- 负责角色：软件测试
- 时点：2026-11-11 19:00–19:10（Asia/Shanghai；设备/工具时间戳为 UTC）
- 被测对象：`artifacts/android_implementation.md` 声明的当前 Android 交付；APK `dengbei_care/app/build/outputs/apk/debug/app-debug.apk`
- 上游失败证据：`evidence/product_acceptance_20261111/PRODUCT-ACCEPTANCE-001.md`（TEST_FAIL：D-01/D-02/D-03/D-04a）
- 上游修复：`artifacts/android_fix_product_D01-D04a.md`、提交 `dacf888`、重建证据 `evidence/product_android_rebuild_20261111/`
- 设备：Redmi 8A（`olivelite`），Android 10 / API 29，serial `12f046690506`
- **结论：`TEST_PASS`（限定范围：当前 APK 的真机安装、D-01/D-02/D-03/D-04a 回归、受影响页面与关键路径；不含真实 BLE/平台跨链路）**

## 1. 被测身份与安装

| 项 | 实测 |
|---|---|
| 本地 APK | 10,365,464 B，SHA-256 `71a041d322368c20a181d5716c44ac4d1b2c8a3869926b0047cd96b61e2c10d8`（与重建声明一致） |
| 安装前设备包 | `base.apk` SHA-256 `d20542219faeaf4be0d73ce4650c57e3ea25bc54183ff0117a3f61c0e41bd8b6`（= 上一轮实测缺陷的旧候选） |
| 安装动作 | `android_test install` → `installed_version_verified`；`versionCode=10 / versionName=1.9 / sha256=71a041d3…` |
| 安装后设备包 | `base.apk` SHA-256 `71a041d322368c20a181d5716c44ac4d1b2c8a3869926b0047cd96b61e2c10d8`（与本地候选逐字节一致） |
| 升级语义 | `firstInstallTime=2026-11-10 19:25:25` 保留，`lastUpdateTime=2026-11-11 19:00:36`；数据未被清除（已保存 ID 仍为 0） |
| 签名/清单 | `apkSigningVersion=2`；`minSdk=26 / targetSdk=34`；launcher `MainActivity` |

原始记录：`device_package_pre.txt`、`device_path_pre.txt`、`device_apk_hash_pre.txt`、`device_path_post.txt`、`device_apk_hash_post.txt`、`device_package_post.txt`（均在本目录）。

## 2. 本轮独立真机执行（原始证据见本目录）

| 项 | 操作 | 实际结果 | 原始证据 |
|---|---|---|---|
| D-01 | 「我的」→ 点击「查看今日报告」 | 打开 `ReportActivity`（“早起报告”“尚未生成过报告”“重新生成”），未切换到「设定」 | `ui_report_activity.xml`、`screen_report_activity.png`、`ui_info` |
| D-02 | 「我的」→ 点击「打开传感器列表」 | 进入 `SensorIdListFragment`（“已保存的 ID（0）”“点选可多选…”“删除选中”禁用），页签保持「我的」 | `ui_sensor_id_list_after_fix.xml`、`screen_sensor_list.png` |
| D-03a | 检查按钮 bounds | 「注册传感器」`[32,976][688,1072]` 与「网关与上传设置」`[32,1120][688,1216]` 不相交；两者均在底部导航 `[0,1264][720,1424]` 之上 | `ui_my_scrolled.xml` |
| D-03b | 两输入框为空时点击「注册传感器」中心 | 页面保持原状，未打开「网关与上传设置」对话框；随后列表仍为 0 条；logcat 无上传/HTTP 请求 | `ui_after_addsensor_click.xml`、`screen_addsensor_click.png`、`logcat_full_session.txt` |
| D-04a | 首页空态文案 | 显示“点击下方「去添加」添加设备；也可在「我的」→「打开传感器列表」查看已保存 ID”；旧文案“前往「我的」→「传感器列表」”不存在 | `ui_home_after_install.xml` |
| D-04a | 首页「去添加」 | 进入同一 `SensorIdListFragment`（已保存的 ID（0）） | `ui_home_add_click.xml` |
| 布局回归 | 「我的」页顶部快速入口 | 「查看今日报告」`[32,327][688,423]`、「打开传感器列表」`[32,431][688,527]`，均不与底部导航相交；页面可上下滚动 | `ui_my_after_install.xml`、`ui_my_scrolled.xml` |
| 功能回归 | 网关对话框打开/关闭 | 正常打开，含 BLE 地址输入、搜索、保存/清除、上传状态“无（待发 0 条）”、重试上传；返回键关闭 | `ui_gateway_dialog.xml` |
| 功能回归 | 手动「重试上传」 | 提示已触发，状态保持“云端上传状态：无（待发 0 条）”，无崩溃 | `ui_after_retry_upload.xml` |
| 功能回归 | 守护启停 | 「启动」→ 按钮变「停止」；再点 → 恢复「启动」 | `ui_guard_started.xml`、`ui_guard_stopped.xml` |
| 功能回归 | 冷启动 | `force-stop` 后启动：`MainActivity` 前台，首页正常显示；进程 `17637`，无 FATAL/ANR | `ui_after_cold_start_2.xml`、`logcat_coldstart_and_retry.txt` |
| 页签/设置页 | 首页/设定/我的切换 | 设定页正常渲染（设备选择、参数 2/0.1/35/29/75/40） | `ui_settings_page.xml` |
| 无崩溃 | 全会话 logcat | 无本应用 `FATAL EXCEPTION`、无 `ANR in com.jinyuni.dengbei_care` | `logcat_full_session.txt`、`logcat_coldstart_and_retry.txt` |

结构化复核脚本：`verify_ui_checks.py` → **13/13 PASS，EXIT=0**（`verify_ui_checks.txt`，独立解析上述 UI 树断言按钮 bounds 不相交、报告/列表页文本、注册点击无对话框、首页文案与可达路径、守护与冷启动回归）。

## 3. 复用证据的边界（非本轮独立执行）

宿主机 261 项单测、布局静态核查 15/15、独立静态 54/54、门禁 12/12 由 Android 实现在 `evidence/product_android_rebuild_20261111/` 与 `evidence/product_fix_20261111/` 提供；本轮按其声明复用这些结果，未重新运行，不将其表述为本轮独立复测。本轮独立证据为真机安装字节核对与上表实际交互、UI 树与日志。

## 4. 未执行项（不得写成通过）

| 项 | 状态 | 原因 |
|---|---|---|
| 真实 BLE/GATT/MTU、网关发现与配对 | NOT EXECUTED | 无已配置网关/已绑定传感器；用户明确跨链路属后续联调阶段 |
| MQTT/`/upload_data` 真实写入与回读、401/403 验收 | NOT EXECUTED | 无隔离账号与平台写读授权；本轮仅验证 App 侧状态展示与手动重试入口 |
| 传感器列表“有数据时”的列出/单项删除/批量删除 | NOT EXECUTED | 当前无已绑定数据；未写入用户数据 |
| API 31+ 蓝牙权限分支 | NOT EXECUTED | 设备为 Android 10 / API 29 |
| 9 台真实设备快照与第 10 台拒绝 | NOT EXECUTED | 无真实多设备源 |
| 调温/改变光强/遮光/功耗 | NOT APPLICABLE | 用户明确当前不能调节温度/光强、不能测功耗，不安排相关硬件测试 |
| `docs/` 随包手册 D-04b 修正 | 不在本能力范围 | 由技术文档角色与文档校核能力负责；本轮仅核对 `dengbei_care/用户使用说明书.md` 中与实测相关的入口/滚动/删除说明与行为一致 |
| 固件 `FW-LIGHT-DEF2-ENABLE` 人工跳过项 | 不在本能力范围 | 固件对象；按既有 `SKIP_CONFIRMED` 记录保持 NOT EXECUTED，本报告不重复询问 |

## 5. 判定

当前 APK 与设备安装字节一致；上一轮四项 Android 用户可见缺陷（D-01/D-02/D-03/D-04a）在本轮真机上均不再复现，受影响页面与关键路径（冷启动、页签、设定、网关对话框、上传状态/重试、守护启停）无回归，未发现新的本应用崩溃或 ANR。真实跨端链路与需外部数据的项目保持未执行，不因本结论变为通过。

**结论：`TEST_PASS`（限定于当前 APK 与上表已执行范围）。**
