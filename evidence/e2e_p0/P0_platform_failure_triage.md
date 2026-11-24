# E2E-JOINT-P0 平台段失败分析（软件归因 PT-001）

- 负责角色：软件测试
- 能力：软件失败分析（`software_tester.software_failure_analysis`）
- 原始失败：`evidence/e2e_p0/P0_platform_stage_report.md` **TEST_FAIL**（`failure_cause=MEASURED_DEFECT`）
- 被测对象：既有平台服务 `8.140.23.253`（EMQX 5.8.0 `8883` TLS / Flask `5000` / MySQL 9.0.1）＋ Android 候选 APK AB-003（`com.jinyuni.dengbei_care`，versionCode 10 / versionName 1.9，SHA-256 `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b`）
- 代码索引：`artifacts/codegraph_e2e_index.md`（Android 根 `D:\mypro\beiwo2\AIP\dengbei_care`）
- 分析时点：2026-11-24（Asia/Shanghai；原始失败时间窗 2026-11-23T14:39Z–14:44Z，设备本地 22:39–22:44 UTC+8）
- 分析时 HEAD：`1185c18`（分析开始时工作区干净）
- **本轮结论：`REPAIR_REQUIRED`（对象：既有平台 EMQX `8883` listener 的服务器证书与部署配置）**

> 本文件只做失败归因与责任边界判断，**不是复验通过、不是修复、不代表 P0 或产品整体通过**。
> 本轮未修改产品源码、固件、测试期望、服务、数据库、权限或环境配置；未连接服务器、未登录任何服务、未读取/复制凭据、未调用设备工具。
> “实测”均引用同版本既存原始证据；“静态”为本轮源码/字节码核对；“推断”明确标注，不作为根因证实。

---

## 1. 结论摘要

1. **原始失败是可复现的实测缺陷，不是测试工具或网络路径问题。** 手机真实路径到 `8.140.23.253:8883` 的 TCP 与 TLS 传输层握手成功；失败发生在 **App 作为 TLS 客户端校验服务器证书主机名** 的阶段：`javax.net.ssl.SSLHandshakeException: No subjectAltNames on the certificate match`。同一错误在 2026-11-23T14:38:12Z、14:38:56Z、14:39:05Z、14:43:12Z 多次复现。
2. **故障机制已定位到服务器证书内容。** EMQX 当前 8883 服务器证书 `subject CN=117.72.84.210`，`SAN` 仅 `IP Address:117.72.84.210`，不含 App 配置的当前地址 `8.140.23.253`。证书由 App 信任锚 `assets/jd-ca.crt`（`CN=Tiktok Root CA`，`O=LinkValue`）签发且链可信；失败点是 **SAN 主机名不匹配**，不是证书过期、链不受信或端口未放行。
3. **App 的行为是 Paho 1.2.5 / Android Conscrypt 的默认安全行为，不是 App 实现偏离。** `MqttConnectOptions` 构造默认 `httpsHostnameVerificationEnabled=true`；`SSLNetworkModule.start()` 在此开关开启时设置 `endpointIdentificationAlgorithm="HTTPS"`，由系统 TLS 栈执行主机名校验。App 的 `setupSSL()` 设置了基于 `jd-ca.crt` 的 `socketFactory`，但**没有**关闭主机名校验，也没有设置自定义 `HostnameVerifier`。
4. **`服务器迁移记录.md` 中“App 不校验主机名，只校验 CA 链”的描述与实测不符。** 该描述是迁移时的客户端行为假设，不是产品需求；不能据此把 App 的默认校验证书主机名判为缺陷，也不建议为迎合该描述而降低 TLS 校验。
5. **责任对象是平台侧证书/部署配置，不是手机、网关、固件或测试脚本。** 同一日志窗口内 BLE 网关链路正常（`GwID=B1:17:9C:7A:4E:E0`、`dev=1`、`writeCharacteristic ok=true, bytes=6`），证明失败点仅在 App↔EMQX 的 TLS 校验。
6. **影响：P0 平台段不能判 PASS，P1 上传链路被阻断在 TLS 阶段（MQTT CONNECT 尚未发出）。** 无测试身份（`registerphone`/`mac_addr` 为空）仍使 P1 上传/持久化段 `NOT EXECUTED`；这是独立缺口，不是本次返工对象，也不能被写成通过。
7. **另有一个独立的外部条件缺口，不能与实测缺陷混为一谈。** 平台只读基线要求的“近期服务日志、连接数”仍 `NOT EXECUTED`：本能力无 `existing_service_ssh_inspection` 的 `inspect_existing_service_readonly` 授权；DB 回读无授权。该缺口不是 TLS 失败的原因，也不能替代证书缺陷的结论。
8. **结果判断：`REPAIR_REQUIRED`。** 修复对象为既有平台服务 EMQX 8883 的服务器证书/部署配置；修复后须用当前 App 与同一 P0 平台段窗口重新取得 TLS/CONNACK 证据，再回到原受影响测试复验；不得沿用本轮失败证据或旧证书结论。

