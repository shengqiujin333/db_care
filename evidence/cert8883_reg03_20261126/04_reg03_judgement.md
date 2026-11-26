# CERT8883-REG-03 不受影响路径回归判定（只读）

- 用例：`CERT8883-REG-03`（TD-CERT8883-001 §3.3，`case_kind=regression_unaffected_paths`）
- 阶段：`platform_host_readonly` + `phone_readonly`
- 执行时间窗（UTC）：2026-11-26T03:50:49Z ~ 2026-11-26T03:57:00Z（CST 11:50 ~ 11:57）
- 被测对象：既有平台 `8.140.23.253` 的 EMQX 5.8.0 listener、Dashboard 18083、Flask 5000、MySQL 3306；以及 Android App `com.jinyuni.dengbei_care` 当前运行时（AB-003，未重建）
- 被测版本身份：
  - 平台服务：EMQX 5.8.0（18083 `/status`：`Node emqx@127.0.0.1 is started` / `emqx is running`）；Flask `Werkzeug/3.1.8 Python/3.12.3`；MySQL 9.0.1（仅握手）
  - 8883 叶子证书：serial `77ADCFD924BA0F87F845C81934D4A3F346D4BB83`，issuer `CN=Tiktok Root CA`，SAN `IP:8.140.23.253, IP:117.72.84.210`，`Verify return code: 0 (ok)`（与 `CERT8883-SRV-01` 第 5 轮 PASS 一致）
  - App：versionCode 10 / versionName 1.9；APK SHA-256 主机侧与设备侧均为 `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b`
  - 手机：Redmi 8A / Android 10 / API 29 / serial `12f046690506`；Wi-Fi `ZTE_607157`，wlan0 `192.168.0.103/24`
- 方式边界：仅 TCP connect、TLS 握手（以项目 `jd-ca.crt` 校验）、明文 GET、MySQL 问候读取；**未 POST、未登录 Dashboard/SSH/DB、未写目标文件、未重启服务**。本地仅写入本证据目录与报告。

---

## 0. 测量有效性（先说明，避免误用）

在本 Windows 主机上做 connect-only 端口扫描**不可信**：控制端口 1 / 9 / 9999 / 65000 也全部返回 `OPEN`（约 0.01 s），说明主机侧存在透明 TUN/代理本地应答（见 `37_host_scan_control.txt`）。因此：

- 主机侧 **`35_bounded_port_scan.txt` 仅作过程记录，不作为可达性证据**；
- 外部可达性以**手机侧**（真实 `ZTE_607157` Wi-Fi，`192.168.0.103`）的 `nc` 探测为准（`40_phone_port_probe.txt`），对照 `9999` 不可达确认该方法能区分开放/关闭；
- 主机侧 TLS/HTTP 结果因完成真实协议交互（证书校验、HTTP 头/体、MySQL 问候）仍有效。

---

## 1. 逐项判据与实测

