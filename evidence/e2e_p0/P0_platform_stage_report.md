# E2E-JOINT-P0 平台段执行记录（8883 放行后复测：EMQX TLS 被服务器证书 SAN 不匹配阻断）

- 用例：`E2E-JOINT-P0`（P0 身份、手机联网与接收就绪；`stage_order`: embedded → phone → platform）
- 负责角色：软件测试
- 本轮执行时间：2026-11-23 **14:39Z–14:44Z**（本地 22:39–22:44，UTC+8）
- 被测对象：既有平台服务 `8.140.23.253`（项目资料声明为 EMQX 5.8.0 / Flask 注册服务 5000 / MySQL 9.0.1）＋ Android App 候选 APK（AB-003）
- 设计依据：`artifacts/e2e_joint_test_design.md` §3.4/§4.1/§8；`artifacts/e2e_cases.yaml` ITEM-001
- 触发：操作者回复 `REASSESS`——**“已放行 TCP 8883，请从测试手机复测 EMQX TLS 连接。服务器侧只读核验结果暂未提供。”**
- **本轮结论（本调用范围＝平台段复测）**：
  - **平台段：TEST_FAIL（`MEASURED_DEFECT`）**。放行 8883 后，手机真实路径的 TCP 与 **TLS 传输握手成功**；但 App 作为真实消费端连接 `ssl://8.140.23.253:8883` 时 **TLS 握手失败**：`SSLHandshakeException: No subjectAltNames on the certificate match`。服务器证书 SAN 只有旧服务器 IP `117.72.84.210`，不含当前地址 `8.140.23.253`。这是当前交付版本的实测缺陷，直接阻断 P1 上传链路。
  - 平台只读基线：**服务身份与端口已实测**（EMQX 运行、Dashboard 200、Flask 存活、MySQL 握手）；**服务日志、连接数、DB 回读仍 NOT EXECUTED**（无 SSH 只读入口；EMQX 管理 API 需认证，未使用任何凭据登录）。
  - 本调用**不返回 TEST_PASS**；P0 整体不判通过。未修改、未重启、未写入、未直连数据库。

> 与上一轮的关系：上一轮（13:58Z–14:10Z）平台段因 8883/5000/18083 从手机真实路径 TCP 超时而 `NOT EXECUTED`。本轮同一用例、同一前置、同一被测对象复测；操作者只声明放行 8883，未提供服务器侧只读核验结果。上一轮的主机 `Meta Tunnel` 伪造接受证据保留（`platform_stage_baseline_result.json` 等），本轮主机路径已能取得真实应用层响应，二者按各自时间窗分别记录。

---

## 1. 前置条件复核

| 检查 | 实际结果 | 证据 |
|---|---|---|
| 设备在线 | `12f046690506  device product:olivelite model:Redmi_8A` | `platform_stage_retest_env.txt` |
| Wi-Fi/默认网络 | `wlan0` 192.168.0.103/24；`dumpsys wifi` 当前 `ZTE_607157`（BSSID `7c:21:d8:60:71:57`，rssi≈-45）；`ip rule`/`ip route` 默认路由 `default via 192.168.0.1 dev wlan0 table 1027` | `platform_stage_retest_network.txt` |
| App 进程/服务 | PID 4677；`MqtttService` `isForeground=true`、`MqttService` 已绑定；至 14:43Z 仍在运行 | `platform_stage_retest_app_state.txt` |
| 探测工具 | 手机 `/system/bin/curl`、`nc`、`toybox`、`ping` 可用；`openssl` 不存在（证书细节用主机 openssl 复核） | `platform_stage_retest_env.txt` |
| 密码/凭据 | 本轮未输入任何密码；未登录 Dashboard/SSH/DB；证据无明文凭据 | 本报告 §9 |

---

## 2. 手机真实路径复测（操作者指定入口）

### 2.1 TCP 与 TLS 传输层

