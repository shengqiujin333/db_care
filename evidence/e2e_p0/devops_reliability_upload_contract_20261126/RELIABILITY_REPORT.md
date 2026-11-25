# 上传受理修复后可靠性观测与监控方案（MonitoringArtifact，第四轮）

- 负责角色：运维工程
- 能力：可靠性运维
- 观测时间：2026-11-25T21:22Z ~ 21:32Z（CST 2026-11-26 05:22 ~ 05:32），变更后 **7 小时 23 分 ~ 7 小时 33 分**
- 变更基准：CST 2026-11-25 21:59:02，`/root/abr.py` 由 8,949 B / sha256 `534dd40a…5063`（缺陷基线，带 `X-Upload-Token` 门禁）
  替换为 8,688 B / sha256 `a062b05f…017c`（批准候选，门禁删除 4 行），`user_script.service` 同刻重启 PID 214002 → 312368
- 上游依据：`evidence/e2e_p0/devops_platform_upload_contract_20261125/DEPLOYMENT_EVIDENCE.md`（部署结果，candidate）；
  `artifacts/software_test_report_cert8883.md`（原 401 失败证据）；`artifacts/software_failure_analysis_cert8883_upload401.md`（归因）
- 观测对象：既有平台 `8.140.23.253`（EMQX 5.8.0 `ssl:default` 0.0.0.0:8883；Flask `user_script.service` 0.0.0.0:5000）；
  Android App `com.jinyuni.dengbei_care` vC10 / v1.9（APK 未变，`lastUpdateTime` 2026-11-23 21:25:13）；Redmi 8A `12f046690506`
- 本轮性质：**只读观测**。未重启、未部署、未改配置、未登录 Dashboard/DB、未向任何写接口发请求、未操作手机界面、未安装/清数据

---

## 1. 结论摘要

| 维度 | 判定 | 依据 |
|---|---|---|
| 上传服务在线运行的是批准候选 | **PASS** | `sha256=a062b05f…017c`、8,688 B、mtime 21:59:02；`X-Upload-Token`/`UPLOAD_TOKEN` 计数 0；`200:def upload_data():` |
| 服务稳定性（变更后 7h27m+） | **PASS** | `user_script` active、`NRestarts=0`、启动 21:59:02 后未重启；`-p err` 无条目；Traceback/5xx 计数 **0** |
| 8883 TLS 传输可用性（App 信任锚 + IP 主机名校验） | **PASS** | 平台内 10/10 握手成功（109 ms）；外网 12/12（中位 130.8 ms）；负例 `8.140.23.254` 被拒；证书 serial/SAN 无漂移 |
| 401 症状是否消失（可比指标） | **PASS（症状消失）** | 21:59:03 起 `POST /upload_data` 共 19 次，状态分布 `{400: 19}`，401 = **0**、5xx = **0** |
| 安卓上传是否恢复 | **未证实（0 次真实尝试 / 0 次 2xx）** | 19 次 POST 全部来自 `127.0.0.1`（部署自检）与测试主机 `27.128.89.184`（脚本探针），载荷校验即 400；最后一次 22:17:15，其后 **7h09m 无任何上传请求**；App 侧 0 次上传尝试 |
| 上传写库分支是否被执行过 | **否（0 次）** | 变更后日志无 `mysql.connect`/MySQLdb 行；19 次请求均在 `phone` 校验处返回，`sensor_data` 写入路径从未运行 |
| 客户端 MQTT 会话 | **未达成** | 手机端 27 次 `无权连接 (5)`（最后 05:20:30），平台 `not_authorized` 492（128 秒增量 0）；身份 `0200000000A1` 仍被拒 |
| App 是否会自动重试上传 | **不会（设计如此，形成恢复依赖）** | `upload_last_status=401`、`upload_blocked_reason=AUTH_REJECTED`、`upload_blocked_at`=16:38:29 CST；7h09m 内 0 次 `CloudUpload` 日志、0 次平台请求 |
| 手机本地采集是否持续 | **NO（新发现，跨组件）** | `temperature` 表最后一行 CST 21:58:41（崩溃前 1m43s），此后 7h33m 无新增；同库 `alarm_events` 最后写入 05:26:30，说明读数仍在产生但**未落本地测量表**；该表本身为“会话式”写入（当日下午已有 17:00–21:57 约 5 小时空档），故只判“重启后未恢复写入”，不判“崩溃导致” |
| 待发箱（未上传数据）量 | 9 条（16:38 时 5 条） | `pending_uploads` 9 行、`attempts` 合计 100、最后一行时间 21:58:41 |
| 回退点 | **PASS（未演练）** | 证书备份 `leaf.crt.bak` = `a91de074…`；上传备份 `abr.py.bak_20261125T135902Z` 8,949 B / `534dd40a…`；`rollback_remote.sh` 在位 |
| 监控/告警覆盖 | **GAP（未改善）** | 无指标采集、无告警、无上传计数器、无变更事件登记、无 App 崩溃上报；本轮 3 项异常均只能事后人工发现 |
| 公网暴露面（本轮新增风险） | **RISK** | 5000 对公网开放且 `/upload_data` 已无认证；本轮 6h 内观测到互联网扫描（`/etc/passwd` 等路径穿越、`/mcp`、`/sse`、`/sdk`、nmap 指纹），全部 404，未触达写路径 |

