# E2E-JOINT-P0 平台段后端修复评估（EMQX 8883 服务器证书 SAN 不匹配）

- 负责角色/能力：后端工程（后端集成调试）
- 原始失败：`evidence/e2e_p0/P0_platform_stage_report.md` **TEST_FAIL / `MEASURED_DEFECT`**（平台段）
- 上游归因：`evidence/e2e_p0/P0_platform_failure_triage.md` **REPAIR_REQUIRED**（对象＝既有平台 EMQX 8883 listener 服务器证书/部署配置）
- 本轮复检时间窗：**2026-11-24T04:54Z**（本地 12:54 CST）
- 本轮 HEAD：`bee64c8`（工作区干净；本轮为该缺陷的首次后端修复调用，此前无服务器侧证书变更）
- 被测对象：既有平台服务 `8.140.23.253`（EMQX 5.8.0，8883 TLS）＋ Android 候选 APK AB-003（`com.jinyuni.dengbei_care` vC 10 / vN 1.9，SHA-256 `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b`）
- **本轮结论**：缺陷**仍然存在且未被任何已生效修复覆盖**；修复对象属本职责范围，但所需动作是**既有服务器上的证书替换与 EMQX 重载**，本轮**没有**该项变更授权与写入口，因此**不返回 FIX_SUBMITTED**，按环境/授权契约申请人工授权。

> 本文件是修复评估与实施计划，**不是修复完成、不是复验通过**，也不代表 P0/P1 或产品整体通过。
> 本轮**没有**登录服务器、没有重启/重载服务、没有写入任何文件或数据、没有连接数据库、没有修改 App/固件/网关源码或测试期望。

---

## 1. 缺陷仍在：与原始失败同版本的复检证据

| 观测项 | 2026-11-23 原始失败 | 2026-11-24 本轮复检 | 判定 |
|---|---|---|---|
| 目标 | `ssl://8.140.23.253:8883` | 同 | 同一对象 |
| 服务器证书 subject/CN | `CN=117.72.84.210` | `CN=117.72.84.210` | 未变 |
| SAN | 仅 `IP Address:117.72.84.210` | 仅 `IP Address:117.72.84.210` | **未变** |
| serial | 未记录 | `77ADCFD924BA0F87F845C81934D4A3F346D4BB82` | 本次基线 |
| SHA-256 指纹 | 未记录 | `48:76:E7:30:CC:AB:3E:35:4C:5B:38:19:B9:D7:0D:F1:10:63:51:91:6E:06:EE:EF:22:6A:37:7C:E3:54:AD:75` | 本次基线 |
| 有效期 | 至 2026-12-21 | 至 2026-12-21（未过期） | 未变 |
| 链校验（App 信任锚 `jd-ca.crt`） | 通过（失败发生在主机名校验） | 通过 | 一致 |
| 主机名校验（`-verify_hostname 8.140.23.253`） | App：`No subjectAltNames on the certificate match` | `Verify return code: 62 (hostname mismatch)` | **同一失败点，可复现** |

证据：`evidence/e2e_p0/backend_repair_cert_recheck.txt`（本轮只读 TLS 握手原始输出，含证书字段、`jd-ca.crt` 字段、`-verify_hostname` 返回码与公钥比对）。

**结论**：失败机制与上游归因一致——证书链可信、端口与传输层可用，唯一不满足的是**服务器证书未包含客户端连接的当前地址**。因此修复目标明确、可验证，不需要重新做归因。

---

## 2. 责任边界复核（为什么必须改服务器侧）

