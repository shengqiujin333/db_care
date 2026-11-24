# E2E-JOINT-P0 平台段 8883 证书变更：运维执行手册（后端工程）

- 负责角色：后端工程（后端集成调试）
- 原始失败：`evidence/e2e_p0/P0_platform_stage_report.md` **TEST_FAIL / MEASURED_DEFECT**
- 归因：`evidence/e2e_p0/P0_platform_failure_triage.md` **REPAIR_REQUIRED**（EMQX 8883 叶子证书 SAN 仅 `117.72.84.210`，与客户端目标 `8.140.23.253` 不匹配）
- 核发/回退条件：`evidence/e2e_p0/P0_backend_cert_issuance_and_rollback.md`
- 签发模板：`evidence/e2e_p0/emqx_8883_san.cnf`
- 本文用途：给执行者（运维）一份**可直接照做**的步骤，并把执行结果按 `evidence/e2e_p0/emqx_8883_execution_record_template.md` 回传；**本文不是修复完成证据**。
- 当前状态（2026-11-24T07:06Z 只读复检）：叶子证书 `CN=117.72.84.210`、`serial=77ADCFD924BA0F87F845C81934D4A3F346D4BB82`、`SHA-256=48:76:E7:30:…:AD:75`、`SAN=IP:117.72.84.210`，`verify_ip 8.140.23.253` **失败（error 64 IP address mismatch）**。原始证据 `evidence/e2e_p0/backend_cert_recheck_20261124_0705Z.txt`。
- **2026-11-24T09:31Z 再复检：serial/SAN/指纹仍与上句完全一致**（`evidence/e2e_p0/backend_cert_recheck_20261124_0931Z.txt`）；操作者 09:24Z 答复“证书没有替换，需要你自行替换”，但服务器变更入口尚未开通，本轮未对线上执行。
- **已固化的单入口执行器**：`scripts/emqx_8883_cert_replace.sh`（`recon` / `issue` / `apply` / `rollback`）。它把本文 §1–§6 的只读侦察、备份、签发硬门禁（无 CA 私钥或 CA 与 `jd-ca.crt` 不匹配即拒签）、安装重载、TLS + `verify_ip` 复核、复核失败自动回退串成一次可复验的执行；本地演练已覆盖拒签/自检/安装/回退四类分支（`evidence/e2e_p0/backend_cert_replace_drill_20261124.txt`）。手工命令仍保留在 §1–§6，供需要逐步取证时使用。

## 0. 唯一允许的变更与禁止项

| 允许 | 禁止 |
|---|---|
| 替换 8883 listener 使用的叶子证书 + 匹配私钥（由 `CN=Tiktok Root CA` 签发，SAN 含 `IP:8.140.23.253`），按现场方式重载 listener | 改 App 地址/端口/信任锚；关闭客户端主机名校验；用 `--insecure` 结果充当复验通过 |
| 变更前备份现用证书/私钥/listener 配置 | 自签“替代 CA”、换公共 CA（App 不信任）、回退 App 到旧 IP `117.72.84.210` |
| 变更后在服务器侧做只读核验 | 改 MQTT 认证库/口令、改其他 listener 语义、改 Flask 路由、清空 mnesia 或数据库 |
| — | 为排障新增公网免校验/诊断端口；把私钥或 CA 私钥复制到项目仓库、证据或聊天记录 |

## 1. 执行前只读确认（记录输出，不改动）

```bash
# 服务与版本
systemctl status emqx --no-pager | sed -n '1,12p'
emqx ctl status || true

# listener 与证书路径（以现场实际输出为准，不猜测路径）
emqx ctl listeners | sed -n '1,40p'
emqx ctl conf show listeners.ssl.default | sed -n '1,40p'

# 现用证书/私钥文件属性与摘要（路径取自上一步输出）
ls -l <certfile> <keyfile>
sha256sum <certfile> <keyfile>
openssl x509 -in <certfile> -noout -subject -issuer -serial -dates -ext subjectAltName

# 变更前基线（在任意有 openssl 的主机执行，记录 serial 与 known_failure 结论）
bash scripts/emqx_8883_cert_verify.sh 8.140.23.253 8883 dengbei_care/app/src/main/assets/jd-ca.crt --expect-baseline
```

