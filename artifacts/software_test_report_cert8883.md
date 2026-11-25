# 8883 证书修复后软件独立验证报告（CERT8883 执行轮，含 UPLOAD-04 复测）

- 负责角色：软件测试
- 能力：软件验证
- 执行时间：
  - 第 1 轮（证书与回归）：2026-11-25T06:50Z ~ 07:08Z（CST 14:50 ~ 15:08）
  - 第 2 轮（模拟账号上传复测，本报告新增）：2026-11-25T08:07Z ~ 08:53Z（CST 16:07 ~ 16:53）
- 被测对象：
  - 平台：既有服务器 `iZ2zeih0gmrk47og5r4z7sZ` / `8.140.23.253`，EMQX 5.8.0（`ssl:default` 0.0.0.0:8883），Flask 5000（PID 214002，第 1/2 轮同一进程），MySQL 9.0.1
  - 客户端：Android App `com.jinyuni.dengbei_care` vC10 / vN1.9，设备侧 `base.apk` SHA-256 `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b`（= 主机 `app-debug.apk`，10,370,964 B，安装时间 2026-11-23 21:25）；Redmi 8A / Android 10 / API 29 / serial `12f046690506`
  - 信任锚：`dengbei_care/app/src/main/assets/jd-ca.crt`，证书 SHA-256 指纹 `75:33:09:B3:…:15:2B`（与基线一致，未更换）
- 运行依据：`artifacts/software_test_design_cert8883.md`、`artifacts/cert8883_e2e_plan.md`、`artifacts/cert8883_cases.yaml`
- 本轮性质：独立验证（不修改产品源码；平台侧只读探测 + 经用户明确指示的上传功能验证；未登录 Dashboard/DB、未直连数据库、未重部署、未发送真实短信）

---

## 1. 结果摘要

| 用例 | 结果 | 说明 |
|---|---|---|
| CERT8883-SRV-01 平台证书独立验收 | **PASS** | 叶子证书由 App 信任锚签发，SAN 命中 `8.140.23.253`，serial 已从缺陷基线变更，负例被拒 |
| CERT8883-CLI-02 App TLS / CONNACK | **PASS（证书/TLS 维度）** | 无 SAN/链/信任锚错误，TLS 完成并收到 MQTT CONNACK；返回 `rc=5 无权连接`，属认证层缺口，另记 |
| CERT8883-REG-03 不受影响路径回归 | **PASS（已探测项）** | listener/HTTP/端口语义与基线一致；App 守护运行、BLE 读取 `DATA_OK`、无崩溃/ANR |
| CERT8883-UPLOAD-04 安卓上传 + 持久化回读 | **FAIL（实测 401）** | 按用户指示用模拟测试账号完成登录/设置网关编码/真实 BLE 读数/真实上传路径；平台 `POST /upload_data` 连续返回 **HTTP 401 unauthorized**；无受理行，持久化回读无法执行 |
| E2E-JOINT-P0 受影响联合用例 P0 | **PASS（证书维度）** | 平台段 + 手机段当前版本复核通过；MQTT 会话未建立系既有身份缺口，不归因证书 |
| E2E-JOINT-P1 注入正事件四段 | **NOT EXECUTED** | 缺固件测试构建注入、已确认平台网关 MAC、平台回读授权 |
| CERT8883-FINAL-05 本轮交付整体 | **TEST_FAIL** | C1/C2/C3 通过；C4 实测上传被平台 401 拒绝，必需行为未达成 |

**一句话**：8883 证书修复在平台侧与真实 App 消费端均已被独立证据证实有效（App 无 `No subjectAltNames`，TLS 正常完成）；但“安卓上传到平台并回读”这一必需行为在本轮**实际执行后失败**——真实 App 上传与同身份的直连探测均被平台 `POST /upload_data` 以 **401 unauthorized** 拒绝，且 401 发生在数据库写入路径之前，因此没有可回读的 `sensor_data` 行。该失败**不是证书问题**，也不是本轮证书变更引入的回归；根因在平台上传受理/鉴权条件，需交回实现侧定性。

---

## 2. CERT8883-SRV-01：平台证书独立验收（PASS）

执行命令（`--expect-new`，以 App 当前信任锚为唯一 CA）：

