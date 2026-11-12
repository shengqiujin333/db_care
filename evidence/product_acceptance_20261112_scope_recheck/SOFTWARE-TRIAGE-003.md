# 安卓对象失败归因（SOFTWARE-TRIAGE-003）

- 负责角色：软件测试
- 能力：软件失败分析（`product_android_triage`）
- 分析对象：`artifacts/android_implementation.md`（Android 当前候选实现；包 `com.jinyuni.dengbei_care`，versionCode 10 / versionName 1.9，debug；APK SHA-256 `71a041d322368c20a181d5716c44ac4d1b2c8a3869926b0047cd96b61e2c10d8`）
- 原始失败（本轮输入）：`evidence/product_acceptance_20261112_scope_recheck/PRODUCT-ACCEPTANCE-RECHECK-002.md` **`TEST_FAIL`**（`failure_cause=EXTERNAL_CONDITION_UNEXECUTED`）
- 历史失败报告：`evidence/product_acceptance_20261112/PRODUCT-ACCEPTANCE-002.md`（同一 `TEST_FAIL`；其范围错误见 §2）
- 代码索引：`artifacts/codegraph_android_index.md`（projectPath `D:\mypro\beiwo2\AIP\dengbei_care`，索引基线 `9b3df8d`）
- 分析时点：2026-11-12 18:28–18:40（Asia/Shanghai；设备工具时间 10:32Z）
- 分析时 HEAD：`3316140dc5826329243769255de58707af9a5a5f`，工作区干净
- **本轮结论：`EXTERNAL_CONDITION_MISSING`（原验证缺少温度/光强刺激、仪器、跨链路环境与平台写授权等外部条件；Android 实现未证实有错）**

> 本文件只做失败归因，不是复验通过、不是修复成功，也不表示产品合格。原始 `TEST_FAIL` 仍然有效；所有未执行项保持未执行，不写成通过。
> 本轮未修改产品源码、测试期望、固件、文档、权限或环境配置；未安装/卸载 App、未清数据、未烧录/复位、未调温/调光/遮光、未测功耗、未访问平台写入接口或数据库。
> 本结论取代 `SOFTWARE-TRIAGE-002` / `ANDROID-ASSIGNMENT-REFERRAL-002` 的结果码选择（`ASSIGNMENT_DECLINED` → `EXTERNAL_CONDITION_MISSING`）；原文件与其原始证据保留，不改写。分派对象（Android）本身没有错配，变化在于：新失败证据已把人工跳过项与实现缺陷分离，剩余 Android 域内未执行项的阻塞条件是外部环境/授权，而不是职责不匹配。

---

## 1. 结论摘要

