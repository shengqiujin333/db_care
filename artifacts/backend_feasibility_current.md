# 后端可行性探索报告（BE-EXP-004 · 当前）

负责角色：后端工程
检查时点：2026-11-10 15:47:50–15:47:57（Asia/Shanghai）＝ 2026-11-10T07:47:50Z–07:47:57Z（UTC）
结论代码：**EXPLORATION_COMPLETE**（本能力在授权范围内的只读核验与隔离样例实验均已执行并取得本轮原始输出）
结论边界：**不是**“上传链路已通过”。`/upload_data` 的受理条件、写入、幂等、持久化与回读仍需端到端阶段以隔离测试身份和明确写授权执行；本报告只固定判别方法与当前证据边界。
被测产品源码：`D:\mypro\beiwo2\AIP`，接手 HEAD `a208c2837343edefd4c1ed844f4b78721aaf307e`；核验开始时 `git status --porcelain` 为空。
唯一环境/授权依据：本轮 Unified 汇总——`existing_service_ssh_inspection`＝enabled，`allowed_external_actions=[inspect_existing_service_readonly]`，30 s 上限；`environment_repair.enabled=true` 但本轮无需修复，未安装/升级任何依赖。

---

## 0. 结论摘要

| 可行性问题 | 本轮结论 | 依据 |
|---|---|---|
| 既有服务是否仍在线、由哪个进程提供 | **已核验成立** | SSH 会话 0.4 s 建连；`user_script` active，pid 20644＝`/root/venv/bin/python /root/abr.py`，监听 0.0.0.0:5000 |
| 是否仍是上次检查的同一部署对象 | **是（未变化）** | `/root/abr.py` size=**8949 B**、mtime/ctime=**2026-10-22 19:31:04.322 +0800**、inode 1088447；与 BE-EXP-003 逐项一致 |
| 服务运行版本与 broker | **已核验成立** | EMQX **5.8.0**，uptime 17 d 9 h 18 m；4 个 listener running、`current_conn=0`；MySQL 容器 `mysql:latest` Up 2 weeks |
| 客户端 6 个 POST 路由是否仍存在 | **已核验成立（6/6）** | GET 全 405 + `Allow: OPTIONS, POST`；`/doesNotExist` 404 作对照 |
| `/upload_data` 预检是否暴露鉴权头要求 | **未暴露** | `OPTIONS /upload_data` → 200，仅 `Allow: OPTIONS, POST`；无 `Access-Control-Allow-Headers`、无 `WWW-Authenticate` |
| 历史 401 的性质 | **仍未知** | 2026-10-22 同一秒 401/401/200，200 那次进入 `mysql.connect()`，三次均来自 127.0.0.1；本轮无写授权、无响应体、不读服务端源码 |
| 2026-10-22 之后是否有真实业务 | **仍为零** | journal 全量 POST 仍为 **14** 条，末次 2026-10-22 19:32:28；自上一轮探测结束（11-09 23:14:10Z）起无任何 POST，本轮新增请求全部是本能力 GET/OPTIONS 探测 |
| 本轮八点需求是否要求服务端增量 | **否**（契约层） | 不新增字段/端点/表结构；光照不上传；上报筛选在传感器侧；地址/端口/证书不变 |
| 3 分钟节拍对后端的量级影响 | **可容纳，但行数上限与“唯一读数数”不同** | 连续守护时 ≤480 行/设备/日；本轮磁盘可用 68 G、内存 available 2010 MiB；网关缓存复读会造成同值不同时间的重复行（§7） |
| 复用结论 | **复用成立；上传受理条件仍待 D1** | 服务、端口、路由、版本、部署对象均已核验；不得写“服务端已通过/已联调” |

判定说明：本轮**重新连接**并对同一部署对象取得新的原始响应；相对 BE-EXP-003 没有出现部署版本变化，也没有出现任何新的真实上传或 MQTT 客户端活动。因此“复用 + D1 门禁”的结论保持，不因客户端进入联调阶段而升级。

---

## 1. 本轮目标、执行与未执行

### 1.1 关键可行性目标（在请求/原型前确定）

| ID | 目标 | 结果 |
|---|---|---|
| Q1 | 既有服务是否在线、运行版本、服务进程与端口 | **成立**（§3） |
| Q2 | 部署对象是否仍是上次检查的 `abr.py`（版本身份） | **成立（size/mtime/ctime/inode 未变，进程未重启）**（§3） |
| Q3 | 客户端契约中的 6 个 HTTP 路径是否存在 | **成立**（§4.1） |
| Q4 | 本轮需求是否产生端点/字段/表结构/部署增量 | **不产生**（§5） |
| Q5 | 事务与幂等假设在隔离环境能否稳定复现 | **桩内成立、真实服务未知**（§6.2） |
| Q6 | 3 分钟节拍与条件上报下的容量/恢复边界 | **给出上界与未证项**（§8、§9） |
| Q7 | 上传写入、真实幂等、持久化与回读 | **未执行**（无写授权/DB 授权；§11 给方法） |
| Q8 | 最近业务活跃度与当前连接 | **仍为零**（§4.3） |

### 1.2 实际执行（有界、只读、可复跑）

