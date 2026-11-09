# 失败责任与人工交接报告（FR-002：既有服务运行期核验）

负责角色：项目协调
日期：2026-11-09 13:24 +08:00
本次结果类型：**`REPLAN_REQUIRED`**（需补充/调整已审批的服务访问连接、执行工具授权与验收分界；同时需要用户提供隔离测试身份与写入/回读授权）
原失败目标：**保留**（`CALL-948dc3ca-7553-43ba-b389-23b3bf028a48` / `g002` `backend_engineer.backend_feasibility_exploration` = `WORK_FAILED`）
工作方式：只读既有成果与配置。未联网、未写库、未部署、未改产品源码、未改 `runtime/environment.yaml` / `runtime/permissions.yaml`、未重跑任何验证、未执行 D1–D5 中任何一条、未代替任何角色写实现或验收结论。

---

## 0. 结论摘要

| 问题 | 结论 |
|---|---|
| 原失败是否已在原目标上解决 | **否**。既有服务的运行期行为（在线性、`/upload_data` 存在性、真实幂等语义、持久化/回读、鉴权与归属）仍未取得任何真实证据 |
| 是否需要后端代码/表结构/部署改动 | 按 `readme.txt` 八点仍是**契约层“不要求”**；该结论未被推翻，但也**不等于服务可用** |
| 剩余工作是否属于后端实现缺陷 | **否**。无服务端源码（`abr.py` 从未进入 Git 历史），也没有可改的实现对象；继续派回后端实现会重复空转 |
| 原失败重派同一能力是否有效 | **无效**。当前配置下 `backend_feasibility_exploration` 的 `allowed_tools=[]`、`allowed_external_actions=[]`，环境未登记任何 `existing_service` 访问入口；无新增入口与授权时重派只能得到同一 `WORK_FAILED` |
| 缺口的性质 | ①**缺少已审批的合法服务连接**（`existing_service` 只有 target 声明，没有 `access` 入口；`network.links=[]`）；②**缺少授权**（合成数据写入、行级回读、清理；数据库直连）；③**缺少资源/现场数据**（测试手机号、测试网关 MAC、测试 dev_id、本机到服务网络可达性）；④**职责与工具未绑定**（图中存在验证能力 `software_tester.product_functional_verification`，但其授权工具只有 `android_test/usb_list_devices/serial_*`，无网络探测动作）；⑤**验收分界冲突**（`g002` 被要求给出运行期结论，而项目自身 E2E 计划已把服务链路列为执行期旅程并允许记 `NOT EXECUTED`） |
| 需要用户实际动手的事项 | 提供**与生产隔离**的测试身份（测试手机号/网关平台 MAC/dev_id）与明确授权（是否允许最小合成写入＋回读＋清理）；如无法提供数据库回读，则明确接受“持久化未验证”作为后续 `NOT EXECUTED` |
| 是否有物理维修/接线需求 | **无**。本失败不涉及焊接、接线、器件更换或供电操作 |

---

## 1. 原失败与当前版本引用

