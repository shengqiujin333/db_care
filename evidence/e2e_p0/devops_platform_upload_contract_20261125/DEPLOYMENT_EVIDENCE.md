# 平台上传受理修复 —— 部署证据（DeploymentEvidence）

- 负责角色：运维工程
- 能力：部署操作
- 记录时间：2026-11-25T14:06Z 起，2026-11-25T14:20Z 复核（CST 22:06 ~ 22:20）
- 目标：既有平台 `8.140.23.253`（`iZ2zeih0gmrk47og5r4z7sZ`）上的 Flask 上传受理服务 `user_script.service`
- 部署单元：`deploy/platform_upload_contract/`（入口 `container.yaml` v2）
- 结论：**当前候选已在线且服务器侧可复验（EXECUTED）**；安卓真实上传 2xx 与 `sensor_data` 回读
  **未在本能力执行**，交软件测试用同一 App / 同一手机复测。

---

## 1. 本次分派的失败与处置边界

上游失败证据：`artifacts/software_test_report_cert8883.md`（CERT8883-UPLOAD-04 实测 FAIL），
归因分析：`artifacts/software_failure_analysis_cert8883_upload401.md`（平台 `POST /upload_data`
在请求体校验与数据库写入之前要求请求头 `X-Upload-Token`，App 按已批准契约不发送该头）。

本能力在 2026-11-25 21:58~21:59 CST 已按用户“选择 A”的授权完成该受理条件的部署修复，
并在 Runtime 退出前提交了工作（本地提交 `dc5b17b`，`git status` 干净）。本次为重试调用，
因此**没有再次改动服务，也没有再重启一次**：只做在线状态漂移复核（drift check），
确认线上对象仍是批准候选、无回滚或无外部改动，再发布本部署证据。

## 2. 实际部署入口与版本

| 项 | 值 |
|---|---|
| 服务单元 | `user_script.service`（`ExecStart=/root/venv/bin/python /root/abr.py`，监听 `0.0.0.0:5000`） |
| 部署对象 | `/root/abr.py`，`644 root:root`，8,688 B，250 行，inode 1054060，mtime 2026-11-25 21:59:02 +0800 |
| 部署版本（候选） | sha256 `a062b05f524dc8f6bce596e4571d339f697ef909b318503ad418ee0add24017c` |
| 上一版本（缺陷基线） | sha256 `534dd40aac40c00597b06710069cbcc70533650e01318e0637cd924851740c63`，8,949 B，254 行，2026-10-22 19:31:04 |
| 变更内容 | 仅删除 4 行（1 行注释、`UPLOAD_TOKEN` 常量、`if request.headers.get('X-Upload-Token') != UPLOAD_TOKEN` 与 `return 401`）；其余字节不变（`10_apply_diff.txt`，令牌值已脱敏） |
| 生效方式 | 重启 `user_script.service`，PID 214002 → 312368，启动 2026-11-25 21:59:02 CST，`NRestarts=0` |
| 执行入口（服务器侧） | `/root/rd_upload_contract_20261125T135855Z/apply_remote.sh --confirm`（dry-run → 基线核验 → 600 备份 → 补丁 → 重启 → 后置核验 → 失败自动回退） |
| 执行驱动（本机） | `evidence/e2e_p0/devops_platform_upload_contract_20261125/apply_driver.py`（SFTP 上传 + 远程执行，凭据不落盘、不回显） |
| 复验入口（公网） | `evidence/e2e_p0/devops_platform_upload_contract_20261125/17_external_recheck.txt`（脚本 `external_postfix_probe.py`，只发不触达数据库的请求体） |
| 复验入口（服务器） | `evidence/e2e_p0/devops_platform_upload_contract_20261125/16_state_drift_check.py` → `16_state_drift_check.txt` |

## 3. 前提与授权核对