**一句话**：上传受理门在线上确已移除且服务稳定运行 7.5 小时，原 401 症状归零；但**“安卓上传已恢复”没有被任何运行证据支持**——真实客户端一次都没重试、`2xx` 为 0、写库分支从未执行；同时手机侧自 21:58:41 起停止写入新测量行、待发箱仅 9 条旧数据，App 又不会自动恢复上传。因此只能表述为“原缺陷症状消失、恢复未证实、且当前存在新的客户端采集停滞”。

---

## 2. 观测窗口与身份

| 项 | 值 | 来源 |
|---|---|---|
| 平台主机 | `iZ2zeih0gmrk47og5r4z7sZ`（8.140.23.253），uptime 2,965,403 s（约 34.3 天） | `19_r4_platform_window.txt` |
| 平台时间 | 2026-11-25T21:22:48Z / 2026-11-26 05:22:48 +0800 | 同上 |
| EMQX | 5.8.0，`emqx@127.0.0.1`，`ActiveEnterTimestamp` 2026-11-25 10:58:15，uptime 18h24m33s，`NRestarts=0` | 同上 |
| 上传服务 | `user_script.service`，`MainPID=312368`，启动 2026-11-25 21:59:02，`NRestarts=0`，`Restart=on-failure`（`RestartUSec=100ms`，`StartLimitBurst=5/10s`） | `19_`、`22_r4_endpoint_and_ops_gaps.txt` |
| 8883 叶子 | `CN=8.140.23.253`，serial `77ADCFD924BA0F87F845C81934D4A3F346D4BB83`，SAN `IP:8.140.23.253, IP:117.72.84.210`，有效期至 2028-11-24T02:55:16Z，文件 mtime 10:58:05 | `19_` |
| App | `com.jinyuni.dengbei_care` vC10 / v1.9，`lastUpdateTime` 2026-11-23 21:25:13，PID 32061（已运行 7h25m） | `20_r4_phone_window.txt` |
| 手机 | Redmi 8A `olivelite`，Android 10 / API 29，serial `12f046690506`，默认网络 101 | 同上 |
| 客户端身份 | `mac_addr=0200000000A1`（测试专用合成 MAC）、`registerphone=14000000001`（模拟测试账号）、`measure_state=starting` | 同上 |
| 观测方式 | SSH 只读（systemctl show / emqx ctl / openssl s_client / ss / curl GET / journalctl / stat / sha256sum）；adb 只读（logcat -d、dumpsys、run-as cat） | 全部脚本位于本目录 |

---

## 3. 变更前后可比证据

### 3.1 上传受理结果（核心可比指标）

| 时间窗 | `POST /upload_data` 总数 | 401 | 400（载荷校验） | 2xx | 5xx |
|---|---|---|---|---|---|
| 16:00 ~ 21:59:02（旧版本，含测试实测） | 9 | **6** | 3 | 0 | 0 |
| **21:59:03 ~ 05:27（新版本，7h28m）** | **19** | **0** | **19** | **0** | **0** |

