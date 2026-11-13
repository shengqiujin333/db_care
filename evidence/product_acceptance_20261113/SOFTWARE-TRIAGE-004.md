# 安卓对象失败归因（SOFTWARE-TRIAGE-004）

- 负责角色：软件测试
- 能力：软件失败分析（`product_android_triage`）
- 分析对象：`artifacts/android_implementation.md`（Android 当前候选；包 `com.jinyuni.dengbei_care`，versionCode 10 / versionName 1.9，debug）
- 原始失败（本轮输入）：`evidence/product_acceptance_20261113/PRODUCT-ACCEPTANCE-003.md` **`TEST_FAIL`**（`failure_cause=EXTERNAL_CONDITION_UNEXECUTED`）
- 代码索引：`artifacts/codegraph_android_index.md`（projectPath `D:\mypro\beiwo2\AIP\dengbei_care`）
- 分析时点：2026-11-13 17:30–17:45（Asia/Shanghai；设备工具时间约 09:33Z）
- 分析时 HEAD：`dd8b829`，`git status --porcelain` 为空
- **本轮结论：`EXTERNAL_CONDITION_MISSING`**（原验证缺少温度/光强刺激、功耗仪器、跨链路环境、平台写授权与真实数据源等外部条件；Android 实现未证实有错）

> 本文件只做失败归因，不是复验通过、不是修复成功，也不表示产品合格。原始 `TEST_FAIL` 保持有效；所有未执行项保持未执行，不写成通过。
> 本轮未修改产品源码、测试期望、固件、文档、权限或环境配置；未安装/卸载 App、未清数据、未烧录/复位、未调温/调光/遮光、未测功耗、未访问平台写入接口或数据库。
> 人工已确认跳过的 `FW-LIGHT-DEF2-ENABLE` 不作为返工依据、不重复排队；原检查者已纠正的范围问题继续采用，另指出本轮报告仍残留的标签错误（§3）。

---

## 1. 结论摘要

1. **人工范围决定复核成立**：`FW-LIGHT-DEF2-ENABLE` 为 `APPROVED_DEFERRED` / NOT EXECUTED（`HR-8A11935AD0A7`，2026-11-11T03:50:42Z，「无法准备，先跳过」，`deferral_allowed=true`）。本轮不重测、不重复询问同一 `test_item_id`、**不作为返工依据**。
2. **重评测项已处理**：`BOT-01` 无活动跳过决定；原检查者按人工要求改为测试侧自动方式（`capture_bot01.py` + pyocd 通用 `cortex_m` HW reset，仅复位）并取得上电五行，`BOOT_LINES_OK=PASS` / `BOT01_RESULT=PASS`。不需要固件新增测试接口，也不因此退回实现。
3. **对象身份闭合**：主机候选 APK 10,370,964 B / SHA-256 `80232c50…` / MD5 `a97ed9be…`；设备 `12f046690506` 在线，在装 `base.apk` 设备侧 SHA-256 **同为 `80232c50…`**，versionCode 10 / versionName 1.9；Android 主源码自基线 `a12d12b` 起 `app/src/main` 与 `build.gradle.kts` **零差异**；本轮 HEAD 相对验收报告 HEAD 仅新增验收证据与清单。
4. **已执行范围没有实测缺陷**：真实设备上本应用 `FATAL EXCEPTION` 0 条、`ANR` 0 条；日志中的 3 条 `ENETUNREACH` 是设备无外网、2 条 `No saved devices` 是零设备时的既有跳过逻辑；APK 条目未命中测试专用组件。报告 §5 的 R1/R3/R6/R8/R9 在各自限定范围内成立。
5. **剩余失败项为外部条件/后续阶段**：真实升温/超温触发（用户明确不能调温）、无光判据启用（人工已延期）、真实光强双态（用户明确不能改变光强）、功耗（无仪器且用户明确不能测）、网关→手机同笔链路（用户安排后续联调）、`/upload_data` 写入/回读（无授权且设备无外网）、真实 BLE 有效批次/多设备/API 31+（无数据源/设备）、release 正式签名产物（未产出）。上述均为 NOT EXECUTED，不是已测缺陷。
6. **不选 `REPAIR_REQUIRED`**：没有实测证据证明本次具体可修改对象（Android 实现）需要修复；把人工延期、未执行或外部条件升级为返工属于虚构修复依据。
7. **不选 `ASSIGNMENT_DECLINED`**：Android 域内仍有未执行验收项，其阻塞是外部环境/授权，不是对象或职责错配。
8. **不选 `RESPONSIBILITY_UNRESOLVED` / `REPLAN_REQUIRED` / `WORK_FAILED`**：责任边界可确定（非 Android 实现缺陷 + 外部条件），调查已完成并取得第一手身份、设备与原始证据。
9. **结果：`EXTERNAL_CONDITION_MISSING`**。缺口与人工可执行准备动作见 §8；本结果不表示测试 PASS，也不授权跳过任何测项。