---

## 2. 对象身份、版本与证据来源

| 项 | 值 | 来源/核对方式 |
|---|---|---|
| 平台服务 | `8.140.23.253`；EMQX 5.8.0（`emqx@127.0.0.1` running，Dashboard/status 200，Cowboy）；Flask `5000`（Werkzeug/3.1.8 Python/3.12.3）；MySQL 9.0.1（仅端口握手） | `P0_platform_stage_report.md` §3；`platform_stage_retest_http_phone.txt`、`platform_stage_retest_identity.txt`、`platform_stage_retest_host.txt`（实测） |
| EMQX 8883 listener | TCP 可连；TLSv1.2 `ECDHE-RSA-AES256-GCM-SHA384` 握手成功（收到 ServerHello/Certificate/ServerKeyExchange/Finished） | `platform_stage_retest_8883_phone.txt`（实测） |
| 服务器证书 | `subject C=US, ST=State, L=City, O=Organization, OU=Organizational Unit, CN=117.72.84.210`；`issuer ... CN=Tiktok Root CA`；`notBefore 2024-10-12 16:07:40Z`；`notAfter 2026-12-21 16:07:40Z`；`X509v3 Subject Alternative Name: IP Address:117.72.84.210`（仅此一项）；链含自签 `Tiktok Root CA` | `platform_stage_retest_cert.txt`（实测，2026-11-23T14:42:28Z） |
| App 候选 | `com.jinyuni.dengbei_care`，versionCode 10 / versionName 1.9，debug；APK 10,370,964 B，SHA-256 `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b` | `P0_phone_stage_report.md` §1/§3.1；`phone_stage_apk_digest.txt`（实测，主机/设备侧一致） |
| App 配置 | `CloudConfig.SERVER_HOST="8.140.23.253"`；`MQTT_BROKER_URI="ssl://8.140.23.253:8883"` | `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/cloud/CloudConfig.kt:18,27`（静态） |
| App TLS 配置 | `setupSSL()` 从 `assets/jd-ca.crt` 构造 `TrustManagerFactory`，`SSLContext("TLSv1.2")`，`options.socketFactory = sslContext.socketFactory`；未调用 `setHttpsHostnameVerificationEnabled(false)`，未设置 `HostnameVerifier` | `MqtttService.kt:1591-1629`（静态）；全文件检索无 `HostnameVerifier` 命中 |
| App 信任锚 | `assets/jd-ca.crt`：`subject/issuer CN=Tiktok Root CA, O=LinkValue`；有效期 2024-10-12T16:05:01Z–2034-10-10T16:05:01Z；SHA-256 指纹 `75:33:09:B3:9F:1B:8F:85:B4:D7:57:76:BF:D3:8A:42:52:0D:C1:A0:5E:A2:0B:A3:11:7E:B3:9C:BA:B2:15:2B` | 本地 `openssl x509` 只读核对（静态） |
| MQTT 客户端库 | Paho `org.eclipse.paho.client.mqttv3:1.2.5` | `dengbei_care/gradle/libs.versions.toml:35`（静态） |
| Paho 主机名校验默认值 | `MqttConnectOptions()` 构造将 `httpsHostnameVerificationEnabled` 置为 `true`；`SSLNetworkModule.start()` 在该字段为真时执行 `SSLParameters.setEndpointIdentificationAlgorithm("HTTPS")` 后 `startHandshake()` | Gradle 缓存中 1.2.5 jar 的 `javap -c -p` 字节码核对（静态） |
| 手机 | serial `12f046690506`，Redmi 8A `olivelite`，Android 10 / API 29；默认网络 `ZTE_607157`（`Active default network: 100 (WIFI)`） | `P0_phone_stage_report.md` §1/§2；`P0_platform_stage_report.md` §1（实测） |
| 网关关联键 | `gateway_ble_addr=84:C2:E4:03:02:02`；`GwID=B1:17:9C:7A:4E:E0`；`uid4=6A002C00` | `P0_phone_stage_report.md` §1；`P0_platform_stage_report.md` §6（引用） |
| 原始失败时间窗 | 平台段复测 `2026-11-23T14:39Z–14:44Z`；TLS 失败复现 14:38:12Z、14:38:56Z、14:39:05Z、14:43:12Z | `platform_stage_retest_logcat_mqtt.txt`、`platform_stage_retest_logcat_tls_fail.txt`（实测） |