| 探测 | 实际结果（2026-11-23T14:40:14Z） | 证据 |
|---|---|---|
| WAN 对照 `https://www.baidu.com` | HTTP **200**，connect 0.142 s，TLS 0.248 s，total 0.297 s | `platform_stage_retest_8883_phone.txt` |
| 手机 → `8.140.23.253:8883` TCP | `curl` **Connected**；`nc -w 6` **rc=0** | 同上 |
| 手机 → 8883 TLS 握手 | **成功**：`TLSv1.2 / ECDHE-RSA-AES256-GCM-SHA384`；收到 ServerHello/Certificate/ServerKeyExchange/Finished；`GET /` 后服务器 `close notify`（MQTT 端口非 HTTP，空响应属预期） | 同上 |
| 服务器证书（主机 openssl 复核） | subject `CN=117.72.84.210`；issuer `CN=Tiktok Root CA`（O=LinkValue）；有效期 2024-10-12 16:07:40Z–2026-12-21 16:07:40Z；**SAN 仅 `IP Address:117.72.84.210`**；链含自签根（默认信任库 `verify error:num=19 self-signed certificate in certificate chain`） | `platform_stage_retest_cert.txt` |

结论：**传输层 TLS 已可用**，证书也确实来自目标服务器（EMQX 在证书之后保持 MQTT 端口语义）。问题不在 TCP 放行或网络路径。

### 2.2 App 真实消费端（关键）

App 的 MQTT 客户端仍按 `ssl://8.140.23.253:8883` 重连（日志 `Saved MAC address: 000000000000` 后连接）。放行 8883 后失败原因由上一轮的 `SocketTimeoutException` 变为 **TLS 证书校验失败**：

| 时间（本地/UTC） | 实际结果 |
|---|---|
| 22:38:56 / 14:38:56Z | `MqttException (0) - javax.net.ssl.SSLHandshakeException: No subjectAltNames on the certificate match` |
| 22:39:05 / 14:39:05Z | 同上（连续重试，同一根因） |
| **22:43:12 / 14:43:12Z** | 同上，复现；栈：`ConscryptFileDescriptorSocket.startHandshake` → `TrustManagerImpl.checkTrusted` → `CertificateException: No subjectAltNames on the certificate match` |

证据：`platform_stage_retest_logcat_mqtt.txt`、`platform_stage_retest_logcat_tls_fail.txt`。

同一日志窗口内网关链路仍正常：BLE `GwID=B1:17:9C:7A:4E:E0, dev=1, valueSize=160, mtu=240`、`ALL_PLACEHOLDER`、`writeCharacteristic(legacy) ok=true, bytes=6`（0xA1 6 B），说明失败点仅在 App↔EMQX 的 TLS 校验，不是网关/手机 BLE。

---

## 3. 平台服务与端口只读基线

方式：手机真实路径 `curl`/`nc` + 获准主机入口 `curl`（均 **GET/TCP connect only**）；未 POST、未登录、未写入、未重启、未连数据库。

| 服务/端口 | 观测（2026-11-23T14:41–14:43Z） | 证据 |
|---|---|---|
| EMQX Dashboard 18083 | `GET /status` → **200**，body `Node emqx@127.0.0.1 is started` / `emqx is running`，`server: Cowboy`；`GET /` → 200 `EMQX Dashboard` HTML | `platform_stage_retest_http_phone.txt`、`platform_stage_retest_host.txt` |
| EMQX 管理 API | `GET /api/v5/status` → 200（同上文本）；`GET /api/v5/monitor_current`、`/api/v5/nodes` → **401** `AUTHORIZATION_HEADER_ERROR`（需认证；未使用凭据） | `platform_stage_retest_identity.txt` |
| EMQX 8883 | TCP 可连；TLS 握手成功；证书 SAN 不匹配当前地址（§2） | `platform_stage_retest_8883_phone.txt`、`platform_stage_retest_cert.txt` |
| EMQX 1883 | 手机 `nc -w 5` **rc=0**（可连） | `platform_stage_retest_http_phone.txt` |
| Flask 注册服务 5000 | `GET /` → **404**；`GET /login` → **405 Allow: POST, OPTIONS**；`GET /upload_data` → **405 Allow: POST, OPTIONS**；`Server: Werkzeug/3.1.8 Python/3.12.3`，`Access-Control-Allow-Origin: *` | `platform_stage_retest_http_phone.txt` |
| MySQL 3306 | 手机 TCP 连接返回 MySQL 握手：`9.0.1`、`caching_sha2_password`、`Got an error reading communication packets`（仅端口握手，**未认证、未读库**） | `platform_stage_retest_identity.txt` |
| EMQX 8083/8084 | 手机 `nc -w 5` **Timeout**（迁移记录声明应监听；本轮从手机不可达，记录差异，不据此判定缺陷） | 同上 |
| 主机路径 | 本轮主机 `curl` 18083/5000 取得真实应用层响应（上一轮 `Meta Tunnel` 伪造接受已不再表现为无首字节）；主机路由仍存在 198.18.0.2/metric 0 默认项，按当前实际响应记录 | `platform_stage_retest_host.txt` |

