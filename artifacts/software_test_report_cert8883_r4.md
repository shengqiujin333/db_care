# 8883 证书修复后软件独立验证报告（CERT8883 第 4 轮：人工范围确认后的最终复验）

- 负责角色：软件测试
- 能力：软件验证
- 执行时间：2026-11-26 10:33 ~ 10:56 CST（2026-11-26T02:33Z ~ 02:56Z）
- 报告状态：**证书修复维度与安卓→平台上传受理维度在当前版本实测通过；无该修复路径上的实测缺陷。E2E-JOINT-P1 未执行且未获得“本阶段不纳入”的明确批准，按已批准测试设计 CERT8883-FINAL-05，本轮交付整体为 `TEST_FAIL`（`failure_cause=EXTERNAL_CONDITION_UNEXECUTED`），不是产品缺陷。**
- 被测对象（当前版本，未更换）：
  - 平台：既有服务器 `8.140.23.253`（`iZ2zeih0gmrk47og5r4z7sZ`）
    - EMQX 5.8.0，`ssl:default` 0.0.0.0:8883（监听进程 `beam.smp` pid 299216）
    - Flask `user_script.service` 0.0.0.0:5000；`ActiveState=active`、PID 312368、启动 2026-11-25 21:59:02 CST、`NRestarts=0`
    - 部署对象 `/root/abr.py`，8,688 B，sha256 `a062b05f524dc8f6bce596e4571d339f697ef909b318503ad418ee0add24017c`（= 已批准候选 `deploy/platform_upload_contract/container.yaml` v2；上传门禁 `X-Upload-Token` 已移除）
  - 客户端：Android App `com.jinyuni.dengbei_care` vC10 / vN1.9；设备侧 `base.apk` SHA-256 `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b`；`lastUpdateTime=2026-11-23 21:25:13`（与第 3 轮一致，未重装）
  - 手机：Redmi 8A `olivelite`，Android 10 / API 29，serial `12f046690506`，默认网络 `ZTE_607157`，wlan0 `192.168.0.103`
  - 信任锚：`dengbei_care/app/src/main/assets/jd-ca.crt`，X.509 SHA-256 指纹 `75:33:09:B3:9F:1B:8F:85:B4:D7:57:76:BF:D3:8A:42:52:0D:C1:A0:5E:A2:0B:A3:11:7E:B3:9C:BA:B2:15:2B`（**未更换**），文件 SHA-256 `5c17cf740ffdf416c2f31f90172ad3565966cd62c0952920b9b2ac974aefe1e4`
- 运行依据：`artifacts/software_test_design_cert8883.md`、测试设计用例清单（CERT8883-SRV-01/CLI-02/REG-03/UPLOAD-04、E2E-JOINT-P0/P1、FINAL-05）、`evidence/e2e_p0/devops_platform_upload_contract_20261125/DEPLOYMENT_EVIDENCE.md`
- 本轮性质：独立验证；未修改产品源码/APK/平台配置；未重部署、未重启服务；未登录 Dashboard/数据库、未直连数据库；平台侧仅只读 SSH 与上传接口功能请求；手机侧仅启动 App、恢复守护与读取/上传观察，未安装、未卸载、未清数据

---

## 1. 结果摘要

