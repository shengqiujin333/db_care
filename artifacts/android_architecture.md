# Android App 方案设计（AA-006）

状态：Android 方案设计（供实现与测试消费；`DESIGN_READY` 仅表示 3 项增量可供实现，**不代表产品验收通过**）
版本：5.0（替代 AA-005 rev 4.0；AA-005 七项增量与后续修复均已交付，本版按当前人工范围决定与实测证据重定范围）
责任角色：Android 工程
范围：仅 Android App（`dengbei_care`，Kotlin）。不含固件、网关、服务器、iOS、硬件与随包手册内容。
工程基线：Git `ada5772055590892106bfe4f0a724d86350c946b`；Android 主源码自 `db53202` 起零差异（`git diff db53202..HEAD -- dengbei_care/app/src/main dengbei_care/app/build.gradle.kts` 为空）。
检查时点：2026-11-13（Asia/Shanghai）。

本版工作方式：只读源码、工件与既有证据；**未连接手机/网关、未安装、未构建、未运行测试、未写数据**。本能力无设备工具入口（`android_app_testing`、`android_readonly_inspection` 等对本能力不可用）。

输入来源（本版实际读取）：
- `readme.txt`（原始需求 1–8 与补充条件；当前不能调温/改变光强/测功耗）
- `artifacts/system_analysis_current.md`、`artifacts/interface_contract.md`（IC-002 v4.0）
- `artifacts/android_implementation.md`（AA-005 七项、D-AA005-1、D-01/D-02/D-03/D-04a 修复与重建）
- `artifacts/android_build.md`（AB-002：候选 APK `71a041d3…`，与独立复验安装字节一致）
- `evidence/android_check_20261111/SOFTWARE-TEST-ANDROID-RECHECK-001.md`（真机安装与 D-01…D-04a 回归 `TEST_PASS`，限定范围）
- `evidence/product_acceptance_20261112_scope_recheck/PRODUCT-ACCEPTANCE-RECHECK-002.md`、`SOFTWARE-TRIAGE-003.md`（产品级 `TEST_FAIL=EXTERNAL_CONDITION_UNEXECUTED`；Android 无实测缺陷；O-1…O-3 观察）
- `evidence/product_acceptance_20261111/SOFTWARE-TRIAGE-001.md`、`PRODUCT-ACCEPTANCE-001.md`（D-01…D-04a 的原始失败与修复依据）
- `evidence/aa005_android_recheck_20261110/…`、`evidence/software_test.md` ST-AA005-R2（真机复验与 O-AA005-1/2/3/5/6 观察）
- `artifacts/failure_resolution_product_acceptance_scope_recheck.md`（FR-005）与 `artifacts/failure_resolution_product_stage_delivery_gate.md`（FR-006，当前阶段/门禁缺口）
- 当前 Android 源码（重点：`MqtttService`、`NotificationsFragment`、`GatewayBleSettingsDialog`、`ReportActivity`、`BlePermissionPolicy`、`ReportGenerator`）
- 人工范围决定（Runtime 注入）：`FW-LIGHT-DEF2-ENABLE = APPROVED_DEFERRED`；`BOT-01 = REASSESS（已执行 PASS）`
- `file_manifest.txt`、最近提交与工作区状态

---

## 0. 本版变更与范围结论（优先于以下历史描述）

1. **AA-005 与后续修复均已交付，且在已执行范围内无 Android 实测缺陷**：
   - 七项增量（G-1…G-7）、D-AA005-1（401/403 全局暂停）、D-01/D-02（底部入口可达）、D-03（控件无重叠）、D-04a（首页文案）均已实现并通过宿主机与真机回归；
   - 候选 APK `71a041d3…`（10,365,464 B）与设备安装字节一致；独立复验 `TEST_PASS`（限定于安装、页面/入口/回归范围）。
