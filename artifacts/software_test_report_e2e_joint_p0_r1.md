# E2E-JOINT-P0 受影响联合用例平台段复验报告（证书修复后，第 1 轮）

- 负责角色：软件测试
- 能力：软件验证
- 用例：`E2E-JOINT-P0`（TD-E2E-SW-006 §4.1；`stage_order`: embedded_reference → phone → platform）
- 本调用范围：证书修复后的**平台段**（App→EMQX TLS/CONNACK、平台只读基线），并以同版本身份复核嵌入式/手机段
- 设计依据：`artifacts/software_test_design_cert8883.md` §3.5；`artifacts/e2e_joint_test_design.md` §0.1/§4.1/§8；`artifacts/e2e_cases.yaml`（E2E-JOINT-P0）
- 执行时间窗（UTC）：**2026-11-26T04:29:43Z ~ 2026-11-26T04:40:11Z**（CST 12:29:43 ~ 12:40:11）
- 被测对象：Android App（AB-003）→ 平台 `8.140.23.253` 的 EMQX 8883 TLS/MQTT；既有传感器/网关（同版本引用）；平台只读服务状态
- **本轮结果**：**TEST_PASS（仅限 `E2E-JOINT-P0` 本调用声明的必需项）**
  - 平台段：无 SAN/链错误，TLS 完成并进入 MQTT CONNECT；CONNACK 实际结果 `rc=5 not_authorized` 分类记录（认证层，不是证书缺陷）；
  - 平台只读基线：服务/端口/日志/连接数经获准只读 SSH 记录；
  - 手机段：同 APK/设备/网关，手机段 PASS 引用 + 本轮逐项复核；
  - 嵌入式段：同版本身份引用（本能力不承担 MCU 板上验证，本轮无 serial/mdk 入口，未重新烧录/串口采集）；
  - **未执行项**（不属本阶段必需项）见 §7，未写成 PASS。

> 与旧平台段的关系：2026-11-23 `P0_platform_stage_report.md` 的 TEST_FAIL（证书 SAN 仅 `117.72.84.210`）属于修复前版本，**未沿用**其失败结论，也未沿用旧证书结论。本轮以修复后新证书、新握手、新 App 日志重新判定。

---

## 1. 三端身份（acceptance 第 1 条）