---

## 2. 当前人工范围决定（引用，不改写）

| 项 | 内容 |
|---|---|
| 测项 | `FW-LIGHT-DEF2-ENABLE`（无光判据启用与实板双态） |
| 人工结论 | **`APPROVED_DEFERRED` / NOT EXECUTED**（`HR-8A11935AD0A7`，2026-11-11T03:50:42Z；原回复「无法准备，先跳过」；`deferral_allowed=true`；`delivery_status=APPROVED_DEFERRED`） |
| 原因 | 缺少完全遮光窗口与同板同构建的光强双态（有光/无光）被动采集证据；EN-01 无判据形制/数值接受记录，EN-02 无双态被动证据；板上保持 `T=0`（全周期 `V=0 D=0`） |
| 本轮处置 | 不重测、不重复询问、**不作为返工依据与整体失败依据**；在证据与交付中显式列为 NOT EXECUTED |
| 重评测项 | `BOT-01` **无活动跳过决定**；按人工要求重评自动测试方式，已在 `PRODUCT-ACCEPTANCE-003` §4.2 以测试侧脚本执行并 PASS；见 §7.1 |

---

## 3. 原检查者需要重评的范围错误（本轮独立复核）

### 3.1 已纠正、本轮确认成立的部分

`PRODUCT-ACCEPTANCE-002` 的 E-1/E-2/E-3（把人工延期项与“未满足”混同、以无光分支禁用作为整体失败依据、同报告标签自相矛盾）已在 `PRODUCT-ACCEPTANCE-RECHECK-002` §2 重评，并被 `PRODUCT-ACCEPTANCE-003` §2.2 继续采用。本轮核对原始报告文本、人工延期记录与当前失败报告后确认：**该重评方向成立**，`FW-LIGHT-DEF2-ENABLE` 不得作为返工依据。

### 3.2 本轮报告仍残留、需要原检查者重评的标签错误

| # | 位置 | 原文/事实 | 问题 | 应重评为 |
|---|---|---|---|---|
| R-1 | `PRODUCT-ACCEPTANCE-003` §10「整体不能通过的依据」 | 保留清单中仍写「真实光强双态（用户不能改光强且**该项已人工延期**）」 | 把已 `APPROVED_DEFERRED` 的测项重新纳入整体失败依据，与同报告「不作为整体失败依据」自相矛盾 | 从保留依据中移除该项；整体不能通过的依据只保留其余外部条件/后续阶段项（温度刺激、功耗、跨链路、平台写授权、真实数据源、release 产物等） |
| R-2 | 同报告 §5 R2、§6 N-3 | N-3「光强变化/遮光刺激」被标为「与 N-1 的人工延期一致」「人工延期 N-1/N-3」 | 当前 `deferred_test_items` 只有 `FW-LIGHT-DEF2-ENABLE`（N-1）；N-3 是**用户明确当前不能改变光强**的现场条件，不是人工确认的跳过测项 | 拆开标签：N-1 = 人工 `APPROVED_DEFERRED`/NOT EXECUTED；N-3 = 用户条件限制/不安排。二者都不构成实现缺陷，但不得混写 |
| R-3 | 同报告 §5 R2 结果列 | 「部分（采样有效；判据未启用）」 | 「判据未启用」容易被读成需求缺口，而它正是人工延期项的主题 | 改为「ADC 采样有效；无光判据启用 = NOT EXECUTED（人工延期）」，不作为实现缺陷或返工依据 |

> 上述为重评建议，不改变 `PRODUCT-ACCEPTANCE-003` 的原始 `TEST_FAIL` 记录，也不由其派生任何返工。原报告文件不改写；本文件与报告并存。

---

## 4. 原始失败引用与对象身份闭合