1. **人工范围决定复核成立**：`FW-LIGHT-DEF2-ENABLE` 已由人工确认 `APPROVED_DEFERRED` / NOT EXECUTED（`HR-8A11935AD0A7`，2026-11-11T03:50:42Z，「无法准备，先跳过」，`deferral_allowed=true`）。按本轮 operator recovery instruction，该测项**不再作为返工依据**，也不得重复排队同一 `test_item_id`。
2. **原检查者的范围错误已独立复核确认**：`PRODUCT-ACCEPTANCE-002` §5 R4 把“无光判据未启用”记为“未满足/未执行”，并在最终判定中把“需求 4 的无光分支仍禁用”作为整体 `TEST_FAIL` 的依据之一；这与同报告 §6 N-1 已正确记录的 `SKIP_CONFIRMED` 自相矛盾。正确分类是：**无光判据启用 = NOT EXECUTED（人工延期）**、**上报公式实现 = 静态一致**、**真实升温/超温触发 = NOT EXECUTED（温度条件）**。`PRODUCT-ACCEPTANCE-RECHECK-002` 已完成该重评，本 triage 独立确认其结果。
3. **Android 对象身份闭合**：本轮复算主机候选 APK 10,365,464 B / SHA-256 `71a041d3…` / MD5 `1e17e0c2…`；`git diff db53202..HEAD -- dengbei_care/app/src/main dengbei_care/app/build.gradle.kts` **为空**；设备 `12f046690506` 在线，在装包 versionCode 10 / versionName 1.9 / debug，`base.apk` 路径与复核报告记录一致（设备侧哈希见复核证据）。
4. **Android 已执行范围没有实测缺陷**：D-01/D-02/D-03/D-04a 在当前 APK 上已逐项关闭；首页/我的/报告/列表/网关对话框/守护启停可用；会话内本应用 `FATAL EXCEPTION` 0 条、`ANR` 0 条。本轮未发现可归因于 Android 的新缺陷。
5. **剩余失败项均为外部条件或后续阶段**：真实升温/超温（用户明确不能调温）、光强双态（人工已跳过）、功耗（无仪器且用户明确不能测）、网关→手机跨链路与真实 BLE/GATT/MTU（用户安排后续联调，当前网关未配置、手机无绑定）、平台 `/upload_data` 写入/回读（无写接口与数据库授权）、API 31+ 权限分支与 9 台真实快照（无对应设备/数据源）。
6. **不选 `REPAIR_REQUIRED`**：没有实测证据证明本次具体可修改对象（Android 实现）需要修复；把“未执行”“缺外部条件”或“人工跳过”升级为返工，属于虚构修复依据。
7. **不选 `ASSIGNMENT_DECLINED`（本轮）**：Android 域内仍有未执行的验收项（真实 BLE/上传/注册/列表/API 31+），其阻塞条件是外部环境与授权；应通过 `EXTERNAL_CONDITION_MISSING` 返回原测试节点，按现有 HumanInteraction 机制处理，而不是把整个分派判为对象错配。
8. **不选 `RESPONSIBILITY_UNRESOLVED` / `REPLAN_REQUIRED` / `WORK_FAILED`**：责任边界已可确定（不是 Android 实现缺陷；其余为外部条件），调查已完成并取得第一手身份与源码证据，无需重新规划或判定执行失败。
9. **结果：`EXTERNAL_CONDITION_MISSING`**。报告列明具体缺口与人工可执行准备动作（§8）。本结果不表示测试 PASS，也不授权跳过任何测项；已批准延期仅限 `FW-LIGHT-DEF2-ENABLE` 与 `E2E-04`，不得扩大。

---

## 2. 人工范围决定复核与原检查者范围错误

### 2.1 当前人工范围决定（引用，不改写）

| 项 | 内容 |
|---|---|
| 测项 | `FW-LIGHT-DEF2-ENABLE`（无光判据启用与实板双态） |
| 人工结论 | **`APPROVED_DEFERRED` / NOT EXECUTED**（`HR-8A11935AD0A7`，2026-11-11T03:50:42Z；原回复「无法准备，先跳过」；`deferral_allowed=true`） |
| 原因 | 缺少完全遮光窗口与同板同构建的光强双态（有光/无光）被动采集证据；EN-01 无判据形制/数值接受记录，EN-02 无双态被动证据；板上保持 `T=0`（全周期 `V=0 D=0`） |
| 本轮处置 | 不重测、不重复询问同一 `test_item_id`、**不作为返工依据**；在证据与交付中显式列为 NOT EXECUTED |

### 2.2 原检查者需要重评的范围错误（本轮独立复核）

| # | 原报告位置 | 原文/事实 | 范围错误 | 应重评为 |
|---|---|---|---|---|
| E-1 | `PRODUCT-ACCEPTANCE-002` §5 R4 | 上传规则标为“**未满足/未执行**”，并以“交付默认 `T=0` ⇒ 无光分支永不触发”说明 | 把人工已确认延期的**启用项**与“未满足”混同 | R4 **判定公式实现**与需求逐项一致；**无光判据启用** = NOT EXECUTED（人工延期）；**真实升温/超温触发** = NOT EXECUTED（外部条件） |
| E-2 | 同报告最终判定 | 以“需求 4 的无光分支仍禁用”作为整体 `TEST_FAIL` 依据之一 | 以人工跳过项作为整体失败/返工依据 | 整体 `TEST_FAIL` 的依据只保留**其余外部条件/后续阶段项**；人工延期项不作为返工依据 |
| E-3 | 同报告 §7 影响范围 | “R4 无光分支仍为 `T=0`：待判据形制/数值接受记录与同板同构建双态证据后回填” | 后续入口本身正确，但未与“本轮不返工”区分 | 保留为需求/接受侧未决事项；固件与 Android 本轮均不因该未决项返工 |
| E-4 | 同报告 §6 N-1 | 已正确记录 `SKIP_CONFIRMED`、不重测、不写成通过 | 披露本身正确，但与 §5 R4“未满足”标签及最终判定不一致 | 以 N-1 的披露为准，修正 R4 标签与最终判定依据 |

