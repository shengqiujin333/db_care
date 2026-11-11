# Android 集成调试修复：产品验收 D-01/D-02/D-03/D-04a（FIX-PROD-01）

状态：修复已保存并通过实现侧自检（`FIX_SUBMITTED`）；**待独立产品验收按原受影响项重新安装/复测**。不代表已 `TEST_PASS`，也不代表真实 BLE/平台链路通过。
责任角色：Android 工程（Android 集成调试）
原始失败：`evidence/product_acceptance_20261111/PRODUCT-ACCEPTANCE-001.md`（`TEST_FAIL`，D-01/D-02/D-03/D-04）
失败归因：`evidence/product_acceptance_20261111/SOFTWARE-TRIAGE-001.md`（`REPAIR_REQUIRED`，仅限 Android 对象 D-01/D-02/D-03/D-04a）
分析时对象：被测 APK `d20542219faeaf4be0d73ce4650c57e3ea25bc54183ff0117a3f61c0e41bd8b6`（源码 `28b78b0`，`versionCode 10 / versionName 1.9`）
修复后实现侧构建：APK 10,365,444 B，SHA-256 `9c6427c9fe6dd19a2b5b38263014008ff0fd79782d8bffa3ce338c3f1540fc59`，MD5 `6aee6d94aaceddc8094bd7a6b52e83be`（同版本 10/1.9；重装须按 `-r` 并核对摘要）

---

## 1. 缺陷、根因与修复

| ID | 原始现象 | 根因（源码级） | 修复 |
|---|---|---|---|
| D-01 | 「我的」→「查看今日报告」点击切到「设定」，报告未打开（按钮 bounds 全落在 `nav_view` 内） | `activity_main.xml` 的 `NavHostFragment` 高度为 `match_parent`，ConstraintLayout 下忽略底部约束，占满父内容区并延伸到底部导航之下；`fragment_notifications.xml` 的「查看今日报告」又 `bottom_toBottomOf="parent"` 固定在 fragment 底部，必然落入导航覆盖区 | ① `activity_main.xml` 将 NavHost 高度改为 `0dp`，由 `top_toTopOf=parent` 与 `bottom_toTopOf=nav_view` 共同约束；② 「我的」页改为 `NestedScrollView` 滚动内容，取消所有 `bottom_toBottomOf="parent"` 固定底部锚定；③ 将「查看今日报告」「打开传感器列表」移到页面顶部，按控件中心即可直接触达 |
| D-02 | 「我的」→「打开传感器列表」同样被底部导航拦截，`SensorIdListFragment` 未进入 | 同 D-01（`fragment_notifications.xml:155-177` 底部按钮锚定 parent 底部） | 同 D-01；按钮仍绑定原回调 `navigate(...SensorIdListFragment)` |
| D-03 | 「注册传感器」中心 `(570,1176)` 落在「网关与上传设置」`[32,1168][688,1264]` 内，点击打开网关对话框 | 两个兄弟控件在 ConstraintLayout 中用固定 `layout_marginTop/layout_marginBottom` 分别锚定，未形成互斥约束，导致 bounds 重叠（`fragment_notifications.xml` 旧 119–165 行） | 「我的」页全部控件改为 `LinearLayout(vertical)` 顺序排列；输入框/按钮按行分配，`addsensorid` 与 `gatewayBleButton` 分属不同行且保留 `spacing_lg` 间距；任何可点击控件不再互相覆盖 |
| D-04a | 首页空态文案“前往「我的」→「传感器列表」添加设备”与实际入口名不符 | `fragment_home.xml` 硬编码旧路径；实际控件名为「打开传感器列表」，首页另有「去添加」可达同一列表页 | 文案改为“点击下方「去添加」添加设备；也可在「我的」→「打开传感器列表」查看已保存 ID”，指向实际存在的控件名与可达路径 |

同时按同类问题排查了其余页面：`fragment_dashboard.xml`、`fragment_device_detail.xml`、`fragment_zhuce.xml` 已是滚动容器，`fragment_home.xml` 内容受约束；NavHost 高度修复后，所有页面底部控件均止于底部导航之上。`fragment_sensor_id_list.xml` 的 `bottomBar` 亦随 NavHost 边界上移。未发现其它底部固定控件仍处于导航覆盖区。

## 2. 变更文件

| 文件 | 变更 |
|---|---|
| `dengbei_care/app/src/main/res/layout/activity_main.xml` | NavHost `layout_height="match_parent"` → `0dp`（保留 top/bottom/left/right 约束） |
| `dengbei_care/app/src/main/res/layout/fragment_notifications.xml` | 重排为 `NestedScrollView + LinearLayout(vertical)`；顶部放置「查看今日报告」「打开传感器列表」；恢复/保留全部既有控件 ID（`usernameText`、`registerButton`、`sensorAddressInput`、`macSendButton`、`macScanButton`、`sensoridinfo`、`sensoridinput`、`sensoridscan`、`sensornameinput`、`addsensorid`、`opensensorlistbutton`、`viewReportButton`、`gatewayBleButton`）；无 `bottom_toBottomOf="parent"` |
| `dengbei_care/app/src/main/res/layout/fragment_home.xml` | 空态引导文案改为实际可达入口 |
| `dengbei_care/用户使用说明书.md` | 同步修复后行为：传感器列表可进入、删除步骤、报告入口在「我的」页顶部、页面可滚动；删除“被遮挡/当前不可用/不能滚动”等过期限制；对“有数据时的列出/删除”保持“尚待实机验证”，未写成已实现 |