### 4.1 原始失败（引用，不改写）

| 项 | 原文事实 |
|---|---|
| 验收对象 | 传感器固件 FD-002 rev 8 发布包、安卓 AB-003 候选 APK（versionCode 10 / versionName 1.9，debug）、既有网关与平台服务、文档包 DOC-PKG-001 rev 14 |
| 结论 | `TEST_FAIL`（`failure_cause=EXTERNAL_CONDITION_UNEXECUTED`） |
| 已执行范围 | 传感器 COM43 被动两窗口、BOT-01 自动复位上电五行、Android 真机关键路径与守护启停、文档 rev 14 关键读者步骤、平台配置静态核对 |
| 本轮“新缺陷” | 报告原文：「已执行范围内**未发现可复现的软件/固件功能缺陷**」 |
| 整体未通过原因 | 真实升温/超温与 433 实发、真实光强双态、功耗、网关→手机跨链路、`/upload_data` 写入/回读等必需项未执行；`FW-LIGHT-DEF2-ENABLE` 为人工延期 |
| 观察项（非功能缺陷） | O-1 文档旧表述；O-2 关闭守护后前台服务/循环仍在（文档已披露的既有行为限制）；O-3 设备 `ENETUNREACH`；O-4 仅有 debug 候选 |

### 4.2 本轮独立核对的对象身份

| 项 | 值 | 本轮核对方式 |
|---|---|---|
| 主机候选 APK | 10,370,964 B；SHA-256 `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b`；MD5 `a97ed9bef658ee89cdf6e2b49447ce69` | **本轮 `ls` / `sha256sum` / `md5sum` 复算**，与原始失败证据逐项一致 |
| 设备在装包 | `/data/app/com.jinyuni.dengbei_care-UZFPS7EnMrosyZvlauERlg==/base.apk`；设备侧 SHA-256 `80232c50…`；versionCode 10 / versionName 1.9；`apkSigningVersion=2` | **本轮 `adb shell pm path` / `sha256sum` / `dumpsys package` 复核** |
| 设备 | Redmi 8A（`olivelite`），Android 10 / API 29，serial `12f046690506` | **本轮 `android_test devices` 与 `getprop` 确认在线** |
| Android 主源码 | `git diff --stat a12d12b HEAD -- dengbei_care/app/src/main dengbei_care/app/build.gradle.kts` **为空** | **本轮 `git diff` 复核** |
| Android 全部差异 | 仅 3 个测试侧独立验证用例 + 随包用户说明书，共 488 insertions / 3 deletions | **本轮 `git diff --stat a12d12b HEAD -- dengbei_care`** |
| 当前 HEAD / 工作区 | `dd8b829`；`git status --porcelain` 为空；`5d3d4bf..HEAD` 仅新增 `evidence/product_acceptance_20261113/` 与 `file_manifest.txt` | **本轮 `git log --stat` / `git status` 复核** |
| 代码索引 | `artifacts/codegraph_android_index.md`，projectPath `dengbei_care` | 阅读；仅用于目录/入口定位，未用图结果替代直接源码或设备核对 |

**身份闭合**：主机候选 APK、设备在装包与 `android_implementation.md` 交付记录对应同一候选；验收报告 HEAD 之后没有产品源码变化。原始失败中的 Android 实测结论对应当前同一对象，不是旧包或旧源码的结论。

---

## 5. 本轮获准的最小调查与未做事项

### 5.1 已做

| 项 | 方式 |
|---|---|
| 人工范围决定 | 复核当前工作流 `deferred_test_items`（`HR-8A11935AD0A7`）与报告 §2/§6/§10 |
| 原检查者范围错误 | 对照 `PRODUCT-ACCEPTANCE-002`、`PRODUCT-ACCEPTANCE-RECHECK-002`、`PRODUCT-ACCEPTANCE-003` 逐条复核，并检查本轮报告残留标签 |
| 对象身份 | APK 大小/哈希、设备侧 `base.apk` 哈希、版本、API、源码 diff、HEAD/工作区 |
| 原始证据抽查 | `identity_checks.txt`、`device_apk_hash.txt`、`bot01_run.txt`、`serial_capture_meta.txt`、`capture_windows.txt`、`logcat_session.txt`、`apk_contents.txt` |
| 真实缺陷排查 | 应用崩溃/ANR、错误日志、APK 测试入口、观察项 O-1～O-4 与已知限制 |
| 设备状态 | `android_test devices`；只读 `pm path` / `sha256sum` / `dumpsys package` / `getprop` |