| 候选方案 | 可行性 | 依据 |
|---|---|---|
| 改 App 连接地址 | **不可行** | 旧地址 `117.72.84.210` 已退役（`服务器迁移记录.md`），回退会引入新的部署错误 |
| 换用公共 CA 证书即可 | **不可行** | App `setupSSL()` 用 `assets/jd-ca.crt` 单独构造 `TrustManagerFactory`，只信任 `Tiktok Root CA`；公共 CA 链无法通过 |
| 关闭 App 主机名校验 / `--insecure` | **禁止** | 降低安全校验、且不能作为产品修复或复验依据（与上游 triage §5.1“不采用”一致） |
| 服务端重签/替换 8883 服务器证书（保持 App 地址、端口、认证、信任锚不变） | **可行且最小** | 唯一需要变更的是证书 SAN/签发对象；客户端契约不变 |
| 新 CA + App 信任锚更新 | 备选（跨职责） | 会使 APK 身份变更（SHA-256/versionCode），需 Android 侧重建并重采手机段证据 |

**责任判定**：对象（EMQX listener 证书与部署配置）属平台后端职责，**不返回 ASSIGNMENT_DECLINED**；本轮受阻于**变更授权与写入口**，不是对象或专业边界错派。

---

## 3. 修复所需材料现状（本地有界核查，不代表服务器实况）

| 材料 | 本地核查结果 | 说明 |
|---|---|---|
| 部署中的服务器证书私钥 | **本地无匹配私钥** | 本地候选 `emqx_02.pem` 公钥 md5 `a372a12ed267cd936b6743c24e035096` ≠ 部署证书公钥 md5 `da7642e60102ef83b37c1b1d3384004a` |
| 签发 CA（`Tiktok Root CA`）私钥 | **未在已登记/资料所指位置找到** | 项目资料指向的本机目录不存在；另一候选目录为空。未做全盘搜索（规则 17） |
| 服务器证书/私钥实际部署路径 | **未确认** | 读取需服务器入口；本轮无该入口授权 |
| App 信任锚 `jd-ca.crt` | 可用（仓库内，`CN=Tiktok Root CA`，至 2034） | 若沿用方案 A 则**无需改动 App**，APK 身份不变 |

**含义**：方案 A 是否可行取决于服务器上 CA 私钥（或已签发好的新证书）是否可用；若不可用，则须改走方案 B（新 CA + App 信任锚 + 重建 APK），必须转 Android 侧协同并重开手机段证据。这一点只能由获授权的服务器访问确认，不能靠推断。

---

## 4. 受权后的最小实施步骤（本轮未执行）

1. **备份与可回退**：备份当前 8883 证书/私钥/EMQX listener 配置，记录 serial/指纹（本轮基线见 §1）；确认回退路径。
2. **签发**：以 `Tiktok Root CA` 签发包含 `SAN=IP:8.140.23.253`（如产品后续使用域名，再加 DNS 名）的服务器证书；沿用现用私钥或同时更新私钥，二者匹配并在同一 PEM（或 EMQX 配置的 certfile/keyfile 分置）。
3. **部署**：按实际部署方式替换证书文件并重载 listener（`emqx` 重载/必要的 listener 重配），**保持 8883/TLS、1883/8083/8084/18083 与 5000/3306 语义不变**；不清理 mnesia/数据库，不改认证库。
4. **服务器侧最小校验**：`openssl s_client -connect 8.140.23.253:8883 -CAfile jd-ca.crt -verify_hostname 8.140.23.253` → 期望 `Verify return code: 0 (ok)`；证书 SAN 含 `8.140.23.253`；`emqx ctl listeners` 显示 8883 running。
5. **原失败复验**（交回测试侧，同一用例/同一 App/同一手机）：App 对 `ssl://8.140.23.253:8883` 不再出现 `No subjectAltNames on the certificate match`，进入 MQTT CONNECT，并记录实际 CONNACK/认证结果（无测试身份时认证结果属另一层，不与 TLS 混判）。
6. **正式面回归**：确认没有为排障开放额外诊断入口、没有放宽鉴权边界、其他 listener 端口与 5000 路由行为未回归；如产生测试专用通道需可排除于正式部署。
7. **索引同步**：本次未改动被 CodeGraph 索引覆盖的源码（仅新增证据文件与文档），无需索引同步。

---

## 5. 本轮未执行/无法执行项（如实记录）

