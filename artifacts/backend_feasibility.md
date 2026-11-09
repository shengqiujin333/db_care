# 后端可行性探索报告（BE-EXP-002）

责任角色：后端工程
检查时点：2026-11-09 14:24–14:28（Asia/Shanghai）＝ 06:24–06:28 UTC
结论代码：**EXPLORATION_COMPLETE**（本轮授权范围内的只读核验目标已全部执行并取得真实证据）
结论边界：**不是**“服务端到端通过”。上传写入、幂等与持久化回读按用户安排在端到端阶段执行，本报告把它们的未知项与判别方法固定下来。

被测产品源码：`D:\mypro\beiwo2\AIP`，接手 HEAD `6967c47`（首次 `git status --porcelain` 仅有本能力新建的未跟踪证据文件）
唯一环境/授权依据：本次 Unified 汇总（`existing_service_ssh_inspection`＝enabled，`allowed_external_actions=[inspect_existing_service_readonly]`，30 s 上限）

---

## 0. 结论摘要

| 可行性问题 | 本轮结论 | 依据 |
|---|---|---|
| 既有服务是否真实在线、端口是否与资料一致 | **已核验成立** | 22/1883/3306/5000/8083/8084/8883/18083 全部 TCP 可连；`ss -lntp` 列出监听与进程归属 |
| EMQX 运行版本 | **已核验成立 = 5.8.0** | `dpkg -l emqx` + `emqx ctl broker`；`systemctl` active/enabled |
| 注册/上传服务是否在跑 | **已核验成立** | `user_script.service` active/enabled，pid 20644 = `python /root/abr.py`，监听 0.0.0.0:5000 |
| 客户端 6 个接口路径是否存在 | **已核验成立（全 6 个）** | GET 探测：6 个均 405 + `Allow: OPTIONS, POST`；`/doesNotExist` 404 作对照 |
| `/upload_data` 是否仍按资料“无鉴权”工作 | **已否证“无鉴权”** | 部署日志 2026-10-22T19:32:28 三次 `POST /upload_data` 中两次 **401**、一次 200 |
| 当前 App 是否带该鉴权 | **否** | `ApiService.uploadData` 仅 `Content-Type`，全工程无 `Authorization`/token 使用；`LoginResponse.token` 解析后未使用 |
| 本轮是否要求改服务端代码/表/部署 | **契约层仍不要求**，但**复核门禁新增 1 项** | readme 八点不上传新字段；新增门禁＝上述 401 与客户端不一致，须在端到端阶段定性 |
| `UNIQUE(dev_id,time)`+`INSERT IGNORE` 是否成立 | **仍未核验（本轮不授权 DB 直连）** | 资料声明；DB 直连与写接口调用均未授权 |
| 真实上传/幂等/持久化回读 | **未执行（按用户安排属端到端阶段）** | 本报告 §9 给出可直接执行的最小实验；缺条件项记未执行，不以桩或 2xx 代替 |
| 服务是否被真实业务占用 | **除本次探测外，近 18 天无任何 HTTP 请求；近 16 天无任何 MQTT 连接** | journal 全量 218 行仅 26 条请求行（末次业务 POST 2026-10-22）；EMQX metrics 全 0 |
| 复用结论 | **“存活+端口+路由存在”成立；“上传可用”未成立（未执行，且存在 401 反向证据待定性）** | 不缺部署，缺的是上传链路的真实依据；不需要重建/重部署 |

判定说明：上一轮（BE-EXP-001，`WORK_FAILED`）没有登记入口，一条真实依据都拿不到。本轮授权变化后已实际连上服务器并取得可复现原始输出，因此不再是“无依据”；但**上传路径出现了与文档不一致的实际行为（401）**，这一项的定性必须由端到端阶段用隔离测试身份完成，本报告不替它下结论，也不把缺口推给“部署/外部依赖”移出目标。

---

## 1. 本轮目标与授权边界

### 1.1 关键可行性目标（本轮）

| ID | 目标 | 结果 |
|---|---|---|
| Q1 | 既有服务是否在线、运行版本是什么 | **成立**（§3） |
| Q2 | 文档所列监听端口是否真的在监听、由哪个进程占用 | **成立**（§3） |
| Q3 | 客户端编码的 6 个 HTTP 路径在当前部署里是否仍存在 | **成立**（§4） |
| Q4 | 部署版本是否仍与 `服务器迁移记录.md` 描述一致 | **不一致（已定位差异）**（§5 F1/F2） |
| Q5 | 本轮 readme 八点是否产生服务端增量 | **不产生**（契约层，§6） |
| Q6 | 现有服务近期是否被真实业务使用过 | **没有**（§4.3） |
| Q7 | 上传写入/幂等/持久化 | **阶段外，未执行**（§9 给方法） |

### 1.2 实际执行的内容（有界、只读）

