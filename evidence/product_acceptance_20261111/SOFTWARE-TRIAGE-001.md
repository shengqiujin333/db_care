# 产品验收失败软件归因（SOFTWARE-TRIAGE-001）

- 负责角色：软件测试
- 能力：软件失败分析（`product_android_triage`）
- 分析对象：Android 候选实现（`artifacts/android_implementation.md`，被测 APK `d2054221…`）
- 原始失败：`evidence/product_acceptance_20261111/PRODUCT-ACCEPTANCE-001.md` **TEST_FAIL**（`failure_cause=MEASURED_DEFECT`）
- 代码索引：`artifacts/codegraph_android_index.md`（projectPath `D:\mypro\beiwo2\AIP\dengbei_care`；主源码基线 `e6fc602`）
- 分析时点：2026-11-11 18:26–18:32（Asia/Shanghai；设备/工具时间为 UTC）
- 分析时 HEAD：`c890527`，工作区干净
- **本轮结论：`REPAIR_REQUIRED`（仅限 Android 实现的对象 D-01/D-02/D-03/D-04a）**

> 本文件只做失败归因与责任边界判断，不是复验通过、不是修复、不代表产品已合格。
> 本轮未修改产品源码、测试期望、固件、文档、权限或环境配置；未安装/卸载 App、未清数据、未写入平台。
> 报告中“已实测”均指本轮在当前候选 APK 上重新执行并留证；“未执行”不等于通过，也不被写成缺陷。

---

## 1. 结论摘要

1. **原始失败中的 Android 用户可见缺陷被本轮独立复现**，且不是测试工具或点击位置造成的假象：
   - 对「查看今日报告」按控件中心坐标 `(360,1328)` 注入一次原始触摸，页面从「我的」切到「设定」（`navigation_dashboard selected=true`），`ReportActivity` 未打开。
   - 对「打开传感器列表」按控件中心点击（选择器 `text=打开传感器列表`），同样切到「设定」，`SensorIdListFragment` 未进入。
   - 这两个按钮的 bounds 分别 `[32,1280][688,1376]`、`[164,1296][456,1392]`，完整落在底部导航容器 `[0,1264][720,1424]` 内；导航容器绘制顺序在后，触摸被其消费。
2. **D-03 从“几何重叠”升级为“已实测的功能阻断”**：点「注册传感器」的控件中心（`570,1176`，落在与「网关与上传设置」的重叠区）后，实际打开的是「网关与上传设置」对话框，不是注册流程。即该按钮中心不可用。
3. **D-04 的两个责任面分开**：
   - **App 文案（Android 实现）**：首页硬编码“前往「我的」→「传感器列表」添加设备”，而「我的」页实际按钮名为「打开传感器列表」，且该入口当前被拦截。
   - **随包手册（技术文档）**：文档称“当前无 UI 路径查看/删除已绑定传感器”。本轮实测「首页 → 去添加」可进入同一 `SensorIdListFragment`（“已保存的 ID（0）”，含“删除选中”控件）；该表述不准确。有数据时的删除行为仍未验证。
4. **D-01/D-02 的机制在源码中可定位**：`activity_main.xml` 的 `NavHostFragment` 使用 `layout_height="match_parent"`，实机 bounds 显示其占满父内容区 `[0,183][720,1424]`，底部约束未使其止于底部导航顶部；`fragment_notifications.xml` 的底部按钮又锚定到该 fragment 的 `parent` 底部，因此必然落入底部导航覆盖区。App 无任何 WindowInsets/bottom padding 处理。
5. **不是外部条件缺失**：上述缺陷无需真实 BLE/服务器/调温/调光/功耗条件，在当前候选 APK 与已登记手机上可确定性复现。真实链路、升温/光强双态、跨链路、`/upload_data`、功耗等未执行项另有责任边界，**不是**本次 Android 返工的理由，也不被写成通过。
6. **需求 4 的无光分支（固件 `T=0`）不在本次 Android 返工范围**：该项已被人工确认 `SKIP_CONFIRMED`（`FW-LIGHT-DEF2-ENABLE`，2026-11-11T03:50:42Z），原因缺“判据形制/数值接受记录与同板同构建双态证据”。这是上游判据接受/固件域的待决事项，本 triage 不把它升级为固件缺陷或 Android 修复要求。
7. **结果判断：`REPAIR_REQUIRED`**。修复对象为 Android 实现的布局与文案（`activity_main.xml`、`fragment_notifications.xml`、`fragment_home.xml` 及其相关页面组织）；修复后须重新安装新 APK 并回到原受影响测试（产品整体功能验收）复验。随包手册的事实错误由技术文档角色另行修正。

