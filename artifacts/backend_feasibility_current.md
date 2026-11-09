# 后端可行性探索报告（BE-EXP-003 · 当前）

负责角色：后端工程
检查时点：2026-11-10 07:08:55–07:14:09（Asia/Shanghai）＝ 2026-11-09T23:08:55Z–23:14:09Z（UTC）；留档证据来自 23:14:02–23:14:09Z 会话
结论代码：**EXPLORATION_COMPLETE**（本能力在授权范围内的只读核验目标已全部执行并取得本轮原始响应）
结论边界：**不是**“上传链路通过”。`/upload_data` 的写入受理、幂等、持久化与回读仍需端到端阶段以隔离测试身份执行；本报告只固定其判别方法和当前证据边界。

被测产品源码：`D:\mypro\beiwo2\AIP`，接手 HEAD `1b565583e556c08f87b560749c92a28dd2d9466d`（核验开始时 `git status --porcelain` 为空，此后仅新增本报告与本轮证据三项）。
唯一环境/授权依据：本次 Unified 汇总——`existing_service_ssh_inspection`＝enabled，`allowed_external_actions=[inspect_existing_service_readonly]`，30 s 上限；`environment_repair.enabled=true` 但本轮无需修复。

---

## 0. 结论摘要

| 可行性问题 | 本轮结论 | 依据 |
|---|---|---|
| 既有服务当前是否在线 | **已核验成立** | SSH 登录成功（0.3 s）；22/1883/3306/5000/8083/8883/18083 从本机 TCP 可连（1–22 ms） |
| 运行版本与服务进程 | **已核验成立** | EMQX **5.8.0**，uptime 17 d 0 h 44 m；`user_script.service` active/enabled，pid 20644 = `python /root/abr.py`，监听 0.0.0.0:5000 |
| 部署代码是否仍是上次检查的对象 | **是（未变化）** | `/root/abr.py` size=8949 B，mtime **2026-10-22 19:31:04 +0800**；与 BE-EXP-002 记录逐项一致；进程自 2026-10-24 06:29:40 起未重启（NRestarts=0） |
| 客户端 6 个 POST 路由是否仍存在 | **已核验成立（6/6）** | GET 全 405 + `Allow: OPTIONS, POST`；`/doesNotExist` 404 作对照 |
| `/upload_data` 的 OPTIONS 预处理是否暴露鉴权头要求 | **未暴露** | `OPTIONS /upload_data` → 200，仅 `Allow: OPTIONS, POST`；无 `Access-Control-Allow-Headers`，仅 `Access-Control-Allow-Origin: *` |
| `/upload_data` 历史 401 的性质 | **收窄但未定性** | 2026-10-22T19:32:28 同一秒 401/401/200；**200 那次已进入 `mysql.connect()`（abr.py:222）**；三次均来自 127.0.0.1，非真实 App |
| 当前客户端是否携带任何凭证 | **否** | `ApiService.uploadData` 仅 `Content-Type`；全工程无 `Authorization`/`Bearer`/`@Header`；`LoginResponse.token` 解析后未使用 |
| 是否被真实业务使用 | **仍无** | journal 全量仅 14 条 POST，末次为 2026-10-22T19:32:28；EMQX 当前 `connections.count=0`、`sessions.count=0`、`subscribers.count=0`、`clients: No clients` |
| 本轮 readme 八点是否要求服务端增量 | **否**（契约层） | 不上传光照/新字段；上报筛选在传感器侧；地址/端口/证书不变 |
| 真实上传/幂等/持久化回读 | **未执行（本能力未获写授权）** | §9 给出 D1–D6 最小实验；不以 HTTP 2xx 或本地桩替代落库结论 |
| 复用结论 | **复用成立；上传可用性待 D1** | 服务、端口、路由、版本均已核验；不得写“服务端已通过/已联调” |

判定说明：BE-EXP-002 已把“服务存在、端口、路由、版本”定为已核验；本轮**重新连接**并对同一部署对象取到了新的原始响应，同时把“历史 401”从“可能是新增鉴权”收窄为“**同一调用方在同一秒内先被拒两次、第三次进入数据库连接**”，因此更接近**载荷/归属校验**而非固定请求头凭证；但没有响应体、没有源码内容、也没有本轮写入授权，**机制仍属未知**，不得据此写实现。

---

## 1. 本轮目标、执行与未执行

### 1.1 关键可行性目标