- 19 次来源：`127.0.0.1` 10 次（部署自检写后核验，21:59:03 / 21:59:20 / 22:15:56 / 22:16:24 各成组），`27.128.89.184` 9 次（测试主机脚本探针，22:00:23 / 22:00:32 / 22:00:41 / 22:04:38 / 22:04:47 / 22:04:56 / 22:16:57 / 22:17:06 / 22:17:15）。
- **最后一次上传请求为 22:17:15，其后 7h09m 无任何 `POST /upload_data`**；本日（11-26）全天计数为空。
- 状态分布证据：`23_r4_upload_caller_matrix.txt`（`upload_requests=19`、`distinct_callers=['127.0.0.1','27.128.89.184']`、`status_counts={'400': 19}`、`upload_2xx_count=0`、`upload_5xx_count=0`）。
- **判读**：401 症状已消失（同一接口、同一身份链路的请求从“401 拒绝”前进到“载荷校验 400”），这是可比改善；但 **2xx 仍为 0，且没有任何一次请求来自真实客户端**，因此不能判“上传恢复”。

### 3.2 写库路径是否被执行

| 判据 | 实测 | 判读 |
|---|---|---|
| 变更后 `mysql.connect` / `MySQLdb` 日志行 | **0** | 无任何请求进入数据库写入路径 |
| Flask Traceback / 5xx | **0** | 无异常栈、无服务端错误 |
| 3306 监听 | 2 个监听套接字（数据库由其他方式托管） | 与基线一致，未连接、未写入 |

**风险（必须记录）**：部署证据只证明“门禁移除 + 请求体能进入载荷校验”。数据库写入分支在本次变更后**从未运行过**，因此若该分支存在独立缺陷（连接、事务、字段），当前所有健康证据都发现不了，必须由真实客户端上传才能暴露。

### 3.3 TLS / 传输层（变更后 7.5 小时）

| 项 | 实测 | 判读 |
|---|---|---|
| 平台内握手（`-verify_ip 8.140.23.253` + App 信任锚） | **10/10**，总耗时 109 ms | 稳定 |
| 外网握手（同一信任锚，测试主机发起） | **12/12**，中位 130.8 ms（min 100.0 / max 226.5），TLSv1.3 | 稳定 |
| 负例（错误主机名 `8.140.23.254`） | `CERTIFICATE_VERIFY_FAILED` | 判据未被放宽 |
| 8883 listener 计数器（128 秒配对） | `not_authorized 492 → 492`、`ssl_error 18 → 18`、`ssl_closed 56 → 56`，`established=0` | 真实 TLS 失败增量 0 |
| `ssl_error` 长期值 | 18（自 22:37 至今 6h45m **未增长**） | 无新增 TLS 失败 |
| 客户端 TLS 错误 | **0**（03:07–05:25 缓冲，1,574 条 MqttService 日志） | 原 `No subjectAltNames` 未复现 |
| 平台内/外网握手对本机计数器的影响 | `ssl_closed` 51→56、`einval` 44→49（本能力 12 次外网握手与负例） | 已在归因中扣除 |

口径说明：`shutdown_count` 随进程重启清零；EMQX 自 10:58:15 未重启，因此本轮 18 与 22:37 的 18 可直接比较（同进程、同窗口）。

### 3.4 认证层（未变化，仍为阻塞）

| 项 | 实测 |
|---|---|
| 手机重连次数（缓冲区间） | 28 次 `Attempting to reconnect to broker` |
| `CONNACK` 结果 | **27 次 `无权连接 (5)`**，最后一次 05:20:30 |
| 平台侧拒绝原文 | `tag: AUTHN, clientid: android_client, msg: authentication_failure, peername: 27.128.89.184:…, username: 0200000000A1, reason: not_authorized`（05:15:32 / 05:20:32 / 05:25:32 等，按 5 分钟节流） |
| `not_authorized` 计数器 | 492（22:30 为 330，6h52m 增 162，约 0.39 次/分） |
| 估计缺口 | 计数器增量高于手机可见重连次数（约 0.2 次/分），差值来源未知（日志节流 + 其他占用同一 clientId 的连接），仅作口径提示 |

判读：MQTT 会话仍未建立，属认证身份缺口，与本轮上传受理变更、与 8883 证书变更均无因果关系；它使“平台 MQTT 通道”仍不可用。

### 3.5 服务与主机稳定性