**复核结论**：`PRODUCT-ACCEPTANCE-RECHECK-002` §2 已按人工范围完成上述重评；本 triage 独立核对原始报告原文与人工延期记录后确认该重评成立。当前有效的失败依据不包括 `FW-LIGHT-DEF2-ENABLE`。

### 2.3 重评后的正确分解

1. **上报公式实现**：`report = (cur > 35.0℃) 或 (有前值 且 (cur - prev) > 0.9℃ 且 dark)`；严格大于边界。源码（`USER/src/fw_core.c:112-126`、`USER/inc/sensor_config.h`）与需求逐项一致。属**已实现项**，非缺陷。
2. **无光判据启用**：`FW-LIGHT-DEF2-ENABLE` = **NOT EXECUTED（人工 `APPROVED_DEFERRED`）**，不是实现缺陷，不是本轮返工依据。
3. **真实升温/超温触发**：NOT EXECUTED（用户明确当前不能调温），属外部条件。
4. **整体验收**：仍不能宣布 PASS，但失败依据应为“其余外部条件/后续阶段项未执行”，不能把人工延期项算作实现缺口。

---

## 3. 原始失败引用与对象身份闭合

### 3.1 原始失败（引用，不改写）

| 项 | 原文事实 |
|---|---|
| 验收对象 | 传感器固件 FD-002 rev 8 发布包、安卓 1.9 debug 候选 AB-002、既有网关与平台服务、文档包 DOC-PKG-001 rev 12 |
| 结论 | `TEST_FAIL`（`failure_cause=EXTERNAL_CONDITION_UNEXECUTED`） |
| Android 实测关闭项 | D-01「查看今日报告」直达 `ReportActivity`；D-02「打开传感器列表」进入列表页；D-03 两控件 bounds 不相交；D-04a 首页文案与实际入口一致 |
| 本轮 “新缺陷” | 报告原文：“已执行范围内**未发现新的实测缺陷**” |
| 整体未通过原因 | 真实升温/超温、光强双态、功耗、网关→手机跨链路、`/upload_data` 写入回读等必需项未执行；`FW-LIGHT-DEF2-ENABLE` 为人工 `APPROVED_DEFERRED` / NOT EXECUTED（不作为返工依据） |
| 观察项（非阻断） | O-1 文档仍写“整产品验收尚未重跑”；O-2 零设备时报告页“重新生成”无可见反馈；O-3 设备无网络时 `MqttService` 记录 `ENETUNREACH` |

### 3.2 本轮独立核对的对象身份

| 项 | 值 | 本轮核对方式 |
|---|---|---|
| 分析对象 | `artifacts/android_implementation.md`（AndroidImplementation） | 阅读 |
| 主机候选 APK | 10,365,464 B；SHA-256 `71a041d322368c20a181d5716c44ac4d1b2c8a3869926b0047cd96b61e2c10d8`；MD5 `1e17e0c281715fddd26d308246d6424c` | **本轮 `stat` / `sha256sum` / `md5sum` 复算，与原始失败证据逐项一致** |
| 设备在装包 | `com.jinyuni.dengbei_care`，versionCode 10 / versionName 1.9，`DEBUGGABLE`；`/data/app/com.jinyuni.dengbei_care-uAuDOuV-VKufDQNnGGCFNg==/base.apk` | **本轮 `dumpsys package` / `pm path` 复核**；设备侧大小与 SHA-256/MD5 见 `PRODUCT-ACCEPTANCE-RECHECK-002` 的 `device_apk_hash.txt`（与主机候选逐字节一致） |
| 设备 | Redmi 8A（`olivelite`），Android 10 / API 29，serial `12f046690506` | **本轮 `android_test devices` 确认在线** |
| Android 主源码 | `git diff --stat db53202..HEAD -- dengbei_care/app/src/main dengbei_care/app/build.gradle.kts` **为空** | **本轮 `git diff` 复核** |
| 当前 HEAD | `3316140`；`git status --porcelain` 为空 | **本轮 `git rev-parse` / `git status` 复核** |
| 代码索引 | `artifacts/codegraph_android_index.md`，projectPath `dengbei_care` | 阅读；本轮用其确认目录与入口，未用图结果替代直接源码核对 |

**身份闭合**：主机候选 APK、设备在装包与 `android_implementation.md` 交付记录对应同一候选；Android 主源码自原始验收基线 `db53202` 未变化。因此原始失败中的 Android 实测结论对应当前同一对象，不是旧包或旧源码的结论。