- 批准产物已确认存在且与线上一致：`deploy/platform_upload_contract/container.yaml` v2
  （`change_status: APPLIED_AND_VERIFIED_SERVER_SIDE`）声明的 sha256 / 字节数 / 行数 / 处理器结构与线上逐项吻合。
- 授权：2026-11-25 用户回复“选择 A”，授权范围 = 删除该 4 行 + 重启 `user_script.service`；
  **未授予** 数据库直连或写 `sensor_data`、EMQX 证书 / 其他 listener / 配置文件变更、App 修改与重装、
  限流 / WAF / 新鉴权机制实现。本次执行与复核均未越出该范围。
- 配置与迁移前提：未改动 `/var/lib/emqx/configs/cluster.hocon`、任何 systemd 单元或系统包；
  无数据库迁移或 Schema 变更；服务解释器与虚拟环境未变（`/root/venv/bin/python`）。
- 未触碰：EMQX 与 8883 证书、MySQL 3306 与 `bwusernamebase`、`/register`、`/login`、
  `/sendVerificationCode`、`/verifyCode`、`/setmac` 语义、App/APK/固件与设备身份。

## 4. 本轮（重试）复核结果

只读漂移复核 `16_state_drift_check.txt`，14 项判据全部 OK，`VERDICT=PASS`：

| 判据 | 实测 | 结果 |
|---|---|---|
| 线上对象 = 批准候选 | sha256 `a062b05f…017c`、8,688 B | PASS |
| 门禁常量已不存在 | `X-Upload-Token` 计数 0、`UPLOAD_TOKEN` 计数 0；`200:def upload_data():` | PASS |
| 服务健康 | `ActiveState=active`、`SubState=running`、`NRestarts=0`、PID 312368（自 21:59:02 起未重启） | PASS |
| 回退点完整 | `/root/abr.py.bak_20261125T135902Z` 8,949 B、`600 root:root`、sha256 `534dd40a…0c63`；`rollback_remote.sh` 在位 | PASS |
| 端口 | 5000 / 8883 / 1883 / 18083 均 LISTEN | PASS |
| `ssl:default` | `listen_on 0.0.0.0:8883`、acceptors 16，EMQX active | PASS |
| 8883 证书回归 | serial `77ADCFD9…B83`、CN `8.140.23.253`、SAN 含 `8.140.23.253` 与 `117.72.84.210`、有效至 2028-11-24 | PASS（未变） |
| 路由语义 | `GET /upload_data` 405、`OPTIONS /upload_data` 200、`GET /login` 405 | PASS |
| 门禁移除 | `POST {"probe":1}` → **400 `phone is required`**（原 401）；携带任意 `X-Upload-Token` 同为 400；`POST {}` → 400 `No json data` | PASS |
| 业务日志 | 21:59:02 后无 401；`journalctl -p err` 自重启起无条目 | PASS |

公网复验 `17_external_recheck.txt`（2026-11-25T14:16:38Z）：

- `POST {"probe":1}` → `400 {"error":"phone is required"}`（原 401）；带任意 `X-Upload-Token` 亦为 400；
- `POST {}` → `400 {"error":"No json data"}`；`GET /upload_data` → 405；`GET /login` → 405；
- 1883/8883/8083/8084/18083/5000/3306 七端口 TCP 可达；
- 8883 以 App 信任锚 `jd-ca.crt` 链校验 OK（TLSv1.3），错误主机名被拒（`verify_code=62`）。

上述探针在 `phone` 校验处即返回，**未进入 `mysql.connect()`，未产生任何业务写入**。

## 5. 部署操作完成 ≠ 软件功能通过

服务器侧证据只能证明“门禁已移除、请求体能进入业务校验、服务与证书未回归”。
以下仍未验证，不得由本证据推导为通过：

- 同一 App（`base.apk` SHA-256 `80232c50…3de4b`）、同一手机 `12f046690506` 的真实上传是否返回 2xx；
- App 待发箱（401 期间保留的 5 条真实 BLE 读数）是否被 ack 清空；
- 按 `(phone, dev_id, time)` 回读 `sensor_data` 的 `temperature` / `humidity` 是否与本地读数一致；
- 同一 `(dev_id, time)` 重放是否不新增行（幂等）。