1. 读取共享清单、Git 状态/HEAD、`readme.txt`、`服务器迁移记录.md`、`artifacts/system_analysis_current.md`、`artifacts/interface_contract.md`、`artifacts/codegraph_prepare_index.md` 与上一轮后端报告。
2. 定点读取当前客户端契约源码（`CloudConfig.kt`、`ApiService.kt`、`UploadPayload.kt`、`CloudUploadRepository.kt`、`UploadOutbox.kt`、`SensorCadence.kt`、`MqtttService.kt` 相关片段）与网关 GATT/433 记录相关源码片段，用于判断数据模型与时间语义。
3. 本机 → 服务器 7 个端口有界 TCP 探测（仅 connect/close，5 s/端口，实测 2–16 ms）。
4. 通过已登记 SSH 入口执行 **1 个留档会话、21 组只读命令**，单命令 ≤15 s，整轮 **7.2 s**（`ssh_readonly_probe_g003.py`）：身份/时间基准、unit 状态与启动/PID/重启次数、监听端口与进程归属、docker 状态、`abr.py` 元数据、EMQX 版本/uptime/listener/stats/clients、GET/OPTIONS 路由探测、journal 全量与“自上一轮探测结束以来”的请求行、10-22 上传窗口原文、资源余量。
5. 本地隔离桩复跑（`upload_contract_stub.py`，仅 127.0.0.1 临时端口 + SQLite）：13 个 `/upload_data` 请求覆盖负温、35.1℃、50 条批、相同批重试、同键不同值、精度、空集、缺字段、非 JSON、480 行/日量级；输出与 2026-11-09 复跑**逐项一致**（归一化后无差异）。

### 1.3 明确未执行及原因（授权边界）

| 动作 | 未执行原因 |
|---|---|
| `POST /upload_data`、`/login`、`/register`、`/setmac` 等任何写/变更调用 | 授权为 `inspect_existing_service_readonly`，明确“不调用写入接口”；本轮只用 GET/OPTIONS |
| MySQL 连接、`docker exec`、任何 SQL、读取表数据 | 授权明确“不直连数据库”；`sensor_data`/`UNIQUE`/`INSERT IGNORE` 仍为资料声明 |
| 读取 `/root/abr.py` 内容或计算其内容摘要 | 授权限定“服务状态、监听端口、必要的近期服务日志”；只取 `stat` 元数据（size/mtime/ctime/inode），未读源码，故 401 分支无法定位 |
| `sudo`、文件/配置/数据修改、启停重启、部署、迁移、改防火墙 | 授权明确禁止 |
| 使用/回显资料中的口令、AccessKey、设备凭据、手机号 | 脚本从资料读取连接口令仅用于 SSH 认证；证据与报告统一脱敏，无明文 |
| MQTT 客户端连接、TLS 业务握手、手机/串口/烧录/逻辑分析仪 | 不属本能力；对应入口在本能力下 absent/unavailable，且会引入凭据使用与额外汇总 |
| 服务端源码审查/本地构建 | 仓库内没有服务端源码根；未获部署/源码授权 |

---

## 2. 本轮原始证据索引（新产出，不复用旧结论作本轮依据）

| 文件 | 内容 | SHA-256（本机此刻） |
|---|---|---|
| `evidence/backend_exploration/ssh_readonly_inspection_g003.txt` | 本轮留档 SSH 会话完整原始 stdout/stderr/exit、时间戳、主机指纹、每命令耗时（323 行） | `7d4f7e3872a4dac0b5d5c1e020b8f0ff3283e754b110a154a337df16b044f870` |
| `evidence/backend_exploration/ssh_readonly_probe_g003.py` | 产生上述证据的可复跑脚本（读资料取连接信息、脱敏、有限等待、无写动作） | `9525f9f8a200deccde6526a933e9ec769a5e16dad6e4ee09d4e9f26169454fd9` |
| `evidence/backend_exploration/upload_contract_observations_g003.txt` | 本轮本地桩复跑原始输出（含 13 用例逐项请求字节/状态/响应/表行与结论边界） | `429395cbe5d6f396659aa9e3bf2dcf9c437f2d3d85c9a4fa6e06476f0ab1813c` |
| `evidence/backend_exploration/upload_contract_stub.py` | 隔离桩脚本（本轮未修改） | `5134c6200ba42fed4ee43adcac2e28c1c7454db0042d3c5ff97bdacef4629a8a` |

主机指纹：`ssh-ed25519` SHA-256 `ty/ysZ6Z9cvGUtYfQ26082EJXBWFFzU9pYBm2tXPbJQ`，与 BE-EXP-002/BE-EXP-003 会话一致；该指纹只证明本轮与上次连到同一主机，**未**与任何权威清单比对，不构成主机身份认证结论。

---

## 3. 运行期核验（本轮实测值）