---

## 4. 本轮获准的最小调查与未做事项

### 4.1 已做

| 项 | 方式 |
|---|---|
| 人工范围决定 | 复核当前工作流 `deferred_test_items`（`HR-8A11935AD0A7`）与原报告 N-1 |
| 原检查者范围错误 | 对照 `PRODUCT-ACCEPTANCE-002` §5/§6/§7 与 `PRODUCT-ACCEPTANCE-RECHECK-002` §2 逐条复核 |
| 对象身份 | APK 大小/哈希、Android 源码 diff、HEAD/工作区、设备在线与在装版本 |
| 责任边界 | 直接读取固件判据宏与判定函数、Android 规则文案与光照相关检索 |
| 文档观察项 | 有界检索 `docs/` 中的“整产品验收尚未重跑”表述 |

### 4.2 未做（如实登记）

- 未重跑整产品验收、未重新安装 APK、未清数据、未改系统设置。
- 未重新执行 UI 点击；本 triage 引用 `PRODUCT-ACCEPTANCE-RECHECK-002` 的当前候选真实设备证据，并仅独立闭合对象身份。
- 未采集 COM43、未烧录/复位、未使用 pyocd；固件侧判读来自直接源码与复核报告的串口事实。
- 未调温、未调光、未遮光、未测功耗；用户明确当前不能进行这些操作。
- 未连接平台、未调用 `/upload_data`、未连数据库、未执行只读 SSH。
- 未读取或修改 `runtime/permissions.yaml` / `runtime/environment.yaml`。
- 未修改产品源码、测试期望、固件、文档或他人证据；仅新增本归因文件并维护清单。

---

## 5. Android 已执行范围逐项判读

| Android 相关项 | 复核报告记录（当前候选 APK） | 本 triage 判读 |
|---|---|---|
| D-01 报告入口 | 点击直达 `ReportActivity` | **已关闭**；无 Android 返工依据 |
| D-02 传感器列表入口 | 点击进入列表页（已保存的 ID（0）） | **已关闭**；无 Android 返工依据 |
| D-03 控件重叠 | `[32,976][688,1072]` 与 `[32,1120][688,1216]` 不相交（间隔 48 px） | **已关闭**；无 Android 返工依据 |
| D-04a 首页文案 | 文案与实际入口一致，两条路径均可达 | **已关闭**；无 Android 返工依据 |
| R6 App 基本可用 | 首页/我的/报告/列表/网关对话框/守护启停可用；设定页默认值 2 / 0.1 / 35 / 29 / 75 | **在已执行范围内成立**；不等于真实 BLE/上传链路通过 |
| 崩溃检查 | 本应用 `FATAL EXCEPTION` 0 条、`ANR` 0 条 | **在已执行范围内成立** |
| 观测有效性 | 零设备时报告页“重新生成”无可见反馈（`No saved devices, skipping report`） | **产品既有设计**；如需“无设备也生成空报告”属需求变更，不是本轮缺陷 |
| 设备网络 | `MqttService` 记录 `ENETUNREACH` | 设备侧网络条件，不是应用崩溃或功能缺陷 |
| 真实 BLE/GATT/MTU、数据展示、告警、上传状态 | 未执行（无网关、无已绑定传感器） | **NOT EXECUTED**；不是已确认缺陷 |
| 注册/登录、网关编码保存、真实告警参数下发 | 未执行（无账号/网关编码等外部数据） | **NOT EXECUTED**；不是已确认缺陷 |
| 有真实绑定数据时的传感器列表列出/删除 | 未执行（当前 0 条绑定） | **NOT EXECUTED**；不是已确认缺陷 |
| API 31+ 权限分支、9 台真实快照、第 10 台拒绝 | 未执行（设备 API 29；无真实多设备源） | **NOT EXECUTED**；不是已确认缺陷 |
| `/upload_data` 真实写入、幂等与回读 | 未执行（无写接口/数据库授权） | **NOT EXECUTED**；不是已确认缺陷 |

> 上表区分“已执行并通过的 Android 范围”与“未执行的 Android 范围”。未执行不等于通过，也不等于缺陷；不得为了关闭本 triage 而把未执行项写成 PASS。

---

## 6. 其他有效目标与真实缺陷排查