| 用例 | 结果 | 说明 |
|---|---|---|
| CERT8883-SRV-01 平台证书独立验收 | **PASS** | 本轮 02:33Z 复验证书由 App 信任锚签发，SAN 含 `8.140.23.253`，serial 已变更，负例被拒 |
| CERT8883-CLI-02 App TLS / CONNACK | **PASS（证书/TLS 维度）** | 本轮窗口无 SAN/链/信任锚错误；TLS 完成并收到 CONNACK `rc=5 无权连接`（认证层缺口另记） |
| CERT8883-REG-03 不受影响路径回归 | **PASS（已探测项）** | 七端口、HTTP 语义、MySQL 握手、Dashboard 与基线一致；App 可启动并正常读取网关 |
| CERT8883-UPLOAD-04 安卓上传 + 持久化回读 | **上传受理 PASS（人工确认范围）；字段级回读 NOT EXECUTED（人工批准延期）** | 本轮真实 App 两次上传（10:43:07、10:51:15）均 `HTTP 200`、待发箱清零、本地新增对应读数；同键重放 `inserted=0`。字段级回读按用户“不用回读，传上去就行了”不执行 |
| E2E-JOINT-P0 受影响联合用例 P0 | **PASS（证书 + 上传维度）** | 平台段与手机段在当前版本复核通过；MQTT 会话未建立属既有身份缺口 |
| E2E-JOINT-P1 注入正事件四段 | **NOT EXECUTED** | 缺固件测试构建注入、已确认平台网关 MAC、平台回读授权；未获得明确“本阶段不纳入”批准 |
| CERT8883-FINAL-05 本轮交付整体 | **TEST_FAIL（EXTERNAL_CONDITION_UNEXECUTED）** | 证书/上传修复实测生效、无该路径缺陷；P1 未执行且未明确豁免，按设计不得判 PASS |

**一句话**：当前版本上，平台 8883 证书由原 App 信任锚正确签发（无需更换信任锚），真实 App 已两次把 BLE 读数上传到 `/upload_data` 并获得 `HTTP 200`、待发箱清零、同键重放不新增行；字段级回读按人工决定未执行。证书与上传修复本身未测出缺陷。唯一未闭环项是 E2E-JOINT-P1（固件注入四段），其外部条件未具备且未获明确范围豁免，因此交付整体为 `TEST_FAIL`（外部条件未执行），交回 Runtime 处理而非实现返工。

---

## 2. 人工范围确认与延期项（本报告显式披露）

上一轮人工问题同时询问了两项：①平台 `sensor_data` 字段级只读回读；②E2E-JOINT-P1 是否纳入本阶段。

人工回复：`无法准备，先跳过 [不用回读，传上去就行了]。`

| 项 | 稳定 ID | 人工结论 | 本报告处理 |
|---|---|---|---|
| 平台 `sensor_data` 字段级只读回读 | `CERT8883-UPLOAD-04` | “无法准备，先跳过”；“不用回读，传上去就行了” | **NOT EXECUTED（APPROVED_DEFERRED）**。不重测、不写成通过；上传受理/落库路径以真实 App 上传与同键重放证明 |
| E2E-JOINT-P1 是否纳入本阶段 | `E2E-JOINT-P1` | 回复未单独给出“纳入/本阶段不纳入” | **NOT EXECUTED**，未获明确豁免；按 FINAL-05，交付整体不得判 PASS |

说明：Runtime 记录的人工延期项仅 `CERT8883-UPLOAD-04`。本报告不把该延期外推到 P1，也不把 P1 未执行当作已批准跳过。

---

## 3. 本轮环境同步与测试环境修复（不改变产品）

1. **版本同步**：Repository 最新提交为 devops_engineer 的可靠性观测（`e4d1c57` 等），其观测窗口（05:22~05:33 CST）早于第 3 轮上传实测（05:42~05:52 CST）。本轮核对平台哈希/PID 与 App APK 摘要，均与第 3 轮相同，无版本漂移。
2. **ADB 连接修复**：开始时手机 `12f046690506` 显示 `offline`；执行 `adb reconnect offline` 后变为 `unauthorized`，再执行 `adb kill-server`/`start-server` 并重新确认 USB 调试授权后恢复 `device`。10:46 再次 `offline`，以同样方式恢复。属测试环境修复，未安装/卸载/清数据。
3. **手机屏幕/锁屏**：设备初始息屏且 `mDreamingLockscreen=true`；用 `KEYCODE_WAKEUP` + `wm dismiss-keyguard` 唤醒并关闭锁屏，随后启动 App。USB 调试授权弹窗按设备既有授权流程勾选“一律允许”并确定。
4. **App 进程缺失（跨组件观察，非本轮修复引入）**：旧进程 32061 于 **2026-11-26 05:54:10 CST** 被系统记录为 `Killing 32061 ... SwipeUpClean`（任务清理/上滑清除，不是崩溃）；此后至 10:36 未运行，故 05:54–10:36 无采集/上传请求。10:36:21 以正常用户路径重新启动 App（PID 12980），守护恢复（首页按钮为“停止”）。崩溃缓冲区仅保留 11-25 22:00:24 的既有导航崩溃，无新增崩溃/ANR。

