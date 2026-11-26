# Android App 对平台 8883 的 TLS/MQTT 复验报告（CERT8883-CLI-02，第 6 轮）

- 负责角色：软件测试
- 能力：软件验证
- 用例：`CERT8883-CLI-02`（TD-CERT8883-001 §3.2，`case_kind=certificate_client_verification`，阶段 `phone`）
- 执行时间窗（CST）：2026-11-26 11:28:53 ~ 11:40:11（UTC 03:28:53Z ~ 03:40:11Z）
- 被测对象：Android App `com.jinyuni.dengbei_care` 作为真实 TLS/MQTT 消费端，连接 `ssl://8.140.23.253:8883`
- 被测版本身份：AB-003（未变化）——versionCode 10 / versionName 1.9，APK SHA-256 `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b`
- 结果：**TEST_PASS（仅限“证书主机名/链错误已消除 + TLS 完成并进入 MQTT CONNECT”这一缺陷维度）**
- 同轮上游：`CERT8883-SRV-01` 第 5 轮 PASS（叶子证书 serial `…B83`、SAN 含 `IP:8.140.23.253`、仅以 `jd-ca.crt` 校验链 OK）

---

## 1. 结论

1. 在声明的同一时间窗内，真实 App 对 `ssl://8.140.23.253:8883` **未再出现** `SSLHandshakeException: No subjectAltNames on the certificate match`，也**未出现** `Trust anchor for certification path not found` / `PKIX path building failed` / `CertPathValidatorException` / `SSLPeerUnverifiedException`（窗口内 0 次；App 当前 logcat 全量缓冲 0 次）。
2. App 完成 TLS 并进入 MQTT CONNECT：Paho 收到服务端 CONNACK 后才抛出 `MqttException`，堆栈位于 `ClientState.notifyReceivedAck → CommsReceiver.run`，不在 TLS/主机名校验。网络层同时捕获到手机 `192.168.0.103` 到 `8.140.23.253:8883` 的 `ESTAB` TCP 连接，时间与 Paho 重连日志相邻。
3. CONNACK 实际结果为 **`rc=5 无权连接`（not authorized，认证层）**，`username=0200000000A1`。按用例分类规则：**本缺陷（证书主机名/链）维度 PASS；认证层缺口另记**，不写成 TLS 失败，也不据此判上传/P1 通过。
4. 同一窗口内网关 BLE 读取仍进入明确终态：`DATA_OK`（`GwID=B1:17:9C:7A:4E:E0`、`dev=1`、`id=6A002C00`、`T=22.3°C`、`H=35.6%`），UI 显示“网关已连接（最近读取 11:31）”，与日志一致；随后另有 BLE 连接超时的 `CONNECT_FAILED` 终态，同样有明确状态与 UI 文案。
5. 修复未改变 App 连接地址、端口、TLS 校验行为或信任锚：APK 内 `assets/jd-ca.crt` 的 X.509 SHA-256 仍为 `75:33:09:B3:…:15:2B`，文件 SHA-256 `5c17cf74…e1e4`，与修复前一致；`CloudConfig` 仍为 `ssl://8.140.23.253:8883`。

## 2. 执行前身份（acceptance 第 1 条）

| 项 | 实测 | 证据 |
|---|---|---|
| 设备 serial | `12f046690506` | `01_device_identity.txt`/`02_device_apk_sha256.txt` |
| 型号 / Android / API | Redmi 8A / Android 10 / API 29 | `01_device_apk_identity.txt`、`adb getprop` |
| APK 路径（设备侧） | `/data/app/com.jinyuni.dengbei_care-h0RE7rmDWPPUA40DY_WMqw==/base.apk` | `02_device_apk_sha256.txt` |
| APK SHA-256（主机侧） | `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b` | `sha256sum` 主机侧 |
| APK SHA-256（设备侧） | `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b` | `02_device_apk_sha256.txt` |
| versionCode / versionName | 10 / 1.9 | `03_dumpsys_package.txt:100-101` |
| 与 AB-003 比较 | **完全一致**，沿用 AB-003 身份，不触发“新组合”分支 | — |
| App 信任锚（APK 内） | X.509 SHA-256 `75:33:09:B3:9F:1B:8F:85:B4:D7:57:76:BF:D3:8A:42:52:0D:C1:A0:5E:A2:0B:A3:11:7E:B3:9C:BA:B2:15:2B`；文件 SHA-256 `5c17cf740ffdf416c2f31f90172ad3565966cd62c0952920b9b2ac974aefe1e4` | `16_apk_trust_anchor.txt` |
| 信任锚是否变更 | 未变更（与设计基线一致） | 同上 |

## 3. 网络、权限与敏感信息（acceptance 第 2 条）