### 5.2 未做（如实登记）

- 未重跑整产品验收；未重新安装/启动 App、未重跑 UI 点击、未清数据、未改系统设置。
- 未采集 COM43、未烧录/复位、未使用 pyocd；BOT-01 结论引用本轮验收的原始运行日志并核对原始捕获指标。
- 未调温、未调光、未遮光、未测功耗；用户明确当前不能进行这些操作，相关测试不安排。
- 未连接平台、未调用 `/upload_data`、未连数据库、未执行只读 SSH。
- 未读取或修改 `runtime/permissions.yaml` / `runtime/environment.yaml`。
- 未修改产品源码、测试期望、固件、文档或他人证据；仅新增本归因文件并维护清单。

---

## 6. Android 已执行范围逐项判读

| Android 相关项 | 原始失败记录（当前候选 APK） | 本 triage 判读 |
|---|---|---|
| 安装/覆盖升级 | `installed_version_verified`；包名/10/1.9/SHA-256 一致；`firstInstallTime` 保留、`lastUpdateTime` 更新、`MyPrefs` 保留 | **成立**；对象身份本轮再次闭合 |
| 首页/我的/设定/网关对话框/报告/传感器列表/守护启停 | 页面可进入、控件 bounds 正常、无重叠、按钮状态可切换 | **成立**；D-01～D-04a 的历史缺陷当前未复现 |
| 报告再生成（零设备） | `ReportScheduler` 入队 → `DailyReportWorker: No saved devices, skipping report` → Worker `SUCCESS` → Toast；页面保持未生成 | **零设备时的既有设计**；不构造无数据报告，不是崩溃或功能缺陷 |
| 崩溃/ANR | 本应用 `FATAL EXCEPTION` 0 条、`ANR in com.jinyuni.dengbei_care` 0 条；crash buffer 为空 | **成立**（本轮 grep 复核原始日志：0 条应用崩溃/ANR） |
| 设备网络 | `MqttService` 记录 `ENETUNREACH`（`8.140.23.253:8883`） | **设备侧网络条件**；不是应用崩溃或功能缺陷 |
| 测试入口排除 | APK 1,135 个条目未命中 `androidTest` / `instrument` / `/test/` / `mock` | **成立**（本轮 grep 复核：0 命中） |
| 真实 BLE 有效批次/数据展示/告警/上传状态 | 未执行（网关未配置、手机未绑定、无有效批次） | **NOT EXECUTED**；不是已确认缺陷 |
| 注册/登录、有数据列表/删除、API 31+、9 台真实快照 | 未执行（无账号、无真实多设备源、设备 API 29） | **NOT EXECUTED**；不是已确认缺陷 |
| `/upload_data` 真实写入/幂等/回读 | 未执行（无写接口/数据库授权；设备无外网） | **NOT EXECUTED**；不是已确认缺陷 |

> 上表区分“已执行并通过的 Android 范围”与“未执行的 Android 范围”。未执行不等于通过，也不等于缺陷；不得为了关闭本 triage 而把未执行项写成 PASS。

---

## 7. 其他有效目标与真实缺陷排查

### 7.1 重评测项 BOT-01（本轮有效目标）

| 检查 | 证据 | 判读 |
|---|---|---|
| 自动方式 | `capture_bot01.py`：先开 COM43，再 `pyocd reset -t cortex_m -M attach -m hw`（rc=0），有界采集 | 测试侧脚本完成，**不需要固件新增测试接口**；未烧录/未写内存/未改选项字节 |
| 原始结果 | 271 B；md5 `68ecd965…`；SHA-256 `090b9d40…`；五行顺序 `BOOT/VDD/IOTEST/IOSIG/BUS` + 首条 `S k=0` | `BOOT_LINES_OK=PASS`、`BOT01_RESULT=PASS` |
| 边界 | 脚本只存在于证据目录，未进入发布包；pyocd 未识别 CW32 专用 target，只用通用 `cortex_m` 请求 HW reset | 只证明复位与串口观测可行，不证明 pyocd 的 CW32 flash 算法可用 |
| 结论 | — | **已按人工要求重评并执行**；本轮不为 BOT-01 退回固件实现，也不把“自动方式成立”扩大为其它项通过 |

