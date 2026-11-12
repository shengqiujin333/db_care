# 固件对象失败归因（FIRMWARE-TRIAGE-002）

- 负责角色：嵌入式测试
- 能力：嵌入式失败分析（`product_firmware_triage`）
- 分析对象：`artifacts/firmware_implementation.md`（固件实现 FWI-003 rev 8；发布包 `fw_release/fw_sensor_FD-002r8.zip`，源码提交 `82a7e74`）
- 原始失败：`evidence/product_acceptance_20261112/PRODUCT-ACCEPTANCE-002.md` **`TEST_FAIL`**（`failure_cause=EXTERNAL_CONDITION_UNEXECUTED`）
- 代码索引：`artifacts/codegraph_firmware_initial_index.md`（sensor projectPath，索引基线 `82a7e74`；本文件同时直接核对当前源码）；`artifacts/codegraph_android_index.md` 属 Android 对象，非本 triage 对象
- 分析时点：2026-11-12 12:10–12:20（Asia/Shanghai；工具时间 UTC 04:10–04:20）
- 分析时 HEAD：`e828d2130664d3f75cfcf82a6fd4eba832a30d9c`，工作区干净
- **本轮结论：`RESPONSIBILITY_UNRESOLVED`（已完成获准调查，仍不能确定合法的修复对象/授权）**

> 本文件只做失败归因，不是复验通过、不是修复成功，也不表示产品合格。
> 本轮未修改产品源码、固件、测试期望、文档、权限或环境配置；未烧录、未复位、未发送串口字节、未调温/调光/遮光、未测功耗、未访问平台写入接口或数据库。
> 原始 `TEST_FAIL` 仍然有效；未执行项保持未执行，不写成通过。

---

## 1. 结论摘要

1. **原始失败对固件对象的可执行部分没有测出实现缺陷。** `PRODUCT-ACCEPTANCE-002` 对固件的结论是：R1（GXHT40 读数）、R3（3 分钟节拍）、R8（UART1 周期行 + 上电五行复用）、R9（光通讯/霍尔退役）在已执行范围内成立；R2 采样有效但判据未启用；**唯一未满足的固件功能项是 R4 的“无光”分支**，且该项被记录为“未满足/未执行”，不是实测到的错误行为。
2. **R4 无光分支不可用的直接原因是交付默认值 `LIGHT_DARK_THRESHOLD_CODE=0`（判据未启用）**，不是判定公式写错：`sensor_decide_report` 已按需求实现 `(cur > 35.0℃) 或 (有前值 且 (cur-prev) > 0.9℃ 且 dark)`；`T=0` 时 `valid=false → dark=false`，升温分支按设计恒不触发，只保留 >35.0℃ 分支（源码与 `fw_release/PACKAGE.txt` 明示）。
3. **启用该判据的前置条件不是设备读数问题，而是两项外部前置**：EN-01 需要一份 LIGHT-DEF-2 判据形制与数值的接受记录（含 IC-002 §4.2 同步），EN-02 需要同板同构建的受光/无光**被动**双态证据。当前两者均不存在；测试设计明确“缺少接受记录时不得执行后续 EN 用例，不得自行选择/回填阈值”。
4. **本 triage 本轮做了授权范围内的最小实测**：COM43/9600 8N1 被动采集 300 s，取得 `S k=975`、`S k=978` 两行，`V=0 D=0 o=8 q=1 r=0 s=0 y=0`，无 `P`；`k` 与上一窗口（`k=906`，03:02:42Z）连续，说明板端仍按 `T=0` 交付行为运行、未发生复位归零。该实测只确认“现状与设计默认一致”，**不能**证明无光判定可用或不可用。
5. **不能用 `REPAIR_REQUIRED`**：没有实测证据证明本次具体可修改对象（固件实现）需要修复；唯一缺口是“判据未启用”，而启用必须由上游接受记录与双态证据授权，直接回填候选值 3590 会把设计明确禁止的猜测升级为返工。
6. **不能用 `EXTERNAL_CONDITION_MISSING`**：该结果会把流程送回原测试节点再次向人询问同一测项；`FW-LIGHT-DEF2-ENABLE` 已由人工确认 `SKIP_CONFIRMED`（2026-11-11T03:50:42Z，「无法准备，先跳过」），当前工作流明确不得重复排队同一 `test_item_id`。再次询问不会产生新条件。
7. **不能用 `ASSIGNMENT_DECLINED`**：本次分派的固件对象就是 R4 的实现载体，领域匹配；问题不是“对象错了”，而是“下一步动作的合法授权对象在当前图中不存在/未确定”。
8. **结论 `RESPONSIBILITY_UNRESOLVED`**：需要重新规划，明确由谁接受 LIGHT-DEF-2 形制与数值、以何种证据形式关闭 EN-01/EN-02；在此之前固件保持 `T=0`，不得返工或把未执行写成通过。