1. 读 `readme.txt`、`服务器迁移记录.md`、`artifacts/system_analysis_current.md`、`artifacts/interface_contract.md` §6、`artifacts/software_e2e_plan.md`（E2E-07）、`file_manifest.txt`、最近提交。
2. 定点读取客户端契约源码：`cloud/CloudConfig.kt`、`ui/zhuce/ApiService.kt`、`cloud/CloudUploadRepository.kt`；全工程检索 `Authorization|token|Bearer|@Header`。
3. 本机 → 服务器 8 个端口的有界 TCP 连通性探测（仅 connect/close，无应用层载荷）：`evidence/backend_exploration/ssh_tcp_probe.py`。
4. 通过已登记 SSH 入口做 6 组只读检查（每次单命令 ≤15 s、整轮 ≤150 s、失败即记录）：
   - `ssh_readonly_probe.py` → `ssh_readonly_inspection.txt`（身份/unit 状态/监听/docker/进程/路由探测/文件元数据/日志/版本/磁盘）
   - `ssh_readonly_probe_followup.py` → `ssh_readonly_inspection_2.txt`（journal 保留期/端口进程映射/EMQX listeners/镜像身份/OS）
   - `ssh_readonly_probe_emqx.py` → `ssh_readonly_inspection_3.txt`（broker 版本与运行时长、统计、metrics、clients、journal 跨度）
   - `ssh_readonly_probe_routes.py` → `http_route_existence.txt`（6 路径 + 1 对照路径，**仅 GET**）
   - `ssh_readonly_probe_http_history.py` → `http_history_counts.txt`（请求行计数、journal 起点）
   - `ssh_readonly_probe_http_requests.py` → `http_request_history.txt`（26 条请求行原文；仅 method/path/status）
5. 在**本机 127.0.0.1** 重跑既有桩实验确认可复现：`upload_contract_stub.py` → `upload_contract_observations_rerun.txt`（13 用例 / 11×2xx / 2×4xx / 538 行 / 61440 B，与上一轮逐项一致）。

### 1.3 明确未执行的动作及原因（保持授权边界）

| 动作 | 未执行原因 |
|---|---|
| `POST /upload_data`、`POST /login`、`/register`、`setmac` 等任何写/变更类接口 | 授权明确“不调用写入接口”；本轮全部改用 GET 观察 `Allow` 头判断路由存在性 |
| MySQL 直连、`docker exec mysql ...`、任何 SQL | 授权明确“不直连数据库”；表结构与 `UNIQUE`/`INSERT IGNORE` 仍为资料声明 |
| 读取 `/root/abr.py` 文件内容 | 授权范围限定“状态、监听端口、必要近期日志”；只取 `stat`/`ls` 元数据，未读内容（因此 401 的代码原因未定性） |
| `sudo`、任何文件/配置/数据修改、启停重启、部署、迁移 | 授权明确禁止 |
| 修改服务器时间、日志级别、防火墙 | 非本能力职责 |
| 使用/回显资料中的任何口令、AccessKey、设备凭据 | 脚本运行时从资料正则读取连接口令并只用于认证；证据文件与报告均脱敏，不含明文 |
| 手机、串口、烧录、逻辑分析仪 | 不属本能力；对应入口在本能力下为 unavailable |

---

## 2. 原始证据索引（全部为本轮新产出）

| 文件 | 内容 | 复跑命令 |
|---|---|---|
| `evidence/backend_exploration/ssh_tcp_probe_output.txt` | 8 端口 TCP 可达性（2026-11-09T06:24:26Z） | `python evidence/backend_exploration/ssh_tcp_probe.py` |
| `evidence/backend_exploration/ssh_readonly_inspection.txt` | 会话 1 全量 stdout/stderr/exit | `python evidence/backend_exploration/ssh_readonly_probe.py` |
| `evidence/backend_exploration/ssh_readonly_inspection_2.txt` | 会话 2 | `python evidence/backend_exploration/ssh_readonly_probe_followup.py` |
| `evidence/backend_exploration/ssh_readonly_inspection_3.txt` | 会话 3（EMQX） | `python evidence/backend_exploration/ssh_readonly_probe_emqx.py` |
| `evidence/backend_exploration/http_route_existence.txt` | 7 条路径的状态码与 `Allow` | `python evidence/backend_exploration/ssh_readonly_probe_routes.py` |
| `evidence/backend_exploration/http_history_counts.txt` | 请求行/方法计数、journal 起点 | `python evidence/backend_exploration/ssh_readonly_probe_http_history.py` |
| `evidence/backend_exploration/http_request_history.txt` | 26 条请求行原文 | `python evidence/backend_exploration/ssh_readonly_probe_http_requests.py` |
| `evidence/backend_exploration/upload_contract_observations_rerun.txt` | 本地桩复跑 | `python evidence/backend_exploration/upload_contract_stub.py` |