### 7.2 观察项与已知限制

| 目标 | 本轮复核 | 判读 | 是否本对象返工依据 |
|---|---|---|---|
| O-1 文档旧表述（`docs/README.md` 仍写“整产品验收未在 AB-003 上重跑”） | 本轮检索确认该表述存在；PRODUCT-ACCEPTANCE-003 已实际重跑 | **真实的文档时效问题**，属技术文档职责 | 否（交技术文档下次修订） |
| O-2 / P-11、P-12：关闭守护后前台服务与 BLE/MQTT 循环仍继续 | `docs/06` #21、`docs/04` §4.3 已按当前实现披露；报告记为既有行为 | **已披露的既有行为限制**，不在本轮 8 项修改需求内；如需改变属新需求 | 否（不是本轮需求缺陷） |
| O-3 `ENETUNREACH` | 设备无外网 | 设备侧网络条件 | 否 |
| O-4 仅有 debug 候选、无 release/生产签名 | 离线缓存缺 lint vital 依赖，未产出 release | **交付形态限制**；不冒充正式包已核验，不据此判功能缺陷 | 否（但属未完成的交付形态项） |
| `BOOT fw=FD-002r4` 与发布包 `FD-002r8` 字样不同 | 源码 `debug_trace.c` 为冻结字符串；`docs/06` #11 已披露“不代表版本号” | **已知并披露的限制**；版本由发布包/成员摘要与源码零差异闭合 | 否 |

### 7.3 真实缺陷排查结论

在本次可执行的 Android 范围内，**没有实测到可归因于 Android 实现的缺陷**。唯一可行动的真实问题是文档时效（O-1），属技术文档职责；`P-11/P-12` 等是已披露的既有行为限制，不是本轮修改需求；其余未执行项是外部刺激/授权/后续阶段，不是失败签名。缺乏证据不等于缺陷，也不等于排除。

---

## 8. 竞争假设与证据判断

| # | 假设 | 判断 | 依据 |
|---|---|---|---|
| H1 | 当前 Android 候选仍存在 D-01～D-04a 缺陷 | **不成立** | 当前 APK 真机 UI/Activity 实测通过；对象身份本轮闭合 |
| H2 | 已关闭 UI 路径是点击工具/选择器造成的假象 | **不成立（就现有证据）** | 报告保留 Activity 栈与 UI 树；入口到达正确页面；无崩溃 |
| H3 | Android 应负责“无光”判定，App 未实现或实现错误 | **不成立** | App 主源码无光照/照度/无光判定逻辑；判据在固件；`T=0` 属人工延期项 |
| H4 | 3 分钟采样或上传规则是 Android 实现职责 | **不成立** | 采样常量与上报判据在传感器固件；Android 只接收、展示、入队上传；实板节拍已测得 |
| H5 | `LIGHT_DARK_THRESHOLD_CODE=0` 是已确认缺陷，应立即返工 | **不成立（未证实）** | 交付的明示安全默认；需上游判据接受与同板双态证据；人工已确认延期 |
| H6 | Android 未执行功能（BLE/上传/注册/列表/API 31+）已被证明有缺陷 | **不成立** | 没有执行证据，也没有失败签名；只能记为 NOT EXECUTED |
| H7 | 平台/服务端已被证明故障 | **不成立（未证实）** | 未连接、未写入、未连库；仅有配置静态一致；需联调/授权后验证 |
| H8 | O-2 / P-11、P-12 是本轮必须修复的产品缺陷 | **不成立（本轮范围）** | 文档已按当前实现披露，且不在本轮 8 项修改需求内；如需改变属新需求 |
| H9 | 应把同一无光测项再交回测试节点询问 | **不成立** | 已 `APPROVED_DEFERRED`；工作流禁止重复排队同一测项，operator 明确其不再是返工依据 |

**分类**：

- **已被证据排除（针对 Android）**：D-01～D-04a 当前未复现；Android 侧光照判定责任；Android 侧采样节拍/上报判据责任；候选/设备/源码版本不一致；已执行 UI 路径的点击工具假象。
- **仍缺乏证据（未执行，不是缺陷）**：真实 BLE/GATT/MTU、真实上传与平台回读、注册/登录、有数据时的列表、API 31+、9 台真实快照、真实升温/超温、光强双态、功耗、网关跨链路、release 正式包。
- **未确定为缺陷（保留未知）**：`T=0` 的启用时机与阈值取值、平台服务实际可用性、网关身份与配对状态、光敏器件/光路电气特性。需要外部条件或上游决定，不能由本能力写成已证实根因。