| 端 | 本轮身份 | 与 §0.1 对比 | 证据 |
|---|---|---|---|
| 传感器 | 发布包 `fw_sensor_FD-002r8.zip`，134,791 B，md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf`，sha256 `16ad3f69…f4d0`；源提交 `82a7e74e…`；axf md5 `b6a15fed2ed5e3d0c4adc51ac87a4830`、hex md5 `4d728dec9b0d28a352c46436b7586c72`；`BOOT uid=6A002C00` | **一致**（发布包字节、uid、构建身份均同） | 本轮 `44_fw_release_identity.txt`（文件级复核）；EV-041 `evidence/test.md`（构建/下载/BOOT，同版本引用） |
| 传感器本轮运行态 | 网关当前回读 `id=6A002C00`，与本板 `uid4` 一致；本轮未烧录/复位 | 一致（uid4） | `52_scan_log_1236.txt` 12:38:17；本轮未执行板上串口/烧录（见 §7） |
| 网关 | BLE 地址 `84:C2:E4:03:02:02`；`GwID=B1:17:9C:7A:4E:E0`；`dev=1`、`valueSize=160`、`mtu=240` | **一致** | `17_prefs.xml`；`22_logcat_window.txt` 12:27:04；`52_scan_log_1236.txt` 12:38:17 |
| 安卓 | APK SHA-256 `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b`（主机=设备），10,370,964 B，versionCode 10 / versionName 1.9；`com.jinyuni.dengbei_care` | **一致**（AB-003 未重建） | `13_device_apk_sha256.txt`、`15_host_apk_sha256.txt`、`07_package_dump.txt`、`14_device_apk_size.txt`、`16_host_apk_size.txt` |
| 手机 | serial `12f046690506`，Redmi 8A（`olivelite`），Android 10 / API 29 | **一致** | `01_devices.txt`、`02_model.txt`、`03_android_release.txt`、`04_sdk.txt`、`05_serial.txt` |
| 平台 | `8.140.23.253`；EMQX 5.8.0（`Node 'emqx@127.0.0.1' 5.8.0 is started`），PID 299216，启动 2026-11-25 10:58:15 CST，`NRestarts=0`；Flask `Werkzeug/3.1.8 Python/3.12.3`，PID 312368，`NRestarts=0`；MySQL `9.0.1`（`mysql:latest` 容器，Up 4 weeks，仅握手） | **一致** | `30d_platform_readonly_version.txt`、`30_platform_readonly.txt`、`46_mysql_greeting.txt`、`35_http_18083_status.txt` |
| 时间窗 | UTC `2026-11-26T04:29:43Z~04:40:11Z`（CST 12:29:43~12:40:11） | 本轮记录 | `00_utc_start.txt`、`53_final_state.txt` |

§0.1 差异说明：P0 手机段当时 `registerphone`/`mac_addr` 为空；本轮运行态已有合成测试身份 `registerphone=14000000001`、`mac_addr=0200000000A1`（经 App 既有登录/保存路径设置）。这是**运行态测试身份变化**，不是 APK/设备/网关版本变化；P0 必需项不受影响，`mac_addr` 来源未确认的缺口保留披露（§7）。

---

## 2. 嵌入式段（同版本身份复核，嵌入式引用）

P0 设计为“上电五行/Boot/首条 S”，属 MCU 板上验证；本能力固定任务**不承担 MCU/固件板上验证**，且当前调用无 `serial_capture`/`mdk_flash` 入口。本轮处理：

1. **版本一致性**：发布包 `fw_sensor_FD-002r8.zip` 本轮独立复核 md5 `5df0c6d4…`、大小 134,791 B、成员 axf `b6a15fed…`/hex `4d728dec…`，与 §0.1/EV-041 逐项一致（`44_fw_release_identity.txt`）。
2. **uid4 运行态**：App 本轮从网关读到 `id=6A002C00`，与 EV-041 `BOOT uid=6A002C00` 一致（`52_scan_log_1236.txt`）。
3. **未重新执行**：本轮未烧录、未复位、未采集 COM43；因此不声称“本轮重新验证了嵌入式板上功能”。该段以 EV-041（TEST_PASS）同版本身份引用，结论不外推到新固件/新板。

> 这不是用模拟代替实测：嵌入式板上项在本调用范围外，按设计“嵌入式引用”处理；引用对象与当次版本一致，并已用运行态 `uid4` 交叉确认。

---

## 3. 手机段本轮复核（acceptance 第 2 条）

**同版本判定**：APK SHA-256、versionCode、设备 serial/型号/API、网关 BLE 地址与 P0 手机段完全一致（§1），因此引用 `evidence/e2e_p0/P0_phone_stage_report.md` 的手机段 PASS，并执行本轮复核：

| 复核项 | 本轮实测 | 结果 | 证据 |
|---|---|---|---|
| 设备在线 | `12f046690506 device product:olivelite model:Redmi_8A` | PASS | `01_devices.txt`、`53_final_state.txt` |
| APK 摘要/版本 | 主机与设备侧 SHA-256 均 `80232c50…de4b`；vC10/vN1.9 | PASS | `07_package_dump.txt`、`13_device_apk_sha256.txt`、`15_host_apk_sha256.txt` |
| `ZTE_607157` 默认网络 | `Active default network: 101`（WIFI），`SSID="ZTE_607157"`、`VALIDATED`、BSSID `7c:21:d8:60:71:57`；wlan0 `192.168.0.103/24`；默认路由 `0.0.0.0/0 -> 192.168.0.1 wlan0` | PASS | `11_connectivity.txt:3,6`、`12_wifi_dump.txt:1061`、`09_ip_route.txt` |
| 守护运行 | `measure_state=stopping`、`finish_guard_time=2026-11-26 13:46:12`（仍在运行窗内）；PID `14224`；`MqtttService isForeground=true` | PASS | `17_prefs.xml`、`18_services.txt`、`19_ps.txt`、`53_final_state.txt` |
| 网关绑定 | `macid_name_map={"6A002C00":"e2ep0"}`；本轮 BLE target `84:C2:E4:03:02:02` | PASS | `17_prefs.xml`、`22_logcat_window.txt` |
| 0xA1 写入 | 12:27:04、12:38:17 均 `writeCharacteristic(legacy) ok=true, bytes=6`（N=1，`0xA1 + count(1) + uid4(4)`=6 B，as-built 语义同 P0 手机段） | PASS | `39_scan_log.txt`、`52_scan_log_1236.txt` |
| 读取就绪 | 12:27:04、12:38:17 `GwID=B1:17:9C:7A:4E:E0, dev=1, valueSize=160, mtu=240`，`link_state=DATA_OK`，`id=6A002C00`；另有 `CONNECT_FAILED` 明确终态（网关链路间歇） | PASS（读取可判定，取得 DATA_OK） | `22_logcat_window.txt`、`51_logcat_1236_onward.txt`、`39_scan_log.txt`、`52_scan_log_1236.txt` |
| UI 与日志一致 | 12:30 UI 显示 `21.2°C`/`38%`，与 12:27 日志 `T=21.2°C`/`H=37.5%` 数值一致；状态文案为“温湿度超限报警”（阈值联动，非缺陷） | PASS（数值一致） | `23_ui_dump.xml`、`43_ui_1232.png` |

**手机段补充（非 P0 必需）**：12:27:05 与 12:38:17 两次 `CloudUpload: uploaded 1 reading(s)`，平台访问日志 12:27:07/12:38:20 `POST /upload_data 200`，门控打开、无 `upload gated off`。该结果只证明 HTTP 受理，不证明 DB 字段级持久化（见 §7）。

---

## 4. 平台段（acceptance 第 3 条）

### 4.1 证书/链/SAN（本轮重新握手）

| 项 | 本轮实测 | 结果 |
|---|---|---|
| 信任锚 `jd-ca.crt` | X.509 SHA-256 `75:33:09:B3:9F:1B:8F:85:B4:D7:57:76:BF:D3:8A:42:52:0D:C1:A0:5E:A2:0B:A3:11:7E:B3:9C:BA:B2:15:2B`；文件 SHA-256 `5c17cf74…e1e4`；与 APK 内锚一致 | 未变更 |
| 线上叶子证书 | serial `77ADCFD924BA0F87F845C81934D4A3F346D4BB83`；SHA-256 `E7:AD:1E:46:FA:5B:5D:06:02:C5:AD:04:B7:8B:1C:AF:2E:78:EE:47:8A:9F:6A:79:EA:B6:3B:D5:54:BD:88:84`；issuer `CN=Tiktok Root CA`；notAfter `2028-11-24T02:55:16Z`；SAN `IP:8.140.23.253, IP:117.72.84.210` | 与 SRV-01 第 5 轮一致 |
| `openssl verify -verify_ip 8.140.23.253` | `Verification: OK`，`Verify return code: 0 (ok)`，脚本退出码 0；`TLSv1.3 TLS_AES_256_GCM_SHA384` | **PASS** |
| 负例对照 | 本轮未重跑 `-verify_ip 8.140.23.254`；引用同轮 SRV-01 负例 `error 64 IP address mismatch`（2026-11-26T03:10Z） | 判据未放宽 |

证据：`31_host_tls_verify_ip.txt`、`31b_host_tls_showcerts.txt`、`32_host_leaf_fields.txt`、`33_jdca_fields.txt`、`34_jdca_filesha.txt`；同轮 `CERT8883-SRV-01` 第 5 轮 PASS（`artifacts/software_test_report_cert8883_srv01_r5.md`）。

### 4.2 App 真实消费端 TLS/CONNECT/CONNACK（acceptance 第 3 条）

| 观察 | 本轮实测 | 结果 |
|---|---|---|
| SAN/链/信任锚错误 | 12:20–12:40 窗口 `SSLHandshakeException`/`No subjectAltNames`/`Trust anchor`/`PKIX`/`CertPathValidatorException`/`SSLPeerUnverifiedException` **全部 0 次** | PASS（无 SAN/链错误） |
| TLS 完成并进入 MQTT CONNECT | 12:23:56、12:28:55、12:33:55、12:38:55 四次 Paho 重连均收到服务端 CONNACK 后才抛 `MqttException`，栈在 `ClientState.notifyReceivedAck → CommsReceiver.run`（不在 TLS 校验） | PASS |
| 服务端同事件证据 | 12:23:57.898 / 12:28:57.794 / 12:33:57.809 / 12:38:57.816 EMQX `authentication_failure, clientid=android_client, username=0200000000A1, reason=not_authorized`（peername 为手机公网出口 `27.128.89.184`） | 服务端确认连接已到达并处理 MQTT CONNECT |
| CONNACK 实际结果 | **`rc=5 无权连接`（not authorized）**，username=`0200000000A1` | **分类记录：认证层缺口，不是 TLS/证书失败** |
| App 连接地址/端口/信任锚行为 | `ssl://8.140.23.253:8883`；未改校验、未用 `--insecure`、未更换信任锚 | 未变化 |