1. **服务器变更**（证书签发、替换、EMQX 重载）：无授权、无写入口（环境登记的管理/变更部署入口未确认；`existing_service_ssh_inspection` 对本能力 `unavailable_for_current_capability`）。
2. **服务器只读检查**（证书路径、CA 私钥、listener 配置）：同上，缺 `inspect_existing_service_readonly` 授权；本轮仅用公开可达端口做 TLS 握手观测。
3. **服务日志/连接数基线、DB 回读**：仍无入口/授权，保持 `NOT EXECUTED`（不因证书修复自动补齐）。
4. **P1 上传/持久化**：仍需隔离测试身份（`registerphone`/`mac_addr`）与平台回读授权；本轮不涉及。
5. **App/固件/网关源码**：未修改；无“为通过而降低校验”的改动。

---

## 6. 证据清单（本轮新增）

| 文件 | 内容 |
|---|---|
| `evidence/e2e_p0/backend_repair_cert_recheck.txt` | 只读 TLS 复检原始输出：服务器证书 subject/issuer/有效期/serial/指纹/SAN、`jd-ca.crt` 字段、`-verify_hostname` 返回码 62、本地候选私钥与部署证书公钥比对 |
| `evidence/e2e_p0/P0_backend_repair_scope.md` | 本文件：缺陷仍在的判定、责任边界复核、材料现状、受权后实施与复验判据、未执行项 |

引用（既有，不改写）：`P0_platform_stage_report.md`、`P0_platform_failure_triage.md`、`platform_stage_retest_cert.txt`、`platform_stage_retest_logcat_tls_fail.txt`、`服务器迁移记录.md`、`docs/05_平台服务器配置与联调前提.md`（§6 变更步骤需授权）。

---

## 8. 后续进展（2026-11-24T05:40Z 操作者答复后）

1. **操作者答复**：“允许维护本项目的证书，请先核发签发和回退条件，再按现有运维方式处理。”按该要求，**核发条件与回退条件已成文**：`evidence/e2e_p0/P0_backend_cert_issuance_and_rollback.md`（含 C1–C6 前置条件、签发模板与命令、部署与回退判据、正式面回归要求）。
2. **判据修订**：IP 形式主机必须用 `openssl verify -verify_ip 8.140.23.253` 与 SAN 文本共同判定；本地演练证实 `-verify_hostname` 会回退比较 CN，单凭其 `OK` 不能证明 IP SAN 正确（`backend_cert_csr_dryrun.txt` §7/§8）。§1 中原始失败使用 `-verify_hostname` 返回 62 仍有效（旧证书 CN 与 SAN 均不含目标地址）。
3. **本轮复检**：2026-11-24T05:46Z 只读握手确认证书 serial/指纹/SAN 与原始失败完全一致（`backend_cert_recheck_20261124_0546Z.txt`），缺陷未被任何已生效变更覆盖。
4. **仍未执行**：服务器登录、CA 私钥确认、真实签发、证书替换、listener 重载——本 CALL 未附加服务器变更入口，登记入口仅为只读检查；因此本轮**不构成修复**，不返回 `FIX_SUBMITTED`。
5. **下一步所需**：一次性服务器变更入口（或运维按 §4–§6 执行并回传取证）与 `Tiktok Root CA` 私钥可用性确认；二者缺失时按 `P0_backend_cert_issuance_and_rollback.md` §3 停止条件改走备选方案，不由本能力自行降级校验或自签替代。

---

## 9. 后续进展（2026-11-24T07:02Z 操作者答复后，本轮）