---

## 4. CERT8883-SRV-01：平台证书独立验收（PASS，本轮复验）

命令：

```
bash scripts/emqx_8883_cert_verify.sh 8.140.23.253 8883 \
     dengbei_care/app/src/main/assets/jd-ca.crt --expect-new
```

证据：`evidence/software_verify_cert8883_r4_20261126/01_srv_cert_verify_current.txt`（2026-11-26T02:33:08Z）

| 判据 | 实测 | 结果 |
|---|---|---|
| `tls_handshake` | OK | PASS |
| subject / CN | `CN=8.140.23.253` | PASS |
| issuer | `CN=Tiktok Root CA, O=LinkValue` | PASS |
| serial | `77ADCFD924BA0F87F845C81934D4A3F346D4BB83`（≠ 缺陷基线 `…B82`） | `serial_changed=YES` |
| SAN | `IP:8.140.23.253, IP:117.72.84.210` | `san_contains_target=YES` |
| `chain_verify`（仅 jd-ca.crt） | OK | PASS |
| `ip_san_match`（`-verify_ip 8.140.23.253`） | OK | PASS |
| 负例 `-verify_ip 8.140.23.254` | 不通过 | `negative_control=OK` |
| 有效期 | 2026-11-25T02:55:16Z → 2028-11-24T02:55:16Z | PASS |
| 叶子 SHA-256 | `E7:AD:1E:46:FA:5B:5D:06:02:C5:AD:04:B7:8B:1C:AF:2E:78:EE:47:8A:9F:6A:79:EA:B6:3B:D5:54:BD:88:84` | 与部署记录一致 |
| 信任锚是否需更换 | 否：原 `jd-ca.crt` 可签发且链校验通过 | 未更换 |
| `overall` | `PASS` | PASS |

---

## 5. CERT8883-CLI-02：App 真实消费端 TLS / CONNACK（证书维度 PASS）

窗口：CST 10:36 ~ 10:54；证据 `19_logcat_latest.txt`、`24_logcat_app_window.txt`。

| 观测 | 实测 | 结果 |
|---|---|---|
| `No subjectAltNames` / `SSLHandshakeException` | 本轮窗口 0 次 | PASS |
| `Trust anchor…` / `PKIX path building failed` / `CertPathValidator` | 本轮窗口 0 次 | PASS |
| TLS 是否完成并进入 MQTT CONNECT | 是（Paho 收到 CONNACK 才返回码） | PASS |
| CONNACK 实际结果 | `无权连接 (5)`（`not_authorized`），最近 10:46:24 | 认证层缺口，非证书失败 |
| 网关 BLE（非 MQTT 路径） | 10:39/10:46 周期失败；10:43、10:51 `DATA_OK`，首页显示“网关已连接” | 未受 MQTT 认证缺口影响 |

**判读**：证书缺陷维度 PASS；`rc=5` 属既有身份/认证缺口，不得写成 TLS 失败，也不得据此判 MQTT/上传/P1 通过。

---

## 6. CERT8883-REG-03：不受影响路径回归（PASS，已探测项）

证据：`08_platform_window_now.txt`、`16_regression_probe_now.txt`（10:33~10:52 CST）。

