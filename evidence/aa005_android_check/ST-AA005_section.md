---

## ST-AA005 · Android AA-005 七项增量与当前候选整体核验 —— 结论 **TEST_FAIL**（缺陷 D-AA005-1）

**被测对象**：Android App `dengbei_care`，实现提交 `7848b99`（android_engineer）；核验时 HEAD `f77873d`；`git diff 7848b99..HEAD -- dengbei_care` 为空（产品树与实现提交一致）。
**版本/产物**：`versionCode 10 / versionName 1.9`（`app/build.gradle.kts`、AGP output-metadata、合并清单、APK 二进制清单四级一致）；`app-debug.apk` 10,365,236 B，SHA-256 `bddc698a89482e3d6f4752bb0d0fd6a58e34f9c63e93bc84307b7d41ef755b47`。
**输入**：`artifacts/android_implementation.md`（AA-005 七项实施记录）、`artifacts/android_tasks.yaml`（ITEM-001…007 验收）、`artifacts/android_architecture.md`（AA-005 rev 4.0）、`artifacts/interface_contract.md`（IC-002 v4.0 §6/§7）、`artifacts/software_test_design.md`（TD-SW-004 rev 4.0 §4.1 L0-27…L0-42、§4.3 L1-25…L1-33）。
**目标环境**：Redmi 8A / Android 10 / API 29 / 序列号 `12f046690506`；核验期间蓝牙关闭、无默认网络；App 由 `android_test` 安装当前 APK；网关未确认上电/广播；手机无 API 31+ 对照设备。
**设备状态基线（核验前）**：App 未安装；`bluetooth_on=0`；`Active default network: none`；`ACCESS_FINE_LOCATION granted=false`。
**设备状态收尾**：守护已停止（按钮回到“启动”）；`gateway_ble_addr` 已清除（续跑复核 2026-11-10T13:25Z：设置对话框“未配置网关地址”、首页链路条“未配置网关地址（搜索附近网关）”，`ui_15`/`ui_16`）；蓝牙/网络保持关闭；App 保持安装；**扫描权限测试中一次性授予的定位权限仍为 granted=true（未撤销）**，原因见 §6。

### 1. 独立执行摘要

| 层 | 方法 | 结果 |
|---|---|---|
| 宿主机独立验证 | 新增 `verification/Aa005SevenItemVerificationTest.kt`（20 项，期望值按任务/IC-002/TD-SW-004 独立重写） | **19 PASS / 1 FAIL**（D-AA005-1） |
| 宿主机全量回归 | `:app:testDebugUnitTest --rerun-tasks`（31 个既有套件 + 本轮独立 20 项） | **255 tests / 1 failure / 0 error / 0 skip**（唯一失败为本轮缺陷门禁） |
| 独立静态核查 | 新增 `evidence/software_verify_aa005_android_check.py`（54 项，独立于实现侧 `static_checks.py`） | **53 PASS + 1 FINDING（E6，与 D-AA005-1 同源）** |
| 既有独立门禁 | 11 个历史门禁 + 本轮新门禁（`evidence/aa005_android_check/run_gates.py`） | **12/12 EXIT=0**（5 个门禁按 AA-005 已批准变更更新，见 §7） |
| 构建 | `:app:assembleDebug --offline` | **BUILD SUCCESSFUL**；APK 摘要同上 |
| 真机功能 | 安装、启动、隐私协议、首页、守护启停、链路状态、网关设置（默认标注/手输/非法拒绝/保存/清除）、扫描权限请求 | 已执行并留证；详见 §3 |
| 真实 BLE/网关/服务链路 | 真实扫描、GATT、MTU、FFE2 返回字节、多设备批次、占位、9/10 台容量、真实上传/401/回读 | **未执行**（外部条件，见 §6） |

### 2. 逐项独立验证结果（宿主机）

新建 `dengbei_care/app/src/test/java/com/jinyuni/dengbei_care/verification/Aa005SevenItemVerificationTest.kt`，不引用实现侧自检的期望常量；JUnit XML 见 `evidence/aa005_android_check/TEST-com.jinyuni.dengbei_care.verification.Aa005SevenItemVerificationTest.xml`。

