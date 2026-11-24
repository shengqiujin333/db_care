# 失败责任与人工交接报告（FR-008：CodeGraph 索引准备 WORK_FAILED／证书脚本根不可索引）

负责角色：项目协调
日期：2026-11-24（Asia/Shanghai）
状态：**未解决**（原失败目标保留；`index_prepare` 的 WORK_FAILED 尚无任何已生效修复）
本次结果类型：`REPLAN_REQUIRED`（必须补充能力或修改已审批的索引覆盖要求；不是成功，也不是功能通过）
工作方式：只读核对失败当次调用记录、产物与证据、当前 Git/工作区、审批后的流程定义、本机已安装工具能力；未修改产品源码、脚本、证书、测试断言或验收文档；未安装/升级/重装任何工具；未登录服务器、未操作手机或设备；未重跑原失败能力。

---

## 0. 结论摘要

1. **原失败未变化**：`code_intelligence_engineer.codegraph_indexing` 在 `index_prepare` 节点返回 `WORK_FAILED`，因此审批图里唯一的成功出口 `CODEGRAPH_READY → test_design` 从未触发；后续平台证书修复、复测全部未开始。
2. **已交付部分可核对且仍有效**：传感器、网关、Android 三个既有索引 `sync` 报 already up to date，源码查询能定位当前源码（`sensor_decide_report`、`decode_frame10`、`CloudConfig.kt`/`setupSSL`/`CloudUploadRepository`）。这三根本身不是本轮失败点。
3. **失败点很具体**：新增的证书脚本根 `scripts\`（仅 `emqx_8883_cert_replace.sh`、`emqx_8883_cert_verify.sh`）初始化/Sync 后 `fileCount=0、nodeCount=0、languages=[]、lastIndexed=null`，两次 `explore` 查不到源码。本轮复核该状态未变。
4. **本轮新增的决定性事实**：本机工具 `@colbymchenry/codegraph` 1.6.0 的语言注册表里**没有 shell/bash**（`dist/types.d.ts:32` 的 `LANGUAGES` 数组，sha256 `4345a74e…52bd1ba`），而 `codegraph.json` 的 `extensions` 只能把扩展名映射到**已支持**的语言 id。因此"缺一个配置文件"不成立：**现有工具链内不存在任何配置手段让 `.sh` 被索引**。这不是忽略规则、gitignore 或权限问题（`scripts\.codegraph\` 已被 `.gitignore` 忽略，索引库确实建成但为空）。
5. **缺项属两类**：①工具能力缺 shell 解析（需新增/更换索引工具能力）；②已审批的索引覆盖要求未定界（`scripts\` 是否必须进入 `code_index`）。两者**都不是**仅凭用户提供文件或一次授权即可消除的问题，故不是 `INPUT_INVALID`。
6. **不是重测问题**：在同一边界下再次调用 `codegraph_indexing` 只会重复"init/sync/explore 得到 0 文件"，不产生新证据；本报告不提交重测建议作为修复。
7. **待用户/规划决断**（不代替决定）：要么批准"三根覆盖 + 脚本以原生只读方式使用并在产物中显式声明限制"，要么批准"新增 shell 索引工具能力"（涉及全局安装/升级授权，且是否有支持 shell 的版本尚未核实）。任一路径都要改已审批的覆盖要求或职责，故本次返回 `REPLAN_REQUIRED`。

---

## 1. 原失败与当前版本引用

| 项 | 引用/值 |
|---|---|
| 原失败调用 | `CALL-f9157713-34b7-408b-9b02-58604e02eb2a`，节点 `index_prepare`，`status: completed` + `result_code: WORK_FAILED` |
| 失败摘要 | `code_intelligence_engineer.codegraph_indexing returned WORK_FAILED` |
| 失败当次产物 | `artifacts/codegraph_tls_index.md`（内容摘要 `a979ea45…`，见运行时资源记录） |
| 失败当次证据 | `evidence/codegraph_tls_20261124.json`（摘要 `b8e44d37…`）+ 采集脚本 `evidence/codegraph_tls_20261124.py` |
| 失败当次源码版本 | 该次报告声明源码 HEAD `5237cdb`；`scripts` 子树 `4cabeaf4760bb35ff49523cca9b7c05d7ca6537f` |
| 本轮接手版本 | HEAD `bf608ab`（`[role:code_intelligence_engineer] Update project work`），工作区干净；`git rev-parse HEAD:scripts` 仍为 `4cabeaf…`（脚本未变） |
| 本轮 HEAD 新增内容 | 仅 `artifacts/codegraph_tls_index.md`、`evidence/codegraph_tls_20261124.json/.py`、`file_manifest.txt`（`git diff --name-status 5237cdb..HEAD`） |
| 审批与路由 | 流程 `WF-CODEX-bfbb63fa…` revision 1，用户 APPROVE（`approvals.jsonl`，2026-11-24T14:48:51Z）；路由 `index_prepare/WORK_FAILED → handoff`（本能力） |
| 被阻塞的下游 | `test_design`（需 `code_index = [index_prepare, CODEGRAPH_READY, codegraph_index]`）→ `platform_restore` 环（CA 核查/最小部署、平台复测、平台归因、平台证书修复）→ `cases` → `complete_validation` |
| 原项目目标（未变） | 核实原 CA 能否签发、修复 8883 服务器证书与客户端地址不匹配，复测安卓到平台上传及受影响端到端用例；是否更换 App 信任锚由用户决定 |
| 拒接记录 | `referral_history: []`（本次失败**没有任何专业能力拒接记录**，也没有更早的责任候选处置可引用） |

**说明**：本报告只处理 `index_prepare` 的 WORK_FAILED；平台 8883 证书缺陷本身（`evidence/e2e_p0/` 记录的 SAN 仅 `117.72.84.210`、`-verify_hostname 8.140.23.253` 返回码 62）**仍未修复**，本轮未做任何服务器侧动作。

---

## 2. 本轮实际核对到的事实

### 2.1 失败当次证据（原文摘要，未重算）

- `roots[0..2]`（sensor / gateway / android）：`sync` 均为 `Already up to date`，`status` 文件数 99 / 34 / 113，查询分别命中 `USER/src/fw_core.c:112`、`APP/feistel_al.c:119` 调用上下文、`CloudConfig.kt`/`MqtttService.setupSSL`/`CloudUploadRepository`；源码节选与工作区一致。
- `roots[3]`（certificate_scripts，`D:\mypro\beiwo2\AIP\scripts`）：`codegraph init --yes` 返回 “No files found to index”；`sync` 返回 `Already up to date`；`status --json` 为 `fileCount 0 / nodeCount 0 / edgeCount 0 / languages [] / lastIndexed null`，同时 `index.state = complete`；`files` 返回 “No files indexed”；两次 `explore`（两个脚本文件名）`current_source_located: false`。
- 该根**没有** `codegraph.json`（`configuration: {}`），`scripts\.codegraph\` 已被 `.gitignore` 忽略、未被 Git 跟踪。

### 2.2 本轮只读复核（2026-11-24T15:17Z）

| 核对项 | 实际结果 |
|---|---|
| `codegraph status --json D:\mypro\beiwo2\AIP\scripts` | exit 0；`version 1.6.0`；`fileCount 0`、`nodeCount 0`、`languages []`、`lastIndexed null`、`pendingChanges {0,0,0}`。与失败当次一致，**索引仍为空** |
| `scripts\` 目录实际内容 | 仅 `emqx_8883_cert_replace.sh`（337 行）、`emqx_8883_cert_verify.sh`（123 行）与空索引目录 `.codegraph`；无 `codegraph.json` |
| 工具语言注册表 | `C:\Users\kason\AppData\Roaming\npm\node_modules\@colbymchenry\codegraph\dist\types.d.ts:32` 的 `LANGUAGES` 为 42 项，**不含 `shell`/`bash`/`sh`**（文件 sha256 `4345a74ee8214581af03b633abf7fed294752d7fd574842943928c94552bd1ba`） |
| 工具文档 | 同包 `README.md` “Supported Languages” 表同样无 Shell 行；`codegraph.json` 的 `extensions` 文档明确“Each value is a supported language id” |
| 历史同类交付 | `CALL-16dd3717-9c85-4371-80dd-e8f976c9c444` 以 **三个根**（sensor/gateway/android）交接并返回 `CODEGRAPH_READY`（`artifacts/codegraph_e2e_index.md` + `evidence/codegraph_refresh_20261123.json`），当时未把 `scripts\` 计入索引根 |

**推论（仅限已核事实）**：`.sh` 为空索引不是配置漏项、不是路径错误、不是权限或 gitignore 阻塞；是安装版本**没有该语言的解析能力**。`codegraph.json` 只能做"扩展名 → 已支持语言 id"的映射，无法凭空增加语言。

---

## 3. 已尝试路径与拒接依据

| 路径 | 谁做了/证据 | 实际结果 | 处置 |
|---|---|---|---|
| 对 `scripts\` 初始化并增量同步 | 失败当次调用，`evidence/codegraph_tls_20261124.json` roots[3] | 索引建成但 0 文件、0 节点；`init` 明示 “No files found to index” | 已实证不可用，**不得原样重复** |
| 对 `scripts\` 做定向查询（两个脚本名） | 同上，`queries[]` | 两次均无源码命中 | 同上 |
| 用根级配置改扩展名映射（`codegraph.json`） | 本轮只读核对工具文档与类型定义 | `extensions` 目标必须是受支持语言 id；shell 不在其中 | **不是可选修复**，文件不存在也不必补 |
| 把 `scripts\` 排除在索引外、以原生只读方式使用 | 本轮只读核对历史交接（`artifacts/codegraph_e2e_index.md`）与 `scripts` 实际体量（2 个文件 460 行） | 技术上可行，但会改变 `index_prepare` 已审批的覆盖要求/下游 `code_index` 语义 | **需重新规划批准**，本能力无权自行定界 |
| 安装/升级到支持 shell 的索引工具 | 未执行（`environment_repair.enabled=false`，能力提示禁止未经授权全局安装） | 是否存在支持 shell 的版本**未核实** | 需授权 + 新的工具能力责任，见 §4/§6 |
| 专业能力拒接 | **不存在**（`referral_history: []`） | 无拒接理由可整理 | 本轮是首次责任处置，不虚构拒接 |

---

## 4. 具体缺项（分类）

| 分类 | 是否缺 | 具体内容 |
|---|---|---|
| 缺少已选专业的合法连接 | **是（部分）** | 流程中 `codegraph_indexing` 只有"整包可查"一条成功出口；没有"部分根可查 + 明确不可解析类型以原生读取"的合法成功出口，也没有"工具能力不足"的转交节点 |
| 库存缺少职责/能力 | **是** | 现有工具链（codegraph 1.6.0）无 shell 解析；项目角色里也没有负责评估/引入 shell 索引工具的能力或已登记入口 |
| 缺少资源/访问入口 | 否（本失败范围内） | `scripts\` 文件本地可读，索引库可建；不需要服务器、手机或设备 |
| 缺少授权 | **是（条件性）** | 若走"引入/升级索引工具"路径：需要全局安装/升级授权（当前 `environment_repair` 关闭、能力提示禁止未授权全局安装）；当前无该授权 |
| 必须由人完成的操作 | 否（本失败范围内） | 不存在只有人能做的接线、焊接或现场操作；本轮也不需要用户手工提供文件 |
| **范围未定界（最便宜的缺口）** | **是** | `scripts\` 是否必须进入 `code_index` 没有审批结论：失败当次把两个 `.sh` 当作"本轮相关代码根"，而此前已批准的交接只要求三个应用根 |

---

## 5. 仍未知的事实（不得当成结论使用）

1. 本机之外是否存在**支持 shell** 的 codegraph 版本或替代工具；未联网核实、未安装、未试用。
2. 两名脚本是否**真的需要代码图分析**：它们调用 `openssl`/`emqx`，是执行器而非待分析的业务源码；这一点是判断"范围可缩"的依据，但**定界属于规划决定**，不是本报告结论。
3. 三个已索引根在**证书修复后会改动**的源文件（Android `CloudConfig.kt`、`MqtttService.kt`，服务器侧 `abr.py`/EMQX 配置）是否会引入需重新同步的变化；服务器侧 `abr.py`/EMQX 配置**没有本地独立源码根**，本轮未取得也未索引。
4. 平台 8883 证书能否用原 CA（`jd-ca.crt`，CN=`Tiktok Root CA`）签发、CA 私钥是否在服务器可用 —— **完全未核查**（下游 `ca_access_and_minimal_deployment` 未开始）。本报告不对其作任何判断。
5. 安卓侧是否需要更换信任锚（用户保留决定权）—— 未触及。

---

## 6. 最小下一步、责任人与完成条件

> 以下为待批准的最小动作，本能力不代为执行，也不启动其他业务工作流。

**A. 首选（最便宜、不动工具链）——修改已审批的索引覆盖要求**
- 动作：由流程设计/规划角色在下一版流程修订中明确 `index_prepare` 的成功出口判据：覆盖"实际参与本次修复的源码根"（sensor/gateway/android），对不可解析类型（`.sh`）以**原生只读**方式使用，并要求产物显式声明该限制；同时在 `codegraph_index` 产物里写明 `scripts\` 的 `fileCount=0` 事实与替代读取方式。
- 责任人：流程设计角色（修订）+ 规划/用户（批准）；执行仍由 `code_intelligence_engineer.codegraph_indexing`。
- 完成条件：新版修订获批，且产物同时满足"每个声称可用根有当前源码查询命中"与"不可解析根被显式排除并给出替代读取方式"。

**B. 备选（若必须让 `.sh` 进入图）——补充工具能力**
- 动作：先由有权限的角色**只读核实**是否存在支持 shell 的版本/工具；如存在，再按授权安装/升级，并在同一流程中新增"工具能力验证"的必要步骤。
- 责任人：需要新增的工具能力责任方（当前项目图内没有）+ 用户授权（安装/升级）。
- 完成条件：工具能对 `scripts\` 报出非空 `fileCount` 并对两个脚本名给出源码命中；在此之前**不得**把脚本根写成"可查"。

**C. 无论 A/B，恢复后必须先确认索引本身**
- 责任人：`code_intelligence_engineer.codegraph_indexing`（按新判据重跑一次并交付）。

**不允许的下一步**：在同一边界、同一工具、同一判据下再次调用本能力（必然重复 0 文件结果，属消耗轮次而非处理责任）。

---

## 7. 恢复后必须重做的独立验证

1. **索引侧（独立于产物自述）**：对每个声称可用的根依次执行 `codegraph status --json`、`codegraph sync`、`codegraph status --json`、一次 `codegraph explore -p <root> --max-files 1 <当前符号/文件>`；核对 `fileCount>0`、`languages` 非空、命中行与当前工作区内容一致；`scripts\` 若仍 `fileCount=0`，产物的表述必须与该状态一致，不得标为就绪。
2. **证书侧（本轮完全未做）**：按已批准流程由部署能力核实原 CA 能否签发、签发 SAN 含 `8.140.23.253` 的服务器证书并替换/重载；独立复测需给出 `-verify_hostname 8.140.23.253` 返回码 0、证书 subject/SAN/serial/指纹、`jd-ca.crt` 链校验结果，并保留原始输出。
3. **端到端**：安卓到平台 MQTT(8883) 连接与上传、`POST /upload_data` 与 `sensor_data` 回读、以及平台/整体受影响的既有用例；只认本轮时间窗内的原始证据，不沿用旧通过结论。
4. **回归范围**：三个已索引工程中，凡本轮被改动的源码需重新 `sync` 后再查询，避免用旧图结果解释新源码。

---

## 8. 影响范围与未做事项

- **被阻塞**：`test_design` 及其后的平台恢复环、证书修复、安卓上传复测、整体验收——即**项目目标本身尚未开始**，当前状态不是"证书已修"也不是"证书修不了"。
- **仍然可用**：传感器/网关/Android 三根索引及其定向查询（例如 Android `CloudConfig.kt`、`MqtttService.setupSSL`、`CloudUploadRepository` 的定位能力），对后续判断"改地址还是改证书"直接有用。
- **实际影响有限但门禁严格**：两个证书脚本共 460 行，原生只读即可审阅；真正的缺口是"整包成功"这一判据没有为不可解析类型留合法出口。
- **本轮未做**：未重跑失败能力；未安装/升级/卸载任何工具；未改 `.gitignore`、`codegraph.json`、脚本、证书、产品源码或验收文档；未登录 `8.140.23.253`；未操作手机（`12f046690506`）、网关、传感器或逻辑分析仪；未连接数据库。

---

## 9. 为什么是 `REPLAN_REQUIRED` 而不是 `INPUT_INVALID`

- `INPUT_INVALID` 的适用情形是"现有结构内缺资源、授权或人工作业"。本轮失败**不缺文件、不缺访问、不需要现场操作**：两个脚本本地可读，索引库已建成，唯一阻塞是"没有 shell 解析能力"与"覆盖要求未定界"。
- 消除该阻塞必须在两条路径中选一条：**补充工具/职责能力**（新增或更换索引工具、并为其授权与验证新增步骤），或**修改已审批的连接/判据**（明确 `scripts\` 不在 `code_index` 范围内、以原生读取替代）。两者都属于"补充职责、能力或修改已审批的连接"。
- 即使先取得用户"允许安装工具"的授权，也仍需有人承担工具选型/安装/验证并在流程中登记入口；仅凭授权不构成可执行结构。因此返回 `REPLAN_REQUIRED`，并交付本报告。

---

## 附：供规划参考的最小事实清单

| 项 | 值 |
|---|---|
| 空索引根 | `D:\mypro\beiwo2\AIP\scripts`（`fileCount 0`，`state complete`，`lastIndexed null`） |
| 脚本 | `scripts\emqx_8883_cert_replace.sh`（337 行）、`scripts\emqx_8883_cert_verify.sh`（123 行） |
| 工具 | `@colbymchenry/codegraph` 1.6.0（npm 全局），无 shell/bash 语言 id |
| 已可用根 | sensor 99 文件 / gateway 34 / android 113（均 already up to date） |
| 未开始 | CA 签发核查、8883 证书替换、安卓上传复测、端到端与整体验收 |