证据：`22_logcat_window.txt`、`47_logcat_1233_onward.txt`、`49_logcat_1238_onward.txt`、`39_scan_log.txt`、`48_scan_log_1233.txt`、`50_scan_log_1238.txt`、`30c/30d/30e_platform_readonly*.txt`；同轮 `CERT8883-CLI-02` 第 6 轮 PASS（证书维度）。

**认证层缺口**：测试身份 `mac_addr=0200000000A1` 为合成值，EMQX 认证库无此用户，故 `not_authorized`。该结果不影响“证书主机名/链已修复”的判定，也不证明 MQTT 业务授权或 P1 发布；真实用户/已确认网关 MAC 的认证不在本轮证据内。

---

## 5. 平台只读基线（acceptance 第 4 条）

方式：既有获准入口 `existing_service_ssh_inspection`（只读 SSH）；仅 `systemctl show/is-active`、`ss -ltnp`/`ss -tn state established`、`journalctl`/`tail`、`docker ps`、`curl` 本机 GET、`emqx version/ctl status`；**未 sudo、未写、未重启、未部署、未登录 Dashboard、未连数据库、未调用写接口**。

| 基线项 | 本轮实测（2026-11-26T04:31–04:39Z） | 结果 |
|---|---|---|
| 服务状态 | `emqx` active/enabled，PID 299216，启动 2026-11-25 10:58:15 CST，`NRestarts=0`；`user_script` active/running，PID 312368，启动 2026-11-25 21:59:02，`NRestarts=0`；`emqx ctl status` → `Node 'emqx@127.0.0.1' 5.8.0 is started` | PASS |
| 监听端口 | `8883`（beam.smp）、`1883`、`8083`、`8084`、`18083`、`5000`（python）、`3306`（docker-proxy）均在 LISTEN；另有既有主机级监听（22、80/443 Caddy、3000 next-server、21115-21119 hbbs/hbbr、32969 hbrclient、44865 containerd、127.0.1.1:5432 postgres 等），均在 2026-11-25 前启动，非本项目 MQTT/上传入口，未见由本轮测试新增 | PASS（项目端口与既有部署一致） |
| HTTP 服务身份 | 本机 `GET /status` → 200 `emqx is running`；`GET /` → 404；`GET /login`、`/upload_data` → 405 `Allow: OPTIONS, POST`（服务端本机与主机路径一致） | PASS |
| 近期服务日志 | `journalctl -u user_script --since 12:20`：12:27:07、12:38:20 `POST /upload_data HTTP/1.1 200`；`journalctl -u emqx -p err --since 12:20` → 无条目；`emqx.log.1` 仅见上述 `AUTHN not_authorized` warning，无 TLS/SSL 错误 | PASS |
| 连接数基线 | `ss -tn state established` 采样：`:8883` 0、`:1883` 0（04:31:05、04:32:04、04:33:15、04:39:32）。App 每 5 min 建立约 1 s 的短连接后因认证被拒，采样间隔可能落在连接之间；服务端 `emqx.log.1` 的同一 username 事件证明连接确实到达 | PASS（记录采样值 0，并说明短连接特性） |
| 未修改/未重启 | EMQX/Flask 启动时间均早于本轮，`NRestarts=0`；本轮无 sudo/写/重启/部署 | PASS |
| DB 字段级回读 | 无授权（只读 SSH 不含 DB 查询；平台无回读接口） | **NOT EXECUTED**（见 §7） |