`NotificationsFragment.kt` 的点击回调、导航目标、`GatewayBleSettingsDialog`、BLE/MQTT/上传/告警/SQLite 逻辑均未改动。

## 3. 兼容面与约束

- 版本保持 `versionCode 10 / versionName 1.9`（同一交付候选的缺陷修复；测试能力需 `adb install -r` 覆盖并核对新摘要）；
- 未改 BLE 布局/AES/topic/`/upload_data`/SQLite v3/告警阈值/去抖/门控；
- 未隐藏或禁用任何控件的既有功能；未改测试期望、门禁脚本、权限或环境配置；
- 未写入/删除设备数据；本能力未安装/操作手机；
- `docs/` 随包手册（D-04b）**不在 Android 实现范围**，未修改：按 triage 由技术文档角色独立修正。需同步的事实（供文档节点使用）：修复后「我的」页顶部「查看今日报告」「打开传感器列表」可直接点击；「打开传感器列表」进入 `SensorIdListFragment`（0 条时显示“已保存的 ID（0）”），存在“点选可多选，删除选中可批量移除”“删除选中”；有数据时的列出/删除行为仍未验证；「注册传感器」位于滚动页中部，点击中心不再打开网关对话框；首页空态文案已改为「去添加」/「打开传感器列表」。

## 4. 实现侧自检（本能力已执行）

| 检查 | 命令/对象 | 结果 |
|---|---|---|
| 干净构建 | `./gradlew clean :app:assembleDebug --offline` | `BUILD SUCCESSFUL`；0 error（11 条既有 warning） |
| 全量宿主机回归 | `./gradlew :app:testDebugUnitTest --offline --rerun-tasks` | **33 套件 / 261 项 / 0 失败 / 0 错误 / 0 跳过**（含新增布局结构回归 `NotificationsLayoutContractTest` 4 项） |
| 布局/文案静态核查 | `python evidence/product_fix_20261111/static_layout_check.py` | **15/15 PASS**：NavHost 0dp + 底部止于 nav；notifications 为滚动容器且无 parent 底部锚定；14 个控件 ID 完整；`addsensorid` 在 `gatewayBleButton` 之前且无重叠结构；两个快速入口在地址区之前；4 个点击回调存在；首页新文案无旧路径；版本未变；底部导航保留 |
| 既有独立静态核查 | `python evidence/software_verify_aa005_android_check.py` | **54/54 PASS，0 FINDING** |
| 既有独立门禁 | `python evidence/aa005_android_check/run_gates.py` | **12/12 EXIT=0** |
| APK 身份/签名 | `aapt dump badging` / `apksigner verify` | `versionCode 10 / versionName 1.9`；v2 验证通过；`Android Debug` 证书（项目既有 debug keystore） |

原始证据：`evidence/product_fix_20261111/`（`build.log`、`unit_test.log`、`static_layout_check.py/.txt`、`static_aa005.log`、`gates.log`、`apk_badging.txt`、`apksigner_verify.txt`）。

## 5. 实机复测方法（交独立产品验收）

安装：`adb -s 12f046690506 install -r <app-debug.apk>`（或 `android_test install`）；核对 `versionCode=10`、`versionName=1.9` 与 APK SHA-256 `9c6427c9…`。安装后直接使用新布局，不需清数据。

| 项 | 步骤 | 期望 |
|---|---|---|
| D-01 | 进入「我的」；对「查看今日报告」控件中心注入一次原始触摸 | 打开 `ReportActivity`（今日报告）；`navigation_dashboard` 不选中、页面不切换到「设定」 |
| D-02 | 进入「我的」；对「打开传感器列表」控件中心触摸 | 进入 `SensorIdListFragment`（显示“已保存的 ID（N）”）；页签不切换 |
| D-03 | 「我的」页向下滚动到「注册传感器」，对其中心触摸（两个输入框留空） | 不出现「网关与上传设置」对话框；空输入无写入/无网络请求；再点「注册传感器」仍为同一结果 |
| D-04a | 首页空态 | 显示“点击下方「去添加」添加设备；也可在「我的」→「打开传感器列表」查看已保存 ID”；点「去添加」进入列表页 |
| 布局 bounds | `ui_dump` 检查「查看今日报告」「打开传感器列表」与 `nav_view` 不相交；`addsensorid` 与 `gatewayBleButton` bounds 不相交 | 均成立（滚动后同样成立） |
| 回归 | 冷启动、首页/设定/我的切换、守护启停、网关对话框打开/关闭、手动重试上传 | 与修复前可用路径一致；无 FATAL/ANR |
| 删除（有数据时） | 无绑定数据时仅验证入口与空态；有数据时的列出/单项删除/批量删除 | 本轮未验证，按原验收边界保持 NOT EXECUTED，不得写成通过 |

## 6. 未执行与边界

1. **未安装/操作手机**：本能力无 `android_test` 入口；实机复测由独立产品验收执行。上述实机期望是修复目标，不是已通过结论。
2. **真实 BLE/GATT/MTU、MQTT、`/upload_data`/401/回读、API 31+、9 台快照、升温/光强/功耗**：保持原验收的 NOT EXECUTED，不因本修复改变。
3. **D-04b 随包手册**：属技术文档角色；本修复仅同步 Android 用户使用说明书（`dengbei_care/用户使用说明书.md`），未改 `docs/`。
4. **“有数据时”的传感器列表列出/删除**：仍无真实绑定数据，保持未验证；修复不宣称该行为通过。
5. 本修复未扩大权限、未访问服务端、未修改版本号；构建产物摘要随构建时间变化，安装验收以包名/版本/签名与行为为准。
