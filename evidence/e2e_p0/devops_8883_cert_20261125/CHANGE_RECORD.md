# E2E-JOINT-P0 平台段 8883 服务器证书变更记录（基础设施管理）

- 负责角色：运维工程（基础设施管理）
- 对象：既有平台 `8.140.23.253` 的 EMQX 5.8.0 `8883` TLS listener 服务器证书/部署文件
- 上游依据：
  - `evidence/e2e_p0/P0_platform_stage_report.md`（TEST_FAIL / `MEASURED_DEFECT`）
  - `evidence/e2e_p0/P0_platform_failure_triage.md`（REPAIR_REQUIRED：SAN 仅 `117.72.84.210`）
  - `evidence/e2e_p0/P0_backend_cert_issuance_and_rollback.md`（核发 C1–C6 / 回退 R1–R4 条件）
  - 操作者答复（2026-11-24T05:40Z）：“允许维护本项目的证书，请先核发签发和回退条件，再按现有运维方式处理。”
- 结果：**已替换并通过基础设施判据（APPLIED）**。证书修复不等于 App→平台上传复测通过；客户端复测由软件测试能力执行。

> 本文件不含私钥、CA 私钥或口令。服务器上只存在叶子证书与其私钥（`600 emqx:emqx`）。

---

## 1. 变更前后对照

| 项 | 变更前 | 变更后 |
|---|---|---|
| subject / CN | `CN=117.72.84.210` | `CN=8.140.23.253` |
| issuer | `CN=Tiktok Root CA, O=LinkValue` | 同（同一 CA，未更换信任锚） |
| SAN | `IP Address:117.72.84.210` | `IP Address:8.140.23.253, IP Address:117.72.84.210` |
| serial | `77ADCFD924BA0F87F845C81934D4A3F346D4BB82` | `77ADCFD924BA0F87F845C81934D4A3F346D4BB83` |
| SHA-256 指纹 | `48:76:E7:30:…:AD:75` | `E7:AD:1E:46:FA:5B:5D:06:02:C5:AD:04:B7:8B:1C:AF:2E:78:EE:47:8A:9F:6A:79:EA:B6:3B:D5:54:BD:88:84` |
| 有效期 | 2024-10-12 → 2026-12-21 | 2026-11-25T02:55:16Z → 2028-11-24T02:55:16Z（730 天） |
| 私钥 | RSA（原文件 `644`） | 新 RSA 2048，安装为 `600 emqx:emqx` |
| `openssl verify -verify_ip 8.140.23.253` | FAIL（error 64 IP address mismatch） | OK |
| App 视角 | `SSLHandshakeException: No subjectAltNames on the certificate match` 的直接原因 | 该原因已消除（待客户端复测确认） |

CA 指纹保持 `75:33:09:B3:…:15:2B`（App 信任锚 `assets/jd-ca.crt` 未被替换）。

## 2. 服务器上的变更对象

| 项 | 路径/值 |
|---|---|
| certfile | `/var/lib/emqx/certs/listeners/ssl/default/cert-2D2ADC29A9BE14AA` |
| keyfile | `/var/lib/emqx/certs/listeners/ssl/default/key-6E6F6ED56285A948` |
| cacertfile | `/var/lib/emqx/certs/listeners/ssl/default/cacert-613B9023E1E698F0`（未改动） |
| listener 配置来源 | `/var/lib/emqx/configs/cluster.hocon` 的 `listeners.ssl.default.ssl_options`（路径未改动） |
| 备份目录 | `/root/emqx-8883-cert-backup-20261125T025805Z/`（`700`；`leaf.crt.bak`、`leaf.key.bak`、`SHA256SUMS.txt`） |
| 作业目录 | `/root/rd_ops_20261125/`（脚本与日志；已删除暂存的私钥副本） |

除本表外未改动任何服务配置、认证库、其他 listener、Flask 路由或数据库。

## 3. 时间线（UTC）

| 时间 | 动作 | 结果 |
|---|---|---|
| 02:45–02:52 | SSH 只读侦察：`emqx ctl status/listeners`、cluster.hocon 的 ssl_options、证书文件与摘要、CA 私钥候选枚举 | 确认线上证书仍为基线；服务器无 CA 私钥（`ca_signing_capability.txt`） |
| 02:53–02:55 | 在操作主机用原 CA 私钥签发新叶子证书并自检 | 链 OK、`-verify_ip 8.140.23.253` OK、负例 IP 不通过、私钥与证书成对 |
| 02:56:17 | 变更前基线快照（listener/端口/HTTP/磁盘） | `00_pre_change_baseline.txt` |
| 02:57:01 | 第一次 apply：安装 + `emqx ctl listeners restart ssl:default` | 文件已换但握手仍返回旧证书 → 脚本按 R1 自动回退，`RESULT overall=ROLLED_BACK`（`01_deploy_apply.txt`） |
| 02:58:05 | 第二次 apply：`emqx ctl conf reload` 仍无效 → `systemctl restart emqx` | 握手返回新 serial `…B83`、`Verify return code: 0 (ok)`、`RESULT overall=APPLIED`（`02_deploy_apply_v2.txt`） |
| 02:58:44 | 外部主机判据（`scripts/emqx_8883_cert_verify.sh --expect-new`） | `tls_handshake=OK / chain_verify=OK / san_contains_target=YES / ip_san_match=OK / negative_control=OK / serial_changed=YES / overall=PASS`（`03_verify_from_host.txt`） |
| 02:59 | 清理：删除证书目录内的 `.pre-` 私钥副本、加固备份权限、`shred` 作业目录暂存私钥 | `04_cleanup.txt` |
| 03:00 | 清理后复跑外部判据 | `overall=PASS`（`05_verify_after_cleanup.txt`） |
| 03:01 | MQTT 层存活探测（TLS 主机名校验开启 + 故意无效凭据） | TLS 建立成功、`CONNACK_RC=5 (not_authorized)`，说明 listener 与认证子系统正常（`06_mqtt_layer_probe.txt`） |