证据：`30_platform_readonly.txt`、`30b_platform_readonly_routes.txt`、`30c_platform_readonly_emqxlog.txt`、`30d_platform_readonly_version.txt`、`30e_platform_readonly_1238.txt`、`35_http_18083_status.txt`、`36/37/38_http_5000_*.txt`、`46_mysql_greeting.txt`、`platform_p0_ssh.py`（可复跑）。

---

## 6. 关联键与同一事件（acceptance 第 5 条）

见 `evidence/e2e_joint_p0_platform_20261126/10_correlation_index.md`。要点：

- `uid4=6A002C00`、APK SHA-256 `80232c50…de4b`、网关 `84:C2:E4:03:02:02`/`GwID=B1:17:9C:7A:4E:E0`、UTC 时间窗 `04:29:43Z~04:40:11Z` 已写入同一索引；
- App 的 CONNACK 与服务端 `authentication_failure` 用 **username=`0200000000A1` + clientid=`android_client` + 同一次约 2 s 内的连接** 配对，不是仅凭时间接近；
- BLE 读取→上传→平台 200 用 **uid4 + 同一读取周期 + App/平台时间** 配对；不据此推断 DB 持久化。

---

## 7. 未执行项、缺口与边界（不写成 PASS）

| 子项 | 状态 | 原因/缺失条件 | 影响与恢复条件 |
|---|---|---|---|
| 嵌入式板上重新烧录/串口采集/上电五行 | **NOT EXECUTED（本轮）** | 本能力不承担 MCU 板上验证，且当前调用无 `serial_capture`/`mdk_flash` 入口 | 以 EV-041 同版本引用 + 发布包文件级复核 + 运行态 `uid4` 交叉确认；不声称板上重验 |
| 平台 `sensor_data` 字段级只读回读、按行数幂等 | **NOT EXECUTED（无授权；随 `CERT8883-UPLOAD-04` 人工延期）** | 只读 SSH 不含 DB 查询，平台无回读接口；人工已明确“不用回读，传上去就行了” | P0 设计 §8.2 将“平台回读授权”列为按可得性记录，不是 P0 必需项；HTTP 200 仅证明受理，不证明行存在 |
| `mac_addr` 来源确认 | **缺口（披露）** | `0200000000A1` 为合成测试值，非用户/网关标签/已确认资料 | 不阻断 P0 接收就绪；影响 P1/MQTT 认证与上传溯源的完整性 |
| 真实物理 >35℃/升温>0.9℃且无光/光强/功耗/433 电气 | **NOT EXECUTED** | 用户明确不能调温/调光/测功耗，无相应仪器 | 属 P1/专项，不是 P0 必需项；不以模拟代替 |
| P1 四段一致、DB 持久化、MQTT 业务发布 | 不在本调用范围 | 需注入构建/确认身份/回读授权 | 另行判定，不由本轮 PASS 外推 |
| `8083/8084` 协议级语义 | 未独立探测 | 本轮只读基线只记录 LISTEN；未做 WS/WSS/MQTT-over-WS 握手 | 与既有部署一致；不是 P0 必需项 |