2. **人工范围决定复核（不改写、不重排）**：
   - `FW-LIGHT-DEF2-ENABLE`：`APPROVED_DEFERRED` / NOT EXECUTED（2026-11-11T03:50:42Z，「无法准备，先跳过」）。属固件/需求判据接受事项，**不是 Android 返工依据**，本版不重复询问、不据此定义 Android 任务；
   - `BOT-01`：`REASSESS` 后已由测试侧以自动方式执行并 PASS；不再重复。
3. **原检查者范围错误（已由复核轮纠正，本设计确认）**：`PRODUCT-ACCEPTANCE-002` §5 R4 把“无光判据未启用（人工延期）”标为“未满足/未执行”，并把它作为整体 `TEST_FAIL` 依据之一，与同报告 §6 N-1 的 `SKIP_CONFIRMED` 记录矛盾。正确分解为：
   - 上报公式实现 = 已实现且静态一致；
   - 无光判据启用 = NOT EXECUTED（人工 `APPROVED_DEFERRED`）；
   - 真实升温/超温触发 = NOT EXECUTED（温度条件，用户明确当前不能调温）。
   `PRODUCT-ACCEPTANCE-RECHECK-002` 已完成该重评；本版不把该项计入 Android 缺陷或任务。
4. **当前产品级失败的性质**：最新 `PRODUCT-ACCEPTANCE-RECHECK-002` 仍 `TEST_FAIL`（`EXTERNAL_CONDITION_UNEXECUTED`）。其依据是真实升温/超温、网关→手机跨链路、平台 `/upload_data` 写入/回读、功耗、Android 真实 BLE/上传/注册/列表/API 31+ 等未执行项；**不是 Android 实现缺陷**。阶段/交付门禁与联调节点仍待有权限方修订（FR-005/FR-006）。
5. **本版新增设计范围**：只处理 3 项**已实测、可在无额外外部条件下实现与宿主机局部断言的真实 Android 行为缺陷**（来自测试观察，非验收阻断项）：
   - N-1 首次授予扫描权限后待执行扫描不自动开始；权限拒绝/蓝牙关闭时提示原因不精确（O-AA005-5、O-AA005-1）；
   - N-2 读取成功但 `devCount=0`/帧被拒后，首页链路状态停留在“正在连接网关…”，缺少终态（O-AA005-6）；
   - N-3 报告页零数据时点击「重新生成」无可感知反馈（RECHECK-002 O-2）。
6. **明确不做**：不设计固件判据启用、不设计调温/调光/功耗测试、不设计真实 BLE/平台联调节点、不修改随包手册 `docs/`（D-04b 属技术文档角色）、不改变已批准协议/表结构/上传契约。
7. **DESIGN_READY 边界**：仅表示 N-1…N-3 的方案可供实现；产品总验收仍受外部条件/阶段门禁限制，不能由本设计或后续实现宣称通过。

---

## 1. 当前已交付与已验证状态（as-built）

| 能力 | 状态 | 证据 |
|---|---|---|
| BLE 权限策略（API<31 直连不要求定位；API31+ CONNECT/SCAN） | 已实现、真机部分验证 | `BlePermissionPolicy`；ST-AA005-R2 真机 API29 直连门（蓝牙关闭阻断、定位未授予不阻断） |
| 网关 BLE 身份发现/保存/清除、有界扫描 | 已实现、真机部分验证 | 真机候选 `xxx 84:C2:E4:03:02:02 RSSI=-49 FFE0`、10.016 s 停止、保存/清除不回退 |
| BLE 批次转发 `time:temps:humis`、拒绝不产部分载荷 | 已实现、宿主验证 | `BleBatchForwarding` 单测；真实多设备 NOT EXECUTED |
| 占位跳过与合法 `0.0/0.0` | 已实现、宿主验证 | `BleObservation`；真实占位/0.0 源 NOT EXECUTED |
| 上传分类、401/403/4xx 全局暂停、手动重试 | 已实现、宿主验证 | AA-005 回归 + D-AA005-1 独立用例 |
| 绑定容量 ≤9、MTU/返回长度完整性 | 已实现、真机部分验证 | 真机 MTU 240、FFE2 160 B、`devCount=0` 拒绝；9 台 NOT EXECUTED |
| 规则文案/时间注释/首页路径文案/底部入口/控件无重叠 | 已实现、真机验证 | 独立复验 `TEST_PASS`（限定范围） |
| 真实跨链路、平台写入/回读、真实触发、功耗、API31+、9 台 | **NOT EXECUTED**（外部条件/后续阶段） | RECHECK-002/FR-005/FR-006；不得写成通过 |

