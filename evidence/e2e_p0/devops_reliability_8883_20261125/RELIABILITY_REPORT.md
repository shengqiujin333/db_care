# 8883 证书变更后可靠性观测与监控方案（MonitoringArtifact）

- 负责角色：运维工程
- 能力：可靠性运维
- 观测时间（UTC）：首轮 2026-11-25T03:27Z ~ 03:42Z（CST 11:27 ~ 11:42）；续接轮 2026-11-25T06:32Z ~ 06:39Z（CST 14:32 ~ 14:39）；第三轮 2026-11-25T14:30Z ~ 14:47Z（CST 22:30 ~ 22:47）。三轮对象、判据、工具一致，用于区分“短暂连上”与“持续可用”
- 续接原因：首轮结束时 MQTT 会话仍被身份占位阻塞，可靠性结论只覆盖 44 分钟；续接轮补做约 2 小时 50 分钟之后的复观测；第三轮是在软件测试实测 `POST /upload_data` 401 失败、实现侧于 CST 21:59:02 重新部署上传服务之后，旧 Runtime 退出的重试观测，覆盖变更后约 11.5 小时并单独核对上传受理结果与回退点
- 变更时间基准：EMQX 于 CST 2026-11-25 10:58:15（02:58:15Z）重启并加载新证书；两轮观测分别覆盖变更后约 27–44 分钟与 3 小时 34–41 分钟
- 被测对象：既有平台 `8.140.23.253`（EMQX 5.8.0，`ssl:default` 0.0.0.0:8883），Android App `com.jinyuni.dengbei_care` vC10/v1.9，手机 Redmi 8A `12f046690506`
- 上游依据：`evidence/e2e_p0/devops_8883_cert_20261125/DEPLOYMENT_EVIDENCE.md`（部署结果）、`CHANGE_RECORD.md`（变更与回退点）、`evidence/e2e_p0/P0_platform_stage_report.md`（原缺陷现象）
- 本轮性质：**只读观测**。未重启、未改配置、未部署、未登录 Dashboard/DB、未写入业务数据、未安装/清理手机应用

---

## 1. 结论摘要

| 维度 | 判定 |
|---|---|
| 8883 TLS 传输可用性（App 信任锚 + 主机名校验） | **PASS**：首轮 20/20 次握手成功（105–133 ms），负例 `8.140.23.254` 被拒；续接轮 10/10 与 5/5 再次成功；第三轮平台内 10/10、外网 12/12（中位 89 ms） |
| 客户端 TLS 阻断是否解除（同一 App、同一手机） | **PASS**：修复前 18 次 `No subjectAltNames` 事件（10:31–10:56）；修复后 0 次，覆盖 11:01–22:30 的可观测重连（三轮累计 64 次）且全部到达 MQTT CONNACK |
| 客户端 MQTT 会话是否建立 | **未达成**：三轮均为 `CONNACK rc=5 无权连接`，第三轮 username 已从占位 `000000000000` 变为测试身份 `0200000000A1`，仍 `reason=not_authorized` |
| 安卓上传 `/upload_data` 是否恢复 | **未证实（0 次成功）**：软件测试 16:21/16:38 实测 401；实现侧 21:59:02 重新部署后 401 不再出现，但此后**没有任何真实客户端上传**（19 次均为空/短载荷探测，返回 400），访问日志中 2xx = 0。不得据此判恢复 |
| 非 MQTT 路径与其他 listener 回归 | **PASS**：三轮均 1883/8883/8083/8084/18083/5000/3306 OPEN；Flask `/login` 与 `/upload_data` GET 405、Dashboard `/status` 200；手机 BLE 22:30 仍 `DATA_OK` |
| 服务稳定性 | **PASS（有新事件）**：EMQX 三轮均 active、`NRestarts=0`、变更后 11h31m 未重启、负载 0.00、根分区 10%/inode 4%；Flask 于 21:59:02 由实现侧按变更窗口重启 1 次。第三轮观测到 App 22:00:24 崩溃，进程约 30 秒后重启、22:30 恢复采集（见 §12.4） |
| 回滚点 | **PASS**：证书备份目录 `700 root:root`，`leaf.crt.bak` 摘要 = 基线 `a91de074…0855f9`；上传服务另有 `abr.py.bak_20261125T135902Z`（8949 B，sha `534dd40a…`）与 `rollback_remote.sh` |
| 监控/告警覆盖 | **GAP**：仍未发现指标采集或告警配置；本轮新增识别：上传受理结果无计数器、App 崩溃无上报、重部署无变更事件记录 |

**一句话**：证书修复在传输层已被平台与客户端两侧独立证据证实有效，并在三轮、覆盖变更后约 11.5 小时的观测中持续成立；上传 401 的受理门已在 21:59:02 由实现侧移除（此后 401 归零），但**没有真实客户端上传被观测到**，成功率仍为 0/0，故不能声明“安卓上传已恢复”；App 仍因平台认证拒绝（`CONNACK 5`）无法建立 MQTT 会话，该阻塞与证书无关。