| ID | 目标 | 结果 |
|---|---|---|
| Q1 | 既有服务是否仍在线、运行哪个版本、由哪个进程提供服务 | **成立**（§3） |
| Q2 | `服务器迁移记录.md` 所列端口是否监听、是否对外可达 | **成立**（§3） |
| Q3 | 当前客户端编码的 6 个 HTTP 路径是否存在 | **成立**（§4） |
| Q4 | 当前部署代码是否仍是上次检查的同一对象 | **成立（size/mtime 未变，进程未重启）**（§3、§5） |
| Q5 | 本轮八点需求是否产生服务端字段/端点/表结构增量 | **不产生**（§6） |
| Q6 | 上传写入、幂等、事务、回读 | **未执行**（无写授权/DB 授权；§9 给方法） |
| Q7 | 最近业务活跃度 | **仍为零**（§4.3） |

### 1.2 实际执行（有界、只读、可复跑）

1. 读取 `readme.txt`、`服务器迁移记录.md`、`artifacts/system_analysis_current.md`、`artifacts/interface_contract.md`、`artifacts/android_architecture.md`、`file_manifest.txt`、Git 状态与 HEAD。
2. 定点读取客户端契约源码：`cloud/CloudConfig.kt`、`cloud/UploadPayload.kt`、`cloud/CloudUploadRepository.kt`、`cloud/UploadOutbox.kt`、`ui/zhuce/ApiService.kt`，并检索 `Authorization|Bearer|@Header|token`。
3. 本机 → 服务器 8 个端口有界 TCP 探测（仅 connect/close，5 s/端口）。
4. 通过已登记 SSH 入口执行 **1 个留档会话、19 组只读命令**，单命令 ≤12 s、整轮 6.9 s（`ssh_readonly_probe_g002.py`）：
   - 身份/OS/时间基准、`systemctl is-active/is-enabled`、`systemctl show`（启动时间/PID/重启次数）；
   - `ss -lnt`、端口→进程归属、`docker ps`、`stat /root/abr.py`；
   - `dpkg -l emqx`、`emqx ctl status/broker/broker stats/listeners/clients list`；
   - HTTP：`GET /`、`OPTIONS /upload_data`、6 条业务路径 + 1 条对照路径的 GET 探测（只读，读 `Allow` 头判路由存在）；
   - `journalctl -u user_script`：全量请求行计数、末 5 条 POST、2026-10-22 19:31–19:33 上传窗口原文；
   - `df -h /`、`free -m`。
5. 对上一次检查的对象做**身份对照**（size/mtime/PID/启动时间/EMQX uptime），判断是否仍为同一部署对象；**未**覆盖或改写任何历史证据文件。

### 1.3 明确未执行及原因（授权边界）

| 动作 | 未执行原因 |
|---|---|
| `POST /upload_data`、`/login`、`/register`、`/setmac` 等任何写/变更类调用 | 授权为 `inspect_existing_service_readonly`，明确“不调用写入接口”；本轮只用 GET/OPTIONS |
| MySQL 连接、`docker exec`、任何 SQL | 授权明确“不直连数据库”；`UNIQUE(dev_id,time)`/`INSERT IGNORE` 仍为资料声明 |
| 读取 `/root/abr.py` 内容 | 授权范围限定“服务状态、监听端口、必要的近期服务日志”；只取 `stat` 元数据，未读源码，故 401 分支无法定位 |
| `sudo`、文件/配置/数据修改、启停重启、部署、迁移、改防火墙 | 授权明确禁止 |
| 使用/回显资料中的口令、AccessKey、设备凭据、手机号 | 脚本运行时从资料读取连接口令仅用于认证；证据与报告统一脱敏，无明文 |
| 手机、串口、烧录、逻辑分析仪、MQTT 客户端连接 | 不属本能力；对应入口在本能力下 absent/unavailable，且会引入凭据使用与额外汇总 |

---

## 2. 本轮原始证据索引（新产出，不复用旧结论作本轮依据）

| 文件 | 内容 |
|---|---|
| `evidence/backend_exploration/ssh_readonly_inspection_g002.txt` | 本轮留档 SSH 会话（23:14:02–23:14:09Z）完整原始 stdout/stderr/exit、时间戳、主机指纹、每命令耗时（319 行） |
| `evidence/backend_exploration/ssh_readonly_probe_g002.py` | 产生上述证据的可复跑脚本（读资料取连接信息、脱敏、有限等待、无写动作） |