| 目标 | 本轮复核 | 结论 | 是否 Android 返工依据 |
|---|---|---|---|
| D-01～D-04a | 复核报告在当前 APK 上逐项实测关闭；对象身份本轮闭合 | 已关闭 | 否 |
| R6 Android 基本可用 | 真实设备 UI/Activity/服务状态；0 FATAL/ANR | 已执行范围内成立 | 否 |
| R4 上报公式实现 | 固件源码 `(cur > 350) 或 (have_prev && (cur-prev) > 9 && dark)`；严格大于；int32 差值 | 与需求公式静态一致 | 否（固件实现项） |
| R4 无光分支启用 | `LIGHT_DARK_THRESHOLD_CODE=0`；`valid = adc_ok && (T != 0)`；人工 `APPROVED_DEFERRED` | NOT EXECUTED（人工延期） | **否（明确不作为返工依据）** |
| R4 真实升温/超温触发 | 用户明确不能调温；复核窗口 `r=0 s=0`、无 `P` | NOT EXECUTED（外部条件） | 否（固件/现场项） |
| 3 分钟采样节拍 | 复核报告实板 `k` 步长 3；源码 `SENSOR_RTC_TICK_PERIOD_MIN=1`、`SENSOR_SAMPLE_TICKS=3` | 成立 | 否（固件实现项） |
| 无光判定责任 | Android 主源码仅在规则文案/注释中出现“无光”；无光照/照度字段、无 lux 逻辑、不推断上报原因 | 判据在固件，不在 Android | 否 |
| O-1 文档旧表述 | 本轮检索确认 `docs/README.md:81`、`docs/04:241`、`docs/06:122`、`docs/01:110` 仍写“整产品验收尚未重跑” | **真实的文档时效问题**；不属 Android 实现，不影响功能 | 否（交技术文档下次修订） |
| O-2 零设备报告刷新 | 复核报告已记录；无设备无可汇总数据 | 既有设计；需求未要求空报告 | 否 |
| O-3 `ENETUNREACH` | 设备无网络 | 设备侧网络条件 | 否 |

**真实缺陷排查结论**：在本次可执行的 Android 范围内，**没有实测到可归因于 Android 实现的缺陷**。唯一可行动的真实问题是文档时效（O-1），属技术文档职责，不是 Android 返工项，也不构成本轮整体失败的技术缺陷。未执行项是外部刺激/授权/后续阶段，不是失败签名。

---

## 7. 竞争假设与证据判断

| # | 假设 | 判断 | 依据 |
|---|---|---|---|
| H1 | 当前 Android 候选仍存在原始失败中的 D-01～D-04a 缺陷 | **不成立** | 复核报告在当前 APK 上逐项实测关闭；对象身份本轮闭合（哈希、源码零差异、设备在线） |
| H2 | D-01～D-04a 的关闭是点击工具/选择器造成的假象 | **不成立（就现有证据）** | 复核报告保留 Activity 栈与 UI 树；两个入口到达正确 Activity/Fragment；无崩溃 |
| H3 | Android 应负责“无光”判定，App 未实现或实现错误 | **不成立** | App 主源码无光照/照度/无光判定逻辑；判据在固件 `light.c` / `fw_core.c`；`T=0` 使分支关闭且属人工跳过项 |
| H4 | 3 分钟采样或上传规则是 Android 实现职责 | **不成立** | 采样常量与上报判据在传感器固件；Android 只接收、展示、入队上传；实板节拍已测得 |
| H5 | `LIGHT_DARK_THRESHOLD_CODE=0` 是已确认的实现缺陷，应立即返工 | **不成立（未证实）** | 它是交付时明示的安全默认；需上游判据接受与同板双态证据；人工已确认延期，不作为返工依据 |
| H6 | Android 的未执行功能（BLE/上传/注册/列表/API 31+）已被证明有缺陷 | **不成立** | 没有执行证据，也没有失败签名；只能记为 NOT EXECUTED。缺乏证据不等于缺陷，也不等于排除 |
| H7 | 平台/服务端已被证明故障 | **不成立（未证实）** | 本轮未连接、未写入、未连库；仅有配置静态一致；需联调/授权后验证 |
| H8 | 文档旧表述导致产品功能失败 | **不成立** | O-1 是文档时效问题，不影响 Android 行为 |
| H9 | 应把同一无光测项再交回测试节点询问 | **不成立** | 已 `APPROVED_DEFERRED`；工作流禁止重复排队同一测项；本轮 operator 明确其不再是返工依据 |

**分类**：