---

## 2. 对象身份、版本与证据来源

| 项 | 值 | 本轮核对方式 |
|---|---|---|
| 分析对象 | Android 实现记录 `artifacts/android_implementation.md`（AA-005 七项增量 + D-AA005-1 修复 + 重建） | 阅读 |
| 被测包名/版本 | `com.jinyuni.dengbei_care`，`versionCode=10`，`versionName=1.9`，debug | 设备 `dumpsys package`（本轮） |
| 主机候选 APK | `dengbei_care/app/build/outputs/apk/debug/app-debug.apk`，10,365,460 B，SHA-256 `d20542219faeaf4be0d73ce4650c57e3ea25bc54183ff0117a3f61c0e41bd8b6` | 本机 `sha256sum`（本轮） |
| 设备在装 APK | `/data/app/com.jinyuni.dengbei_care-O7DEsc1_5VmHNxf00zV_3w==/base.apk`，SHA-256 **同上逐字节一致** | 设备端 `sha256sum`（本轮） |
| 候选源码提交 | Git `28b78b0`（`artifacts/android_build.md` 声明）；当前 HEAD `c890527` | `git diff --stat 28b78b0 HEAD -- dengbei_care` 仅用户说明书变更；主源码 `app/src/main` 无差异 |
| 代码索引基线 | `e6fc602`；`git diff e6fc602 28b78b0 -- dengbei_care/app/src/main` 为空 | 当前主源码与索引基线一致；布局/文案/导航源码均在索引覆盖内 |
| 设备 | Redmi 8A（`olivelite`），Android 10 / API 29，serial `12f046690506`，720×1520 | `android_test devices` / `ui_info`（本轮） |
| App 进程 | `MainActivity`，pid `14933`（与原始验收会话同一运行实例） | `ui_info`（本轮） |
| 崩溃缓冲 | 空（`logcat -d -b crash -t 40` 无输出） | 本轮 |
| 原始失败证据 | `PRODUCT-ACCEPTANCE-001.md`（§3.2/§4/§5）及其 UI dump/截图 | 阅读并逐项复核 |

**版本一致性说明**：主机 APK、设备 `base.apk`、候选源码树三者闭合；本轮所有复现均发生在该候选上，未替换包、未降级、未清数据。

---

## 3. 原始失败与本轮复现

### 3.1 原始失败引用（不重述全部）

- **D-01**「查看今日报告」被底部导航拦截：按钮 bounds 完整落在导航之下，点击后切到「设定」。
- **D-02**「打开传感器列表」同样被拦截；另实测「首页 → 去添加」可进入 `SensorIdListFragment`。
- **D-03**「网关与上传设置」与「注册传感器」/输入区几何重叠；原文注明重叠区点击命中顺序未逐点验证。
- **D-04** 首页引导文案与实际路径不一致；文档“当前无 UI 路径”表述有误。
- 总体判定 `TEST_FAIL`，并列出 R4 未启用、真实触发/跨链路/平台写入/功耗未执行等项。

### 3.2 本轮独立复现（第一手）

环境：同一设备、同一在装 APK（哈希见 §2）、同一 App 进程。所有页面切换后均以 `ui_dump` 留证。

