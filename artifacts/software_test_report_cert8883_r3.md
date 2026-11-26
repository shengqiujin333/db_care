# 8883 证书修复后软件独立验证报告（CERT8883 第 3 轮：上传受理修复复测）

- 负责角色：软件测试
- 能力：软件验证
- 执行时间：2026-11-25T21:42Z ~ 21:51Z（CST 2026-11-26 05:42 ~ 05:51）
- 报告状态：**上传受理修复已实测生效，未测出修复路径功能缺陷；字段级持久化回读与 E2E-JOINT-P1 因外部条件未执行，本轮不能判 PASS。** 若用户无法提供回读授权或明确排除 P1，按设计交付结果为 `TEST_FAIL`（`failure_cause=EXTERNAL_CONDITION_UNEXECUTED`）。
- 被测对象：
  - 平台：既有服务器 `8.140.23.253`（`iZ2zeih0gmrk47og5r4z7sZ`）
    - EMQX 5.8.0 `ssl:default` 0.0.0.0:8883
    - Flask `user_script.service` 0.0.0.0:5000，PID 312368，启动 2026-11-25 21:59:02 CST，`NRestarts=0`
    - 部署对象 `/root/abr.py`，8,688 B，sha256 `a062b05f524dc8f6bce596e4571d339f697ef909b318503ad418ee0add24017c`（= 已批准候选 `deploy/platform_upload_contract/container.yaml` v2）；上传门禁 `X-Upload-Token` 已移除
  - 客户端：Android App `com.jinyuni.dengbei_care` vC10 / vN1.9；设备侧 `base.apk` SHA-256 `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b`；安装/更新时间 2026-11-23 21:25:13
  - 手机：Redmi 8A `olivelite`，Android 10 / API 29，serial `12f046690506`，默认网络 `ZTE_607157`，wlan0 `192.168.0.103`
  - 信任锚：`dengbei_care/app/src/main/assets/jd-ca.crt`，SHA-256 `75:33:09:B3:9F:1B:8F:85:B4:D7:57:76:BF:D3:8A:42:52:0D:C1:A0:5E:A2:0B:A3:11:7E:B3:9C:BA:B2:15:2B`（未更换）
- 运行依据：`artifacts/software_test_design_cert8883.md`、`artifacts/cert8883_cases.yaml`、`artifacts/cert8883_e2e_plan.md`、`evidence/e2e_p0/devops_platform_upload_contract_20261125/DEPLOYMENT_EVIDENCE.md`
- 本轮性质：独立验证；未修改产品源码/APK/平台配置；未重部署、未重启服务；未登录 Dashboard/DB、未直连数据库、未发送短信；平台侧仅只读 SSH 与上传接口功能请求

---

## 1. 结果摘要

| 用例 | 结果 | 说明 |
|---|---|---|
| CERT8883-SRV-01 平台证书独立验收 | **PASS** | 叶子证书由 App 信任锚签发，SAN 命中 `8.140.23.253`，serial 已变更，负例被拒 |
| CERT8883-CLI-02 App TLS / CONNACK | **PASS（证书/TLS 维度）** | 无 SAN/链/信任锚错误；TLS 完成并收到 CONNACK `rc=5 无权连接`（认证层缺口另记） |
| CERT8883-REG-03 不受影响路径回归 | **PASS（已探测项）** | listener/HTTP/MySQL/路由语义与基线一致；App 无新增崩溃/ANR |
| CERT8883-UPLOAD-04 安卓上传 + 持久化回读 | **上传受理与落库路径通过；字段级回读 NOT EXECUTED** | 真实 App 上传 **HTTP 200**、服务端进入 `mysql.connect()`、App `uploaded 9 reading(s)`、待发箱清零；同键重放两次均 `inserted=0`；但无 `sensor_data` 字段级只读回读授权 |
| E2E-JOINT-P0 受影响联合用例 P0 | **PASS（证书 + 上传维度）** | 平台段 + 手机段当前版本复核通过；MQTT 会话未建立属既有身份缺口 |
| E2E-JOINT-P1 注入正事件四段 | **NOT EXECUTED** | 缺固件测试构建注入、已确认平台网关 MAC、平台回读授权 |
| CERT8883-FINAL-05 本轮交付整体 | **不能判 PASS（外部条件待定）** | 证书/上传修复实测生效、无该路径功能缺陷；C4 字段级回读与 P1 未执行；若条件无法提供 → TEST_FAIL（EXTERNAL_CONDITION_UNEXECUTED） |

