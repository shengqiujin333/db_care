# 8883 证书变更后健康证据（HealthEvidence）

- 负责角色：运维工程
- 观测窗口：2026-11-25T03:27Z ~ 03:42Z（CST 11:27 ~ 11:42），变更基准 CST 10:58:15（EMQX 重启加载新证书）
- 续接窗口：2026-11-25T06:32Z ~ 06:39Z（CST 14:32 ~ 14:39），同一对象、同一判据，用于确认持续性而非单点可用
- 第三轮窗口：2026-11-25T14:30Z ~ 14:47Z（CST 22:30 ~ 22:47，变更后 11.5 小时），覆盖实现侧 21:59:02 的上传服务重部署与软件测试 16:38 的 401 失败
- 对应监控报告：`RELIABILITY_REPORT.md`（本目录）
- 判定口径：只对**本轮实际观测到**的事实判 PASS/FAIL；“未执行/未达成”一律如实标注，不用部署记录或口头声明替代

---

## 1. 逐项健康判据

| # | 判据 | 结果 | 证据 |
|---|---|---|---|
| H1 | EMQX 服务持续运行，8883 listener `running=true` | **PASS** | 首轮 `emqx active`、`NRestarts=0`、uptime 29m22s；续接轮 uptime 3h34m18s、`NRestarts=0`、`ssl:default running true`；第三轮 uptime 11h31m47s、`NRestarts=0`、`ssl:default running true`（`01_`、`07_`、`11_`） |
| H2 | 8883 叶子可由 App 信任锚验证，且 SAN 命中实际连接地址 | **PASS** | 首轮 20/20 握手成功；续接轮 10/10 与 5/5；第三轮平台内 10/10（101 ms）与外网 12/12（median 89.0 ms），叶子 sha256 `E7AD1E46…8884`，错误 IP `8.140.23.254` 仍被拒（`04_`、`07_`、`11_`、`13_`） |
| H3 | 客户端 TLS 阻断已解除（同一 APK/同一手机） | **PASS** | 修复前 18 次 `No subjectAltNames`（10:31:02–10:56:45）；修复后 0 次，覆盖 11:01–22:30 共 64 次重连（`05_`、`09_`、`12_`） |
| H4 | 客户端完成 MQTT CONNECT 并获得会话 | **未达成（CONNACK 5）** | 三轮累计 64 次 `无权连接 (5)`；第三轮 username 已为测试身份 `0200000000A1`，平台 `reason=not_authorized`，21:59:47–22:30:30 共 7 次（`02_`、`07_`、`09_`、`12_`） |
| H5 | 安卓上传 `/upload_data` 与落库可回读 | **FAIL（实测 401）/ 修复后未证实** | 16:21:31、16:38:31 真实调用 401（测试报告 §5）；21:59:02 重部署后 401 归零，但 19 次 POST 全为 400 载荷校验、2xx=0、无真实客户端重试、无回读（`17_`、`18_`）；**不判恢复** |
| H6 | 非 MQTT 路径与其他服务未回归 | **PASS** | 三轮均 1883/8083/8084/18083/5000/3306 OPEN；Flask `/login` 405、`/upload_data` GET 405、Dashboard `/status` 200；手机 BLE 22:30:28 仍 `DATA_OK`（`07_`、`10_`、`11_`、`12_`） |
| H7 | 回滚点完整可用 | **PASS** | 证书备份 `leaf.crt.bak` = 基线 `a91de074…0855f9`；上传服务另有 `abr.py.bak_20261125T135902Z`（8949 B，sha `534dd40a…`）与 `rollback_remote.sh`（`06_`、`07_`、`15_`、`17_`） |
| H8 | 变更后可归因于证书的新增故障 | **0 项** | 首轮 `ssl_error` 1 次（本能力负例）；续接轮 +5 突发三段对照增量 0；第三轮 128 秒静默与外网 12 次握手期间增量均 0、客户端 TLS 错误 0（`08_`、`11_`、`13_`） |
| H9 | 监控/告警能及时发现同类故障 | **GAP（不判 PASS）** | 无指标采集/告警；`ssl_error` 变更前累计 2019 次仍无告警；计数器随重启清零；新增：上传受理无计数器、App 崩溃无上报（`03_`、`18_`、`14_`） |
| H10 | App 运行稳定性（第三轮新增） | **FAIL（1 次崩溃，已自恢复）** | 22:00:24 `FATAL EXCEPTION: main` — `Navigation action … action_homeFragment_to_deviceDetailFragment cannot be found`（`HomeFragment.kt:69`）；进程被 SIG 9，约 30 秒后重启，22:30:28 `DATA_OK`，待发 5 条未丢（`12_`、`14_`） |
| H11 | 上传服务变更可观测性（第三轮新增） | **部分满足** | 21:59:02 `abr.py` 8949→8688 B 有备份与 `apply.log`；但无变更事件登记、无指标，故障仅能从访问日志事后统计（`15_`、`16_`、`17_`） |

---

## 2. 关键指标实测值