---

## 2. 观测窗口与身份

| 项 | 值 | 来源 |
|---|---|---|
| 平台主机 | `iZ2zeih0gmrk47og5r4z7sZ`（8.140.23.253） | `01_platform_reliability_window.txt` |
| EMQX | 5.8.0，`emqx@127.0.0.1`，`ActiveEnterTimestamp=2026-11-25 10:58:15 CST` | 同上 |
| 8883 叶子 | `CN=8.140.23.253`，serial `77ADCFD9…B83`，SAN `IP:8.140.23.253, IP:117.72.84.210`，有效至 2028-11-24 | 同上 |
| App 信任锚 | `dengbei_care/app/src/main/assets/jd-ca.crt` 证书指纹 `75:33:09:B3:…:15:2B`（与用例基线一致），文件未修改（`git status` 无变更） | 本轮计算 + 用例基线 |
| App | `com.jinyuni.dengbei_care` vC10 / vN1.9，安装于 2026-11-23 21:25，PID 32200 连续运行 | `05_phone_app_window.txt` |
| 手机 | Redmi 8A `olivelite`，Android 10 / API 29，serial `12f046690506` | 同上 |
| 客户端连接目标 | `ssl://8.140.23.253:8883`（App 日志与代码一致） | 同上 |
| 平台侧客户端标识 | clientId `android_client`（`MqtttService.kt:117` 常量），username `000000000000`（`mac_addr` 默认值） | 平台 `emqx.log.1` + App 源码 |

---

## 3. 变更前后可比证据

### 3.1 TLS 层

| 指标 | 变更前 | 变更后 | 判读 |
|---|---|---|---|
| 手机端 TLS 主机名错误事件 | 18 次（10:31:02–10:56:45，`No subjectAltNames on the certificate match`） | **0 次**（11:01–11:36:45，24 次重连） | 原症状消失 |
| 平台 8883 `ssl_error` | 累计 2019（10 天 4 小时 uptime，均值约 8.4 次/小时，含公网扫描流量） | 1 次（仅本轮故意错误主机名负例） | 真实客户端 TLS 失败为 0 |
| 平台 broker 日志 TLS 失败行 | 日志级别 warning，逐次握手失败不入日志 | `grep -c 'ssl_error\|handshake_failure\|unknown_ca'` = **0** | 无新增 TLS 异常 |
| 主机侧握手抽样 | —（旧证书 `verify_ip 8.140.23.253` 失败） | **20/20 成功**，中位 114.6 ms，`-verify_ip` 通过，负例拒绝 | 传输层稳定可用 |

> 口径说明：平台计数器在 10:58 随进程重启清零，不能直接与变更前累计值相减；上表“变更前”取自变更前基线快照 `00_pre_change_baseline.txt`（restart 前记录），变更后取自本轮。客户端侧 10:31–10:56 / 11:01–11:36 为同一日志缓冲内的时间切分，口径一致。
> 局限：`journalctl -u emqx` 在变更前 24 小时只有 1 行（EMQX 只写文件日志），因此**无法**用平台日志做逐条小时级前后对比；上述判断依赖客户端日志切分 + listener 计数器 + 主动握手抽样三路证据。
> 续接轮补充（见第 11 节）：14:32–14:38 的 `ssl_error` 出现一次 +5 的突发（4→9），随后 120 秒无本地流量与 5 次正确握手期间增量均为 0；同一时段手机侧 TLS 错误为 0。因此该突发**不与 App、也不与本能力探测相关**，只能记为来源不明的外部 8883 连接失败（公网扫描类），不足以判为新证书回归。

### 3.2 MQTT / 认证层（修复后新增现象）

| 观测 | 结果 | 证据 |
|---|---|---|
| 手机重连次数（11:01–11:36:45） | 24 次，均 `Attempting to reconnect to broker` | `05_phone_app_window.txt` |
| 手机端 TLS 异常 | 0 | 同上 |
| 手机端结果 | 24 次 `Failed to connect to broker` / `无权连接 (5)`，无 TLS 异常栈 | 同上 |
| 平台侧认证拒绝 | `clientid: android_client, username: 000000000000, reason: not_authorized`，11:01:03 / 11:06 / 11:11 / 11:16 / 11:21 / 11:26（日志按 1 分钟节流，实际次数多于日志行） | `02_auth_rejection_rate.txt` |
| 8883 listener 计数器（变更后 33 分钟） | `not_authorized 38`、`ssl_error 1`（本能力负例）、`ssl_closed 11`、`einval 7`（探测主动断开） | `01_`、`03_` 证据 |
| 认证后端 | `built_in_database` 启用（bootstrap CSV），`mysql` 后端 `enable=false` | `03_alarms_and_log_retention.txt` |

**归因边界**：`username=000000000000` 来自 App `setupSSL()` 读取 `MyPrefs/mac_addr` 的默认值；P0 手机段已记录 `registerphone` 空、`mac_addr=000000000000`。因此这是**既有的身份就绪缺口**，与证书变更无因果关系；本轮只确认它在 TLS 解除后成为新的首要阻塞点。平台计数器 38 与手机端 24 次不要求逐条相等：日志有 1 分钟节流，且计数包含重启后至 11:01 前的尝试与本能力探测连接。