**一句话**：devops 移除 `X-Upload-Token` 门禁后，真实 App 的上传链路已实测恢复——9 条保留读数与 1 条新读数均 `HTTP 200` 进入数据库写入路径，App 待发箱清零；同 `(dev_id,time)` 重放 `inserted=0`，幂等成立。证书修复维度继续 PASS。剩余缺口是**字段级落库回读**（当前无数据库只读授权）与 **E2E-JOINT-P1**（需固件测试构建注入与已确认平台网关 MAC）。

---

## 2. CERT8883-SRV-01：平台证书独立验收（PASS）

命令（以 App 当前信任锚为唯一 CA，`--expect-new`）：

```
bash scripts/emqx_8883_cert_verify.sh 8.140.23.253 8883 \
     dengbei_care/app/src/main/assets/jd-ca.crt --expect-new
```

证据：`evidence/software_verify_cert8883_r3_20261126/01_srv_cert_verify.txt`

| 判据 | 实测 | 结果 |
|---|---|---|
| `tls_handshake` | OK（2026-11-25T21:42:31Z） | PASS |
| subject / CN | `CN=8.140.23.253` | PASS |
| issuer | `CN=Tiktok Root CA, O=LinkValue` | PASS |
| serial | `77ADCFD924BA0F87F845C81934D4A3F346D4BB83`（≠ 缺陷基线 `…B82`） | `serial_changed=YES` |
| SAN | `IP:8.140.23.253, IP:117.72.84.210` | `san_contains_target=YES` |
| `chain_verify`（仅 jd-ca.crt） | OK | PASS |
| `ip_san_match`（`-verify_ip 8.140.23.253`） | OK | PASS |
| 负例 `-verify_ip 8.140.23.254` | 不通过 | `negative_control=OK` |
| 有效期 | 2026-11-25T02:55:16Z → 2028-11-24T02:55:16Z | PASS |
| 叶子 SHA-256 | `E7:AD:1E:46:FA:5B:5D:06:02:C5:AD:04:B7:8B:1C:AF:2E:78:EE:47:8A:9F:6A:79:EA:B6:3B:D5:54:BD:88:84` | 与部署记录一致 |
| `overall` | `PASS` | PASS |

---

## 3. CERT8883-CLI-02：App 真实消费端 TLS / CONNACK（证书维度 PASS）

窗口：CST 05:44 ~ 05:51；证据 `19_logcat_full_window.txt`、`15_logcat_after_retry2.txt`。

| 观测 | 实测 | 结果 |
|---|---|---|
| `No subjectAltNames` / `SSLHandshakeException` | 0 次 | PASS |
| `Trust anchor…` / `PKIX path building failed` | 0 次 | PASS |
| TLS 是否完成并进入 MQTT CONNECT | 是（Paho 收到 CONNACK 才返回码） | PASS |
| CONNACK 实际结果 | `无权连接 (5)`（`not_authorized`），最近 05:45:31 | 认证层缺口，非证书失败 |
| 网关 BLE（非 MQTT 路径） | 正常读取，首页显示 `网关已连接` | 未受 MQTT 认证缺口影响 |

**判读**：证书缺陷维度 PASS；`rc=5` 属既有身份/认证缺口，不得写成 TLS 失败，也不得据此判上传或 P1 通过。

---

## 4. CERT8883-REG-03：不受影响路径回归（PASS，已探测项）

证据：`04_regression_probe.txt`（2026-11-25T21:42:36Z）、`20_platform_final_window.txt`。