| 项 | 实测 | 证据 |
|---|---|---|
| 当前 SSID | `ZTE_607157`（BSSID `7c:21:d8:60:71:57`，Supplicant state `COMPLETED`） | `35_wifi_window2.txt:1061` |
| 默认路由 | `192.168.0.0/24 dev wlan0 proto kernel scope link src 192.168.0.103` | `36_route_window2.txt` |
| wlan0 地址 | `192.168.0.103/24`（UP） | `36`/`35` |
| 蓝牙 | `bluetooth_on=1` | `07_bluetooth_on.txt` |
| 定位 | `location_mode=3` | `adb settings get secure location_mode` |
| App 权限 | `BLUETOOTH/BLUETOOTH_ADMIN/ACCESS_FINE_LOCATION/ACCESS_COARSE_LOCATION/FOREGROUND_SERVICE` 均 `granted=true` | `03_dumpsys_package.txt:122-161` |
| 密码注入 | 本轮未做任何密码输入，未使用 `ui_set_text`/`text_env`；证据中无 MQTT 密码明文。`mac_addr=0200000000A1`、`registerphone=14000000001` 为既有测试身份，非密码 | `17_prefs_app.xml` |

## 4. App 守护与时间窗起点（acceptance 第 3 条）

- **主证据窗口起点**：`2026-11-26 11:28:53 CST`（进程 `14224` 由 `MainActivity` 启动；logcat 过滤起点 `11-26 11:28:53.000`）。
- **PID**：`14224`（`com.jinyuni.dengbei_care`），结束时（11:40:11 CST）仍在。
- **MqtttService 前台存活**：`ServiceRecord{… .MqtttService}`、`isForeground=true foregroundId=1`、`app=ProcessRecord{… 14224}`（`33_services_window2.txt`、`46_services_end.txt`）。
- 说明：上一会话进程 `12980` 在 11:25:57 被系统 `SwipeUpClean` 结束（`25_events_buffer.txt:33236`），随后 11:28:53 重新启动为 `14224`；本轮以重启后的完整窗口为主证据，早先窗口仅作交叉参考。

## 5. 逐项判据与实测（acceptance 第 4~8 条）

| # | 判据 | 实测（同一窗口 11:28:53–11:40:11 CST） | 结果 |
|---|---|---|---|
| 1 | 不再出现 `No subjectAltNames on the certificate match` / 链/信任锚错误 | 窗口内 0 次；当前 App logcat 全量缓冲 0 次（36 次重连全部到达 CONNACK） | PASS |
| 2 | TLS 握手完成并进入 MQTT CONNECT | 11:33:54.720 重连 → 11:33:56.020 CONNACK；11:38:54.775 重连 → 11:38:55.875 CONNACK。异常为 `MqttException`，堆栈 `notifyReceivedAck`/`CommsReceiver`（已收到服务端响应），不在 TLS | PASS |
| 3 | 网络层证明到 8883 的连接 | 11:38:57.5 / 11:38:58.7 采样到 `ESTAB 192.168.0.103:43424/43426 → 8.140.23.253:8883`，与 11:38:54.775 重连相邻 | PASS |
| 4 | CONNACK 实际结果 | `rc=5 无权连接`（not authorized），username `0200000000A1`（`43_mqtt_slice_113854.txt`） | PASS（认证层结果，见 §6） |
| 5 | UI 与日志一致 | 11:31:57 `link_state=DATA_OK`，UI 同窗显示“网关已连接（最近读取 11:31）”、`22.3°C`；早先窗口 11:23:51 `CONNECT_FAILED`，11:24:33 捕获的 UI 显示“网关连接失败（重试 / 检查网关电源）” | PASS（附 §7 说明） |
| 6 | 同一窗口网关 BLE 读取进入明确终态 | `11:31:54` 开始，`11:31:57` `DATA_OK`，`Successfully connected and read data on attempt 1`；另有 `CONNECT_FAILED` 明确终态 | PASS |
| 7 | 服务器证书同窗身份 | 03:35:28Z 重新握手：serial `77ADCFD9…B83`、SHA-256 `E7:AD:1E:…:88:84`、SAN `IP:8.140.23.253, IP:117.72.84.210`，`Verification: OK`、`Verify return code: 0 (ok)` | PASS |
| 8 | 仍出现 SAN/链错误则 FAIL | 未命中 | — |

### 关键原始行

```
11-26 11:31:57.414 14224 14241 I MqttService: GwID=B1:17:9C:7A:4E:E0, dev=1, valueSize=160, mtu=240
11-26 11:31:57.416 14224 14241 I MqttService:  id=6A002C00, H=35.6%, T=22.3°C
11-26 11:31:57.460 14224 14241 I MqttService: link_state=DATA_OK msg=网关已连接（最近读取 11:31）
11-26 11:31:57.946 14224 14259 I CloudUpload: uploaded 1 reading(s)
11-26 11:33:54.720 14224 14259 D MqttService: Attempting to reconnect to broker
11-26 11:33:56.020 14224 14224 E MqttService: Failed to connect to broker
11-26 11:33:56.020 14224 14224 E MqttService: 无权连接 (5)
11-26 11:33:56.020 14224 14224 E MqttService: 	at org.eclipse.paho.client.mqttv3.internal.ClientState.notifyReceivedAck(ClientState.java:1053)
11-26 11:38:54.775 14224 14259 D MqttService: Attempting to reconnect to broker
11-26 11:38:55.875 14224 14224 E MqttService: Failed to connect to broker
11-26 11:38:55.875 14224 14224 E MqttService: 无权连接 (5)
```