会话指纹（两次独立连接一致）：`ssh-ed25519` host key SHA-256 `ty/ysZ6Z9cvGUtYfQ26082EJXBWFFzU9pYBm2tXPbJQ`。该指纹只证明本轮两次连接到同一主机，**没有**与任何权威清单比对，不能作为主机身份认证结论。

---

## 3. 运行期核验结果（已取得真实响应）

| 项 | 实测值 | 证据 |
|---|---|---|
| 主机 / 内核 | `iZ2zeih0gmrk47og5r4z7sZ` / `Linux 6.8.0-136-generic x86_64` | 会话 1 `identity` |
| 发行版 | Ubuntu 24.04.4 LTS | 会话 2 `os_release` |
| 时间基准 | 服务器 UTC `2026-11-09T06:24:44Z`；与资料时区（CST）一致 | 会话 1 |
| `emqx.service` | `active`/`enabled`，`SubState=running`，`NRestarts=0`，启动于 2026-10-24 06:29:44 CST | 会话 1 `systemd_units/systemd_detail` |
| `user_script.service` | `active`/`enabled`，`NRestarts=0`，启动于 2026-10-24 06:29:40 CST，`ExecStart=/root/venv/bin/python /root/abr.py` | 同上 |
| EMQX 版本 / 运行时长 | `5.8.0`（dpkg `ii emqx 5.8.0 amd64`）；uptime 16 天 7 小时 56 分 | 会话 1 `emqx_version`、会话 3 `emqx_broker` |
| EMQX 监听 | 1883/8883/8083/8084 全部 `running: true`、`acceptors: 16`、`current_conn: 0` | 会话 2 `emqx_listeners` |
| HTTP 服务 | 0.0.0.0:5000，pid 20644 `python`（`abr.py`）；banner `Werkzeug/3.1.8 Python/3.12.3` | 会话 1 `http_upload_route`、会话 2 `port_process_map` |
| MySQL | 容器 `mysql` 运行中（Up 2 weeks），镜像 `mysql:latest`，id `sha256:d7f7ebf3…00b5`，digest 同值，image created 2024-07-22；`restart_policy=unless-stopped`，**无 healthcheck** | 会话 1 `docker_ps/mysql_container`、会话 2 `mysql_image/mysql_uptime_state` |
| 磁盘 / 内存 | `/` 79 G，已用 7.1 G，可用 68 G（10%）；Mem 3499 MiB，可用 1917 MiB；**无 swap** | 会话 1 `disk` |
| 其他共租服务（非本项目，仅记录存在） | caddy(80/443/2019)、postgres(127.0.0.1:5432)、RustDesk hbbs/hbbr(21115–21119)、next-server(127.0.0.1:3000)、containerd、hbrclient | 会话 2 `port_process_map` |
| `/root/abr.py` | 存在，8949 B，mtime **2026-10-22 19:31:04 +0800**；备份 `abr.py.bak_20260926` 7048 B（2026-09-26 08:57） | 会话 1 `abr_files` |

对外 TCP 可达性（本机 → 8.140.23.253，单次 5 s 上限）：22/1883/3306/5000/8083/8084/8883/18083 **全部 TCP_OPEN**（15 ms 内）。

> 由此可确认：**既有服务确实存在且在运行**，与“用户已部署”一致；同时 3306 绑定 `0.0.0.0`（`docker-proxy`），MySQL 端口对公网开放（§5 F7）。

---

## 4. 接口资料核对

### 4.1 路由存在性（GET 只读探测，未调用写逻辑）

| 路径 | 客户端是否使用 | 实测 | 判读 |
|---|---|---|---|
| `/login` | 是（`ApiService.login`） | 405 + `Allow: OPTIONS, POST` | 路由存在 |
| `/upload_data` | 是（`ApiService.uploadData`） | 405 + `Allow: OPTIONS, POST` | 路由存在 |
| `/sendVerificationCode` | 是 | 405 + `Allow: OPTIONS, POST` | 路由存在 |
| `/verifyCode` | 是 | 405 + `Allow: OPTIONS, POST` | 路由存在 |
| `/register` | 是 | 405 + `Allow: OPTIONS, POST` | 路由存在 |
| `/setmac` | 是 | 405 + `Allow: OPTIONS, POST` | 路由存在 |
| `/doesNotExist`（对照） | — | 404 | 探测方法有效（404≠405 可区分） |

结论：**当前部署的 `abr.py` 仍暴露客户端编码的全部 6 个 POST 路由**，HTTP 基址 `http://8.140.23.253:5000`、明文端口与资料一致；响应头含 `Access-Control-Allow-Origin: *`（存在 CORS 中间件）。

### 4.2 部署版本与资料不一致（关键）