1. **操作者答复**：“由运维按照现有方案执行，完成后回传执行记录”。证书变更的执行责任在运维侧；本能力本轮不自行登录服务器，也不以声明代替执行记录。
2. **本轮只读复检（07:06Z）**：8883 叶子证书仍为 `CN=117.72.84.210`、`serial=77ADCFD924BA0F87F845C81934D4A3F346D4BB82`、`SAN=IP:117.72.84.210`；`openssl verify -verify_ip 8.140.23.253` 返回 **error 64 IP address mismatch**，即变更尚未生效（原始输出 `backend_cert_recheck_20261124_0705Z.txt`）。
3. **新增可执行材料（不构成修复）**：运维执行手册（`P0_backend_cert_ops_runbook.md`）、只读核验脚本（`scripts/emqx_8883_cert_verify.sh`，已用当前失败证书验证 `--expect-new` 判 FAIL、`--expect-baseline` 记录 known_failure）、运维回传模板（`emqx_8883_execution_record_template.md`）。
4. **仍未完成**：服务器侧签发/替换/重载与变更后服务器判据、服务日志与连接数基线、DB 回读、P1 上传/持久化；缺项不因新增材料而变为已执行。
5. **下一步**：运维回传记录后，核对线上证书与记录一致，再交回软件测试在同一用例、同一 App、同一手机复验 App→EMQX TLS/CONNACK；不得以降低校验或 `--insecure` 替代。
6. **索引**：本轮未改被 CodeGraph 索引覆盖的产品源码，无需索引同步。

---

## 10. 本节进展（2026-11-24T09:24Z 操作者答复后，本轮）

### 10.1 操作者答复与本轮判定

- 答复原文：“**证书没有替换，需要你自行替换**”（2026-11-24T09:24Z 收到，`permission_decision` 为空）。
- 含义确认：运维未执行变更；缺陷状态未变。**答复是“由本角色执行替换”的工作指示，不构成服务器访问授权**（本次调用未附一次性操作授权，登记入口均为只读或对本能力不可用）。
- **本轮结论：缺陷仍在、未修复，不返回 `FIX_SUBMITTED`。** 本轮把“拿到入口即可一次做完”的执行器做成并验证完毕，剩余唯一阻塞是**变更入口与 CA 私钥可用性**（§10.5）。

### 10.2 本轮只读复检（09:31Z，证明未被任何变更覆盖）

原始输出：`evidence/e2e_p0/backend_cert_recheck_20261124_0931Z.txt`

| 判据 | 实测 | 结论 |
|---|---|---|
| TLS 握手 | OK（收到叶子证书） | 端口/传输层正常 |
| 叶子 subject / issuer | `CN=117.72.84.210` / `CN=Tiktok Root CA, O=LinkValue` | 仍是旧证书，签发 CA 未变 |
| serial / SHA-256 | `77ADCFD924BA0F87F845C81934D4A3F346D4BB82` / `48:76:E7:30:…:AD:75` | 与 11-23 原始失败、11-24 05:46Z、07:06Z 三次基线**完全一致** |
| SAN | 仅 `IP:117.72.84.210` | `san_contains_target=NO` |
| 链校验（App 信任锚 `jd-ca.crt`） | OK | 失败点不是证书链 |
| `openssl verify -verify_ip 8.140.23.253` | **error 64 IP address mismatch** | 与 App 的 `No subjectAltNames on the certificate match` 同一失败点 |
| 负例 `-verify_ip 8.140.23.254` | 失败（预期） | 判据未被放宽 |

### 10.3 本轮产出：单入口、带硬门禁的替换执行器（新增，未对线上执行）

`scripts/emqx_8883_cert_replace.sh`（模式 `recon` / `issue` / `apply` / `rollback`），要点：

| 项 | 实现 |
|---|---|
| 只读侦察 `recon` | 服务/listener/证书路径探测、当前叶子字段、`verify_ip` 判定、CA 私钥候选探测、1883/8083/8084/18083/5000 连通性基线；**不写任何文件** |
| 拒绝自签替代 | 无 `--ca-key` 直接停止（退出码 2）；CA 私钥公钥与 `jd-ca.crt` 不匹配即停止，禁止用错误 CA 签发 |
| 签发自检硬门禁 | 新证书必须同时通过：信任锚链校验、`-verify_ip 8.140.23.253`、负例不通过、证书与私钥公钥一致、serial 与基线不同；任一不满足**不触碰线上文件** |
| 部署 | 备份旧 cert/key（`SHA256SUMS.txt` + 同目录 `.pre-20261124…` 副本）→ 记录并保留原文件属主/属组后安装（避免 EMQX 工作用户读不到私钥）→ 按现场命令重载 listener |
| 复核 | 本机 8883 TLS + `-verify_ip` + serial 变化 + 8883/1883/8083/8084/18083/5000 连通性 |
| 自动回退 | 复核任一不满足即恢复备份并重载，回到“已知失败”基线（退出码 3）；回退失败退出码 4 并显式要求人工介入 |
| 独立回退 | `rollback` 模式可从指定备份目录恢复并复核基线 serial |
| 秘密处理 | 私钥/CA 私钥不打印、不回传、不写入项目仓库；工作目录 `umask 077` |