| # | acceptance 判据 | 实测（原始证据） | 结果 |
|---|---|---|---|
| 1 | 8883 仍为 TLS listener（由 SRV-01 握手证据支持） | `openssl s_client -connect 8.140.23.253:8883 -CAfile jd-ca.crt -verify_ip 8.140.23.253` → `New, TLSv1.3, Cipher is TLS_AES_256_GCM_SHA384`、`Verification: OK`、`Verify return code: 0 (ok)`；SRV-01 r5 serial/SAN 一致（`31`/`38`/`39`） | PASS |
| 2 | 1883 TCP 可连 | 手机 `nc 8.140.23.253 1883` → `rc=0`（`40`） | PASS |
| 2 | 18083 `GET /status` 200 且 body 表明 emqx 运行 | 200，body `Node emqx@127.0.0.1 is started\nemqx is running`（`32`） | PASS |
| 2 | 5000 `GET /` 404；`GET /login`、`GET /upload_data` 405 且 Allow 为 POST/OPTIONS | `/` → 404；`/login` → 405 `Allow: OPTIONS, POST`；`/upload_data` → 405 `Allow: OPTIONS, POST`（`32`） | PASS（见 §2 差异 1） |
| 3 | 3306 返回 MySQL 握手并可辨识版本；不认证、不执行 SQL、不读库 | 77 字节问候：`protocol_version=10`、`server_version='9.0.1'`（`34`） | PASS |
| 4 | 8083/8084 若仍与既有手机侧超时一致，记录为既有部署差异 | 手机 `nc 8083` `rc=1 dt=2s`、`nc 8084` `rc=1 dt=3s`，与 2026-11-23 基线 `nc: Timeout rc=1` 一致（`40`；基线 `platform_stage_retest_identity.txt`） | PASS（既有差异） |
| 5 | 未发现新增公网免校验/诊断端口或放宽的鉴权边界 | 手机对文档端口、常见诊断端口及服务器侧既有监听集合的有界探测：可达 `22, 80, 443, 1883, 8883, 18083, 5000, 3306, 21115, 21116, 21117`；其余（含 2375/2376、3000、5432、6379、9200、11211、15672、27017、21118/21119、32969、44865 等）均不可达；`/api/v5/nodes` 仍 401 `WWW-Authenticate: Basic`；80/443 为既有 Caddy（80→308→https；443 TLS internal error、无站点）；21115-21117 接受 TCP 但对 HTTP 探测无响应，身份未识别，属既有主机级监听（见 §2/§4），证书替换只改 EMQX 证书文件并重启 EMQX，不能新开这些无关端口（`32`/`40`/`41`/`50`/`51`/`37`） | PASS（就本次变更可归因范围；21115-21117 既有暴露另行披露） |
| 5 | 未发生 POST、登录 Dashboard/SSH/DB、写目标文件或重启服务 | 本轮命令仅 connect/TLS/GET/问候读取；脚本 `reg03_probe.py`、`host_scan_control.py` 可复核；无 POST/凭据/SSH/DB 调用 | PASS（自证+方法） |
| 6 | App 能正常启动/守护运行；至少一次网关读取进入明确终态；无新增崩溃/ANR | PID `14224`（11:28:53 由 `MainActivity` 启动）持续存活；`MqtttService` `isForeground=true`；`MainActivity` 为 `mResumedActivity`；本轮窗口内 `link_state=DATA_OK`（11:51:17）与 `link_state=CONNECT_FAILED`（11:54:59）两类明确终态，`CloudUpload: uploaded 1 reading(s)`；窗口内 `AndroidRuntime`/`FATAL EXCEPTION`/`ANR` 均 0；crash buffer 仅 11-25 22:00 旧崩溃（PID 31852，已由可靠性证据记录），当前 PID 无新崩溃（`10`/`11`/`13`/`16`/`30`/`45`/`47`/`48`/`49`） | PASS |
| 7 | 与既有基线逐项列出差异；仅可归因于本次证书变更的回归判 FAIL | 见 §2；无可归因于本次证书变更的回归 | PASS |

---

## 2. 与既有基线逐项差异

基线来源：`evidence/e2e_p0/platform_stage_retest_http_phone.txt`、`platform_stage_retest_identity.txt`、`platform_stage_retest_host.txt`（2026-11-23）；`evidence/e2e_p0/devops_8883_cert_20261125/00_pre_change_baseline.txt`、`07_post_change_state.txt`（2026-11-25）；`evidence/e2e_p0/devops_reliability_upload_contract_20261126/19_r4_platform_window.txt`（2026-11-26 05:22Z，服务器侧）。