**版本闭合说明**：本轮分析针对原始失败报告中的同一 App APK 摘要与同一平台地址/端口；未替换 APK、未切换平台入口、未重跑网络探测。服务器证书为当次 TLS 握手实际返回的证书（`platform_stage_retest_cert.txt`），不是迁移资料中的历史声明。

---

## 3. 原始失败与可复现事实

### 3.1 失败现象（引用原始证据，不重新判定）

| 时间（本地/UTC） | 观察 | 证据 |
|---|---|---|
| 22:29:12 / 14:29:12Z | 8883 放行前：`SocketTimeoutException: failed to connect to .../8.140.23.253 (port 8883) ... after 30000ms` | `platform_stage_retest_logcat_mqtt.txt`（放行前对照） |
| 22:38:12 / 14:38:12Z | 放行后：`SSLHandshakeException: No subjectAltNames on the certificate match`；栈含 `SSLNetworkModule.start` → `ConscryptFileDescriptorSocket.startHandshake` | `platform_stage_retest_logcat_mqtt.txt` |
| 22:38:56 / 14:38:56Z | 同一异常复现 | `platform_stage_retest_logcat_tls_fail.txt` |
| 22:39:05 / 14:39:05Z | 同一异常复现 | `platform_stage_retest_logcat_tls_fail.txt` |
| 22:43:12 / 14:43:12Z | 同一异常复现；同一窗口 BLE 读取正常（`GwID=B1:17:9C:7A:4E:E0, dev=1`、`writeCharacteristic ok=true, bytes=6`） | `platform_stage_retest_logcat_tls_fail.txt` |

同一窗口内 App 日志的 BLE 侧正常，说明手机、网关、App 的 BLE 读取/绑定路径未被该失败影响；失败发生在 App↔EMQX 的 TLS 客户端校验。

### 3.2 传输层与证书事实

- 手机真实路径：TCP `Connected`；`nc -w 6` `rc=0`；TLS 握手成功（TLSv1.2）；HTTP `GET /` 到 MQTT 端口收到 `close notify`（非 HTTP 端口，空响应属预期）。→ **网络路径、8883 放行、EMQX listener 与传输层 TLS 均可用**（`platform_stage_retest_8883_phone.txt`）。
- 服务器证书：`CN=117.72.84.210`；SAN 仅 `IP Address:117.72.84.210`；由 `Tiktok Root CA` 签发；有效期至 2026-12-21（未过期）。→ **证书本身未过期，链也来自目标服务器；唯一不匹配的是主机名/SAN**（`platform_stage_retest_cert.txt`）。
- App 目标地址：`ssl://8.140.23.253:8883`（`CloudConfig.kt`）；运行日志中的连接目标同为 `8.140.23.253`。→ **客户端校验的主机名与证书 SAN 不一致**。