### 10.4 本轮验证：本地演练（不涉及线上、不使用产品密钥）

原始输出：`evidence/e2e_p0/backend_cert_replace_drill_20261124.txt`（本地临时自建测试 CA 模拟“App 信任锚 + 线上旧证书”，演练后已删除测试密钥）

| 分支 | 期望 | 实测 |
|---|---|---|
| T1 缺 CA 私钥 | 拒绝、不写 | `STOP: issue/apply 需要 --ca-key…不得自签替代`，退出码 2 |
| T2 CA 私钥与信任锚不匹配 | 拒绝 | `STOP: CA 私钥与 jd-ca.crt 不匹配`，退出码 2 |
| T3 `issue` | 签发并自检（不安装） | `issue_chain_verify=OK`、`issue_ip_san_match=OK`、`issue_negative_control=OK`、`issue_key_matches_cert=OK` |
| T4 `apply` 正常路径 | 安装+重载+复核通过 | `RESULT overall=APPLIED`；安装后 `verify_ip 8.140.23.253 → OK` |
| T5 复核失败 | 自动回退到基线 | `RESULT overall=ROLLED_BACK`；回退后 serial 等于基线 serial |
| T6 `rollback` 独立可用 | 恢复备份并复核 | `RESULT overall=ROLLED_BACK`、`baseline_restored=YES` |
| T7 `recon` | 只读记录与缺项 | `RESULT overall=RECORDED`、`ca_key_candidates=none` 等如实输出 |

演练证明的是**执行器分支行为**（拒签、门禁、复核、回退），**不是**线上证书已替换，也不是 App 侧复验通过。

### 10.5 剩余阻塞（本轮未获得，不能自行扩权）

| # | 阻塞 | 具体需要 | 影响 |
|---|---|---|---|
| B1 | **服务器变更入口未开通** | 对平台服务器 `8.140.23.253` 的一次性 root 变更授权（仅：备份、替换 8883 certfile/keyfile、按现场方式重载 listener、失败回退）；本次调用授权集只含浏览器测试动作，登记 SSH 入口对本能力 `unavailable_for_current_capability`，一次性操作授权为空 | 无法登录、无法替换 → 无法产生真实修复 |
| B2 | **签发 CA 私钥可用性未确认** | `Tiktok Root CA` 私钥需在服务器受控路径可用（可由 `recon` 只读探测确认）；本地无该私钥，且不得索取/传输私钥 | 不可用则**不能**由该 CA 签发，App 也不会信任自签替代 → 须改走“新 CA + App 信任锚更新 + APK 重建”（跨 Android/发布范围，需重新规划） |
| B3 | 服务日志/连接数基线、DB 回读 | 只读入口授权 | 与证书缺陷无关的独立缺口，保持 `NOT EXECUTED` |
| B4 | P1 上传/持久化 | 隔离测试身份与平台回读授权 | 保持 `NOT EXECUTED`；证书修复不会自动补齐 |

### 10.6 拿到 B1 后的一次性执行顺序（已固化）