| 项 | 既有基线 | 本次实测（2026-11-26 03:52Z） | 差异与归因 |
|---|---|---|---|
| 18083 `GET /status` | 200，`emqx is running` | 200，`emqx is running` | 无差异 |
| 18083 `GET /` | 200 EMQX Dashboard | 200 EMQX Dashboard | 无差异；未登录 |
| 18083 `/api/v5/nodes` | 401 `AUTHORIZATION_HEADER_ERROR` | 401 同左 | 无差异，鉴权边界未放宽 |
| 5000 `GET /` | 404 | 404 | 无差异 |
| 5000 `GET /login` | 405 `Allow: POST, OPTIONS` | 405 `Allow: OPTIONS, POST` | **仅 Allow 方法顺序不同，方法集合相同**；不由证书替换引起（`abr.py` 于 2026-11-25 21:59 因上传契约修复重新部署，见可靠性证据），不判回归 |
| 5000 `GET /upload_data` | 405 `Allow: POST, OPTIONS` | 405 `Allow: OPTIONS, POST` | 同上 |
| 1883 | 手机 `nc` rc=0 | 手机 `nc` rc=0 | 无差异 |
| 8883 | 基线为旧证书 serial `…B82`、SAN 仅旧 IP | serial `…B83`、SAN 含 `8.140.23.253`、TLSv1.3、链校验 OK | **本次证书修复的预期变化**（SRV-01 已判定），本次仅复核仍为 TLS listener |
| 8083 / 8084（手机侧） | `nc: Timeout` rc=1 | rc=1（2~3 s 超时） | 与基线一致，**既有部署/网络差异**（服务器侧 listener 仍 running，见部署证据），不判回归 |
| 3306 | 握手显示 9.0.1 | 握手显示 9.0.1 | 无差异；未认证、未执行 SQL |
| 公网 80/443 | 迁移记录/拓扑未列为项目入口 | 80 Caddy 308→https；443 TLS internal error（无站点） | 既有主机级 Caddy，证书替换只改 EMQX 证书文件并重启 EMQX，不会新增/改变 Caddy 监听；不归因本次变更 |
| 公网 21115/21116/21117 | 既有服务器侧监听集合包含（2026-11-26 05:22Z `ss` 快照）；非项目文档入口 | 手机侧 TCP 可达；HTTP GET 无响应，身份未识别 | 既有主机级监听，非 EMQX/Flask/MySQL，也不由证书文件替换产生；**作为既有外部暴露如实记录，不归因本次变更**，建议安全/运维另行核查 |
| App 运行 | 既有手机段：DATA_OK 等终态 | DATA_OK（11:51:17）+ CONNECT_FAILED（11:54:59）；MQTT CONNACK `无权连接 (5)` | App/APK 未变化；CONNACK rc=5 为既有认证缺口（CLI-02 已记录），非本回归新增 |
| 崩溃/ANR | — | 本窗口 0 新增；crash buffer 仅 11-25 22:00 旧崩溃 | 无新增 |

---

## 3. 未执行与限制（不冒充通过）

1. **8083/8084 协议级语义未独立探测**：本轮手机侧仍不可达（与基线一致），未做 WS/WSS/MQTT-over-WS 握手；其“running”仅引用既有部署证据，不作为本次功能通过。
2. **1883 仅验证 TCP 可连**：未发送 MQTT CONNECT、未使用任何凭据；acceptance 只要求 TCP 可连。
3. **公网端口检查为有界扫描**：手机侧覆盖文档端口、常见诊断端口以及 2026-11-26 05:22Z 服务器侧 `ss` 监听集合（含 21115-21119、32969、44865）；未做 1–65535 全端口扫描。21115/21116/21117 仅证明 TCP 可达，HTTP 探测无响应，**服务身份未识别**，已作为既有暴露另行披露。主机 connect-only 因本地 TUN 拦截被判无效。
4. **未登录**：未登录 Dashboard/SSH/数据库，未 POST、未写目标文件、未重启服务；因此服务器内部配置文件级别比对不在本轮证据内，引用部署能力既有证据。
5. **未重启/未启动 App**：App 为 11:28:53 启动的既有会话（PID 14224），本轮只做只读观察；没有执行 UI 点击或业务操作。
6. `ui_dump` 中 `11:55:44`/`11:56:03` 的 `AndroidRuntime START ... com.wetest.uia2.Main` 是测试工具 uiautomator2 自身进程，不是 App 崩溃。

