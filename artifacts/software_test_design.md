# 软件测试设计（TD-SW-005）

状态：软件测试设计产物（供实现、独立验证与后续测试执行消费）。`DESIGN_READY` 只表示本设计可执行，**不代表任何用例已执行/通过，也不代表产品验收通过**。
版本：5.0（替代 TD-SW-004 v4.0；v4.0 全文保留在 Git 历史 `a95aec6^` 及更早提交中，本版只定义当前阶段）
对象：**Android App（`dengbei_care`，Kotlin）当前候选的 3 项行为修复**——`artifacts/android_tasks.yaml` 的 ITEM-001（N-1 扫描权限结果交付与自动续跑）、ITEM-002（N-2 BLE 读取终态链路状态）、ITEM-003（N-3 报告页“重新生成”可感知反馈），以及受影响面的回归。
范围：测试设计（准备、刺激、观测、判据、清理、证据、退出规则）。**不执行**本轮 3 项增量的功能测试，**不修改产品源码**，**不做固件/网关/服务器/硬件改动**，**不安排调温、改变光强、遮光或功耗测试**。
基线：Git `main`，HEAD `cb423d59f71461f64be3e5548588d0f1e07b2c0b`（2026-11-13T12:01:49+08:00）；接手时 `git status --porcelain` 为空。
检查时点：2026-11-13（Asia/Shanghai）。

上游输入（本版实际读取；摘取与本设计直接相关的内容）：

| 输入 | 内容/用途 | SHA-256（本地文件，仅标识内容） |
|---|---|---|
| `artifacts/android_tasks.yaml` | ITEM-001/002/003 的目标、验收条件、设备依赖、兼容面 | `1daa5badc6898a8676ef013126c164ca8df81f0bd184da43549d4c77b6ac47a6` |
| `artifacts/android_architecture.md` | AA-006 v5.0：N-1/N-2/N-3 机制、状态机、D-35…D-40 | `f4c47ed04197614259db73f62ef5efd27703b35dbb6ed7e0b6fd561fce0a2640` |
| `artifacts/interface_contract.md` | IC-002 v4.0：BLE 布局、160 B/9 设备、上报规则、上传字段（兼容面） | `9eeda5acc435559d9fdac3c96bd0c72ca9d5f4a9efd1e2e3a81dded6cab10171` |
| `readme.txt` | 原始需求 1–8、补充条件（当前有光、不调温/不改光/不测功耗、跨链路后续联调） | `d9e18992097814d13fbf1cc4f184de9f9d020dea1752fd889a7254559d33bac4` |
| `evidence/product_acceptance_20261112_scope_recheck/PRODUCT-ACCEPTANCE-RECHECK-002.md` | 当前产品级 `TEST_FAIL`、观察项 O-2、人工范围决定与重评 | 引用，不重跑 |
| `evidence/product_acceptance_20261112_scope_recheck/SOFTWARE-TRIAGE-003.md` | Android 域失败归因、原检查者范围错误复核 | 引用，不改写 |
| `artifacts/failure_resolution_product_stage_delivery_gate.md` | 阶段/交付门禁仍未对齐（产品级） | 引用，不归本能力 |
| 当前 Android 源码 | `NotificationsFragment`、`GatewayBleSettingsDialog`、`MqtttService`、`GatewayFrameCodec`、`GatewayReadGuard`、`BlePermissionPolicy`、`ReportActivity`、`DailyReportWorker`、`ReportScheduler`、`GatewayDiscovery` | 见 §1.1 |

---

## 0. 结论摘要

1. **本版只覆盖 3 项可独立验收的 Android 行为修复**（ITEM-001/002/003）。三项均有：纯逻辑/静态的宿主机断言面、真机（Redmi 8A / API 29）上可构造的刺激与观察、明确的失败判据和证据格式。
2. **人工范围决定已核对，跳项不作为返工依据**：`FW-LIGHT-DEF2-ENABLE` 为人工 `APPROVED_DEFERRED` / NOT EXECUTED（`HR-8A11935AD0A7`，2026-11-11T03:50:42Z，「无法准备，先跳过」）。本设计**不重测、不重复询问、不列为返工依据、不纳入本阶段退出**；它属固件/需求判据接受事项，不是 Android 任务。
3. **原检查者范围错误已重评**：`PRODUCT-ACCEPTANCE-002` §5 R4 把“无光判据未启用（人工延期）”标为“未满足/未执行”，并作为整体 `TEST_FAIL` 依据之一，与其 §6 N-1 的 `SKIP_CONFIRMED` 自相矛盾。按 `PRODUCT-ACCEPTANCE-RECHECK-002` §2 与 `SOFTWARE-TRIAGE-003` §2 的纠正：**上报公式实现 = 已实现且静态一致；无光判据启用 = NOT EXECUTED（人工延期）；真实升温/超温触发 = NOT EXECUTED（温度条件）**。本设计不把人工延期项或真实温度触发列为 Android 验收前提。
4. **BOT-01 已闭合，不重复**：人工 `REASSESS` 后由嵌入式测试以“端口先开 + pyocd 通用 `cortex_m` HW reset”自动执行并 PASS；属固件/嵌入式阶段，不在本设计内。
5. **当前产品级 `TEST_FAIL` 的性质不变**：依据为真实升温/超温、跨链路、平台 `/upload_data` 写入/回读、功耗、Android 真实 BLE/上传/注册/列表/API 31+ 等**外部条件或后续阶段项未执行**，不是本轮 3 项 Android 修复的缺陷。本设计不把这些项塞进 Android 修复的退出条件，也不把阶段结果写成产品通过。
6. **阶段判定口径**：本设计覆盖的 3 项任务全部必需用例 PASS，只能得出“本轮 Android 增量阶段通过（限定范围）”；**阶段通过 ≠ 完整产品通过**。真实跨链路、平台写入/回读、真实温度触发、光强双态、功耗、API 31+、9 台真实快照继续保持 NOT EXECUTED 并在报告中披露。
7. **当前环境事实（本版实测，只读）**：宿主机测试运行器可用（`33` 套件 / `261` 项 / `0` 失败 / `0` 跳过；本轮 `--offline` 复跑为 UP-TO-DATE，`BUILD SUCCESSFUL`）；`assembleDebug` 成功（UP-TO-DATE）；当前 APK 10,365,464 B / SHA-256 `71a041d3…`（修复前基线 AB-002）；Redmi 8A/API 29 在线、在装 versionCode 10 / versionName 1.9、蓝牙开、定位开、无默认网络；真实网关本轮未核验；`android_app_testing` 对**本设计能力**不可用（缺 `android_test`），因此真机执行由后续测试执行能力承接（§8）。

---

## 1. 版本身份、as-built 基线与环境事实

### 1.1 被测对象与源码身份

| 项 | 事实 | 来源 |
|---|---|---|
| 仓库/分支 | `D:\mypro\beiwo2\AIP`，`main`，HEAD `cb423d5`，工作区干净 | `git status --porcelain`（空） |
| Android 主源码 | `git diff db53202..HEAD -- dengbei_care/app/src/main dengbei_care/app/build.gradle.kts` **为空** | 本轮复核 |
| 版本常量 | `versionCode 10` / `versionName "1.9"`；`minSdk 26` / `targetSdk 34` / `compileSdk 34` | `app/build.gradle.kts` |
| 当前候选 APK（修复前基线） | 10,365,464 B；SHA-256 `71a041d322368c20a181d5716c44ac4d1b2c8a3869926b0047cd96b61e2c10d8`；MD5 `1e17e0c281715fddd26d308246d6424c` | 本轮 `stat`/SHA-256/MD5 复算；与 AB-002 一致 |
| 在装 App | `com.jinyuni.dengbei_care`，versionCode 10 / versionName 1.9，DEBUGGABLE；首次安装 2026-11-10 19:25:25，末次更新 2026-11-11 19:18:26 | ADB `dumpsys package`（只读） |
| 新代码目标（尚未实现） | `protocol/GatewayReadState.kt`、`report/ReportRegenerateFeedback.kt` 当前均不存在；`NotificationsFragment` 仍经 `ActivityCompat.requestPermissions(requireActivity(), …)`；`MqtttService.onCharacteristicRead` 的 GATT 失败分支与多数 `Rejected` 分支未设置终态；`ReportActivity` 未提供无数据/失败的可见反馈 | `Glob`/`Grep`/`Read`（限定目录） |
| 历史测试设计 | TD-SW-004 v4.0（AA-005 G-1…G-7）保留在 Git 历史；**本版不据其新增项裁剪当前范围**，仅沿用其证据类别与“不得豁免”原则 | Git 历史 |