| 检查 | 实测 | 与基线 |
|---|---|---|
| 8883 TLS（App 信任锚 + 主机名校验） | TLSv1.3 / `TLS_AES_256_GCM_SHA384`，校验 OK | 一致 |
| 1883 / 8083 / 8084 / 18083 / 5000 | 全部 OPEN | 一致 |
| 3306 | MySQL 9.0.1 握手（未认证、未执行 SQL） | 一致 |
| Dashboard `GET /status` | `200`，`emqx is running` | 一致 |
| Flask `GET /` | `404` | 一致 |
| Flask `GET /login` | `405`，`Allow: OPTIONS, POST` | 一致 |
| Flask `GET /upload_data` | `405`，`Allow: OPTIONS, POST` | 一致 |
| `user_script` | active/running，PID 312368，`NRestarts=0`，`-p err` 无条目 | 正常 |
| 新增免校验/诊断入口 | 未发现（仍观测到互联网扫描 404 噪声，均未触达写路径） | 一致 |
| App 启动/守护/网关读取 | 正常启动、守护运行、`DATA_OK` 两次；无新增崩溃/ANR | 正常 |

---

## 7. CERT8883-UPLOAD-04：安卓上传 + 持久化（第 4 轮实测）

### 7.1 触发前的实际条件

| 项 | 实测 | 证据 |
|---|---|---|
| 测试身份 | `registerphone=14000000001`（模拟测试账号，非真实用户） | `20_prefs_latest.xml` |
| 平台身份 | `mac_addr=0200000000A1`（合成测试 MAC，非真实网关） | 同上 |
| 网关绑定 | `gateway_ble_addr=84:C2:E4:03:02:02`；设备 `6A002C00`/`e2ep0` | 同上 |
| 默认网络 | `ZTE_607157`，`wlan0 192.168.0.103` | `02_phone_state_now.txt` |
| 上传状态 | `upload_last_status=OK`、`upload_blocked_reason` 空、`upload_pending_count=0`、`upload_blocked_at=0` | `13_prefs_after_relaunch.xml`、`20_prefs_latest.xml` |
| 守护状态 | `measure_state=stopping`（守护运行中，`finish_guard_time=2026-11-26 13:46:12`） | 同上 |

### 7.2 真实 App 上传结果（HTTP 200，两次）

| 时间（CST） | 动作/观测 | 实测 | 证据 |
|---|---|---|---|
| 10:43:07.244 | BLE 读取 | `id=6A002C00, H=36.3%, T=21.1°C` | `24_logcat_app_window.txt` |
| 10:43:07.832 | App 上传 | `CloudUpload: uploaded 1 reading(s)` | 同上 |
| 10:43:10 | 平台访问日志 | `27.128.89.184 - - "POST /upload_data HTTP/1.1" 200 -` | `11_platform_window_after_relaunch.txt` |
| 10:51:15.074 | BLE 读取 | `id=6A002C00, H=36.3%, T=21.1°C` | `24_logcat_app_window.txt` |
| 10:51:15.533 | App 上传 | `CloudUpload: uploaded 1 reading(s)` | 同上 |
| 10:51:17 | 平台访问日志 | `27.128.89.184 - - "POST /upload_data HTTP/1.1" 200 -` | `21_platform_window_latest.txt` |
| 10:54 | App prefs | `upload_last_status=OK`、`upload_pending_count=0`、`upload_blocked_reason` 空 | `20_prefs_latest.xml` |
| 10:54 | 本地库 | `temperature` 127 行；新增 `(6A002C00,1795660987,21.1,36.3)`、`(6A002C00,1795661475,21.1,36.3)`；`pending_uploads` 0 行 | `22_local_db_latest.db`、`23_local_db_latest_analysis.txt` |
| 10:53 | 首页 UI/截图 | “网关已连接（最近读取 10:51）”、`21.1°C`、`36%`、“最近读取 刚刚”、守护运行 | `17_ui_home_after_upload.txt`、`18_ui_home_after_upload.png` |

**判读**：真实 BLE 读数入队 → 真实 App `POST /upload_data` → 平台 `HTTP 200` → App 记为 `OK` 并清空待发箱 → 本地测量表新增对应行。当前版本的真实上传路径连续两次走通。

### 7.3 幂等（同 `(dev_id,time)` 重试不新增行）

