# 后端可行性探索报告（BE-EXP-001）

责任角色：后端工程
日期：2026-11-09（Asia/Shanghai）
结论：**WORK_FAILED** —— 契约层结论为“本轮无需改服务端”，但既有已部署服务的运行行为与关键数据约束在本轮**未成立**（无服务端源码、无获准的联网/回读入口）；本地隔离桩只固定契约与风险模型，不能替代真实依据。

被测产品源码：`D:\mypro\beiwo2\AIP`，接手 HEAD `8b0b2d1`（首次 `git status --porcelain` 为空）
关联输入：`readme.txt`、`服务器迁移记录.md`、`artifacts/interface_contract.md`（IC-002 v3.0）、`artifacts/system_analysis_current.md`、Android `cloud/*` 与 `MqtttService.kt`、网关 `APP/app_um2006A.c`
实验原始输出：`evidence/backend_exploration/upload_contract_observations.txt`
可复跑脚本：`evidence/backend_exploration/upload_contract_stub.py`

---

## 0. 结论摘要

| 可行性问题 | 本轮结论 | 依据 / 边界 |
|---|---|---|
| 本轮需求是否要求改后端代码、表结构或重新部署 | **不要求**（契约层） | 上报字段、单位、端点、表结构均不变；新增的“升温 >0.9℃ 且无光 / >35.0℃”是传感器侧筛选，服务端不重算；`readme` 八点无新增持久化字段 |
| 既有 `POST /upload_data` 是否真实在线、可用、幂等 | **未核验 → 不成立** | 用户声明已部署；环境未登记联网入口、`allowed_external_actions` 为空；仓库无服务端源码（`abr.py` 从未提交）。本轮未访问 `8.140.23.253` |
| `UNIQUE(dev_id,time)` + `INSERT IGNORE` 是否满足客户端重试幂等 | **契约语义上可用，运行未证** | 本地桩：相同批次重试 `inserted=0`、行数不变；同键不同值静默丢弃（first-wins）；秒级键无法区分同秒两个真实样本 |
| 云端写入量是否被“条件上报”降低 | **不会**（按当前 as-built） | 网关 BLE 读取返回每设备最近一次缓存值，Android 每 180 s 读一次并按**接收时刻**入库/上传：上界 480 行/设备/日，与传感器是否新上报无关 |
| `mac` 能否用于来源网关审计 | **不能** | 文档 `sensor_data` 表无 `mac` 列；请求中的 `mac` 不落库 |
| 手机号/网关/设备绑定与写接口鉴权 | **未知** | 迁移记录只描述字段与 `INSERT IGNORE`，未声明鉴权/归属校验；本轮无法核验 |
| 服务中断后的数据恢复 | **客户端约 8 h 后停止自动重试** | `maxAttempts=20` + 60→1800 s 退避；达上限行保留但不再自动重试；后端无补偿/重放入口证据 |
| 容量（1000 设备） | **约 12–26 GB/年**（算术区间） | 480 行/设备/日上限；与迁移记录 1 分钟估算交叉核对；非实测磁盘 |
| 可否直接进入后端实现 | **不需要实现**；需要先补“服务核验入口” | 复用是唯一可在本仓库落地的路径；任何服务端改造都缺源码与部署/数据库授权 |

判定说明：这不是“服务故障”。本轮没有获得任何真实服务响应，也没有服务端源码；因此既不能宣布复用通过，也不能把缺口推给“部署/外部依赖”后当作完成。`WORK_FAILED` 的原因是**关键服务行为与数据约束缺少本轮真实依据**，缺口与最小区分实验见 §9。

---

## 1. 目标、方法与授权边界

### 1.1 本次要回答的关键可行性目标

| ID | 目标 | 为什么影响后端方案 |
|---|---|---|
| Q1 | 本轮是否产生服务端增量（端点/字段/表/迁移） | 决定“复用”还是“最小改造” |
| Q2 | 既有 `/upload_data` 契约能否承载本轮读数（负温、35.1℃、0.1℃ 精度、50 条批） | 决定是否需要改服务端校验/类型 |
| Q3 | `(dev_id,time)` 幂等语义是否足以支撑客户端重试/补传 | 决定是否会重复入库或静默丢数 |
| Q4 | 写入量、单行体积与容量是否需要后端改造 | 决定是否需要分区/清理/扩容 |
| Q5 | 身份、归属与写权限是否为复用前提 | 决定是否必须加鉴权/绑定校验 |
| Q6 | 服务/数据库的可核验入口是否存在 | 决定本轮能否给出“复用成立”的结论 |