> 修复实现后的“测试对象”必须是**新构建的新 APK**，不是上表 AB-002。执行时必须重新记录提交号、构建命令、APK 大小/SHA-256、设备侧安装字节摘要；不得用旧候选的 261/0 结果代替新候选的运行结果。

### 1.2 本轮环境实测（只读、有限等待；不是测试通过结论）

| 项 | 实测 | 证明边界 |
|---|---|---|
| 宿主机测试运行器 | `dengbei_care/gradlew --offline :app:testDebugUnitTest :app:assembleDebug --console=plain` → `BUILD SUCCESSFUL in 14s`，退出码 0 | 运行器与构建入口可用；本轮为 UP-TO-DATE，不证明新增用例 |
| 宿主机既有结果 | `app/build/test-results/testDebugUnitTest/*.xml`：**33 套件 / 261 项 / 0 失败 / 0 错误 / 0 跳过**（结果文件最新时间 2026-11-11 19:17:27） | 修复前基线；实现后必须重跑并记录当轮结果 |
| 构建产物 | `app/build/outputs/apk/debug/app-debug.apk` = 10,365,464 B，摘要见 §1.1 | 修复前基线；不代表新构建 |
| Android 设备 | `12f046690506` 在线；`Redmi 8A`（`olivelite`）；Android 10 / API 29 | 仅身份与在线 |
| 设备无线状态 | `bluetooth_on=1`（开）、`location_mode=3`（定位开）、`Active default network: none` | 时刻状态；执行轮开始/结束都要重记并恢复 |
| 设备工具 | `android_app_testing` 对当前设计能力 **unavailable_for_current_capability**（缺设备工具 `android_test`）；`android_readonly_inspection` 可用（本轮只读查询成功） | 本能力不能安装/启动/点按/改设置；真机执行需后续能力 |
| 真实网关 | 既有 433→BLE 网关；本轮未上电核验、无身份/配对证据 | 不证明在线、广播、连接 |
| 代码索引 | `dengbei_care/.codegraph` 存在；本会话未挂载 `codegraph_explore` MCP | 使用限定目录 `Glob`/`Grep`/`Read` 定位，未伪造图结果 |

> 本设计**未**安装/卸载 App、未启动、未点按、未改蓝牙/定位、未连接网关、未烧录/复位、未访问平台写入接口或数据库；只做了身份、版本、源码、宿主运行器与构建的只读核对。

### 1.3 证据类别与证明边界

| 代码 | 类别 | 能证明 | 不能证明 |
|---|---|---|---|
| H | 宿主机 JVM 纯逻辑测试（实现侧 + 测试侧独立验证） | 映射/判据/边界/状态机 | 真机权限对话、系统蓝牙/定位、WorkManager 实际调度、真实 BLE |
| S | 静态核查/构建 | 接线存在性、禁区（无 Activity 级请求）、兼容面未改、编译/告警 | 运行时行为 |
| D | 真机软件（Redmi 8A / API 29） | 权限结果回传、自动续跑、界面文案、销毁行为、报告页反馈 | 真实传感器/平台链路（除非另有真实源） |
| R | 真实网关读取（devCount=0） | 真实 GATT 读取后的终态显示与下一周期 | 多设备/9 台/真实占位源（需另备条件） |
| V | 已部署服务 | 上传/持久化/回读 | **本阶段不涉及，本设计不产出 V 用例** |

---

## 2. 人工范围决定、原检查者范围错误与阶段边界

### 2.1 当前人工范围决定（引用，不改写）

| 项 | 记录 | 本设计处置 |
|---|---|---|
| `FW-LIGHT-DEF2-ENABLE` | `APPROVED_DEFERRED` / NOT EXECUTED；`HR-8A11935AD0A7`，2026-11-11T03:50:42Z；原回复「无法准备，先跳过」；`deferral_allowed=true` | **不重测、不重复询问、不作为返工依据、不纳入本阶段退出**；报告中显式列为 NOT EXECUTED |
| `BOT-01` | `REASSESS` 后已由嵌入式测试以自动方式执行并 PASS（2026-11-12） | 已闭合；不重复安排；属固件/嵌入式 |

### 2.2 原检查者需要重评的范围错误（本设计确认）

| # | 原报告位置 | 范围错误 | 正确分类（已由复核轮纠正） |
|---|---|---|---|
| E-1 | `PRODUCT-ACCEPTANCE-002` §5 R4 | 把人工已确认延期的“无光判据启用”与“未满足/未执行”混同 | 上报公式实现 = 已实现、静态一致；无光判据启用 = NOT EXECUTED（人工延期）；真实升温/超温触发 = NOT EXECUTED（温度条件） |
| E-2 | 同报告最终判定 | 以“需求 4 的无光分支仍禁用”作为整体 `TEST_FAIL` 依据之一 | 整体失败依据只保留其余外部条件/后续阶段项；人工延期项不作为返工或整体失败依据 |
| E-3 | 同报告 §7 | 后续回填入口正确，但未与“本轮不返工”区分 | 保留为需求/接受侧未决事项；不据此要求固件或 Android 返工 |
| E-4 | 同报告 §6 N-1 | 披露正确，但与 §5 R4 标签及最终判定矛盾 | 以 N-1 披露为准，修正 R4 标签与最终判定依据 |

**结论**：本设计不把 `FW-LIGHT-DEF2-ENABLE`、真实温度触发、光强双态、功耗、跨链路、平台写入/回读列入 ITEM-001/002/003 的验收前提。若实现或执行阶段试图用这些项作为本轮返工理由，属于重复原检查者范围错误，应退回本设计的分类。

### 2.3 本阶段范围与阶段外项

**本阶段（本设计覆盖）**：
- ITEM-001：扫描权限结果由 Fragment launcher 发起/消费；授权后自动续跑；真实原因文案；拒绝/不再询问/销毁；启动与保存/清除不请求定位。
- ITEM-002：`GatewayReadState` 纯逻辑映射；`MqtttService.onCharacteristicRead` 所有分支设终态；真实网关 `devCount=0` 的终态显示与下一周期可读。
- ITEM-003：`ReportRegenerateFeedback` 纯逻辑；报告页点击进度/禁用/终态提示/按钮恢复；零设备真机提示；连续点击不并行；销毁后不崩溃。
- 受影响面的回归：首页/我的/设定/报告入口/传感器列表入口/网关对话框/守护启停/冷启动；构建与宿主机全量用例；兼容面（协议/存储/上传/报告统计/通知）。

**阶段外（不进入本阶段退出，不得由本设计豁免为通过）**：

| 项 | 处置 | 责任阶段/依据 |
|---|---|---|
| 真实升温/超温触发、光强双态/遮光、功耗 | **NOT EXECUTED**；用户明确当前不能调温/改光/测功耗 | 外部条件/后续阶段 |
| 433→网关→BLE→手机同笔一致性、真实 BLE/GATT/MTU/多设备/9 台 | **NOT EXECUTED**；用户安排后续联调 | 联调阶段（真实网关 devCount=0 单点除外，见 D-2） |
| 平台 `/upload_data` 写入、幂等、持久化回读、401/403 | **NOT EXECUTED**；无写接口/数据库授权 | 平台/联调阶段 |
| API 31+ 权限分支 | **NOT EXECUTED**；无 API 31+ 设备 | 设备条件 |
| 注册/登录、有真实绑定数据的列表、真实告警下发 | **NOT EXECUTED**；无账号/数据 | 外部数据 |
| `FW-LIGHT-DEF2-ENABLE` | **NOT EXECUTED（人工延期）**；不属于 Android | 固件/需求判据接受 |
| iOS | 不在范围（用户明确暂不改） | — |