判定：若 `<certfile>` 的 serial 仍为 `77ADCFD924BA0F87F845C81934D4A3F346D4BB82`，说明变更尚未发生，继续下一步；若已不同，先核对是否他人已执行并取回其记录。

## 2. 备份（必须先于任何写入）

```bash
umask 077
BK=/root/emqx-8883-cert-backup-$(date +%Y%m%d%H%M)
install -d "$BK"
cp -a <certfile> "$BK/leaf.crt.bak"
cp -a <keyfile>  "$BK/leaf.key.bak"
# 如本次需要改 listener 配置，同时备份该配置文件
systemctl cat emqx > "$BK/emqx.unit.txt" 2>/dev/null || true
sha256sum "$BK"/* | tee "$BK/SHA256SUMS.txt"
```

记录：备份目录、文件摘要、`emqx` 版本、`emqx ctl listeners` 原始输出、变更前 `uptime`/`current_conn`。私钥备份不得离开服务器受控目录。

## 3. 签发新叶子证书（在受控目录，`umask 077`）

模板：`evidence/e2e_p0/emqx_8883_san.cnf`（SAN 含 `IP:8.140.23.253`，保留 `IP:117.72.84.210` 仅作资料兼容；`CA:FALSE`、`serverAuth`）。

```bash
umask 077
W=/root/emqx-8883-cert-work-$(date +%Y%m%d%H%M); install -d "$W"; cd "$W"

# 1) 新私钥 + CSR（CN 沿用既有风格，判据是 SAN）
openssl req -new -newkey rsa:2048 -nodes \
  -keyout emqx_8883.key -out emqx_8883.csr -config emqx_8883_san.cnf

# 2) 由 App 信任锚签发（CA 私钥仅在本命令期间本地可见）
openssl x509 -req -in emqx_8883.csr \
  -CA jd-ca.crt -CAkey <tiktok_root_ca.key> -CAcreateserial \
  -days 730 -sha256 \
  -extfile emqx_8883_san.cnf -extensions v3_server \
  -out emqx_8883.pem

# 3) 部署前自检：任一不满足即停止，不得部署
openssl x509 -in emqx_8883.pem -noout -subject -issuer -serial -fingerprint -sha256 \
  -dates -ext subjectAltName,extendedKeyUsage
openssl verify -CAfile jd-ca.crt -verify_ip 8.140.23.253 emqx_8883.pem   # 期望 OK
openssl verify -CAfile jd-ca.crt -verify_ip 8.140.23.254 emqx_8883.pem   # 期望失败（反例）
openssl x509 -in jd-ca.crt -noout -fingerprint -sha256                   # 期望仍为 75:33:09:B3:…:15:2B
```

签发记录必填：新证书 serial、SHA-256、`notBefore/notAfter`、SAN 原文、签发时间、执行者、`jd-ca.crt` 指纹。

**停止条件（C1 不满足）**：服务器上没有可用的 `Tiktok Root CA` 私钥，也没有已签好且 SAN 正确的新证书 → **不要**自签、不要自建 CA、不要改 App；按模板 §回传记录“阻塞”并交回规划/审批（备选方案会更换 CA/信任锚并重建 APK，改变 APK 身份）。

## 4. 部署与重载

```bash
# 归档旧文件后再替换（备份已在 §2 完成）
cp -a <certfile> <certfile>.pre-<YYYYmmddHHMM>
install -m 644 -o <owner> -g <group> emqx_8883.pem <certfile>
install -m 600 -o <owner> -g <group> emqx_8883.key <keyfile>

# 重载方式按现场实际部署确认（热重载优先；确需重启则记录 uptime 变化）
<EMQX 现场重载命令，例如 emqx ctl conf reload 或 systemctl reload/restart emqx>
sleep 3
emqx ctl listeners | sed -n '1,40p'
```