| 项 | 引用/值 |
|---|---|
| 原始失败 | `CALL-948dc3ca-7553-43ba-b389-23b3bf028a48`，节点 `g002`，能力 `backend_engineer.backend_feasibility_exploration`，结果 `WORK_FAILED`，摘要“returned WORK_FAILED”（Runtime `failure_evidence`） |
| 尝试历史 | 仅 1 次同能力调用（同上 CALL）；`referral_history=[]`，即**尚不存在他方拒接记录** |
| 失败报告（本轮受审对象） | `artifacts/backend_feasibility.md`（BE-EXP-001，26,375 B，sha256 `a1ccb119544102ed428035f3d633515d1d8c63e1f9732f21c132c3f0be51e203`） |
| Runtime 资源引用 | `runtime/resources/WF-CODEX-fe400cc9-…/CALL-948dc3ca-…/artifact_exploration.yaml` → 指向同一 `artifacts/backend_feasibility.md`；`content_digest` 与上表一致 |
| 失败报告的原始实验证据 | `evidence/backend_exploration/upload_contract_observations.txt`（5,660 B，sha256 `8f200994…`，13 个用例、11×2xx/2×400、538 行、仅监听 `127.0.0.1`）；复跑脚本 `evidence/backend_exploration/upload_contract_stub.py`（13,982 B，sha256 `5134c620…`） |
| 工作流 | `WF-CODEX-fe400cc9-dc65-42d0-96c4-dd8b411082c1` revision 1（1.yaml，3,111,292 B，sha256 `62ea6399f04b452ed0deac06928b20d00dce87675724072e08087e2657a8d47a`）；`g002` `contract_digest=4c5bc788…`，结果分支仅 `EXPLORATION_COMPLETE`/`WORK_FAILED`，两者都要求交付 `exploration` |
| `g002` 绑定 | `existing_system=g001.analysis`、`task`、`requirements=user_input`、`code_index=g000.codegraph_index`；发布 `delivered.g002.exploration` |
| 下游消费 | `delivered.g002.exploration` 作为 `service_context` 绑定到 Android 设计/实现/发布与 `g010`（`software_tester.product_functional_verification`）等节点；`g010` 另绑定 `subject=product_candidate`。即 `g002` 未完成会阻断其后整条交付链 |
| 环境/授权来源（本轮判定依据） | `runtime/environment.yaml`（8,529 B，sha256 `6050d562…`）；`runtime/permissions.yaml`（27,200 B，sha256 `0264162c…`） |
| 产品仓库状态 | HEAD `06f9c59`（`[role:backend_engineer]`，即 BE-EXP-001 交付提交）；接手时 `git status --porcelain` 为空 |
| 需求基线 | `readme.txt`（2,427 B，sha256 `c6f19954…`），其中第 5 点“复用平台服务器”、补充条件 4“跨链路验证安排在后续联调阶段” |
| 接口契约 | `artifacts/interface_contract.md`（IC-002 v3.0，sha256 `6af5f0a6…`）§6：端点取迁移记录与 `CloudConfig` 当前配置，**不代表外部在线验证** |
| 项目自有 E2E 计划 | `artifacts/software_e2e_plan.md`（E2E-SW-003 rev 2.0，sha256 `52922cef…`）：测试账号/绑定“**未准备**”；broker/HTTP“**未探测（无网络/无授权）**”；MySQL 行级“**无授权入口**”；E2E-07 阻塞条件为“网络/服务不可达 → `NOT EXECUTED`；无 DB 回读授权 → 持久化子项 `NOT EXECUTED`，不得以 HTTP 2xx 替代” |

检查时点：2026-11-09 13:20–13:25 +08:00。摘要只标识本次读取的对象内容，不证明远端服务或部署状态。

---

## 2. 失败目标与需求的关系（避免把非需求项当成缺陷）

`readme.txt` 第 5 点要求“平台 broker 相关的服务器信息已经放到项目文件夹内”，并未要求改造或重新部署服务端；运行时环境也记录“用户确认服务器已经部署，本轮优先确认并复用”。因此：

1. **需求侧可成立的部分**：本轮八点不引入新的持久化字段、端点或表结构；`/upload_data` 请求体（`phone/mac/readings[devId,time,temperature,humidity]`）与单位不变；条件上报（升温 >0.9℃ 且无光 / >35.0℃）是传感器侧筛选，服务端不重算。
2. **不能替代的部分**：上述是**契约层**结论。“复用”的可行性还依赖真实服务行为（在线、端点、幂等、持久化、鉴权）。`backend_feasibility_exploration` 的提示词明确要求：必要服务行为未成立、关键数据约束无法证实或关键原因不清楚时返回 `WORK_FAILED`，且**不得**把真实服务失败归成“部署/外部依赖”后移出目标。故该 `WORK_FAILED` 在专业上是合规的，不是误报，也不应被改写成通过。
3. **同时必须指出**：本项目已批准的执行计划自己把服务链路核验放在**执行期旅程**（E2E-07）并允许在无入口/无授权时记 `NOT EXECUTED`。因此 `g002` 与执行期验证之间的**验收分界**需要由重新规划明确，而不是由本能力改文档或改验收目标。

---

## 3. 已尝试的责任路径与依据

| 轮次 | 尝试的责任 | 结果 | 依据 |
|---|---|---|---|
| `CALL-948dc3ca`（唯一一次） | `backend_engineer.backend_feasibility_exploration`：查现有服务/接口/数据模型/部署条件，比较复用与最小改造 | `WORK_FAILED`（交付了完整 `exploration`） | 报告 §1.3/§8/§11：未访问 `8.140.23.253`；无服务端源码；无联网入口与外部动作授权；无测试账号/回读/清理授权。报告同时保留“本轮无需服务端改动”的契约层结论与 D1–D5 最小区分实验 |