> **阶段通过 ≠ 完整产品通过**。上述阶段外项必须在最终报告中逐项披露；本设计不因阶段内用例通过而将其改写为通过。

---

## 3. 需求与验收追踪矩阵

### 3.1 原始需求中与 Android 相关的落点（只列与本轮 3 项有关的验证边界）

| ID | 需求/约束 | Android 落点 | 本轮如何验证 | 不在本轮的证据 |
|---|---|---|---|---|
| R-01 | GXHT40 替换后 BLE 布局/单位/负温不变 | App 不重算、不改布局 | REG-05（兼容面不变）、REG-01（既有 codec 用例） | 真实传感器读数 |
| R-02 | 光敏分压与光照 | App 不显示/不推断光照 | REG-05 静态核查；**不设计光强刺激** | 无光判据、双态 |
| R-03 | 180 s 采集 | App 读取节拍/文案 | N-2 的下一周期读取（D-2-D-02） | 真实触发周期长跑 |
| R-04 | 升温 >0.9℃ 且无光 / >35℃ | App 不重算门控 | REG-05 静态核查 | 真实温度/光判据（阶段外） |
| R-05 | 服务器复用 | 既有地址/topic/上传字段不变 | REG-05 静态核查 | 平台写入/回读 |
| R-06 | 只改 Android、不改 iOS | 仅 Android 修复 | REG-05；本设计只动 Android 测试面 | — |
| R-07 | 当前有光 | App 不据此分支 | 不设计光强测试 | — |
| R-08 | UART1 调试 | 不属 Android | — | 固件/嵌入式 |

### 3.2 任务验收条件逐条映射

“必需”列：**是** = 本阶段退出必须；**条件** = 任务文本明确“在可得条件下”或以真实源为条件，缺失时记 NOT EXECUTED 并披露；**否** = 由 H/S/既有证据覆盖，不要求真机。

#### ITEM-001（N-1）

| # | 验收条件（原文要点） | 用例 | 证据类别 | 必需 |
|---|---|---|---|---|
| 1.1 | 权限请求由 `NotificationsFragment` 的 `registerForActivityResult(RequestMultiplePermissions)` 发起并消费；对话框不再直接调用 Activity 级 `requestPermissions` | N1-S-01、N1-S-02 | S | 是 |
| 1.2 | API<31 真机首次点击「搜索附近网关」并授予定位后，无需再次点击即自动扫描（≤3 s 出现 `GatewayDiscovery scan started`），10 s 内 `stopScan`；停止后无新回调 | N1-D-01、N1-D-02 | D | 是 |
| 1.3 | 授权后蓝牙关闭 → “蓝牙未开启（去开启）”且不扫描；定位总开关关闭 → “搜索网关需要开启定位（去开启定位）”；权限拒绝 → 具体缺失权限；不得统一“未授予扫描权限，无法搜索网关” | N1-H-01、N1-D-03、N1-D-04、N1-D-05 | H/D | 是 |
| 1.4 | 拒绝/“不再询问”后清除 `pendingScan`，不自动第二次请求；再次点击才重新请求；对话框关闭/页面销毁后不迟发扫描、不操作已销毁视图 | N1-D-06、N1-D-07、N1-S-02 | D/S | 是 |
| 1.5 | 启动 App、保存/清除网关均不请求定位；仅“搜索附近网关/扩大搜索”触发请求 | N1-S-03、N1-D-09、N1-D-10 | S/D | 是 |
| 1.6 | 新增宿主机静态核查（launcher 存在、对话框无 Activity 级请求、销毁清理）通过；现有 261 项全通过、`assembleDebug` 成功、无新增编译告警 | N1-S-01、N1-S-02、REG-01、REG-02 | S/H | 是 |
| 1.7 | 首次授权自动开始、拒绝/蓝牙关闭/定位关闭文案与无重复请求由软件测试在真机独立验收；不以宿主机断言代替 | 全部 N1-D | D | 是 |

#### ITEM-002（N-2）

| # | 验收条件（原文要点） | 用例 | 证据类别 | 必需 |
|---|---|---|---|---|
| 2.1 | 新增纯逻辑 `GatewayReadState`（无 Android 依赖）：有效→DATA_OK；全占位→GATEWAY_NO_DATA；NO_DEVICE→GATEWAY_NO_DEVICE；FRAME_TOO_SHORT→READ_INVALID；LENGTH_MISMATCH/LENGTH_NOT_ALIGNED→READ_INCOMPLETE；DECRYPT_FAILED/CIPHERTEXT_NOT_ALIGNED→READ_INVALID；未知→READ_INVALID；带稳定状态码与文案类别 | N2-H-01…H-11 | H | 是 |
| 2.2 | `MqtttService` 在每次 `onCharacteristicRead` 的 Success/Rejected/长度拒绝分支都调用同一映射并写首页链路状态；读取结束不存在仍停留 CONNECTING 的路径 | N2-S-01、N2-S-02、N2-D-06 | S/D | 是 |
| 2.3 | 真实网关 `devCount=0` 读取（MTU 240、FFE2 160 B）后首页显示“网关已连接，暂无设备（N=0）”，下一个 180 s 周期仍可正常读取；不再长时间“正在连接网关…” | N2-D-01、N2-D-02 | R/D | 是 |
| 2.4 | 有效批次仍 DATA_OK；全占位 GATEWAY_NO_DATA；READ_INCOMPLETE/READ_INVALID 文案与原因可区分；状态更新不改变本地入库/上传/告警 | N2-H-01…H-10、N2-D-03、N2-D-04、REG-05 | H/D | 条件（真实源可得时） |
| 2.5 | 新增宿主机映射矩阵全通过；现有 261 项全通过、`assembleDebug` 成功、无新增编译告警 | REG-01、REG-02 | H/S | 是 |
| 2.6 | 真实 `devCount=0`、占位与完整批次的显示由软件测试真机独立验收；不宣称真实多设备/9 台已通过 | N2-D-01…D-04 | R/D | 是（占位/完整批次为“可得时”） |

#### ITEM-003（N-3）

| # | 验收条件（原文要点） | 用例 | 证据类别 | 必需 |
|---|---|---|---|---|
| 3.1 | 新增纯逻辑 `ReportRegenerateFeedback`（无 Android 依赖）：零设备、无当日读数、生成内容为空、成功、失败、重复点击等输入均返回确定的消息类别/按钮状态；无数据不算错误但必须提示 | N3-H-01…H-08 | H | 是 |
| 3.2 | 点击「重新生成」立即显示进度并禁用按钮；结束（成功/失败/无数据）后按钮恢复，不出现无反馈的静默完成 | N3-S-01、N3-D-01 | S/D | 是 |
| 3.3 | Worker 成功但无可生成数据（无设备/无当日样本/文本为空或与上次相同）→ Toast/Snackbar“暂无可生成的数据（尚无设备读数）”；Worker 失败 → “报告生成失败，请稍后重试”；有数据成功 → 刷新报告文本与生成时间 | N3-H-01…H-05、N3-D-01、N3-D-04、N3-D-05 | H/D | 是（失败与有数据真机为条件/既有证据） |
| 3.4 | 连续点击不并行触发多次生成；页面销毁后回调不操作已销毁视图、不崩溃 | N3-S-01、N3-D-02、N3-D-03 | S/D | 是 |
| 3.5 | 报告内容/统计规则、`KEY_REPORT_TEXT/KEY_REPORT_TIME`、`ReportGenerator` 输出与通知行为不变；不新增线上字段 | N3-S-02、REG-05 | S | 是 |
| 3.6 | 新增宿主机纯逻辑测试全通过；现有 261 项全通过、`assembleDebug` 成功、无新增编译告警 | REG-01、REG-02 | H/S | 是 |
| 3.7 | 零设备/无数据提示与按钮恢复由软件测试真机独立验收；有数据时刷新沿用既有行为与证据 | N3-D-01 | D | 是 |

---

## 4. 测试层次与执行主体