| 事实 | 值 |
|---|---|
| 资料（`服务器迁移记录.md` §八）描述 | `INSERT IGNORE` + `UNIQUE(dev_id,time)`，**未提任何鉴权** |
| 部署文件时间 | `abr.py` mtime 2026-10-22 19:31:04，比迁移记录（2026-09-26）晚 26 天；体积 7048 → 8949 B |
| 部署日志中的上传请求 | 2026-10-22T19:32:28 同一秒三次 `POST /upload_data`：**401、401、200** |
| 客户端发送的凭证 | 无。`ApiService.uploadData` 仅设 `Content-Type: application/json`；全工程检索 `Authorization|Bearer|@Header` 无命中；`LoginResponse.token` 解析后未被任何代码使用 |
| `/login` 同期表现 | 2026-10-22 17:28–18:12 出现 400/415/500/500，说明当日处于改动与调试窗口 |

同一秒“两次 401 + 一次 200”最合理的解释是**上传路径新增了身份/归属校验，带正确身份的那一次才通过**。但本能力未读 `abr.py` 内容、也没有 401 响应体，**不能**据此断定具体机制（token？手机号已注册？MAC 归属？）。这属于必须由端到端阶段用隔离测试身份定性的项（§9 D1）。

### 4.3 业务活跃度（真实且负面）

| 观察 | 值 | 含义 |
|---|---|---|
| `user_script` journal 起点 | 2026-09-25T22:16:33+08:00（unit 首启），全量 218 行 | 日志保留覆盖整个部署期 |
| 请求行总数 | 26 行（POST 14、GET 11、HEAD 1） | 全部请求可枚举 |
| 最后一次业务 `POST` | 2026-10-22T19:32:28（3 次 upload_data） | 之后 18 天无任何 POST |
| 本次探测之外的最近请求 | 2026-10-22T19:32:28 | 说明 10-22 之后服务零业务流量 |
| EMQX metrics（自 10-24 重启起） | `client.connect=0`、`authentication.success/failure=0`、`messages.*=0`、`bytes.received/sent=0`、`connections.count=0`、`clients list: No clients` | **16 天 7 小时内没有任何 MQTT 客户端连接过** |
| EMQX `retained.count` | 3 | 3 条重启前遗留的保留消息（未读取内容，超出只读范围） |

结论：整条“传感器→网关→App→服务”链路在当前窗口内**没有任何真实运行证据**；本轮无法用历史流量推断上传、幂等或持久化行为，也无法推断 MQTT 认证是否仍按“MAC/ MAC+后缀”工作。

---

## 5. 差异与风险清单（本轮实际观测）

| ID | 观测 | 影响 | 处置责任/阶段 |
|---|---|---|---|
| **F1** | `/upload_data` 在 2026-10-22 出现过 **401**，当前 App 不带任何凭证 | 若该门禁仍在，App 上传将恒失败：客户端把非 2xx 当失败，按 60→1800 s 退避重试 20 次后**行滞留待发箱、不再自动重试** | 端到端阶段定性（§9 D1）；若确认为新增鉴权，属**服务端/客户端契约变更**，需明确授权后才改 |
| **F2** | 部署 `abr.py` 晚于资料 26 天、体积 +1901 B；资料未提及该变更 | 资料已不是当前部署的准确描述；“按资料复用”存在盲区 | 后续任何依赖接口细节的设计/测试必须以运行时证据或授权源码审查为准，不得只引资料 |
| **F3** | `POST /login` 在 10-22 出现 500×2、415×1 | 登录链路当时不稳定；当前无流量无法判断是否已修 | 端到端阶段随注册/登录旅程一并复验 |
| **F4** | HTTP 服务是 `Werkzeug/3.1.8` **开发服务器**，单进程，无生产 WSGI | 并发/长连接能力未测；1000 设备规模下的稳定性无依据（不能据 4 核/3.5 G 推断） | 若端到端压测暴露问题，属**部署形态**变更，需另行授权；本轮不建议改 |
| **F5** | MySQL 镜像为浮动 tag `mysql:latest`，无 healthcheck | 无法从外部确认实际 server 版本是否仍为资料所写 9.0.1；重拉镜像会产生不可预期的升级 | 记录为运维风险；**不要**为此重部署或拉镜像 |
| **F6** | 4 个 MQTT listener `acceptors=16`、`current_conn=0`；`max_conns=infinity` | 容量参数不是当前瓶颈；但也说明从未在现网负载下验证 | 端到端阶段用真实设备/隔离客户端验证即可 |
| **F7** | MySQL 3306 绑定 `0.0.0.0` 且 Docker 直接发布到公网 | 暴露面风险（与资料“仅内部使用”不一致） | 属安全/运维变更，需明确授权；本轮不改防火墙 |
| **F8** | HTTP 明文 5000 + `Access-Control-Allow-Origin: *` | 现状如此（App 已配 `network_security_config` 放行）；不构成本轮改造理由 | 保持现状，不在探索阶段改协议 |
| **F9** | 服务与 caddy/postgres/RustDesk/next-server 共租同一主机（总内存 3.5 G，核数未查） | 资源竞争存在；容量估算不能只看本项目 | 容量结论只作量级参考 |

