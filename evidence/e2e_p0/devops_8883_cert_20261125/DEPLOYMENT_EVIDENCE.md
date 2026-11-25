# 8883 证书部署执行与复验记录（DeploymentEvidence）

- 负责角色：运维工程
- 能力：部署操作
- 时间：2026-11-25T03:16Z ~ 03:18Z（UTC）
- 目标：既有平台 `8.140.23.253`，EMQX 5.8.0，TLS listener `ssl:default`（0.0.0.0:8883）
- 上游依据：
  - `deploy/emqx_8883_tls/container.yaml`（已批准的基础设施交付清单，本次部署单元）
  - `evidence/e2e_p0/devops_8883_cert_20261125/CHANGE_RECORD.md`（变更记录与回退点）
  - `evidence/e2e_p0/P0_platform_stage_report.md`（TEST_FAIL / MEASURED_DEFECT）
  - 操作者 2026-11-24 授权：允许维护本项目证书，先核发签发与回退条件
- 结论：**当前线上部署即批准候选（证书 serial `…B83`），无漂移，服务健康，可复验**。
  本轮未重复下发安装：线上证书文件摘要与批准记录逐字节一致，重复安装只会再重启一次 EMQX
  且不改变任何字节，故以“复核部署状态”完成本节点。客户端上传复测不在本能力范围。

---

## 1. 部署单元与版本标识

| 项 | 值 |
|---|---|
| 部署对象 | 既有平台服务（EMQX broker + 注册服务 + MySQL），本轮仅 8883 TLS 证书 |
| 服务版本 | EMQX 5.8.0（`emqx 5.8.0 install ok installed`，node `emqx@127.0.0.1`） |
| 部署单元版本 | TLS 证书交付清单 v1（`container.yaml`） |
| listener | `ssl:default` 0.0.0.0:8883，`running: true` |
| 证书 subject / CN | `CN=8.140.23.253` |
| issuer | `CN=Tiktok Root CA, O=LinkValue`（与 App 信任锚同一 CA） |
| serial | `77ADCFD924BA0F87F845C81934D4A3F346D4BB83` |
| SHA-256 指纹 | `E7:AD:1E:46:…:54:BD:88:84` |
| 有效期 | 2026-11-25T02:55:16Z → 2028-11-24T02:55:16Z |
| SAN | `IP:8.140.23.253`、`IP:117.72.84.210` |
| 证书文件 | `/var/lib/emqx/certs/listeners/ssl/default/cert-2D2ADC29A9BE14AA`，`644 emqx:emqx`，sha256 `27964ca8…b62ccaa` |
| 私钥文件 | `/var/lib/emqx/certs/listeners/ssl/default/key-6E6F6ED56285A948`，`600 emqx:emqx`，与证书公钥配对 |
| CA 文件 | `/var/lib/emqx/certs/listeners/ssl/default/cacert-613B9023E1E698F0`，sha256 `db910d3c…f213200`（与变更前一致） |
| 配置引用 | `/var/lib/emqx/configs/cluster.hocon` 的 `listeners.ssl.default.ssl_options`（mtime 2025-10-26，本次未改） |
| 部署入口 | 服务器侧 `deploy/emqx_8883_tls/apply_remote.sh`（本次未重跑）；外部核验 `scripts/emqx_8883_cert_verify.sh` |
| 回退点 | `/root/emqx-8883-cert-backup-20261125T025805Z/`（`700 root:root`；`leaf.crt.bak` = 基线 sha256 `a91de074…0855f9`） |

变更前后对照见 `CHANGE_RECORD.md` 第 1 节：subject/CN 由 `117.72.84.210` 改为 `8.140.23.253`，
SAN 增加 `IP:8.140.23.253`，serial 递增，CA 与 App 信任锚均未更换。

## 2. 本轮执行的核对（只读，不改动线上）