- **已被证据排除（针对 Android）**：D-01/D-02/D-03/D-04a；Android 侧光照判定责任；Android 侧采样节拍/上报判据责任；候选/设备/源码版本不一致；已执行 UI 路径的点击工具假象。
- **仍缺乏证据（未执行，不是缺陷）**：真实 BLE/GATT/MTU、真实上传与平台回读、注册/登录、有数据时的列表、API 31+、9 台真实快照、真实升温/超温、光强双态、功耗、网关跨链路。
- **未确定为缺陷（保留未知）**：`T=0` 的启用时机与阈值取值、平台服务实际可用性、网关身份与配对状态、光敏器件/光路电气特性。它们需要外部条件或上游决定，不能由本能力写成已证实根因。

---

## 8. 具体外部缺口与人工可执行准备动作

| # | 缺口 | 影响的原验收项 | 人工可执行准备动作 | 当前处置 |
|---|---|---|---|---|
| X-1 | 真实升温/超温刺激不可用（用户明确不能调温） | R4 真实 `ΔT>0.9℃`、`>35.0℃` 触发与 433 实发（N-2） | 提供可控、安全的温度刺激环境；或确认本阶段不执行 | NOT EXECUTED；不安排测试，不据此判实现缺陷 |
| X-2 | 功耗测量仪器不可用（用户明确不能测功耗） | `E2E-04` CR2032 峰值/深睡电流（N-6） | 提供电流表/SMU/示波器等仪器；或确认本阶段不执行 | NOT EXECUTED（已批准延期）；硬件测试职责 |
| X-3 | 网关→手机跨链路环境未准备 | 433→网关→BLE→手机同笔一致性、真实 BLE/GATT/MTU、数据展示、告警、上传状态（N-4/N-8） | 为后续联调阶段准备网关供电/身份、BLE 配对与手机绑定 | NOT EXECUTED；按用户安排进入后续联调 |
| X-4 | 平台 `/upload_data` 写入/回读授权不可用 | R5 平台写入、幂等与回读（N-5） | 提供写接口/隔离身份授权，或明确本阶段不执行 | NOT EXECUTED；本能力无写/数据库授权 |
| X-5 | 账号/网关编码等外部数据不可用 | 注册/登录、网关编码保存、真实告警参数下发（N-9） | 提供测试账号与网关编码，或明确本阶段不执行 | NOT EXECUTED；不写入平台、不改用户数据 |
| X-6 | API 31+ 真机与真实多设备源不可用 | API 31+ 权限分支、9 台真实快照、第 10 台拒绝（N-11） | 提供 API 31+ 设备与真实多设备源，或明确本阶段不执行 | NOT EXECUTED；宿主机矩阵不替代真机 |
| X-7 | 无光判据接受记录与同板双态被动证据不具备 | `FW-LIGHT-DEF2-ENABLE`（N-1/N-3） | **已由人工 `APPROVED_DEFERRED`；不得重复询问同一测项**。若后续提供接受记录与被动双态证据，再按 EN-01/EN-02 重开 | NOT EXECUTED（人工延期）；**不作为返工依据** |

> 返回原测试节点后，测试能力应只对**尚未**被人工确认延期/限制的项按现有 HumanInteraction 机制处理；`FW-LIGHT-DEF2-ENABLE` 不得重复排队。是否可延期只由已审批的测项范围决定；本结果本身既不表示测试 PASS，也不授权跳过。

---

## 9. 责任边界与结果判断

| 对象 | 判断 |
|---|---|
| Android 实现（本次分派对象） | **已执行范围内无实测缺陷，不构成本次返工对象**；D-01～D-04a 已关闭。未执行的 Android 功能保持 NOT EXECUTED，阻塞条件为外部环境/授权/后续联调 |
| 传感器固件/需求上游 | 需求 4 无光分支 `T=0` 待“判据形制/数值接受 + 同板同构建双态证据”；满足后由固件实现回填并重建。**当前不升级为返工** |
| 用户/现场 + 嵌入式测试 | 真实升温/超温、光强双态、遮光窗口；用户当前不能准备，测项保持未执行 |
| 硬件测试 | 功耗与光路/分压电气特性；无仪器，保持未执行 |
| 联调阶段（网关/手机/平台） | 433→网关→BLE→手机同笔一致性、真实 BLE、`/upload_data` 写入回读；需网关供电/配对、绑定数据与平台授权 |
| 技术文档 | O-1 旧表述待下次修订更新；不影响 Android 功能 |
| 责任是否未定 | **否**。Android 无返工依据；其余对象的缺口与前置条件已逐项列出 |
| 外部条件是否可立即补齐 | **否（当前）**。无光项已人工延期；温度/光强/功耗受用户限制；联调按用户安排在后续阶段 |