### 1.2 实际执行的方法（有界）

1. 读 `readme.txt`、`file_manifest.txt`、最近提交、`服务器迁移记录.md`、IC-002 §6、`system_analysis_current.md`。
2. 静态核对 Android 上传链路：`CloudConfig.kt`、`ApiService.kt`、`UploadPayload.kt`、`CloudUploadRepository.kt`、`UploadOutbox.kt`、`MqtttService.kt`（BLE 轮询、入库/上传触发、网关缓存复读）。
3. 静态核对网关聚合/缓存路径：`CH592EVT/EVT/EXAM/BLE/beiwov2/APP/app_um2006A.c`（`sensorres[]` 与 `build_padded_frame_blocks`）。
4. 确认仓库是否存在服务端源码：工作树无 `abr.py`，`git log --all --diff-filter=A` 亦无 `abr.py` 历史。
5. 在**本机 127.0.0.1 临时端口**运行 Flask+SQLite 桩，按迁移记录声明的 DDL/语义执行 13 个有界请求（§4）。
6. 由代码常量与表结构做容量/恢复的算术推导（§10），不冒充实测。

### 1.3 明确未执行的通道及原因

| 通道 | 状态 | 原因 |
|---|---|---|
| `ssl://8.140.23.253:8883` MQTT 探测/认证 | 未执行 | 环境 `existing_service` 未登记联网入口，`allowed_external_actions` 为空；不猜凭据、不使用迁移记录中的真实设备/root 凭据 |
| `http://8.140.23.253:5000/upload_data` 请求 | 未执行 | 同上；且写入为生产副作用，需隔离测试账号与清理授权 |
| MySQL 直连 / 回读 | 未执行 | 管理、重部署、数据库直连入口均未确认；无授权 |
| 服务器版本/运行状态（emqx status、容器、systemd） | 未执行 | 同上 |
| `蹬被子报警器服务器部署方法.png` | 未读取 | 当前模型不支持图像输入；未据此推断任何配置 |
| 任何服务端源码审查 | 不可执行 | 仓库无 `abr.py` 或其它服务端源码 |

没有访问未授权数据、没有写生产记录、没有部署/迁移、没有使用文档中的凭据。

---

## 2. 已核验事实（本轮，静态/本地）

| 事实 | 证据位置 |
|---|---|
| 仓库无服务端源码；`abr.py` 在全部 Git 历史中从未出现 | `find` 无匹配；`git log --all --diff-filter=A --name-only` 中 `abr.py` 路径数 = 0 |
| HTTP 基址与 broker 地址集中在 `CloudConfig`：`ssl://8.140.23.253:8883`、`http://8.140.23.253:5000` | `cloud/CloudConfig.kt` |
| 上传端点 `POST /upload_data`，请求体 `{phone,mac,readings[{devId,time,temperature,humidity}]}`；2xx 视为成功 | `ui/zhuce/ApiService.kt`、`cloud/UploadPayload.kt`、`cloud/CloudUploadRepository.kt:RetrofitUploadApi` |
| 单批上限 50；失败退避 60→1800 s；`maxAttempts=20`；成功即逐行删除；上传在独立作用域串行 | `CloudUploadRepository.Config`、`uploadPendingOnce` |
| 本地待发箱 `pending_uploads` 以 `(device_id,time)` 去重；`attempts>=20` 的行不再自动重试 | `UploadOutbox.dueRows`、`SqliteUploadOutboxStore` |
| BLE 每 180 s 读一次，手动"立即读取"可提前触发 | `SensorCadence.SAMPLE_PERIOD_MS`、`MqtttService` 的 `READ_INTERVAL_MS`/`wakeReadNow` |
| BLE 路径的 `time` = 手机接收时刻（`System.currentTimeMillis()/1000`），不是传感器采样时刻 | `MqtttService.processTemperatureHumidityData`（BLE 调用不传 `timeOverride`） |
| 网关为每个已绑定设备保存最近一次有效值（`sensorres[]`），收到 RF 后重建并写入 BLE 特征值；GATT 读取只是取回该缓存，无新鲜度标志/采样时间 | `APP/app_um2006A.c`（`sensorres`、`build_padded_frame_blocks`、`SimpleProfile_SetParameter`） |
| `0.0/0.0` 仍走"连续 3 次才处理"的旧哨兵分支（与本轮无直接关系，但影响首次读数延迟） | `MqtttService.kt:743-761` |
| 环境未登记任何指向 `existing_service` 的网络/回读入口 | Unified 环境 `targets.existing_service` 无 `access` 条目；`current_capability_permissions.allowed_external_actions=[]` |