---

## 6. 本轮需求（readme 八点）对服务端的增量

| 变更点 | 对线上可见差异 | 需后端改动 |
|---|---|---|
| 1 GXHT40、2 光敏分压 | 无（不上空口、不进请求体） | 否 |
| 3 采集 3 分钟 | 请求体不变，只影响节奏 | 否 |
| 4 升温 >0.9℃ 且无光 / >35℃ 才上报 | 筛选在传感器侧；服务端不重算 | 否 |
| 5 复用平台服务器 | 地址/端口/证书不变（本轮已实测端口与路由） | 否（但须过 §9 D1 门禁） |
| 6 Android 同步、退役光通信/霍尔 | `mac` 语义不变 | 否 |
| 7 全暗阈值 1/3 | 不上传光照 | 否 |
| 8 UART1 调试 | 无 | 否 |

**结论**：本轮需求本身不产生端点、字段、表结构或部署增量。真正需要后端关注的是**既有部署已偏离资料**（F1/F2），这不是 readme 要求的新功能。

---

## 7. 已核验 / 仅声明 / 未知 / 不满足

| 项 | 分类 |
|---|---|
| 服务在线、systemd unit 状态、启动时间、NRestarts | **已核验（本轮原始输出）** |
| 8 个端口监听与进程归属、对外 TCP 可达 | **已核验** |
| EMQX 版本 5.8.0、uptime、4 个 listener running | **已核验** |
| 6 个 HTTP 路由存在（405+Allow）、未知路径 404 | **已核验** |
| HTTP 栈为 Werkzeug 开发服务器、Python 3.12.3 | **已核验** |
| MySQL 容器运行、镜像 id/digest、无 healthcheck、浮动 tag | **已核验** |
| 磁盘 68 G 可用、内存 1.9 G 可用、无 swap | **已核验** |
| 部署日志存在 401/500/415 历史（含 upload_data 的 401） | **已核验** |
| 近 18 天无 HTTP 业务请求、近 16 天无 MQTT 客户端 | **已核验** |
| 客户端请求体字段/单位/批上限/待发箱与退避常量 | **已核验（静态源码）** |
| `sensor_data` 表、`UNIQUE(dev_id,time)`、`INSERT IGNORE` | **仅资料声明**（DB 未授权连接） |
| 401 的具体机制（token/归属/注册态） | **未知**（未读源码、无响应体） |
| 上传成功响应体与 `inserted` 计数 | **未知**（未调用写接口） |
| 事务边界、部分成功、MySQL `IGNORE` 全语义 | **未知** |
| MQTT 认证仍为“用户名=MAC、密码=MAC+后缀” | **未知**（无客户端连接、未读配置） |
| 备份/恢复演练、监控告警 | **未知**（未查到相关证据） |
| 实际 server 版本 9.0.1 | **仅资料声明**（浮动 tag，无外部可见版本） |
| 本地桩结果代表生产服务 | **不满足/明确禁止** |
| 上传、幂等、持久化回读的端到端成立 | **未执行（阶段外）** |

---

## 8. 复用 vs 最小改造（更新）

| 方案 | 内容 | 本轮可实施性 | 结论 |
|---|---|---|---|
| A 复用既有服务 | 不改端点/schema/部署，按现有契约上传 | 存活/端口/路由已成立；**上传可用性待 D1** | 仍是首选，但不能宣称“已可用” |
| B 最小改造 | 若 D1 证实新增了鉴权：服务端补“兼容旧客户端”或客户端补 token；另可加 `mac`/采样时间列、冲突上报、批次事务 | 需源码 + 部署 + DB 授权，本轮全部未获准 | 条件性增量，触发条件＝D1 结果 |
| C 重建/替换 | 重写 Flask/重部署/迁库 | 无依据且被禁止 | 否决 |

不因客户端要联调就重做服务或部署；F1 未定性前不写任何服务端代码（也没有可写的源码）。

---

## 9. 端到端阶段必须执行的最小区分实验

> 全部使用**与生产隔离的测试手机号、测试网关 MAC、测试 dev_id、合成温湿度**；不得使用资料中的真实设备凭据与任何真实用户数据。执行前须落实：网络入口（本轮 SSH 只读之外的 HTTP 写授权）、测试身份、写入授权、只读回读入口、清理方式。
> 缺任一项即记 **NOT EXECUTED 及原因**，不得以 HTTP 2xx 替代持久化结论。