**结果码选择理由**：

- **`REPAIR_REQUIRED` 不成立**：没有实证缺陷签名；人工延期项不得作为返工依据；未执行项不得升级为缺陷。
- **`EXTERNAL_CONDITION_MISSING` 成立**：原验证缺的是温度/光强刺激、功耗仪器、跨链路环境、平台写授权与 API 31+ 设备等外部条件；Android 实现未证实有错。
- **`RESPONSIBILITY_UNRESOLVED` 不成立**：责任边界已确定为“非 Android 实现缺陷 + 外部条件”，不是无法定位。
- **`ASSIGNMENT_DECLINED` 不采用**：分派对象（Android）与 Android 域内未执行项（BLE/上传/注册/列表/API 31+）匹配；阻塞在外部条件，不在职责错配。早期 triage 的转交结论保留为历史，但其结果码由本轮取代。
- **`REPLAN_REQUIRED` 不采用**：当前不需要重新规划角色/对象；需要的是把外部条件缺口按原测试节点的 HumanInteraction 机制处理。
- **`WORK_FAILED` 不成立**：调查已完成并取得第一手身份、源码与设备证据。

---

## 10. 未执行项与证据局限

1. 本 triage 未重跑整产品验收；Android 已执行范围的结论引用 `PRODUCT-ACCEPTANCE-RECHECK-002` 的当前候选真实设备证据，本轮只独立闭合对象身份（APK 哈希、源码 diff、设备在线与在装版本）。
2. 未重新安装/启动 App；未复测 UI 点击；未读取本轮新的 UI 树或截图。
3. 未采集 COM43、未烧录/复位、未使用逻辑分析仪；固件侧判读来自直接源码与复核报告的串口事实。
4. 未调温、未调光、未遮光、未测功耗；这些是用户明确限制或人工已确认延期的条件，不安排相关硬件测试。
5. 未连接平台、未调用写接口、未连数据库、未执行只读 SSH；平台结论仅为静态配置一致，不是服务可用性结论。
6. `LIGHT_DARK_THRESHOLD_CODE=0` 的启用时机、阈值数值、判据形制接受记录仍未形成；不能由本能力替代上游决定。
7. Android 真实 BLE/上传/注册/列表/API 31+/9 台等行为仍未执行；不能写成通过或缺陷。
8. 板端镜像哈希不可从板端回读；原始固件身份属“发布包/成员哈希 + 源码零差异 + `k` 连续”的闭合推断，本 triage 不重新证明板端镜像。
9. 设备在装包哈希来自 `PRODUCT-ACCEPTANCE-RECHECK-002` 的设备侧复算（本轮确认安装路径一致），本 triage 未重新在设备上计算哈希。

---

## 11. 本轮改动

- 新增：`evidence/product_acceptance_20261112_scope_recheck/SOFTWARE-TRIAGE-003.md`（本文件）。
- 更新：`file_manifest.txt`（登记本文件；不改写既有条目）。
- **未修改**：`dengbei_care/**` 产品源码与资源、`app/src/test/**`、固件与 Keil 工程、`docs/**`、`runtime/permissions.yaml`、`runtime/environment.yaml`；未安装/卸载 App、未烧录/复位、未改设备或平台数据。`SOFTWARE-TRIAGE-002` / `ANDROID-ASSIGNMENT-REFERRAL-002` 保留，不改写。

---

**最终判定：`EXTERNAL_CONDITION_MISSING`。** Android 实现在已执行范围内未发现可证实缺陷；D-01～D-04a 在当前候选上关闭；`FW-LIGHT-DEF2-ENABLE` 是人工 `APPROVED_DEFERRED` 的 NOT EXECUTED 项，按本轮 operator 决定不再作为返工依据；原检查者 R4“未满足”标签与整体失败依据已由复核报告重评，本 triage 独立确认。剩余缺口是真实温度/光强刺激、功耗仪器、跨链路环境、平台写授权与 API 31+ 设备等外部条件/后续阶段。原始 `TEST_FAIL` 保持有效，本 triage 不表示产品通过，也不授权跳过任何测项。
