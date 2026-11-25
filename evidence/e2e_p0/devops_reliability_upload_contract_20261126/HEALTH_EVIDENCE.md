# 上传受理修复后健康证据（HealthEvidence，第四轮）

- 负责角色：运维工程
- 观测窗口：2026-11-25T21:22Z ~ 21:32Z（CST 2026-11-26 05:22 ~ 05:32），变更基准 CST 2026-11-25 21:59:02（`/root/abr.py` 门禁移除 + `user_script.service` 重启）
- 对应监控报告：`RELIABILITY_REPORT.md`（本目录）
- 判定口径：只对**本轮实际观测到**的事实判 PASS / FAIL / 未达成；“未执行”一律如实标注，不用部署记录、部署自检或变更意愿替代运行证据

---

## 1. 逐项健康判据

| # | 判据 | 结果 | 证据 |
|---|---|---|---|
| H1 | 线上运行的是批准候选版本 | **PASS** | `/root/abr.py` 8,688 B、`644 root:root`、mtime 2026-11-25 21:59:02、sha256 `a062b05f524dc8f6bce596e4571d339f697ef909b318503ad418ee0add24017c`；`X-Upload-Token`=0、`UPLOAD_TOKEN`=0、`200:def upload_data():`（`19_`） |
| H2 | 服务持续稳定运行、无重启循环、无错误日志 | **PASS** | `user_script` active/running、`NRestarts=0`、`ExecMainStartTimestamp=21:59:02`、PID 312368；`journalctl -p err` 自重启起无条目；Traceback/5xx 计数 0；`Restart=on-failure`、`StartLimitBurst=5/10s` 未触发（`19_`、`22_`） |
| H3 | 8883 证书可由 App 信任锚验证且主机名命中 | **PASS** | serial `77ADCFD9…B83`、SAN `8.140.23.253` + `117.72.84.210`、有效期至 2028-11-24；平台内 10/10（109 ms）、外网 12/12（中位 130.8 ms）；负例 `8.140.23.254` 被拒（`19_`、`21_`） |
| H4 | 8883 无新增真实 TLS 失败 | **PASS** | `ssl_error` 6h45m 未增长（18→18）；128 秒配对增量 `not_authorized/ssl_error/ssl_closed` 全 0；`established=0`；本机探测影响仅在 `ssl_closed`/`einval`（`19_`） |
| H5 | 客户端 TLS 错误不再出现 | **PASS** | 03:07–05:25 缓冲 1,574 条 MqttService 日志中 `No subjectAltNames`/握手异常 **0**（对比修复前 18 次）（`20_`） |
| H6 | 原 401 症状消除（可比指标） | **PASS（症状维度）** | 21:59:03 起 `POST /upload_data` 19 次，状态 `{400:19}`、401=0、5xx=0；来源 `127.0.0.1`(10) + `27.128.89.184`(9)（`23_`） |
| H7 | 安卓真实上传已恢复 | **未达成（无运行证据，不判 PASS）** | 真实客户端 0 次尝试、`2xx=0`；最后一次上传请求 22:17:15，其后 7h09m 静默；App `upload_last_status=401`、`AUTH_REJECTED` 暂停态未变（`20_`、`23_`） |
| H8 | 上传写库分支可执行 | **NOT EXECUTED（0 次）** | 变更后无 `mysql.connect`/`MySQLdb` 行；19 次请求均在 `phone` 校验处 400；`sensor_data` 无新增行可回读（`19_`） |
| H9 | MQTT 会话建立 | **未达成** | 27 次 `无权连接 (5)`（最后 05:20:30）；平台 `not_authorized=492`、`username=0200000000A1`、`reason=not_authorized`（`19_`、`20_`） |
| H10 | 非上传路径未回归 | **PASS** | 1883/8883/8083/8084/18083/5000/3306 监听一致；`/login` 405、`/upload_data` GET 405、OPTIONS 200、Dashboard `/status` 200（`19_`） |
| H11 | 回退点完整、回退路径明确 | **PASS（未演练）** | `abr.py.bak_20261125T135902Z` 8,949 B/`600`/sha `534dd40a…5063`；`rollback_remote.sh`+`verify_upload_contract.sh`+`BACKUP_SHA256SUMS.txt` 在位；证书备份 `leaf.crt.bak`=`a91de074…`（`19_`、`22_`） |
| H12 | App 运行稳定性 | **PASS（无新增崩溃）** | PID 32061 连续运行 7h25m；`logcat -b crash` 仅第三轮 22:00:24 同一条栈，无新增 `FATAL EXCEPTION`（`20_`） |
| H13 | App 持续采集并产生新待发数据 | **FAIL（本轮实测）** | `temperature` 最后一行 21:58:41，7h33m 无新增；`measure_state=starting`；待发 9 条（最后 21:58:41）；同库 `alarm_events` 最后 05:26:30 仍在写（`20_`、`25_`、`26_`） |
| H14 | 网关 BLE 链路稳定 | **FAIL（间歇）** | 03:07–05:26：`DATA_OK` 18 次、`CONNECT_FAILED` 19 次（约 3 分钟周期，16000 ms 超时 / `status=133`）（`20_`） |
| H15 | 监控/告警能及时发现同类故障 | **GAP（不判 PASS）** | 无 exporter/prometheus/telegraf/filebeat/zabbix/alert 服务；无 prometheus 配置；无自定义 timer/cron；`emqx ctl alarms` 不可用；上传无计数器；App 崩溃无上报；变更无事件登记（`19_`、`22_`） |
| H16 | 公网写接口滥用风险受控 | **RISK（未受控）** | `/upload_data` 已无认证且 5000 对公网开放；6h 内观测到 `/etc/passwd`、`@fs/…`、webpack/vite/nuxt 路径穿越、`/mcp`、`/sse`、`/sdk`、nmap 指纹等扫描，全部 404，未触达写路径（`22_`） |