| ID | 有界动作 | 原始证据 | 验收依据 |
|---|---|---|---|
| **D1（最高优先）** | 用测试载荷 `POST /upload_data` 一次，**不带**任何凭证；再用 `/login` 得到的 token（若存在）重试一次 | 两次状态码 + 响应体原文（脱敏） | 若第一次 401 且第二次 2xx → 确认存在鉴权，须给出“谁改谁补”的结论；若都 2xx → 401 属历史其他逻辑，记录排除依据 |
| D2 | 相同 `(dev_id,time)` 连续 POST 两次（相同值） | 两次状态码/响应体、回读行、行数 | 第一次 `inserted=1`；第二次 `inserted=0` 且行数不变 |
| D3 | 同键不同值 1 次；批内一项非法 1 次；空 `readings` 1 次 | 状态码/响应体、回读行 | 冲突取舍（first-wins？覆盖？）、原子性、4xx 边界有可复现结论 |
| D4 | 按获准只读入口回读 `(dev_id,time,temperature,humidity)` | 回读行原文 | 与请求逐字段一致（含 `DECIMAL(6,2)` 舍入）；无回读入口 → 持久化子项 NOT EXECUTED |
| D5 | 未知 `phone`/`mac`/`dev_id` 各 1 次（仅合成） | 状态码/响应体 | 归属/鉴权是否存在、是否可任意写 |
| D6 | 一次真实 App 上传（真机）在服务端日志与回读两侧对账 | logcat、服务日志行、回读行 | 2xx 与落库同时成立；顺带确认 F1 在真实客户端上的表现 |

D1 可与 D6 合并执行以减少写入次数；清理只删本实验的测试行，删除动作需单独授权。

---

## 10. 容量与恢复（算术 + 本轮实测资源）

**行数上界**（沿用 as-built：BLE 每 180 s 读一次网关缓存，按接收时刻入库）

```
每设备每日 = 86400/180 = 480 行
1000 设备每日 ≈ 480,000 行；每年 ≈ 175,200,000 行
```

前提：守护运行、BLE 读成功、网关持续返回缓存值。条件上报只减少 433 空帧，**不减少**云端行数。

**体积**：按 InnoDB 经验 66–150 B/行 ⇒ 32–72 MB/日 ⇒ 约 **12–26 GB/年**（1000 设备）。
**本轮实测资源**：`/` 79 G、可用 68 G（10%）、内存 1.9 G 可用、无 swap，且与 caddy/postgres/RustDesk 共租。据此：一年量级可容纳；三年以上或设备数上升需重新评估，并需监控磁盘与内存（本项目服务与其它服务共享主机，不能只看本表）。

**客户端恢复窗口**（静态）：退避 60,120,…,1800 s×15，`maxAttempts=20` ⇒ 约 8.0 h 后停止自动重试，行保留但不重发；服务端无重放入口证据。叠加 F1：
```
若 /upload_data 恒返回 401 → 每次上传 20 次退避内全部失败 → 约 8 h 后全部滞留，云端 0 行
```
这是当前最需要先排除的单点风险。

---

## 11. 给下游的 service_context（以本轮实测更新）

```yaml
service_context:
  mode: reuse_existing_deployment        # 不重建、不重部署；本轮无服务端源码
  verified_at: "2026-11-09T06:24–06:28Z (SSH read-only, paramiko; host key sha256 ty/ysZ6…bJQ)"
  broker:
    uri: ssl://8.140.23.253:8883
    version_verified: "EMQX 5.8.0"
    listeners_verified: {1883: tcp, 8883: ssl, 8083: ws, 8084: wss, 18083: dashboard}
    listeners_running: true
    current_connections: 0
    auth_model: "资料声明 username=MAC / password=MAC+固定后缀；本轮无客户端连接，机制未核验"
    trust_anchor: dengbei_care/app/src/main/assets/jd-ca.crt   # md5 f1a8212e3c690d57894b8a3a5fd901ab，与资料一致
    topics_unchanged: ["/<mac>/a/r0", "/<mac>/a/r1", "/<mac>/a/s0"]
  http:
    base: http://8.140.23.253:5000
    runtime: "Werkzeug/3.1.8 dev server, Python 3.12.3, systemd unit user_script.service, pid 20644"
    code_on_disk: "/root/abr.py size=8949 mtime=2026-10-22T19:31:04+08:00"
    routes_verified_exist: ["/login", "/upload_data", "/sendVerificationCode", "/verifyCode", "/register", "/setmac"]
    route_probe_method: "GET only -> 405 + Allow: OPTIONS, POST; unknown path -> 404"
    upload_request: {phone: string, mac: string, readings: [{devId: "8-hex", time: unix_seconds, temperature: degC, humidity: "%RH"}]}
    upload_auth: "unknown_and_disputed: observed HTTP 401 on 2026-10-22 (2x) then 200; client sends no credential; resolve at E2E-D1"
    client_batch_limit: 50
    client_success_rule: "any 2xx => ack+delete outbox rows"
  storage:
    database: bwusernamebase
    table: sensor_data            # 资料声明，本轮未连接 DB
    declared_unique: "(dev_id, time)"
    declared_write: "INSERT IGNORE"
    mac_persisted: false
    server_version: "资料声明 MySQL 9.0.1；实测镜像 mysql:latest id sha256:d7f7ebf3…00b5（2024-07-22），版本未能从外部确认"
    container: "Up, restart=unless-stopped, no healthcheck, 3306 published on 0.0.0.0"
  host_resources: {disk_total: "79G", disk_avail: "68G", mem_available_mib: 1917, swap: none, co_tenants: [caddy, postgres, rustdesk, next-server]}
  activity_evidence:
    http_requests_total_in_journal: 26      # since 2026-09-25; last business POST 2026-10-22T19:32:28+08:00
    mqtt_clients_since_2026-10-24_restart: 0
    note: "近 18 天无 HTTP 业务流量、近 16 天无 MQTT 连接；不能据历史推断上传/幂等/持久化"
  invariants_to_preserve:
    - 请求字段名/单位/端点不变（IC-002 §6）
    - MQTT topic 与载荷格式不变
    - 不新增未约定的判据/字段
    - 不因联调而重建/重新部署
  must_not:
    - 不在探索/实现阶段写生产数据、执行迁移、直连数据库
    - 不在缺少源码与授权时声称服务端改造完成
    - 不把 2xx 当作持久化通过
  blockers_for_design_signoff: [D1]      # 上传鉴权定性前，不得写“服务端复用已通过”
  required_at_e2e: [D1, D2, D3, D4, D5]
```