| 层 | 内容 | 执行主体 | 当前可用性 |
|---|---|---|---|
| H0 | 纯逻辑矩阵：`BlePermissionPolicy` 原因/文案；`GatewayReadState`；`ReportRegenerateFeedback`；必要的状态机纯函数 | 软件测试（独立验证测试）+ 实现（自检测试） | 宿主机运行器可用；新类尚未实现 |
| S1 | 静态核查：launcher/无 Activity 级请求/销毁清理；读取分支终态；报告页接线；兼容面 | 软件测试（脚本/JUnit 静态测试） | 可用（读源码） |
| H2 | 全量宿主机回归 + `assembleDebug` + 告警检查 | 软件测试 | 可用（本轮 261/0、构建成功） |
| D1 | 真机 API 29：权限/扫描/报告/销毁/回归 | 后续软件测试执行能力（需 `android_test`） | 本设计能力不可用 |
| R1 | 真实网关 `devCount=0` 读取与下一周期 | 后续软件测试执行能力（需 `android_test` + 真实网关） | 未核验 |
| V | 平台写入/回读 | 不属本阶段 | 无授权 |

**执行分工与独立验证**：
- 实现必须按任务验收新增宿主机自检（N1 静态核查、N2 映射矩阵、N3 反馈矩阵）并保证全量 261+ 通过。
- 软件测试在执行阶段除复核实现自检外，**另写独立验证用例**（建议放在 `app/src/test/java/com/jinyuni/dengbei_care/verification/` 下，使用独立期望表，不复制实现断言）：
  - `GatewayReadStateItem002VerificationTest`（N-2 全矩阵 + 未知输入）；
  - `ReportRegenerateFeedbackItem003VerificationTest`（N-3 全矩阵 + 重复点击）；
  - `NotificationsPermissionWiringItem001VerificationTest`（N-1 静态接线/清理核查）。
- 独立用例必须只用纯 JVM/JUnit 与文件读取，不依赖 Android 运行时；真机行为不能由这些用例替代。

---

## 5. 用例设计

> 通用约定：每个 D 用例开始/结束记录设备状态（蓝牙、定位、权限、网络、在装版本）；每个用例保存原始证据（命令、时间、日志切片、UI dump/截图）。清理要求见 §7。

### 5.1 N-1 扫描权限结果交付与自动续跑（ITEM-001）

#### H0/S1 用例

| ID | 目的 | 准备/刺激 | 判据 | 证据 |
|---|---|---|---|---|
| N1-H-01 | 策略原因/文案矩阵（纯逻辑） | `BlePermissionPolicy.evaluate` 输入组合：API29/API31、权限集合、`needScan=true/false`、`adapterEnabled`、`locationEnabled` | `BLUETOOTH_OFF` → `蓝牙未开启（去开启）`；定位总开关关 → `LOCATION_OFF` → `搜索网关需要开启定位（去开启定位）`；缺 `FINE_LOCATION` → `LOCATION_PERMISSION_MISSING` → 含“定位权限”；缺 `BLUETOOTH_SCAN` → `SCAN_PERMISSION_MISSING`；`canScan=false` 的每种输入都给出**不同且具体**的 `reason/userMessage`，不得统一为“未授予扫描权限” | 测试输出 |
| N1-H-02 | 挂起动作状态机（如实现抽出纯函数） | `IDLE → REQUESTING → (granted) POLICY_RECHECK → SCANNING / REASON_SHOWN`；`denied → REASON_SHOWN → IDLE`；`destroy → IDLE` | 每种转换的“是否保留 pendingScan / 是否启动扫描 / 是否显示原因”确定；拒绝后无自动第二次请求；销毁后无迟发动作 | 测试输出 |
| N1-S-01 | launcher 接线静态核查 | 读取 `NotificationsFragment.kt` | 存在 `registerForActivityResult(ActivityResultContracts.RequestMultiplePermissions())`（或等价 `RequestMultiplePermissions` launcher）并在回调中消费结果；扫描权限结果不再依赖 `onRequestPermissionsResult` 分发；launcher 仅在 Fragment 生命周期内注册 | 静态检查输出（文件/行号） |
| N1-S-02 | 对话框路径与销毁清理静态核查 | 读取 `GatewayBleSettingsDialog.kt`、`NotificationsFragment.kt` | 对话框代码**无** `ActivityCompat.requestPermissions(` 调用；`dismiss()/stopScan()/onDestroyView()` 清除 `pendingScan`、停止扫描、释放回调；不存在“请求结果到达已销毁视图”的路径 | 静态检查输出 |
| N1-S-03 | 请求触发范围静态核查 | 读取点击处理器与 `onViewCreated/onCreate` | 只有「搜索附近网关」「扩大搜索」点击路径调用权限请求；启动、保存、清除路径不请求定位；Manifest 无新增权限声明 | 静态检查输出 + `git diff` Manifest |

> 静态核查必须**限定扫描权限路径**：`NotificationsFragment` 的相机权限仍使用 Activity 级请求（既有功能），不得误判为本项缺陷；检查器应只匹配扫描/定位相关请求。

#### D1 用例（真机 API 29）

| ID | 目的 | 准备/刺激 | 观测 | 判据 | 清理 |
|---|---|---|---|---|---|
| N1-D-01 | 首次授权后自动扫描（正常） | 安装新候选；清除 App 权限（`pm revoke … ACCESS_FINE_LOCATION` 或系统设置）；确保蓝牙开、定位开；打开「网关与上传设置」→ 点击「搜索附近网关」→ 在系统对话框点“允许” | logcat `GatewayDiscovery: scan started …`；UI 状态“正在搜索…”；10 s 后 `scan stopped` | 授权后**无需再次点击**；`scan started` 在授权完成 ≤3 s 内出现；`stopScan/scan stopped` 在点击后 ≤10 s（+调度余量 ≤1 s）；停止后无新候选回调 | `pm grant` 恢复权限/按原状态复原；关闭对话框；记录前后状态 |
| N1-D-02 | 有界停止且停止后无回调（正常） | 同 D-01，保持扫描 12 s | 日志时间戳；UI/候选更新次数 | 10 s 上限触发；`scan stopped` 后无 `onScanResult` 驱动的界面/日志新增；无重复 `startScan` | 复原 |
| N1-D-03 | 授权后蓝牙关闭（异常） | 蓝牙开、定位开；先授予定位；打开对话框后**关闭系统蓝牙**；点击「搜索附近网关」 | Toast/状态文案；logcat 无 `scan started` | 显示“蓝牙未开启（去开启）”；**不启动扫描**；不显示“未授予扫描权限，无法搜索网关” | 恢复蓝牙开；记录初始状态 |
| N1-D-04 | 定位总开关关闭（异常） | 蓝牙开；定位总开关关；定位权限已授予；点击搜索 | 文案；日志 | 显示“搜索网关需要开启定位（去开启定位）”；不扫描；原因不是“权限缺失” | 恢复定位开 |
| N1-D-05 | 权限拒绝（异常） | 清除定位权限；点击搜索；在系统对话框点“拒绝” | 文案；日志；无 `scan started` | 提示具体缺失权限（含“定位权限”语义），不启动扫描；不弹第二次请求 | 恢复权限状态 |
| N1-D-06 | “不再询问”/永久拒绝（异常） | 先拒绝一次，再触发并选择“不再询问”（或 `pm revoke` + `pm set-permission-flags … user-fixed` 模拟）；再次点击搜索 | 系统对话框是否再次出现；文案；日志 | 不自动重复弹窗；`pendingScan` 已清除；仅用户再次点击才重新请求；提示具体缺失项 | 清除 `user-fixed` 标志并恢复权限 |
| N1-D-07 | 销毁/关闭期间无迟发（异常） | 在权限对话框显示期间按返回/关闭对话框；另一次在扫描进行中关闭对话框/离开页面 | logcat（无 `scan started` 迟发、无 `stopScan` 异常）；无崩溃/ANR | 关闭后不启动新扫描、不更新已销毁视图；无 `FATAL`/`ANR`；已启动的扫描被停止 | 复原页面/权限 |
| N1-D-08 | 重复点击不并行（异常） | 连续快速点击「搜索附近网关」3 次（授权后） | `scan started`/`scan stopped` 次数与时间 | 不出现并行扫描；同一时刻最多一个 `GatewayDiscovery` 扫描；最终有界停止 | 复原 |
| N1-D-09 | 启动/保存/清除不请求定位（正常） | 冷启动 App；进入我的页；打开网关对话框；保存合法地址；清除地址 | 系统权限对话框是否出现；`dumpsys package` 权限状态；logcat | 启动、保存、清除均**不触发**定位权限请求；只有搜索/扩大搜索触发 | 恢复原网关地址/清除状态 |
| N1-D-10 | 扩大搜索路径同样自动续跑（正常） | 清除定位权限；点击「扩大搜索（不过滤）」→ 授予 | 同 D-01 | 授权后自动开始**不过滤**扫描（日志 `filterByFfe0=false`）；10 s 内停止 | 复原 |