自重启（21:59:02）至 22:20 的平台日志中，`POST /upload_data` 全部来自
`127.0.0.1` 或本轮探针主机（均为 `{"probe":1}` / `{}`，返回 400），**尚无一次 200**，
即真实 App 上传路径在本部署版本上尚未被实际执行过。

## 6. 回退方案（可执行，未演练）

- 回退点：`/root/abr.py.bak_20261125T135902Z`（`600 root:root`，与变更前逐字节一致）。
- 回退命令：`bash /root/rd_upload_contract_20261125T135855Z/rollback_remote.sh --backup /root/abr.py.bak_20261125T135902Z`
- 回退后预期：恢复 `X-Upload-Token` 门禁，App 上传再次 401 —— 属**已知失败状态**，不得记为健康。
- 演练状态：`NOT_DRILLED_IN_PRODUCTION`（演练需再中断 5000 两次，未获该额外窗口授权）；
  回退脚本已离线自测，执行器在后置核验失败时会自动回退。

## 7. 残留风险

1. **安全**：`/upload_data` 现为无认证写入，且 5000 端口对公网开放，任何可达客户端都能向
   `sensor_data` 写入；限流 / 来源白名单 / 正式鉴权尚未实现（需另行决策与授权）。
2. 备份 `abr.py.bak_20261125T135902Z` 仍含被移除的共享令牌值（仅 root 可读）；
   彻底移除需另行授权删除该备份（删除即失去回退点）。
3. 未设立上传量监控 / 告警，异常灌入不会被自动发现。
4. 上传日志窗口显示 22:00 与 22:04 各 3 次 400 请求，均为本轮探针主机所发，非真实客户端。

## 8. 交接

- 交软件测试：用同一 APK、同一手机执行真实上传，确认 `POST /upload_data` 返回 2xx、待发箱被清空，
  并在获准的只读入口按 `(phone, dev_id, time)` 回读字段、验证同键重放不增行。
  若仍非 2xx，保留请求时间、状态码、响应体与平台日志窗口交回本能力。
- 尚未取得的授权：`sensor_data` 只读回读入口（数据库直连不在本能力授权内）。
- 不在本能力范围：App 上传复测、端到端 P1 判定、限流/鉴权方案实施。

## 9. 复验命令

```bash
# 服务器只读漂移复核（14 项判据，凭据从项目资料读取，不回显）
C:/Users/kason/anaconda3/python.exe evidence/e2e_p0/devops_platform_upload_contract_20261125/16_state_drift_check.py

# 公网接口复验（只发 {} 与 {"probe":1}，不触达数据库）+ 8883 链/负例
C:/Users/kason/anaconda3/python.exe evidence/e2e_p0/devops_platform_upload_contract_20261125/external_postfix_probe.py

# 服务器侧核验（在服务器上，已授权 SSH 会话）
bash /root/rd_upload_contract_20261125T135855Z/verify_upload_contract.sh --phase post-fix
```

预期：`VERDICT=PASS`；`POST probe -> 400 phone is required`；`8883 chain OK / negative control rejected`。

## 10. 交付判定

| 判据 | 结果 |
|---|---|
| 线上服务运行的是批准候选版本（sha256/字节数/行数一致） | PASS |
| 上传门禁已移除，请求体校验与写库顺序未变 | PASS |
| 服务单元健康、无重启循环、重启后无错误日志 | PASS |
| 回退点与回退脚本可用 | PASS（未演练） |
| 8883 证书 / 其他端口 / 路由语义无回归 | PASS |
| 安卓真实上传 2xx + 持久化回读 | **NOT EXECUTED**（交软件测试） |

**部署结果：EXECUTED —— 当前候选已在线、可复验；业务上传通过与否以软件测试的独立复测为准。**