仅用户声明、本轮未核验的内容（来源均为 `服务器迁移记录.md`）：

- EMQX 5.8.0，端口 1883/8883/8083/8084/18083，内置 mnesia 认证（用户名=MAC，密码=MAC+固定后缀）；
- MySQL 9.0.1（Docker，3306），库 `bwusernamebase`、表 `user_table`（109 行）；`sensor_data` 的 DDL 与 `UNIQUE(dev_id,time)`、`INSERT IGNORE`；
- `abr.py` 端口 5000，`/login` + `/upload_data`；`user_script.service` 开机自启；证书链 md5 与 TLS 握手曾验证；
- 迁移记录中的端到端 MQTT 测试与 `curl` 幂等测试为**迁移时**的结果，不是本轮证据。

迁移记录含明文账号/口令/AccessKey。本报告不复制、不使用；任何后续核验必须使用与生产隔离的测试账号。

---

## 3. 本轮增量与接口差异

| 变更点（readme） | 对后端可见的线上差异 | 是否需要后端改动 |
|---|---|---|
| 1 GXHT40、2 光敏分压 | 无：光照/判据不上空口、不进请求体 | 否 |
| 3 采集改 3 分钟 | 无协议字段变化；影响**请求节奏**，不影响 schema | 否 |
| 4 条件上报（升温且无光 / >35℃） | 无：筛选在传感器侧；请求体仍是同一 5 字段 | 否 |
| 5 复用平台服务器 | 地址/端口/证书不变 | 否（前提是服务行为经核验成立） |
| 6 Android 同步、退役光通信/霍尔 | `mac` 语义不变；无新端点/新字段 | 否 |
| 7 全暗阈值 1/3 | 无：不上传光照 | 否 |
| 8 UART1 调试 | 无 | 否 |

**但有一个反直觉的量级差异**：条件上报只减少 433 空口帧，**不减少云端行数**。当前 as-built 下 Android 每 180 s 从网关缓存读取一次，并按接收时刻入库+上传；网关没有“是否有新样本”的标志，也没有采样时间。因此只要守护运行、BLE 读成功，每设备每天最多 480 行（正常单手机路径；MQTT 回环/多手机共享网关可能额外增行，未核验），即使传感器当天一次都没有触发上报。这属于后端容量与数据语义的输入，不是后端可单独修复的缺陷（修复点在网关/App 的新鲜度或去重语义），本轮不实施。

---

## 4. 本地隔离实验（桩）

### 4.1 参数与规模

| 项 | 值 |
|---|---|
| 时间 | 2026-11-09T05:16:02Z（脚本打印） |
| 运行时 | Python 3.10.9 / Flask 2.2.2 / Werkzeug 开发服务器 |
| 存储 | 临时 SQLite 文件（`UNIQUE(dev_id,time)` + `INSERT OR IGNORE`），实验后删除 |
| 监听 | `127.0.0.1` 临时端口（本次 56840），**未触外网** |
| 请求数 | 13 个（11×2xx，2×400） |
| 数据量 | 538 行；SQLite 15 页 × 4096 B ≈ 61 KB（仅供量级参考） |
| 命令 | `python evidence/backend_exploration/upload_contract_stub.py` |

### 4.2 预期与实际（节选；完整原始输出见同目录 observations 文件）