| # | UTC 时间 | 动作 | 实际结果 | 原始证据（本目录 `software_triage_20261111/`） |
|---|---|---|---|---|
| 1 | 10:26:22–10:26:39 | `devices` → `ui_info` → `ui_dump` | 设备在线；App 前台；当前为「首页」 | 工具输出 + `ui_info` |
| 2 | 10:26:52 | 点底部「我的」 | 进入「我的」页 | — |
| 3 | 10:27:02 | `ui_dump` | 「查看今日报告」`[32,1280][688,1376]`；「打开传感器列表」`[164,1296][456,1392]`；「网关与上传设置」`[32,1168][688,1264]`；「注册传感器」`[452,1128][688,1224]`；底部导航 `[0,1264][720,1424]`；`nav_host_fragment_activity_main` `[0,183][720,1424]` | `ui_my_page.xml`、`screenshot_my_page.png` |
| 4 | 10:27:13 | `ui_click` 选择器 `text=查看今日报告` | 切到「设定」；`navigation_dashboard selected=true`；未进入报告 | `ui_after_view_report_click.xml` |
| 5 | 10:27:57 | 先回「我的」，再 `ui_click` 选择器 `text=打开传感器列表` | 切到「设定」；未进入传感器列表 | `ui_after_open_sensor_list_click.xml` |
| 6 | 10:28:27 | 首页点「去添加」 | 进入“已保存的 ID（0）”，含“点选可多选，删除选中可批量移除”与（禁用）“删除选中” | `ui_via_home_add_route.xml` |
| 7 | 10:29:31 | 回「我的」，`ui_click` 选择器 `text=注册传感器` | **打开「网关与上传设置」对话框**（`android:id/alertTitle`、网关 BLE 地址输入、搜索附近网关、重试上传） | `ui_after_register_sensor_click.xml` |
| 8 | 10:32:37 | `tap x=360 y=1328`（「查看今日报告」bounds 中心） | 切到「设定」；`navigation_dashboard selected=true` | `ui_after_raw_tap_report_center.xml`、`raw_tap_360_1328.txt` |
| 9 | 10:30:03 | `ui_press back` | 关闭上述对话框，返回「我的」 | — |

**复现结论**：

- `ui_click` 选择器命中的确实是目标控件（选择器文本唯一），但触摸落点被覆盖层消费；第 8 步用**原始坐标触摸**再次得到同一结果，排除了选择器/自动化库差异。
- 「注册传感器」点击中心（`570,1176`）落在「网关与上传设置」`[32,1168][688,1264]` 内，实际打开后者；这不是“可能的风险”，而是已实测的功能阻断。
- 全过程无崩溃（crash buffer 为空），页面切换稳定、可重复。

---

## 4. 缺陷机制与源码关联

### 4.1 D-01/D-02：底部按钮被底部导航覆盖（Android 布局）

**实测几何**（720×1520，density=2.0）：

```text
父内容区 container                [0,55][720,1424]
nav_host_fragment_activity_main   [0,183][720,1424]   ← 占满父内容区（含底部导航区域）
nav_view                          [0,1264][720,1424]  ← 绘制顺序靠后，覆盖其上
viewReportButton                  [32,1280][688,1376] ← 完全在 nav_view 内
opensensorlistbutton              [164,1296][456,1392]← 完全在 nav_view 内
```

**源码关联**：

| 文件 | 事实 | 作用 |
|---|---|---|
| `dengbei_care/app/src/main/res/layout/activity_main.xml:28-38` | `NavHostFragment` 使用 `android:layout_height="match_parent"`，同时声明 `layout_constraintBottom_toTopOf="@id/nav_view"` | 实测 fragment 高度解析为父内容区全高，底部约束未使其止于 `nav_view` 顶部 |
| 同上 `:16-26` | `BottomNavigationView` 约束到 parent 底部，高度 80dp | 覆盖 fragment 底部区域并优先接收触摸 |
| `dengbei_care/app/src/main/res/layout/fragment_notifications.xml:155-177` | `viewReportButton` 约束 `bottom_toBottomOf="parent"`；`gatewayBleButton` 约束在其上方 | 两个按钮被固定在 fragment 底部，必然落入 nav_view 覆盖区 |
| `NotificationsFragment.kt:312-323` | 两个按钮的点击回调本身存在（导航到 `SensorIdListFragment` / 启动 `ReportActivity`） | 回调正常，故障不在逻辑层，而在布局可达性 |
| 全工程 | 未发现 `WindowInsets`/`fitsSystemWindows`/底部 padding 处理（限定目录检索 0 命中） | 无补偿机制 |

**影响**：同一页面底部两个标注入口在声明的设备上不可用；由于 `match_parent` 的高度语义与屏幕高度无关，只要底部导航覆盖底部 80dp，这两个按钮在任何屏幕上都会落在覆盖区（页面顶部内容是否溢出另论）。「查看今日报告」是否还有其它入口（如通知）本轮未验证。

### 4.2 D-03：同级控件重叠，注册按钮中心被网关按钮覆盖（Android 布局）

**实测 bounds**：

```text
sensornameinput    [33,1104][533,1224]
addsensorid        [452,1128][688,1224]   （注册传感器，drawing-order=10）
gatewayBleButton   [32,1168][688,1264]    （网关与上传设置，drawing-order=13）
```