---

## 4. 实测缺陷分析（供实现/部署处理）

**现象**：App 连接 `ssl://8.140.23.253:8883` 时 TLS 校验失败，`No subjectAltNames on the certificate match`，反复重试均失败。

**当前版本与输入**：
- App：`com.jinyuni.dengbei_care`，versionCode 10 / versionName 1.9，SHA-256 `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b`（手机段已核验，主机/设备一致）。
- App 配置：`serverUri = ssl://8.140.23.253:8883`（迁移记录 §App 修改；运行日志显示连接目标 8.140.23.253:8883）。
- 服务器证书：SAN 仅 `IP:117.72.84.210`（旧服务器地址），无 `8.140.23.253`；链根为自签 `Tiktok Root CA`。

**复现步骤**：
1. 手机连接 `ZTE_607157`（默认网络 100/WIFI）。
2. 启动 App 守护，等待 MQTT 重连周期。
3. `adb logcat -s MqttService` 观察：`Attempting to reconnect to broker` → `SSLHandshakeException: No subjectAltNames on the certificate match`。

**预期**：App 与 EMQX 建立 TLS 会话并进入 MQTT CONNECT（是否通过认证是另一层，当前无测试身份）。
**实际**：TLS 在证书主机名校验阶段失败，MQTT CONNECT 未发出。

**根因边界（已确认部分）**：服务器证书的主机名标识与 App 配置的服务器地址不一致。Android 默认 TrustManager/Conscrypt 在链可信后仍校验 SAN；当前 APK 使用 Paho 默认 SSL socket 路径，实际行为**与迁移记录“App 不校验主机名，只校验 CA 链”的描述不一致**。该结论来自本轮原始异常栈与证书 SAN，不是猜测。

**建议修复边界（由实现/部署决定，测试不改产品代码）**：
- 优先：为 EMQX 重新签发/部署包含 `SAN=IP:8.140.23.253`（及所需域名）的服务器证书，并保持 App 信任的 CA 链一致；
- 或由实现评估 App 侧 TLS 配置（是否按设计校验证书主机名）并给出正式方案；
- 测试侧不修改断言、不降低校验、不用 `--insecure` 结果冒充 App 通过。

**影响**：P1 上传/持久化链路的前置（App→EMQX TLS）不成立；P1 仍 `NOT EXECUTED`。

---

## 5. 平台基线逐项状态

| 基线项 | 状态 | 原因/证据 |
|---|---|---|
| 服务身份（EMQX/Flask/MySQL 实际响应） | **已实测（有限）** | EMQX `emqx is running`、Dashboard HTML；Flask `Werkzeug/3.1.8 Python/3.12.3`；MySQL `9.0.1` 握手 |
| 监听端口 | **已实测** | 5000/1883/8883/18083/3306 可连；8083/8084 手机超时；见 §3 |
| EMQX TLS 可用性（App 消费端） | **FAIL** | 证书 SAN 仅 117.72.84.210；App `SSLHandshakeException` |
| 近期服务日志 | **NOT EXECUTED** | 无 SSH 只读入口（`existing_service_ssh_inspection` 对本能力不可用）；操作者未提供 |
| 连接数基线 | **NOT EXECUTED** | EMQX 管理 API 401，未使用凭据登录；无其他授权入口 |
| DB 回读（`sensor_data`） | **NOT EXECUTED** | 无 DB 回读授权（设计允许记录缺口） |
| 未修改/未重启/未写入 | **已遵守** | 仅 GET/TCP connect/TLS/只读日志；无 POST/登录/DB/重启 |
| 平台与既有部署一致 | **部分核验** | 服务/端口与迁移记录大体一致；8083/8084 不可达、证书为旧 IP，记录差异 |

---

## 6. 三端身份与关联键（平台段本轮补充）