| 项 | 实测 | 与基线 |
|---|---|---|
| `user_script` | active / running，`NRestarts=0`，内存 60.3 MiB，启动后未重启 | 正常 |
| `emqx` | active / running，`NRestarts=0`，内存 265.7 MiB，uptime 18h24m | 正常 |
| 系统错误日志 | `journalctl -u user_script -p err` 自 21:59:03 起 **无条目** | 正常 |
| 主机负载 / 内存 | load 0.00，1,497 / 3,499 MiB，root 分区 10%，inode 4% | 无容量压力 |
| 端口 | 1883/8883/8083/8084/18083/5000/3306 均监听（另有 22/53/80/443/2019/3000/4370/5370/5432/21115-21119 等其他服务，未变更） | 与基线一致 |
| HTTP 语义 | `/login` GET 405、`/upload_data` GET 405、`/upload_data` OPTIONS 200、Dashboard `/status` 200 | 与基线一致 |

---

## 4. 症状 / 影响 / 证实原因

| 层次 | 内容 |
|---|---|
| 原症状（已消除） | App 上传时平台 `POST /upload_data` 返回 **HTTP 401 `{"error":"unauthorized"}`**（16:21:31、16:38:31 实测），请求在写库前被拒 |
| 原证实原因（已处置） | 处理器在请求体校验前要求请求头 `X-Upload-Token`；App 按已批准契约不发送该头。实现侧 21:59:02 删除该 4 行并重启服务（部署证据已核验） |
| 现症状（本轮实测） | 平台侧 401 = 0；真实客户端 **0 次上传尝试**；无 2xx；本地待发 9 条未清 |
| 现影响 | “安卓→平台上传并回读”仍不可验证：平台无受理行 → 无 `sensor_data` 可回读；MQTT 会话亦未建立（另一缺口） |
| 现原因（证实部分） | ① App 在 16:38:31 收到 401 后按设计置 `upload_blocked_reason=AUTH_REJECTED` 并**全局暂停自动上传**，无自动恢复（7h09m 无重试）→ 变更后没有请求到达平台；② 手机本地测量表自 21:58:41 起不再新增（该表为会话式写入，当日下午已有 5 小时空档），故当前也没有新的待发数据可触发上传 |
| 现原因（未证实部分） | 平台写库分支是否可用**未知**——0 次请求进入 `mysql.connect`，无 2xx、无回读。不得以“401 消失”推断写库可用 |
| 分离结论 | “部署完成”≠“上传恢复”。当前只能声明：门禁已移除、服务稳定、症状 401 归零；不能声明上传链路健康 |

### 4.1 手机侧三项独立运行事实（跨组件，非本轮平台变更引入）

**F1 — 上传不自动恢复**：prefs `upload_last_status=401`、`upload_blocked_reason=AUTH_REJECTED`、`upload_blocked_at`=2026-11-25 16:38:29 CST、`upload_pending_count=5`（App 自记）与本地库实存 9 条不一致（App 计数未随新增行更新），7h09m 内 `CloudUpload` 日志 0 行、平台 0 次请求。任何“修复后应自动恢复”的假设不成立，恢复必须依赖显式“重试上传”动作或新的门控逻辑。

**F2 — 本地测量落库自重启后未恢复（新）**：

| 项 | 实测 |
|---|---|
| `temperature` 表 | 124 行，最早 2026-11-24 12:55:24 CST，**最后 21:58:41 CST**，此后 7h33m 无新增 |
| 按小时分布 | 11-24 14:00–18:00 共 88 行；11-25 15:00 4 行、16:00 9 行、21:00 1 行（**17:00–21:57 亦为空档**） |
| `pending_uploads` 表 | 9 行（16:21:29–16:52:53 共 8 行 18.0 ℃/35.8 %，`attempts` 各 20/0；1 行 21:58:41 21.7 ℃/37.1 %），`attempts` 合计 100 |
| `alarm_events` 表 | 515 行，**最后写入 05:26:30 CST**（约每 7–8 分钟一条，与 BLE `DATA_OK` 节拍一致），内容 `threshold / 温湿度超限报警` |
| 崩溃时点 | 22:00:24 CST `FATAL EXCEPTION`（导航图缺陷，见 §4.2），进程重启为 32061 |
| prefs 状态 | `measure_state=starting`（自重启后未进入 running） |
| 同期 BLE | 03:07–05:26 共 18 次 `DATA_OK`、19 次 `CONNECT_FAILED`（约 3 分钟周期，成功率约 49%） |