| 门禁 | 覆盖 | 结果 |
|---|---|---|
| `g1_directConnectMatrix_matchesIndependentSpec` | SDK 26/29/30/31/33 × 6 组授予 × 适配器开/关 × 定位开/关；直连不因定位权限/开关阻断；`missing` 仅含 CONNECT；reason 优先级 | PASS |
| `g1_scanMatrix_matchesIndependentSpec_forApiBelow31` | API<31 扫描：`adapter && FINE_LOCATION && locationEnabled`；`requiresLocationServices=true`；reason 分类 | PASS |
| `g1_scanMatrix_matchesIndependentSpec_forApi31Plus` | API31+ 扫描：定位状态无关；无 SCAN 必拒绝；适配器关闭必拒绝；SCAN+CONNECT 全授予必可扫描 | PASS |
| `g1_permissionRequestSets_byApi` | 直连/扫描请求集合按 API；通知权限仅 API≥33 | PASS |
| `g1_userMessages_areSpecificAndActionable` | 每种失败原因的消息含具体原因与动作 | PASS |
| `g2_normalize_acceptsDocumentedForms_andRejectsInvalid` | 1–2 位段/冒号/短横/12 位连写规范化；非法长度/字符/空段/首尾分隔符拒绝 | PASS |
| `g2_rank_dedupesAndOrders` | 非法 MAC 丢弃；同 MAC 去重（FFE0 取或、RSSI 取强）；FFE0 优先 → RSSI 降序 → MAC 升序；不改入参 | PASS |
| `g2_bindingStates_andResolveAddress` | 未配置/迁移默认“未确认”/已保存三态；清除后绝不回退默认 | PASS |
| `g3_singleDevicePayloadIsExact_andParsesBack` | `1700000000:25.5:60.0` 逐字；可被 `ReadingAttribution` 解析回读 | PASS |
| `g3_localeIndependent_andNegativeTemperature` | US/CN/DE/FR 四种 Locale；负温保号；无空格 | PASS |
| `g3_multiDeviceOrder_andRefusalBranches` | 多设备按绑定顺序；NO_BOUND/COUNT/DUPLICATE/UNKNOWN/ORDER/ALL_PLACEHOLDER 全分支；拒绝结果不带载荷 | PASS |
| `g5_placeholderAndValidZero` | 8 B 全零占位；有效 ID 的 0.0/0.0 为合法读数；非全零不误判 | PASS |
| `g5_codecX10FieldsAreAdditive_andLayoutUnchanged` | 偏移/字节序/符号不变；x10 整数字段 additive（H=600、T=-50） | PASS |
| `g6_uploadResultClassificationMatrix` | 2xx/401/403/其余 4xx/408/429/5xx/null/3xx/异常分类 | PASS |
| `g6_authRejected_blocksTheRejectedBatch_andKeepsAllRows` | 同批 401 → 两行 park、保留、不重试同批 | PASS |
| **`g6_authRejected_mustPauseAllAutomaticRequests_untilManualRetry`** | **全局“无自动重试”**：401 后其余到期行不得自动上传 | **FAIL（D-AA005-1）** |
| `g6_manualRetry_restoresParkedRows_andSucceeds` | 手动重试恢复被暂停行并成功删除 | PASS |
| `g7_capacity_expectedSize_andMtu` | 0..9 Ok / 10..255 Exceeded；1..9 帧长 32..160；与 codec 公式 0..255 全等；MTU 边界 34/35、162/163 | PASS |
| `g7_lengthVerdict_alignThenCoverage` | 先对齐后覆盖；不足/非对齐/正常/尾部 16 B 对齐 | PASS |
| `g4_frozenRuleText_andBannedWords` | 冻结文案逐字；无“较上次下降超过 0.9”；无 lux/照度/光照/勒克斯；>35.0 严格；24 h | PASS |

### 3. 真机功能核验（Android 10 / API 29）

原始证据目录：`evidence/aa005_android_check/`（UI 树 XML、截图、logcat、安装结果、权限/版本快照）。