| 步骤 | 动作 | 结果 | 证据 |
|---|---|---|---|
| 1 | SSH 只读读取 systemd 单元、EMQX 版本/uptime/listener、证书与私钥身份/权限/摘要、备份目录、cluster.hocon 引用、端口与 HTTP、变更窗口日志 | 全部符合批准记录：`deployed_cert_matches_approved=YES`、`served_serial_matches_deployed=YES`、`ca_digest_unchanged=YES`、`backup_contains_baseline_cert=YES`、`keypair_match=YES`、`ssl_listener_running=YES`、`overall=DEPLOYED_VERIFIED` | `08_deploy_state.txt`（由 `deploy_state_probe.py` 生成，可复跑） |
| 2 | 从外部主机对 `8.140.23.253:8883` 做握手 + 链 + SAN + `-verify_ip` + 负例 | `chain_verify=OK`、`ip_san_match=OK`、`negative_control=OK`、`serial_changed=YES`、`overall=PASS` | `09_deploy_verify_from_host.txt` |
| 3 | MQTT 层探测：开启主机名校验、使用故意无效凭据（不发业务数据） | TLS 1.3 握手成功、`peer_cn=8.140.23.253`、`CONNACK_RC=5`；错误主机名被拒（`negative_hostname_control=OK`），证明校验真实开启 | `10_mqtt_hostname_probe.txt`（由 `mqtt_hostname_probe.py` 生成） |
| 4 | 服务与端口回归 | `emqx active/enabled`、`user_script active/enabled`、`NRestarts=0`、启动于 10:58:15 CST；1883/8883/8083/8084/18083/5000/3306 全 OPEN；Flask `/login` GET 405、Dashboard `/status` 200（与基线一致） | `08_deploy_state.txt` |

补充（历史验证，同一部署版本）：`03_verify_from_host.txt`、`05_verify_after_cleanup.txt`、
`07_post_change_state.txt` 均为 `PASS/APPLIED`；`06_mqtt_layer_probe.txt` 为 MQTT 层存活首证。

## 3. 部署前提与授权核对

- 批准产物存在且与线上一致：`container.yaml`（v1）声明的证书路径、serial、摘要与线上文件一致。
- 配置面零变更：仍由原路径引用证书，`cluster.hocon` 未被修改（mtime 早于本次变更）。
- 私钥最小暴露：仅服务器 `600 emqx:emqx` 一份 + 回退备份 `600`；工作目录暂存私钥已清除。
- 回退方案可用：备份目录存在、`700`，基线证书摘要与变更前记录一致；回退脚本
  `deploy/emqx_8883_tls/rollback_remote.sh` 与命令见 `CHANGE_RECORD.md` 第 8 节。
  回退后回到**已知失败**状态（serial `…B82`，`verify_ip 8.140.23.253` 失败），不得计为通过。
- 未执行数据库直连、未调用写入接口、未改动认证库与其他 listener、未重装/重启无关服务
  （EMQX 上次重启是本次变更窗口内的一次，之后 `NRestarts=0`、uptime 连续）。

## 4. 未执行 / 不在本能力范围

- Android App 使用同一 APK、同一手机复测 App→`ssl://8.140.23.253:8883` 的 TLS/CONNACK/上传：
  由软件测试能力执行；本机仅证明到 MQTT CONNACK 层（无效凭据 rc=5）。
- `/upload_data` 真实落库与 MySQL `sensor_data` 回读（需测试身份与 DB 授权）。
- EMQX 认证库/Dashboard 凭据侧操作、连接数长期基线、证书自动轮换。
- 证书更换后的客户端信任锚影响：**无需更换**，App `assets/jd-ca.crt` 未变，链校验通过
  （`ca_signing_capability.txt`：原 CA 可签发，服务器无 CA 私钥，操作主机私钥与信任锚配对）。

## 5. 复验命令（可在任意具备 openssl 的主机重跑）

```bash
# 外部判据（只读）
bash scripts/emqx_8883_cert_verify.sh 8.140.23.253 8883 \
     dengbei_care/app/src/main/assets/jd-ca.crt --expect-new

# 服务器只读状态核对（需已授权 SSH 访问，脚本从项目资料读取连接信息，不回显口令）
C:/Users/kason/anaconda3/python.exe evidence/e2e_p0/devops_8883_cert_20261125/deploy_state_probe.py

# MQTT 层主机名校验探测（故意无效凭据，不写业务数据）
C:/Users/kason/anaconda3/python.exe evidence/e2e_p0/devops_8883_cert_20261125/mqtt_hostname_probe.py
```

预期：`overall=PASS`、`overall=DEPLOYED_VERIFIED`、`CONNACK_RC=5 / overall=PASS`。

## 6. 交付判定

| 判据 | 结果 |
|---|---|
| 8883 listener 提供批准候选证书（serial/摘要一致） | PASS |
| SAN 命中客户端实际连接地址 `8.140.23.253`，App 信任锚可验证整链 | PASS |
| 负例（错误 IP / 错误主机名）仍失败，判据未被放宽 | PASS |
| 证书与私钥配对、私钥权限 600、备份与回退点完整 | PASS |
| 其他 listener / Flask / MySQL 端口与 HTTP 无回归 | PASS |
| Android 实际上传复测 | **NOT EXECUTED**（交软件测试能力） |

**部署结果：当前候选已在线且可复验（EXECUTED）。** 该结论只覆盖基础设施部署与服务器/协议层判据，
不代表 Android→平台业务上传通过；后续由软件测试在同一 APK、同一手机上复测。