**对后端设计的直接含义**

1. 本轮**没有服务端实现任务**可排；后端章节应写成“复用 + 门禁（D1）”，不能写成“已联调/已通过”。
2. D1 未定性前，任何“上传链路可用”的结论都不成立；这是当前唯一会改变结论的未知项。
3. 服务端加固（鉴权/归属/冲突上报/事务/采样时间列）仍是**条件性增量**，触发条件只有 D1–D3 的结果，不是 readme 的直接要求。
4. 不重部署、不重拉镜像、不改防火墙、不直连数据库；F5/F7 记录为运维风险并转相应责任方。

---

## 12. 结论

1. **成立**：既有服务真实存在并在运行；EMQX 5.8.0 与 4 个 listener 正常；`user_script`/`abr.py` 在 5000 端口服务；客户端使用的 6 个 POST 路由全部存在；磁盘/内存余量可容纳一年量级数据；本轮 readme 八点不产生服务端增量。
2. **不成立/已否证**：资料所述“上传无鉴权”与部署日志不符（出现 401）；“近期有真实业务流量”不成立（18 天无 POST、16 天无 MQTT 连接）。
3. **未知（阶段外，已给判别方法）**：上传是否可用、`UNIQUE`+`INSERT IGNORE` 真实语义、事务边界、持久化与回读、MQTT 认证机制。
4. **不需要做**：不需要重做服务、不需要重新部署、不需要为联调改服务端；D1 未定性前不应新增服务端功能。
5. **下一授权动作**：在端到端阶段执行 §9 的 D1（必要时合并 D6），并落实隔离测试身份、写入授权与（若有）只读回读入口；缺条件的子项记未执行及原因。

---

## 附录 A：本次确认的文件身份（SHA-256，本机计算）

检查时点：2026-11-09T06:28Z（06:24–06:28 会话）。摘要只标识本次读取的对象内容，**不证明**远端或板上对象未变化；远端 `abr.py` 等只能给出 `size/mtime`，未取内容摘要。