| 步骤 | 实际观察 | 结论 |
|---|---|---|
| 安装当前 APK | `android_test install` 处理安装确认；包 `com.jinyuni.dengbei_care`、label 爱守护、`version_code=10`、`version_name=1.9`、sha256 `bddc698a…` 与构建产物一致（`android_install.json`） | PASS（安装成功≠业务通过） |
| 首次启动 | 隐私协议对话框（`用户协议与隐私政策`、同意并继续）；同意后进入首页；无崩溃/ANR（logcat 无 `AndroidRuntime:E`） | PASS（启动路径） |
| 首页 + 守护 | 空设备首页；点“启动”后按钮变“停止”，前台服务通知“爱守护正在运行”；点“停止”后恢复“启动” | PASS（UI 状态） |
| 权限策略（API 29 直连） | `dumpsys package` 显示 `ACCESS_FINE_LOCATION: granted=false`；保存网关触发立即读取后日志：`BLE connect blocked: reason=BLUETOOTH_OFF sdkInt=29 missing=[] needScan=false adapterEnabled=false` | PASS（**直连不要求定位权限**；本轮因蓝牙关闭止于适配器检查，未发起 GATT） |
| 链路状态可见 | 首页 `linkStatusText` 显示“蓝牙未开启（去开启）”（`ui_09`、`ui_14`）；不再静默返回 | PASS |
| 读取节拍 | logcat 六次连接尝试：19:29:05、19:32:05、19:35:05、19:38:05、19:41:05、19:44:05（间隔恰 180 s） | PASS（节拍门控；非真实 BLE 读取） |
| 网关设置默认态 | 对话框标题“网关与上传设置”；状态“正在使用默认网关地址（未确认）：84:C2:E4:03:02:02”；平台身份 `mac_addr` 单列 | PASS |
| 非法手输 | 输入 `ZZ:11:22` → 保存后仍显示默认地址、输入保留、出现 toast；未写入 | PASS |
| 合法手输/规范化 | 输入小写短横 `aa-bb-cc-dd-ee-01` → 保存后 `ui_07` 显示“BLE 连接目标：AA:BB:CC:DD:EE:01”、输入框 `AA:BB:CC:DD:EE:01`，`mac_addr` 保持原值（该步保存日志未单独留存）；另有一次清除后复测保存 `84:C2:E4:03:02:99`，日志 `gateway_ble_addr saved=84:C2:E4:03:02:99 (platform mac_addr untouched)`（`logcat_save_clear.txt`）与 `ui_13` 一致 | PASS |
| 清除 | 清除后状态“未配置网关地址”，保存值空、清除标记写入；无回退默认 | PASS |
| 扫描权限（API<31） | 仅点击“搜索附近网关”时弹出系统定位权限框（“获取设备的定位信息”/爱守护）；**启动与保存网关均不弹定位框**；拒绝后未启动扫描；第二次点击显示“拒绝且不再询问”（仅用户操作时请求，不自动重复） | PASS（请求时机/范围） |
| 扫描结果 | 蓝牙关闭时无 `GatewayDiscovery` 扫描日志、无候选；未执行真实 BLE 扫描 | 未执行（外部条件） |
| 上传状态区 | 显示“云端上传状态：无（待发 0 条）”与“重试上传”按钮 | PASS（呈现） |
| 网络/服务 | 手机无默认网络；App 周期性尝试 `8.140.23.253:8883` 均以 `ENETUNREACH` 失败；无上传请求发出 | 未执行（外部条件） |
| 详情页/卡片 | 无真实读数/卡片，未进入详情页；规则文案由源码/布局断言 | 未执行（无设备数据） |

**与 G-1 的核心真机结论**：API 29 直连策略评估在**定位权限已拒绝**的情况下 `missing=[]`，仅因适配器关闭而阻断，证明直连前置不再依赖 `ACCESS_FINE_LOCATION`。

**安全副作用说明**：启动 App 后其 MQTT 客户端按既有逻辑尝试连接生产 broker（无网络，`ENETUNREACH`）；未建立连接、未发布/上传任何数据。未使用真实用户账号；`mac_addr` 为空（新装）。

### 4. 缺陷 D-AA005-1（阻断项，实测）

**来源验收**：`artifacts/android_tasks.yaml` ITEM-005（G-6）——“401/403 与其他 4xx **不产生自动重试请求**；本次批次行全部保留且不被删除；MyPrefs 写入原因码/HTTP 状态/时间/待发条数”;AA-005 §6.5“AuthRejected/RequestRejected **不**进入自动退避，保留全部行、**置全局阻塞状态**”。