- 重叠区 `x=452..688, y=1168..1224`（236×56 px）；`addsensorid` 中心 `(570,1176)` 位于重叠区内。
- 第 7 步实测：点击 `addsensorid` 中心打开的是「网关与上传设置」对话框。
- 源码关联：`fragment_notifications.xml:119-165` 中两个按钮分别用固定 `layout_marginTop`/`layout_marginBottom` 锚定，没有互斥约束或滚动容器，导致兄弟控件重叠。

**影响**：用户按「注册传感器」中心点击会得到网关设置对话框；注册流程仅在其未被覆盖的边缘区域可触发（本轮未逐点测量边缘可用面积）。这是对原始 D-03 的加强证据，不是新对象。

### 4.3 D-04a：首页引导文案与实际入口名不一致（Android 文案）

**源码**：`dengbei_care/app/src/main/res/layout/fragment_home.xml:89-96` 为硬编码文本：

> 前往「我的」→「传感器列表」添加设备

**实际**：底部页签为「首页 / 设定 / 我的」；「我的」页上的按钮文案为「打开传感器列表」（`strings.xml:52`），并不存在名为「传感器列表」的页签或按钮。且该按钮当前被 D-02 拦截，而首页下方的「去添加」按钮才可达同一列表页。

**影响**：用户按首页指引找不到标注入口；即使 D-02 修复后，文案也应指向实际控件名或「去添加」。属 Android 实现的可修改文案。

### 4.4 D-04b：随包手册“无 UI 路径”表述不准确（文档域）

**文档原文**：`docs/04_安卓App安装与使用.md:173/225/239`、`docs/06_维护排障与已知限制.md:93`、`docs/README.md:78` 称“当前无 UI 路径查看/删除已绑定传感器”。

**实测**：首页 →「去添加」→ `SensorIdListFragment`，页面含“已保存的 ID（0）”“点选可多选，删除选中可批量移除”“删除选中”（当前禁用）。该路径在原始验收与本轮均成立。

**边界**：无已绑定数据，未能验证“有数据时列出/删除”的行为；文档应表述为“标注入口被拦截；首页「去添加」可进入列表页；有数据时行为未验证”，不得把未实现行为写成已实现。

**责任**：技术文档角色。该问题不是 Android 源码缺陷，但修复文档时须与 D-01/D-02 修复后的最终行为保持一致。

---

## 5. 非本次修复对象的失败/未执行项

| 项 | 状态 | 证据与责任边界 |
|---|---|---|
| R4 无光分支未启用（固件 `LIGHT_DARK_THRESHOLD_CODE=0`，实板 `V=0 D=0`） | **未执行（人工 `SKIP_CONFIRMED`）**，不是本轮新测缺陷 | `FW-LIGHT-DEF2-ENABLE`：2026-11-11T03:50:42Z 回复“无法准备，先跳过”，原因缺判据形制/数值接受记录与同板同构建双态证据。责任属固件/需求上游；本 triage 不据此要求固件返工，也不写成通过 |
| 真实 ΔT>0.9 ℃、>35.0 ℃ 触发与 433 实发 | **NOT EXECUTED** | 用户明确当前不能调温；不得安排调温测试。责任：后续具备条件时的测试/联调 |
| 433→网关→BLE→手机跨链路与同笔一致性 | **NOT EXECUTED** | 用户安排后续联调阶段；网关未接线核验、手机未绑定。责任：后续联调 |
| `/upload_data` 真实写入、幂等与回读 | **NOT EXECUTED** | 当前能力无平台写/数据库授权。责任：后端/联调按授权执行 |
| CR2032 功耗 | **已批准延期（NOT EXECUTED）** | 无仪器；用户明确不能测功耗。责任：硬件测试 |
| 报告的其他入口（通知等） | **未验证** | 不影响 D-01 的成立；修复后应一并确认是否存在替代入口 |
| 传感器列表“有数据时的列出/删除” | **未验证** | 当前 0 条绑定；本 triage 不据此判定列表功能通过或失败 |

> 上述条目均**不是** Android 返工的理由，也**不是**“证据排除”；它们保持各自的未执行状态。整体产品验收仍不能因 Android 修复而自动通过。

---

## 6. 竞争假设与排除