主机指纹：`ssh-ed25519` SHA-256 `ty/ysZ6Z9cvGUtYfQ26082EJXBWFFzU9pYBm2tXPbJQ`，与 BE-EXP-002 两次会话一致；该指纹只证明本轮与上次连到同一主机，**未**与任何权威清单比对，不构成主机身份认证结论。

---

## 3. 运行期核验（本轮实测值）

| 项 | 实测值 | 证据段 |
|---|---|---|
| 主机 / 内核 / 发行版 | `iZ2zeih0gmrk47og5r4z7sZ` / Linux 6.8.0-136-generic x86_64 / Ubuntu 24.04.4 LTS | `identity` |
| 服务器时间 | UTC `2026-11-09T23:08:56Z`（本地 07:08:56 +08:00） | `identity` |
| `emqx.service` | active/enabled；启动 `2026-10-24 06:29:44 CST`；PID 20845；NRestarts=0 | `systemd_units`/`systemd_detail` |
| `user_script.service` | active/enabled；启动 `2026-10-24 06:29:40 CST`；PID 20644；NRestarts=0 | 同上 |
| EMQX 版本 / uptime | `5.8.0`（dpkg `ii emqx 5.8.0 amd64`）；`17 days, 0 hours, 44 minutes` | `emqx_version`/`emqx_broker` |
| EMQX 监听 | 1883/8883/8083/8084 全 `running: true`、`acceptors: 16`、`current_conn: 0`、`max_conns: infinity` | `emqx_listeners` |
| EMQX 当前负载 | `connections.count=0`、`sessions.count=0`、`subscribers.count=0`、`subscriptions.count=0`、`retained.count=3`；`clients list: No clients` | `emqx_broker_stats`/`emqx_clients` |
| HTTP 服务 | 0.0.0.0:5000，`python` pid 20644（`/root/venv/bin/python /root/abr.py`）；`Server: Werkzeug/3.1.8 Python/3.12.3` | `process_owner`/`http_headers_root` |
| `/root/abr.py` | size=**8949 B**，mtime=**2026-10-22 19:31:04.322 +0800**；备份 `abr.py.bak_20260926` 7048 B（2026-09-26 08:57:58） | `abr_metadata` |
| MySQL | 容器 `mysql`，镜像 `mysql:latest`，`Up 2 weeks`；`0.0.0.0:3306->3306/tcp`（docker-proxy）；未执行任何 SQL | `docker_ps`/`process_owner` |
| 资源 | `/` 79 G，已用 7.1 G，可用 68 G（10%）；Mem 3499 MiB，available 1952 MiB；swap 0 | `resources` |
| 对外 TCP | 22/1883/3306/5000/8083/8883/18083 全部 TCP_OPEN（1–22 ms） | `local_tcp_probe` |

结论：**既有服务真实存在且持续运行**；当前部署对象与 BE-EXP-002 检查时是同一份 `abr.py`（size/mtime 未变）且进程从未重启，因此上次观测到的 401/200 行为对**当前部署**仍有参考价值，但**不等于本轮复测**。

---

## 4. 接口与活跃度核对

### 4.1 路由存在性（仅 GET/OPTIONS，未触发写逻辑）

| 路径 | 客户端使用 | 实测 | 判读 |
|---|---|---|---|
| `/login` | 是 | 405 + `Allow: OPTIONS, POST` | 存在 |
| `/upload_data` | 是 | 405 + `Allow: OPTIONS, POST` | 存在 |
| `/sendVerificationCode` | 是 | 405 + `Allow: OPTIONS, POST` | 存在 |
| `/verifyCode` | 是 | 405 + `Allow: OPTIONS, POST` | 存在 |
| `/register` | 是 | 405 + `Allow: OPTIONS, POST` | 存在 |
| `/setmac` | 是 | 405 + `Allow: OPTIONS, POST` | 存在 |
| `/doesNotExist`（对照） | — | 404 | 探测方法有效 |

- `GET /` → 404（Flask 无根路由），响应头 `Access-Control-Allow-Origin: *`，无 `WWW-Authenticate`。
- `OPTIONS /upload_data` → **200**，`Allow: OPTIONS, POST`，**无** `Access-Control-Allow-Headers`，**无** `WWW-Authenticate`。因此从预检响应**看不出**客户端需要补任何自定义鉴权头；这只能说明 CORS 预检不要求头，**不能**证明 POST 不需要凭证。

