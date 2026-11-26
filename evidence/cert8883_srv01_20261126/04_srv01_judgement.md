# CERT8883-SRV-01 判定表（平台 8883 叶子证书独立核验）

- 用例：CERT8883-SRV-01（TD-CERT8883-001 §3.1）
- 执行时间窗（UTC）：2026-11-26T03:09:59Z ~ 2026-11-26T03:10:51Z
- 执行主机：kason（MINGW64_NT-10.0-26120 x86_64，Windows 11）
- `openssl version`：OpenSSL 3.1.2 1 Aug 2023（Library: OpenSSL 3.1.2 1 Aug 2023，platform mingw64）
- 被测对象：既有平台 `8.140.23.253:8883`（EMQX 5.8.0，TLS listener）
- 方式：只读 TCP/TLS 握手 + 本地 openssl 校验；未登录、未写、未改本地信任库、未使用 `--insecure`
- 结论：**PASS**

## 1. 执行前身份记录（acceptance 第 1 条）

| 项 | 实测 | 基线/要求 | 结果 |
|---|---|---|---|
| UTC 时间 | 2026-11-26T03:09:59Z | 必须记录 | 已记录 |
| 主机 | kason（MINGW64_NT-10.0-26120） | 必须记录 | 已记录 |
| openssl | 3.1.2 (1 Aug 2023) | 必须记录 | 已记录 |
| `dengbei_care/app/src/main/assets/jd-ca.crt` X.509 SHA-256 | `75:33:09:B3:9F:1B:8F:85:B4:D7:57:76:BF:D3:8A:42:52:0D:C1:A0:5E:A2:0B:A3:11:7E:B3:9C:BA:B2:15:2B` | 基线同一值 | **一致，未触发信任锚变更分支** |
| `jd-ca.crt` 文件 SHA-256 | `5c17cf740ffdf416c2f31f90172ad3565966cd62c0952920b9b2ac974aefe1e4` | 与 r4 记录一致 | 一致 |
| CA subject/issuer | `CN=Tiktok Root CA, O=LinkValue`（自签） | — | 一致 |
| CA 有效期 | 2024-10-12T16:05:01Z → **2034-10-10T16:05:01Z** | 上限 2034-10-10 | 一致 |

证据：`00_precheck_identity.txt`

## 2. 逐项判据