| 用例 | 预期 | 实际 | 支持/不支持的结论 |
|---|---|---|---|
| 既有负温向量 `-5.0/60.0` | 接收 | `200 inserted=1`，存储 `-5,60` | 契约类型/单位兼容（桩内） |
| 升温分支值 `31.0/60.0` | 接收 | `200 inserted=1` | 服务端不需要理解判据 |
| 超温分支值 `35.1/55.0` | 接收 | `200 inserted=1` | 严格 35.0 边界在传感器侧 |
| 50 条批 | 一个请求，全部入库 | `3757 B`，`200 inserted=50` | 客户端批上限与请求体规模 |
| 同批重试 | 不重复 | `inserted=0, ignored=50`，行数不变 | 桩内幂等成立 |
| 批内同键不同值 | 先到先得 | `inserted=1, ignored=1`，保留 30.0 | 冲突无感知、无版本 |
| 已存在键改值重试 | 保留原值 | `inserted=0`，键行仍为 `31.0/60` | 静默 first-wins，客户端仍会删待发箱行 |
| 同设备相邻秒两笔 | 两行 | `inserted=2` | 秒级粒度可容纳不同秒读数 |
| 同秒两笔（批内） | 只一行 | 见上 | 同秒多个真实样本不可区分 |
| 未知 `mac` | 落库（表无 mac 列） | `200` 且无 mac 存储 | 无法审计来源网关 |
| `25.555/60.004` | 桩保留原值 | 存储原值 | **不能外推** MySQL `DECIMAL(6,2)` 的四舍五入 |
| 空 `readings` | 桩策略 | `200 inserted=0` | 真实服务策略未知 |
| 缺 `phone` / 非 JSON | 桩返回 400 | `400` | 真实服务状态码/响应体未知 |
| 1 设备 1 天（480 次） | 480 行 | 480 行，DB≈61 KB | 行数上界可被表结构容纳；体积非 InnoDB |

### 4.3 桩的结论边界（必须随结论一起引用）

- **已证实（仅桩内）**：本轮请求体可被这份 DDL/语义接收；重试不重复；同键冲突 first-wins；`mac` 不在表中；480 行/日/设备的规模可被该表结构容纳。
- **未证实**：真实服务是否存在该端点、真实响应码与 `inserted` 计数、真实 MySQL 的 `INSERT IGNORE` 全语义（MySQL 会把部分非约束错误降级为 warning，SQLite 不会）、事务边界/部分成功、鉴权与 `phone/mac/dev_id` 归属校验、持久化与回读、服务并发能力。
- 因此本实验只用于**固定契约与风险模型**，不作为“已部署服务可用”的证据。

---

## 5. 关键假设与证据状态

| # | 假设 | 状态 | 影响 / 若为假 |
|---|---|---|---|
| A1 | 已部署服务仍有 `POST /upload_data`，且接受本文请求体 | 用户声明，**未核验** | 复用前提；若缺失/改签名，需要服务端源码与部署授权 |
| A2 | `sensor_data` 存在，`UNIQUE(dev_id,time)` + `INSERT IGNORE` 生效 | 用户声明，**未核验** | 决定重试是否幂等；若 UNIQUE 缺失，重试会产生重复行 |
| A3 | 服务对重复键返回 2xx（`inserted:0`），客户端据此删待发箱 | 用户声明，**未核验** | 若返回非 2xx，客户端将无限重试到 20 次后滞留 |
| A4 | 同键不同值只保留第一笔 | 桩内成立；真实语义未核验 | 秒级冲突/缓存复读会静默丢新值，且无告警 |
| A5 | 上传不需要鉴权，`phone/mac/dev_id` 不做归属校验 | 文档未声明；**未知** | 任意客户端可写任意手机号/设备；复用风险评估的一部分 |
| A6 | 表无 `mac` 列 | 文档声明；**静态成立** | 不能按网关审计；跨网关/换绑排查只能靠 App 侧 |
| A7 | 单请求 50 行、平均 ~5.6 请求/s（1000 设备）可被 Flask 服务承受 | 未测 | 服务端并发/连接池能力未知；不据此扩容 |
| A8 | 客户端重试窗口足够覆盖真实断网 | 静态：约 8 h 后停止 | 长断网/长停机后云端永久缺行，后端无补偿入口 |
| A9 | 180 s 轮询 + 缓存复读 → 480 行/设备/日 | 静态代码成立 | 容量估算与“无新数据”语义 |
| A10 | 数值域：温度 ±3276.7℃、湿度 ≤6553.5%RH，`DECIMAL(6,2)` 可容纳 | 静态算术 | 非法/异常值也会被存成“合法”数值；服务端域校验未知 |