| 检查 | 实测 | 与基线 |
|---|---|---|
| 8883 TLS（App 信任锚） | TLSv1.3，`peer_cn=8.140.23.253`，链校验 OK | 一致 |
| 1883 / 8083 / 8084 / 18083 / 5000 | 全部 OPEN | 一致 |
| 3306 | MySQL 9.0.1 握手（未认证、未执行 SQL） | 一致 |
| Dashboard `GET /status` | `200`，`emqx is running` | 一致 |
| Flask `GET /` / `/login` / `/upload_data` | `404` / `405`（`Allow: OPTIONS, POST`）/ `405` | 一致 |
| `user_script` | active/running，PID 312368，`NRestarts=0`，变更后无错误日志 | 正常 |
| 新增免校验/诊断入口 | 未发现 | 一致 |
| App 启动/守护/网关读取 | 正常；无 `FATAL EXCEPTION` / `ANR in com.jinyuni`（本窗口） | 正常 |

注：`/upload_data` 现在是无认证写入（部署修复的直接结果，已由部署证据披露为安全风险）；本轮只做了同键幂等重放，未新增业务行。

---

## 5. CERT8883-UPLOAD-04：安卓上传 + 持久化（第 3 轮实测）

### 5.1 触发前的实际条件

| 项 | 实测 | 证据 |
|---|---|---|
| 测试身份 | `registerphone=14000000001`（模拟测试账号，非真实用户） | `02_phone_identity_state.txt`、`13_prefs_after_start.xml` |
| 平台身份 | `mac_addr=0200000000A1`（合成测试 MAC，非真实网关） | 同上 |
| 网关绑定 | `gateway_ble_addr=84:C2:E4:03:02:02`；设备 `6A002C00`/`e2ep0` | 同上 |
| 默认网络 | `ZTE_607157`，`wlan0 192.168.0.103` | `03_wifi_dump.txt`、`04_phone_state_parsed.txt` |
| 待发箱 | 9 行（8×18.0℃/35.8%，1×21.7℃/37.1%），`attempts` 合计 100 | `05_local_db_before.db`、`04_phone_state_parsed.txt` |
| 上传阻塞状态 | `upload_blocked_reason=AUTH_REJECTED`、`upload_last_status=401`、`upload_blocked_at=1795595909`（CST 16:38:29） | `02_phone_identity_state.txt` |
| 守护状态 | **`measure_state=starting`**（App 22:00:24 崩溃重启后未恢复守护运行） | 同上 |

**关键前置**：首次点击“重试上传”时门控仍关闭（`guardActive=false`），未产生任何上传请求；通过 App 首页实际“启动”按钮恢复守护运行（`measure_state=stopping`，`finish_guard_time=2026-11-26 13:46:12`）后，门控打开，再次点击“重试上传”才真正发起上传。该操作是产品正常用户路径，未直接编辑 prefs。

### 5.2 真实 App 上传结果（HTTP 200）

| 时间（CST） | 动作/观测 | 实测 | 证据 |
|---|---|---|---|
| 05:46:51 | App 内点击“重试上传” | 触发 `RETRY_UPLOAD` | `18_ui_after_upload.xml` |
| 05:46:52.632 | App 日志 | `CloudUpload: uploaded 9 reading(s)` | `15_logcat_after_retry2.txt` 第 407 行 |
| 05:46:54 | 平台访问日志 | `27.128.89.184 - - "POST /upload_data HTTP/1.1" 200 -`；前一行 `conn = mysql.connect()` | `14_platform_after_retry2.txt` |
| 05:46:54 后 | App prefs | `upload_last_status=OK`、`upload_blocked_reason` 空、`upload_pending_count=0` | `24_prefs_after_upload.xml` |
| 05:46 后 | 本地待发箱 | `pending_uploads=0` | `16_local_db_after.db` |
| 05:49:06.243 | 新读数自动上传 | `CloudUpload: uploaded 1 reading(s)`；平台 05:49:08 `POST /upload_data` 200 | `19_logcat_full_window.txt`、`20_platform_final_window.txt` |
| 05:49 后 | 本地温度表新增 | `(6A002C00, 1795643345, 21.2, 35.5)`，待发箱仍为 0 | `21_local_db_final.db` |