**判读**：读数仍在产生并触发告警（告警表持续写入、BLE 有 `DATA_OK` 与温湿度值），但**测量表与待发箱自 21:58:41 起不再新增**，且 `measure_state` 停在 `starting`。该表本身是“会话式”写入（同一天 17:00–21:57 已有约 5 小时空档，且 16:21–16:53 测试期间集中写入 9 条），因此本轮只能确认“**崩溃重启后测量写入没有恢复**”，不能据此断定崩溃是唯一原因，也不能排除与 App 启动流程/生命周期有关。结论的可靠性含义是明确的：**当前没有新的待发读数可被上传**，端到端“持续采集→上传”在手机上尚未成立。根因需实现侧定位，本能力不修改产品代码，也不判定其可修复性。

**F3 — 网关 BLE 间歇失败**：约 3 分钟一轮的 `CONNECTING → DATA_OK / CONNECT_FAILED`（18 成功 / 19 失败，7.5 小时窗口），失败类型为 16,000 ms 超时与 `status=133`。与平台变更无因果，属网关/射频侧观察，交测试与实现复现。

### 4.2 App 崩溃（沿用第三轮证据并复核）

- 22:00:24.731 `FATAL EXCEPTION: main` — `java.lang.IllegalArgumentException: Navigation action/destination …:id/action_homeFragment_to_deviceDetailFragment cannot be found from the current destination Destination(…:id/deviceDetailFragment)`，栈在 `HomeFragment.kt:69` / `DeviceCardAdapter.kt:47`。
- 本轮 `logcat -b crash` 复核：仍为该条 22 行栈，之后 **无新的崩溃记录**（应用 7h25m 连续运行）。
- 影响：进程被 SIG 9 后重启，采集守护中断约 30 秒；本轮证据显示其后续影响延续到“测量落库未恢复”（F2），需实现侧确认二者关系。

---

## 5. 监控方案（建议项，未改动任何 SLO）

本轮再次确认**主机无任何指标采集或告警设施**：运行服务清单中无 exporter/prometheus/telegraf/filebeat/zabbix/grafana/alertmanager；无 `/etc/prometheus`、`/etc/telegraf`、`/opt/prometheus`；`cluster.hocon` 无 prometheus 集成；`crontab -l` 为空、无自定义 timer；`emqx ctl alarms` 在该 5.8.0 构建不可用（仅打印用法）；`journald` 149 MB、EMQX 仅保留 `emqx.log.1`（74 KB，notice 级别）。

| # | 观测项 | 建议阈值/判据 | 检查方式 | 本轮基线 |
|---|---|---|---|---|
| M1 | 8883 真实 TLS 失败率 | 10 分钟内 `ssl_error` 增量 ≥ 10 → 告警（排除已知探测） | `emqx ctl listeners` 增量 + 日志 | 6h45m 增量 0（累计 18，含历史与扫描） |
| M2 | 8883 握手抽样 | 20 次抽样 < 20 成功 → 严重 | `ssl_probe`（本目录 `r4_external_tls.py` 可复用） | 外网 12/12、平台内 10/10 |
| M3 | 证书身份漂移 | serial ≠ `77ADCFD9…B83` 或 SAN 缺 `8.140.23.253` → 严重 | `openssl x509 -serial -ext subjectAltName` | 一致 |
| M4 | 证书到期 | 剩余 < 60 天 → 告警 | `x509 -checkend` | 约 728 天 |
| M5 | 服务存活/重启 | 任一单元非 active 或 `NRestarts` 增加 → 严重 | `systemctl show` | 两单元 `NRestarts=0` |
| M6 | 认证拒绝率 | 身份补齐后 > 5 次/分持续 5 分钟 → 告警 | `shutdown_count.not_authorized` 增量 + AUTHN 日志 | 0.39 次/分（身份缺口，属预期噪声） |
| M7 | 端口/HTTP 回归 | 七端口任一不可达；`/login` ≠ 405；`/status` ≠ 200 | 端口探测 + `curl GET` | 全部一致 |
| M8 | 上传受理结果分布（**本轮强化**） | 真实客户端连续 10 分钟 `2xx=0` → 告警；`401/5xx` 突增 → 告警 | 访问日志按 `POST /upload_data` 状态码聚合 | 19 次全 400、2xx=0 |
| M9 | **上传“静默”检测（新增）** | 预期有采集的设备 30 分钟内无 `POST /upload_data` → 告警；无 `mysql.connect` 成功记录 → 提示写库路径未验证 | 访问日志 + `mysql.connect` 计数 | 22:17:15 后 7h09m 静默，无人被告知 |
| M10 | App 崩溃/ANR | 任一 `FATAL EXCEPTION` → 告警 | `logcat -b crash` / 崩溃上报 | 22:00:24 一次，无上报 |
| M11 | 变更事件登记 | 单元重启 + 文件哈希变化即记事件（时间、sha256、操作者） | `systemctl show` + `sha256sum` 定时抓取 | 21:59:02 变更仍只能人工发现 |
| M12 | **本地采集持续率（新增）** | `temperature` 表 15 分钟无新增但 `alarm_events` 有写入 → 告警（采集与落库不一致） | 本地库快照对比 | 7h33m 无新增，告警表正常 |
| M13 | **公网写接口暴露/滥用（新增）** | 5000 出现非白名单来源访问或写入量异常 → 告警 | Nginx/防火墙日志或 Flask 访问日志按来源聚合 | 6h 内扫描流量已存在（全部 404） |
| M14 | 磁盘/日志保留 | 根分区 > 80%、inode > 80% → 告警；EMQX 日志轮转大小确认 | `df`、日志配置 | 10% / 4% |