**客户端身份关联说明**：平台日志中的 `android_client` 与 App 源码常量一致，username 与 App 默认 `mac_addr` 一致，重连节奏与手机日志时间戳吻合；但来源地址 `27.128.89.184` 是测试 LAN 的共用出口（本能力的主机探测亦来自该地址），因此这是强相关而非密码学绑定。本轮结论同时依赖平台与手机两侧独立日志，不以单一来源定论。

---

## 4. 症状 / 影响 / 证实原因

| 层次 | 内容 |
|---|---|
| 原症状 | App 连接 `ssl://8.140.23.253:8883` 时 `SSLHandshakeException: No subjectAltNames on the certificate match` |
| 原影响 | App→EMQX 的 TLS 不成立，MQTT CONNECT 未发出；P1 上传链路前置缺失 |
| 证实原因（已消除） | 旧叶子证书 SAN 仅含 `IP:117.72.84.210`，与客户端实际连接地址不一致；Android Conscrypt 在链可信后仍执行主机名校验 |
| 现症状 | 24 次重连均在 1 秒内收到 `CONNACK rc=5`，客户端提示“无权连接” |
| 现影响 | MQTT 会话未建立 → 上传仍无法执行；但 TLS 层不再阻断 |
| 现原因（未消除） | App 的平台身份为占位值 `000000000000`，built_in_database 认证拒绝；属测试前置身份未配置，不是代码/证书回归 |

**区分“暂时连上”与“稳定可用”**：两轮观测中没有任何客户端会话建立成功，故**不声明“已恢复连接”**；可声明的是“TLS 传输在变更后约 3.5 小时、两个独立窗口内持续可用（25 次主机侧抽样全部成功、57 次真实客户端握手全部通过主机名校验）”。二者不可混写。

---

## 5. 监控方案（建议项，未改动任何 SLO）

本轮**未发现**主机上有 EMQX 指标采集或告警配置（运行服务清单无 exporter/采集器；EMQX 管理 API 需认证，本轮未登录）。以下为最小可执行监控项，均已给出可复跑的只读检查方式：

| # | 观测项 | 建议阈值/判据 | 检查方式 | 当前基线 |
|---|---|---|---|---|
| M1 | 8883 TLS 握手失败率 | 10 分钟内真实失败 ≥ 10 次 → 告警（排除已知探测） | `emqx ctl listeners` 的 `shutdown_count.ssl_error` 增量 | 两轮合计真实失败 0；14:32 出现一次无归因的 +5 突发（静默/正确握手期间均 0 增量） |
| M2 | 8883 握手抽样成功率 | 20 次抽样 < 20 成功 → 严重 | `connectivity_sample.py` | 20/20 |
| M3 | 证书身份漂移 | 服务端 serial ≠ `77ADCFD9…B83` 或 SAN 缺 `8.140.23.253` → 严重 | `openssl s_client` + `x509 -serial -ext subjectAltName` | 一致 |
| M4 | 证书到期 | 剩余 < 60 天 → 告警（当前至 2028-11-24） | `x509 -checkend` | 约 730 天 |
| M5 | listener/服务存活 | `ssl:default running=false` 或 `NRestarts` 增加 → 严重 | `emqx ctl listeners`、`systemctl show emqx` | 正常 |
| M6 | 认证拒绝率 | 身份补齐后：> 5 次/分钟持续 5 分钟 → 告警（凭据/扫描）；持续为 0 且客户端在线 → 检查客户端连接 | `shutdown_count.not_authorized` 增量 + `emqx.log` `tag: AUTHN` | 当前约 1.1 次/分，**属预期噪声**（身份未配置） |
| M7 | 端口/HTTP 回归 | 1883/8883/8083/8084/18083/5000/3306 任一不可达；Flask `/login` 非 405、Dashboard `/status` 非 200 | 端口探测 + `curl GET` | 与基线一致 |
| M8 | 上传接口可用性 | `/upload_data` 5xx 比例、连续失败 | Flask 访问日志（需日志入口；本轮仅 GET 基线） | 未纳入 |

**已识别的可观测性缺口（不作为通过项）**：
1. 无指标采集/告警：变更前 `ssl_error` 已累计 2019 次（约 8.4 次/小时，持续多日）仍未触发任何告警，说明缺陷可长期不被告知。
2. `shutdown_count` 等 listener 计数器随进程重启清零，重启即丢失故障历史；建议外部按固定间隔抓取增量留存。
3. EMQX 文件日志级别为 `warning`，逐次 TLS 失败不入日志；`emqx ctl alarms` 在该 5.8.0 构建中不可用（只打印命令用法）。
4. `journalctl` 对 emqx 仅有启停行，排障依赖 `/var/log/emqx/emqx.log.*`。
5. 证书到期无自动检查；建议在 M4 落地前保留人工月度核对。