### 5.2 N-2 读取尝试的终态链路状态（ITEM-002）

#### H0 映射矩阵（独立期望表）

| ID | 输入（解密/长度/结果） | 期望状态码 | 期望文案类别（关键子串） |
|---|---|---|---|
| N2-H-01 | `Success(devCount≥1)`，≥1 条有效读数 | `DATA_OK` | “网关已连接（最近读取 HH:mm）” |
| N2-H-02 | `Success`，全部为占位 | `GATEWAY_NO_DATA` | “网关暂无传感器数据（等待传感器上报）” |
| N2-H-03 | `Success`，有效与占位混合（≥1 有效） | `DATA_OK` | 同 H-01 |
| N2-H-04 | `Rejected(NO_DEVICE)` | `GATEWAY_NO_DEVICE` | “网关已连接，暂无设备（N=0）” |
| N2-H-05 | `Rejected(FRAME_TOO_SHORT)` | `READ_INVALID` | “数据无效”+原因类别 |
| N2-H-06 | `Rejected(LENGTH_MISMATCH)` | `READ_INCOMPLETE` | “读取不完整”+长度类别 |
| N2-H-07 | `Rejected(CIPHERTEXT_NOT_ALIGNED)` | `READ_INVALID` | “数据无效”+对齐原因 |
| N2-H-08 | `Rejected(DECRYPT_FAILED)` | `READ_INVALID` | “数据无效”+解密失败原因 |
| N2-H-09 | `LengthVerdict.Refuse(LENGTH_TOO_SMALL / LENGTH_NOT_ALIGNED)` | `READ_INCOMPLETE` | “读取不完整”+长度/对齐原因 |
| N2-H-10 | 未知/未映射的映射输入（不在上述枚举内的未来原因） | `READ_INVALID` | “数据无效”+原因；**不得**返回/保留 `CONNECTING` |
| N2-H-11 | 上述任一输入 | 状态码 | 状态码稳定、可日志断言；文案类别可区分；同一输入重复调用结果一致（无状态泄漏） |

附加断言：
- `GatewayReadState` 不引用任何 Android 类型（静态/反射检查 import）。
- 对 H-01…H-10 的全集断言“**不存在**输出 `CONNECTING` 的路径”（用枚举/穷举或断言函数签名不含该值）。
- `DATA_OK/GATEWAY_NO_DATA` 的判定必须与 `BleObservation.classify` 的“有效/占位”语义一致；不得把合法 `0.0/0.0` 当占位（由既有 `BleObservationTest` 回归覆盖）。
- GATT `status != GATT_SUCCESS` 不作为 `GatewayReadState` 的映射输入类型要求；它由 `MqtttService` 的读取接线落到**可区分的终态**（设计期望 `READ_INVALID`，带原因；见 N2-S-01/N2-D-06），不得静默停留 `CONNECTING`。

#### S/D/R 用例

| ID | 目的 | 准备/刺激 | 观测 | 判据 | 清理 |
|---|---|---|---|---|---|
| N2-S-01 | 读取分支接线静态核查 | 读取 `MqtttService.kt` 的 `onCharacteristicRead` | 每个退出路径（Success 有效/全占位、Rejected 各原因、长度拒绝、GATT 非成功）都调用同一映射并 `setLinkStatus`；读取开始后除读取前状态外不再写入 `CONNECTING` | 静态检查输出（分支/行号）；允许实现抽取辅助函数，但不得有“只写日志不写状态”的分支 | — |
| N2-S-02 | 无 Android 依赖与集中映射静态核查 | 读 `protocol/GatewayReadState.kt` + `MqtttService.kt` | `GatewayReadState` 无 Android import；`MqtttService` 不重复实现状态判定（只做调用/写状态） | 静态检查输出 | — |
| N2-D-01 | 真实网关 `devCount=0` 终态（正常/关键） | 安装新候选；配置真实网关地址（由 App 发现/用户确认，不猜地址）；启动守护/连接；等待一次读取完成 | 首页链路状态；logcat `link_state=GATEWAY_NO_DEVICE`/`dev=0`/`valueSize`/`mtu`；UI dump | 读取完成后 ≤3 s 首页显示“网关已连接，暂无设备（N=0）”；不再显示“正在连接网关…”；无崩溃。同时记录实际协商 MTU 与 FFE2 返回字节（修复前证据为 MTU 240 / 160 B；若实际不同，记录实际值并说明，不单独据此判缺陷） | 停止守护；恢复网关地址原状态（如为测试配置） |
| N2-D-02 | 下一 180 s 周期仍可读（正常） | 保持守护运行，观察 ≥2 个读取周期（约 6.5 min，含余量） | 每周期 `link_state` 与 `dev=`/`valueSize`；无 `FATAL`/`ANR` | 第 2 个周期仍产生 `GATEWAY_NO_DEVICE`（或按真实批次映射的终态）；不回到长时间 CONNECTING；无异常崩溃 | 停止守护 |
| N2-D-03 | 有效批次 DATA_OK（条件） | 具备真实网关 + 至少 1 台真实传感器在网并上报 | 首页状态、logcat `dev=… valid=…`、入库/上传日志 | 显示 `DATA_OK` 文案；不改变本地入库/上传/告警语义 | 复原数据/停止守护 |
| N2-D-04 | 全占位 GATEWAY_NO_DATA（条件） | 真实网关有设备槽位但全部为占位（按可得源构造；不得伪造 BLE） | 日志 `placeholder=… valid=0`；首页状态 | 显示“网关暂无传感器数据（等待传感器上报）”；不产生读数/不写库 | 复原 |
| N2-D-05 | 状态更新不改变业务语义（正常/回归） | 在 D-01/D-02 会话中观察 | 本地 DB 行数、上传队列、告警计数 | `GATEWAY_NO_DEVICE`/`READ_*` 状态不新增读数/不入库/不触发告警/不改变上传；`DATA_OK`/`GATEWAY_NO_DATA` 行为与既有证据一致 | 清理测试数据 |
| N2-D-06 | 无长时间 CONNECTING（异常/关键） | 从连接开始到读取完成全程录 logcat + 每秒 UI 采样（≤20 s） | `link_state=CONNECTING` 的持续时间；读取完成后的状态 | 读取尝试结束后不存在持续 CONNECTING；若无法读取（超时/失败），也必须落到可区分的终态（至少 READ_INVALID/READ_INCOMPLETE 并有原因），不得静默停留 | 停止守护 |

### 5.3 N-3 报告页“重新生成”可感知反馈（ITEM-003）

#### H0 反馈矩阵（独立期望表）

| ID | 输入（succeeded / hasContent / deviceCount / sampleCount / inProgress） | 期望消息类别 | 期望按钮 |
|---|---|---|---|
| N3-H-01 | `true / false / 0 / 0 / false` | `NO_DATA`：“暂无可生成的数据（尚无设备读数）” | 恢复可点击 |
| N3-H-02 | `true / false / ≥1 / 0 / false`（无当日读数） | `NO_DATA` | 恢复 |
| N3-H-03 | `true / false / ≥1 / ≥1 / false`（生成文本为空或与上次相同） | `NO_DATA` | 恢复 |
| N3-H-04 | `true / true / ≥1 / ≥1 / false` | `SUCCESS`（刷新文本/生成时间，无错误提示） | 恢复 |
| N3-H-05 | `false / * / * / * / false` | `FAILURE`：“报告生成失败，请稍后重试” | 恢复 |
| N3-H-06 | `* / * / * / * / true`（重复点击/进行中） | 保持进行中，不发完成提示 | 保持禁用 |
| N3-H-07 | 上述所有终态 | 按钮恢复决策确定 | 除进行中外均恢复 |
| N3-H-08 | 文案稳定性 | 两个用户文案为固定类别/关键子串，不随设备数变化而变成“未授予/失败” | — |