**已识别的可观测性缺口（不作为通过项）**
1. 无指标/告警平台：本轮 3 个异常（上传静默 7h09m、测量落库停滞 7h33m、BLE 约 49% 失败）全部靠人工观测发现。
2. `shutdown_count` 随重启清零；EMQX 重启即丢故障历史。
3. EMQX 日志 `notice` 级别 + 1 分钟事件节流（本轮出现 `log_events_throttled_during_last_period, dropped: authentication_failure => 1`），逐次失败无法还原。
4. Flask 访问日志不含客户端 User-Agent / 请求体 / 响应体，无法把请求绑定到具体 App 实例或业务载荷。
5. 无变更事件记录：`abr.py` 21:59:02 的替换只能通过文件 mtime 与备份名推断。
6. `emqx ctl alarms` 不可用；无 Prometheus 端点。
7. **安全类缺口**：`user_script.service` 单元文件以 `Environment=` 明文保存云访问凭据（`ALIBABA_CLOUD_ACCESS_KEY_ID/SECRET`），本轮已从证据文件中脱敏，不回显具体值；建议轮换并改用受保护配置（如 `EnvironmentFile` + 600 权限），改动需另行授权。

---

## 6. 回退条件与回退后预期

### 6.1 回退点复核（只读）

| 对象 | 路径 | 实测 |
|---|---|---|
| 上传服务 | `/root/abr.py.bak_20261125T135902Z` | 8,949 B、`600 root:root`、mtime 2026-10-22 19:31:04、sha256 `534dd40aac40c00597b06710069cbcc70533650e01318e0637cd924851740c63`（= 缺陷基线） |
| 回退脚本 | `/root/rd_upload_contract_20261125T135855Z/rollback_remote.sh` | `rwxr-xr-x`，1,641 B，2026-11-25 21:58 |
| 核验脚本 | 同目录 `verify_upload_contract.sh` | `rwxr-xr-x`，3,103 B |
| 摘要清单 | 同目录 `BACKUP_SHA256SUMS.txt` | 首 16 位 `534dd40aac40c005`，与备份一致 |
| 8883 证书（独立变更） | `/root/emqx-8883-cert-backup-20261125T025805Z` | `700 root:root`，`leaf.crt.bak` 前缀 `a91de074d8ca0954`（= 变更前基线） |
| 演练状态 | — | **NOT_DRILLED**（演练需中断 5000 端口，无该窗口授权；脚本在后置核验失败时会自动回退） |

### 6.2 触发回退的条件（满足任一，需按授权执行）

1. 变更后出现 **5xx 或 Traceback**，或 `mysql.connect` 报错并影响既有接口；
2. 上传接口出现**无法归因的错误分类**且影响面大于原缺陷（如其他客户端、其他接口连带失败）；
3. 出现**未授权写入证据**（非白名单来源成功写入 `sensor_data`），需立即止损；
4. 服务进入重启循环（`NRestarts` 增长 / `StartLimitBurst=5/10s` 触发失败锁定）。

### 6.3 回退后预期（必须如实披露）