---

## 6. 回退条件与回退后预期

回退点（已复核）：`/root/emqx-8883-cert-backup-20261125T025805Z/`，`700 root:root`；`leaf.crt.bak` sha256 `a91de074…0855f9`（= 变更前基线），`leaf.key.bak` 与 `SHA256SUMS.txt` 齐全（`06_rollback_point.txt`）。

**触发回退的条件（满足任一）**
1. 出现可归因于新证书的既有消费端 TLS 握手失败激增（M1 超阈值，且失败集中在真实客户端而非扫描）；
2. 其他 8883 消费端报告链/信任/主机名错误，尤其是以域名而非 IP 连接者；
3. 8883 listener 无法启动，或 EMQX 出现反复重启 / `NRestarts` 增长；
4. 线上证书 serial/SAN 与批准候选发生漂移。

**回退后预期（必须如实披露）**：回到 SAN 仅含 `IP:117.72.84.210` 的旧证书，App 将**重新出现** `No subjectAltNames`，即回到**已知失败**状态。因此回退只用于止损“新引入的更大范围影响”，不得记为“恢复健康”，也不得用回退后的状态证明任何用例通过。

**当前判断**：新证书同时保留 `IP:117.72.84.210` 的 SAN 且 CA 未变，已知消费端（App 按 IP 连接）不因换证失效；暂不需要回退。残余风险是按域名连接的未知消费端（清单未提供）。

---

## 7. 影响范围

| 对象 | 影响 |
|---|---|
| Android App（当前 APK） | TLS 阻断解除；无需更换信任锚（`jd-ca.crt` 未变、链校验通过）；仍受身份缺失阻塞 |
| 其他 8883 客户端（按 IP `117.72.84.210` 或 `8.140.23.253`） | 两个 IP 均在 SAN 内，链根未变，不受影响 |
| 按域名连接 8883 的客户端（若有） | 新证书未含 DNS SAN，可能校验失败；清单未知，列为残余风险 |
| 1883/8083/8084/18083/5000/3306 | 未改动；端口与 HTTP 基线与变更前一致 |
| MySQL / Flask 业务 | 本轮未连接、未写入；上传落库未验证（NOT EXECUTED） |
| 监控/告警 | 见第 5 节缺口；不影响本次修复有效性，但影响后续发现速度 |

---

## 8. 未执行项与边界

1. **安卓→平台上传与落库**：NOT EXECUTED。需先补齐测试身份（`mac_addr` 非占位、`registerphone` 非空）并由测试能力执行；本轮不判 PASS，也不把它算作证书缺陷。
2. **客户端 MQTT 会话/授权结果**：NOT ACHIEVED（`CONNACK 5`）；按测试设计属认证层维度记录，与 TLS 维度分开。
3. **EMQX 管理 API / Dashboard 指标**：未登录，未取连接数曲线、消息速率；仅用 CLI 与日志替代。
4. **数据库回读**：无授权，未执行。
5. **变更前平台日志逐条对比**：不可得（见 3.1 局限）。
6. 本轮未修改代码、证书、配置、系统服务；未安装/清理手机应用；未采集业务数据。

---

## 9. 复跑命令

```bash
# 平台只读窗口与握手抽样（含前后窗口切分）
C:/Users/kason/anaconda3/python.exe evidence/e2e_p0/devops_reliability_8883_20261125/reliability_probe.py
# 认证拒绝率与 broker 日志（只读，不发 CONNECT）
C:/Users/kason/anaconda3/python.exe evidence/e2e_p0/devops_reliability_8883_20261125/auth_rejection_probe.py
# 告警/日志保留/计数器（只读）
C:/Users/kason/anaconda3/python.exe evidence/e2e_p0/devops_reliability_8883_20261125/alarms_logretention_probe.py
# 主机侧 20 次 TLS 抽样（App 信任锚 + 主机名校验）
C:/Users/kason/anaconda3/python.exe evidence/e2e_p0/devops_reliability_8883_20261125/connectivity_sample.py
# 手机侧只读日志切分（不改动 App）
C:/Users/kason/anaconda3/python.exe evidence/e2e_p0/devops_reliability_8883_20261125/phone_window_probe.py
# 回退点复核（只读）
C:/Users/kason/anaconda3/python.exe evidence/e2e_p0/devops_reliability_8883_20261125/rollback_point_check.py
# 续接轮：平台窗口 / ssl_error 归因 / 非 8883 回归 / 手机窗口（只读）
C:/Users/kason/anaconda3/python.exe evidence/e2e_p0/devops_reliability_8883_20261125/resume_check.py
C:/Users/kason/anaconda3/python.exe evidence/e2e_p0/devops_reliability_8883_20261125/ssl_error_attribution.py
C:/Users/kason/anaconda3/python.exe evidence/e2e_p0/devops_reliability_8883_20261125/resume_regression_probe.py
C:/Users/kason/anaconda3/python.exe evidence/e2e_p0/devops_reliability_8883_20261125/resume_phone_probe.py
# 第三轮（22:30 之后）：平台窗口 / 外网握手 / 变更事实 / 上传受理统计 / 手机窗口（只读）
C:/Users/kason/anaconda3/python.exe evidence/e2e_p0/devops_reliability_8883_20261125/resume3_check.py
C:/Users/kason/anaconda3/python.exe evidence/e2e_p0/devops_reliability_8883_20261125/resume3_external_tls.py
C:/Users/kason/anaconda3/python.exe evidence/e2e_p0/devops_reliability_8883_20261125/resume3_change_check.py
C:/Users/kason/anaconda3/python.exe evidence/e2e_p0/devops_reliability_8883_20261125/resume3_upload_outcomes.py
C:/Users/kason/anaconda3/python.exe evidence/e2e_p0/devops_reliability_8883_20261125/resume3_phone_probe.py
```