### 3.3 App 客户端行为（静态核对）

- `setupSSL()` 为 Paho 提供自定义 `SSLSocketFactory`（信任锚 `jd-ca.crt`），但未关闭 Paho 的主机名校验。
- Paho 1.2.5 `MqttConnectOptions` 默认 `httpsHostnameVerificationEnabled=true`；`SSLNetworkModule.start()` 在此默认下调用 `setEndpointIdentificationAlgorithm("HTTPS")`，Android Conscrypt 随即按 SAN 校验主机名。这与实测异常栈一致。
- 因此，**失败不是“App 没有使用 jd-ca.crt”或“链不受信”**：若链校验失败，异常应为 `Trust anchor for certification path not found`/`PKIX path building failed`；实际异常是链校验通过后的 `No subjectAltNames on the certificate match`。

> 说明：异常栈中出现 `android.security.net.config.NetworkSecurityTrustManager` / `RootTrustManager` 类，说明本次运行的系统 TLS 信任路径细节仍有未展开之处；但**无论该路径最终使用哪个 TrustManager，决定性的失败都是主机名/SAN 校验**，且服务端证书 SAN 不含 `8.140.23.253` 已由独立证书输出证实。该细节不改变责任对象；若实现需要进一步确认证书链/信任锚在同一设备上的装配方式，可用最小实验（见 §7）补充。

---

## 4. 竞争假设与判定

| # | 假设 | 判定 | 依据 |
|---|---|---|---|
| H1 | 服务器证书 SAN 未包含当前地址 `8.140.23.253` | **成立（直接原因）** | 证书 SAN 仅 `117.72.84.210`；App 异常为 SAN 不匹配；证书未过期 |
| H2 | 服务器证书过期 | **排除** | `notAfter=2026-12-21`，失败时仍有效 |
| H3 | 8883 被防火墙/安全组阻断 | **排除** | 手机 TCP `rc=0`；TLS 握手完成并收到服务器证书 |
| H4 | EMQX 未运行/未监听 8883 | **排除** | TLS 握手成功；Dashboard `/status` 200；`emqx is running` |
| H5 | App 连接了错误地址 | **排除** | `CloudConfig` 与运行日志均为 `ssl://8.140.23.253:8883` |
| H6 | App 信任锚错误/证书链不受信 | **不支持** | 链校验通过后才报主机名错误；`jd-ca.crt` 与服务器链根同为 `Tiktok Root CA` |
| H7 | App 实现偏离“只校验 CA 链、不校验主机名”的迁移描述 | **不成立** | Paho 1.2.5 默认开启主机名校验；App 未提供关闭开关/自定义 `HostnameVerifier`。该描述是迁移时的错误假设，不是产品需求 |
| H8 | 测试工具/Playwright/curl 造成假象 | **排除** | 失败栈来自 App 自身 Paho/Conscrypt；原始测试是 App 真实路径日志 |
| H9 | 8083/8084 手机不可达导致 TLS 失败 | **排除** | 8083/8084 与 8883 是不同 listener；8883 传输层握手已成功。8083/8084 超时另行记录，不据此判缺陷 |
| H10 | 服务端证书更新所需 CA 私钥/重签材料不可用 | **未知（影响修复可行性，不影响故障归因）** | 本轮未接触服务器与密钥材料；修复能力需自行确认（见 §6/§7） |
| H11 | MySQL/Flask/Dashboard 或平台其他服务共同导致本次失败 | **无证据** | 失败发生在 App↔EMQX 8883 TLS；Flask/MySQL 仅观察到端口/握手，未被本次失败调用 |