交易/一致性：服务端是否把一批 50 行放在单事务、部分失败如何返回，**未知**；客户端只按 2xx 整批 ack，若服务端出现“部分写入 + 2xx”，则未写入行会被客户端删除。
并发：两个客户端同 `(dev_id,time)` 并发写入时由唯一键决定胜负，**无冲突检测/合并**；并发下谁的请求先到不确定。
恢复：EMQX/MySQL/abr.py 的开机自启只是迁移记录声明；备份、恢复演练、数据库迁移回滚均无证据。

---

## 6. 复用 vs 最小改造 vs 重建

| 方案 | 内容 | 本轮可实施性 | 结论 |
|---|---|---|---|
| A 复用既有服务（推荐） | 不改端点/schema/部署，按现有契约上传 | **可实施**（客户端已按此编码） | 唯一可落地的路径；但 A1–A3 必须先核验，否则只是未证假设 |
| B 最小改造 | 增加写鉴权/归属校验、冲突上报、批次事务、返回逐行结果、增加 `mac`/采样时间列 | **不可实施**：仓库无服务端源码，且无部署/DB 授权 | 需求未要求；只有核验发现必须时才提出，需另行授权 |
| C 重建/替换后端 | 重写 Flask/迁移数据库/重新部署 | 明确禁止且无依据 | 否决 |

即使选择 B，也必须先有 A1–A3 的核验结果，否则无法定义“最小”。本轮**不写任何服务端代码**（也没有可写的源码）。

---

## 7. 给下游的 service_context（复用契约）

```yaml
service_context:
  mode: reuse_existing_deployment          # 本轮无服务端源码可改；不重部署、不重建
  source_of_truth: 服务器迁移记录.md        # 仅资料声明；运行状态未核验
  broker:
    uri: ssl://8.140.23.253:8883
    alt_ports_not_for_app: [1883, 8083, 8084, 18083]
    auth_model: "username=gateway_mac; password=gateway_mac + fixed_suffix"   # 原样保持
    trust_anchor: dengbei_care/app/src/main/assets/jd-ca.crt                  # 原样保持
    topics_unchanged: ["/<mac>/a/r0", "/<mac>/a/r1", "/<mac>/a/s0"]
  http:
    base: http://8.140.23.253:5000
    upload: "POST /upload_data"
    request: {phone: string, mac: string, readings: [{devId: "8-hex upper", time: unix_seconds, temperature: "degC", humidity: "%RH"}]}
    client_batch_limit: 50
    client_success_rule: "any 2xx => ack+delete outbox rows"
    response_body_used_by_client: false
  storage:
    database: bwusernamebase
    table: sensor_data
    declared_columns: [id, phone, dev_id, time, temperature, humidity]
    declared_unique: "(dev_id, time)"
    declared_write: "INSERT IGNORE"
    mac_persisted: false
    sample_time_semantics: "Android receive time (BLE) / gateway epoch (MQTT); not sensor sample time"
  invariants_to_preserve:
    - 请求字段名/单位/端点不变（IC-002 §6；Android 兼容面）
    - MQTT topic 与载荷格式不变
    - 不在 App 或服务端新增未约定的判据/字段
  must_not:
    - 不因客户端需联调而重建或重新部署服务
    - 不在探索/实现阶段写生产数据或执行生产迁移
    - 不在缺少源码与授权时声称已完成服务端改造
  required_before_acceptance: [D1, D2, D3, D4, D5]   # §9
```

对后端设计的下游含义：

1. 本轮没有“服务端实现任务”可排；若设计文档仍要写后端章节，应写成**复用 + 核验门禁**，不得写成“已完成/已联调”。
2. 任何“服务端加固”（鉴权、归属、冲突、事务、采样时间）都是**条件性增量**，触发条件是 §9 的核验结果，而不是本轮 readme 的直接要求。
3. 客户端 E2E（E2E-SW-002 J-1/J-5）仍需真实服务与隔离测试账号；后端探索不替代它。

---

## 8. 已核验 / 仅声明 / 未知 / 不满足

