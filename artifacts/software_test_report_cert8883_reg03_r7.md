# 平台证书替换后的不受影响路径回归报告（CERT8883-REG-03，第 7 轮）

- 负责角色：软件测试
- 能力：软件验证
- 用例：`CERT8883-REG-03`（TD-CERT8883-001 §3.3，`case_kind=regression_unaffected_paths`，阶段 `platform_host_readonly` + `phone_readonly`）
- 执行时间窗（UTC）：2026-11-26T03:50:49Z ~ 2026-11-26T03:57:00Z（CST 11:50 ~ 11:57）
- 被测对象：既有平台 `8.140.23.253` 的 EMQX/Flask/MySQL 端口与路由语义；Android App `com.jinyuni.dengbei_care` 当前运行时
- 被测版本身份：EMQX 5.8.0；Flask `Werkzeug/3.1.8 Python/3.12.3`；MySQL 9.0.1（仅握手）；8883 叶子 serial `77ADCFD924BA0F87F845C81934D4A3F346D4BB83`、SAN `IP:8.140.23.253, IP:117.72.84.210`、issuer `Tiktok Root CA`；App AB-003 vC10/vN1.9，APK SHA-256 `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b`（主机侧与设备侧一致）
- 结果：**TEST_PASS（仅限 CERT8883-REG-03 的回归范围）**
- 方式边界：仅 TCP connect、TLS 握手（项目 `jd-ca.crt` 校验）、明文 GET、MySQL 问候读取；未 POST、未登录 Dashboard/SSH/DB、未写目标文件、未重启服务。

---

## 1. 结论

在声明的同一时间窗内，证书替换后：

1. `8.140.23.253:8883` 仍为 TLS listener：`TLSv1.3 / TLS_AES_256_GCM_SHA384`，以 `jd-ca.crt` 校验 `Verification: OK`、`Verify return code: 0 (ok)`，与 `CERT8883-SRV-01` 第 5 轮 PASS 的 serial/SAN 一致。
2. `1883` 从手机侧 TCP 可连（`nc rc=0`）；`18083 GET /status` 返回 200 且 body 为 `Node emqx@127.0.0.1 is started` / `emqx is running`；`5000 GET /` 为 404，`GET /login` 与 `GET /upload_data` 均为 405 且允许方法集合为 `{OPTIONS, POST}`；`3306` 返回 MySQL 问候（protocol 10，server version `9.0.1`），未认证、未执行 SQL。
3. `8083/8084` 手机侧仍超时（`rc=1`，2~3 s），与 2026-11-23 基线一致，记录为**既有部署/网络差异**，不归因于本次证书变更。
4. 有界公网探测未发现**由本次变更新增**的免校验/诊断入口：手机侧覆盖文档端口、常见诊断端口及服务器侧既有监听集合，可达 `22, 80, 443, 1883, 8883, 18083, 5000, 3306, 21115, 21116, 21117`；`80/443` 为既有 Caddy（80→308→https，443 无站点），`22` 为需认证的 SSH；`18083 /api/v5/nodes` 仍返回 401，`5000` 路由仍为 POST-only。**既有 21115/21116/21117 公网 TCP 监听接受连接但对 HTTP 无响应、身份未识别**，属既有主机级监听，不是证书替换引入，已单独披露供安全/运维核查。
5. App 守护运行正常：PID `14224` 自 11:28:53 存活，`MqtttService isForeground=true`、`MainActivity` 为 resumed；本轮窗口取得 `DATA_OK`（11:51:17）与 `CONNECT_FAILED`（11:54:59）两个明确终态，`CloudUpload: uploaded 1 reading(s)`；窗口内 `AndroidRuntime`/`FATAL`/`ANR` 均为 0。
6. 与既有基线逐项差异见 §3；**未发现可归因于本次证书变更的回归**。

## 2. 测量有效性说明（重要）

本 Windows 主机存在透明 TUN/代理本地应答：控制端口 `1/9/9999/65000` 也全部在约 0.01 s 返回 `OPEN`（`37_host_scan_control.txt`）。因此主机 connect-only 扫描（`35_bounded_port_scan.txt`）**不作为可达性证据**；外部可达性以手机侧真实 Wi-Fi 探测为准（`40_phone_port_probe.txt`，对照 `9999` 不可达）。主机侧 TLS/HTTP/MySQL 结果因完成真实协议交互仍有效。

## 3. 与既有基线逐项差异