---

## 2. 原始失败引用与对象身份

### 2.1 原始失败（引用，不改写）

来源：`evidence/product_acceptance_20261112/PRODUCT-ACCEPTANCE-002.md`（负责角色：软件测试；能力：产品整体功能验收）。

| 项 | 原文事实 |
|---|---|
| 验收对象 | 传感器固件 FD-002 rev 8 发布包 `fw_sensor_FD-002r8.zip`、安卓 1.9 debug 候选、既有网关与平台服务、文档包 rev 12 |
| 结论 | `TEST_FAIL`（`failure_cause=EXTERNAL_CONDITION_UNEXECUTED`） |
| R1 GXHT40 | 通过（读数路径，有限）：`q=1`、`t/h` 合理；EV-038 位级 I²C `0x44` 双 CRC |
| R2 光敏采样 | 部分：ADC 采样有效（`o=8`、`n/x/a` 域内且逐周期变化）；`V=0 D=0` 为 `T=0` 默认，不得判为“已识别有光” |
| R3 3 分钟 | 通过（实板节拍）：300 s 窗口 `k` 906→909 步长 3 |
| **R4 上传规则** | **未满足/未执行**：公式已实现，但 `T=0` ⇒ 无光分支永不触发，仅 >35℃ 分支保留；真实升温/超温未执行 |
| R8 UART1 | 通过（周期行实测；上电五行按人工 REASSESS 复核后复用同镜像 EV-038 证据，非本轮新采集） |
| R9 退役 | 通过（静态）：仅退役注释残留，PB04 已改 `LIGHT_ADC` |
| 未执行 | N-1 无光判据（`SKIP_CONFIRMED`）、N-2 真实升温/超温、N-3 光强变化、N-4 跨链路、N-5 平台写入、N-6 功耗、N-8 BLE 等 |

### 2.2 本 triage 独立核对的对象身份