| 项 | 实测值 | 证据段 |
|---|---|---|
| 主机 / 内核 / 发行版 | `iZ2zeih0gmrk47og5r4z7sZ` / Linux 6.8.0-136-generic x86_64 / Ubuntu 24.04.4 LTS | `identity` |
| 服务器时间 | UTC `2026-11-10T07:47:51Z`（本地 15:47:51 +08:00） | `identity` |
| `emqx.service` | active/enabled；启动 `2026-10-24 06:29:44 CST`；PID 20845；NRestarts=0 | `systemd_units`/`systemd_detail` |
| `user_script.service` | active/enabled；启动 `2026-10-24 06:29:40 CST`；PID 20644；NRestarts=0 | 同上 |
| EMQX 版本 / uptime | `5.8.0`（dpkg `ii emqx 5.8.0 amd64`）；`17 days, 9 hours, 18 minutes` | `emqx_version`/`emqx_broker` |
| EMQX 监听 | 1883/8883/8083/8084 全 `running: true`、`acceptors: 16`、`current_conn: 0`、`max_conns: infinity` | `emqx_listeners` |
| EMQX 当前负载 | `connections.count=0`、`sessions.count=0`、`subscribers.count=0`、`subscriptions.count=0`、`retained.count=3`；`clients list: No clients` | `emqx_broker_stats`/`emqx_clients` |
| HTTP 服务 | 0.0.0.0:5000，`python` pid 20644（`/root/venv/bin/python /root/abr.py`）；`Server: Werkzeug/3.1.8 Python/3.12.3` | `process_owner`/`http_headers_root` |
| `/root/abr.py` | size=**8949 B**，mtime=ctime=**2026-10-22 19:31:04.322 +0800**，inode=1088447；备份 `abr.py.bak_20260926` 7048 B（2026-09-26 08:57:58） | `abr_metadata` |
| MySQL | 容器 `mysql`，镜像 `mysql:latest`，`Up 2 weeks`；`0.0.0.0:3306->3306/tcp`（docker-proxy）；未执行任何 SQL | `docker_ps`/`process_owner` |
| 资源 | `/` 79 G，已用 7.1 G，可用 **68 G**（10%）；Mem 3499 MiB，available **2010 MiB**；swap 0 | `resources` |
| 对外 TCP | 22/1883/3306/5000/8083/8883/18083 全部 TCP_OPEN（2–16 ms） | `local_tcp_probe` |

结论：既有服务持续运行，且当前部署对象与 BE-EXP-003 检查时**是同一份 `abr.py`**（size/mtime/ctime/inode 与进程启动时间均未变）。上一轮观测到的 401/200 行为对**当前部署**仍有参考价值，但**不等于本轮复测**。

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
- `OPTIONS /upload_data` → **200**，`Allow: OPTIONS, POST`，**无** `Access-Control-Allow-Headers`，**无** `WWW-Authenticate`。
- `OPTIONS /login` → **200**，与 `/upload_data` 相同的预检形状。
- 该结果只说明 CORS 预检不要求自定义头，**不能**证明 POST 不需要凭证或归属校验。

### 4.2 历史 401（保留原结论，本轮无新增机制证据）

`journal_20261022_upload_window` 原文（本轮重新读取，未修改）：

```
19:31:04  systemd: Stopping/Stopped/Started user_script.service
19:31:05  python[647759]:  * Serving Flask app 'abr'  ... Running on 0.0.0.0:5000
19:32:28  python[647759]: "POST /upload_data HTTP/1.1" 401 -
19:32:28  python[647759]: "POST /upload_data HTTP/1.1" 401 -
19:32:28  python[647759]: /root/abr.py:222: DeprecationWarning: 'db' is deprecated, use 'database'
19:32:28    conn = mysql.connect()
19:32:28  python[647759]: "POST /upload_data HTTP/1.1" 200 -
```

可确认事实：401/200 发生在更换 `abr.py` 并重启服务后 84 秒；三次均来自 127.0.0.1（部署者本机自测，不是真实 App）；同一秒同一端点先两次 401、第三次 200；200 那次执行到 `abr.py:222` 的 `mysql.connect()`。**机制仍未知**：可能是请求体/归属校验，也可能是三次请求头不同；本轮未读源码、无响应体、未调用写接口，不能定性。

### 4.3 业务活跃度（仍为零）

| 观察 | 值 |
|---|---|
| journal 请求行总数 | **107**（含历史探测与本轮 15 条只读请求行） |
| **POST 行总数** | **14**（全部为部署/调试期） |
| 末次 POST | **2026-10-22T19:32:28+08:00**（3 次 `/upload_data`：401/401/200） |
| 自上一轮探测结束（2026-11-10 07:14:10 +08:00）以来 | 请求行 15 条，**全部为本轮 GET/OPTIONS 探测**；无 POST |
| 2026-10-22 之后 | 无任何 POST |
| EMQX 自 2026-10-24 06:29 重启起 | `connections/sessions/subscribers=0`，`No clients`，`retained=3`（重启前遗留） |

结论：**没有任何真实“传感器→网关→App→服务”流量**可用于推断上传、幂等或持久化；本轮也不得以历史 200 代替当前写入验证。

---

## 5. 本轮八点需求逐项对服务端的增量与关键假设