| 指标 | 值 |
|---|---|
| 主机侧 TLS 握手 | 20/20 成功；时延 min 105.1 / median 114.6 / max 133.0 ms |
| 负数控制 | `verify_ip=8.140.23.254` 被拒（IP mismatch） |
| 平台 8883 `shutdown_count`（33 分钟） | `not_authorized 38`、`ssl_error 1`、`ssl_closed 11`、`einval 7` |
| 平台 broker 日志 TLS 失败行 | 0 |
| 手机重连 / 成功 / TLS 错误 | 24 / 0 / 0（11:01–11:36:45） |
| 服务资源 | 负载 0.00，内存 258 MiB / 3499 MiB，根分区 10%，`NRestarts=0` |
| 证书剩余有效期 | 约 730 天（至 2028-11-24T02:55:16Z） |
| 续接轮握手 | 10/10（14:32）、5/5（14:38），`verify_ip` 全程通过 |
| 续接轮客户端 | 13:39–14:34：829 条日志、TLS 错误 0、重连 33、成功 0、占位身份 33 |
| 续接轮资源 | 内存 271 MiB、负载 0.00、根分区 10%、inode 4%、uptime 3h34m、`NRestarts=0` |
| `ssl_error` 归因对照 | 静默 120 秒增量 0；套接字采样 60 秒增量 0；5 次正确握手增量 0 |
| 第三轮外网握手 | 12/12 成功；时延 min 80.1 / median 89.0 / max 105.8 ms；TLSv1.3 |
| 第三轮 listener 增量（128 秒） | `not_authorized` +2、`ssl_error` 0、`ssl_closed` 0、`established` 0 |
| 第三轮上传受理（16:00–22:45） | 28 次 POST：401=6（全部 ≤21:59:02）、400=22、2xx=**0**；21:59:03 后 19 次全 400 |
| 第三轮 App 状态 | vC10/1.9 未变；`upload_last_status=401`、`AUTH_REJECTED`（16:38:29）、待发 5 条；22:00:24 崩溃 1 次后自恢复 |
| 第三轮资源 | 内存 1480/3499 MiB、负载 0.00、根分区 10%、inode 4%、EMQX uptime 11h31m、`NRestarts=0` |

---

## 3. 结论与边界

1. **证书维度：健康**。平台侧与客户端侧各自独立证明 TLS 阻断已解除，且判据未被放宽（负例仍失败）；该结论在间隔 2h50m 的第二轮观测中重复成立（TLS 错误 0、握手全通过、证书无漂移）。
2. **端到端可用：未恢复**。App 能完成 TLS，但因平台身份是占位值 `000000000000` 被认证拒绝，MQTT 会话未建立（两轮可观测累计 57 次，平台计数器 260 次含未落日志的尝试）；因此不能声明“安卓上传已恢复”，也不能把该失败记作证书缺陷。
3. **不声明生产健康**：本轮只覆盖两个合计约 3.5 小时的观测点与既有服务现状；无长期趋势、无连接数曲线、无上传接口日志、无 DB 回读。
4. **后续必要动作**（不在本能力执行）：由测试/实现侧补齐隔离测试身份（`mac_addr`、`registerphone`），再按 `CERT8883-UPLOAD-04` / `E2E-JOINT-P0/P1` 执行上传与联合复测；运维侧按第 5 节监控项补齐告警与证书到期检查。
5. **跨组件观察（本能力不判定、不修复）**：14:34:06 起手机连接网关 BLE 连续 5 次失败（`status=133`、16000 ms 超时，`CONNECT_FAILED`），而 14:31:06 仍 `DATA_OK`；与证书变更无可证因果，需网关/测试能力另行复现。该事件使“即便身份补齐”也无法在此时段完成端到端上传，属需要保留的现场事实。
6. **第三轮修正（22:47）**：以上第 1～2 条仍然成立，但对象已有变化——实现侧 21:59:02 重新部署了上传服务（`abr.py` 8949→8688 B，移除 4 行受理门），此后 **401 归零**；然而真实客户端未重试（App 仍为 `AUTH_REJECTED` 暂停态、待发 5 条），2xx=0 且无回读，故上传仍**不得判恢复**，只可表述为“原 401 症状已消失、恢复未证实”。
7. **第三轮新增失败项**：App 22:00:24 `FATAL EXCEPTION`（`action_homeFragment_to_deviceDetailFragment cannot be found`，`HomeFragment.kt:69`），进程被系统结束，约 30 秒后自恢复并续采；属产品 UI 缺陷，交回实现，不计入证书维度，也不由本能力修复。
8. **不声明生产健康**：本轮仍为限时只读观测，无指标平台、无长期趋势、无 `sensor_data` 回读；上传成功率为 0 成功 / 0 真实尝试（不可测量）。

---

## 4. 证据索引

`01_platform_reliability_window.txt`、`02_auth_rejection_rate.txt`、`03_alarms_and_log_retention.txt`、
`04_connectivity_sample.txt`、`05_phone_app_window.txt`、`06_rollback_point.txt`（首轮）；
`07_resume_window.txt`、`08_ssl_error_attribution.txt`、`09_resume_phone_window.txt`、`10_service_regression_resume.txt`（续接轮；均在本目录）。
第三轮：`11_resume3_platform_window.txt`、`12_resume3_phone_window.txt`、`13_resume3_external_tls.txt`、`14_resume3_app_crash.txt`、`15_resume3_upload_service_change.txt`、`16_resume3_deploy_context.txt`、`17_resume3_upload_gate_apply_log.txt`、`18_resume3_upload_outcomes.txt` 及对应 `resume3_*.py` 探针（均在本目录）。
部署侧证据仍以 `evidence/e2e_p0/devops_8883_cert_20261125/DEPLOYMENT_EVIDENCE.md` 为准，本轮不重复部署、不改动线上。