| 项 | 值 | 本轮核对方式 |
|---|---|---|
| 分析对象 | `artifacts/firmware_implementation.md`（FWI-003 rev 8） | 阅读 |
| 发布包 | `fw_release/fw_sensor_FD-002r8.zip`，134,791 B，md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf` | **本轮 `md5sum` / `unzip -l` 复算一致** |
| 包成员 | axf `b6a15fed2ed5e3d0c4adc51ac87a4830`（297,380 B）、hex `4d728dec9b0d28a352c46436b7586c72`（75,275 B）、PACKAGE.txt（3,202 B） | **本轮 `unzip -l` 核对** |
| 产品源码 | 提交 `82a7e74e5d6bae855a573e1f23f83936ba8db049`；`git diff --stat 82a7e74..HEAD -- <sensor 工程>` 仅 5 个 `test/` 宿主 harness/mock 文件（`host_fw_core_verify_ev.c`、`host_light_verify_ev.c`、`host_rf_report_verify_ev.c`、`host_vdd_verify_ev.c`、`mock_mcu/mock_cw32.h`），**产品源码与工程文件零差异，且 5 个文件不在发布包内** | **本轮 `git diff --stat` 复核** |
| 当前 HEAD | `e828d2130664d3f75cfcf82a6fd4eba832a30d9c`，工作区干净 | **本轮 `git rev-parse` / `git status` 复核** |
| 板端镜像 | 板端哈希不可回读；由“EV-038 下载记录 + 源码零差异 + `k` 连续”闭合为推断 | 原始验收 §3.5 + 本轮串口连续性计算 |

**身份闭合**：原始失败中的固件发布包、当前源码基线与本轮读数对应同一交付对象；`test/` 下 5 个验证 harness 变更不进入发布包，不改变被测二进制。

---

## 3. 本轮最小实测（真实目标，授权范围内）

### 3.1 COM43 被动采集（未发送字节、未复位）

- 端口：COM43，Prolific `067B:2303`，9600 8N1；`serial_list_ports` 本轮确认在线（COM26 为 CW-DAPLink 附带串口，未用作传感器调试口）。
- 窗口：**2026-11-12T04:11:14.853Z – 04:16:15.050Z（300 s）**；收到 **160 B**，md5 `ca04706ec56ffe4ecead5298204669fd`，SHA-256 `daeed8014b75ef8dce60988c470a4be315b2498bce01a6c32dbb3b390411f40f`。
- 原始两行（CRLF 结尾，逐字节保留于 `serial_capture_com43_20261112_triage.bin`；仓库内以 `serial_capture_hex_triage.txt` 复核）：

```text
S k=975 p=254 H=1 V=0 o=8 n=2893 x=2895 a=2893 D=0 t=255 h=421 q=1 r=0 s=0 y=0
S k=978 p=255 H=1 V=0 o=8 n=2891 x=2897 a=2894 D=0 t=254 h=425 q=1 r=0 s=0 y=0
```

判读（只记录事实）：

1. `k` 步长 **3**，与 `SENSOR_RTC_TICK_PERIOD_MIN=1`、`SENSOR_SAMPLE_TICKS=3` 一致；窗口内无其它字节。
2. `V=0 D=0`、`r=0 s=0 y=0`、无 `G`/`P`：与交付默认 `T=0` 的既定行为一致；**不得**解释为“已识别有光”或“光路故障”。
3. `o=8`、`q=1`、`t=25.5/25.4℃`、`h=42.1/42.5%RH`，`n/x/a` 在 `n` 上下小幅波动，说明本周期 ADC 有采样、GXHT40 读数有效。
4. **连续性**：上一窗口（`PRODUCT-ACCEPTANCE-002` §3.1）起点 03:02:42.532Z 为 `k=906`；`k=975` 距其 +69，对应名义时间 04:11:42.532Z，落在本轮窗口内，偏差在 LSI 量级。与“同一启动会话连续运行、未再复位”一致。
5. **局限**：板端镜像哈希不可回读；该连续性只是推断，不替代独立镜像哈希证明。

原始：`serial_capture_com43_20261112_triage.bin`（本机留存，`.gitignore` 忽略）、`serial_capture_hex_triage.txt`、`serial_capture_meta_triage.txt`。

### 3.2 本轮未执行的实测

- 未复位/未烧录：`sensor_programming` 对本能力 `unavailable_for_current_capability`，且本轮无需换镜像。
- 未取得上电五行：沿用人工 REASSESS 已核实的 EV-038 同镜像证据，未重采。
- 未调温、未调光、未遮光、未测功耗：用户明确当前不能进行，且测试设计 EN-02 要求被动观测、不得以遮光/照射取数。
- 未做逻辑分析仪采集：本 triage 的区分点不在 I²C/电气波形；光判据是固件内部 ADC 与常量，非总线可观测项。

---

## 4. 已证实事实（源码、构建身份、文档）

| # | 事实 | 位置/证据 |
|---|---|---|
| F1 | 判据默认禁用：`#define LIGHT_DARK_THRESHOLD_CODE 0u /* 0 = 未启用 (唯一禁用值) */` | `USER/inc/sensor_config.h`（本轮直接读取） |
| F2 | `valid = adc_ok && (LIGHT_DARK_THRESHOLD_CODE != 0)`；`dark = light_is_dark(mean, valid, T)` | `USER/src/light.c:203-204` |
| F3 | `light_is_dark`：`!valid → false`；`threshold==0 → false`；`mean >= T` 为暗 | `USER/src/fw_core.c:93-104` |
| F4 | 上报判定：`cur > 350 (35.0℃)` 或 `have_prev && (cur-prev) > 9 (0.9℃) && dark`；严格大于边界 | `USER/src/fw_core.c:109-127`、`sensor_config.h` 上报段 |
| F5 | 采样节拍：1 分钟 tick ×3；实板 `k` 步长 3 | `sensor_config.h:26-27`、本轮与上轮串口 |
| F6 | 发布包 PACKAGE.txt 明示：“No-light criterion is DISABLED in this delivery … Enabling the criterion requires an accepted criterion value plus passive lit/dark evidence; that test item is human-confirmed deferred (NOT EXECUTED)” | `fw_release/PACKAGE.txt:41-44` |
| F7 | 接口合同未批准替代阈值：“当前需求未给替代阈值，本版不批准中点、350/250、滞回、温补、取消光门或硬件改动…在此之前保持 UNKNOWN，不声称无光功能完成” | `artifacts/interface_contract.md:85`（§4.2） |
| F8 | 测试设计门禁：EN-01 缺少接受记录时“不得执行后续 EN 用例；只记录 NOT EXECUTED 与缺项；不得自行选择/回填阈值”；EN-02 要求同板同构建被动双态，且“不得以遮光/照射作为测试手段” | `artifacts/test_design.md` §5.5 |
| F9 | 该测项已人工确认跳过：`FW-LIGHT-DEF2-ENABLE`，`SKIP_CONFIRMED`，2026-11-11T03:50:42Z，「无法准备，先跳过」 | 当前工作流 `deferred_test_items`（HR-8A11935AD0A7） |
| F10 | 设计与实现均声明：`T` 保持 0；启用需判据接受记录 + 同机同位置被动双态证据（FWR-120）；本轮不具备 | `artifacts/firmware_design.md` §0/§3.2、`artifacts/firmware_implementation.md` §0 |