要求：8883 仍为 TLS listener 且 running；1883/8083/8084/18083 与 Flask 5000、MySQL 3306 行为不变；不改认证库、不新增 listener。若同一 PEM 同时含证书链，确保顺序为“叶子→CA”，且私钥与证书匹配（`openssl x509 -noout -pubkey` 与 `openssl pkey -pubout` 一致）。

## 5. 变更后判据（当场取证）

```bash
bash scripts/emqx_8883_cert_verify.sh 8.140.23.253 8883 dengbei_care/app/src/main/assets/jd-ca.crt --expect-new
```

必须同时满足（脚本会逐项输出 `RESULT`）：

1. `tls_handshake=OK`；
2. `chain_verify=OK`（App 信任锚仍为 `Tiktok Root CA`）；
3. `san_contains_target=YES`；
4. `ip_san_match=OK`（`-verify_ip 8.140.23.253` 返回 0；这是 IP 端点的权威判据，`-verify_hostname` 会回退比较 CN，不得单独使用）；
5. `negative_control=OK`（对 `8.140.23.254` 仍失败，证明判据未被放宽）；
6. 叶证书 `serial` 与变更前不同，且与签发记录一致；
7. `emqx ctl listeners` 中 8883 running；其他端口/服务响应无回归。

附：证书链与密钥匹配

```bash
openssl x509 -in <certfile> -noout -pubkey | openssl sha256
openssl pkey -in <keyfile> -pubout | openssl sha256
```

## 6. 回退（触发条件与步骤）

**触发**（变更后 10 分钟内出现任一）：R1 `ip_san_match` 不为 OK 或 SAN 文本不含目标 IP；R2 8883 TLS 握手失败/listener 未 running；R3 其他 listener 或 Flask 5000 回归；R4 现有客户端被非预期拒绝且窗口内无法解释。

```bash
cp -a "$BK/leaf.crt.bak" <certfile>
cp -a "$BK/leaf.key.bak"  <keyfile>
<同一重载命令>
bash scripts/emqx_8883_cert_verify.sh 8.140.23.253 8883 dengbei_care/app/src/main/assets/jd-ca.crt --expect-baseline
```

回退成功判据：取回 `serial=77ADCFD924BA0F87F845C81934D4A3F346D4BB82`、`baseline_state=known_failure`，其他端口/5000 恢复；记录回退时间、原因、执行者。回退后仍是**已知失败状态**，不得写成通过。

## 7. 回传内容（执行记录）

按 `evidence/e2e_p0/emqx_8883_execution_record_template.md` 填写，并附原始输出（可粘贴文本或服务器受控路径清单）：

- 变更时间窗（本地/UTC）、执行者、维护窗口与回退决策人；
- 只读确认输出：listener 配置、certfile/keyfile 路径、变更前 serial/指纹；
- 备份目录与摘要；
- 签发记录（serial/SHA-256/SAN/有效期/CA 指纹；**不含私钥**）；
- 部署与重载命令、`uptime`/`current_conn` 变化；
- §5 脚本完整输出（`RESULT` 行齐全）；
- 其他端口/5000 回归观测；
- 是否触发回退及原因。

回传后由后端工程核对记录与线上实际证书一致，再交回软件测试在**同一用例、同一 App、同一手机**上复验 App→EMQX TLS/CONNACK；客户端复验不由本步代判。

## 8. 边界与已知缺口

- 本文不改变任何契约：App 目标 `ssl://8.140.23.253:8883`、信任锚 `jd-ca.crt`、MQTT 认证格式、其他 listener 与数据语义保持不变。
- 服务日志/连接数基线、DB 回读、P1 上传/持久化仍是独立缺口，不因证书变更自动补齐。
- 若 CA 私钥不可用，本次修复不能以“自签”或“关闭校验”替代；需回到审批/规划决定是否走“新 CA + App 信任锚更新 + APK 重建”的路径。