### 4.2 历史 401 的收窄（关键新增证据）

`journal_20261022_upload_window` 原文（本轮读取，未修改）：

```
19:31:04  systemd: Stopping/Stopped/Started user_script.service
19:31:05  python[647759]: * Serving Flask app 'abr'  ... Running on 0.0.0.0:5000
19:32:28  python[647759]: "POST /upload_data HTTP/1.1" 401 -
19:32:28  python[647759]: "POST /upload_data HTTP/1.1" 401 -
19:32:28  python[647759]: /root/abr.py:222: DeprecationWarning: 'db' is deprecated, use 'database'
19:32:28    conn = mysql.connect()
19:32:28  python[647759]: "POST /upload_data HTTP/1.1" 200 -
```

可确认的事实：
1. 401/200 发生在**更换 abr.py 并重启服务后 84 秒**，来自 **127.0.0.1**（部署者在服务器本机自测，不是真实 App/设备）；
2. **同一秒、同一端点**先两次 401、第三次 200；
3. 200 那次在返回前执行到 `abr.py:222` 的 `mysql.connect()`。

由此**收窄**（非定性）：
- 若三次请求的 HTTP 头完全相同，则 401 只能来自**请求体内容/归属状态**（如 phone 未注册、mac 未绑定、dev_id 不属该用户等），而不是固定 header 凭证；
- 若部署者在三次 curl 中改过头（例如临时加了 token），则仍可能是 header 鉴权；
- 本轮未读 `abr.py` 内容、未取得 401 响应体，**无法区分**。因此上一版“可能是新增鉴权”的两种解释都保留，但“**与请求体/身份归属相关**”的权重更高。

### 4.3 业务活跃度（仍为零）

| 观察 | 值 |
|---|---|
| journal 请求行总数 | 92（含本轮 3 次短会话与 BE-EXP-002 的 127.0.0.1 GET/OPTIONS 探测行，不用于业务判断） |
| **POST 行总数** | **14**（全部为部署/调试期） |
| 末次 POST | **2026-10-22T19:32:28+08:00**（3 次 `/upload_data`：401/401/200） |
| 2026-10-22 之后 | 无任何 POST；仅有 BE-EXP-002 与本轮的只读探测 |
| EMQX 自 2026-10-24 06:29 重启起 | `connections/sessions/subscribers=0`，`No clients`，`retained=3`（重启前遗留） |

结论：**没有任何真实“传感器→网关→App→服务”流量**可用于推断上传、幂等或持久化；本轮也不得以历史 200 代替当前写入验证。

---

## 5. 资料 vs 部署差异（本轮复核）

| ID | 观测 | 本轮变化 | 影响 |
|---|---|---|---|
| F1 | `/upload_data` 在 2026-10-22 出现 401/401/200，客户端不带凭证 | **新增**：200 那次进入 `mysql.connect()`；三次均来自 127.0.0.1 | 上传存在**受理前校验**；机制未定；端到端 D1 必须先定性 |
| F2 | 部署 `abr.py` 8949 B / 2026-10-22 19:31:04，晚于资料（2026-09-26 的 7048 B） | **未变化**（仍为同一文件） | 资料已不是当前部署的准确描述；接口细节须以运行时证据或授权源码审查为准 |
| F3 | Werkzeug 3.1.8 开发服务器、单进程 | 未变化 | 并发/长连接能力无依据；容量结论只作量级参考 |
| F4 | MySQL 浮动 tag `mysql:latest`、无 healthcheck、3306 发布到 0.0.0.0 | 容器已 Up 2 weeks | 版本不可从外部确认；公网暴露是运维风险，本轮不改 |
| F5 | 服务与 caddy/postgres/RustDesk/next-server 共租；3.5 G 内存、无 swap | available 1952 MiB | 容量估算不能只看本项目 |
| F6 | 6 路由存在但 `GET /` 404；CORS `*` | 未变化 | 不影响客户端（客户端只调具体路径） |

---

## 6. 本轮需求（readme 八点）对服务端的增量