证据：`15_idempotency_new_reading.txt`。使用 **10:43:07 真实上传的同一条读数**（`dev_id=6A002C00, time=1795660987, 21.1/36.3, phone=14000000001, mac=0200000000A1`）重放一次：

```
utc=2026-11-26T02:52:04Z status=200 elapsed_ms=143 body={"inserted":0,"message":"ok"}
```

**判读**：该键已存在，`inserted=0`。这证明写库/幂等路径实际执行且未新增行；**不是**字段级回读，不替代字段值核对。

### 7.4 未执行：`sensor_data` 字段级只读回读（人工批准延期）

- 人工结论：`无法准备，先跳过`，补充“不用回读，传上去就行了”。
- 本报告按 `test_item_id=CERT8883-UPLOAD-04` 记录该子项 **NOT EXECUTED（APPROVED_DEFERRED）**，不写成 PASS。
- 因此“平台存储的温湿度逐字段一致”未被核对；本轮的落库证据是“HTTP 200 + 同键重放 inserted=0 + App 清空待发箱”，属于受理与幂等维度，不等于字段值回读。

### 7.5 测试数据与残留（披露）

- App 当前登录 `14000000001`、`mac_addr=0200000000A1`、守护运行至 2026-11-26 13:46:12；待发箱 0 条。
- 平台保留该模拟账号与该测试设备的读数；本轮新增两条 `(6A002C00, 1795660987 / 1795661475)`。无删除/DB 写授权，未清理。
- 未发送短信、未使用真实用户凭据、未直连数据库、未登录 Dashboard、未修改服务端配置。

---

## 8. E2E-JOINT-P0 / P1

### E2E-JOINT-P0：PASS（证书 + 上传维度）

- 三端身份：传感器 `uid=6A002C00`（引用既有同版本证据）；网关 BLE `84:C2:E4:03:02:02` / `GwID=B1:17:9C:7A:4E:E0`（本轮 BLE 读取正常）；APK/设备见封面。
- 手机段：设备在线、APK 摘要一致、`ZTE_607157` 默认、守护运行、网关读取正常（`DATA_OK` 两次）。
- 平台段：SRV-01 / CLI-02 / REG-03 本轮实际结果；真实上传 `200` 与写库/幂等路径已实测。只读基线覆盖服务状态、七端口监听、近期日志与 `ss -tn` established 连接计数（8883/1883/8083/8084/3306 均为 0；5000 见 1 条外部已建立、尚未发出 HTTP 请求的连接），证据 `25_platform_conn_counts.txt`。
- CONNACK `rc=5` 按认证层分类记录，不计入证书失败；不得写成 MQTT 会话已建立。

### E2E-JOINT-P1：NOT EXECUTED

缺：固件测试构建（`measure_sample()` 后注入 `367`、默认关闭、正式包排除）、已确认平台网关 MAC（当前 `0200000000A1` 为合成测试值）、平台字段级回读授权。四段无法组成，故整例不判通过、不部分通过。人工回复未给出“P1 本阶段不纳入”的明确批准。

---

## 9. 边界与未执行

1. `sensor_data` 字段级只读回读：NOT EXECUTED（人工批准延期，见 §2/§7.4）。
2. E2E-JOINT-P1：NOT EXECUTED（缺固件测试构建、已确认平台网关 MAC、回读授权）。
3. 真实物理 >35℃ / 升温+无光 / 光强 / 功耗 / 433 电气：无条件，未执行。
4. release/生产签名、iOS、API 31+、9 台真实快照：未产出/无设备，不在本轮。
5. 既有跨组件观察（非本轮修复引入，交责任方）：MQTT 认证缺口（`CONNACK rc=5`）；App 22:00:24 导航崩溃；05:54:10 进程被系统 `SwipeUpClean` 清理后未自启；BLE 约 50% 周期失败。
6. 平台 `/upload_data` 当前无认证且公网可达（既有部署风险，非本轮测试引入）。

---

## 10. 证据索引