候选身份：`versionCode 10 / versionName 1.9`，debug，APK SHA-256 `71a041d322368c20a181d5716c44ac4d1b2c8a3869926b0047cd96b61e2c10d8`（AB-002，与被测安装字节一致）。

---

## 2. 需求到 Android 的落点（原始需求核对）

| 需求 | Android 落点 | 当前状态 |
|---|---|---|
| 1 GXHT40 | 单位/BLE 布局不变，不重算 | 已实现 |
| 2 光敏分压 | 光照不上空口/BLE；App 不推断/不显示 | 已实现 |
| 3 180 s 采样 | 读取节拍/文案 180 s；无数据不判异常 | 已实现 |
| 4 升温 >0.9℃ 且无光 / >35℃ | App 不重算门控；规则文案正确 | 已实现；真实触发 NOT EXECUTED（温度条件）；判据启用人工延期（非 Android） |
| 5 broker/服务器 | 既有地址/证书/topic/upload 复用 | 已实现；真实写入/回读 NOT EXECUTED（无授权/网络） |
| 6 只改 Android、不改 iOS | 仅 Android；光/霍尔不引入 | 已实现 |
| 7 当前有光 | App 不显示光照/不补偿 | 已实现 |
| 8 UART1 | 不属 Android | 固件域 |
| 环境：网关由 App 发现 | 有界扫描/保存/清除/默认迁移 | 已实现；真实广播/配对已在真机部分验证 |
| 环境：真机 API29 | 权限按 API 分流 | 已实现、真机部分验证 |
| 补充：无调温/改光/功耗 | 不安排相关硬件测试 | 遵守 |
| 补充：跨链路后续联调 | 记录为后续阶段，不作 Android 返工 | 遵守 |

---

## 3. 人工范围决定与原检查者范围错误（本版重评）

| 项 | 记录 | 本版处置 |
|---|---|---|
| `FW-LIGHT-DEF2-ENABLE` | `APPROVED_DEFERRED` / NOT EXECUTED（`HR-8A11935AD0A7`） | 不作为返工依据；不设计固件/Android 变更；在交付中继续明示 NOT EXECUTED |
| `BOT-01` | `REASSESS`；测试侧自动执行并 PASS（2026-11-12） | 已闭合；不重复安排 |
| 原 `PRODUCT-ACCEPTANCE-002` R4 | 将“无光判据未启用（延期）”标为“未满足”并计入整体失败 | 范围错误；正确分类见 §0.3；`RECHECK-002` 已纠正，本设计不据该项定义任务 |
| 当前 `RECHECK-002` | `TEST_FAIL`，`EXTERNAL_CONDITION_UNEXECUTED` | 保留；失败依据只含外部条件/阶段项，不是 Android 实现缺陷 |

---

## 4. 本版设计增量（可独立检验）

### N-1 扫描权限结果交付与自动续跑（O-AA005-5、O-AA005-1）