**这些未执行项均未被计入 PASS**；本报告的 TEST_PASS 仅指 §1–§6 已执行并一致的必需项。

---

## 8. 逐条 acceptance 对照

| # | acceptance | 实际结果 | 判定 |
|---|---|---|---|
| 1 | 三端身份记录并与 §0.1 一致或差异解释 | §1 全部记录；APK/设备/网关/平台一致；测试身份新增已解释 | PASS |
| 2 | 同版本引用手机段 PASS + 本轮复核设备/APK/网络/守护/绑定/0xA1/读取 | §3 逐项有本轮原始证据，取得 DATA_OK | PASS |
| 3 | 以 SRV-01/CLI-02 实际结果为准：无 SAN/链错误、TLS 完成进入 CONNECT、CONNACK 分类 | §4：本轮 TLS verify 0、0 个 TLS 错误签名、CONNACK rc=5 分类记录，并有服务端同事件日志 | PASS |
| 4 | 平台只读基线（服务/端口/日志/连接数）按获准入口记录；未授权项 NOT EXECUTED | §5：SSH 只读全部执行；DB 回读 NOT EXECUTED 如实记录 | PASS（必需子项） |
| 5 | 关联键写入同一证据索引，不用时间接近猜事件 | §6 + `10_correlation_index.md`，username+clientid+同轮配对 | PASS |
| 6 | 必需项失败→段 FAIL；未授权项→NOT EXECUTED，不冒充通过；不复用旧平台段/旧证书结论 | 必需项无失败；未授权/未执行项见 §7；旧证据未作本轮判据 | 符合 |