| 文件 | SHA-256 |
|---|---|
| `readme.txt` | `c6f19954d54c29136db95d75dcc28816d84d5ce57ea449714a0c58ba80a1e6dd` |
| `服务器迁移记录.md` | `e883e4df73b109dd7ac08c862e350ed9b11372a9e073930dd7bcf9e4749811ed` |
| `artifacts/interface_contract.md` | `6af5f0a65c819033390e94099c77ede83af53b6a2750edf55debd9fe42d2ff22` |
| `dengbei_care/app/src/main/assets/jd-ca.crt` | md5 `f1a8212e3c690d57894b8a3a5fd901ab`（与资料一致） |
| `evidence/backend_exploration/upload_contract_stub.py` | `5134c6200ba42fed4ee43adcac2e28c1c7454db0042d3c5ff97bdacef4629a8a` |
| `evidence/backend_exploration/upload_contract_observations.txt` | `8f200994dcc3b338aff1f74cf4ed5b81faa8a046aa5a62c45cd50027a3e153e1`（未改动） |
| `evidence/backend_exploration/upload_contract_observations_rerun.txt` | `2001d7d8d68dc548d21ad7d399aa75d14139e6a6075645ee438b5f1da483d9ad` |
| `evidence/backend_exploration/ssh_tcp_probe.py` | `7ff0137ad5c6c637f77f888345bddf6d8eabd84b9cb081a1bc44fa88ed1cb909` |
| `evidence/backend_exploration/ssh_tcp_probe_output.txt` | `076565b3e2cebb5099728df7b0ad3f228a89faa2f296a105ec5de5a29c0bfb58` |
| `evidence/backend_exploration/ssh_readonly_probe.py` | `a181e5417c5c74dc98c29b40adf6513fd13fa6e4dcf7288b74517566eea6ebd3` |
| `evidence/backend_exploration/ssh_readonly_inspection.txt` | `cea6dd4da0363cd892efa9ba9eb96e12143490df49b88f9e2e39ea5ad38e66be` |
| `evidence/backend_exploration/ssh_readonly_probe_followup.py` | `ff88b7c4ac8bfee853964a1b0c558df257894e93a00ec0e7e1e210db4ef93755` |
| `evidence/backend_exploration/ssh_readonly_inspection_2.txt` | `051055532ef507560244e80231cacf11fb4c7a1866dc335b4e0d27f13001dda3` |
| `evidence/backend_exploration/ssh_readonly_probe_emqx.py` | `5c66bc91028f8e3c1939fe8ab4b9bb20f1b22986ec57abd9ff65500e97b6a051` |
| `evidence/backend_exploration/ssh_readonly_inspection_3.txt` | `13f87c3faa662664c3ecdb227e66d98811abdf8d250821c0e3153b37f3b59b28` |
| `evidence/backend_exploration/ssh_readonly_probe_routes.py` | `cced0b9ac222e19a0ca5e629fe17cf3aac4681d3469031053196080ef3a39dad` |
| `evidence/backend_exploration/http_route_existence.txt` | `6c037a66849fb03d6a7021564e0d3256200dd52d677b4eab202db3d063256a77` |
| `evidence/backend_exploration/ssh_readonly_probe_http_history.py` | `70ce72e2686183a362c69392559c77f462b5c13332c0d5886dd744fab9b2cda3` |
| `evidence/backend_exploration/http_history_counts.txt` | `7192ae78a28913c9fe61f550d526e8a7c07400b178357e7210eda767ca5c2fb9` |
| `evidence/backend_exploration/ssh_readonly_probe_http_requests.py` | `5adc4492c6aa4b4aeebc32f2047ebbc708bc2b6215f3ad0fdc4765bf87234975` |
| `evidence/backend_exploration/http_request_history.txt` | `85e2f7752012b8efb86926adabc1ad2297e0b2ca32321deba56c84e437ae78d6` |

本报告自身不含摘要（无法自引用），其对象身份由本次 Git 提交记录标识。

---

## 附录 B：可复跑性

```
cd D:\mypro\beiwo2\AIP
python evidence/backend_exploration/ssh_tcp_probe.py
python evidence/backend_exploration/ssh_readonly_probe.py
python evidence/backend_exploration/ssh_readonly_probe_followup.py
python evidence/backend_exploration/ssh_readonly_probe_emqx.py
python evidence/backend_exploration/ssh_readonly_probe_routes.py
python evidence/backend_exploration/ssh_readonly_probe_http_history.py
python evidence/backend_exploration/ssh_readonly_probe_http_requests.py
python evidence/backend_exploration/upload_contract_stub.py
```

- 脚本从 `服务器迁移记录.md` 正则读取主机/账号/口令，**不在命令行或代码中硬编码、不打印**；写证据文件前统一脱敏（口令、AccessKey、手机号）。
- 依赖：本机 Anaconda `python`（3.x）+ 已存在的 `paramiko 2.8.1`、`flask`、`requests`；本轮未安装/升级任何依赖、未改系统包。
- 每个 SSH 会话单命令 ≤15 s、整轮 ≤150 s；无后台任务、无遗留进程。
- 桩脚本仍只监听 `127.0.0.1` 临时端口，退出时删除临时 SQLite；**不得**用它替代真实服务结论。

---

## 附录 C：历史记录（保留，不重写）

**BE-EXP-001（2026-11-09 13:19，提交 `06f9c59`）结果 `WORK_FAILED`**：当时环境未登记任何服务端访问入口、`allowed_external_actions` 为空，因此没有任何真实服务响应；契约层结论为“本轮无需改服务端”，并用本地桩固定契约与风险模型（13 请求、538 行、first-wins 等）。

本轮变化：Unified 汇总已登记 `existing_service_ssh_inspection`（enabled，动作 `inspect_existing_service_readonly`，30 s），上一轮“无入口”的原因消失；本次已实际连接并取得原始输出。**上一轮的桩结论与其边界不变，旧证据文件未修改**（摘要见附录 A）。上一轮“迁移记录仅声明、运行期未证”的条目中：在线、版本、端口、路由存在性已由本轮转为**已核验**；`UNIQUE`/`INSERT IGNORE`、鉴权机制、持久化仍为**未核验**，并新增“401 与客户端不一致”这一实测差异。