**现状（实测）**：API<31 首次点击「搜索附近网关」弹出定位权限；授权后扫描不自动开始，需再次点击；蓝牙关闭时授权后提示“未授予扫描权限”，未说明真实原因。
**机制（源码）**：`GatewayBleSettingsDialog.ensureScanPermission` 经 `NotificationsFragment.requestPermissionsCompat` 调用 `ActivityCompat.requestPermissions(requireActivity(), …)`；结果回调按 Activity 的 FragmentManager 分发，`NotificationsFragment` 位于 NavHost 子 FragmentManager，`pendingScan` 可能收不到 `onRequestPermissionsResult`。`onPermissionResult` 使用 `hasScanPermission()`（含适配器状态）判断，无法区分“未授权/定位关闭/蓝牙关闭”。
**目标行为**：
1. 权限请求必须由 `NotificationsFragment` 使用 `registerForActivityResult(ActivityResultContracts.RequestMultiplePermissions())` 发起，结果 100% 回到 fragment（生命周期安全）；
2. 授权后立即重新评估 `BlePermissionPolicy.evaluate(needScan=true)`：`canScan=true` → 执行挂起的扫描动作；否则用 `policy.reason/userMessage` 给出真实原因（`BLUETOOTH_OFF`/`LOCATION_OFF`/权限缺失），不得统一提示“未授予扫描权限”；
3. 拒绝/不再询问：清除 `pendingScan`，提示具体缺失项，不自动重复请求；用户再次点击才重新请求；
4. 页面销毁/对话框关闭时清除挂起动作与结果回调，不泄漏、不迟发扫描；
5. 仅用户显式点击搜索/扩大搜索时请求权限，启动 App 不请求定位。
**涉及文件**：`ui/notifications/NotificationsFragment.kt`、`ui/gateway/GatewayBleSettingsDialog.kt`（必要时 `ble/BlePermissionPolicy.kt` 增加结果描述纯函数）。
**状态机**：`IDLE → PERMISSION_REQUESTING → (granted) POLICY_RECHECK → SCANNING | REASON_SHOWN`；`→ (denied) REASON_SHOWN → IDLE`；对话框销毁 → `IDLE`。
**正常/异常目标**：授权且蓝牙/定位可用 → 一次扫描自动开始并有界 10 s 停止；授权但蓝牙关闭 → “蓝牙未开启（去开启）”，不启动扫描；定位关闭 → “搜索网关需要开启定位”；拒绝 → 明确缺失权限且无扫描；重复点击不产生并行扫描。
**验证条件**：真机 Redmi 8A/API29（软件测试）；宿主机静态核查（无 `ActivityCompat.requestPermissions` 从对话框路径、存在 `registerForActivityResult`）。

### N-2 读取尝试的终态链路状态（O-AA005-6）

**现状（实测）**：真实读取成功、MTU 240、FFE2 160 B、`devCount=0` → `NO_DEVICE` 拒绝后，首页状态仍停在“正在连接网关…”，直到下一轮才可能改变。
**机制（源码）**：`processBleSamples` 只为有效/占位批次设置 `DATA_OK/GATEWAY_NO_DATA`；`onCharacteristicRead` 的 `Rejected(NO_DEVICE)` 等分支只写日志，不设置终态。
**目标行为**：每次读取尝试结束必须落到可区分终态，不得停留在 `CONNECTING`：
| 结果 | 状态码 | 用户文案 |
|---|---|---|
| 有效读数（含混合占位，至少 1 条） | `DATA_OK` | 网关已连接（最近读取 HH:mm） |
| 全占位 | `GATEWAY_NO_DATA` | 网关暂无传感器数据（等待传感器上报） |
| `NO_DEVICE`（devCount=0） | `GATEWAY_NO_DEVICE` | 网关已连接，暂无设备（N=0） |
| 长度不足/非 16 对齐 | `READ_INCOMPLETE` | 读取不完整（MTU/长度不足） |
| 解密失败/帧头不足 | `READ_INVALID` | 网关已连接，数据无效（原因） |
未知/其他拒绝 | `READ_INVALID` | 同上（带原因） |
**涉及文件**：`protocol/GatewayReadState.kt`（新增纯逻辑映射：RejectionReason/LengthVerdict → 状态码与文案类别）、`MqtttService.kt`（在 `Rejected`/`Success` 分支统一设置终态）。
**正常/异常目标**：正常有效批次 → DATA_OK；全占位 → GATEWAY_NO_DATA；真实 devCount=0 → GATEWAY_NO_DEVICE 且不再显示 CONNECTING；长度/解密失败 → 对应终态；同一轮不被后续占位覆盖为错误状态。
**验证条件**：宿主机映射矩阵；真机可用真实网关 `devCount=0` 复现（测试能力）。