---

## 4. 结论

`CERT8883-REG-03` 在本时间窗与本证据范围内 **PASS**：

- 8883 仍为 TLS listener；1883 TCP 可连；18083 `/status` 200 且表明 emqx 运行；5000 三个路由 GET 行为与既有基线语义一致；3306 握手可辨识 MySQL 9.0.1；
- 8083/8084 手机侧超时与既有基线一致，记录为既有部署差异，不归因本次证书变更；
- 未发现由本次变更新增的公网免校验/诊断入口或放宽的鉴权边界（有界探测）；既有 80/443 Caddy 与 21115/21116/21117 公网 TCP 监听（身份未识别）已作为既有暴露单独披露，非证书替换引入；未发生 POST、登录、目标写入或服务重启；
- App 守护运行正常，本轮至少取得 `DATA_OK` 与 `CONNECT_FAILED` 两个明确终态，无新增崩溃/ANR；
- 与基线文本的差异仅为 5000 `Allow` 方法顺序，方法集合一致且与证书变更无因果关系；其余差异均为既有条件。

本结论只覆盖 `CERT8883-REG-03` 的回归范围，不改变 `CERT8883-CLI-02` 的证书维度结论，也不代表 MQTT 认证缺口、上传持久化或整体交付通过。

## 5. 证据索引（`evidence/cert8883_reg03_20261126/`）

| 文件 | 内容 |
|---|---|
| `00_precheck_identity.txt` | UTC/主机/git HEAD/工具版本/APK 主机侧与设备侧摘要/基线引用 |
| `10_phone_identity.txt` | 设备型号/Android/API/serial/PID/包信息 |
| `11_services.txt` | `MqtttService` 前台与 `MqttService` 绑定 |
| `12_ps_A.txt` | 进程列表（App PID 14224） |
| `13_logcat_since_1140.txt` / `48_logcat_since_1151.txt` | 手机运行窗口原始 logcat |
| `14_crash_buffer.txt` / `49_crash_buffer_end.txt` | crash buffer（仅 11-25 旧崩溃） |
| `15_events_recent.txt` | events 缓冲 |
| `16_activities.txt` / `17_window.txt` | `MainActivity` resumed 与窗口状态 |
| `19_connectivity.txt` / `43_phone_network.txt` / `44_wifi_head.txt` / `46_wifi_full.txt` | Wi-Fi `ZTE_607157`、wlan0 地址 |
| `21_dumpsys_package.txt` | versionCode/versionName、权限 |
| `30_phone_scan.txt` | 窗口扫描：崩溃/ANR/TLS 错误 0，终态与上传行 |
| `31_tls_probe.txt` / `38_openssl_s_client.txt` / `39_jdca_fingerprint.txt` | TLS 握手与 CA 身份 |
| `32_http_probes.txt` | 18083/5000 GET-only 原始响应 |
| `33_tcp_connect.txt` | 主机 TCP connect（过程记录） |
| `34_mysql_greeting.txt` | MySQL 9.0.1 握手 |
| `35_bounded_port_scan.txt` / `37_host_scan_control.txt` | 主机有界扫描与其**无效性控制** |
| `40_phone_port_probe.txt` | 手机侧有界端口可达性（权威） |
| `41_web_ports_identity.txt` | 80/443 Caddy 身份 |
| `50_phone_remaining_ports.txt` / `51_high_ports_identity.txt` | 21115-21119/32969/44865 手机侧可达性；21115-21117 身份识别尝试（无响应） |
| `42_foreground_state.txt` | 前台/网络状态汇总 |
| `45_ui_dump.xml` / `47_ui_screenshot.png` | UI 状态（`网关连接失败（重试 / 检查网关电源）`、21.9°C） |
| `reg03_probe.py` / `host_scan_control.py` | 可复跑只读探测脚本 |