---

## 5. 责任边界与结果判断

### 5.1 主责对象（REPAIR_REQUIRED）

| 项 | 内容 |
|---|---|
| 对象 | 既有平台服务 `8.140.23.253` 上 EMQX 5.8.0 的 **8883 TLS listener 服务器证书/证书部署配置** |
| 当前对象身份 | `CN=117.72.84.210`，`SAN=IP:117.72.84.210`，issuer `Tiktok Root CA`，有效期至 2026-12-21；App 连接地址 `8.140.23.253` |
| 故障机制 | 服务器证书的主机名标识未随服务器地址迁移更新；客户端按 HTTPS 端点标识校验 SAN，找不到 `8.140.23.253`，在 TLS 握手阶段终止；MQTT CONNECT 未发出 |
| 证据 | `platform_stage_retest_cert.txt`（证书原始输出）；`platform_stage_retest_logcat_tls_fail.txt` / `platform_stage_retest_logcat_mqtt.txt`（App 异常栈，4 次复现）；`platform_stage_retest_8883_phone.txt`（TCP/TLS 成功，排除网络） |
| 影响范围 | P0 平台段不能判 PASS；P1 的 App→EMQX MQTT 发布前置不成立；HTTP `/upload_data` 链路未被本次失败直接证明，但因无测试身份 P1 仍 `NOT EXECUTED` |
| 修复责任领域 | 平台后端/部署（既有服务证书与 listener 配置）；图中平台失败分支声明的处理能力为 `backend_engineer.backend_integration_debug` |
| 复验目标 | 同一 P0 用例、同一 App/手机/平台地址；App 对 `ssl://8.140.23.253:8883` 的 TLS 握手不再出现 `No subjectAltNames on the certificate match`，进入 MQTT CONNECT 并记录实际 CONNACK/认证结果；随后按原设计继续 P0 平台只读基线与 P1 资格判定 |

**修复边界（供实现/部署决策，测试不代改产品）**：
- **优先**：重签/部署包含 `SAN=IP:8.140.23.253`（如适用，再加所需 DNS 名）的服务器证书，保持由 App 当前信任锚 `assets/jd-ca.crt`（`Tiktok Root CA`）签发并保持 8883 listener 语义；不改变 App 地址/端口/认证格式。
- **备选**：若 CA 私钥不可用或产品决定更换信任锚，则由实现评估“新 CA + 新服务器证书 + App 信任锚更新/重建 APK”的完整方案；这会改变 APK 身份，必须重新记录 SHA-256/versionCode 并重新采集手机段证据。
- **不采用**：仅为绕过失败而关闭 App 的证书主机名校验、使用 `--insecure`/自定义空校验、或以证书链自签为理由降低校验；这些不能作为产品修复，也不能作为复验通过依据。
- **前置授权/资源**：服务器证书替换需要既有服务的管理/部署授权；本能力与环境当前未确认该项授权（`existing_service_ssh_inspection` 对本能力 `unavailable_for_current_capability`；无服务器变更入口）。修复能力若需该入口，应按其合同申请人工授权，不得由测试侧代改服务器。

### 5.2 App / 手机 / 网关 / 固件

| 对象 | 判断 | 依据 |
|---|---|---|
| Android App | **本次失败不归因于 App 缺陷** | App 使用 Paho 1.2.5 默认主机名校验；未关闭校验是安全默认行为。迁移记录的相反描述与实测不符，属于文档假设错误，不构成修复 App 的依据 |
| 手机/Wi-Fi | **正常** | 设备在线；`ZTE_607157` 默认网络；手机到平台 TCP/TLS 成功 |
| 网关/BLE | **正常** | 同一日志窗口 `GwID`、`dev=1`、`bytes=6` 正常；失败点不在 BLE |
| 传感器固件 | **不在本失败路径** | 本次为平台 TLS；固件段已有独立 EV-041 结论，不在此重判 |