---

## 2. 关键实测值

| 指标 | 值 |
|---|---|
| 上传受理（21:59:03–05:27，7h28m） | 19 次 POST：400=19、401=0、2xx=0、5xx=0 |
| 上传静默时长 | 最后请求 22:17:15 → 观测结束 7h09m 无请求 |
| 上传真实来源 | `127.0.0.1` 10 次（部署自检）、`27.128.89.184` 9 次（测试主机脚本）；无 App 来源 |
| 写库路径执行次数 | 0（无 `mysql.connect` 行） |
| 平台内握手 | 10/10，109 ms |
| 外网握手 | 12/12，min 100.0 / median 130.8 / max 226.5 ms，TLSv1.3 |
| listener 计数器（128 秒） | `not_authorized` 492→492、`ssl_error` 18→18、`ssl_closed` 56→56、`established` 0 |
| 证书剩余有效期 | 约 728 天（至 2028-11-24T02:55:16Z） |
| 服务资源 | `user_script` 60.3 MiB、`emqx` 265.7 MiB、load 0.00、内存 1,497/3,499 MiB、root 10%、inode 4% |
| 手机日志窗口 | 03:07:10–05:25:29，1,574 条 MqttService；TLS 错误 0；重连 28；`CONNACK 5` 27；崩溃 0（窗口内） |
| 手机本地库 | `temperature` 124 行（最后 21:58:41）；`pending_uploads` 9 行（attempts 合计 100）；`alarm_events` 515 行（最后 05:26:30） |
| App prefs | `upload_last_status=401`、`AUTH_REJECTED`、`upload_blocked_at`=16:38:29、`upload_pending_count=5`、`measure_state=starting` |
| 公网扫描（6h） | `/etc/passwd` 类 11 次、`@fs` 5 次、webpack/vite/nuxt/next 8 次、`/mcp`+`/sse`+`/sdk`+nmap 等；全部 404 |

---

## 3. 结论与边界

1. **部署对象健康**：线上确为批准候选、门禁已移除、服务 7.5 小时稳定、无错误日志与 5xx，回退点完整。该项可判 PASS。
2. **原 401 症状已消失**：同一接口、同一身份链路的请求从“401 拒绝”前进到“载荷校验 400”，且 401 归零 6h 以上。**这只是症状维度的改善**。
3. **上传未恢复**：真实客户端 0 次尝试、`2xx=0`、写库分支 0 次执行、无 `sensor_data` 回读。按本轮可验证目标判 **未达成**，不得表述为“安卓上传已恢复”。
4. **门禁移除带来的两个新风险**：① 写库分支从未被执行，其可用性无运行证据；② `/upload_data` 无认证且公网可达，已出现互联网扫描（全部 404）。最小缓解（安全组/来源白名单/限流）需授权后实施，本能力未改动线上配置。
5. **App 侧三项独立事实（跨组件，交回实现/测试，不由本能力修复）**：
   - `AUTH_REJECTED` 后全局暂停且无自动恢复 → 平台修复后链路仍长期静默；
   - 测量表自 21:58:41 无新增、`measure_state=starting`，而告警表持续写入 → 无新待发读数；
   - 网关 BLE 约 49% 连接尝试失败（18 成功/19 失败）。
6. **MQTT 会话仍未建立**（`CONNACK 5`，身份 `0200000000A1`），属既有认证缺口，与本轮上传受理变更、与 8883 证书变更均无因果。
7. **不声明生产健康**：本证据为 7.5 小时限时只读观测，无指标平台、无长期趋势、无数据库回读；上传成功率为 0 成功 / 0 真实尝试（不可测量）。回退会把系统带回“401 拒绝所有上传”的已知失败状态，只用于止损，不得记为恢复健康。

---

## 4. 证据索引（本目录）

`19_r4_platform_window.txt`、`20_r4_phone_window.txt`、`21_r4_external_tls.txt`、`22_r4_endpoint_and_ops_gaps.txt`、
`23_r4_upload_caller_matrix.txt`、`24_r4_local_db_snapshot.db`(+`-journal`)、`25_r4_local_pending_rows.txt`、
`26_r4_phone_collection_gap.txt`、`RELIABILITY_REPORT.md` 及对应 `r4_*.py` 只读探针。

部署侧事实以 `evidence/e2e_p0/devops_platform_upload_contract_20261125/DEPLOYMENT_EVIDENCE.md` 为准；本轮未重复部署、未改动线上。