预期：TLS 抽样 `20/20`、`negative_control=OK`；手机 `post_fix_tls_error_events=0`；回退点 `baseline_cert_in_backup=YES`。

---

## 10. 证据清单（本目录）

| 文件 | 内容 |
|---|---|
| `01_platform_reliability_window.txt` | 服务/主机状态、listener 计数器、会话、握手抽样、变更前后日志窗口计数 |
| `02_auth_rejection_rate.txt` | 75 秒 8883 建连采样、认证拒绝计数增量、`emqx.log` 认证失败原文、认证后端配置 |
| `03_alarms_and_log_retention.txt` | listener 完整块、`emqx ctl alarms` 可用性、日志级别/轮转、运行服务清单 |
| `04_connectivity_sample.txt` | 20 次 TLS 握手（含时延）与错误主机名负例 |
| `05_phone_app_window.txt` | 手机身份/APK 版本/进程、App 日志缓冲按 11:00 切分与分类计数 |
| `06_rollback_point.txt` | 备份目录、文件摘要与 `SHA256SUMS.txt` |
| `reliability_probe.py` 等 6 个脚本 | 只读、限时、可复跑；连接信息取自项目资料且不回显 |
| `07_resume_window.txt` | 续接轮平台窗口：服务/uptime/NRestarts、listener 计数器与 90 秒配对增量、证书 serial/SAN/mtime、10 次握手、AUTHN 原文、回退点摘要 |
| `08_ssl_error_attribution.txt` | `ssl_error` 归因对照：120 秒纯静默、60 秒套接字采样、5 次正确握手的增量分离 |
| `09_resume_phone_window.txt` | 续接轮手机窗口：13:39–14:34 的 829 条日志分类、33 次重连、0 TLS 错误、BLE 网关连接失败事件 |
| `10_service_regression_resume.txt` | 续接轮非 8883 回归：单元状态、监听端口、Flask/Dashboard HTTP 码、Flask 变更后错误行数、磁盘 inode |
| `resume_check.py`、`ssl_error_attribution.py`、`resume_regression_probe.py`、`resume_phone_probe.py` | 续接轮只读探针（同样是限时、脱敏、不改动线上） |
| `11_resume3_platform_window.txt` | 第三轮平台窗口：单元状态/uptime、listener 计数器与 128 秒配对增量、证书身份与文件摘要、平台内 10 次握手、AUTHN 原文、上传服务进程与文件变更、端口/HTTP 基线、证书回退点 |
| `12_resume3_phone_window.txt` | 第三轮手机窗口：设备/APK 版本、前台进程、`MyPrefs` 上传状态、21:54~22:30 应用日志分类（TLS 错误、CONNACK、401 提及、崩溃计数） |
| `13_resume3_external_tls.txt` | 外网 12 次 TLS 握手（App 信任锚 + IP SAN 主机名校验）、时延与叶子证书摘要 |
| `14_resume3_app_crash.txt` | 22:00:24 `FATAL EXCEPTION` 完整栈与系统侧 `Force finishing` 记录原文 |
| `15_resume3_upload_service_change.txt` | 上传服务变更事实：`abr.py` 大小/摘要/mtime、变更前副本、单元时间线、启动日志、`/root` 近期变更 |
| `16_resume3_deploy_context.txt` | 实现角色 21:59:02 重部署目录清单与 `deploy_v2.log` 尾部（仅作窗口上下文） |
| `17_resume3_upload_gate_apply_log.txt` | 上传门补丁 `apply.log` 全文（pre/post 校验、401→400 行为、备份摘要、自述待复测项） |
| `18_resume3_upload_outcomes.txt` | 16:00~22:45 `POST /upload_data` 逐次状态与来源统计（401/400/2xx） |
| `resume3_check.py`、`resume3_phone_probe.py`、`resume3_external_tls.py`、`resume3_change_check.py`、`resume3_deploy_context.py`、`resume3_apply_log.py`、`resume3_upload_outcomes.py` | 第三轮只读探针（限时、脱敏、不写入、不重启、不连 DB） |

---

## 11. 续接观测（CST 14:32 ~ 14:39，变更后 3h34m ~ 3h41m）

上一次执行在 11:42 收尾后中断，本轮按同一对象、同一判据重新观测，用于确认“持续可用”而非“短暂连上”。