**判读**：真实 App 上传路径已恢复：真实 BLE 读数入队 → `POST /upload_data` → 平台 200 并进入 `mysql.connect()` 写库分支 → App 收到成功并 ack 清空待发箱。新增读数也能持续走通同一路径。

### 5.3 幂等（同 `(dev_id,time)` 重试不增行）

证据：`17_idempotency_replay.txt`、`22_idempotency_new_reading.txt`。使用**与 App 完全相同的 9 条读数**（`phone=14000000001`、`mac=0200000000A1`）重放：

| 时间（UTC） | 请求 | 状态 | 响应体 |
|---|---|---|---|
| 21:48:06Z | 重放第 1 次（9 条） | 200 | `{"inserted":0,"message":"ok"}` |
| 21:48:08Z | 重放第 2 次（9 条） | 200 | `{"inserted":0,"message":"ok"}` |
| 21:49:40Z | 重放 05:49 新读数（1 条） | 200 | `{"inserted":0,"message":"ok"}` |

**判读**：`INSERT IGNORE` + 唯一 `(dev_id,time)` 语义成立；真实上传的 9 条与新 1 条键均已存在，同键重试不新增行。该证据可证明“已落库且幂等”，但**不能替代字段级回读**（未观察 `temperature`/`humidity` 存储值原文）。

### 5.4 未执行：`sensor_data` 字段级只读回读

- 现状：平台无对外回读接口；`existing_service_ssh_inspection` 只允许服务状态/端口/近期日志，明确**不直连数据库**；数据库直连不在本能力授权内。
- 因此“按 `(phone, dev_id, time)` 回读 `temperature`/`humidity` 逐字段一致”**NOT EXECUTED**。
- 需要：用户授予只读数据库查询入口，或提供等效的已授权回读方式；也可明确本阶段不要求字段级回读（则按 NOT EXECUTED 记录，不得写成 PASS）。

### 5.5 测试数据与残留（披露）

- App 当前登录 `14000000001`、`mac_addr=0200000000A1`、守护运行至 2026-11-26 13:46:12；待发箱 0 条。
- 平台 `user_table` 保留该模拟账号；`sensor_data` 保留本轮真实上传的 9 条 + 1 条测试读数（`dev_id=6A002C00`）。无删除/DB 写授权，未清理。
- 未发送短信、未使用真实用户凭据、未直连数据库、未登录 Dashboard、未修改服务端配置。
- 测试 MAC `0200000000A1` 与手机号 `14000000001` 均为测试专用，不是真实网关/用户。

---

## 6. E2E-JOINT-P0 / P1

### E2E-JOINT-P0：PASS（证书 + 上传维度）

- 三端身份：传感器 `uid=6A002C00`（引用既有同版本证据）；网关 BLE `84:C2:E4:03:02:02` / `GwID=B1:17:9C:7A:4E:E0`（本轮 BLE 读取正常）；APK/设备见封面。
- 手机段：设备在线、APK 摘要一致、`ZTE_607157` 默认、守护运行、网关读取正常。
- 平台段：SRV-01 / CLI-02 / REG-03 实际结果；平台上传 200 与写库路径已实测。
- CONNACK `rc=5` 按认证层分类记录，不计入证书失败；不得写成 MQTT 会话已建立。

### E2E-JOINT-P1：NOT EXECUTED

缺：固件测试构建（`measure_sample()` 后注入 `367`、默认关闭、正式包排除）、已确认平台网关 MAC（当前 `0200000000A1` 为合成测试值）、平台字段级回读授权。故整例不判通过、不部分通过。

---

## 7. 边界与未执行

1. `sensor_data` 字段级只读回读：NOT EXECUTED（无数据库授权/无回读接口）。
2. E2E-JOINT-P1：NOT EXECUTED（缺固件测试构建、已确认平台网关 MAC、回读授权）。
3. 真实物理 >35℃ / 升温+无光 / 光强 / 功耗 / 433 电气：无条件，未执行。
4. release/生产签名、iOS、API 31+、9 台真实快照：未产出/无设备，不在本轮。
5. MQTT 认证缺口（`CONNACK rc=5`）、App 22:00:24 导航崩溃、崩溃后守护未自动恢复、BLE 间歇失败：属既有跨组件观察，非本轮证书/上传修复引入，另行交责任方。