```
bash scripts/emqx_8883_cert_verify.sh 8.140.23.253 8883 \
     dengbei_care/app/src/main/assets/jd-ca.crt --expect-new
```

原始输出：`evidence/software_verify_cert8883_20261125/01_srv_cert_verify.txt`

| 判据 | 实测 | 结论 |
|---|---|---|
| `tls_handshake` | OK | PASS |
| subject / CN | `CN=8.140.23.253` | PASS |
| issuer | `CN=Tiktok Root CA, O=LinkValue`（= App 信任锚） | PASS |
| serial | `77ADCFD924BA0F87F845C81934D4A3F346D4BB83`（≠ 缺陷基线 `…B82`） | `serial_changed=YES` |
| SAN | `IP:8.140.23.253, IP:117.72.84.210` | `san_contains_target=YES` |
| `chain_verify`（仅 jd-ca.crt） | OK | PASS |
| `ip_san_match`（`-verify_ip 8.140.23.253`） | OK | PASS |
| 负例 `-verify_ip 8.140.23.254` | 不通过 | `negative_control=OK` |
| 有效期 | 2026-11-25T02:55:16Z → 2028-11-24T02:55:16Z（早于 CA 2034-10-10） | PASS |
| `overall` | `PASS` | PASS |

第 2 轮平台 HTTP 服务仍由同一 PID 214002 提供（`journalctl` 16:21 记录），未发生重启；证书维度沿用第 1 轮证据，第 2 轮未发现与证书相关的异常。

---

## 3. CERT8883-CLI-02：App 真实消费端 TLS / CONNACK（证书维度 PASS）

第 1 轮已实测（同一 APK/设备）：

| 观测 | 结果 | 证据 |
|---|---|---|
| TLS 主机名错误 `No subjectAltNames` / `SSLHandshakeException` | **0 次** | `13_logcat_after_ble.txt`、`16_logcat_final.txt` |
| 链/信任锚错误 `Trust anchor…` / `PKIX…` | **0 次** | 同上 |
| TLS 是否完成并进入 MQTT CONNECT | 是：Paho 收到 CONNACK 才返回 `rc=5` | `15:01:55` 重连 → `15:01:56` CONNACK |
| CONNACK 实际结果 | `rc=5 无权连接`（`not_authorized`），username `000000000000` | `16_logcat_final.txt` 第 6235–6258 行 |
| 网关 BLE（非 MQTT 路径） | `DATA_OK`，`GwID=B1:17:9C:7A:4E:E0`，`dev=1`，`id=6A002C00`，`T=18.2°C`，`H=35.4%` | `15:00:35`，`13_logcat_after_ble.txt` |

第 2 轮复核：16:36、16:38 等窗口仍为 MQTT `rc=5`（既有认证层缺口），无 TLS 异常；同一 APK/证书未变化。

**判读**：证书缺陷维度 PASS。第 2 轮的上传失败发生在 HTTP 5000 端口，与 8883 TLS/CONNACK 无关。

---

## 4. CERT8883-REG-03：不受影响路径回归（PASS，已探测项）

第 1 轮主机只读探测：`evidence/software_verify_cert8883_20261125/04_regression_probe.txt`；平台只读 SSH 基线：`17_platform_baseline_ssh.txt`。

| 检查 | 实测 | 与基线 |
|---|---|---|
| 8883 TLS（App 信任锚） | TLSv1.3，`peer_cn=8.140.23.253`，链校验 OK | 正常 |
| 1883 / 8083 / 8084 / 18083 / 5000 | 全部 Open | 一致 |
| 3306 | MySQL 9.0.1 握手（未认证、未执行 SQL） | 一致 |
| Dashboard `GET /status` | `200`，`emqx is running` | 一致 |
| Flask `GET /` / `/login` / `/upload_data` | `404` / `405` / `405`（`Allow: POST, OPTIONS`） | 一致 |
| EMQX 服务 | `active`，`NRestarts=0` | 正常 |
| 新增免校验/诊断入口 | 未发现（仅既有端口/路由） | 一致 |
| App 启动/守护/网关读取 | 正常，`DATA_OK`；无崩溃/ANR | 正常 |

第 2 轮新增回归观察：`20_logcat_capture.txt` 全量检索无 `FATAL EXCEPTION` / `ANR in com.jinyuni` / 进程死亡；App 在 401 后按设计保留数据并显示状态，未崩溃。

