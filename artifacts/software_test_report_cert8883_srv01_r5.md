# 平台 8883 叶子证书独立核验报告（CERT8883-SRV-01，第 5 轮）

- 负责角色：软件测试
- 能力：软件验证
- 执行时间窗（UTC）：2026-11-26T03:09:59Z ~ 2026-11-26T03:10:51Z（CST 11:09 ~ 11:10）
- 用例：`CERT8883-SRV-01`（TD-CERT8883-001 §3.1，`case_kind=certificate_server_verification`，阶段 `platform_host_readonly`）
- 被测对象：既有平台 `8.140.23.253:8883` 当前实际返回的叶子证书（EMQX 5.8.0，TLS listener `ssl:default`）
- 被测版本身份：非代码构建产物，以线上证书不可变身份标识 —— serial `77ADCFD924BA0F87F845C81934D4A3F346D4BB83`，SHA-256 `E7:AD:1E:46:FA:5B:5D:06:02:C5:AD:04:B7:8B:1C:AF:2E:78:EE:47:8A:9F:6A:79:EA:B6:3B:D5:54:BD:88:84`
- 结果：**TEST_PASS（仅限本用例范围）**
- 方式边界：只读 TCP/TLS 握手 + 本地 `openssl` 校验；未登录、未写、未使用 `--insecure`、未改本地信任库；判定不依赖部署方自述

---

## 1. 结论

在 2026-11-26T03:10Z 的实际握手中，平台 `8.140.23.253:8883` 返回的叶子证书：

1. 由 App 当前信任锚 `dengbei_care/app/src/main/assets/jd-ca.crt`（`CN=Tiktok Root CA, O=LinkValue`）签发，仅以该 CA 校验链为 `OK`；
2. `SAN` 原文包含 `IP Address:8.140.23.253`，`openssl verify -verify_ip 8.140.23.253` 返回 0；
3. 负例 `-verify_ip 8.140.23.254` 不通过（`error 64 IP address mismatch`），判据未被放宽；
4. serial 已从缺陷基线 `77ADCFD924BA0F87F845C81934D4A3F346D4BB82` 变更为 `…B83`，且与批准签发/部署记录逐项一致；
5. `notAfter=2028-11-24T02:55:16Z`，晚于执行时刻且早于 CA 的 2034-10-10。

**App 信任锚未变更**（SHA-256 仍为 `75:33:09:B3:…:15:2B`），因此 Project Goal 中“若确实需要更换 App 信任锚，由我决定”的分支**未被触发**，本轮无需用户决策。

## 2. 执行前身份（acceptance 第 1 条）

| 项 | 实测 |
|---|---|
| UTC 时间 | 2026-11-26T03:09:59Z |
| 主机 | `kason`（MINGW64_NT-10.0-26120 x86_64，Windows 11，AMD64） |
| `openssl version` | OpenSSL 3.1.2 1 Aug 2023（mingw64） |
| `jd-ca.crt` X.509 SHA-256 | `75:33:09:B3:9F:1B:8F:85:B4:D7:57:76:BF:D3:8A:42:52:0D:C1:A0:5E:A2:0B:A3:11:7E:B3:9C:BA:B2:15:2B` |
| 与基线比较 | **完全一致** → 不进入信任锚变更分支（§9），继续判定 |
| `jd-ca.crt` 文件 SHA-256 | `5c17cf740ffdf416c2f31f90172ad3565966cd62c0952920b9b2ac974aefe1e4` |
| CA 有效期 | 2024-10-12T16:05:01Z → 2034-10-10T16:05:01Z |

## 3. 判据与实测（全部满足）

| # | acceptance 判据 | 实测 | 结果 |
|---|---|---|---|
| 1 | `tls_handshake=OK` | `RESULT tls_handshake=OK`（2026-11-26T03:10:03Z） | PASS |
| 2 | `chain_verify=OK`（仅以 `jd-ca.crt` 为 CA） | `openssl verify -CAfile jd-ca.crt leaf.pem` → `OK`（rc=0）；`RESULT chain_verify=OK` | PASS |
| 3 | 叶子 issuer 为 `CN=Tiktok Root CA, O=LinkValue` | `issuer=…, O = LinkValue, OU = Engineering, CN = Tiktok Root CA` | PASS |
| 4 | SAN 含 `IP Address:8.140.23.253`；`san_contains_target=YES` | `IP Address:8.140.23.253, IP Address:117.72.84.210`；`RESULT san_contains_target=YES` | PASS |
| 5 | `ip_san_match=OK`（`-verify_ip 8.140.23.253` 返回 0） | `OK`，rc=0；`RESULT ip_san_match=OK`（未使用 `-verify_hostname`/CN 代替） | PASS |
| 6 | `negative_control=OK`（`-verify_ip 8.140.23.254` 不通过） | `error 64 at 0 depth lookup: IP address mismatch`，`verification failed`，rc=2；`RESULT negative_control=OK` | PASS |
| 7 | `serial_changed=YES` | 线上 `…B83` ≠ 缺陷基线 `…B82`；`RESULT serial_changed=YES` | PASS |
| 8 | 线上 serial 与部署/签发记录一致 | `…B83`、SHA-256 `E7:AD:1E:46:…:54:BD:88:84`、SAN 两项、notAfter `2028-11-24T02:55:16Z` 与 `deploy/emqx_8883_tls/container.yaml` expected 段及部署记录逐项一致 | PASS |
| 9 | `notAfter` 晚于执行时刻且不晚于 CA 的 2034-10-10 | `notAfter=Nov 24 02:55:16 2028 GMT` | PASS |
| 10 | 非旧证书（旧 SAN 仅 `117.72.84.210` / 旧 serial `…B82`） | 线上 serial `…B83`、SAN 含目标 IP、指纹与旧证 `48:76:E7:…:AD:75` 不同 | 未命中 FAIL 条件 |
| — | `RESULT overall` | `overall=PASS`，脚本退出码 0 | PASS |