| # | 假设 | 判断 | 依据 |
|---|---|---|---|
| H1 | 测试工具点错位置/选择器串到导航项，按钮本身可用 | **不成立** | 选择器文本唯一且命中目标控件；原始坐标触摸（360,1328 为按钮 bounds 中心）得到相同结果；实测 bounds 完整落在导航内 |
| H2 | 按钮被有意隐藏或禁用，属设计 | **不成立** | 两个按钮 `clickable=true`、`enabled=true`、`visible-to-user=true`；源码有明确点击回调；文档与首页文案把它们描述为可用入口 |
| H3 | Android 10/该机型/分辨率特例 | **不成立（不足以解释）** | fragment `match_parent` 占满父内容区，底部按钮锚定其底部；与屏幕高度无关，声明设备上确定性复现；App 无 inset 补偿 |
| H4 | 设备上装的是旧包，与源码不一致 | **不成立** | 设备 `base.apk` SHA-256 = 主机候选 = `d2054221…`；候选源码 `28b78b0`，当前 HEAD 主源码无差异 |
| H5 | 页面切换是短暂动画/竞态，稍后会回到报告 | **不成立** | 点击后 dump 稳定显示 `navigation_dashboard selected=true` 与设定页内容；报告 Activity 未曾出现在前台（`ui_info` 仍为 `MainActivity`） |
| H6 | 「注册传感器」中心被覆盖只是几何风险，用户可从其它位置点到 | **部分成立但不免责** | 中心已实测打开网关对话框；按钮仍有边缘可点区域，但按常规点击中心即可触发错误功能，属可用性缺陷 |
| H7 | 文档“无 UI 路径”正确，因为「去添加」只是空态提示 | **不成立** | 「去添加」是按钮，点击后进入 `SensorIdListFragment`；本轮与原始验收均复现。反面边界：有数据时的删除未验证 |
| H8 | 这是固件 `T=0` 或外部条件缺失导致的整体失败，Android 不需修 | **不成立** | Android 缺陷独立于固件/外部条件可复现；R4 与未执行项另有责任边界（§5），不能吞并已实测的 App 缺陷 |

---

## 7. 责任边界与结果判断

| 对象 | 判断 |
|---|---|
| Android 实现 | **需要修复**：D-01/D-02（底部控件可达性）、D-03（控件重叠）、D-04a（首页文案）。均为当前候选上可实测、可修改的产品源码/资源 |
| 技术文档 | **需要独立修正**：D-04b 的手册事实错误与跨文档一致性；不得把未实现行为写成已实现。不是 Android 返工的一部分 |
| 固件/需求 | R4 无光分支 `T=0` 属上游判据接受与固件域；当前为人工 `SKIP_CONFIRMED`，**不作为本轮返工或通过结论** |
| 外部条件 | 本 triage 的缺陷**不依赖**外部设备/网络/温度/光强/功耗；无需等待外部条件 |
| 责任是否未定 | **否**。对象、版本、机制、影响与复验目标均已定位 |
| 结果码 | **`REPAIR_REQUIRED`** |
| 不等于 | 不等于已修复、不等于 `TEST_PASS`、不等于整体产品验收通过；原始 `TEST_FAIL` 仍然有效 |

---

## 8. 修复边界建议（交 Android 实现；本能力不执行）

### 8.1 必须满足的行为

1. **底部控件可达**：在「我的」页，`viewReportButton` 与 `opensensorlistbutton` 的可见区域与可点击区域必须完整位于 `nav_view` 之上；按控件中心触摸应触发各自原回调（打开报告 / 进入传感器列表），而不是切换页签。
2. **无重叠**：同一页面内所有可点击控件（含 `addsensorid`、`sensornameinput`、`gatewayBleButton`、`viewReportButton`、`opensensorlistbutton`）的 bounds 不得互相覆盖；「注册传感器」中心点击必须进入注册流程（空输入时保持无副作用），不得打开网关对话框。
3. **页面可容纳**：`fragment_notifications` 内容已高于可见区域（底部控件位于 y≈1392，nav 顶部在 y=1264），应通过滚动容器/重新排布/调整间距等方式保证全部控件可到达；仅把 `match_parent` 改为 `0dp` 仍会与既有内容重叠（按当前固定 margin 推算）。
4. **文案一致**：首页引导文案必须指向实际存在的控件名或实际可达路径（如「我的」→「打开传感器列表」，或直接指向「去添加」），并在 D-02 修复后仍成立。
5. **不改功能语义**：不得改变 BLE/MQTT、上传、告警、SQLite、阈值、去抖、门控、版本与既有测试期望；不得通过隐藏按钮、禁用按钮或改测试来“通过”。
6. **兼容性**：修复应考虑不同屏高与系统栏/底部导航高度；不得只在 720×1520 上写死坐标。