---

## 5. CERT8883-UPLOAD-04：安卓上传 + 持久化回读（第 2 轮，实测 FAIL）

### 5.1 用户指示与测试身份

用户对上一轮 `HUMAN_INPUT_REQUIRED` 回复 **REASSESS**：“当前不发送真实短信。请用已有测试账号或模拟账号，继续验证安卓上传和平台回读。”

据此本轮使用**模拟测试账号**（未发送任何短信）：

| 项 | 实测 |
|---|---|
| 账号创建 | `POST http://8.140.23.253:5000/register`，`{"phoneNumber":"14000000001","password":"<测试专用口令，见 41_test_run_notes.txt>"}` → `200 {"message":"User registered successfully"}` |
| 号码性质 | `140` 前缀为中国大陆未分配的移动号段，**不是真实用户号码**，不涉及 `user_table` 既有真实数据 |
| 登录（API） | `POST /login` 同凭据 → `200 {"message":"Login successful","token":"some-jwt-token"}` |
| 登录（App 既有路径） | App“我的→账户→登录”输入手机号/密码 → `MyPrefs.registerphone=14000000001`，界面“用户：已登录”（未直接编辑 prefs） |
| 平台网关 MAC | 用户未提供已确认的平台网关 MAC；本轮使用**明确合成的测试 MAC `0200000000A1`**，经 App 既有“网关编码→保存”路径设置，平台 `POST /setmac` → `200`（16:21:22，服务端 `abr.py:169 mysql.connect()`）。该值**不是**由 BLE 地址或网关 GwID 推断得到 |
| 守护/网络 | `measure_state=stopping`（运行中）；默认网络 `ZTE_607157`，`wlan0 192.168.0.103`（network 101） |
| 网关/读数 | `gateway_ble_addr=84:C2:E4:03:02:02`，绑定设备 `6A002C00`（e2ep0）；16:21:29 BLE `DATA_OK`，`GwID=B1:17:9C:7A:4E:E0`、`dev=1`、`id=6A002C00`、`T=18.0°C`、`H=35.8%` |

证据：`21_prefs_after_login.xml`、`23_prefs_after_mac.xml`、`27_platform_http_log_slice.txt`、`36_app_upload_window.txt`、`41_test_run_notes.txt`。

### 5.2 真实 App 上传路径的实测结果

| 时间（CST） | 动作 | 实测 | 证据 |
|---|---|---|---|
| 16:21:29 | 真实 BLE 读数入队（`6A002C00, time=1795594889, 18.0, 35.8`） | 门控打开（`guard=true`、`registerphone` 非空），`CloudUpload` 发起上传 | `36_app_upload_window.txt` 第 45625–45687 行；`26_db_content.txt` |
| 16:21:31 | App `POST /upload_data` | **HTTP 401**；App 分类 `AUTH_REJECTED`，行保留，全局暂停自动上传 | 平台日志 `27_platform_http_log_slice.txt`；prefs `upload_last_status=401` |
| 16:38:29 | 用户点击“重试上传” | 再次 `POST /upload_data` | UI dump `35_ui_upload_401_status.xml` |
| 16:38:31 | 重试结果 | **HTTP 401**；待发 5 条；界面“云端上传被拒绝（HTTP 401），数据已保留（待发 5 条）” | `32_prefs_after_retry.xml`、`34_ui_upload_401.png`、`35_ui_upload_401_status.xml` |

App 本地待发箱实测（`25_bwtemperature_database1.db`）：`pending_uploads` 中 `(6A002C00, 1795594889, 18.0, 35.8, attempts=20)`；本地温度行同样为 `time=1795594889 / 18.0 / 35.8`。即上传载荷来自**真实 BLE 读数**，不是占位或手写数据。

### 5.3 平台收到的请求（只读日志）

`journalctl -u user_script` 16:20–16:45 窗口内共 **5 次** `POST /upload_data`：

| 时间 | 来源 | 状态 |
|---|---|---|
| 16:21:31 | App 真实上传（1 条） | 401 |
| 16:27:17 | 主机直连探测（同账号/同 MAC/同键） | 401 |
| 16:28:57 | 主机直连探测（空 JSON） | 400 `No json data` |
| 16:28:57 | 主机直连探测（未注册手机号） | 401 |
| 16:38:31 | App 用户手动“重试上传”（5 条） | 401 |