**现象**：`CloudUploadRepository` 在 `AuthRejected`/`RequestRejected` 时对**本批**行执行 `outbox.park(...)`（`attempts=maxAttempts`），并写 `upload_blocked_reason` 等状态；但 `isGateOpen()` / `uploadPendingOnce()` **从不读取阻塞状态**。因此：
- 单批上限（50）之外的到期行、以及此前处于退避中而稍后才到期的行，会在下一次自动触发（每轮 BLE / 入队后 / 网络恢复 / 冷启动）继续被自动上传；
- 界面显示“上传被拒绝/请求被拒绝、重试上传”，但后台仍在自动请求；
- 每批最终也会被 401 拒绝并 park，故不丢数据，但“暂停自动重试”的语义未成立。

**最小复现（宿主机独立门禁，已执行）**：
1. 内存待发箱放 2 行均到期；`Config(batchLimit=1)`；伪 Api 恒返回 `AuthRejected(401)`。
2. 第 1 次 `uploadPendingOnce()` → `upload_blocked`，第 1 行 attempts=20（park），第 2 行仍 attempts=0。
3. 第 2 次 `uploadPendingOnce()`（模拟下一次自动触发）→ **再次发出请求**（调用数 2），返回 `upload_blocked`。
4. 期望：第 2 次返回 `empty`、调用数保持 1，直到用户手动 `retryBlockedNow()`。

**失败断言原文**（JUnit XML）：
```
org.junit.ComparisonFailure: 401 后不得再产生自动上传请求（其余到期行必须等待手动重试）
expected:<[empty]> but was:<[upload_blocked]>
```

**静态佐证**：`evidence/software_verify_aa005_android_check.py` 的 FINDING E6——`CloudUploadRepository`/`UploadOutbox` 不读取 `upload_blocked_reason`（该键仅由 `PrefsUploadGate` 写入、由设置对话框展示）。

**影响**：401/403/4xx 后仍会产生自动 HTTP 请求（请求次数与待发行数/批上限相关），与验收和用户可见的“已暂停/重试上传”不一致；不丢行、不影响 BLE/MQTT/告警/入库。

**建议修复边界（交实现）**：在 `uploadPendingOnce`（或仅自动触发路径）取批前检查全局阻塞标记，阻塞时直接返回 `empty`/`blocked`；`enqueueAndUpload` 仍可入队（不丢数据）；`retryBlockedNow()` 清除阻塞后立即上传一次；不改变 `UploadResult` 分类、待发箱表结构、批量/退避常量。补一条与 `g6_authRejected_mustPauseAllAutomaticRequests_untilManualRetry` 同形的回归用例。

### 5. 观察项（不单独构成失败，交上游/实现参考）

| ID | 观察 | 证据/影响 |
|---|---|---|
| O-AA005-1 | 扫描权限刚授予但蓝牙关闭时，`GatewayBleSettingsDialog.onPermissionResult` 用 `hasScanPermission()`（含适配器状态）判定，弹出“未授予扫描权限，无法搜索网关”，实际原因是蓝牙未开启；再次点击才会显示 `userMessage`“蓝牙未开启（去开启）” | 代码 `GatewayBleSettingsDialog.kt:125-135,230-251,293-303`；真机流程 19:31:18 授予 → 19:31:22 无扫描日志。建议在该分支使用 `policy.reason/userMessage` |
| O-AA005-2 | API31+ `canScan` 额外要求 `BLUETOOTH_CONNECT`，`scanRequestPermissions` 在仅有 SCAN 时补请求 CONNECT；TD-SW-004 T-SW-L0-28 公式为 `adapter && SCAN`，ITEM-002 文字为“扫描仅请求 BLUETOOTH_SCAN”，而 ITEM-001 文字为“请求 CONNECT 与（扫描场景）SCAN” | 规范内部张力；Android 12 “附近设备”权限组通常同时授予两项，真实影响低。本能力未据此判失败 |
| O-AA005-3 | `用户使用说明书.md` 仍保留“应用版本:1.8”及卡片“最近上报/可能长时间无上报”措辞，与 App 当前“最近读取/可能长时间无数据”、版本 1.9 不一致；不属于 ITEM-006 明确验收项 | 文档一致性建议同步；不影响构建/功能 |
| O-AA005-4 | 新装 App 在 `mac_addr` 为默认 `000000000000` 时仍会使用迁移默认网关地址发起连接（设计明确“默认未确认”）；本轮蓝牙关闭未触发 GATT | 与 AA-005 §6.2 一致；真实连接需网关上电 |

### 6. 未执行项与外部条件（不得写成通过）