| 变更点 | 对线上可见差异 | 服务端增量 | 需要下游保持的假设 |
|---|---|---|---|
| 1 GXHT40 | 无（不上空口字段、不进请求体） | 否 | 温度仍以 ℃、湿度 %RH、Unix 秒上传；负温需可存（`DECIMAL(6,2)` 可表示） |
| 2 光敏分压 | 无（光照不进请求体） | 否 | 服务端不得推断/重算“无光”，也不新增光照列 |
| 3 采集 3 分钟 | 请求体不变，节奏变化 | 否 | App BLE 轮询同为 180 s；云端行数上界 480/设备/日（连续守护） |
| 4 升温 >0.9℃ 且无光 / >35℃ 才上报 | 传感器侧筛选，服务端不重算 | 否 | 云端看不到“未上报样本”，不得据稀疏数据反推连续曲线；上报 ≠ 告警确认 |
| 5 复用 broker/服务器 | 地址/端口/证书不变 | 否（须过 D1） | 不复用旧凭据测试；不重部署、不重拉镜像 |
| 6 Android 同步、退役光通信/霍尔 | `phone/mac/devId/time/temperature/humidity` 语义不变 | 否 | 客户端仍无 `Authorization`/`Bearer`；不改端点/schema |
| 7 当前有光 | 无（设备侧语义） | 否 | 不上传光照，不用云端数据判断现场照度 |
| 8 UART1 调试 | 无 | 否 | 调试串口不进入线上协议 |

**结论**：本轮需求不产生端点、字段、表结构或部署增量；后端没有可排的实现任务。唯一会改变后端结论的未知项仍是上传受理条件（§11 D1），它不是新功能，而是既有部署偏离资料带来的复核门禁。

---

## 6. 契约与隔离样例实验

### 6.1 当前客户端契约（静态源码，本轮重读）

| 项 | 当前值 | 证据/位置 |
|---|---|---|
| HTTP 基址 / MQTT URI | `http://8.140.23.253:5000` / `ssl://8.140.23.253:8883` | `cloud/CloudConfig.kt`（SHA-256 `d7daae2d…6cfb`） |
| 上传端点与头 | `POST /upload_data`；仅 `Content-Type: application/json`；无 `Authorization`/`Bearer`/`@Header` | `ui/zhuce/ApiService.kt`（`7d9fd0d1…b444`） |
| 请求体 | `{"phone","mac","readings":[{"devId","time","temperature","humidity"}]}`；空读数不发请求 | `cloud/UploadPayload.kt`（`c0fff0ed…98a9`） |
| 单位 | `time`=Unix 秒；`temperature`=℃；`humidity`=%RH；解码阶段已除 10，上传不再二次换算 | 同上 |
| 批大小 | 50 条/请求 | `CloudUploadRepository.DEFAULT_BATCH_LIMIT` |
| 成功判据 | **任意 2xx 即整批 ack 并删除待发箱行** | `RetrofitUploadApi.uploadData(...).isSuccessful` |
| 失败恢复 | 指数退避 60→1800 s，最多 20 次（约 8 h）；到上限保留行但不再自动重试 | `CloudUploadRepository.Config` |
| 上传门控 | 守护运行中（`measure_state=="stopping"`）且已注册手机号非空 | `PrefsUploadGate` |
| 本地幂等键 | `(dev_id, time)`；`INSERT OR IGNORE` | `cloud/UploadOutbox.kt`（`63d06a21…dbf7`） |
| 触发点 | 入队后、每轮 BLE 开始、MQTT 连接成功、App 冷启动 | `MqtttService.kt` |
| 时间语义 | MQTT 路径用 payload 首段网关时间；BLE 路径用 **App 接收时刻**（无 timeOverride） | `MqtttService.kt:821-824, 1408-1410` |

### 6.2 隔离桩复跑（只证明桩内语义，不外推）

本轮复跑结果（`upload_contract_observations_g003.txt`）：

- 13 用例：11×2xx、2×4xx；总行数 538，SQLite 文件约 61,440 B（15 页 × 4096 B）。
- 与 2026-11-09 的复跑输出**归一化后逐行一致**（仅时间戳/临时端口不同），说明该桩实验可复现。
- 桩内成立：契约字段/单位/50 条批可被接收；相同 `(dev_id,time)` 重试不增行；同键不同值被 `INSERT OR IGNORE` 静默丢弃；空集/缺字段/非 JSON 的桩内返回码；480 行/日可被该表结构容纳。
- 桩不能证明：真实 `/upload_data` 的状态码/`inserted`、MySQL `INSERT IGNORE` 全语义（含告警降级）、批事务原子性、鉴权/归属校验、持久化与回读、`mac` 是否落库（文档表无 `mac` 列）、`DECIMAL(6,2)` 舍入。

### 6.3 本轮新增的数据模型观察（静态源码，未联调）

网关 GATT `FFE2` 的读回调只把已存缓冲复制给客户端（`Profile/gattprofile.c:547-556`），**不在读取时清零**；缓冲在收到新的 433 记录时由 `app_um2006A.c:210` 更新（连续解码失败 100 次后才写全零帧）。Android BLE 路径对每个非零帧调用 `processTemperatureHumidityData`，时间取 `System.currentTimeMillis()/1000`，**没有按值/按帧去重**。因此：

> 在守护运行期间，若网关在两次 433 上报之间被 App 每 180 s 读取，同一缓存记录会以**不同接收时间**重复入库并重复上传；服务端 `UNIQUE(dev_id,time)` 不能识别这种“同值不同秒”，接口契约 §6 已提示“不同接收秒的同一缓存复读”不可识别。

这是**数据语义与容量假设**，不是服务端缺陷，也不是本能力要修改的 Android/网关实现。下游（Android/系统设计/联调）应决定是否在客户端按“同网关+同值+同缓存代次”去重，或接受重复行并在查询侧处理；在未有该决定前，后端只能按“≤480 行/设备/日”做容量上界，不能把行数等同于独立采样数。

---