零个 200。`/upload_data` 请求期间**没有**出现 `abr.py:222` 的 `mysql.connect()` 弃用告警行（该行在 2026-10-22 历史 200 请求中出现过），说明 401 在进入数据库写入路径之前返回。

### 5.4 直连探针的响应体

| 请求 | 状态/响应 |
|---|---|
| 同账号 + 同 MAC + 同读数 | `401 {"error":"unauthorized"}` |
| 未注册手机号 + 不同 MAC | `401 {"error":"unauthorized"}` |
| 空 JSON | `400 {"error":"No json data"}` |

证据：`28_upload_direct.hdr/body`、`29_upload_empty.hdr/body`、`30_upload_unregistered.hdr/body`。

### 5.5 分析

1. **App 侧行为符合产品设计**：门控打开、真实入队、发起 HTTP、401 后保留数据、暂停自动重试、界面可见、支持手动重试，均与 `CloudUploadRepository`/`PrefsUploadGate` 的 401 分类一致；未发现 App 崩溃、丢数据或绕过门控。
2. **失败点在平台 `POST /upload_data` 受理/鉴权**：注册账号（`/register` 200）、登录（`/login` 200）、设置网关编码（`/setmac` 200）均成功，但上传仍 401；注册与未注册手机号在直连探针中同为 401。
3. **401 发生在 DB 写入之前**：日志无 `abr.py:222 mysql.connect()`，因此不是 `sensor_data` 唯一键冲突或字段级落库失败，也**没有产生可回读的行**。
4. **不能归因于证书修复**：上传走明文 HTTP 5000，8883/TLS 维度第 1/2 轮均正常；平台 HTTP 进程未重启，`/setmac` 可正常访问 MySQL。
5. **根因尚未定性**，竞争假设如下（不经服务端源码/平台侧复核无法排除）：
   - **H1 平台要求额外凭证/会话状态**：`/upload_data` 可能校验 `Authorization`/token 或服务端内存中的“已验证”状态；App 契约不发送任何认证头（`CloudLogUploadRepository` 的拦截器为空），`/login` 返回的 `token` 也未被客户端使用。若成立，所有真实 App 上传都会失败，属平台/客户端契约缺口。
   - **H2 平台要求已确认的网关/身份属性**：例如真实网关的平台 MAC 白名单；本轮合成 MAC `0200000000A1` 不满足。注意 401 在 DB 前返回，`user_table` 查询可能性较低，但不能排除静态白名单/缓存。
   - **H3 模拟账号缺少真实注册路径的验证状态**：本轮账号由 `/register` 直接创建（用户指示不发短信），可能缺少 `/verifyCode` 建立的运行时已验证状态。该假设无法在不发真实短信、不使用真实用户凭据的前提下排除。
6. **持久化回读未执行**：没有 200/受理行，按 `(dev_id,time)` 回读没有对象；本环境也未提供 `sensor_data` 的只读查询入口，数据库直连不在授权内。因此“平台回读字段一致”“同键重试不增行”两个子项均**未执行**，不能判通过。

### 5.6 交回实现/平台的必需信息

- 平台侧需查明 `POST /upload_data` 的 401 受理条件（部署对象 `/root/abr.py` 8949 B，2026-10-22 19:31:04，inode 1088447，进程未重启；本轮未读取其源码）。
- 若 401 需要特定 header/token、内存验证状态或真实网关 MAC，请给出与当前 App 契约一致的最小修复或正式的测试身份条件；客户端不得在拦截器中临时加鉴权头绕过。
- 修复后必须用**同一 App 真实上传路径**重做本用例，并在获准的只读入口回读 `(phone, dev_id, time, temperature, humidity)`，再验证同 `(dev_id,time)` 重试不增行。
- 若需要已确认的平台网关 MAC 或经过验证的真实测试账号，请用户提供；本轮未擅自使用真实用户/设备凭据。

### 5.7 测试数据与残留状态（披露）