（`39_scan_window2.txt`：窗口内 `tls_error_signatures_total=0`；`45_scan_full_final.txt`：当前全量缓冲同样为 0，36 次重连均落 `无权连接 (5)`。）

## 6. 失败分层与边界（acceptance 第 6 条）

| 层 | 结果 | 说明 |
|---|---|---|
| 证书主机名 / 链（本用例缺陷维度） | **PASS** | 新叶子证书 SAN 含 `IP:8.140.23.253`，App 未再报 SAN/链错误，TLS 完成并收到 CONNACK |
| MQTT 认证 | **缺口（另记）** | `CONNACK rc=5 not authorized`，用户名 `0200000000A1`；属用户名/口令/授权层，不是 TLS |
| HTTP 上传 / 持久化 | 本用例不判定 | 11:31:57 `CloudUpload: uploaded 1 reading(s)` 仅为 HTTP 层受理日志，不证明平台持久化；`CERT8883-UPLOAD-04` 的字段级回读已按人工确认延期，另记 |
| 联合正向 P1 | 本用例不判定 | 不得由 CONNACK/TLS 结果推断 |

认证层缺口建议由实现/平台侧核对 EMQX 认证库与 ACL 是否接受测试身份 `0200000000A1`；本轮不修改任何产品配置。

## 7. UI broker 状态的可见性说明（不影响证书维度判定）

- 首页可见的网关状态由 `HomeViewModel._linkStatus` 驱动（`HomeFragment.kt:89`），本轮实测与 `link_state` 日志逐字一致。
- 代码中的 MQTT broker 标签 `_mqtt_broker_state`（“网关：已联网/断网”）在 `MqtttService.kt:1579/1586/1637/1642` 赋值，但唯一观察者 `NotificationsFragment.kt:138-141` **已被注释**，AB-003 界面当前不渲染该 broker 标签。因此“UI 上单独读取 broker 标签”这一观察项在 AB-003 中不可执行；本报告用可见的网关状态与日志一致性替代，并如实披露该 UI 限制。该限制不是 TLS 失败，也不改变 §5 的证书维度结论。

## 8. 未执行 / 边界

- 未修改 App 代码、未关闭校验、未使用 `--insecure`、未更换信任锚、未改连接地址/端口。
- 未卸载/清数据；未修改网关或传感器；未烧录。
- 未执行真实物理温度/光照/功耗条件；未执行 P1 注入构建；未做平台数据库字段级回读（人工确认延期，`CERT8883-UPLOAD-04`）。
- 本 PASS 仅证明 `CERT8883-CLI-02` 在本身份（APK `80232c50…de4b`、serial `12f046690506`）与本时间窗内的证书/TLS 维度；不证明 MQTT 认证、上传持久化或产品整体验收。

## 9. 证据索引（`evidence/software_verify_cert8883_cli02_20261126/`）

| 文件 | 内容 |
|---|---|
| `01_device_identity.txt` | 设备 serial/型号/Android/API 与主机侧 APK 摘要 |
| `02_device_apk_sha256.txt` | 设备侧 `base.apk` SHA-256 |
| `03_dumpsys_package.txt` | versionCode/versionName、权限授予 |
| `05_ps.txt` / `23_ps_final.txt` / `34_ps_window2.txt` | App PID |
| `16_apk_trust_anchor.txt` | APK 内 `jd-ca.crt` 指纹与摘要（未变更） |
| `17_prefs_app.xml` | `mac_addr`/`registerphone` 既有测试身份（无密码） |
| `31_logcat_window2.txt` | 主证据窗口原始 logcat（11:28:53 起） |
| `33_services_window2.txt` / `46_services_end.txt` | `MqtttService` 前台与 PID |
| `35_wifi_window2.txt` / `36_route_window2.txt` | `ZTE_607157` 与默认路由原文 |
| `37_ui_dump_window2.xml` / `38_ui_window2.png` | UI“网关已连接（最近读取 11:31）”与温度 |
| `39_scan_window2.txt` | 窗口扫描：TLS 错误 0、CONNACK 5、BLE DATA_OK |
| `40_scan_full_after_fix.txt` / `45_scan_full_final.txt` | 全量缓冲扫描：TLS 错误 0，全部 CONNACK 5 |
| `41_leaf_correlation.txt` | 同轮线上叶子证书 serial/SAN/链校验 |
| `42_tcp_poll_8883.txt` | 手机→`8.140.23.253:8883` 的 ESTAB 采样 |
| `43_mqtt_slice_113854.txt` | 11:38:54 重连与 CONNACK 原始切片 |
| `25_events_buffer.txt` | 上一进程 `SwipeUpClean` 结束与重启事件 |
| `scan_cli02_log.py` | 本用例日志扫描脚本（可复跑） |