---

## 5. 竞争假设与排除

| # | 假设 | 判断 | 依据 |
|---|---|---|---|
| H1 | 固件遗漏启用无光判据，是代码缺陷 | **不成立** | `T=0` 是 FD-002 rev 8 明示的交付默认，注释/PACKAGE/设计/任务元数据一致；不是遗漏 |
| H2 | 上报公式实现错误 | **不成立** | F4 与需求公式逐项一致：严格 `>0.9℃`、AND `dark`、OR 严格 `>35.0℃`；得 0.9 不触发、35.0 不触发超温分支 |
| H3 | 光敏采样通路损坏 | **不成立（就采样事实而言）** | `o=8`、`q=1`、`n/x/a` 域内且逐周期变化；但**绝对明暗分类无法证实/证伪**，因无无光样本 |
| H4 | 缺接受记录等于固件实现缺陷 | **不成立** | 它是上游判据接受事项；固件设计已给出形制与候选值，尚无接受记录；实现无权自批 |
| H5 | 应直接用候选 3590 回填并重建 | **不成立（当前）** | EN-01/EN-03/EN-06 与 IC-002 §4.2 明确禁止在无接受/无双态证据时回填；会制造无法验证的分类 |
| H6 | 板端可能未运行交付镜像 | **部分缓解，未完全排除** | 包/成员哈希一致、产品源码零差异、`k` 连续；板端哈希不可回读，属推断 |
| H7 | UART1 调试通道故障 | **不成立（已执行范围）** | 两行格式完整、≤96 B、CRLF、步长 3；无杂散字节 |
| H8 | >35℃ 分支已损坏 | **未证实** | 未做升温刺激，无失败签名；不能把未执行当缺陷 |
| H9 | 软件/平台/网关侧导致固件失败 | **不成立** | 原始失败对固件对象的未执行项均为物理刺激/上游接受/后续联调；无跨对象错误签名 |
| H10 | 把同一无光测项再交回测试节点询问即可解决 | **不成立** | AI 已 `SKIP_CONFIRMED`；工作流禁止重复排队同一测项；缺的是接受记录而非新设备条件 |