1. `recon`（只读）：确认 certfile/keyfile 路径、现场重载命令、CA 私钥可用性、其他端口基线；任一缺失即停止并回传缺口（不得猜路径、不得自签）。
2. `issue`：在服务器受控目录签发并自检；不满足门禁即停止。
3. `apply`：备份 → 安装 → 重载 → 复核；复核失败自动回退。
4. 取证：保存 `RESULT` 行齐全的原始输出、新证书 serial/SHA-256/SAN、listener 与其他端口观测、回退记录（若发生）。
5. 交回：软件测试在同一用例/同一 App（APK 摘要不变）/同一手机上复验 App→EMQX TLS 与 CONNACK；本能力不代判客户端通过。
6. 正式面回归：不新增诊断/免校验入口，其他 listener 与 Flask 5000 行为不变，认证库与数据库未改动。

### 10.7 索引

本轮改动仅新增 shell 脚本与证据/文档（`scripts/`、`evidence/e2e_p0/`），未改动被 CodeGraph 索引覆盖的产品源码（`dengbei_care/`、固件、网关），因此无需索引同步，也未执行同步。

---

## 11. 本节进展（2026-11-24T10:19Z 权限审查答复后，本轮）

### 11.1 操作者答复与本轮判定

- 答复原文：“**审查已完成，请用当前 Agent CLI 已有权限继续原任务。如仍缺少访问权限或现场条件，请报告具体阻塞。**”（2026-11-24T10:13Z 收到，`permission_decision=CONTINUE`，`scope=existing_permissions_only`，`operation_ref=null`）。
- 含义确认：审查**未新增**任何一次性操作授权，要求继续使用本调用已有权限；若仍缺入口则**如实报告具体阻塞**。已有权限集未包含服务器变更或只读入口（见 §11.4）。
- **本轮结论：缺陷仍在、未被任何已生效修复覆盖；本能力在现有授权内无法产生真实修复，不返回 `FIX_SUBMITTED`，按契约交回重新规划作范围/授权决定。**

### 11.2 本轮只读复检（10:19:59Z，证明未被任何变更覆盖）

原始输出：`evidence/e2e_p0/backend_cert_recheck_20261124_1015Z.txt`（`scripts/emqx_8883_cert_verify.sh --expect-baseline`）

| 判据 | 实测 | 结论 |
|---|---|---|
| TLS 握手 | OK（收到叶子证书） | 端口/传输层仍正常，网络与 8883 放行不是问题 |
| subject / issuer | `CN=117.72.84.210` / `CN=Tiktok Root CA, O=LinkValue` | 仍是旧证书，签发 CA 未变 |
| serial / SHA-256 | `77ADCFD924BA0F87F845C81934D4A3F346D4BB82` / `48:76:E7:30:…:AD:75` | 与 11-23 原始失败及 11-24 三次基线**完全一致** |
| 有效期 | 2024-10-12 – 2026-12-21（未过期） | 过期不是原因 |
| SAN | 仅 `IP Address:117.72.84.210` | `san_contains_target=NO` |
| 链校验（App 信任锚 `jd-ca.crt`） | `OK` | 失败点不是证书链或信任锚 |
| `openssl verify -verify_ip 8.140.23.253` | `error 64 IP address mismatch` | 与 App 的 `No subjectAltNames on the certificate match` 同一失败点 |
| 负例 `-verify_ip 8.140.23.254` | 不通过（预期） | 判据未被放宽 |
| `RESULT overall` | `RECORDED`（`baseline_state=known_failure`） | 缺陷状态未变 |

### 11.3 已完成的修复准备工作（可复用，均不构成线上修复）

| 产出 | 状态 | 说明 |
|---|---|---|
| 根因与责任边界 | 已定 | 平台侧 EMQX 8883 服务器证书 SAN 未随地址迁移更新；非 App/手机/网关/固件缺陷（引用 `P0_platform_failure_triage.md`） |
| 核发与回退条件 | 已定稿 | `P0_backend_cert_issuance_and_rollback.md`（C1–C6 前置、SAN/扩展规范、R1–R4 回退触发与判据） |
| 运维执行手册 | 已定稿 | `P0_backend_cert_ops_runbook.md` |
| 单入口执行器 | 已实现并本地演练 | `scripts/emqx_8883_cert_replace.sh`（`recon`/`issue`/`apply`/`rollback`；拒签、硬门禁、备份、自动回退；T1–T7 分支见 `backend_cert_replace_drill_20261124.txt`） |
| 只读验收判据 | 已实现并用于本轮 | `scripts/emqx_8883_cert_verify.sh`（`--expect-new` / `--expect-baseline`，含负例对照） |
| 回传模板 | 已定稿 | `emqx_8883_execution_record_template.md` |