### N-3 报告页“重新生成”的可感知反馈（RECHECK-002 O-2）

**现状（实测）**：零设备/无读数时点击「重新生成」后，页面内容无变化、无提示，用户无法判断操作是否生效。
**目标行为**：
1. 点击后立即显示进度并禁用按钮，防止重复触发；
2. Worker 成功但无可生成数据（无设备/无当日读数/生成文本为空或与上次相同）→ Toast/Snackbar“暂无可生成的数据（尚无设备读数）”，恢复按钮；
3. Worker 失败 → “报告生成失败，请稍后重试”，恢复按钮；
4. 有数据成功 → 刷新报告文本与生成时间，恢复按钮；
5. 不改变报告内容/统计规则与既有 `KEY_REPORT_TEXT/KEY_REPORT_TIME` 契约。
**涉及文件**：`report/ReportActivity.kt`、`report/ReportRegenerateFeedback.kt`（新增纯逻辑判定：输入 succeeded/hasContent/deviceCount/sampleCount → 消息类别）、`res/values/strings.xml`（文案）。
**正常/异常目标**：零设备点击有明确提示；无当日数据有提示；成功刷新；失败提示；连续点击不并行触发；无崩溃。
**验证条件**：宿主机纯逻辑 + 真机零设备场景（当前设备已满足）。

---

## 5. 设计决策（新增）

| ID | 决策 |
|---|---|
| D-35 | 扫描权限请求改由 Fragment 的 `ActivityResultLauncher` 发起并消费结果；对话框只保留“发起请求 + 消费结果”的回调接口，不直接调用 `ActivityCompat.requestPermissions(Activity)` |
| D-36 | 权限结果后的判定必须使用 `BlePermissionPolicy` 的 `reason/userMessage`（含适配器/定位状态），不把 `canScan=false` 一律解释为“未授权” |
| D-37 | 每次 BLE 读取尝试必须设置终态（`DATA_OK/GATEWAY_NO_DATA/GATEWAY_NO_DEVICE/READ_INCOMPLETE/READ_INVALID`），状态映射集中在纯逻辑模块，`CONNECTING` 不得跨读取结果残留 |
| D-38 | 报告页反馈以“用户可感知、可重复点击但串行”为准；无数据不是错误，但必须有非静默提示；不改报告内容/统计与存储键 |
| D-39 | 3 项增量均不改 BLE 布局/AES/topic/`/upload_data`/SQLite/告警语义；版本保持 `10 / 1.9`，交付以 APK 摘要区分（与既有迭代一致） |
| D-40 | 外部条件项（真实 BLE/MTU/多设备/9 台、平台写入/回读/401、API31+、真实触发/光强/功耗）不由 Android 实现虚假关闭；保持 NOT EXECUTED 并记明责任阶段 |

沿用决策：D-01…D-34（BLE 布局、纯逻辑可测、不重算门控、上传分类、容量/完整性、文案与观测语义等）不变。

---

## 6. 兼容面（不得改动）

BLE 记录布局/字节序/符号、AES/ECB 与密钥、GATT UUID、`0xA1` 头与分块、`gateway_ble_addr`/`mac_addr` 语义、topic 与 `time:temps:humis`、`/upload_data` 字段与单位、SQLite v3 表结构与 `(dev_id,time)` 键、告警阈值/去抖/文案、180 s 节拍、报告统计规则与 `MyPrefs` 既有键。仅新增状态/反馈与权限请求接线。

---

## 7. 验证策略

### 7.1 宿主机（实现阶段必须给出）