**未发生的路径（不得假装已尝试）**：

- 没有他方角色对 `g002` 出过拒接报告（`referral_history=[]`），因此本报告不虚构“某专业拒接”。
- 没有执行过 `mdk_*`、`serial_*`、`sigrok_*`、`android_test`、任何 HTTP/MQTT/DB 请求；没有重跑桩实验；没有重测原失败。
- 没有以“服务不可达”“外部依赖”作为结论，也没有宣告服务器损坏或无法复用。

**当前责任候选为何不能再接**：同一能力在**相同入口、相同工具、相同授权**下重新调用，只会再次得到“无真实服务证据 → `WORK_FAILED`”。这属于结构缺口，不是能力不够努力；按公司规则也不得以原样重测消耗轮次。

---

## 4. 仍未知的事实（不得用推断代替）

1. 服务当前是否在线；运行版本与服务进程状态（EMQX 5.8.0 / MySQL 9.0.1 / `abr.py` 仅为迁移记录声明）。
2. `POST /upload_data` 是否存在、真实状态码与响应体（是否含 `inserted`）、批次是否单事务、部分失败如何返回。
3. 真实 MySQL 上 `UNIQUE(dev_id,time)` + `INSERT IGNORE` 的全语义（含非约束错误降级为 warning 的行为），以及同键不同值的取舍。
4. 写接口是否存在鉴权与 `phone/mac/dev_id` 归属校验；是否存在任意写面。
5. 持久化与回读是否一致；行级数据是否可核验；磁盘余量与备份/恢复演练。
6. `mac` 是否真不落库（文档声明，静态成立；运行期未证）。
7. 真实部署下“每 180 s 缓存复读 → 480 行/设备/日”的实际上界与并发承受度（当前只有静态代码推导与算术区间）。
8. 服务中断 > ~8 h 后客户端停止自动重试造成的缺行，是否有服务端补偿/重放入口。
9. 本机（当前运行主机）到该服务的网络可达性；是否仅手机侧可达。
10. 生产数据现状：不能假设测试身份可用、也不能假设可用真实用户数据做实验。

---

## 5. 具体缺项（逐项定性）

| # | 缺项 | 现状事实（本轮核对） | 分类 | 归属 |
|---|---|---|---|---|
| S1 | `existing_service` 无任何已登记访问入口 | `targets.existing_service` 仅有 `enabled/deployment_status/verification_status/configuration_source/management_access` 字段；`access` 下只有 `sensor_serial_observation`、`sensor_programming`、`android_app_testing`、`android_readonly_inspection`、`i2c_passive_capture` 五项；`network.switches={}`、`network.links=[]` | **缺少已选专业的合法连接**（需修改/新增已审批连接登记） | 环境/配置所有者 + 重新规划审批；**本能力不得改配置文件** |
| S2 | 无网络服务探测/写入/回读的授权动作 | `backend_engineer.backend_feasibility_exploration`：`allowed_tools=[]`、`allowed_external_actions=[]`；`software_tester.product_functional_verification`：工具仅 `android_test/usb_list_devices/serial_list_ports/serial_capture`，外部动作仅 `operate_test_equipment`；全仓权限文件无 HTTP/MQTT/DB 服务动作 | **缺少授权**（能力/工具与外部动作未绑定到该入口） | 重新规划 + 权限所有者 |
| S3 | 隔离测试身份未准备 | `software_e2e_plan.md` §1：“测试手机号 / 测试网关平台 MAC / ≥1 个绑定设备 ID：**未准备**” | **缺少资源/入口数据**（用户提供） | 用户 |
| S4 | 写入/回读/清理授权未给 | 报告 §9 D3/D4/D5 需要“合成数据写入 + 只读回读 + `DELETE` 清理”；E2E 计划：“DB 行级需另行授权”“无 DB 回读授权 → 持久化子项 `NOT EXECUTED`” | **缺少授权**（生产副作用需明确边界） | 用户 + 重新规划（写入类动作不得默认开启） |
| S5 | 运行期服务核验在审批图上没有明确归属 | `g002` 被当作运行期结论的来源；但服务链路执行属于 `g010`（产品功能验证）与 E2E-07；`g010` 目前无网络工具且其阻塞条件允许 `NOT EXECUTED` | **验收分界/职责绑定不清**（需改已审批的验收与执行归属） | 重新规划/验收范围所有者 |
| S6 | `g002` 结果分支无“契约可行、运行期待验”的出口 | `g002` 合同只有 `EXPLORATION_COMPLETE`（成功）与 `WORK_FAILED`（失败）；报告的实际语义是“契约层可行 + 运行期未证” | **契约/验收语义缺口**（需由重新规划决定出口与下游门禁） | 重新规划 |
| S7 | 需人完成的操作 | 见 §7（提供测试身份与授权、确认网络可达性/可选现场执行） | **必须由人完成的操作** | 用户 |