附加断言：
- `ReportRegenerateFeedback` 无 Android import；输入穷举/参数化，输出确定。
- “无数据”不记为失败（`succeeded=true` 但无内容 → `NO_DATA`，不是 `FAILURE`）。
- “重复点击/进行中”可由纯逻辑的 `inProgress` 输入或 UI 侧串行控制实现；本设计只断言**可观察行为**（不并行、保持禁用/无完成提示）与纯逻辑对终态输入的确定输出，不强制内部签名。
- 不改 `KEY_REPORT_TEXT/KEY_REPORT_TIME` 语义与 `ReportGenerator` 输出。

#### S/D 用例

| ID | 目的 | 准备/刺激 | 观测 | 判据 | 清理 |
|---|---|---|---|---|---|
| N3-S-01 | 报告页接线静态核查 | 读取 `ReportActivity.kt`、布局、Worker | 点击后立即禁用按钮并显示进度；监听 RUNNING/SUCCEEDED/FAILED（含快速完成）；终态显示消息并恢复按钮；销毁后不回调视图 | 静态检查输出；不得只依赖 RUNNING 才禁用/显示进度 | — |
| N3-S-02 | 兼容面静态核查 | `git diff` 兼容面 | `KEY_REPORT_TEXT/KEY_REPORT_TIME` 名称不变；`ReportGenerator`/统计/通知调度未改；无新增线上字段；BLE/协议/存储/上传文件无改动 | 静态 diff 输出 | — |
| N3-D-01 | 零设备点击反馈（关键/正常） | 当前 0 个已保存设备；打开报告页；点击「重新生成」 | 点击瞬间按钮禁用/进度；Toast/Snackbar；按钮恢复；页面文本/时间；logcat `No saved devices, skipping report` | 点击瞬间禁用+进度；Worker 完成后终态提示出现（合理调度窗口 ≤15 s；超过则记录并判异常）；提示为“暂无可生成的数据（尚无设备读数）”；按钮恢复；不出现静默完成；无崩溃 | 复原页面；无数据变更 |
| N3-D-02 | 连续点击不并行（异常） | 零设备；快速点击 3 次 | WorkManager 日志/`dumpsys jobscheduler`；`DailyReportWorker` 执行次数；按钮状态 | 不产生并行生成；同一时刻最多 1 个 one-shot 工作；最终有且仅有一个终态提示；按钮恢复 | 复原 |
| N3-D-03 | 页面销毁后不崩溃（异常） | 点击后立即按返回退出报告页；重复 3 次 | logcat `FATAL`/`ANR`；进程存活 | 无崩溃/ANR；无对已销毁视图的回调异常 | 复原 |
| N3-D-04 | 失败提示（条件/宿主优先） | 纯逻辑矩阵覆盖；真机失败诱导仅在存在**非破坏性**方法时执行 | 文案类别 | `FAILURE` 文案正确、按钮恢复；无安全诱导方法时记 NOT EXECUTED（不伪造） | — |
| N3-D-05 | 有数据成功刷新（条件/既有证据） | 若存在已保存设备与当日读数；点击生成 | 报告文本、生成时间、按钮 | 文本与时间刷新；按钮恢复；内容/统计规则不变；无数据时可沿用既有证据并标注边界 | 清理测试数据 |

### 5.4 回归与完整交付检查

| ID | 目的 | 准备/刺激 | 判据 | 证据 |
|---|---|---|---|---|
| REG-01 | 全量宿主机用例 | `./gradlew --offline :app:testDebugUnitTest` | `261 + 新增` 项全通过，0 失败/0 错误/0 跳过；记录套件/用例数、提交号、时间 | XML 汇总 + 命令输出 |
| REG-02 | 构建与告警 | `./gradlew --offline :app:assembleDebug`；为取得真实编译告警使用 `--rerun-tasks`（或在实现开始前先记录 as-built 完整构建告警清单作为基线） | `BUILD SUCCESSFUL`；产物存在；与修复前基线相比**无新增**编译告警（无法取得基线时，要求新构建告警为空或逐条证明为既有）；记录 APK 大小/SHA-256 | 构建日志 + 摘要 |
| REG-03 | 真机核心路径回归 | 冷启动；首页/我的/设定；报告入口；传感器列表入口；网关对话框；守护启动/停止 | 页面可达、无崩溃；既有文案/默认值不变（设定 2 / 0.1 / 35 / 29 / 75） | UI dump/截图 + logcat |
| REG-04 | 崩溃/ANR | 上述会话全程 | 本应用 `FATAL EXCEPTION` 0、`ANR in com.jinyuni…` 0 | logcat 统计 |
| REG-05 | 兼容面 | 源码 diff + 既有用例 | BLE 记录布局/字节序/符号、AES/ECB、GATT UUID、`0xA1`、topic、`time:temps:humis`、`/upload_data` 字段/单位、SQLite v3 表/键、告警阈值/去抖、180 s、报告统计、`MyPrefs` 键均未改；无新增权限/线上字段 | `git diff` + 静态检查 |
| REG-06 | 候选身份与安装一致性 | 新构建 APK；`adb install -r`；设备侧摘要 | 主机 APK 与设备 `base.apk` 的 SHA-256 一致；`versionCode/versionName` 与实现声明一致；记录安装前后版本 | `pm path` + `sha256sum` + `dumpsys package` |

---

## 6. 证据规范

每次执行建立独立目录：`evidence/aa006_software_test_<YYYYMMDD>/`（由执行能力创建；本设计不预建空目录）。至少保存：

1. **身份**：HEAD、`git status --porcelain`、构建命令、APK 大小/SHA-256/MD5、设备序列号/型号/API、`versionCode/versionName`、设备侧 APK 摘要、安装时间。
2. **环境**：每轮开始/结束的 `bluetooth_on`、`location_mode`、权限授予状态、默认网络、网关地址配置状态。
3. **用例原始证据**：
   - H：JUnit XML + 控制台摘要；独立验证用例的期望表（与本文件一致）。
   - S：静态检查脚本/测试输出（文件+行号）与 `git diff` 兼容面。
   - D/R：命令、时间戳、logcat 切片（含 `GatewayDiscovery scan started/stopped`、`MqttService link_state=`、`DailyReportWorker`、`ReportScheduler`）、UI dump/截图、Toast/Snackbar 捕获方式。
4. **判定表**：用例 ID → PASS/FAIL/NOT EXECUTED/CONDITIONAL/UNKNOWN CAUSE → 证据文件 → 备注。
5. **清理证据**：设备权限/蓝牙/定位/网关地址/测试数据恢复前后对照。
6. **未执行项**：逐项写明原因、缺失条件、责任阶段、未来恢复后必须重做的验证；不得以空文件或“无输出”冒充证据。

证据路径与原始 `.bin` 的处理沿用项目既有约定：可提交的文本/XML/日志摘要入库；被 `.gitignore` 排除的原始二进制在本机保留并以 hex/摘要文本复核，登记在清单中。

---

## 7. 清理要求（每个用例执行后）

| 对象 | 清理 |
|---|---|
| 设备权限 | 将 `ACCESS_FINE_LOCATION`/`BLUETOOTH_SCAN`/`BLUETOOTH_CONNECT`/`POST_NOTIFICATIONS` 恢复到执行前状态；清除测试设置的 `user-fixed` 标志 |
| 设备开关 | 蓝牙、定位总开关恢复到初始状态（本轮初始：蓝牙开、定位开） |
| 网关配置 | 保存/清除用例使用测试值；结束后恢复原 `gateway_ble_addr`（或恢复“未配置”标记），不把测试地址留给用户 |
| App 数据 | 报告/列表/绑定相关测试若产生测试数据，使用测试副本并在结束后删除；不清理用户真实数据 |
| 扫描/服务 | 停止扫描、停止守护、关闭对话框；确认无遗留回调/定时器 |
| 进程/后台任务 | 结束本次自建的测试进程与临时命令；记录已停止；不遗留后台任务 |
| 证据 | 清理前后各留一份关键状态证据，证明恢复完成且未误删 |