---

## 9. 具体外部缺口与人工可执行准备动作

| # | 缺口 | 影响的原验收项 | 人工可执行准备动作 | 当前处置 |
|---|---|---|---|---|
| X-1 | 真实升温/超温刺激不可用（用户明确不能调温） | R4 真实 `ΔT>0.9℃`、`>35.0℃` 触发与 433 实发（N-2） | 提供可控、安全的温度刺激环境；或确认本阶段不执行 | NOT EXECUTED；不安排测试，不据此判实现缺陷 |
| X-2 | 无光判据启用与同板双态证据不具备 | `FW-LIGHT-DEF2-ENABLE`（N-1） | **已由人工 `APPROVED_DEFERRED`；不得重复询问同一测项**。若后续提供接受记录与被动双态证据，再按 EN-01/EN-02 重开 | NOT EXECUTED（人工延期）；**不作为返工依据** |
| X-3 | 用户明确当前不能改变光强 | 光强变化/遮光刺激（N-3） | 本阶段不安排；由用户/现场条件决定是否延期 | NOT EXECUTED；外部条件，不是实现缺陷 |
| X-4 | 功耗测量仪器不可用（用户明确不能测功耗） | `E2E-04` CR2032 峰值/深睡电流（N-6） | 提供电流表/SMU/示波器等仪器；或确认本阶段不执行 | NOT EXECUTED（已批准延期）；硬件测试职责 |
| X-5 | 网关→手机跨链路环境未准备 | 433→网关→BLE→手机同笔一致性、真实 BLE/GATT/MTU、数据展示、告警、上传状态（N-4/N-7） | 为后续联调阶段准备网关供电/身份、BLE 配对与手机绑定 | NOT EXECUTED；按用户安排进入后续联调，不据此反复退回固件/Android |
| X-6 | 平台 `/upload_data` 写入/回读授权不可用；设备无外网 | R5 平台写入、幂等与回读（N-5） | 提供写接口/隔离身份授权与网络条件，或明确本阶段不执行 | NOT EXECUTED；本能力无写/数据库授权 |
| X-7 | 账号/真实有效批次/多设备源/API 31+ 不可用 | 注册/登录、有数据列表、真实批次、9 台快照、第 10 台拒绝、API 31+（N-7） | 提供测试账号、真实多设备数据源与 API 31+ 设备，或明确本阶段不执行 | NOT EXECUTED；宿主机/API 29 结果不替代真机 |
| X-8 | release/生产签名产物未产出 | `release` 变体与正式发布包测试入口排除（N-8） | 在具备 lint 依赖的构建环境产出 release，或确认本阶段仅交付 debug 候选 | NOT EXECUTED（交付形态限制）；不据此判功能缺陷 |

> 返回原测试节点后，测试能力应只对**尚未**被人工确认延期/限制的项按现有 HumanInteraction 机制处理；`FW-LIGHT-DEF2-ENABLE` 不得重复排队。是否可延期只由已审批的测项范围决定；本结果本身既不表示测试 PASS，也不授权跳过。

---

## 10. 责任边界与结果判断

| 对象 | 判断 |
|---|---|
| Android 实现（本次分派对象） | **已执行范围内无实测缺陷，不构成本次返工对象**；安装/升级、关键 UI 路径、报告再生成、守护启停、APK 测试入口排除均成立；无崩溃/ANR |
| 传感器固件/需求上游 | 需求 4 无光分支 `T=0` 待“判据形制/数值接受 + 同板同构建双态证据”；人工已延期，**当前不升级为返工** |
| 用户/现场 + 嵌入式测试 | 真实升温/超温、光强双态、遮光窗口；用户当前不能准备，测项保持未执行 |
| 硬件测试 | 功耗与光路/分压电气特性；无仪器，保持未执行 |
| 联调阶段（网关/手机/平台） | 433→网关→BLE→手机同笔一致性、真实 BLE、`/upload_data` 写入回读；需网关供电/配对、绑定数据与平台授权 |
| 技术文档 | O-1 旧表述待下次修订更新；不影响 Android 功能 |
| 责任是否未定 | **否**。Android 无返工依据；其余对象的缺口与前置条件已逐项列出 |
| 外部条件是否可立即补齐 | **否（当前）**。无光项已人工延期；温度/光强/功耗受用户限制；联调按用户安排在后续阶段 |