### 5.3 独立缺口（不是本次返工对象，也不能被写成通过）

1. **平台只读基线（日志/连接数）**：`existing_service_ssh_inspection` 对本能力不可用（缺 `inspect_existing_service_readonly`），EMQX 管理 API 401 且未使用凭据；对应项 `NOT EXECUTED`。这是授权/入口缺口，不是 TLS 缺陷的原因；修复证书不自动补齐该基线，需另行取得只读授权或由具备入口的能力执行。
2. **DB 回读**：无授权，`NOT EXECUTED`；不得以 HTTP 2xx 替代。
3. **测试身份**：`registerphone` 空、`mac_addr=000000000000`；P1 上传/持久化段 `NOT EXECUTED`；不得伪造或改 prefs/DB。
4. **8083/8084**：手机侧 `nc` 超时，已记录为部署差异；未判缺陷，需服务器/安全组侧确认。
5. **文档事实错误**：`服务器迁移记录.md` §四注称“App 不校验主机名，只校验 CA 链”，与 Paho 1.2.5 实测不符；应由文档所有者更正，不应通过修改 App 代码去“符合”该错误描述。

---

## 6. 最小下一步与复验判据

### 6.1 修复侧（平台后端/部署）

1. 确认 8883 当前证书文件、私钥、签发 CA（`Tiktok Root CA`）私钥的可用性；记录当前证书 serial/指纹/部署路径。
2. 重签或替换包含 `SAN=IP:8.140.23.253` 的服务器证书（与 App 信任锚一致），按 EMQX 部署方式加载并确认 listener 仍为 `8883`/TLS。
3. 在服务器侧（或获准的受控客户端）做最小验证：
   - `openssl s_client -connect 8.140.23.253:8883 -CAfile <jd-ca.crt> -verify_hostname 8.140.23.253` → 预期 `Verify return code: 0 (ok)`；
   - 证书输出 SAN 包含 `8.140.23.253`。
   - 该命令是建议的修复侧验证，不是本能力已执行的测试。
4. 保留原失败证据与本次 triage；不得以“证书已换”口头声明代替复测。

### 6.2 测试侧（原 P0 平台段复验）

1. 使用同一用例 `E2E-JOINT-P0`、同一手机 `12f046690506`、`ZTE_607157` 默认网络、同一 App 候选（若 APK 未变，记录原 SHA-256；若 App 更新，记录新 SHA-256/版本并重做手机段）。
2. 启动守护，观察 App 日志对 `ssl://8.140.23.253:8883` 的连接：
   - **通过本缺陷的判据**：不再出现 `No subjectAltNames on the certificate match`；TLS 握手完成并进入 MQTT CONNECT；记录实际 CONNACK/认证结果（无测试身份时认证失败或被服务拒绝是另一层，不得与 TLS 失败混判）。
   - **仍失败**：若仍出现 SAN/链错误，保留新证书输出与完整异常栈，交回平台修复；不得复用本轮证据。
3. 平台只读基线：按已获准入口记录服务/端口/日志/连接数；无入口的项继续如实 `NOT EXECUTED`，不得因 TLS 修复而改判通过。
4. P1：仍需隔离测试身份（`registerphone` 非空、已确认来源的 `mac_addr`）与平台回读授权；证书修复不自动使 P1 可执行。P1 未执行时保持 `NOT EXECUTED`。

### 6.3 未确定项与最小判别实验