---

## 8. 前置条件、工具与执行交接

### 8.1 当前可执行性

| 项 | 当前状态 | 对本设计的影响 | 恢复/交接 |
|---|---|---|---|
| 宿主机运行器 | **可用**（本轮 `BUILD SUCCESSFUL`，261/0） | H0/H2 可在实现后立即执行 | 实现后重跑并记录当轮 |
| 新纯逻辑类 | **未实现**（`GatewayReadState`、`ReportRegenerateFeedback` 不存在） | H0 用例当前不可执行 | 由 Android 实现按任务完成 |
| 静态核查 | **可用**（源码可读） | S1 可在实现后执行 | 执行能力运行脚本/测试 |
| `android_app_testing` | 对**本设计能力**不可用（缺 `android_test`） | 全部 D/R 用例本能力不能执行 | 后续软件测试执行能力需该工具 |
| 只读 ADB | 可用（本轮身份/版本查询成功） | 只能做身份/状态核对，不能点按/改设置 | 真机操作用 `android_test` |
| 设备状态 | 在线；蓝牙开、定位开、无默认网络；App 已装 10/1.9 | D-01/D-09/D-10 的前置可通过设置/授权变化构造 | 执行轮记录并恢复 |
| 真实网关 | 既有网关但本轮未核验；当前手机未配置网关地址（首页“未配置网关地址”） | N2-D-01/D-02 需真实网关在线、地址经 App 发现/用户确认 | 现场上电/确认；不得猜地址 |
| 真实传感器数据 | 当前无绑定设备、无真实上报源 | N2-D-03/D-04 为条件用例；缺失记 NOT EXECUTED | 需要真实网关+传感器或明确条件 |
| 有数据报告 | 当前 0 已保存设备 | N3-D-05 为条件用例 | 既有证据可沿用，另注明 |
| 温度/光强/功耗 | 用户明确当前不能准备 | **本设计不安排任何相关测试** | 不创建该条件，不提返工 |
| 平台写/回读 | 无授权 | 本阶段不涉及 | 联调/平台阶段 |

### 8.2 测试环境自修复边界（当前能力）

- `environment_repair` 范围为 `test`；`allow_target_changes=false`、`allow_system_packages=false`。可准备：宿主脚本、夹具、数据构造/解析脚本、隔离测试配置；**不可**安装系统包、改设备/目标设置、改网络/驱动、修改权限文件，也不得通过改产品源码或降低断言让验证通过。
- 本轮未发现需要修复的宿主运行器/依赖问题；缺 `android_test`、真实网关条件、真实传感器源属工具/设备条件缺口，由执行阶段按既有授权处理，不自行扩权。
- 若后续自修复，必须限时、保留可恢复配置，并记录原因/命令/版本/改动/复核结果；单问题不超过 `max_attempts_per_issue=3`。

### 8.3 最小执行顺序（交接给后续软件测试执行能力）

1. 构建新候选，记录提交号、命令、APK 摘要；安装到 `12f046690506`，核对设备侧字节一致（REG-06）。
2. 跑 REG-01/REG-02（全量宿主 + 构建 + 告警），再跑新增独立验证用例（H0）。
3. 跑 S1 静态核查（N1-S-01…03、N2-S-01/02、N3-S-01/02、REG-05）。
4. 跑 D1：N1-D-01…D-10（按用例逐个构造/恢复设备状态）；N3-D-01…D-03；REG-03/04。
5. 需要真实网关时，先做有界探测（≤30 s）：网关供电/广播/地址；经 App 发现或用户确认后配置；执行 N2-D-01/D-02；条件具备再执行 N2-D-03/D-04。
6. 每步保存证据并更新判定表；任何必需项失败或条件缺失，按 §10 返回，不写通过。
7. 结束后按 §7 恢复设备与数据，记录清理证据。

---

## 9. 设计发现、风险与未知项

### 9.1 设计发现（交实现与执行阶段，不在此判产品缺陷）

| ID | 发现（as-built，HEAD `cb423d5`） | 影响 | 处置 |
|---|---|---|---|
| F-01 | `MqtttService.onCharacteristicRead` 的 `status != GATT_SUCCESS` 分支（约 951–955 行）只 `Log.i` 并完成，不设置链路状态 | 读取失败后可能仍停留 CONNECTING，违反 D-37/验收 2.2 | 实现将该分支纳入同一终态映射（设计期望 `READ_INVALID`，带原因）；N2-H-10/N2-S-01/N2-D-06 检查 |
| F-02 | `GatewayBleSettingsDialog.onPermissionResult()` 仅用 `hasScanPermission()` 判断，失败统一提示“未授予扫描权限，无法搜索网关” | 无法区分蓝牙关闭/定位关闭/权限缺失 | 按 `BlePermissionPolicy.evaluate(needScan=true).reason/userMessage` 显示；N1-H-01/D-03/D-04/D-05 |
| F-03 | `NotificationsFragment.requestPermissionsCompat` 仍走 `ActivityCompat.requestPermissions(requireActivity(), …)`；结果依赖 Activity 的 FragmentManager 分发 | NavHost 子 Fragment 可能收不到结果，导致授权后不自动扫描 | 改为 Fragment `registerForActivityResult(RequestMultiplePermissions)`；N1-S-01/D-01 |
| F-04 | `GatewayBleSettingsDialog.pendingScan` 在 `onPermissionResult` 先读出再置空，但 `dismiss()/onDestroyView()` 未显式清除 `pendingScan`（仅 `stopScan()`） | 销毁后可能保留挂起动作/回调 | `dismiss()/onDestroyView()` 清除 `pendingScan` 与 launcher 回调；N1-S-02/D-07 |
| F-05 | `ReportActivity` 仅在 WorkManager `RUNNING` 时显示 loading；快速完成或观察者错过 RUNNING 时可能不显示进度 | “立即进度 + 终态反馈”可能缺失 | 点击即禁用按钮/显示进度，不依赖 RUNNING；终态必须显示消息并恢复；N3-S-01/D-01 |
| F-06 | `DailyReportWorker` 零设备直接 `Result.success()`，不写 `KEY_REPORT_TEXT/TIME`；无当日样本时会写空/相同文本 | UI 无法仅凭 WorkManager 状态区分“成功有内容/成功无数据” | 实现需提供确定的无数据判定（可用 WorkManager 输出 Data 或纯反馈输入；不得新增线上字段）；N3-H-01…H-05/N3-S-01 |
| F-07 | `ReportScheduler.runNow` 使用 `enqueueUniqueWork(..., REPLACE)` | 连续点击会替换而非并行，但快速点击仍可能触发多次 Worker 启动 | 界面在生成中禁用按钮；N3-D-02 断言不并行 |
| F-08 | 扫描权限与相机权限共用 Activity 级请求路径（相机为既有功能） | 静态检查若粗暴匹配 `ActivityCompat.requestPermissions` 会误伤相机 | 静态核查只针对扫描/定位路径；N1-S-01/S-02 限定匹配 |
| F-09 | 状态文案目前部分硬编码在 Kotlin，`strings.xml` 无本项新增文案 | 文案断言不能依赖资源 ID | 以 UI 可见文本/语义类别断言；N3-H-08、D 用例 |
| F-10 | 真实网关 `devCount=0` 需先有网关地址；当前 App 首页显示“未配置网关地址” | 直接跑 D-2 会被前置卡住 | 执行前经 App 发现流程或用户确认地址；不得猜默认值 |
| F-11 | 既有 `BlePermissionPolicy.userMessage` 对 `LOCATION_OFF` 追加“：缺少 …”后缀 | 与验收文案“搜索网关需要开启定位（去开启定位）”不完全逐字一致 | 判据按**语义+关键子串**（“需要开启定位”），不强制逐字；若实现统一为验收原文则更好 |
| F-12 | WorkManager 在 MIUI 上可能延迟调度 | 终态提示的时间可能受系统调度影响 | 记录实际延迟；合理窗口取 ≤15 s；超过 15 s 判异常并附证据；进度/禁用必须在点击瞬间出现 |