- 回退即恢复 `X-Upload-Token` 门禁 → App 上传**再次 401**，回到**已知失败状态**；不得记为“恢复健康”，也不得用回退后的状态证明任何用例通过。
- 回退**不能**修复 F1（App 暂停态）、F2（本地落库停滞）、认证缺口（`CONNACK 5`）或 BLE 间歇失败。
- 因此当前不建议回退：没有证据表明该变更引入了新的失败面（无 5xx、无 Traceback、无回归；401 已归零）；风险点集中在“未验证的写库分支”和“无认证暴露”，需要验证与授权决策，而不是回滚。

---

## 7. 影响范围

| 对象 | 影响 |
|---|---|
| Android App（当前 APK vC10/1.9） | 401 症状消失；但不会自动重试，需显式“重试上传”；本地 9 条待发；本地测量落库自 21:58:41 停滞（F2） |
| 平台上传接口 `/upload_data` | 已无认证门禁；5000 对公网开放；写库分支未被执行过，可用性未知 |
| 平台 8883 / EMAQ 证书消费者 | 未变更：本轮 12/12 外网 + 10/10 平台内握手通过，负例仍拒；无回归 |
| 其他端口/服务 | 未变更；1883/8083/8084/18083/5000/3306 与基线一致 |
| MySQL / `sensor_data` | 本轮未连接、未写入；因 0 次 2xx，可判定**没有新增业务行** |
| 监控与发现能力 | 见 §5 缺口；本次修复后 7.5 小时内的所有异常均无人被告知 |
| 安全 | `/upload_data` 无认证 + 公网可达；互联网扫描已在 5000 端口出现（全部 404，未触达写路径） |

---

## 8. 未执行项与边界

1. **真实 App 上传复测**：NOT EXECUTED（属软件测试能力；本轮只读观测未点击“重试上传”，避免以运维动作代替测试证据）。
2. **`sensor_data` 回读与幂等验证**：NOT EXECUTED（无授权只读入口；且 0 次 2xx 无对象）。
3. **写库分支运行验证**：NOT EXECUTED（需真实客户端上传）。
4. **回退演练**：NOT DRILLED（需中断 5000，未获授权）。
5. **限流/来源白名单/正式鉴权**：未实施（需另行授权与产品决策）。
6. **F2 根因定位**：未定位（不在本能力；只交付可复现事实与证据）。
7. **EMQX 管理 API / Dashboard 指标**：未登录（无认证凭据）；用 CLI 与文件日志替代。
8. 本轮未修改代码、配置、证书、服务或手机应用；未清数据、未安装、未重启。

---

## 9. 复跑命令

```bash
# 平台窗口（服务、listener 增量、证书、握手、上传受理、回退点、可观测性）
C:/Users/kason/anaconda3/python.exe evidence/e2e_p0/devops_reliability_upload_contract_20261126/r4_platform_probe.py
# 外网 8883 TLS 抽样（App 信任锚 + IP SAN 主机名校验 + 负例）
C:/Users/kason/anaconda3/python.exe evidence/e2e_p0/devops_reliability_upload_contract_20261126/r4_external_tls.py
# 端点来源/状态矩阵与运维缺口（重启策略、扫描噪声、日志保留）
C:/Users/kason/anaconda3/python.exe evidence/e2e_p0/devops_reliability_upload_contract_20261126/r4_upload_caller_matrix.py
C:/Users/kason/anaconda3/python.exe evidence/e2e_p0/devops_reliability_upload_contract_20261126/r4_endpoint_and_ops_gaps.py
# 手机只读窗口（进程、prefs、崩溃缓冲、日志分类）
C:/Users/kason/anaconda3/python.exe evidence/e2e_p0/devops_reliability_upload_contract_20261126/r4_phone_probe.py
# 本地库快照分析（先 adb exec-out run-as cat 到 24_r4_local_db_snapshot.db）
C:/Users/kason/anaconda3/python.exe evidence/e2e_p0/devops_reliability_upload_contract_20261126/r4_local_snapshot_analysis.py
```

预期：`abr_sha_is_approved_candidate=YES`、`upload_2xx_count=0`、`handshake_rate=10/10`、`negative_control=rejected`、`measurements_continue_after_crash=NO`。

---

## 10. 证据清单（本目录）