| 变更点 | 对线上可见差异 | 需后端改动 |
|---|---|---|
| 1 GXHT40、2 光敏分压 | 无（不上空口、不进请求体） | 否 |
| 3 采集 3 分钟 | 请求体不变，只影响节奏 | 否 |
| 4 升温 >0.9℃ 且无光 / >35℃ 才上报 | 筛选在传感器侧，服务端不重算 | 否 |
| 5 复用平台服务器 | 地址/端口/证书不变（本轮实测端口/路由/版本） | 否（须过 §9 D1） |
| 6 Android 同步、退役光通信/霍尔 | `mac` 语义不变 | 否 |
| 7 全暗阈值 1/3 | 不上传光照 | 否 |
| 8 UART1 调试 | 无 | 否 |

**结论**：本轮需求不产生端点、字段、表结构或部署增量。当前唯一会改变后端结论的未知项是 §9 的 D1（上传受理条件）；它不是 readme 的新功能，而是**既有部署已偏离资料**带来的复核门禁。

---

## 7. 已核验 / 仅声明 / 未知 / 不满足

| 项 | 分类 |
|---|---|
| 服务在线、unit 状态、启动时间、PID、NRestarts | **已核验（本轮原始输出）** |
| EMQX 5.8.0、uptime、4 listener running、当前连接 0 | **已核验** |
| 6 条 HTTP 路由存在（405+Allow）、对照 404、OPTIONS 200 | **已核验** |
| HTTP 栈 Werkzeug 3.1.8 / Python 3.12.3；5000 由 abr.py 提供 | **已核验** |
| `/root/abr.py` size/mtime；MySQL 容器运行、3306 发布 | **已核验（仅元数据）** |
| 2026-10-22 上传窗口 401/401/200、200 进入 mysql.connect | **已核验（日志原文）** |
| 10-22 之后无 POST（约 18.5 天）、自 10-24 起无 MQTT 客户端 | **已核验** |
| 客户端无 Authorization/Bearer、token 未使用、批上限 50、2xx 即成功、退避 60→1800 s×20 | **已核验（静态源码）** |
| `sensor_data` 表、`UNIQUE(dev_id,time)`、`INSERT IGNORE`、`DECIMAL(6,2)` | **仅资料声明**（未连 DB） |
| 401 的具体机制（token / 归属 / 注册态 / 请求体校验） | **未知**（未读源码、无响应体） |
| 上传成功响应体、`inserted` 计数、事务边界、部分成功 | **未知**（未调用写接口） |
| MQTT 认证仍为“用户名=MAC、密码=MAC+后缀” | **未核验（本轮无客户端连接）** |
| MySQL 实际 server 版本 9.0.1 | **仅资料声明**（浮动 tag） |
| 真实上传/幂等/持久化回读 | **未执行（阶段外）** |
| 本地桩代表生产服务 | **不满足/明确禁止** |

---

## 8. 复用 vs 最小改造

| 方案 | 内容 | 本轮可实施性 | 结论 |
|---|---|---|---|
| A 复用既有服务 | 不改端点/schema/部署，按现有契约上传 | 服务/端口/路由/版本已核验；**上传受理条件待 D1** | **首选**，但不得写“已通过” |
| B 最小改造 | 若 D1 证实需要归属/凭证：客户端补对应字段或服务端兼容；另可加 `mac`/采样时间列、冲突上报、批次事务 | 需源码 + 部署 + DB 授权（本轮均无） | **条件性增量**，触发条件＝D1–D3 |
| C 重建/替换 | 重写 Flask/重部署/迁库 | 无依据且被禁止 | 否决 |

不因客户端要联调就重做服务或部署；D1 未定性前不写任何服务端代码（本地也没有服务端源码可改）。本轮未产生任何产品源码改动。

---

## 9. 端到端阶段必须执行的最小区分实验

> 前置条件（缺一即记 NOT EXECUTED 及原因）：隔离测试手机号、测试网关 MAC、测试 dev_id、合成温湿度；HTTP 写授权；只读回读入口；清理策略与授权。**不得**使用资料中的真实设备凭据或真实用户数据。
> 执行时同步记录：请求字节（脱敏）、状态码、响应体、服务端日志行、回读行。