| 目标 | 断言 |
|---|---|
| N-1 | 静态：NotificationsFragment 使用 `registerForActivityResult`；对话框路径无 `ActivityCompat.requestPermissions(Activity)`；拒绝/蓝牙关闭/定位关闭文案来自 policy；页面销毁清理挂起 |
| N-2 | `GatewayReadState` 映射矩阵：有效/全占位/NO_DEVICE/长度不足/非对齐/解密失败/未知 → 状态码与文案类别；不残留 CONNECTING |
| N-3 | `ReportRegenerateFeedback` 矩阵：零设备、无数据、成功、失败、重复点击；消息与按钮恢复决策 |
| 回归 | 现有 261 项宿主机用例全通过；`assembleDebug` 成功；无新增告警 |

### 7.2 真机（测试能力执行）

1. API29 首次扫描授权后自动开始（≤3 s 内出现 `scan started`），10 s 停止；拒绝/蓝牙关闭/定位关闭分别显示正确原因；无重复弹窗。
2. 真实网关 `devCount=0` 读取后状态变为“网关已连接，暂无设备”，不再停留 CONNECTING；后续有效/占位批次状态正确。
3. 零设备/无当日数据点击「重新生成」出现明确提示并恢复按钮；有数据时刷新；连续点击不并行；无崩溃/ANR。
4. 回归：首页/我的/设定、报告入口、传感器列表入口、网关对话框、守护启停、冷启动。

### 7.3 不在本设计验证范围

真实升温/超温、真实 433→网关→BLE→手机同笔、平台 `/upload_data` 写入/回读/401、功耗、API31+、9 台真实快照、`docs/` 随包手册 D-04b。保持 NOT EXECUTED 或相应责任角色；本设计不以模拟替代。

---

## 8. 未决与交接

| ID | 内容 | 归属/处置 |
|---|---|---|
| U-1 | 真实 BLE 扫描/GATT/MTU/多设备/9 台容量 | 软件/嵌入式联调阶段；当前 NOT EXECUTED |
| U-2 | 平台 `/upload_data` 写入、幂等、401/403、持久化回读 | 端到端/平台阶段；无写/读授权 |
| U-3 | 真实升温/超温触发、光强双态、功耗 | 外部条件；用户明确当前不能调温/改光/测功耗，不安排 |
| U-4 | `FW-LIGHT-DEF2-ENABLE` | 人工 `APPROVED_DEFERRED`；固件/需求域，不属 Android |
| U-5 | `BOT-01` | 已 REASSESS 并执行 PASS；历史保留 |
| U-6 | 产品级验收/交付门禁、联调节点、平台写授权 | FR-005/FR-006 `REPLAN_REQUIRED`，验收范围所有者 |
| U-7 | `docs/` 随包手册 D-04b（“无 UI 路径”等旧表述） | 技术文档角色；Android 侧说明书已同步 |
| U-8 | 原 `PRODUCT-ACCEPTANCE-002` R4 范围错误 | 已由 RECHECK-002 纠正；本设计确认，不作返工依据 |
| U-9 | API 31+ 权限分支、扫描权限结果自动续跑的真机行为 | 测试能力；本版 N-1 目标 |
| U-10 | 观察 O-AA005-2（API31+ canScan 同时要求 CONNECT 的规范张力） | 记录，不改变当前策略；如需调整需上游确认 |
| U-11 | 传感器列表“有数据时”的列出/删除 | 无真实数据，NOT EXECUTED；不因本设计写成通过 |

---

## 9. 自检

- [x] 复核人工决定：跳过项非 Android 返工依据、BOT-01 已闭合、未重复询问
- [x] 指出原检查者 R4 范围错误并采用 RECHECK-002 纠正后的分解
- [x] 未把产品级 `TEST_FAIL` 说成 Android 缺陷或已通过
- [x] 新增 3 项均有独立状态/接口/预期行为、正常/异常目标与验证条件
- [x] 未设计固件判据启用、调温/调光/功耗或跨链路段
- [x] 未改协议/表结构/上传契约/版本；未操作设备或构建
- [x] 外部条件与未执行项保留，并明确责任阶段
- [x] 未修改其他角色工件（`docs/` 交技术文档）

任务清单见 `artifacts/android_tasks.yaml`（3 项：N-1、N-2、N-3）。