| 项 | 分类 |
|---|---|
| 请求体字段、单位、批上限、重试与退避常量、`(dev_id,time)` 客户端幂等 | **已核验（静态）** |
| 网关缓存复读、180 s 轮询、BLE 接收时刻语义 | **已核验（静态）** |
| 仓库无服务端源码、无 `abr.py` 历史 | **已核验（本地）** |
| 契约可被同构 DDL/语义接收、重试不增行、同键 first-wins | **已核验（本地桩，不可外推）** |
| EMQX/MySQL/abr.py 版本、端口、表、DDL、自启、迁移时测试 | **仅用户声明（资料）** |
| 服务当前在线、`/upload_data` 存在且响应 `inserted` | **未知** |
| `UNIQUE`/`INSERT IGNORE` 在真实 MySQL 的实际行为、事务边界 | **未知** |
| 鉴权/归属/`phone` 校验、滥用面 | **未知** |
| 持久化与回读一致、备份恢复、磁盘余量 | **未知** |
| 桩实验结果代表生产服务 | **不满足 / 明确禁止** |
| 复用既有服务的运行期成立 | **不满足（本轮无依据）** |

---

## 9. 真实服务的最小区分实验（未执行；需授权）

> 目标：用最少写入、可回滚的方式，把 A1–A5 从“声明/未知”变成“成立/否定”。全部实验只使用**与生产隔离的测试手机号、测试网关 MAC、测试 dev_id 和合成温湿度**；不得使用文档中的真实账号或任何真实用户数据。执行前需确认网络入口、测试账号、写入与回读授权、清理方式。

| ID | 对象与有界动作 | 原始证据 | 验收依据 | 所需授权/前置 |
|---|---|---|---|---|
| D1 | 对 `8.140.23.253:8883` 用 `jd-ca.crt` 做一次 TLS 握手 + MQTT CONNECT（**测试**用户名，非真实设备凭据），有限等待 10 s | 证书链、`CONNACK`、时间戳 | 证书链与信任锚一致；认证结果可解释；失败原因可复现 | 联网入口 + 测试 broker 账号 |
| D2 | 不写库的存活探测：`POST /login` 传无效测试数据（迁移记录称返回 400），有限等待 10 s | HTTP 状态码、响应体原文 | 端点存在且可处理请求；不据此判断上传/持久化 | 联网入口 |
| D3 | 用测试 `(phone, dev_id)` 与未来时间戳连续 POST `/upload_data` 两次（相同载荷），再按获准只读入口回读 | 两次状态码/`inserted`、回读行、行数 | 第一次 `inserted=1`；第二次 `inserted=0` 且行数不变；回读值/时间与请求逐字段一致 | 联网入口 + 测试账号 + 写入授权 + 只读回读 + 清理 `DELETE` 授权 |
| D4 | 同键不同值、批内一项非法、空 `readings`、缺 `phone` 各一次 | 状态码/响应体、回读行、服务日志（若获准） | 冲突取舍、部分成功/原子性、4xx 边界有明确可复现结论 | 同 D3 |
| D5 | 未知 `phone`/`mac`/`dev_id` 与不带鉴权的请求各一次（仅合成数据） | 状态码/响应体、是否落库 | 归属/鉴权是否存在；是否存在任意写 | 同 D3；不触真实用户数据 |

通过这些实验后，复用结论才能从“契约层可行”升级为“运行期成立”；若 D3/D4 否定幂等或事务假设，再评估 §6 方案 B，并明确所需源码与部署授权。

---

## 10. 容量与恢复的算术（非实测）

**云端行数上界**

```
每设备每日读取次数 = 86400 / 180 = 480
1000 设备每日行数 ≈ 480,000
1000 设备每年行数 ≈ 175,200,000
```

前提：守护运行、BLE 读成功、网关持续返回缓存值；按正常单手机路径计。传感器条件上报只影响空口帧，不改变该上界（§3）。MQTT 回环/多手机共享网关可能额外增行，未核验，不并入本上界。

**体积估算（区间，按 InnoDB 经验值，不是实测）**

```
行体（phone≤20 + dev_id8 + time8 + 2×DECIMAL(6,2) + 头/空位图） ≈ 46–70 B
再加唯一索引 (dev_id,time) 与页开销 → 约 66–150 B/行
480,000 行/日 × 66–150 B ≈ 32–72 MB/日
→ 约 12–26 GB/年（1000 设备）
```

交叉核对：迁移记录按 1 分钟/条给出“1000 设备半年 26 GB”（≈259,200 行/设备/半年、约 200 B/行）；按 180 s 折算为 1/3 行数 ⇒ 约 8.6 GB/半年、约 17 GB/年。与上式同量级。80 GB 系统盘对一年内规模可行，但需监控磁盘、索引膨胀与备份空间；3 年以上或设备数上升需重新评估（不据此扩容）。