| 项 | 缺失条件 | 状态 |
|---|---|---|
| 真实 BLE 扫描候选/FFE0/名称/RSSI/10 s 超时/stopScan | 蓝牙关闭；网关上电与广播未确认；未开启蓝牙以避免改变设备/无线现场 | NOT EXECUTED |
| 真实 GATT 读取、协商 MTU、FFE2 实际 `value.size`、16 B 对齐/不足拒绝 | 同上；无真实网关 | NOT EXECUTED |
| 多设备完整批次、数量/ID/顺序拒绝、占位/0.0 真实源 | 无真实多设备/可控源 | NOT EXECUTED |
| 9 台绑定、第 10 台拒绝、历史 >9 台不读取 | 无 9 台测试绑定/真实设备；仅完成代码门禁与静态断言 | NOT EXECUTED（UI/设备端） |
| 真实 `/upload_data`、401/403、手动重试、持久化回读 | 手机无默认网络；无隔离测试账号；无服务写入/回读授权 | NOT EXECUTED |
| API 31+ 权限弹窗/拒绝/设置跳转 | 无 API31+ 真机或 AVD | NOT EXECUTED |
| 详情页规则/高温强调/24 h/卡片相对时间真机截图 | 无真实读数与设备卡片 | NOT EXECUTED（源码/布局断言通过） |
| 调温/改变光强/遮光/功耗 | 用户明确禁止安排此类硬件试验 | NOT APPLICABLE |

### 7. 本轮测试资产与环境修复（透明记录）

1. **新增独立验证**：`app/src/test/.../verification/Aa005SevenItemVerificationTest.kt`（20 项）与 `evidence/software_verify_aa005_android_check.py`（54 项）。均为本能力测试资产，不改产品源码。
2. **历史门禁按已批准变更更新（无删断言/无降低强度）**：
   - `software_verify_cloud_upload_item009.py`：D1 改为 `api.uploadDetailed` 包 try/catch；F2 改为 `uploadData` + `UploadResult.fromHttp` 且无 `isSuccessful`（AA-005 G-6）。
   - `software_verify_timestamp_write_item007.py`：A6 接受 BLE 分支以显式 `receiveTime` 传入（等价于原“不传 timeOverride”的接收时刻语义），并新增“接收时刻无固定间隔回填”断言（AA-005 G-5 重构）。
   - `software_verify_card_status_item011.py`：提示文案更新为“尚未读取到数据/最近读取 … 可能长时间无数据”（AA-005 D-30），仍断言不出现“离线”。
   - `software_verify_detail_presentation_item012.py`：冻结规则文案方向更新为“升高”（AA-005 G-4），推断词表增加“因升高”。
   - `software_verify_release_item014.py`：版本期望 10/1.9；清单覆盖范围限定 Android 产品文件（`dengbei_care/`，排除 iOS 资料），避免把固件/后端历史提交算入本 Android 交付。
3. **测试工具依赖修复**：运行 `android_test` 时报 `MISSING_DEPENDENCY`（`adbutils` → `uiautomator2` → `apkutils`）；在用户 Python 环境以 pip 缓存非交互安装 `adbutils 2.12.0`、`uiautomator2 3.7.1`、`apkutils 4.0.0`（`apkutils` 依赖使 `lxml` 4.9.1→6.1.3）。未改产品源码、未安装系统包；安装后 `android_test install/launch/ui_*` 可用。
4. **原始证据**：`evidence/aa005_android_check/`（UI 树、截图、logcat、安装结果、权限/版本快照、JUnit XML、静态/门禁日志）。
5. **设备清理**：守护已停止；网关已清除；蓝牙/网络保持关闭；App 保持安装；一次性定位授权未撤销（配置的设备工具无 revoke 动作，未使用未授权的 shell 入口）。无用户数据产生。

### 8. 判定

- **G-1/G-2/G-3/G-4/G-5/G-7**：宿主机独立矩阵、静态核查与（在可用条件下）真机 UI/日志均未发现与验收不符；相关真实链路项按 §6 记为未执行。
- **G-6**：`UploadResult` 分类、单批 park、手动重试恢复均通过；但“401/403/4xx 后不产生自动重试请求（全局阻塞）”未满足，且已由独立门禁稳定复现（D-AA005-1）。
- 依据 TD-SW-004 §9.3“任一必需项失败/未执行 → TEST_FAIL”，本轮结论 **TEST_FAIL**；`failure_cause=MEASURED_DEFECT`（同时保留 §6 的真实链路未执行项，不以模拟豁免）。