## 7. 增量的事务 / 兼容 / 并发 / 恢复 / 资源关键假设

| 类别 | 关键假设 | 当前证据级别 |
|---|---|---|
| **事务** | 服务端以 `UNIQUE(dev_id,time)` + `INSERT IGNORE` 实现逐行幂等；整批不保证原子，也不保证返回逐条结果 | **仅资料声明 + 桩模型**；真实 MySQL 未连接 |
| 事务 | 客户端把任意 2xx 当作整批成功并删除待发箱行；若服务端“部分成功”仍回 2xx，本地不会补发缺失行 | **已核验（静态源码）**；服务端行为未知 |
| 事务 | `mac` 不在文档表结构中，无法按网关审计来源；归属仅靠 `phone`/`dev_id` | **资料声明**；DB 未核验 |
| **兼容** | 请求 JSON 字段名/单位/端点不变；不新增光照字段；负温可用 | **已核验（源码 + 本轮桩）**；真实服务解析未验 |
| 兼容 | `time` 为 App 观测时间（BLE 接收/网关 MQTT 时间），不是传感器采样时刻；同秒多样本会合并、不同秒复读会重复 | **已核验（源码）**；接口契约已声明该限制 |
| 兼容 | `DECIMAL(6,2)` 会将 25.555 舍入为 25.56；负温/35.1 可表示 | **资料声明**；未回读 |
| 兼容 | MQTT topic/载荷格式不变；broker 仍为 8883 + `jd-ca.crt` 信任锚 | **端口/listener 已核验**；TLS/MQTT 业务握手与认证未执行 |
| **并发** | HTTP 由 Werkzeug 3.1.8 开发服务器单进程提供；并发模型、线程数、超时、限流未知 | **仅版本/进程已核验**；无压测、无并发证据 |
| 并发 | 客户端每进程用 Mutex 串行化上传，批 50；多手机可同时对服务端形成突发 | **已核验（源码）**；服务端承载未知 |
| 并发 | MySQL 容器端口发布到 0.0.0.0；最大连接数/连接池未知 | **容器状态已核验**；配置未读 |
| **恢复** | 服务自 2026-10-24 起未重启（NRestarts=0）；MySQL 容器 Up 2 weeks | **已核验** |
| 恢复 | 客户端退避 60→1800 s、最多 20 次（约 8 h）后停止自动重试；App 冷启动/BLE 轮次/MQTT 重连会再次触发 | **已核验（源码）** |
| 恢复 | 服务端无重放入口证据；无备份/恢复演练证据 | **未知** |
| 恢复 | 401 若对真实身份持续成立，待发行将在约 8 h 后滞留、云端长期 0 行 | **条件性风险**；D1 未定性 |
| **资源** | 连续守护 ≤480 行/设备/日；1000 设备 ≈48 万行/日、≈1.752 亿行/年；按 66–150 B/行估算 12–26 GB/年 | **算术 + 资料经验值**；非 InnoDB 实测 |
| 资源 | 磁盘可用 68 G、内存 available 2010 MiB、无 swap，且与 caddy/postgres/RustDesk 等共租 | **本轮实测** |
| 资源 | 条件上报只减少 433 空帧，**不自动减少云端行数**；实际行数取决于 App 是否去重缓存复读（§6.3） | **静态推断**；未联调 |

---

## 8. 容量与恢复（算术 + 本轮实测资源）

```
每设备每日 = 86400/180 = 480 行（连续守护、每次轮询都产生一行）
1000 设备每日 ≈ 480,000 行；每年 ≈ 175,200,000 行
InnoDB 经验 66–150 B/行 ⇒ 32–72 MB/日 ⇒ 约 12–26 GB/年（1000 设备）
```

本轮实测 `/` 可用 68 G、内存 available 2010 MiB、无 swap，且与 caddy/postgres/RustDesk 共租：一年量级可容纳，三年以上或设备数上升需重新评估并监控磁盘/内存。**该上界假设每次 180 s 轮询都产生一行**；若客户端对缓存复读去重，实际行数会低于上界；若不去重，则即使传感器条件上报很稀疏，云端仍可能接近该上界（§6.3）。

客户端恢复窗口（静态源码）：退避 60,120,…,1800 s、`maxAttempts=20` ⇒ 约 8.0 h 后停止自动重试，行保留但不重发；服务端无重放入口证据。若 `/upload_data` 对真实身份恒 401，则约 8 h 后全部滞留、云端 0 行——这是当前最需要先排除的单点风险（D1）。

---

## 9. 已核验 / 仅声明 / 未知 / 不满足