| ID | 有界动作 | 原始证据 | 验收依据 |
|---|---|---|---|
| **D1（最高优先）** | 用**已注册的测试身份**发送 1 条 `/upload_data`（不带任何凭证）；再用**未注册/不匹配归属**的合成身份发送 1 条 | 两次状态码 + 响应体原文（脱敏）+ 服务日志行 | 若已注册 2xx、未注册 401 → 确认是**归属/注册校验**，真实 App 路径可用；若已注册也 401 → 需要凭证，属契约变更，须明确授权后处理；两次都 2xx → 排除鉴权，记录 401 为历史逻辑 |
| D2 | 相同 `(dev_id,time)` 连续 POST 两次（相同值） | 两次状态码/响应体、回读行、行数 | 第一次 `inserted=1`；第二次 `inserted=0` 且行数不变 |
| D3 | 同键不同值 1 次；批内一项非法 1 次；空 `readings` 1 次 | 状态码/响应体、回读行 | 冲突取舍（first-wins/覆盖）、原子性、4xx 边界有可复现结论 |
| D4 | 通过获准只读入口回读 `(dev_id,time,temperature,humidity)` | 回读行原文 | 与请求逐字段一致（含 `DECIMAL(6,2)` 舍入）；无回读入口 → 持久化子项 NOT EXECUTED |
| D5 | 未知 `phone`/`mac`/`dev_id` 各 1 次（仅合成） | 状态码/响应体 | 归属校验是否存在、是否可任意写 |
| D6 | 一次真实 App 上传（真机）与服务端日志、回读两侧对账 | logcat、服务日志行、回读行 | 2xx 与落库同时成立；确认真实客户端在 D1 门禁下的表现 |

D1 可与 D6 合并以减少写入次数；清理只删本实验的测试行，删除动作需单独授权。
**在 D1 完成前，任何“服务端复用已通过/上传可用”的表述都不成立。**

---

## 10. 容量与恢复（算术 + 本轮实测资源）

```
每设备每日 = 86400/180 = 480 行
1000 设备每日 ≈ 480,000 行；每年 ≈ 175,200,000 行
InnoDB 经验 66–150 B/行 ⇒ 32–72 MB/日 ⇒ 约 12–26 GB/年（1000 设备）
```

条件上报只减少 433 空帧，**不减少**云端行数（App 按接收时刻入库）。本轮实测 `/` 可用 68 G、内存 available 1952 MiB、无 swap，且与 caddy/postgres/RustDesk 共租：一年量级可容纳，三年以上或设备数上升需重新评估并监控磁盘/内存。

客户端恢复窗口（静态源码）：退避 60,120,…,1800 s、`maxAttempts=20` ⇒ 约 8.0 h 后停止自动重试，行保留但不重发；服务端无重放入口证据。若 `/upload_data` 对真实身份恒 401，则约 8 h 后全部滞留、云端 0 行 —— 这是当前最需要先排除的单点风险（D1）。

---

## 11. 给下游的 service_context（本轮刷新）