- App 当前仍登录测试账号 `14000000001`，`mac_addr=0200000000A1`，守护运行，待发箱保留 5 条真实读数（因 401）。
- 平台 `user_table` 新增该模拟账号；本轮无删除/DB 授权，未清理。该账号与 `0200000000A1` 均为测试专用，不是真实用户或真实网关。
- 未发送短信；未直连数据库；未登录 Dashboard；未修改服务端配置/服务；SSH 仅读取近期 `journalctl`。
- 如需恢复现场：App “退出”清除 `registerphone`，网关编码可清除；平台账号需由有 DB 授权的角色处理。

---

## 6. E2E-JOINT-P0 / P1

### E2E-JOINT-P0：PASS（证书维度）

- 三端身份：传感器 `uid=6A002C00`（FW FD-002r8，引用既有同版本证据）；网关 BLE `84:C2:E4:03:02:02` / `GwID=B1:17:9C:7A:4E:E0`；APK/设备见封面。
- 手机段第 1/2 轮复核：设备在线、APK 摘要一致、`ZTE_607157` 默认、守护运行、网关读取 `DATA_OK`。
- 平台段：CERT8883-SRV-01 / CLI-02 / REG-03 的实际结果；平台服务/端口/日志/连接数经只读 SSH 独立记录。
- CONNACK `rc=5` 按认证层分类记录；P0 的目标“证明证书修复解除 App→EMQX TLS 阻断”成立。**不得**把 P0 PASS 写成 MQTT 会话已建立或上传已通过。

### E2E-JOINT-P1：NOT EXECUTED

缺：固件测试构建（`measure_sample()` 后注入 `367`、默认关闭、正式包排除）、已确认平台网关 MAC、平台写与回读授权。第 2 轮的 401 进一步说明上传段目前不可用，故整例不判通过、不部分通过。

---

## 7. 本轮测试环境动作披露

第 1 轮：
- 为获得干净 TLS 窗口，对 App 执行 `force-stop` 后重新启动（未卸载、未清数据、未降级、未改动源码或 APK）。
- 手机唤醒/上滑解锁（测试手机操作），通过首页实际按钮“启动”恢复守护运行。
- 未修改 `MyPrefs` 身份字段；未写平台数据；未登录 Dashboard/DB；未调用写接口。

第 2 轮：
- 关闭屏幕状态下唤醒/解锁，通过 App 实际界面完成登录、网关编码保存、上传状态查看和“重试上传”。
- 未直接编辑 `MyPrefs`；未卸载、清数据、降级或改 APK；未使用真实用户凭据；未发送短信。
- 经用户 REASSESS 指示，使用产品既有 `POST /register` 创建模拟账号（非短信验证路径），并执行真实上传与有限次直连探针（见 §5.3/§5.4）；空 JSON/未注册号探针均被拒，未产生额外写入。
- 平台侧仅执行状态/日志读取和上述 HTTP 功能请求；未连数据库、未改配置、未重启服务。
- `adb kill-server` 仅用于结束本机 logcat 采集，不改变手机状态。

---

## 8. 边界与未执行

1. 真实物理 >35℃ / 升温+无光 / 光强 / 功耗 / 433 电气：本轮无条件，未执行，不用模拟代替。
2. release/生产签名、iOS、API 31+、9 台真实快照：未产出/无设备，不在本轮。
3. `sensor_data` 行级回读与幂等不增行：因 401 无受理行而未执行；数据库直连不在授权内。
4. 平台 `/upload_data` 401 根因（header/token/内存验证/网关白名单）：未读取服务端源码，未定性。
5. 本轮结论仅覆盖“证书修复维度 + 已执行回归 + 模拟身份上传实测”，不代表完整产品验收通过。

---

## 9. 证据索引

### 第 1 轮目录：`evidence/software_verify_cert8883_20261125/`

| 文件 | 内容 |
|---|---|
| `01_srv_cert_verify.txt` | 8883 叶子证书握手、链、SAN、IP 校验、负例、`overall=PASS` |
| `02_logcat_raw.txt` | 修复后同版本 App 早段窗口（0 TLS 错误，CONNACK rc=5） |
| `03_myprefs.xml` | 设备端 `MyPrefs` 实测（第 1 轮：无 `registerphone`/`mac_addr`，`measure_state=stopping`） |
| `04_regression_probe.txt` | 主机侧端口/TLS/HTTP/MySQL 回归探测 |
| `05_logcat_clean_window.txt` ~ `16_logcat_final.txt` | App 重启、守护启动、BLE `DATA_OK`、CONNACK、UI/通知状态 |
| `08_app_ui.png`、`10_app_ui_guard_started.png` | App 首页实际界面 |
| `14_ui_dump_now.xml` | 当前 UI 控件树 |
| `17_platform_baseline_ssh.txt` | 平台服务/监听器/连接数/Flask 日志只读基线 |
| `18_sim_state.txt` | 测试手机 SIM/电话状态（无 SIM，说明不能收注册验证码） |
| `regression_probe.py`、`platform_baseline_ssh.py`、`extract_ui_text.py` | 第 1 轮可复跑脚本 |