| 关联键 | 值 | 来源 |
|---|---|---|
| `uid4` | `6A002C00` | 嵌入式 EV-041 `BOOT … uid=6A002C00`（引用，不重判） |
| 传感器构建/下载 | axf md5 `b6a15fed2ed5e3d0c4adc51ac87a4830`；hex md5 `4d728dec9b0d28a352c46436b7586c72`；`mdk_flash` Verify OK；`fw_sensor_FD-002r8.zip` md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf` | EV-041（引用） |
| `gateway_ble_addr` | `84:C2:E4:03:02:02` | 手机段 App 正常扫描 + 保存路径 |
| 网关 `GwID` | `B1:17:9C:7A:4E:E0` | 手机段/本轮日志 22:41:30 |
| APK SHA-256 | `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b`（versionCode 10 / 1.9） | 手机段主机/设备侧一致 |
| 手机 | `12f046690506`，Redmi 8A `olivelite`，Android 10 / API 29 | 手机段；本轮在线复核 |
| Wi-Fi | `ZTE_607157`，默认网络 100（WIFI），IP 192.168.0.103/24 | 手机段；本轮复核 |
| 平台服务身份 | EMQX `emqx@127.0.0.1` running、Dashboard 200、Cowboy；Flask Werkzeug/3.1.8 Python/3.12.3；MySQL 9.0.1（仅握手） | 本轮 §3 |
| EMQX 8883 证书 | subject/SAN `117.72.84.210`；issuer `Tiktok Root CA`；2024-10-12–2026-12-21 | 本轮 `platform_stage_retest_cert.txt` |
| 平台时间窗（本轮） | 2026-11-23T14:39Z–14:44Z | 本报告 |
| 测试身份 | `registerphone` 空、`mac_addr` 空（`000000000000`） | 手机段；P1 上传/持久化 **NOT EXECUTED** |

---

## 7. 未执行项与缺失条件

1. **服务日志**：需 `existing_service_ssh_inspection` 的 `inspect_existing_service_readonly` 授权，或操作者/具备入口的能力提供脱敏只读输出；本轮未执行。
2. **连接数**：EMQX 管理 API 需认证；未使用任何凭据登录；无其他授权入口。未执行。
3. **DB 回读**：无授权；未执行。
4. **8083/8084**：手机路径超时，需服务器/安全组侧确认（记录差异，不判缺陷）。
5. **P1 上传/持久化**：无隔离测试身份；且当前 App→EMQX TLS 已被实测缺陷阻断，P1 `NOT EXECUTED`。

上述缺项不改变本轮已实测的 TLS 缺陷结论；平台段不因缺项改判 PASS。

---

## 8. 证据清单（本轮新增）

| 文件 | 内容 |
|---|---|
| `platform_stage_retest_env.txt` | 设备在线、默认网络/路由、手机探测工具可用性 |
| `platform_stage_retest_network.txt` | `dumpsys wifi`（ZTE_607157）、`ip rule`/`ip route`（默认网络 1027） |
| `platform_stage_retest_8883_phone.txt` | 手机 WAN 对照、8883 TCP、TLS 握手、1883/5000/18083 对照原始输出 |
| `platform_stage_retest_cert.txt` | 服务器证书 subject/issuer/有效期/SAN 与链校验原始输出 |
| `platform_stage_retest_http_phone.txt` | 手机只读 GET（18083 /status、/、5000 /、/login、/upload_data）与 1883 nc |
| `platform_stage_retest_identity.txt` | EMQX API 401、MySQL 9.0.1 握手、8083/8084 超时 |
| `platform_stage_retest_host.txt` | 主机路由、ping、18083 /status、8883 TLS 原始输出 |
| `platform_stage_retest_app_state.txt` | App PID、前台服务、logcat 缓冲信息 |
| `platform_stage_retest_logcat_mqtt.txt` | App MQTT 日志切片（含 TLS 失败栈） |
| `platform_stage_retest_logcat_tls_fail.txt` | 14:43:12Z 最新一轮 TLS 失败与 BLE 正常读取对照 |

上一轮证据（保留）：`platform_stage_baseline.mjs`、`platform_stage_baseline_result.json`、`platform_stage_ping.txt`、`platform_stage_host_network.txt`、`platform_stage_tcp_rtt_probe.txt`、`platform_stage_control_port_probe.txt`、`platform_stage_curl_18083_status.txt`、`platform_stage_curl_5000_upload_data.txt`、`platform_stage_phone_vantage.txt`。

---

## 9. 脱敏与边界声明

- 本轮未读取、未复制任何 SSH/DB/Dashboard/Wi-Fi 凭据；未登录任何服务；证据中无密码明文。
- 平台地址、`uid4`、网关地址/`GwID`、证书主题为设备/服务标识，非凭据。
- 实测缺陷只陈述证书 SAN 与 App 运行异常的可复现证据；不判断实现者能否修复，不扩大为其他组件结论。
- 未执行项不作为通过；P0 整体结论只由各段实际证据形成，当前不能判 PASS。
