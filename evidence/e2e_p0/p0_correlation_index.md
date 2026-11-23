# E2E-JOINT-P0 单一证据索引（三端身份 / 网关 / 时间窗 / 关联键）

- 用例：`E2E-JOINT-P0`（P0 身份、手机联网与接收就绪）
- 负责角色：软件测试
- 本索引时间：2026-11-23T13:24:30Z–13:44:00Z（本地 21:24–21:44，UTC+8）
- 用途：把三端身份、网关身份、时间窗与关联键收敛到同一处；各条结论的原始证据见 `P0_phone_stage_report.md` §7。

## 关联键

| 键 | 值 | 所属端 | 来源 |
|---|---|---|---|
| `uid4` | `6A002C00` | 传感器 | EV-041 `BOOT … uid=6A002C00`（08:13:24Z） |
| `gateway_ble_addr` | `84:C2:E4:03:02:02` | 网关（BLE） | App“搜索附近网关”扫描候选（RSSI=-50, FFE0）+ App 保存路径 |
| 网关 `GwID` | `B1:17:9C:7A:4E:E0` | 网关（FFE2 头块） | 21:43:12 App 日志 |
| APK SHA-256 | `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b` | 手机 | 主机 APK 与设备侧 `base.apk` 一致；versionCode 10 / versionName 1.9 |
| 手机 serial | `12f046690506`（Redmi 8A, API 29） | 手机 | `adb devices -l` / `getprop` |
| 平台服务身份 | `8.140.23.253`；TCP 5000/1883/8883/3306/18083/8083/8084 开放 | 平台 | 本轮只读 TCP 探测 |
| 平台 HTTP 观测 | 5000 上 GET 路径均 `socket hang up`（无应用层应答） | 平台 | 本轮只读 GET |
| Wi-Fi | SSID `ZTE_607157`，BSSID `7c:21:d8:60:71:57`，默认网络 100（WIFI），IP `192.168.0.103/24` | 手机 | `dumpsys wifi` / `dumpsys connectivity` |
| 本地绑定 | `macid_name_map={"6A002C00":"e2ep0"}` | App | `MyPrefs.xml` |
| 读取终态 | 绑定前 `devCount=0`；绑定后 `devCount=1`、`valid=0 placeholder=1`（`GATEWAY_NO_DATA`） | App/网关 | 21:40:08 / 21:43:12 App 日志 |
| 0xA1 写入 | N=1、6 B（`writeCharacteristic(legacy) ok=true, bytes=6`） | App→网关 | 21:40:08、21:43:12 App 日志 |
| 测试身份 | `registerphone` 空、`mac_addr` 空（`000000000000`） | App/平台 | UI“用户：未登录”、App 日志、网关保存日志 |
| 时间窗 | 2026-11-23T13:24:30Z–13:44:00Z（嵌入式引用 08:13Z） | 三端 | 本轮 |

## 证据边界

- 手机段：本轮实测，证据见 `P0_phone_stage_report.md` §3。
- 嵌入式段：引用 EV-041（`evidence/test.md`、`evidence/ev041/`），本索引不重新判定。
- 平台段：仅只读 TCP/HTTP 探测；服务状态/日志/连接数/DB 回读 NOT EXECUTED（无授权入口）。
- P1 上传/持久化：无测试身份，NOT EXECUTED；测试绑定与网关地址保留（无清理授权）。