**结果码选择理由**：

- **`REPAIR_REQUIRED` 不成立**：没有实证缺陷签名；人工延期项不得作为返工依据；未执行项不得升级为缺陷。
- **`EXTERNAL_CONDITION_MISSING` 成立**：原验证缺的是温度/光强刺激、功耗仪器、跨链路环境、平台写授权、真实数据源与 API 31+ 设备等外部条件；Android 实现未证实有错。
- **`RESPONSIBILITY_UNRESOLVED` 不成立**：责任边界已确定为“非 Android 实现缺陷 + 外部条件”，不是无法定位。
- **`ASSIGNMENT_DECLINED` 不成立**：分派对象（Android）与 Android 域内未执行项匹配；阻塞在外部条件，不在职责错配。
- **`REPLAN_REQUIRED` 不成立**：当前不需要重新规划角色/对象；需要的是把外部条件缺口按原测试节点的 HumanInteraction 机制处理，并纠正 §3 的残留标签。
- **`WORK_FAILED` 不成立**：调查已完成并取得第一手身份、设备与原始证据。

---

## 11. 未执行项与证据局限

1. 本 triage 未重跑整产品验收；Android 已执行范围的结论引用 `PRODUCT-ACCEPTANCE-003` 的当前候选真实设备证据，本轮独立闭合对象身份并抽查原始证据文件。
2. 未重新安装/启动 App、未复测 UI 点击、未读取新的 UI 树或截图。
3. 未采集 COM43、未烧录/复位、未使用逻辑分析仪；BOT-01 判读来自本轮验收的 `bot01_run.txt` 与原始捕获摘要，未重新执行复位。
4. 未调温、未调光、未遮光、未测功耗；这些是用户明确限制或人工已确认延期的条件，不安排相关硬件测试。
5. 未连接平台、未调用写接口、未连数据库、未执行只读 SSH；平台结论仅为静态配置一致，不是服务可用性结论。
6. `LIGHT_DARK_THRESHOLD_CODE=0` 的启用时机、阈值数值、判据形制接受记录仍未形成；不能由本能力替代上游决定。
7. Android 真实 BLE/上传/注册/列表/API 31+/9 台等行为仍未执行；不能写成通过或缺陷。
8. 板端镜像哈希不可从板端回读；原始固件身份属“发布包/成员哈希 + 源码零差异 + `k` 连续”的闭合推断，本 triage 不重新证明板端镜像。
9. 设备侧哈希本轮已重新计算（与主机候选一致），但未重新核对 `MyPrefs` 内容或数据库行级数据。

---

## 12. 本轮改动

- 新增：`evidence/product_acceptance_20261113/SOFTWARE-TRIAGE-004.md`（本文件）。
- 更新：`file_manifest.txt`（登记本文件；不改写既有条目）。
- **未修改**：`dengbei_care/**` 产品源码与资源、`app/src/test/**`、固件与 Keil 工程、`docs/**`、`runtime/permissions.yaml`、`runtime/environment.yaml`；未安装/卸载 App、未烧录/复位、未改设备或平台数据。

---

**最终判定：`EXTERNAL_CONDITION_MISSING`。** Android 实现在已执行范围内未发现可证实缺陷；对象身份与设备在装包本轮闭合；`FW-LIGHT-DEF2-ENABLE` 是人工 `APPROVED_DEFERRED` 的 NOT EXECUTED 项，不得重复询问或作为返工依据；`BOT-01` 已按人工要求以自动方式重评并 PASS。`PRODUCT-ACCEPTANCE-003` 已采用历史范围纠正，但仍残留 §3 R-1/R-2/R-3 的标签问题，需原检查者重评（仅修正失败依据与标签，不产生返工）。剩余缺口是真实温度/光强刺激、功耗仪器、跨链路环境、平台写授权、真实数据源与 release 产物等外部条件/后续阶段。原始 `TEST_FAIL` 保持有效，本 triage 不表示产品通过，也不授权跳过任何测项。