| # | 判据（acceptance） | 实测 | 结果 |
|---|---|---|---|
| 1 | `tls_handshake=OK`（8883 返回叶子证书） | `RESULT tls_handshake=OK`（2026-11-26T03:10:03Z） | PASS |
| 2 | `chain_verify=OK`（仅以 `jd-ca.crt` 为 CA） | `RESULT chain_verify=OK`；`openssl verify -CAfile jd-ca.crt leaf.pem` → `OK`，rc=0 | PASS |
| 3 | 叶子 `issuer` 为 `CN=Tiktok Root CA, O=LinkValue` | `issuer=C = US, ST = California, L = San Francisco, O = LinkValue, OU = Engineering, CN = Tiktok Root CA` | PASS |
| 4 | SAN 原文含 `IP Address:8.140.23.253`，`san_contains_target=YES` | `X509v3 Subject Alternative Name: IP Address:8.140.23.253, IP Address:117.72.84.210`；`RESULT san_contains_target=YES` | PASS |
| 5 | `ip_san_match=OK`（`-verify_ip 8.140.23.253` 返回 0） | `openssl verify -CAfile jd-ca.crt -verify_ip 8.140.23.253 leaf.pem` → `OK`，rc=0；`RESULT ip_san_match=OK` | PASS |
| 6 | `negative_control=OK`（`-verify_ip 8.140.23.254` 不通过） | `error 64 at 0 depth lookup: IP address mismatch`，`verification failed`，rc=2；`RESULT negative_control=OK` | PASS |
| 7 | `serial_changed=YES` | 线上 `serial=77ADCFD924BA0F87F845C81934D4A3F346D4BB83`，缺陷基线 `…B82`，二者不同 → `RESULT serial_changed=YES` | PASS |
| 8 | 线上 serial 与部署/签发记录一致 | 线上 `…B83` / SHA-256 `E7:AD:1E:46:FA:5B:5D:06:02:C5:AD:04:B7:8B:1C:AF:2E:78:EE:47:8A:9F:6A:79:EA:B6:3B:D5:54:BD:88:84` / SAN 两个 IP / notAfter `2028-11-24T02:55:16Z`，与 `deploy/emqx_8883_tls/container.yaml` 的 `expected` 段及 `devops_8883_cert_20261125/DEPLOYMENT_EVIDENCE.md` 逐项一致 | PASS |
| 9 | `notAfter` 晚于执行时刻且不晚于 CA 的 2034-10-10 | `notAfter=Nov 24 02:55:16 2028 GMT`（晚于 2026-11-26T03:10Z；早于 2034-10-10） | PASS |
| 10 | 判据未被放宽（不得以 `-verify_hostname`/CN 代替 IP SAN） | 权威判据使用 `-verify_ip`；补充对照 `-verify_hostname 8.140.23.254` 亦为 `error 62 hostname mismatch`（仅记录，不作 IP 判据） | PASS |
| 11 | 未取回旧证书（旧 SAN 仅 `117.72.84.210` / 旧 serial `…B82`） | 线上为 serial `…B83`、SAN 含 `IP:8.140.23.253`、指纹与旧证 `48:76:E7:…:75` 不同 | 未命中 FAIL 条件 |
| 12 | 握手稳定性（补充） | 03:10:44Z / 03:10:48Z / 03:10:51Z 三次采样 serial、SHA-256、SAN 完全一致 | PASS（无多实例/回滚漂移） |
| — | `RESULT overall` | `overall=PASS`（脚本退出码 0） | PASS |

## 3. 叶子证书身份（当次实测）

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

TLS：`New, TLSv1.3, Cipher is TLS_AES_256_GCM_SHA384`，`Verification: OK`，`Verify return code: 0 (ok)`。

## 4. 说明与边界

1. 新证书 SAN 同时保留旧地址 `117.72.84.210` 与目标地址 `8.140.23.253`，与批准签发记录一致；本用例只要求目标 IP 命中，保留旧 IP 不构成本用例失败，但属于部署方设计与证书最小化范围，测试侧不代为判定其必要性。
2. 本用例只证明服务器侧证书/链/IP SAN/负例，不证明 App 运行时信任路径、MQTT CONNECT、HTTP 上传与持久化（分别属 CERT8883-CLI-02 / UPLOAD-04 / P0 / P1）。
3. serial 变更幅度小（`…B82` → `…B83`），但叶子 SHA-256、subject、有效期、SAN 均与旧证不同，为实际重新签发证书，非同一证书。
4. 全程只读：只发起 TCP/TLS 握手与本地 `openssl` 校验；无账号、无写入、无 `--insecure`、未改本地信任库；未依赖部署方自述判定（部署记录仅用于第 8 条一致性比对）。

## 5. 证据索引

| 文件 | 内容 |
|---|---|
| `00_precheck_identity.txt` | UTC/主机/openssl version/CA 指纹与文件摘要/基线引用 |
| `01_srv_cert_verify_expect_new.txt` | `scripts/emqx_8883_cert_verify.sh --expect-new` 完整原始输出（含全部 `RESULT` 行，`overall=PASS`） |
| `02_s_client_verify_ip.txt` | `openssl s_client … -verify_ip … -verify_return_error` 原文（Verification: OK / rc 0） |
| `03_leaf_fields_chain_and_negative.txt` | `openssl x509` 字段、链校验、`-verify_ip` 正/负例原始输出与返回码 |
| `04_srv01_judgement.md` | 本判定表 |
| `05_handshake_stability_samples.txt` | 三次重复握手的 serial/SHA-256/SAN 一致性 |
| `leaf_8.140.23.253.pem` | 本次握手取回的叶子证书（公开证书，无私有信息） |
