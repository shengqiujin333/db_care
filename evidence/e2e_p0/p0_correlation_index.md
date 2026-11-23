# E2E-JOINT-P0 单一证据索引（三端身份 / 网关 / 时间窗 / 关联键）

- 用例：`E2E-JOINT-P0`（P0 身份、手机联网与接收就绪）
- 负责角色：软件测试
- 本索引时间：手机段 2026-11-23T13:24:30Z–13:44:00Z；平台段首轮 13:58Z–14:10Z、8883 放行后复测 14:39Z–14:44Z（本地 21:24–22:44，UTC+8）
- 用途：把三端身份、网关身份、时间窗与关联键收敛到同一处；手机段原始证据见 `P0_phone_stage_report.md` §7，平台段见 `P0_platform_stage_report.md` §8。

## 关联键

| 键 | 值 | 所属端 | 来源 |
|---|---|---|---|
| `uid4` | `6A002C00` | 传感器 | EV-041 `BOOT … uid=6A002C00`（08:13:24Z） |
| `gateway_ble_addr` | `84:C2:E4:03:02:02` | 网关（BLE） | App“搜索附近网关”扫描候选（RSSI=-50, FFE0）+ App 保存路径 |
| 网关 `GwID` | `B1:17:9C:7A:4E:E0` | 网关（FFE2 头块） | 21:43:12 App 日志 |
| APK SHA-256 | `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b` | 手机 | 主机 APK 与设备侧 `base.apk` 一致；versionCode 10 / versionName 1.9 |
| 手机 serial | `12f046690506`（Redmi 8A, API 29） | 手机 | `adb devices -l` / `getprop` |
| 平台服务身份 | `8.140.23.253`：EMQX `emqx@127.0.0.1` running（Dashboard 18083 `/status` 200、Cowboy）；Flask `Werkzeug/3.1.8 Python/3.12.3`（5000，`/login`/`/upload_data` 仅 POST）；MySQL `9.0.1` 握手（3306，仅端口，未认证）；端口 5000/1883/8883/18083/3306 可连，8083/8084 手机超时；**服务日志/连接数/DB 回读 NOT EXECUTED** | 平台 | 平台段复测 14:39Z–14:44Z |
| EMQX 8883 证书 | subject/SAN 仅 `117.72.84.210`；issuer `Tiktok Root CA`（自签链）；2024-10-12–2026-12-21；**无 `8.140.23.253` SAN** | 平台 | `platform_stage_retest_cert.txt` |
| App→EMQX TLS | **FAIL**：`SSLHandshakeException: No subjectAltNames on the certificate match`（22:38:56、22:39:05、22:43:12 复现）；传输层 TLS 握手本身成功 | App/平台 | `platform_stage_retest_logcat_*` |
| 平台路径判定 | 首轮（13:58–14:10）主机 TCP 被 `Meta Tunnel`（198.18.0.2，metric 0）伪造接受（服务端口与控制端口 1/54321/65000 均 <1 ms `open`、无首字节），手机真实路径 5000/8883/1883/18083 均 8 s TCP 超时；**复测（14:39–14:44）操作者放行 8883 后，手机真实路径 TCP/TLS 握手成功、主机亦取得真实应用层响应**，但 App 因证书 SAN 不匹配失败 | 平台 | `platform_stage_*`（两轮） |
| Wi-Fi | SSID `ZTE_607157`，BSSID `7c:21:d8:60:71:57`，默认网络 100（WIFI），IP `192.168.0.103/24` | 手机 | `dumpsys wifi` / `dumpsys connectivity` |
| 本地绑定 | `macid_name_map={"6A002C00":"e2ep0"}` | App | `MyPrefs.xml` |
| 读取终态 | 绑定前 `devCount=0`；绑定后 `devCount=1`、`valid=0 placeholder=1`（`GATEWAY_NO_DATA`） | App/网关 | 21:40:08 / 21:43:12 App 日志 |
| 0xA1 写入 | N=1、6 B（`writeCharacteristic(legacy) ok=true, bytes=6`） | App→网关 | 21:40:08、21:43:12 App 日志 |
| 测试身份 | `registerphone` 空、`mac_addr` 空（`000000000000`） | App/平台 | UI“用户：未登录”、App 日志、网关保存日志 |
| 时间窗 | 手机段 2026-11-23T13:24:30Z–13:44:00Z；平台段 13:58Z–14:10Z、复测 14:39Z–14:44Z；嵌入式引用 08:13Z | 三端 | 本轮 |

## 证据边界

- 手机段：本轮实测，证据见 `P0_phone_stage_report.md` §3。
- 嵌入式段：引用 EV-041（`evidence/test.md`、`evidence/ev041/`），本索引不重新判定。
- 平台段：**TEST_FAIL（MEASURED_DEFECT）**。首轮因主机 `Meta Tunnel` 伪造接受与手机 TCP 超时未取得服务证据；复测（操作者放行 8883 后）服务/端口已实测，但 EMQX 服务器证书 SAN 仅 `117.72.84.210`，App 连接 `8.140.23.253:8883` 时 TLS 校验失败（`No subjectAltNames on the certificate match`）。服务日志/连接数/DB 回读仍 NOT EXECUTED。详见 `P0_platform_stage_report.md`。
- P1 上传/持久化：无测试身份，NOT EXECUTED；测试绑定与网关地址保留（无清理授权）。