目录：`evidence/software_verify_cert8883_r4_20261126/`

| 文件 | 内容 |
|---|---|
| `01_srv_cert_verify_current.txt` | 本轮 8883 叶子握手、链、SAN、IP 校验、负例、`overall=PASS` |
| `02_phone_state_now.txt` | 本轮设备/包版本/进程/网络/prefs/崩溃缓冲/APK 摘要 |
| `08_platform_window_now.txt` | 上传前平台只读基线（服务 PID/哈希/日志/监听） |
| `11_platform_window_after_relaunch.txt` | 重启动后平台日志窗口（真实上传 200） |
| `12_local_db_after_relaunch.db`、`22_local_db_latest.db` | 本地库快照（重启动后 / 最新） |
| `13_prefs_after_relaunch.xml`、`20_prefs_latest.xml` | 上传前后 prefs（`upload_last_status=OK`、待发 0） |
| `14_local_db_analysis.txt`、`23_local_db_latest_analysis.txt` | 本地库解析（新增读数、待发 0） |
| `15_idempotency_new_reading.txt` | 10:43 读数同键重放（200，`inserted=0`） |
| `16_regression_probe_now.txt` | 七端口、HTTP 语义、MySQL 握手原始输出 |
| `17_ui_home_after_upload.txt`、`18_ui_home_after_upload.png` | 首页 UI 与截图（21.1℃/36%、最近读取 10:51、守护运行） |
| `19_logcat_latest.txt` | 本轮完整 logcat 缓冲（含 10:43/10:51 上传、CONNACK、BLE 周期） |
| `21_platform_window_latest.txt` | 10:43 后平台日志（10:43/10:51 真实上传、10:52 重放） |
| `25_platform_conn_counts.txt` | 平台只读 `ss -tn` established 连接计数基线（8883/1883/8083/8084/3306 为 0） |
| `24_logcat_app_window.txt` | App 相关日志窗口摘要（两次真实上传 + 历史上传） |
| `phone_state_now.py`、`analyze_local_db.py`、`idempotency_new_reading.py`、`regression_probe_now.py`、`platform_conn_counts.py`、`extract_app_log.py` | 可复跑的只读/幂等测试脚本 |

前轮证据：`artifacts/software_test_report_cert8883_r3.md`、`evidence/software_verify_cert8883_r3_20261126/`（05:42~05:52 CST，同为当前版本；本报告引用但不重复其原始输出）。

清单检查：仓库 `scripts/` 目录当前仍无 `check_file_manifest.py`，无法运行该机械检查；已用只读路径存在性自检确认本轮新增的清单项全部存在。

---

## 11. 结论

1. **证书修复维度：PASS（本轮复验）**。平台 8883 由 App 原信任锚签发、SAN/IP 正确、负例被拒；真实 App 无 SAN/链/信任锚错误，TLS 完成并收到 CONNACK。**不需要更换 App 信任锚**。
2. **上传受理修复：当前版本实测生效**。真实 App 两次（10:43:07、10:51:15）将 BLE 读数上传至 `/upload_data`，平台均 `HTTP 200`，App `uploaded` 并清空待发箱，本地测量表新增对应行；同键重放 `inserted=0`。
3. **字段级回读：NOT EXECUTED（人工批准延期）**；本报告不把上传 200 写成“字段值回读通过”。
4. **E2E-JOINT-P1：NOT EXECUTED**，缺固件测试构建注入、已确认平台网关 MAC 与回读授权；未获得明确“本阶段不纳入”批准。
5. **本轮交付结果：`TEST_FAIL`（`failure_cause=EXTERNAL_CONDITION_UNEXECUTED`）**。该结果仅因 P1 外部条件未具备，**不是**证书/上传修复路径上的实测缺陷；若用户明确本阶段不纳入 P1，则证书+上传范围已具备通过依据，需由 Runtime 按流程记录该范围决定。
6. 本报告只覆盖当前组合版本的已执行范围；阶段通过不代表完整产品验收通过。