| 项 | 状态 | 最小下一步 |
|---|---|---|
| CA 私钥/重签材料是否可用 | 未知（本轮未访问服务器/密钥） | 修复侧确认；不可用时按 §5.1 备选方案评估 |
| 证书替换是否需重启/重载 EMQX listener | 未验证 | 按实际部署方式确认；只记录，不猜测 |
| App 运行时最终使用的 TrustManager 路径（栈中系统类） | 未完全展开；不影响 SAN 结论 | 修复后用同一 App 复验；若需单独证明信任锚，可在受控环境用同一 APK 连接测试证书，不修改产品代码 |
| 8083/8084 手机不可达 | 已记录差异，未判缺陷 | 服务器/安全组侧确认；不影响 8883 TLS 结论 |
| 服务日志/连接数 | NOT EXECUTED（授权缺口） | 取得 `inspect_existing_service_readonly` 或由获准能力执行 |

---

## 7. 未执行、未知与边界声明

- **未执行**：未重跑手机/平台网络探测；未连接 `8.140.23.253`；未登录 EMQX/Flask/SSH/DB；未读取/复制任何凭据；未修改/重启/重写服务器；未调用 `android_test`/`usb_list_devices`/串口/烧录/逻辑分析仪；未修改产品源码、测试期望、固件、网关、权限或环境配置。
- **未证明**：证书替换后的实际 TLS/CONNACK；服务端证书私钥/CA 私钥可用性；平台日志/连接数/DB 回读；P1 上传/持久化；8083/8084 的部署原因；生产/发布包行为。未执行项均不等于通过。
- **不是根因**：把“App 不校验主机名”的迁移描述当作需求、把 8083/8084 超时当作 8883 失败原因、把 TCP/TLS 传输成功当作 MQTT 认证成功、把手机系统 curl 的 `-k`/自签链继续行为当作 App 校验结果。
- **脱敏**：本报告未包含 Wi-Fi 密码、SSH/DB/Dashboard 凭据或密钥材料；`uid4`、网关地址、`GwID`、证书主体为设备/服务标识，不是凭据。
- **结论强度**：根因已由同版本运行的原始异常栈与独立证书输出证实；修复方案与可修复性由实现/部署决定，本能力不代替其判断。

---

## 8. 证据清单（本次引用，均为既有文件）

| 文件 | 用途 |
|---|---|
| `evidence/e2e_p0/P0_platform_stage_report.md` | 原始失败报告（TEST_FAIL/MEASURED_DEFECT，复现步骤、根因边界、未执行项） |
| `evidence/e2e_p0/P0_phone_stage_report.md` | 手机段 PASS 与 APK/设备/网络/网关关联键（同版本信息） |
| `evidence/e2e_p0/platform_stage_retest_cert.txt` | 服务器证书 subject/issuer/有效期/SAN 原始输出 |
| `evidence/e2e_p0/platform_stage_retest_8883_phone.txt` | 手机真实路径 TCP/TLS 握手成功原始输出 |
| `evidence/e2e_p0/platform_stage_retest_logcat_tls_fail.txt` | TLS SAN 失败栈与 BLE 正常读取对照（14:38–14:43Z） |
| `evidence/e2e_p0/platform_stage_retest_logcat_mqtt.txt` | 放行前 SocketTimeout 与放行后 TLS 失败对照 |
| `evidence/e2e_p0/platform_stage_retest_identity.txt` | EMQX API 401、MySQL 9.0.1 握手、8083/8084 超时 |
| `evidence/e2e_p0/platform_stage_retest_http_phone.txt` | 手机只读 GET/端口探测原始输出 |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/MqtttService.kt` | `setupSSL()`/连接路径静态核对 |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/cloud/CloudConfig.kt` | 服务器地址/端口常量 |
| `dengbei_care/app/src/main/assets/jd-ca.crt` | App 信任锚（本地 openssl 只读核对） |
| `dengbei_care/gradle/libs.versions.toml` | Paho 1.2.5 版本 |
| `artifacts/e2e_joint_test_design.md` §4.1/§8 | P0 判据、阶段边界、复验规则 |
| `artifacts/e2e_cases.yaml` ITEM-001 | P0 任务/验收摘要 |
| `服务器迁移记录.md` §三/§四/§五 | 证书迁移声明与“App 不校验主机名”描述（已与实测对照，列为文档差异） |