```yaml
service_context:
  mode: reuse_existing_deployment        # 不重建、不重部署；本地无服务端源码
  verified_at: "2026-11-09T23:08:55Z–23:14:09Z (SSH read-only, paramiko 2.8.1; host key sha256 ty/ysZ6…bJQ)"
  service_identity:
    abr_py: {size_bytes: 8949, mtime: "2026-10-22T19:31:04+08:00", unchanged_since: "BE-EXP-002"}
    user_script: {state: active, enabled: true, started: "2026-10-24T06:29:40+08:00", pid: 20644, restarts: 0}
    runtime: "Werkzeug/3.1.8, Python 3.12.3, single process (dev server)"
  broker:
    uri: ssl://8.140.23.253:8883
    version_verified: "EMQX 5.8.0"
    uptime_verified: "17 d 0 h 44 m (started 2026-10-24T06:29:44+08:00)"
    listeners_running: {1883: true, 8883: true, 8083: true, 8084: true}
    current_connections: 0
    auth_model: "资料声明 username=MAC / password=MAC+固定后缀；本轮无客户端连接，机制未核验"
    trust_anchor: dengbei_care/app/src/main/assets/jd-ca.crt   # sha256 5c17cf74…e1e4
    topics_unchanged: ["/<mac>/a/r0", "/<mac>/a/r1", "/<mac>/a/s0"]
  http:
    base: http://8.140.23.253:5000
    routes_verified_exist: ["/login", "/upload_data", "/sendVerificationCode", "/verifyCode", "/register", "/setmac"]
    route_probe_method: "GET -> 405 + Allow: OPTIONS, POST; OPTIONS /upload_data -> 200; unknown -> 404"
    upload_request: {phone: string, mac: string, readings: [{devId: "8-hex", time: unix_seconds, temperature: degC, humidity: "%RH"}]}
    upload_acceptance: "unknown: 2026-10-22 401/401/200 on the deployed artifact; the 200 executed mysql.connect (abr.py:222); all three from 127.0.0.1; resolve at E2E-D1"
    client_batch_limit: 50
    client_success_rule: "any 2xx => ack+delete outbox rows; non-2xx => backoff 60..1800 s, max 20 attempts"
    client_credentials: "none (no Authorization/Bearer/@Header; LoginResponse.token unused)"
  storage:
    database: bwusernamebase
    table: sensor_data            # 资料声明，本轮未连接 DB
    declared_unique: "(dev_id, time)"
    declared_write: "INSERT IGNORE"
    mac_persisted: false
    server_version: "资料声明 MySQL 9.0.1；实测镜像 mysql:latest（Up 2 weeks, 3306 published 0.0.0.0）"
  host_resources: {disk_total: "79G", disk_avail: "68G", mem_available_mib: 1952, swap: none, co_tenants: [caddy, postgres, rustdesk, next-server]}
  activity_evidence:
    post_lines_in_journal: 14
    last_post: "2026-10-22T19:32:28+08:00"
    mqtt_clients_since_2026-10-24_restart: 0
    retained_messages: 3
    note: "无真实业务流量；历史数据不能替代当前写入/幂等/持久化验证"
  invariants_to_preserve:
    - 请求字段名/单位/端点不变（IC-002 §6）
    - MQTT topic 与载荷格式不变
    - 不新增未约定的判据/字段
    - 不因联调而重建/重新部署
  must_not:
    - 不在探索/实现阶段写生产数据、执行迁移、直连数据库
    - 不在缺少源码与授权时声称服务端改造完成
    - 不把 2xx 当作持久化通过
  blockers_for_design_signoff: [D1]      # 上传受理条件定性前，不得写“服务端复用已通过”
  required_at_e2e: [D1, D2, D3, D4, D5]
```

**对后端设计的直接含义**

1. 本轮**没有服务端实现任务**可排；后端章节应写成“复用 + 门禁（D1）”，不能写成“已联调/已通过”。
2. D1 未定性前，任何“上传链路可用”的结论都不成立；这是当前唯一会改变结论的未知项。
3. 服务端加固（鉴权/归属/冲突上报/事务/采样时间列）仍是**条件性增量**，触发条件只有 D1–D3 的结果。
4. 不重部署、不重拉镜像、不改防火墙、不直连数据库；F4/F5 记录为运维风险并转相应责任方。
5. Android 侧 AA-004 的 G-6（401/403 分类与可观测）已把“不猜测鉴权协议”为设计约束；后端不为其新增协议，待 D1 结论。

---

## 12. 结论

1. **成立**：既有服务真实存在且持续运行（同一 `abr.py`、进程未重启）；EMQX 5.8.0 + 4 个 listener 正常；`user_script`/`abr.py` 在 5000 端口服务；客户端 6 个 POST 路由全部存在；磁盘/内存余量可容纳一年量级数据；本轮八点需求不产生服务端增量。
2. **收窄但未定性**：2026-10-22 上传窗口的 401/401/200 中，200 那次进入 `mysql.connect()`，三次均来自服务器本机同一秒；更像**载荷/归属校验**，但无源码、无响应体，不能定论。
3. **未知（阶段外，已给判别方法）**：上传受理条件、`UNIQUE`+`INSERT IGNORE` 真实语义、事务边界、持久化与回读、MQTT 认证机制、MySQL 实际版本。
4. **不需要做**：不需要重做服务、不需要重新部署、不需要为联调改服务端；D1 未定性前不应新增服务端功能。
5. **下一授权动作**：在端到端阶段执行 §9 D1（必要时合并 D6），落实隔离测试身份、写入授权与（若有）只读回读入口；缺条件的子项记未执行及原因。

**本报告的完成只表示“探索完成、设计可继续”，不表示上传链路或整体服务通过。**

---

## 附录 A：本轮确认的对象身份（本机 SHA-256）

检查时点：2026-11-09T23:14:09Z。摘要只标识本次读取的**本地对象内容**，不证明远端或板上对象未变化；远端 `abr.py` 仅有 size/mtime。