| 项 | 分类 |
|---|---|
| 服务在线、unit 状态、启动时间、PID、NRestarts | **已核验（本轮原始输出）** |
| EMQX 5.8.0、uptime、4 listener running、当前连接 0 | **已核验** |
| 6 条 HTTP 路由存在（405+Allow）、对照 404、OPTIONS 200 | **已核验** |
| HTTP 栈 Werkzeug 3.1.8 / Python 3.12.3；5000 由 abr.py 提供 | **已核验** |
| `/root/abr.py` size/mtime/ctime/inode；MySQL 容器运行、3306 发布 | **已核验（仅元数据）** |
| 2026-10-22 上传窗口 401/401/200、200 进入 mysql.connect | **已核验（日志原文）** |
| 10-22 之后无 POST（约 19 天）、自 10-24 起无 MQTT 客户端 | **已核验** |
| 客户端无 Authorization/Bearer、批上限 50、2xx 即成功、退避 60→1800 s×20、门控条件 | **已核验（静态源码）** |
| 网关读回调不清零、App BLE 路径不去重 → 复读重复行 | **已核验（静态源码，未联调）** |
| `sensor_data` 表、`UNIQUE(dev_id,time)`、`INSERT IGNORE`、`DECIMAL(6,2)` | **仅资料声明**（未连 DB） |
| 401 的具体机制（token / 归属 / 注册态 / 请求体校验） | **未知**（未读源码、无响应体） |
| 上传成功响应体、`inserted` 计数、事务边界、部分成功 | **未知**（未调用写接口） |
| MQTT 认证仍为“用户名=MAC、密码=MAC+后缀” | **未核验（本轮无客户端连接）** |
| MySQL 实际 server 版本 9.0.1 | **仅资料声明**（浮动 tag `mysql:latest`） |
| TLS 证书链/主机名/有效期 | **未核验（本轮未做 TLS 业务握手）**；迁移资料自称当时 `Verify return code: 0` |
| 真实上传/幂等/持久化回读 | **未执行（阶段外）** |
| 本地桩代表生产服务 | **不满足/明确禁止** |

---

## 10. 复用 vs 最小改造

| 方案 | 内容 | 本轮可实施性 | 结论 |
|---|---|---|---|
| A 复用既有服务 | 不改端点/schema/部署，按现有契约上传 | 服务/端口/路由/版本/部署对象已核验；**上传受理条件待 D1** | **首选**，但不得写“已通过” |
| B 最小改造 | 若 D1 证实需要归属/凭证：客户端补对应字段或服务端兼容；另可加 `mac`/采样时间列、冲突上报、批次事务 | 需源码 + 部署 + DB 授权（本轮均无） | **条件性增量**，触发条件＝D1–D3 |
| C 重建/替换 | 重写 Flask/重部署/迁库 | 无依据且被禁止 | 否决 |

不因客户端要联调就重做服务或部署；D1 未定性前不写任何服务端代码（本地也没有服务端源码可改）。本轮未产生任何产品源码改动。

---

## 11. 端到端阶段必须执行的最小区分实验

> 前置条件（缺一即记 NOT EXECUTED 及原因）：隔离测试手机号、测试网关 MAC、测试 dev_id、合成温湿度；HTTP 写授权；只读回读入口；清理策略与授权。**不得**使用资料中的真实设备凭据或真实用户数据。
> 执行时同步记录：请求字节（脱敏）、状态码、响应体、服务端日志行、回读行。

| ID | 有界动作 | 原始证据 | 验收依据 |
|---|---|---|---|
| **D1（最高优先）** | 用**已注册的测试身份**发送 1 条 `/upload_data`（不带任何凭证）；再用**未注册/不匹配归属**的合成身份发送 1 条 | 两次状态码 + 响应体原文（脱敏）+ 服务日志行 | 若已注册 2xx、未注册 401 → 确认是**归属/注册校验**，真实 App 路径可用；若已注册也 401 → 需要凭证，属契约变更，须明确授权后处理；两次都 2xx → 排除鉴权，记录 401 为历史逻辑 |
| D2 | 相同 `(dev_id,time)` 连续 POST 两次（相同值） | 两次状态码/响应体、回读行、行数 | 第一次 `inserted=1`；第二次 `inserted=0` 且行数不变 |
| D3 | 同键不同值 1 次；批内一项非法 1 次；空 `readings` 1 次 | 状态码/响应体、回读行 | 冲突取舍（first-wins/覆盖）、原子性、4xx 边界有可复现结论 |
| D4 | 通过获准只读入口回读 `(dev_id,time,temperature,humidity)` | 回读行原文 | 与请求逐字段一致（含 `DECIMAL(6,2)` 舍入）；无回读入口 → 持久化子项 NOT EXECUTED |
| D5 | 未知 `phone`/`mac`/`dev_id` 各 1 次（仅合成） | 状态码/响应体 | 归属校验是否存在、是否可任意写 |
| D6 | 一次真实 App 上传（真机）与服务端日志、回读两侧对账 | logcat、服务日志行、回读行 | 2xx 与落库同时成立；确认真实客户端在 D1 门禁下的表现；同时观察 BLE 缓存复读是否造成重复行（§6.3） |

D1 可与 D6 合并以减少写入次数；清理只删本实验的测试行，删除动作需单独授权。
**在 D1 完成前，任何“服务端复用已通过/上传可用”的表述都不成立。**

---

## 12. 给下游的 service_context（本轮刷新）