**区分**：
- 已被证据排除（针对固件实现）：公式错误、采样通路损坏、UART1 故障、光通讯/霍尔残留、源码漂移。
- 仍缺乏证据（未执行，不是缺陷）：无光分类正确性、真实升温/超温触发、跨链路、功耗。
- 未确定为缺陷（保留未知）：LIGHT-DEF-2 数值是否足够分隔明暗、接受责任归属、板端镜像哈希。

---

## 6. 责任边界（逐项）

| 项 | 状态 | 责任/边界 |
|---|---|---|
| R1 GXHT40 | 已执行范围内成立 | 固件实现；无返工依据 |
| R2 光敏采样 | 采样有效，分类未知 | 采样属固件；分类需 EN-01/EN-02 前置 |
| R3 3 分钟 | 成立 | 固件实现；无返工依据 |
| **R4 无光分支** | **未启用/未执行** | 判据形制与数值接受（上游/需求/架构）→ 双态被动证据（现场/测试）→ 固件回填重建。**当前不能确定合法修复对象** |
| R4 >35℃ 分支 | 未执行 | 用户不能调温；外部条件，不安排测试 |
| R8 UART1 | 成立（周期行实测；上电五行复用） | 固件实现；无返工依据 |
| R9 退役 | 成立（静态） | 固件实现；无返工依据 |
| N-1 无光测项 | `SKIP_CONFIRMED`，NOT EXECUTED | 不得重复排队；等待判据接受与证据规则明确 |
| N-2 真实升温/超温 | NOT EXECUTED | 用户条件限制；不是固件缺陷 |
| N-3 光强变化/遮光 | NOT EXECUTED（不安排） | 用户条件限制；EN-02 亦禁止以遮光/照射取数 |
| N-4 跨链路 | NOT EXECUTED | 用户安排后续联调；非固件阶段门禁 |
| N-5 平台写入 | NOT EXECUTED | 无写/数据库授权；非固件对象 |
| N-6 功耗 | NOT EXECUTED | 硬件测试职责；无仪器 |
| N-8 手机 BLE/展示 | NOT EXECUTED | 后续联调；非固件对象 |

**结论**：原始失败中没有“固件实现已证实需要修复”的项；R4 的缺口是**前置责任未确定**，不是实现错误。

---

## 7. 结果判断

- **不选 `REPAIR_REQUIRED`**：无实证缺陷签名；`T=0` 是有意交付默认，回填阈值被 EN-01/IC-002 明确禁止。
- **不选 `EXTERNAL_CONDITION_MISSING`**：会把同一 `FW-LIGHT-DEF2-ENABLE` 送回原测试节点重复询问；已 `SKIP_CONFIRMED`，且缺的是上游接受记录而非可立即准备的设备条件。
- **不选 `ASSIGNMENT_DECLINED`**：分派对象（固件）与失败项（R4）领域匹配，不存在对象错配。
- **不选 `WORK_FAILED`**：调查已完成并取得第一手读数。
- **选择 `RESPONSIBILITY_UNRESOLVED`**：已完成获准调查，仍不能确定合法的修复对象/授权——判据接受责任在当前流程中未指派，物理双态证据在用户条件下不可取得；需要重新规划。