### 8.2 最小可复核判据（修复后）

- `ui_dump` 中 `viewReportButton`/`opensensorlistbutton` 的 bounds 与 `nav_view` 不相交。
- 对两个按钮中心各注入一次原始触摸：分别打开 `ReportActivity` / 进入 `SensorIdListFragment`。
- `addsensorid` 与 `gatewayBleButton` 的 bounds 不相交；点击前者中心不出现「网关与上传设置」对话框。
- 首页文案与实际控件名一致；`file_manifest.txt` 同步；全量宿主机既有测试保持通过（仅允许与布局/文案直接相关的测试资产同步，不得降低断言）。

---

## 9. 复验目标

修复提交并重新构建/安装后，回到**原受影响测试**（产品整体功能验收）复验：

| 复验项 | 方法 | 期望 |
|---|---|---|
| D-01 | 「我的」→ 按「查看今日报告」中心触摸 | 打开今日报告；页签不切换 |
| D-02 | 「我的」→ 按「打开传感器列表」中心触摸 | 进入 `SensorIdListFragment`；页签不切换 |
| D-03 | 「我的」→ 按「注册传感器」中心触摸（空输入） | 无网关对话框；无副作用（空输入不写入） |
| D-04a | 首页空态文案 | 指向实际可达入口；与「我的」页控件名一致 |
| 回归 | 冷启动、首页、设定、我的、网关对话框、守护启停 | 与修复前既有可用路径一致；无 FATAL/ANR |
| 文档（独立） | 随包手册相关章节 | 与实际路径一致；不改写未验证行为 |
| 未执行项 | §5 列表 | 保持 `NOT EXECUTED`；不得因本次修复写成通过 |

**判定**：只有重新安装修复后的 APK 并在原受影响测试中看到上述行为、且既有通过范围无回归，才能把 D-01/D-02/D-03/D-04a 视为修复。分析完成、代码看起来合理或仅宿主机单测通过都不算。

---

## 10. 未执行项与证据局限

1. 本轮未重新安装 App；使用设备上已在装、且哈希与候选一致的包（§2）。
2. 未验证「查看今日报告」的通知等替代入口。
3. 未验证传感器列表在**有绑定数据**时的列出/单项删除/批量删除行为。
4. 未执行真实 BLE、GATT/MTU、MQTT、`/upload_data`、401/403、平台回读，因此不对服务/协议层作任何通过或缺陷结论。
5. 未调温、未调光、未遮光、未测功耗；R4 与相关触发保持 `NOT EXECUTED`。
6. 未修改产品源码、测试期望、固件、文档、权限或环境配置；仅新增本归因与复现证据。
7. 截图与 UI dump 只证明当前候选、当前设备与当前时刻；修复换代后必须重新采集。
8. 本轮设备操作仅为页面导航、打开并关闭一个只读对话框；未安装/卸载/清数据/改系统设置/写平台。

---

## 11. 本轮实际改动

- 新增：`evidence/product_acceptance_20261111/SOFTWARE-TRIAGE-001.md`（本文件）。
- 新增证据目录：`evidence/product_acceptance_20261111/software_triage_20261111/`
  - `ui_my_page.xml`（我的页 bounds）
  - `ui_after_view_report_click.xml`（选择器点击「查看今日报告」后）
  - `ui_after_open_sensor_list_click.xml`（选择器点击「打开传感器列表」后）
  - `ui_after_raw_tap_report_center.xml`、`raw_tap_360_1328.txt`（原始坐标触摸按钮中心后）
  - `ui_via_home_add_route.xml`（首页「去添加」可达列表页）
  - `ui_after_register_sensor_click.xml`（点击「注册传感器」中心打开网关对话框）
  - `screenshot_my_page.png`（我的页真机截图，可见重叠与底部按钮被导航遮挡）
  - `device_package_dump_triage.txt`（versionCode 10 / versionName 1.9）
  - `device_crash_buffer.txt`（空）
- 更新：`file_manifest.txt`（登记上述新增文件）。
- **未修改**：`dengbei_care/**` 产品源码与资源、`app/src/test/**`、固件、文档、`runtime/permissions.yaml`、`runtime/environment.yaml`。