### 第 2 轮目录：`evidence/software_verify_cert8883_r2_20261125/`

| 文件 | 内容 |
|---|---|
| `01_pkg_dump.txt` | 包版本 vC10/1.9、codePath、lastUpdateTime |
| `02_prefs.xml` | 第 2 轮起始 prefs（无 registerphone/mac_addr，守护运行） |
| `03_connectivity.txt`、`39_wifi_dump.txt`、`40_wlan0.txt` | 默认网络 `ZTE_607157`、wlan0 `192.168.0.103` |
| `05_root_get.body`、`06_register_options.hdr` | HTTP 可达性与路由存在性 |
| `07_register.hdr/body` | 模拟账号创建 `200 User registered successfully` |
| `08_login.hdr/body` | 模拟账号登录 `200 Login successful` |
| `11_ui_home.xml`、`12_ui_mine.xml`、`13_ui_login.xml`、`14_ui_after_back.xml` | App 实际 UI 控件树（首页/我的/登录） |
| `20_logcat_capture.txt` | 第 2 轮完整 logcat 原始采集（16:07–16:53） |
| `21_prefs_after_login.xml`、`23_prefs_after_mac.xml`、`32_prefs_after_retry.xml` | 登录/设置网关编码/重试后的 prefs 实测 |
| `24_db_list.txt`、`25_bwtemperature_database1.db`、`26_db_content.txt` | 本地库与待发箱行（真实读数与 401 保留状态） |
| `27_platform_http_log_slice.txt`、`33_platform_http_log_upload_window.txt` | 平台 HTTP 服务只读日志窗口（登录/setmac/upload 401） |
| `28_upload_direct.*`、`29_upload_empty.*`、`30_upload_unregistered.*` | 直连探针状态/响应体 |
| `31_ui_gateway_dialog.xml`、`34_ui_upload_401.png`、`35_ui_upload_401_status.xml` | “云端上传被拒绝（HTTP 401），数据已保留”界面证据 |
| `36_app_upload_window.txt` | App 上传窗口的 MqttService/CloudUpload 日志切片 |
| `38_apk_sha256.txt` | 设备侧 `base.apk` SHA-256（与主机一致） |
| `41_test_run_notes.txt` | 本轮身份、读数、请求、残留状态的完整运行记录 |
| `platform_log_slice_ssh.py`、`platform_log_slice2_ssh.py`、`extract_upload_window.py` | 本轮可复跑脚本（只读日志/本地切片） |

---

## 10. 结论与下一步

1. **证书修复本身：PASS**。平台 8883 证书由 App 信任锚签发、SAN/IP 正确、负例被拒；真实 App 不再出现 `No subjectAltNames`，TLS 完成并收到 CONNACK。
2. **安卓上传到平台：FAIL（实测）**。真实 App 使用模拟测试账号完成登录/网关编码设置/真实 BLE 读数上传，平台 `POST /upload_data` 连续 401；同身份直连探针同为 401；未注册号也 401；401 发生在数据库写入前。
3. **持久化回读：NOT EXECUTED**。没有受理行可回读，且无授权的 `sensor_data` 只读入口；不能以 401 或 HTTP 2xx 替代回读。
4. **必需下一步**：
   - 平台/实现侧查明 `/upload_data` 受理条件（源码、header/token、内存验证状态或网关身份），给出与 App 契约一致的最小修复；
   - 若需要已确认平台网关 MAC 或经过验证的真实测试账号，由用户提供；
   - 修复后用同一 App 真实路径重跑 C4，并按获准只读入口回读字段、验证幂等；
   - 回归范围保持 8883 证书与不受影响路径不变。
5. 本报告不判断实现者能否修复；按契约返回 **TEST_FAIL**，失败证据与竞争假设已给出。
