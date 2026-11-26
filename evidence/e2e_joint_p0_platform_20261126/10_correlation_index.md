# E2E-JOINT-P0 关联键与证据索引（证书修复后平台段，2026-11-26）

- 用例：`E2E-JOINT-P0`（受影响联合用例 P0；`stage_order`: embedded_reference → phone → platform）
- 本调用阶段：`platform`（受证书修复影响的平台段复验），并以同版本身份复核嵌入式/手机段
- 执行时间窗（UTC）：**2026-11-26T04:29:43Z ~ 2026-11-26T04:40:11Z**（CST 12:29:43 ~ 12:40:11）
- 设计依据：`artifacts/software_test_design_cert8883.md` §3.5；`artifacts/e2e_joint_test_design.md` TD-E2E-SW-006 §4.1/§8
- 被测对象：Android App（AB-003）→ 平台 `8.140.23.253` 的 EMQX 8883 TLS/MQTT；既有传感器/网关同版本身份

## 1. 单一关联索引（逐项均引用本轮原始证据）

| 关联键 | 本轮值 | 来源证据 |
|---|---|---|
| 传感器 `uid4` | `6A002C00` | 本轮 App 读取 `id=6A002C00`（`52_scan_log_1236.txt` 12:38:17）；同板历史 `BOOT uid=6A002C00`（`evidence/test.md` EV-041） |
| 传感器构建/下载身份 | axf md5 `b6a15fed2ed5e3d0c4adc51ac87a4830`（297,380 B）；hex md5 `4d728dec9b0d28a352c46436b7586c72`（75,275 B）；`mdk_flash` Verify OK | EV-041（`evidence/test.md` §身份闭合）；本轮文件级复核发布包 md5 一致（`44_fw_release_identity.txt`） |
| 传感器发布包 | `fw_sensor_FD-002r8.zip`，134,791 B，md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf`，sha256 `16ad3f69a9c83fc4984cf6efa2d9c0f43b5c5498716b5f253df090ef955bf4d0`；源提交 `82a7e74e…` | 本轮 `44_fw_release_identity.txt`；`fw_release/PACKAGE.txt` |
| 网关 BLE 地址 | `84:C2:E4:03:02:02` | 本轮 `17_prefs.xml`；`52_scan_log_1236.txt` 12:38:17 `GwID=B1:17:9C:7A:4E:E0`（同轮 BLE target 行见 `22_logcat_window.txt`） |
| 网关 `GwID` | `B1:17:9C:7A:4E:E0`（6 B，FFE2 头块） | 本轮 `51_logcat_1236_onward.txt` 12:38:17、`22_logcat_window.txt` 12:27:04 |
| 网关帧/MTU | `dev=1`、`valueSize=160`、`mtu=240`（requiredMtu=35） | 本轮 `52_scan_log_1236.txt` 12:38:16–17 |
| APK SHA-256 | `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b`（主机侧=设备侧） | 本轮 `13_device_apk_sha256.txt`、`15_host_apk_sha256.txt` |
| APK 版本/大小 | versionCode 10 / versionName 1.9；10,370,964 B | 本轮 `07_package_dump.txt`、`14_device_apk_size.txt`、`16_host_apk_size.txt` |
| 手机 | serial `12f046690506`，Redmi 8A（`olivelite`），Android 10 / API 29 | 本轮 `01_devices.txt`、`02_model.txt`、`03_android_release.txt`、`04_sdk.txt`、`05_serial.txt` |
| 默认网络 | `Active default network: 101`（WIFI），SSID `ZTE_607157`（BSSID `7c:21:d8:60:71:57`），`VALIDATED`，wlan0 `192.168.0.103/24`，默认路由 `0.0.0.0/0 -> 192.168.0.1 wlan0` | 本轮 `11_connectivity.txt`、`12_wifi_dump.txt:1061`、`09_ip_route.txt` |
| 守护运行 | PID `14224`；`MqtttService` `isForeground=true`；`measure_state=stopping`；`finish_guard_time=2026-11-26 13:46:12` | 本轮 `18_services.txt`、`19_ps.txt`、`53_final_state.txt`、`17_prefs.xml` |
| 测试身份 | `registerphone=14000000001`；`mac_addr=0200000000A1`（均为合成测试值，`mac_addr` 来源未确认） | 本轮 `17_prefs.xml`；披露见报告 §7 |
| 平台 IP/服务 | `8.140.23.253`；EMQX `5.8.0`（`Node 'emqx@127.0.0.1' 5.8.0 is started`），PID 299216；Flask `Werkzeug/3.1.8 Python/3.12.3`，PID 312368；MySQL `9.0.1`（仅握手） | 本轮 `30d_platform_readonly_version.txt`、`35_http_18083_status.txt`、`36_http_5000_root.txt`、`46_mysql_greeting.txt`、`30_platform_readonly.txt` |
| 平台 UTC 时间窗 | `2026-11-26T04:29:43Z ~ 2026-11-26T04:40:11Z` | `00_utc_start.txt`、`53_final_state.txt` |

## 2. 同一事件配对（不靠时间接近猜事件）

### 2.1 App TLS/MQTT 重连 ↔ EMQX 服务端认证日志

| 轮次 | App 端（CST/UTC） | EMQX 服务端（CST） | 关联依据 |
|---|---|---|---|
| R1 | 12:23:54.915 重连 → 12:23:56.100 `无权连接 (5)`（UTC 04:23:56Z） | 12:23:57.898 `authentication_failure, clientid=android_client, username=0200000000A1, reason=not_authorized, peername=27.128.89.184:22907` | 固定 `clientid/android_client` + `username=0200000000A1`（=本轮 `mac_addr`）+ 同一次约 2 s 内的连接 |
| R2 | 12:28:54.927 → 12:28:55.978 `无权连接 (5)` | 12:28:57.794 同上，peername `:22843` | 同上 |
| R3 | 12:33:54.940 → 12:33:55.977 `无权连接 (5)` | 12:33:57.809 同上，peername `:22739` | 同上 |
| R4 | 12:38:54.953 → 12:38:55.990 `无权连接 (5)` | 12:38:57.816 同上，peername `:22873` | 同上 |

证据：App 端 `22_logcat_window.txt`、`47_logcat_1233_onward.txt`、`49_logcat_1238_onward.txt`；服务端 `30c/30d/30e_platform_readonly*.txt` 的 `emqx_log_tail`。

判读：App 端在收到服务端 CONNACK 后才抛 `MqttException`（栈在 `notifyReceivedAck`/`CommsReceiver`），服务端日志同时记录同一 username 的认证失败，说明 **TCP/TLS 已完成、MQTT CONNECT 已发出并被 EMQX 处理**；失败发生在认证层（`not_authorized`），不是 SAN/链/TLS 失败。

### 2.2 App BLE 读取 → 上传 → 平台受理

| 轮次 | App BLE/上传（CST） | 平台访问日志（CST） | 关联依据 |
|---|---|---|---|
| U1 | 12:27:04 `id=6A002C00, H=37.5%, T=21.2°C` → `link_state=DATA_OK` → `writeCharacteristic bytes=6` → 12:27:05.067 `CloudUpload: uploaded 1 reading(s)` | 12:27:07 `POST /upload_data HTTP/1.1 200` | 时间窗 + 同一 `uid4`；仅证明 HTTP 受理，不证明 DB 行 |
| U2 | 12:38:17 `id=6A002C00, H=35.2%, T=23.1°C` → `link_state=DATA_OK` → `writeCharacteristic bytes=6` → 12:38:17.864 `CloudUpload: uploaded 1 reading(s)` | 12:38:20 `POST /upload_data HTTP/1.1 200` | 同上 |

证据：App 端 `39_scan_log.txt`、`52_scan_log_1236.txt`；平台 `30c/30e_platform_readonly*.txt` 的 `upload_tail_since`。

## 3. 时间窗与版本边界

- 本轮所有手机侧原始日志时间戳为 CST（UTC+8），平台 `date -u` 与主机 `date -u` 为 UTC；配对时同时记录两者，未用“时间接近”单独推断。
- 本轮未烧录/复位传感器、未改动网关；嵌入式段为同版本引用（见报告 §2）。
- 本轮未使用 2026-11-23 旧平台段 FAIL 或旧证书结论；旧证据仅作为历史对照，未作为本轮判据。