| 项 | 既有基线 | 本次实测 | 差异与归因 |
|---|---|---|---|
| 18083 `/status` | 200 `emqx is running` | 200 同左 | 无差异 |
| 18083 `/` | 200 Dashboard | 200 Dashboard | 无差异；未登录 |
| 18083 `/api/v5/nodes` | 401 | 401 | 鉴权边界未放宽 |
| 5000 `/` | 404 | 404 | 无差异 |
| 5000 `/login`、`/upload_data` | 405 `Allow: POST, OPTIONS` | 405 `Allow: OPTIONS, POST` | 仅方法顺序不同、集合相同；与证书变更无因果关系（`abr.py` 于 2026-11-25 21:59 因上传契约修复重新部署，另案记录） |
| 1883 | 手机 `nc` rc=0 | 手机 `nc` rc=0 | 无差异 |
| 8883 | 旧 serial `…B82`、SAN 仅旧 IP | serial `…B83`、SAN 含新 IP、TLSv1.3、链 OK | 本次证书修复的预期变化；本轮只复核仍为 TLS listener |
| 8083/8084（手机侧） | `nc: Timeout` rc=1 | rc=1（2~3 s） | 与基线一致，既有部署差异 |
| 3306 | 9.0.1 握手 | 9.0.1 握手 | 无差异 |
| 公网 80/443 | 未列为项目入口 | Caddy 308→https；443 TLS internal error | 既有主机级服务；证书替换不新增/改变 Caddy 监听，不归因本次变更 |
| 公网 21115/21116/21117 | 既有服务器侧监听集合包含；非项目文档入口 | 手机侧 TCP 可达；HTTP GET 无响应、身份未识别 | 既有主机级监听，非 EMQX/Flask/MySQL，也不由证书文件替换产生；作为既有外部暴露记录，建议另行核查，不归因本次变更 |
| App 运行 | 既有 DATA_OK 等终态 | DATA_OK + CONNECT_FAILED；CONNACK `无权连接 (5)` | App/APK 未变；rc=5 为既有认证缺口（CLI-02 已记录） |
| 崩溃/ANR | — | 本窗口 0；crash buffer 仅 11-25 22:00 旧崩溃（PID 31852） | 无新增 |

## 4. 未执行与限制

1. 8083/8084 手机侧仍不可达，未做 WS/WSS/MQTT-over-WS 协议级握手；其 running 仅引用既有部署证据。
2. 1883 仅验证 TCP 可连，未发送 MQTT CONNECT、未使用凭据。
3. 公网端口检查为手机侧有界扫描：覆盖文档端口、常见诊断端口及服务器侧既有监听集合（含 21115-21119、32969、44865），未做 1–65535 全端口扫描；21115/21116/21117 仅证明 TCP 可达，HTTP 无响应、身份未识别，已作为既有暴露披露；主机 connect-only 因 TUN 拦截判无效。
4. 未登录 Dashboard/SSH/DB，未 POST、未写目标文件、未重启服务；服务器内部配置文件级比对不在本轮证据内。
5. 未启动/未操作 App，App 为 11:28:53 启动的既有会话，本轮仅只读观察。
6. `11:55:44`/`11:56:03` 的 `AndroidRuntime START ... com.wetest.uia2.Main` 为 uiautomator2 测试工具进程，不是 App 崩溃。

## 5. 证据索引

- 判定表与完整原始输出：`evidence/cert8883_reg03_20261126/04_reg03_judgement.md`
- 平台：`31_tls_probe.txt`、`32_http_probes.txt`、`34_mysql_greeting.txt`、`37_host_scan_control.txt`、`38_openssl_s_client.txt`、`39_jdca_fingerprint.txt`、`40_phone_port_probe.txt`、`41_web_ports_identity.txt`、`50_phone_remaining_ports.txt`、`51_high_ports_identity.txt`
- 手机：`10_phone_identity.txt`、`11_services.txt`、`13_logcat_since_1140.txt`、`16_activities.txt`、`30_phone_scan.txt`、`45_ui_dump.xml`、`47_ui_screenshot.png`、`48_logcat_since_1151.txt`
- 环境：`00_precheck_identity.txt`、`43_phone_network.txt`、`46_wifi_full.txt`
- 可复跑脚本：`reg03_probe.py`、`host_scan_control.py`

## 6. 结论边界

本 PASS 只证明 `CERT8883-REG-03` 在上述时间窗、版本与有界探测范围内，证书替换未使其他 listener、HTTP 路由、MySQL 端口语义、鉴权边界或 App 非 MQTT 运行路径发生可归因回归。它不改变 `CERT8883-CLI-02` 的证书维度结论，也不代表 MQTT 认证缺口、上传持久化、P0/P1 或整体交付通过。