EMQX 服务重启一次（02:58Z）：`uptime` 由 10 天 4 小时归零，重启前后 `current_conn` 均为 0；
重启后 1883/8883/8083/8084/18083/5000/3306 全部 OPEN，Flask `/login` GET 返回 405（原样），
Dashboard `/status` 返回 200（原样）。

## 4. 判据核对（服务器侧 + 外部）

| # | 判据 | 结果 | 证据 |
|---|---|---|---|
| 1 | 新证书 SAN 含 `IP Address:8.140.23.253` | PASS | 02/03 |
| 2 | `openssl verify -CAfile jd-ca.crt -verify_ip 8.140.23.253` = OK | PASS | 02/03/05 |
| 3 | 握手返回叶子 serial/指纹与签发记录一致（`…B83`） | PASS | 02/03 |
| 4 | 负例 `8.140.23.254` 仍失败（判据未放宽） | PASS | 03/05 |
| 5 | 8883 listener `running=true`，仍为 TLS | PASS | 02 |
| 6 | 其他 listener / Flask 5000 / MySQL 3306 无回归 | PASS | 02 |
| 7 | 客户端（App）复测不再出现 SAN 不匹配 | **NOT EXECUTED**（交软件测试能力） | — |

## 5. 运维发现（供后续同类变更使用）

1. **本部署上仅重载 listener 不足以换证**：`emqx ctl listeners restart ssl:default` 与
   `emqx ctl conf reload` 之后握手仍返回旧叶子证书（文件已更新、listener 状态 running）。
   必须 `systemctl restart emqx`。该结论由第一次 apply 的失败与第二次成功对照实测得出，
   已固化进 `deploy/emqx_8883_tls/apply_remote.sh`（conf reload → service restart → 复核 → 失败回退）。
2. **证书文件被 EMQX 以固定文件名引用**：替换沿用原路径，避免改 `cluster.hocon`，因此配置面零变更。
3. **私钥权限**：原 keyfile 为 `644`；本次安装为 `600 emqx:emqx`，并删除证书目录内 `.pre-` 私钥副本，
   仅保留 `700` 备份目录中的 `600` 备份。
4. **SAN 保留旧 IP**：按已定稿模板保留 `IP:117.72.84.210`（历史资料兼容，不作为连接目标）。
   下一次轮换可直接只保留 `8.140.23.253`。

## 6. 未执行 / 未证明

- Android App → `ssl://8.140.23.253:8883` 的真实 TLS/CONNACK/上传复测（由软件测试能力执行，需同一 APK 与同一手机）。
- P1 上传与 MySQL `sensor_data` 持久化回读（需测试身份与 DB 授权）。
- EMQX 服务日志/连接数基线、Dashboard 凭据侧操作（本轮仅只读端口与 `/status`）。
- 证书自动轮换（本次为一次性替换；到期 2028-11-24）。
- 未验证手机上其它 App、其他网络出口的行为。

## 7. 影响范围

- 正向：App 连接 `8.140.23.253:8883` 时 TLS 主机名校验可命中 SAN，不再在握手阶段被拒。
- 中性：`117.72.84.210` 仍被新证书覆盖（旧地址已下线，不构成连接目标）。
- 风险：任何依赖“证书 CN/SAN 必须是 117.72.84.210”的旧客户端若仍存在，会在换证后失去覆盖（SAN 仍保留该 IP，故无此风险）。
- 未改变：MQTT 认证规则、App 信任锚、端口语义、Flask/MySQL、数据库内容。

## 8. 回退

仓库脚本：`deploy/emqx_8883_tls/rollback_remote.sh`；在服务器上以备份目录为参数执行：

```bash
# 将 deploy/emqx_8883_tls/rollback_remote.sh 上传到服务器后执行
bash rollback_remote.sh \
  --backup-dir /root/emqx-8883-cert-backup-20261125T025805Z \
  --certfile  /var/lib/emqx/certs/listeners/ssl/default/cert-2D2ADC29A9BE14AA \
  --keyfile   /var/lib/emqx/certs/listeners/ssl/default/key-6E6F6ED56285A948 \
  --ca        /root/rd_ops_20261125/jd-ca.crt
```

回退后 serial 应回到 `77ADCFD924BA0F87F845C81934D4A3F346D4BB82`，且 `verify_ip 8.140.23.253`
重新失败——即回到**已知失败**状态，不得计为通过。

## 9. 交回

- 交回软件测试：同一用例 `E2E-JOINT-P0`、同一 APK（`com.jinyuni.dengbei_care` vC10/vN1.9，
  SHA-256 `80232c50…3de4b`）、同一手机 `12f046690506`，复验 App→EMQX TLS 与 CONNACK，
  再继续受影响的端到端用例与 P1（P1 仍需测试身份与 DB 回读条件）。
- 交回后端/文档：`P0_backend_cert_ops_runbook.md` 中“按现状重载 listener”的描述应补注
  “本环境需 `systemctl restart emqx`”，以免下次仍按 listener restart 判断已生效。