### 11.1 平台侧

| 项 | 首轮（11:27–11:42） | 续接轮（14:32–14:39） | 判读 |
|---|---|---|---|
| EMQX 状态 | active，uptime 29m22s，`NRestarts=0` | active，uptime 3h34m18s，`NRestarts=0` | 未重启、未自愈式异常 |
| 8883 listener | `running=true` | `running=true`，`current_conn=0` | 一致 |
| 证书 serial / SAN | `…B83` / 双 IP | `…B83` / 双 IP，证书文件 mtime 10:58:05 | 无漂移 |
| 主机侧握手 | 20/20（Verify 0） | 10/10（14:32）与 5/5（14:38） | 重复可用 |
| `not_authorized` | 38 | 260（14:32）→ 266（14:38） | 持续认证拒绝 |
| `ssl_error` | 1（本能力负例） | 3→4→9（见 11.2 归因） | 无 App 归因证据 |
| 资源 | 内存 258 MiB、负载 0.00、根分区 10% | 内存 271 MiB、负载 0.00、根分区 10%、inode 4% | 无容量压力 |

### 11.2 `ssl_error` 突发的归因对照（`08_ssl_error_attribution.txt`）

首次续接采样中 `ssl_error` 在 97 秒内 4→9，若直接判为回归会误判。做三段分离：

| 段 | 条件 | `ssl_error` 增量 |
|---|---|---|
| 静默 120 秒 | 本机不发任何流量 | **0** |
| 套接字采样 60 秒 | 仅读取内核套接字表（0.3 秒一次） | **0** |
| 5 次正确握手 | 同一 CA、`-verify_ip` 通过 | **0** |

同段内 `einval +2`、`ssl_closed +3`，说明本能力探测只影响这两个计数，不产生 `ssl_error`；同一时间手机侧 TLS 错误为 0。结论：该突发来自未知外部 8883 连接（公网扫描类流量），**不能归因于新证书或 App**，也不作为通过项；`ssl_error` 因重启清零、日志级别为 warning 而缺少来源明细，仍是可观测性缺口（第 5 节缺口 2、3）。

### 11.3 客户端侧（`09_resume_phone_window.txt`）

| 指标 | 结果 |
|---|---|
| 观测区间（设备时间） | 13:39:38 ~ 14:34:37，829 条 `MqttService` 日志 |
| TLS 主机名/信任错误 | **0**（对比修复前 18 次） |
| 重连尝试 / 成功建立会话 | 33 / **0** |
| CONNACK `无权连接(5)` | 33 次，13:41:02 ~ 14:31:47 |
| 每次重连读取平台身份 | `mac_addr=000000000000`（占位）33 次 |
| BLE 采集 | 14:31:06 仍 `DATA_OK`；14:34:06 起 5 次尝试均失败（`status=133`、16000 ms 超时），`CONNECT_FAILED` |

判读：TLS 维度在变更后 3.5 小时内**持续**正常，无一次主机名错误；认证拒绝为**稳定复现的既有阻塞**，不是偶发。14:34 的 BLE 网关连接失败是本次新观察到的独立事件（此前 14:31 读取正常），影响“网关→手机”采集，与本证书变更无因果关系，也不由本能力修复；记录为运行观察，供网关/测试能力复现。

### 11.4 非 8883 回归（`10_service_regression_resume.txt`）

| 项 | 结果 |
|---|---|
| 端口 | 1883/8883/8083/8084/18083/5000/3306 全部 OPEN |
| Flask | `/login` 405、`/upload_data` GET 405（POST-only，未提交数据）、变更后错误行 0 |
| Dashboard | `/status` 200 |
| 单元 | `emqx`、`user_script` 均 active；`NRestarts=0` |
| 备注 | `mysql`/`mariadb` systemd 单元为 inactive，但 3306 有 2 个监听套接字，说明数据库由其他方式托管；本轮不连接、不写入，仅记录事实 |

### 11.5 续接轮后的结论修正

1. 首轮“TLS 持续可用”的表述由 44 分钟窗口扩展到约 3.5 小时两轮独立观测，结论方向不变。
2. 认证阻塞的量化从 24 次增加到 57 次（24+33）且跨 3.5 小时稳定复现，属于确定性的前置条件缺失，不是抖动。
3. 上传仍未执行；本轮不因“TLS 好转”或“App 在重连”推断上传已恢复。
4. 新增 1 项与本变更无关的运行观察（BLE 网关连接失败），按跨组件事实记录，不改变证书维度判定。
5. 平台 `not_authorized` 增量（约 1.2 次/分）高于手机日志可见的重连次数（约 0.7 次/分），差值来源未知（可能为日志缓冲区截断或其他占用同一 clientId/占位用户名的设备）；本条仅作口径提示，不作为归因结论。

---

## 12. 第三轮观测（CST 22:30 ~ 22:47，变更后 11h32m ~ 11h49m；覆盖 21:59:02 上传服务重部署）

### 12.1 本轮起因与观测对象的变化