**客户端恢复窗口**

```
退避序列（attempts 1..20）：60,120,240,480,960,1800×15
累计 ≈ 28,860 s ≈ 8.0 h
达 20 次后：行保留在 pending_uploads，但不再满足 dueRows 条件 → 无自动重试
```

因此服务端停机 > ~8 h 会产生云端缺行，且后端没有重放/补偿入口的证据；这属于客户端重试策略与运维恢复的边界，不能由服务端“自动补回”。

---

## 11. 结论与请求的下一步

1. **设计结论（可支持）**：本轮 readme 的八点不要求服务端代码、表结构或部署变更；复用既有 `POST /upload_data` 是唯一与本仓库能力相符的路径。此为契约层结论，不等于服务已可用。
2. **不成立的项**：既有服务的运行期行为、真实 `UNIQUE`/`INSERT IGNORE`、鉴权/归属、持久化与回读，本轮均无真实依据；本地桩不能替代。按能力契约返回 **WORK_FAILED**。
3. **请求的最小外部操作**：提供与生产隔离的**测试服务入口/测试账号**及获准的只读回读（或明确授权在测试窗口内写入并清理合成数据），以执行 §9 的 D1–D5。对象、动作、证据与验收依据已列明；未获授权前不连接、不写库、不猜凭据。
4. **不需要做的事**：不需要重做服务、不需要重新部署、不需要为“联调”修改服务端；在 D1–D5 没有否定 A1–A4 之前，也不应新增服务端功能。

---

## 附录 A：本次确认的文件身份（SHA-256）

| 文件 | SHA-256 |
|---|---|
| `readme.txt` | `c6f19954d54c29136db95d75dcc28816d84d5ce57ea449714a0c58ba80a1e6dd` |
| `服务器迁移记录.md` | `e883e4df73b109dd7ac08c862e350ed9b11372a9e073930dd7bcf9e4749811ed` |
| `artifacts/interface_contract.md` | `6af5f0a65c819033390e94099c77ede83af53b6a2750edf55debd9fe42d2ff22` |
| `artifacts/system_analysis_current.md` | `2645bbffc048fbbe5ed8781ec5365a1b5d8cbc17fa9279364d5e16a803bc31a5` |
| `dengbei_care/.../cloud/CloudConfig.kt` | `d7daae2db108ec9b8f349ec61a9148bfd762e58d3fdd78e110fd81a7279b6cfb` |
| `dengbei_care/.../cloud/UploadPayload.kt` | `c0fff0ed06404895c52b216d4ce0c31657c2c4a6a509f29d015e6d579a6d98a9` |
| `dengbei_care/.../cloud/CloudUploadRepository.kt` | `c221de8da8d78ecc4b41e341daca69ece754fe8e18fd95920cdb065612b84a1c` |
| `dengbei_care/.../cloud/UploadOutbox.kt` | `63d06a21e4f2c22d9e77580b81fc6056fe523d8e17ddfef6c6246a6da543dbf7` |
| `dengbei_care/.../ui/zhuce/ApiService.kt` | `7d9fd0d15994174f13857fff29db880edd697f156fbfc4bbc4316237aab7b444` |
| `dengbei_care/.../MqtttService.kt` | `dd3847ff2fec80289f0e82282140fab77b8f0c78e85e59ff6a94040b934a88ac` |
| `evidence/backend_exploration/upload_contract_stub.py` | `5134c6200ba42fed4ee43adcac2e28c1c7454db0042d3c5ff97bdacef4629a8a` |
| `evidence/backend_exploration/upload_contract_observations.txt` | `8f200994dcc3b338aff1f74cf4ed5b81faa8a046aa5a62c45cd50027a3e153e1` |

检查时点：2026-11-09T05:16Z（实验）/ 13:16+08:00。摘要只标识本次读取的对象内容，不证明远端服务或板上对象未变化。

## 附录 B：实验可复跑性

```
cd D:\mypro\beiwo2\AIP
python evidence/backend_exploration/upload_contract_stub.py
# 预期：EXIT=0，13 个用例，11×2xx / 2×400，538 行；仅监听 127.0.0.1
```

脚本不使用外部网络、不读取凭据、不写仓库外持久文件（临时 SQLite 在退出时删除）；Flask/Werkzeug 为迁移记录之外的本机开发依赖（已存在，未安装/升级）。