```yaml
service_context:
  mode: reuse_existing_deployment        # 不重建、不重部署；本地无服务端源码
  verified_at: "2026-11-10T07:47:50Z–07:47:57Z (SSH read-only, paramiko 2.8.1; host key sha256 ty/ysZ6…bJQ)"
  service_identity:
    abr_py: {size_bytes: 8949, mtime: "2026-10-22T19:31:04+08:00", ctime: "2026-10-22T19:31:04+08:00", inode: 1088447, unchanged_since: "BE-EXP-002/003"}
    user_script: {state: active, enabled: true, started: "2026-10-24T06:29:40+08:00", pid: 20644, restarts: 0}
    runtime: "Werkzeug/3.1.8, Python 3.12.3, single process (dev server)"
  broker:
    uri: ssl://8.140.23.253:8883
    version_verified: "EMQX 5.8.0"
    uptime_verified: "17 d 9 h 18 m (started 2026-10-24T06:29:44+08:00)"
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
    client_gate: "guard running (measure_state=='stopping') AND registered phone non-empty"
    client_time_semantics: "BLE = app receive unix seconds; MQTT = payload first segment"
  storage:
    database: bwusernamebase
    table: sensor_data            # 资料声明，本轮未连接 DB
    declared_unique: "(dev_id, time)"
    declared_write: "INSERT IGNORE"
    mac_persisted: false
    server_version: "资料声明 MySQL 9.0.1；实测镜像 mysql:latest (Up 2 weeks, 3306 published 0.0.0.0)"
  data_semantics_risks:
    - "同一网关缓存记录可在两次 433 上报之间被每 180 s 读取，App 以接收时间入库/上传 → 同值不同秒重复行，服务端唯一键无法识别"
    - "客户端任意 2xx 即整批删除待发行；服务端部分成功/插入忽略不会反馈到客户端"
    - "mac 不在文档表结构中，无法按网关审计来源"
  host_resources: {disk_total: "79G", disk_avail: "68G", mem_available_mib: 2010, swap: none, co_tenants: [caddy, postgres, rustdesk, next-server]}
  activity_evidence:
    post_lines_in_journal: 14
    last_post: "2026-10-22T19:32:28+08:00"
    post_since_previous_probe: 0
    mqtt_clients_since_2026-10-24_restart: 0
    retained_messages: 3
    note: "无真实业务流量；历史数据不能替代当前写入/幂等/持久化验证"
  invariants_to_preserve:
    - 请求字段名/单位/端点不变（IC-002 §6）
    - MQTT topic 与载荷格式不变
    - 不新增未约定的判据/字段/光照上传
    - 不因联调而重建/重新部署
  must_not:
    - 不在探索/实现阶段写生产数据、执行迁移、直连数据库
    - 不在缺少源码与授权时声称服务端改造完成
    - 不把 2xx 当作持久化通过
  blockers_for_design_signoff: [D1]      # 上传受理条件定性前，不得写“服务端复用已通过”
  required_at_e2e: [D1, D2, D3, D4, D5, D6]
```

**对后端设计的直接含义**

1. 本轮**没有服务端实现任务**可排；后端章节应写成“复用 + 门禁（D1）”，不能写成“已联调/已通过”。
2. D1 未定性前，任何“上传链路可用”的结论都不成立；这是当前唯一会改变结论的未知项。
3. 服务端加固（鉴权/归属/冲突上报/事务/采样时间列）仍是**条件性增量**，触发条件只有 D1–D3 的结果。
4. 不重部署、不重拉镜像、不改防火墙、不直连数据库；F4/F5 类运维风险保留记录并转相应责任方。
5. 3 分钟节拍与条件上报**不要求**服务端配置变更；但云端行数≠独立采样数，容量与报表语义必须按 §6.3/§8 的重复行风险处理。
6. Android 侧“缓存复读去重”不在本能力实现范围；已作为联调/系统设计输入记录，不越权修改。

---

## 13. 结论

1. **成立**：既有服务真实存在且持续运行（同一 `abr.py`、进程未重启）；EMQX 5.8.0 + 4 个 listener 正常；`user_script`/`abr.py` 在 5000 端口服务；客户端 6 个 POST 路由全部存在；磁盘/内存余量可容纳一年量级数据；本轮八点需求不产生服务端增量。
2. **仍未知**：2026-10-22 上传窗口 401/401/200 的机制；上传受理条件、`UNIQUE`+`INSERT IGNORE` 真实语义、事务边界、持久化与回读、MQTT 认证机制、MySQL 实际版本、TLS 业务握手。
3. **新增数据模型观察**：网关缓存读不清零 + App BLE 路径按接收时间去重缺失 ⇒ 云端可能出现同值不同秒重复行；唯一键无法识别。该观察基于静态源码，未联调证实。
4. **不需要做**：不需要重做服务、不需要重新部署、不需要为 3 分钟节拍改服务端；D1 未定性前不应新增服务端功能。
5. **下一授权动作**：在端到端阶段执行 §11 D1（必要时合并 D6），落实隔离测试身份、写入授权与（若有）只读回读入口；缺条件的子项记未执行及原因。

**本报告的完成只表示“探索完成、设计可继续”，不表示上传链路或整体服务通过。**

---

## 附录 A：本轮确认的对象身份（本机 SHA-256）

检查时点：2026-11-10T07:47:57Z。摘要只标识本次读取的**本地对象内容**，不证明远端或板上对象未变化；远端 `abr.py` 仅有 size/mtime/ctime/inode。