### 9.2 未知项与执行处置

| ID | 未知 | 处置 |
|---|---|---|
| U-1 | 真实网关当前是否在线、地址/身份 | 有界探测；不可用则 N2-D-01/D-02 NOT EXECUTED（必需项），不得写通过 |
| U-2 | 是否有真实传感器源产生有效/占位批次 | 有则执行 N2-D-03/D-04；无则 NOT EXECUTED 并披露 |
| U-3 | MIUI 12.5/API29 权限对话框的“不再询问”行为 | 优先 UI 两次拒绝；必要时 `pm set-permission-flags user-fixed`；方法必须记录；不可用则该项 NOT EXECUTED |
| U-4 | 执行能力是否获准切换蓝牙/定位、授予/撤销权限 | 属目标设置变更；不在本设计能力授权内；执行能力未获准时相应 D 用例 NOT EXECUTED，不改判通过 |
| U-5 | WorkManager 快速完成时 UI 观察者是否错过 RUNNING | 以“点击即禁用 + 终态消息”判据覆盖，不依赖中间状态 |
| U-6 | 无数据报告“文本与上次相同”的判定范围（空白/换行） | 实现须定义并记录；测试按“与 `KEY_REPORT_TEXT` 现有值精确比较（可去尾部空白）”断言；不改变报告内容 |
| U-7 | 新候选是否仍为 versionCode 10 / versionName 1.9 | 实现按既有兼容面保持；若变更，记录并说明；测试以实际 APK 摘要区分 |
| U-8 | 真机执行工具何时可用 | 由后续能力/授权决定；本设计不修改权限文件 |

---

## 10. 准入与退出准则（最终判定规则）

### 10.1 准入

- 3 项任务已实现，存在可构建的新候选；`file_manifest.txt` 已同步。
- 宿主机运行器可用（已确认）；新纯逻辑类可由测试引用。
- 设备在线、App 已安装新候选、设备侧摘要与主机一致。
- 真实网关/传感器条件按用例逐项确认；不具备时按 NOT EXECUTED 记录，不伪造。
- 测试使用测试配置/测试数据；不污染用户真实数据；设备状态可恢复。

### 10.2 逐用例判定

- **PASS**：期望与实际一致，原始证据完整可复现。
- **FAIL**：期望与实际不一致；附最小复现、时间点、日志/UI 证据。
- **NOT EXECUTED**：前置缺失（工具/设备/网关/授权/实现未提供/无法构造）；**不得记为通过**。
- **CONDITIONAL**：判据依赖未确认语义（如实现未定义“文本相同”的比较范围）；记录实测，保留未满足。
- **UNKNOWN CAUSE**：异常但根因未定位；保留，不归因外部条件。

### 10.3 任务级判定

| 任务 | 通过条件 |
|---|---|
| ITEM-001 | N1-H-01、N1-S-01…03、N1-D-01…10 全部 PASS；任一必需项 FAIL/NOT EXECUTED/UNKNOWN → **不通过** |
| ITEM-002 | N2-H-01…11、N2-S-01/02、N2-D-01/02/05/06、REG 相关项 PASS；N2-D-03/04 在真实源可得时必须 PASS，缺失记 NOT EXECUTED 并披露（任务文本为“可得条件下”）；不得宣称真实多设备/9 台已通过 |
| ITEM-003 | N3-H-01…08、N3-S-01/02、N3-D-01/02/03 PASS；N3-D-04/05 为条件/既有证据项，缺失记 NOT EXECUTED 并披露；不得把零设备提示缺失记为通过 |

### 10.4 阶段判定（不得豁免）

1. 3 项任务均按 §10.3 通过，才能得出“本轮 Android 增量阶段通过（限定范围）”。
2. 任一必需项 FAIL → 返回 **TEST_FAIL**（`failure_cause=MEASURED_DEFECT`），附证据交实现；不得以“其他用例通过”抵消。
3. 任一必需项因外部条件未执行 → 返回 **TEST_FAIL**（`failure_cause=EXTERNAL_CONDITION_UNEXECUTED`）或在该能力契约允许时如实登记 NOT EXECUTED；**不得写成 PASS**。
4. **阶段通过 ≠ 产品通过**：真实升温/超温、光强双态、功耗、跨链路、平台写入/回读、API 31+、9 台真实快照、注册/登录、有数据列表、真实告警等保持 NOT EXECUTED，必须在报告中显式披露。
5. **人工延期项 `FW-LIGHT-DEF2-ENABLE` 不计入本阶段必需项**，不得重排、不得作为返工或整体失败依据；其状态始终为 NOT EXECUTED。
6. 原因未知不等于外部条件缺失；不得据此改写目标或提前终止。`BLOCKED` 只能由 Runtime 在 loop 上限时产生，本设计不使用。
7. 本设计自身只产生设计与判定规则；**不因设计完成宣称任何用例通过**。

### 10.5 缺陷归属

- 扫描权限结果/自动续跑/文案/销毁 → Android 实现。
- 读取终态映射/接线/首页状态 → Android 实现（BLE 帧布局/网关固件问题另交嵌入式/联调）。
- 报告页反馈/WorkManager 接线 → Android 实现。
- 网关真实读取异常 → 先区分 Android 终态显示与网关/链路；不把未定位原因归为“外部条件”。
- 温度/光判据/433/UART1 → 固件/嵌入式（阶段外）。
- 电气/功耗/光路 → 硬件测试（阶段外）。
- 测试工具/依赖/夹具 → 本能力按 test 范围有限修复；缺授权的真机/设备条件如实登记。

---

## 11. 能力边界与停止条件

本能力在写出本文件、更新 `file_manifest.txt` 后结束，返回 `DESIGN_READY`。以下均不属本能力：执行真机/网关测试、修改产品源码、实现 3 项修复、修改固件/网关/服务器、调温/调光/遮光/测功耗、重跑产品级验收、修改权限或环境配置、创建 E2E 联调节点。发现需改动时按 §9/§10.5 登记交接后停止在能力边界。

**本阶段不产出独立 E2EPlan**：用户已明确 433→网关→BLE→手机→平台跨链路属后续联调阶段；本设计的真实网关读取路径（N2-D-01/D-02）只覆盖 Android 读取终态，不冒充跨端联调或完整产品验收。

---

## 12. 自检

- [x] 人工范围决定已核对：`FW-LIGHT-DEF2-ENABLE` NOT EXECUTED、不重测/不重排/不作为返工依据；`BOT-01` 已闭合不重复。
- [x] 原检查者 R4 范围错误已指出并采用纠正后的分解；未把人工延期项或真实温度触发列入本轮 Android 验收前提。
- [x] ITEM-001/002/003 的每条验收条件均映射到用例与证据类别（§3.2）。
- [x] 正常路径（授权后自动扫描、有界停止、有效批次、零设备提示、成功刷新）与异常路径（蓝牙关闭、定位关闭、拒绝、不再询问、销毁、重复点击、长度不足/非对齐、解密失败、未知输入、无数据、失败）均有设计。
- [x] 宿主/静态/真机/真实网关证据分开，逐项注明不能证明的真实链路。
- [x] 未安排调温、改变光强、遮光或功耗测试；未把阶段外项塞入本阶段退出。
- [x] 跳项与阶段外项保留为 NOT EXECUTED，并有未来恢复条件；未写成通过。
- [x] 明确“阶段通过 ≠ 产品通过”，并保留当前产品级 `TEST_FAIL` 的性质。
- [x] 未修改产品源码、固件、网关、服务、文档或权限/环境配置；未执行真机操作。
- [x] 新候选身份与安装一致性列入 REG-06；旧 261/0 结果只作修复前基线，不替代新候选运行。