> 说明：本表没有任何“硬件损坏”“某专业修不了”“必须重装/重部署”的判断。S1–S2、S5–S6 是**结构与授权**缺口，S3–S4、S7 是**用户资源与授权**缺口；二者不能互相替代。

---

## 6. 最小下一步、责任、完成条件与恢复后的独立验证

### 6.1 先由重新规划做两项决定（二者必选其一，不得两头空转）

**方案 A（推荐：把运行期证据交给执行期验证）**

1. 认可 `g002` 的结论为“契约层复用可行、运行期未证”，并把 `delivered.g002.exploration` 的 `service_context.mode=reuse_existing_deployment` 连同 **`runtime_proof_required: D1–D5`** 一起下传；
2. 明确 D1–D5 由 `g010`（`software_tester.product_functional_verification`）的 E2E-07 服务链路旅程承接；无入口/无授权时按 E2E-SW-003 记 `NOT EXECUTED`，且**不得**在总体验收中把未执行当通过；
3. 为 `g010` 绑定 S1/S2 的入口与动作授权（只读探测为默认；写入类动作须由用户在 §7 明确开启）。

**方案 B（保留 `g002` 为运行期证据责任人）**

1. 为 `backend_engineer` 增加一个受控的服务核验入口与动作授权（仅 D1/D2 只读探测，或 D3–D5 的受限写入）；
2. 由用户提供 §7 的测试身份与授权；
3. `g002` 在新入口下重跑 D1–D5 后再判 `EXPLORATION_COMPLETE`。

> 两种方案都必须**保留** `artifacts/backend_feasibility.md` 原文与 `WORK_FAILED` 历史；不得通过改写该报告宣称服务已可用。

### 6.2 动作清单

| # | 最小下一步 | 应由谁处理 | 完成条件 | 恢复后必须重做的独立验证 |
|---|---|---|---|---|
| N1 | 选定方案 A 或 B，并把服务核验归属、验收分界写进新的已审批计划 | 重新规划/审批 | 计划中明确“谁执行 D1–D5”“无入口时记 `NOT EXECUTED` 还是阻断”“`g002` 是否解除” | 复验时按新范围逐项判定；旧 `WORK_FAILED` 保留为历史 |
| N2 | 登记一个有界的 `existing_service` 访问入口（`environment.yaml`），并与权限文件中的能力动作对应 | 环境/配置所有者 + 权限所有者 | 入口有明确 host/port、超时（建议单次 ≤10–15 s）、允许动作清单；`enabled=true` 且对目标能力 `effective_status` 为可用 | 入口可用性以一次有界只读探测的原始响应为准（不是配置存在即通过） |
| N3 | 提供隔离测试身份：测试手机号、测试网关平台 MAC、测试 dev_id（金额/时间用合成值） | 用户 | 四项齐备，且**不含**真实用户数据；不使用迁移记录中的真实设备/root 凭据 | 用测试键做 D3 后才可声明幂等成立 |
| N4 | 明确写入/回读/清理授权边界 | 用户（授权）+ 重新规划（动作登记） | 明确：是否允许两次合成 `POST /upload_data`；是否允许行级只读回读；是否允许 `DELETE` 清理测试行；否则只做 D1/D2 | 有回读才可声明持久化；无回读则持久化子项记 `NOT EXECUTED` |
| N5 | 确认网络可达性（本机或指定的受控探测主机） | 用户 | 有界探测能得到确定响应（含明确的拒绝/超时也记录） | 同一窗口内 D1/D2 原始响应 + 时间戳 |
| N6 | 执行 D1–D5（按报告 §9，仅合成数据） | 方案 A：`g010`；方案 B：`backend_engineer` | D1 证书链/`CONNACK` 可解释；D2 端点存在；D3 首次 `inserted=1`、重试 `inserted=0` 且行数不变、回读逐字段一致；D4 冲突/部分失败/4xx 边界有可复现结论；D5 鉴权/归属结论明确 | 上述每条都必须附原始请求/响应与回读结果；仅“命令成功/2xx”不作为业务结论 |
| N7 | 服务缺口若被 D3/D4 否定（如无 `UNIQUE`、非 2xx 重试、无事务） | 重新规划（决定是否新增后端改造职责） | 先在计划中明确“是否需要服务端改造”，并落实源码与部署/DB 授权；不得在无源码时声称已修 | 改造后由独立验证重跑 D3/D4 |
| N8 | 服务链路结论进入总体验收 | `g010` + 交付/验收责任人 | 每条旅程有证据索引；`NOT EXECUTED` 不得计为通过；真实服务证据不得与桩/主机单测混写 | 以当前候选版本（提交号 + APK/固件摘要 + 服务版本）重新取证 |