| 文件 | 内容 |
|---|---|
| `19_r4_platform_window.txt` | 平台只读窗口：单元状态/NRestarts/启动时间、主机资源、EMQX 状态、listener 计数器与 128 秒配对增量、证书身份与文件摘要、平台内 10 次握手、AUTHN 原文、上传服务文件身份、上传状态计数与逐次请求、业务端点分布、错误扫描、端口与 HTTP 语义、两处回退点、可观测性现状 |
| `20_r4_phone_window.txt` | 手机只读窗口：设备/APK、进程运行时长、网络、prefs 上传与身份状态、崩溃缓冲、应用日志尾部（BLE 周期、CONNACK）、分类计数 |
| `21_r4_external_tls.txt` | 外网 12 次握手（App 信任锚 + IP SAN 校验）、时延、叶子摘要、错误主机名负例 |
| `22_r4_endpoint_and_ops_gaps.txt` | 来源/状态聚合、公网扫描原文、`Restart` 策略与单元文件（凭据已脱敏）、EMQX 告警节流原文、日志轮转、定时任务、磁盘、备份保留 |
| `23_r4_upload_caller_matrix.txt` | `POST /upload_data` 逐次来源+状态、首末请求时间、状态汇总（19 次全 400、2xx=0、5xx=0） |
| `24_r4_local_db_snapshot.db` / `-journal` | 手机本地库只读快照（`run-as cat` 复制，未写回设备） |
| `25_r4_local_pending_rows.txt` | `pending_uploads` 逐行内容（设备、传感器时间、温湿度、重试次数） |
| `26_r4_phone_collection_gap.txt` | 快照分析：温度表按小时计数、待发行明细、告警表最新写入、落库停滞机器判据 |
| `r4_platform_probe.py`、`r4_external_tls.py`、`r4_phone_probe.py`、`r4_endpoint_and_ops_gaps.py`、`r4_upload_caller_matrix.py`、`r4_local_snapshot_analysis.py` | 本轮只读、限时、脱敏探针（不重启、不部署、不写接口、不连 DB、不操作手机界面） |

---

## 11. 交接与后续建议

**给软件测试（本轮必须覆盖的判定边界）**
1. 平台侧 401 已归零属**可比改善**，不等于上传恢复；必须用同一 APK/同一手机执行真实上传并取得 **2xx** 与回读，才能声明通过。
2. 注意 F2：App 当前不会产生新的待发读数（`temperature` 表 21:58:41 后 7h33m 无新增、`measure_state=starting`，而告警表每 7–8 分钟仍在写）。若仅以“重试上传”9 条旧数据判定通过，只能证明“存量补传”而非“持续采集上传”；建议分列两种情形并注明依据。
3. 注意 F1：App 不会自动恢复上传，需显式触发；若测试只观察等待，会得到“0 次尝试”而非失败证据。
4. 平台写库分支从未执行（0 次 `mysql.connect`），首次真实上传可能暴露独立缺陷；若出现非 2xx，请保留时间、状态码、响应体与平台日志窗口。

**给实现（本轮新发现，交回定位）**
1. F2：崩溃重启后 `measure_state=starting` 且测量落库未恢复（7h33m 无新行），而告警写入继续（最后 05:26:30）→ 请确认采集持久化与告警路径的状态机差异；注意该表为会话式写入（当日下午另有 5 小时空档），不能只归因崩溃。
2. F1：401 分类为 `AUTH_REJECTED` 后进入全局暂停且无退避自动恢复，导致平台修复后链路长期静默 → 请评估是否需要可配置的自动恢复策略。
3. 22:00:24 导航图崩溃（沿用第三轮证据，未新增实例）。
4. 单元文件明文云凭据（已脱敏记录）→ 建议轮换与改造，需授权。

**需用户/授权决策**
1. `/upload_data` 无认证 + 5000 公网可达，且已观测到扫描流量；最小缓解（安全组/来源白名单/限流）需要授权后才能实施。
2. `sensor_data` 只读回读入口与数据库直连仍无授权；上传落库证明依赖该入口或测试侧获准渠道。
3. MQTT 认证身份补齐（`mac_addr` 非占位、EMQX 内置库放行）仍需产品/测试身份条件。

**不声明生产健康**：本轮为 7.5 小时限时只读观测，无指标平台、无长期趋势、无数据库回读；上传成功率为 0 成功 / 0 真实尝试（不可测量），恢复结论未成立。