---

## 8. 最小进一步调查与复验触发条件

| 项 | 最小动作 | 完成后的复验目标 |
|---|---|---|
| 判据接受（EN-01） | 由授权方（需求/架构，必要时经人工确认）记录 LIGHT-DEF-2 形制与数值（候选 3590，可用带 `(3514,3660]`，或改判），并同步 IC-002 §4.2 | 存在可引用的接受记录；否则不得启用 |
| 双态证据（EN-02） | 同板、同构建、同摆放的**被动**受光/无光样本；按 EN-02 不得以遮光/照射取数 | 至少各 1 条不同周期 `S`（`mean<T` 与 `mean>=T`），`o=8`、`n/x` 未饱和 |
| 启用与重建（EN-03） | 固件仅改唯一常量并 rebuild、下载（需 `sensor_programming` 授权） | 配置 diff + 0 Error + `Verify OK` + 新摘要 |
| 实板复验（EN-04/05） | 受光/无光分类、边界 `mean==T`、ADC 全超时、宿主 30.0→31.0/35.0 向量 | 分类与上报一致；不得把历史 `V=0 D=0` 当通过 |
| 真实升温/超温 | 待用户具备安全调温条件（当前不安排） | 433 实发与 App 展示一致性 |
| 后续联调/平台/功耗 | 按联调阶段、平台授权与仪器条件执行 | 各自原验收项 |

**若上游明确修订本轮范围**（例如书面接受“本次交付不含无光分支”），则按修订后的范围重新判定，不得由本角色自行降级需求。

---

## 9. 未执行项与证据局限

1. 未复位、未烧录、未重采上电五行；上电五行沿用人工 REASSESS 已核实的 EV-038 同镜像证据。
2. 未调温、未调光、未遮光、未测功耗；用户明确限制，且 EN-02 要求被动观测。
3. 未取得无光/受光双态样本，因此对 `T` 的分离能力、候选值 3590 是否合适**未作判断**。
4. 板端镜像哈希不可回读；身份为“包/成员哈希 + 源码零差异 + `k` 连续”的闭合推断。
5. 未连接平台、未写入接口、未连数据库；未使用逻辑分析仪（本区分点不在总线/电气波形）。
6. `k` 与名义时间的偏差属 LSI 量级，不用作精确时钟证明。
7. 本 triage 不重跑整产品验收；`TEST_FAIL` 仍对应当前候选，未关闭。

---

## 10. 本轮改动

- 新增：`evidence/product_acceptance_20261112/FIRMWARE-TRIAGE-002.md`（本文件）。
- 新增：`evidence/product_acceptance_20261112/serial_capture_com43_20261112_triage.bin`（COM43 原始 160 B；本机留存，`.gitignore` 忽略，不随仓库分发）。
- 新增：`evidence/product_acceptance_20261112/serial_capture_hex_triage.txt`（十六进制转储，随仓库复核）。
- 新增：`evidence/product_acceptance_20261112/serial_capture_meta_triage.txt`（窗口/端口/字节数/哈希/两行原文）。
- 更新：`file_manifest.txt`（登记上述文件）。
- **未修改**：传感器固件 `USER/**`、`MDK/**`、`EWARM/**`、`UM2005C/**`、`COMMON/**`、发布包、网关、Android、`docs/**`、`runtime/permissions.yaml`、`runtime/environment.yaml`；未烧录/复位、未改设备或平台数据。

---

**最终判定：`RESPONSIBILITY_UNRESOLVED`。** 固件实现在已执行范围内未发现可证实缺陷；R4 无光分支的 `T=0` 是明示交付默认，启用需上游判据接受记录与同板同构建被动双态证据，两者当前均不具备，且同一测项已人工 `SKIP_CONFIRMED` 不得重复排队。原始 `TEST_FAIL` 保持有效，需重新规划判据接受与证据责任后再决定是否启用、重建与复验。