---

## 8. 证据索引

目录：`evidence/software_verify_cert8883_r3_20261126/`

| 文件 | 内容 |
|---|---|
| `01_srv_cert_verify.txt` | 8883 叶子握手、链、SAN、IP 校验、负例、`overall=PASS` |
| `02_phone_identity_state.txt` | 设备/包版本/prefs/网络/崩溃缓冲/APK SHA-256 |
| `03_wifi_dump.txt`、`04_phone_state_parsed.txt` | 默认网络 `ZTE_607157`、本地库基线与待发行 |
| `05_local_db_before.db`、`16_local_db_after.db`、`21_local_db_final.db` | 上传前/后/最终本地库快照 |
| `06_ui_mine_before.xml`、`07_ui_upload_settings_before.xml` | “我的”与上传设置对话框（401 状态、重试上传） |
| `08_platform_before.txt` | 上传前平台只读基线（PID/哈希/日志/监听） |
| `09_ui_after_retry.xml` | 门控关闭时的重试结果（未产生请求） |
| `10_platform_after_retry.txt`、`14_platform_after_retry2.txt`、`20_platform_final_window.txt` | 各阶段平台只读日志（200/写库/无错误） |
| `11_logcat_after_retry.txt`、`15_logcat_after_retry2.txt`、`19_logcat_full_window.txt` | App 日志窗口（上传成功、TLS 无错误、无新崩溃） |
| `12_ui_home_before_start.xml`、`13_prefs_after_start.xml` | 守护启动前后状态（`starting` → `stopping`） |
| `17_idempotency_replay.txt`、`22_idempotency_new_reading.txt` | 同键重放 `inserted=0` |
| `18_ui_after_upload.xml`、`23_ui_upload_success.png` | 上传成功界面（“云端上传正常（待发 0 条）”） |
| `24_prefs_after_upload.xml` | 上传成功后 prefs（`upload_last_status=OK`、待发 0、守护运行） |
| `parse_phone_state.py`、`extract_ui.py`、`platform_window_ssh.py`、`idempotency_replay.py`、`idempotency_new_reading.py` | 可复跑测试脚本（只读/幂等重放） |

清单检查：仓库 `scripts/` 目录当前只有证书替换/校验脚本，没有 `check_file_manifest.py`，无法运行该机械检查；已用只读路径存在性自检确认本轮新增的清单项全部存在。

---

## 9. 结论与待决条件

**本轮交付结果：当前不能判 PASS；未测出证书修复或上传受理修复路径上的功能缺陷。** 若用户无法提供字段级回读授权、且未明确批准本阶段不纳入 P1，按设计正式结果为 `TEST_FAIL`（`failure_cause=EXTERNAL_CONDITION_UNEXECUTED`）。

1. **证书修复维度：PASS**。平台 8883 由 App 信任锚签发、SAN/IP 正确、负例被拒；真实 App 无 SAN/链错误，TLS 完成并收到 CONNACK。
2. **上传受理修复：实测生效**。真实 App 9 条保留读数与 1 条新读数均 `HTTP 200`、进入 `mysql.connect()` 写库路径、App `uploaded` 并清空待发箱；同键重放 `inserted=0` 两次。
3. **不能据此判完整持久化通过**：字段级 `sensor_data` 回读未执行；E2E-JOINT-P1 未执行。
4. **待用户决定的外部条件**：
   - 平台 `sensor_data` 字段级只读回读授权（或明确本阶段跳过字段级回读）；
   - E2E-JOINT-P1 是否纳入本阶段；如纳入，需固件测试构建注入 `367` 与已确认平台网关 MAC。
5. 本报告只覆盖当前组合版本的已执行范围；阶段通过不代表完整产品验收通过。