| 对象 | SHA-256 |
|---|---|
| `readme.txt` | `d9e18992097814d13fbf1cc4f184de9f9d020dea1752fd889a7254559d33bac4` |
| `服务器迁移记录.md` | `e883e4df73b109dd7ac08c862e350ed9b11372a9e073930dd7bcf9e4749811ed` |
| `artifacts/interface_contract.md` | `6af5f0a65c819033390e94099c77ede83af53b6a2750edf55debd9fe42d2ff22` |
| `dengbei_care/.../cloud/CloudConfig.kt` | `d7daae2db108ec9b8f349ec61a9148bfd762e58d3fdd78e110fd81a7279b6cfb` |
| `dengbei_care/.../cloud/UploadPayload.kt` | `c0fff0ed06404895c52b216d4ce0c31657c2c4a6a509f29d015e6d579a6d98a9` |
| `dengbei_care/.../cloud/CloudUploadRepository.kt` | `c221de8da8d78ecc4b41e341daca69ece754fe8e18fd95920cdb065612b84a1c` |
| `dengbei_care/.../cloud/UploadOutbox.kt` | `63d06a21e4f2c22d9e77580b81fc6056fe523d8e17ddfef6c6246a6da543dbf7` |
| `dengbei_care/.../ui/zhuce/ApiService.kt` | `7d9fd0d15994174f13857fff29db880edd697f156fbfc4bbc4316237aab7b444` |
| `dengbei_care/.../telemetry/SensorCadence.kt` | `fe863f0339bcf3f23c42423561557a62e28ae777adb9a9f9901902b72ff27e68` |
| `dengbei_care/.../MqtttService.kt` | `dd3847ff2fec80289f0e82282140fab77b8f0c78e85e59ff6a94040b934a88ac` |
| `CH592EVT/.../beiwov2/Profile/gattprofile.c` | `1678efa31d36b662b619a791d2ebd5c902e454c2c4f743ac372995d08b269790` |
| `CH592EVT/.../beiwov2/APP/app_um2006A.c` | `b351b44161635c0339b9be1ab3c21213d50a4520660d3c557f41d29ebabba78a` |
| `evidence/backend_exploration/ssh_readonly_probe_g003.py` | `9525f9f8a200deccde6526a933e9ec769a5e16dad6e4ee09d4e9f26169454fd9` |
| `evidence/backend_exploration/ssh_readonly_inspection_g003.txt` | `7d4f7e3872a4dac0b5d5c1e020b8f0ff3283e754b110a154a337df16b044f870` |
| `evidence/backend_exploration/upload_contract_observations_g003.txt` | `429395cbe5d6f396659aa9e3bf2dcf9c437f2d3d85c9a4fa6e06476f0ab1813c` |

本报告自身不含摘要（无法自引用），其对象身份由本次 Git 提交记录标识。

---

## 附录 B：可复跑性

```
cd D:\mypro\beiwo2\AIP
C:/Users/kason/anaconda3/python.exe evidence/backend_exploration/ssh_readonly_probe_g003.py
C:/Users/kason/anaconda3/python.exe evidence/backend_exploration/upload_contract_stub.py > evidence/backend_exploration/upload_contract_observations_g003.txt 2>&1
```

- SSH 脚本从 `服务器迁移记录.md` 正则读取主机/账号/口令，不在命令行或代码中硬编码、不打印；写证据前统一脱敏。
- 单命令 ≤15 s，整轮 ≤150 s；留档会话 7.2 s；无后台任务、无遗留进程。
- 桩只监听 127.0.0.1 临时端口，不访问 8.140.23.253、EMQX 或 MySQL；运行结束删除临时 SQLite 文件。
- 依赖：本机 Anaconda `python 3.10.9` + 已存在 `paramiko 2.8.1`、`flask 2.2.2`、`requests`；本轮未安装/升级任何包。
- 只读边界：GET/OPTIONS + systemctl/ss/docker ps/stat/emqx ctl/journalctl/df/free；无 POST、无 SQL、无 sudo、无启停、无源码读取。
- 代码定位：本会话未挂载 `codegraph_explore` MCP 工具；使用限定目录的原生 Grep/Read。`artifacts/codegraph_prepare_index.md` 索引存在但未在本轮被调用，因此本报告的静态源码结论来自直接读取，不声称图查询结果。

---

## 附录 C：历史记录（保留，不重写）

| 版本 | 文件 | 结论 | 关键内容 |
|---|---|---|---|
| BE-EXP-001（2026-11-09 13:19） | Git 历史 | `WORK_FAILED` | 当时无服务端访问入口、无授权，仅本地桩固定契约与风险模型 |
| BE-EXP-002（2026-11-09 14:24–14:28） | `artifacts/backend_feasibility.md`（history） | `EXPLORATION_COMPLETE` | 首次只读实测在线/版本/端口/路由；发现 401 与部署偏离资料；留 D1–D6 |
| BE-EXP-003（2026-11-10 07:08–07:14） | `artifacts/backend_feasibility_g002.md`（history，原文保留） | `EXPLORATION_COMPLETE` | 同一部署对象未变；OPTIONS 不要求鉴权头；200 已进入 `mysql.connect`；三次均来自 127.0.0.1；客户端无凭证；仍无业务流量 |
| **BE-EXP-004（本轮）** | 本文件 | `EXPLORATION_COMPLETE` | 重新只读核验同一 `abr.py` 未变；21 命令/7.2 s；POST 仍 14 条、末次 10-22；本地桩复跑逐项一致；新增“网关缓存复读→同值不同秒重复行”的数据语义假设 |

历史证据文件与旧结论未改动；本报告是新检查时点的记录，不把旧在线状态当作本轮在线证据（本轮已重新连接）。