补充（不改变判据）：03:10:44Z / 03:10:48Z / 03:10:51Z 连续三次独立握手，serial、SHA-256、SAN 完全一致，未见多实例或回滚漂移。

### 关键原始 `RESULT` 行

```
RESULT tls_handshake=OK
RESULT san_contains_target=YES
RESULT chain_verify=OK
RESULT ip_san_match=OK
RESULT negative_control=OK
RESULT serial_changed=YES
RESULT overall=PASS
```

### 叶子证书当次原文

```
subject=C = US, ST = State, L = City, O = Organization, OU = Organizational Unit, CN = 8.140.23.253
issuer=C = US, ST = California, L = San Francisco, O = LinkValue, OU = Engineering, CN = Tiktok Root CA, emailAddress = snack_yx@126.com
serial=77ADCFD924BA0F87F845C81934D4A3F346D4BB83
notBefore=Nov 25 02:55:16 2026 GMT
notAfter=Nov 24 02:55:16 2028 GMT
sha256 Fingerprint=E7:AD:1E:46:FA:5B:5D:06:02:C5:AD:04:B7:8B:1C:AF:2E:78:EE:47:8A:9F:6A:79:EA:B6:3B:D5:54:BD:88:84
X509v3 Subject Alternative Name:
    IP Address:8.140.23.253, IP Address:117.72.84.210
```

TLS 层：`New, TLSv1.3, Cipher is TLS_AES_256_GCM_SHA384`，`Verification: OK`，`Verify return code: 0 (ok)`。

## 4. 复现命令（只读）

```bash
bash scripts/emqx_8883_cert_verify.sh 8.140.23.253 8883 \
  dengbei_care/app/src/main/assets/jd-ca.crt --expect-new

openssl s_client -connect 8.140.23.253:8883 -servername 8.140.23.253 \
  -CAfile dengbei_care/app/src/main/assets/jd-ca.crt \
  -verify_ip 8.140.23.253 -verify_return_error </dev/null 2>&1 | tail -40

openssl x509 -in dengbei_care/app/src/main/assets/jd-ca.crt -noout -fingerprint -sha256
```

## 5. 范围与未执行（明确区分）

| 项 | 状态 | 说明 |
|---|---|---|
| CERT8883-SRV-01 服务器证书/链/IP SAN/负例/serial | **PASS** | 本轮实测，证据见 §6 |
| App 运行时信任路径与 MQTT CONNECT/CONNACK（CLI-02） | 不在本用例范围 | 本用例不据服务器证书结果代替 App 实测 |
| HTTP 上传受理/持久化（UPLOAD-04）、P0/P1、交付整体（FINAL-05） | 不在本用例范围 | 本用例通过不推导其他用例或整体交付通过 |

**本报告只证明**：2026-11-26T03:10Z 该时刻线上 8883 叶子证书符合 §3 的 9 项判据；不证明 App 侧行为、上传、持久化或整体交付。

## 6. 证据索引

目录：`evidence/cert8883_srv01_20261126/`

| 文件 | 内容 |
|---|---|
| `00_precheck_identity.txt` | UTC/主机/openssl version/CA 指纹与文件摘要/基线引用 |
| `01_srv_cert_verify_expect_new.txt` | `emqx_8883_cert_verify.sh --expect-new` 完整原始输出（含全部 `RESULT` 行） |
| `02_s_client_verify_ip.txt` | `openssl s_client … -verify_ip … -verify_return_error` 原文 |
| `03_leaf_fields_chain_and_negative.txt` | `openssl x509` 字段、链校验、`-verify_ip` 正/负例原文与返回码 |
| `04_srv01_judgement.md` | 逐项判定表与说明 |
| `05_handshake_stability_samples.txt` | 三次重复握手一致性采样 |
| `leaf_8.140.23.253.pem` | 本次握手取回的叶子证书（公开证书） |