### 6.3 完成判据（本次失败可被认为已解除的最小条件）

1. 审批计划中明确 D1–D5 的**执行归属**与**验收分界**（N1）；
2. 若计划要求执行期取证：入口与授权落实（N2），测试身份与授权边界落实（N3/N4）；
3. 至少完成 **D1 + D2**（只读）并保存原始证据；若计划要求“复用成立”，还须完成 **D3（含回读）**；
4. 未执行项在后续报告中逐项写明原因与缺失条件，且不进入通过统计。

---

## 7. 需要用户提供/实际操作的事项（本能力未代做）

1. **测试身份（必须）**：与生产隔离的测试手机号、测试网关平台 MAC、至少一个测试 dev_id（8 位 HEX）。若不便提供 MAC，则明确“仅用合成 MAC 做接口级探测”，并由 D5 结论说明归属校验是否存在。
2. **授权边界（必须选择）**：
   - 只读：仅做 TLS/端点存活探测（D1/D2），持久化与幂等记 `NOT EXECUTED`；
   - 受限写入：允许用测试键做两次合成 `POST /upload_data`（D3），并允许只读回读；
   - 完整：在受限写入基础上允许删除本次测试行（清理），并说明由谁执行、何时执行。
3. **数据库回读途径（可选但影响结论）**：若无法提供行级只读入口，请明确接受“持久化/幂等仅能由 HTTP 响应与 App 侧待发箱间接观察，不能声明已持久化”。
4. **网络可达性**：确认当前主机或指定受控主机可访问服务端口；若只有手机侧可达，请说明，后续由手机侧旅程取证。
5. **不要在消息中发送真实用户数据或生产凭据**；迁移记录中的真实账号/口令/密钥不得用于本轮探测。

以上均为“提供数据/授权”或“在具备职责的节点执行”，本能力**没有**连接服务、没有写库、没有调用任何设备工具、没有代替测试或实现角色执行。

---

## 8. 本次能力的边界与未做事项（可核对）

- 未修改 `artifacts/backend_feasibility.md`、`artifacts/software_e2e_plan.md`、`artifacts/interface_contract.md`、`readme.txt` 或任何验收文件；未改产品源码、固件/Android 工程、网关。
- 未修改 `runtime/environment.yaml`、`runtime/permissions.yaml`、工作流 revision；未新增节点或绑定。
- 未执行 `curl`/HTTP、MQTT、MySQL、`mdk_*`、`serial_*`、`sigrok_*`、`android_test`；未访问 `8.140.23.253`；未重跑 `upload_contract_stub.py`。
- 未把 D1–D5 中任何一条写成已执行；未宣告服务在线/离线、未宣告复用通过或失败、未宣告器件或部署损坏。
- 接手时 `git status --porcelain` 为空；本次仅新增本报告并更新 `file_manifest.txt`。

---

## 9. 交回重新规划的具体请求