---

## 9. 证据索引（`evidence/e2e_joint_p0_platform_20261126/`）

| 文件 | 内容 |
|---|---|
| `00_utc_start.txt` / `53_final_state.txt` | 起止 UTC/CST、git HEAD、设备在线、App PID |
| `01_devices.txt`~`05_serial.txt` | 设备 serial/型号/Android/API |
| `07_package_dump.txt` | versionCode/versionName、权限、lastUpdateTime |
| `09_ip_route.txt` / `11_connectivity.txt` / `12_wifi_dump.txt` | 默认网络 101、`ZTE_607157`、路由、wlan0 |
| `13_device_apk_sha256.txt` / `15_host_apk_sha256.txt` / `14`,`16` | 设备/主机 APK 摘要与大小 |
| `17_prefs.xml` | `gateway_ble_addr`、`macid_name_map`、`registerphone`、`mac_addr`、`measure_state`、guard 时间 |
| `18_services.txt` / `19_ps.txt` / `54_services_final.txt` / `53_final_state.txt` | 前台服务与 PID |
| `22_logcat_window.txt` / `47_logcat_1233_onward.txt` / `49_logcat_1238_onward.txt` / `51_logcat_1236_onward.txt` | 本轮 App 原始日志窗口 |
| `39_scan_log.txt` / `48_scan_log_1233.txt` / `50_scan_log_1238.txt` / `52_scan_log_1236.txt` | 日志签名计数与关键行 |
| `23_ui_dump.xml` / `43_ui_1232.png` | UI 数值与状态 |
| `31_host_tls_verify_ip.txt` / `31b_host_tls_showcerts.txt` / `31c_host_tls_plain.txt` | 主机 TLS 握手原文 |
| `32_host_leaf_fields.txt` / `leaf_8.140.23.253_round.pem` | 线上叶子 serial/SAN/指纹 |
| `33_jdca_fields.txt` / `34_jdca_filesha.txt` | 信任锚身份 |
| `35_http_18083_status.txt` / `36/37/38_http_5000_*.txt` | HTTP 只读响应 |
| `30_platform_readonly.txt` / `30b_*.txt` / `30c_platform_readonly_emqxlog.txt` / `30d_platform_readonly_version.txt` / `30e_platform_readonly_1238.txt` | SSH 只读基线（服务/端口/日志/连接数） |
| `44_fw_release_identity.txt` | 传感器发布包文件级身份 |
| `46_mysql_greeting.txt` | MySQL 9.0.1 握手 |
| `42_crash_buffer.txt` | crash buffer（仅 11-25 旧崩溃，当前 PID 无新增） |
| `24_phone_tcp_now.txt` / `25_tcp_poll_8883.txt` / `40`,`41_proc_net_*` | 手机 TCP 表采样（补充；`ss` 子命令受权限限制，未作为唯一证据） |
| `10_correlation_index.md` | 关联键与同事件配对索引 |
| `platform_p0_ssh.py` / `run_tls_check.py` / `scan_p0_log.py` / `mysql_greeting.py` / `poll_8883.py` | 可复跑只读脚本 |

---

## 10. 脱敏与边界声明

- 未回显 SSH/DB/Wi-Fi 密码；SSH 脚本从项目迁移记录读取凭据并在输出中脱敏；证据中无凭据明文。
- 未修改 App、网关、传感器或平台配置；未重装/清数据；未烧录；未重启服务。
- 未登录 Dashboard/SSH 管理接口；未 POST 由测试发起的写请求（平台 `POST /upload_data` 来自真实 App 产品路径）；未直连数据库。
- 本 PASS 仅覆盖 `E2E-JOINT-P0` 在本时间窗、本版本的已测必需项；不代表 MQTT 认证/业务发布、DB 持久化、P1、N1、物理触发、功耗/射频、正式发布或完整产品验收通过。