| 对象 | SHA-256 |
|---|---|
| `readme.txt` | `c6f19954d54c29136db95d75dcc28816d84d5ce57ea449714a0c58ba80a1e6dd` |
| `服务器迁移记录.md` | `e883e4df73b109dd7ac08c862e350ed9b11372a9e073930dd7bcf9e4749811ed` |
| `artifacts/interface_contract.md` | `6af5f0a65c819033390e94099c77ede83af53b6a2750edf55debd9fe42d2ff22` |
| `artifacts/android_architecture.md` | `d565e1413f98824c34e5b6923f943d1b89225f0169e85252969bdc0ee3e4703c` |
| `dengbei_care/.../cloud/CloudConfig.kt` | `d7daae2db108ec9b8f349ec61a9148bfd762e58d3fdd78e110fd81a7279b6cfb` |
| `dengbei_care/.../cloud/CloudUploadRepository.kt` | `c221de8da8d78ecc4b41e341daca69ece754fe8e18fd95920cdb065612b84a1c` |
| `dengbei_care/.../cloud/UploadOutbox.kt` | `63d06a21e4f2c22d9e77580b81fc6056fe523d8e17ddfef6c6246a6da543dbf7` |
| `dengbei_care/.../cloud/UploadPayload.kt` | `c0fff0ed06404895c52b216d4ce0c31657c2c4a6a509f29d015e6d579a6d98a9` |
| `dengbei_care/.../ui/zhuce/ApiService.kt` | `7d9fd0d15994174f13857fff29db880edd697f156fbfc4bbc4316237aab7b444` |
| `dengbei_care/app/src/main/assets/jd-ca.crt` | `5c17cf740ffdf416c2f31f90172ad3565966cd62c0952920b9b2ac974aefe1e4` |
| `dengbei_care/app/src/main/res/xml/network_security_config.xml` | `1f428a2cb45ea27663cb3ad1df2d993398e1a0abc2cbe4fe63fa703e993c4bd5` |
| `evidence/backend_exploration/ssh_readonly_probe_g002.py` | `e6bea685a5b51658ee38efc91e49217c822f1a47baaf1332e5401eeb96137c7a` |
| `evidence/backend_exploration/ssh_readonly_inspection_g002.txt` | `cee54a3b2908755b8e8a599092b8212339aace83b997072c78720a7fd6e7d1d7` |

本报告自身不含摘要（无法自引用），其对象身份由本次 Git 提交记录标识。

---

## 附录 B：可复跑性

```
cd D:\mypro\beiwo2\AIP
python evidence/backend_exploration/ssh_readonly_probe_g002.py
```

- 脚本从 `服务器迁移记录.md` 正则读取主机/账号/口令，不在命令行或代码中硬编码、不打印；写证据前统一脱敏。
- 单命令 ≤12 s，整轮 ≤150 s；留档会话 6.9 s；无后台任务、无遗留进程。
- 依赖：本机 Anaconda `python 3.10.9` + 已存在 `paramiko 2.8.1`；本轮未安装/升级任何包。
- 只读边界：GET/OPTIONS + systemctl/ss/docker ps/stat/emqx ctl/journalctl/df/free；无 POST、无 SQL、无 sudo、无启停。

---

## 附录 C：历史记录（保留，不重写）

**BE-EXP-001（2026-11-09 13:19，提交 `06f9c59`）结果 `WORK_FAILED`**：当时未登记服务端访问入口、`allowed_external_actions` 为空，没有真实服务响应；仅用本地桩固定契约与风险模型（13 请求、538 行、first-wins 等）。

**BE-EXP-002（2026-11-09 14:24–14:28，文件 `artifacts/backend_feasibility.md`，SHA-256 `b775ec915c72d5ae3bb6cedb62e594e047b2c9666789438810a0ee358b393fd2`）结果 `EXPLORATION_COMPLETE`**：登记 SSH 入口后首次实测在线/版本/端口/路由，发现 `/upload_data` 历史 401 与部署偏离资料，留下 D1–D6。其正文与 `evidence/backend_exploration/*` 历史证据**未改动**；本报告是新的检查时点记录，不重写旧结论，也不把旧在线状态当作本轮在线证据（本轮已重新核验）。

**本轮（BE-EXP-003）相对 BE-EXP-002 的新增事实**：同一部署对象未变（size/mtime/PID/uptime）；OPTIONS 预检不要求鉴权头；200 那次已进入 `mysql.connect()`；三次上传均来自 127.0.0.1 且为部署后 84 秒的自测；当前客户端仍无任何凭证。结论仍为“复用 + D1 门禁”，不升级为“上传通过”。