1. **确定验收分界（N1，最高优先）**：`g002` 的 `EXPLORATION_COMPLETE` 是否接受“契约层可行 + 运行期核验门禁”，还是必须由 `g002` 亲自完成 D1–D5。这决定后续是否需要重复调用同一能力。
2. **登记服务访问连接（N2）**：在 `runtime/environment.yaml` 增加一个受控、有界的 `existing_service` 访问入口（默认只读探测，写入类动作显式单列），并在权限文件中把它绑定到实际执行的能力。
3. **绑定执行职责与工具（N2/S2）**：若由 `g010` 执行，请为其增加该入口的探测/回读动作；若由后端能力执行，请给 `backend_feasibility_exploration` 增加同一入口与动作，避免“有连接但无授权工具”。
4. **落实用户资源与授权（N3/N4/N5）**：把 §7 的测试身份、授权边界、可达性与回读方式作为执行前置写入计划；缺项时对应子项记 `NOT EXECUTED`，不得计入通过。
5. **保留原失败证据**：`CALL-948dc3ca`、`artifacts/backend_feasibility.md`、`evidence/backend_exploration/*` 与本次 `REPLAN_REQUIRED` 一并保留；后续报告不得删除“运行期未证”的表述。
6. **后端实现保持待命、不重派**：在 N1 未定、N2/N3/N4 未落实前，不要再次把同一 `WORK_FAILED` 原样派回 `backend_feasibility_exploration`；也不要以“服务端改造”名义在缺少 `abr.py` 源码与部署/DB 授权的情况下开工。

以上请求不改变 `readme.txt` 的验收要求，也不表示任何缺口已修复或任何服务行为已验证通过。

---

## 附录 A：本次确认的对象身份（SHA-256，检查时点 2026-11-09 13:20–13:25 +08:00）

| 对象 | 大小 | SHA-256 |
|---|---|---|
| `artifacts/backend_feasibility.md` | 26,375 B | `a1ccb119544102ed428035f3d633515d1d8c63e1f9732f21c132c3f0be51e203` |
| `evidence/backend_exploration/upload_contract_observations.txt` | 5,660 B | `8f200994dcc3b338aff1f74cf4ed5b81faa8a046aa5a62c45cd50027a3e153e1` |
| `evidence/backend_exploration/upload_contract_stub.py` | 13,982 B | `5134c6200ba42fed4ee43adcac2e28c1c7454db0042d3c5ff97bdacef4629a8a` |
| `artifacts/software_e2e_plan.md` | 13,828 B | `52922cefe5291c136a88eb8ea325161638d08d4cf0e538fb9c1522d226193abe` |
| `artifacts/interface_contract.md` | 13,692 B | `6af5f0a65c819033390e94099c77ede83af53b6a2750edf55debd9fe42d2ff22` |
| `readme.txt` | 2,427 B | `c6f19954d54c29136db95d75dcc28816d84d5ce57ea449714a0c58ba80a1e6dd` |
| `runtime/environment.yaml`（Runtime 配置，非产品成果） | 8,529 B | `6050d56222d2d9d84ae4bd2a4e41c1420938c4d7eab220eb5c0e289995865e77` |
| `runtime/permissions.yaml`（Runtime 配置，非产品成果） | 27,200 B | `0264162ca0ea7f14782c77b72491b41ce5058eddf4f96f61ef6daa72e2550688` |
| 工作流 revision `1.yaml`（Runtime 配置，非产品成果） | 3,111,292 B | `62ea6399f04b452ed0deac06928b20d00dce87675724072e08087e2657a8d47a` |
| Runtime 资源引用 `artifact_exploration.yaml` | 138 B | `9d8838d7cb12ce01eeac6b4529a2624550c335302628ad935cbfb3283ed6fd95` |

## 附录 B：本次核对使用的只读事实（可复现）

1. `existing_service` 在 `runtime/environment.yaml` 中没有 `access` 条目；`network.switches`/`network.links` 为空。
2. `runtime/permissions.yaml` 中 `backend_engineer.backend_feasibility_exploration.allowed_tools=[]`、角色 `allowed_external_actions=[]`；`software_tester.product_functional_verification` 工具与服务无关。
3. `artifacts/software_e2e_plan.md` §1 明确“测试账号/绑定 未准备”“broker 未探测（无网络/无授权）”“MySQL 行级直查（无授权入口）”，§2 E2E-07 明确无入口时记 `NOT EXECUTED`。
4. `artifacts/backend_feasibility.md` §1.3/§9/§11 明确 D1–D5 未执行及所需授权，且 §7 `service_context` 标注 `mode: reuse_existing_deployment` 与 `source_of_truth: 服务器迁移记录.md`（仅资料声明）。
5. 仓库内不存在服务端源码：`abr.py` 在任何 Git 历史中均未出现（BE-EXP-001 §2 已记录，本轮未再执行全历史检索）。