- 触发：软件测试在 16:21:31 / 16:38:31 实测 `POST /upload_data` 返回 **401**（`artifacts/software_test_report_cert8883.md` §5），旧 Runtime 退出后由本能力重试可靠性检查。
- 对象变化（本轮实测，非推断）：
  - `/root/abr.py` 于 **21:59:02** 被替换：8949 B / sha256 `534dd40aac40c005…` → **8688 B / sha256 `a062b05f524dc8f6…`**；
  - `user_script.service` 于 **21:59:02** 重启，PID 214002 → **312368**，`NRestarts=0`（systemd 只记录一次启动，未计为崩溃重启）；
  - 变更目录 `/root/rd_upload_contract_20261125T135855Z`（`apply.log`、`rollback_remote.sh`、`verify_upload_contract.sh`、`BACKUP_SHA256SUMS.txt`），`apply.log` 记录 `removed=4 lines`、`VERDICT=PASS phase=post-fix`，并自述“next: real App upload retest must confirm 2xx and persistence readback”。
- 因此本轮**分段判定**：16:38 之前的 401 证据只属于旧版本；21:59:02 之后的受理结果单独统计。证书维度不受该重部署影响（`/var/lib/emqx` 证书文件 mtime 仍为 10:58:05，sha 未变）。

### 12.2 平台与 TLS（变更后 11h32m）

| 项 | 实测（CST 22:30 ~ 22:37） | 判读 |
|---|---|---|
| EMQX 单元 | active，`NRestarts=0`，`ActiveEnterTimestamp=2026-11-25 10:58:15`，uptime 11h31m47s | 未重启 |
| 8883 listener | `running=true`，`current_conn=0`，`established=0` | 可用 |
| 8883 计数器 128 秒配对增量 | `not_authorized` 328→330（+2），`ssl_error` 18→18（**0**），`ssl_closed` 48→48（0）；另有一次 96 秒配对 326→328 | 无真实 TLS 失败 |
| 证书身份 | serial `77ADCFD924BA0F87F845C81934D4A3F346D4BB83`，SAN `8.140.23.253` + `117.72.84.210`，有效期至 2028-11-24，文件 sha256 前缀 `27964ca8ed1c1596` | 无漂移 |
| 平台内握手（`-verify_ip` + App 信任锚） | 10/10，总耗时 101 ms | 通过 |
| 外网握手（同一信任锚，测试主机发起） | **12/12**，时延 min 80.1 / median 89.0 / max 105.8 ms，TLSv1.3 `TLS_AES_256_GCM_SHA384`，返回的叶子 sha256 `E7AD1E46…8884` | 通过 |
| 主机资源 | 负载 0.00，内存 1480/3499 MiB，根分区 10%，inode 4%，主机 uptime 34 天 | 无容量压力 |

`ssl_error` 自续接轮 9 次增至 18 次（跨约 7h51m，约 1.1 次/小时），但本轮 128 秒静默窗口与 12 次外网握手期间增量均为 **0**，客户端 TLS 错误为 0；因此仍判为**来源不明的外部 8883 失败尝试**（公网扫描类），不归因于新证书、不归因于 App，也不作为通过项。

### 12.3 上传受理结果（关键：0 次成功）

`journalctl -u user_script` 16:00 ~ 22:45 内 `POST /upload_data` 共 **28** 次：

| 结果 | 次数 | 时间/来源 | 说明 |
|---|---|---|---|
| 401 unauthorized | **6** | 16:21:31、16:27:17、16:28:57、16:38:31（`27.128.89.184`）；21:58:57、21:59:02（`127.0.0.1`，部署自检基线） | 全部发生在 **21:59:02 之前**；最后一例是部署前基线探测 |
| 400 body 校验 | **22** | 21:59:03 之后 19 次（`127.0.0.1` 部署自检 + `27.128.89.184` 9 秒级脚本探测）；其余为 16:28 空 JSON 与 22:00–22:17 的脚本探测 | `{"error":"No json data"}` / `{"error":"phone is required"}`，未进入 DB 写入路径 |
| 2xx 受理 | **0** | — | 21:59:03 之后仍为 0 |

要点：

1. **401 症状在 21:59:02 之后未再出现**（可比指标改善）：同身份、同 MAC 的探测从 `401 unauthorized` 前进到载荷校验（400），说明受理门被移除。
2. **没有任何真实客户端上传被观测到**：手机侧 `upload_last_status=401`、`upload_blocked_reason=AUTH_REJECTED`、`upload_blocked_at`=16:38:29、`upload_pending_count=5`；21:59 之后日志与平台记录中都没有来自该 App 的 `POST /upload_data`。即 App 仍处于暂停自动上传的状态，未重试。
3. 因此上传成功率是 **0 成功 / 0 真实尝试**（不可测量），**不得判恢复**；2xx 与 `sensor_data` 回读仍缺证据。
4. 观测限制：Flask 访问日志不含 User-Agent 与请求体，无法把每条 400 与具体客户端逐条绑定；`27.128.89.184` 是测试 LAN 共用出口（本机探测、其他角色测试同源），故按“节奏特征 + App 侧状态”判定为脚本探测，不当作 App 行为。