即：**修复方案、判据、执行器与回退路径均已就绪；缺的是执行该变更所需的服务器入口与该 CA 私钥的可用性。**

### 11.4 具体阻塞（本轮如实报告，无法由本能力自行解除）

| # | 阻塞项 | 客观事实 | 为什么本能力无法自行解除 |
|---|---|---|---|
| B1 | **服务器变更入口未授权** | 本调用 `allowed_external_actions` 仅 `operate_test_browser`；`existing_service_ssh_inspection` 对当前能力 `unavailable_for_current_capability`（缺 `inspect_existing_service_readonly`）；`current_call_operation_grant=null` | 证书替换等于对既有生产服务写文件并重载 listener，属外部变更动作；登记入口未对本能力开放，自行改用 SSH 会绕过环境文件的入口授权约束 |
| B2 | **签发 CA 私钥可用性未确认** | 本地未找到 `Tiktok Root CA` 私钥（§3）；私钥只能存在于服务器受控路径，需 B1 才能只读确认 | 无 CA 私钥即无法签发被 App `jd-ca.crt` 信任的新证书；自签或换公共 CA 都会被 App 拒绝，属禁止的“降低校验/伪造替代” |
| B3 | 服务日志/连接数只读基线、DB 回读 | 仍 `NOT EXECUTED` | 同为入口/授权缺口，与证书缺陷独立；不因证书修复自动补齐 |
| B4 | P1 上传/持久化 | 仍 `NOT EXECUTED`（无隔离测试身份 `registerphone`/`mac_addr`） | 独立缺口，不由本能力伪造 |

**关键判断**：B2 使“沿用现有信任锚、仅换证书”的方案在本能力可及范围内不可执行；而 B2 不可用时的唯一替代是 **方案 B：新 CA + App 信任锚 `jd-ca.crt` 更新 + APK 重建**——这会改变 APK 身份（SHA-256/versionCode），跨 Android 实现与发布范围，并需重采手机段证据。**该选择不是后端集成调试可以单方面决定的。**

### 11.5 请规划/审批环节作的范围决定（二选一，均不由本能力自行决定）

- **方案 A（推荐，最小变更）**：授予对 `8.140.23.253` 的一次性变更入口，范围仅限：只读侦察证书/私钥/CA 私钥路径与现场重载方式、备份、替换 8883 certfile/keyfile、按现场方式重载 listener、失败回退。授权后按 §10.6 一次执行并回传 `RESULT` 齐全的原始输出；APK 与手机段证据不变。
- **方案 B（方案 A 不可行时）**：批准更换信任锚（新 CA 签发含 `SAN=IP:8.140.23.253` 的服务器证书 + 更新 `dengbei_care/app/src/main/assets/jd-ca.crt` + 重建 APK）。需重新规划为 Android 实现 + 发布 + 手机段证据重采，P0 手机段已通过结论随之作废。

### 11.6 本轮边界与未执行声明

- 未登录服务器（SSH、EMQX Dashboard、Flask、DB），未读取或复制任何凭据，未做任何写操作，未重启或重载服务。
- 仅执行：本地只读 TLS 握手 + 本地 `openssl` 证书校验（10:19:59Z）。
- 未修改 App/固件/网关/平台源码，未改测试期望，未改 `permissions.yaml` 或 `environment.yaml`。
- 未执行项（B1–B4）一律不等于通过；本文件不是复验通过、不代表 P0/P1 或产品整体通过。

### 11.7 索引

本轮仅新增一份只读复检原始输出与本文档更新，未改动被 CodeGraph 索引覆盖的产品源码（`dengbei_care/`、固件、网关），因此无需索引同步，也未执行同步。