### 12.4 App 崩溃与自恢复（本轮新事件，交回实现）

| 项 | 实测 |
|---|---|
| 时间 | 22:00:24.731 CST，进程 31852 |
| 异常 | `FATAL EXCEPTION: main` — `java.lang.IllegalArgumentException: Navigation action/destination ...:id/action_homeFragment_to_deviceDetailFragment cannot be found from the current destination Destination(...:id/deviceDetailFragment)` |
| 栈位置 | `HomeFragment$setupRecyclerView$1.invoke(HomeFragment.kt:69)` ← `DeviceCardAdapter$CardViewHolder` 点击回调（`DeviceCardAdapter.kt:47`） |
| 影响 | `Force finishing activity`，进程被 SIG 9，前台退回桌面；采集/上传守护中断约 30 秒 |
| 自恢复 | 进程 32061 重启；22:27 曾 5 次 BLE `status=133` 失败并 `CONNECT_FAILED`；22:30:06 重连网关，22:30:28 `DATA_OK`（`id=6A002C00`、T=22.7 ℃、H=36.4 %）；待发箱仍为 5 条，无丢数据 |
| 归因 | 纯 UI 导航图缺陷（在 `deviceDetailFragment` 上再次点击设备卡触发），**与证书、TLS、上传受理均无关**；本能力只记录可复现栈，不修实现 |
| 说明 | 22:00 的点击不是本能力执行（本轮只做只读观测），故未主动复现崩溃 |

### 12.5 回退点与回退条件（更新）

| 对象 | 回退点 | 核对结果 |
|---|---|---|
| 8883 证书 | `/root/emqx-8883-cert-backup-20261125T025805Z` | `700 root:root`；`leaf.crt.bak` sha256 前缀 `a91de074d8ca0954` = 变更前基线；`leaf.key.bak`、`SHA256SUMS.txt` 齐全 |
| 上传服务 | `/root/abr.py.bak_20261125T135902Z`（8949 B，mode 600，sha256 `534dd40a…5063`） | 与 `apply.log` 的 `before_sha256` 一致；`rollback_remote.sh` + `BACKUP_SHA256SUMS.txt` 存在 |

回退后果须如实披露：上传服务回退即回到“401 拒绝所有上传”的已知失败状态；证书回退即回到 `No subjectAltNames` 的已知失败状态。两者都只能用于止损，**不得记为恢复健康**，也不得用回退后的状态证明任何用例通过。

新增回退触发条件（上传服务）：变更后出现 5xx / DB 连接异常 / 客户端上传出现无法归因的错误分类，且影响面大于原缺陷 → 用上述备份回退并重跑真实客户端复测。

### 12.6 监控项更新（仅建议，未改动任何 SLO）

M1–M8 保持原判据不变。本轮新增/强化：

| # | 观测项 | 建议阈值/判据 | 检查方式 | 当前基线 |
|---|---|---|---|---|
| M9 | 上传受理结果分布 | 真实客户端连续 10 分钟 2xx=0 → 告警；401/400 突增 → 告警 | Flask 访问日志按 `POST /upload_data` 状态码聚合 | 28 次中 401=6（均在 21:59 前）、400=22、2xx=0 |
| M10 | App 崩溃/ANR | 任一 `FATAL EXCEPTION` → 告警 | `logcat -b crash` 或崩溃上报 SDK | 22:00:24 崩溃 1 次，无自动上报 |
| M11 | 变更事件登记 | 服务重启 + 文件哈希变化即记事件 | `systemctl show` + `sha256sum` 定时抓取 | 21:59:02 `abr.py` 变更已由本轮人工发现 |

未变的可观测性缺口：无指标采集/告警平台；listener `shutdown_count` 随重启清零；EMQX 日志级别 warning 且无逐次 TLS 失败明细；证书到期无自动检查。

### 12.7 第三轮结论

1. **证书/TLS 维度：持续健康**。变更后 11.5 小时、三轮独立观测；平台内 10/10 与外网 12/12 握手全部通过，负例仍被拒，证书 serial/SAN 无漂移，`ssl_error` 在受控窗口内增量为 0。
2. **上传维度：症状改善但未证实恢复**。401 受理门已在 21:59:02 移除，此后 401 归零；但真实客户端未重试、2xx=0、无落库回读，成功率不可测，不能判 PASS。
3. **MQTT 会话：仍未建立**（`CONNACK 5`，`username=0200000000A1`）；属认证层既有缺口，与本证书变更无因果。
4. **新增 2 项未归因运行事件**：22:00:24 App 崩溃（UI 导航图）、22:27 BLE 连接失败（随后自恢复）；均记录事实与复现线索，交回实现/测试，运维侧不修产品代码。
5. **未执行项**：真实 App 上传复测与 `sensor_data` 回读（无写入口与 DB 直连授权）；本能力不据此判通过，也不代替测试能力执行。
