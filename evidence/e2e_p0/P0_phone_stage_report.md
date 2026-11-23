# E2E-JOINT-P0 执行证据（软件测试 / 手机段复验，ADB 授权恢复后）

- 用例：`E2E-JOINT-P0`（P0 身份、手机联网与接收就绪）
- 负责角色：软件测试
- 本轮执行时间：2026-11-23 **13:24:30Z–13:44:00Z**（本地 21:24–21:44，UTC+8）
- 被测对象：Android App 候选 APK（AB-003，debug 变体）、既有网关（经 App 正常发现）、既有平台服务（8.140.23.253）
- 设计依据：`artifacts/e2e_joint_test_design.md` §3.2/§4.1/§8；`artifacts/e2e_cases.yaml` ITEM-001
- **本轮结论（本调用执行范围＝手机段）**：
  - **手机段：TEST_PASS**（下表全部手机必需项均有本轮原始证据且一致）
  - 嵌入式段：**引用 EV-041 TEST_PASS**（`evidence/test.md`、`evidence/ev041/`，本文件不重新判定；非本调用范围）
  - 平台段：属同一用例的**后续独立阶段**（`stage_order` 第三段），本调用不判定；随本调用附带的只读连通性记录见 §6，服务状态/日志/连接数/DB 回读 **NOT EXECUTED**（无 SSH 只读与 DB 回读授权）
  - **P0 整体**：由 Runtime 在平台段完成后形成；本报告的 TEST_PASS **只证明手机段**，不冒充 P0 整体通过。

> 与上一轮的关系：上一轮（16:31–16:40 本地）因手机 USB 调试未授权，手机段 NOT EXECUTED。经操作者按 `HUMAN_INPUT_REQUIRED`（HR-DFA32A85CE11）在手机端确认调试授权并回复“已准备好”后，本轮同一用例、同一前置重新实测；上一轮的主机侧 APK 摘要与平台连通性记录保留作对照（`platform_baseline_result_first.json`）。未复用上一轮的未授权状态作为本轮结论。

---

## 1. 三端身份、网关身份与关联键（同一证据索引）

| 关联键 | 本轮值 | 来源 / 证据 |
|---|---|---|
| 传感器 `uid4` | `6A002C00` | 嵌入式 EV-041 当次 `BOOT … uid=6A002C00`（2026-11-23T08:13:24Z 下载复位后）；本报告只引用 |
| 传感器构建/下载身份 | axf md5 `b6a15fed2ed5e3d0c4adc51ac87a4830`（297,380 B）；hex md5 `4d728dec9b0d28a352c46436b7586c72`；`mdk_flash` Verify OK；发布包 `fw_sensor_FD-002r8.zip` md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf` | EV-041（`evidence/ev041/`、`evidence/test.md`） |
| App 候选 APK | `dengbei_care/app/build/outputs/apk/debug/app-debug.apk`，10,370,964 B；**SHA-256 `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b`**；versionCode 10 / versionName 1.9；applicationId `com.jinyuni.dengbei_care` | 本轮主机侧 `sha256sum`、`output-metadata.json` |
| 设备侧 `base.apk` | `/data/app/com.jinyuni.dengbei_care-h0RE7rmDWPPUA40DY_WMqw==/base.apk`；**SHA-256 同 `80232c50…de4b`**；10,370,964 B | 本轮 `pm path` + 设备侧 `sha256sum`（`phone_stage_apk_digest.txt`） |
| 手机 | serial `12f046690506`；Redmi 8A（`olivelite`）；Android 10 / API 29 | 本轮 `adb devices -l`、`getprop` |
| 网关 BLE 地址 `gateway_ble_addr` | **`84:C2:E4:03:02:02`** | App“搜索附近网关”正常扫描结果（`RSSI=-50 FFE0`），经 App“保存网关地址”路径落盘（`GatewayBleSettings: gateway_ble_addr saved=…`） |
| 网关 `GwID` | **`B1:17:9C:7A:4E:E0`**（6 B，来自 FFE2 头块） | 21:43:12 App 日志 `GwID=B1:17:9C:7A:4E:E0, dev=1, valueSize=160, mtu=240` |
| 网关 MTU / 帧长 | MTU 240（`requiredMtu=35`）；`cipher length=160`；`valueSize=160`，16 B 对齐 | `phone_stage_logcat_post_bind_cycle.txt` |
| 平台服务身份 | 主机 `8.140.23.253`；HTTP 5000、EMQX 8883/1883/8083/8084、Dashboard 18083、MySQL 3306 **TCP 均可连接**；HTTP GET 无应用层应答 | `platform_baseline_result_phone_stage.json` |
| App 本地身份 | `registerphone` 空（UI“用户：未登录”）；`mac_addr` 空（日志 `mac_addr=000000000000 (平台身份)`，网关对话框“平台身份（mac_addr）：”为空） | `phone_stage_app_ui_6_mine.xml`、`phone_stage_logcat_cycles.txt` |
| 时间窗 | 本轮手机/网关/平台窗口 **2026-11-23T13:24:30Z–13:44:00Z**；嵌入式窗口引用 EV-041 08:13Z | 本报告 |

> 关联纪律：不用时间接近猜同一事件；`uid4 + 网关地址/GwID + 时间窗` 作为本轮索引。`gateway_ble_addr` 与 `mac_addr` 是不同字段，未互相推断。

---

## 2. 前置条件复核（人工回复“已准备好”后）

| 检查 | 实际结果 | 结论 |
|---|---|---|
| `adb devices -l` | `12f046690506  device … model:Redmi_8A transport_id:2` | 已授权（上一轮为 `unauthorized`） |
| `adb get-state` | `device` | 可用 |
| Wi-Fi 密码环境变量 | `RESTLESSDREAM_ANDROID_SECRET_WIFI=SET(len=10)`（只校验存在与长度，未回显值） | 可用于 `ui_set_text` |

原始：`phone_stage_device_identity.txt`（本轮）、`phone_stage_adb_state.txt`（上一轮阻塞对照）。

---

## 3. 手机段执行记录（逐项对应任务验收）

### 3.1 设备与 APK 身份

| 项 | 实际结果 |
|---|---|
| 设备 serial/型号/API | `12f046690506` / Redmi 8A `olivelite` / API 29（Android 10） |
| 安装 | `android_test install`：包 `com.jinyuni.dengbei_care`（爱守护），versionCode 10 / versionName 1.9，工具核对 SHA-256 `80232c50…de4b`，`installed_version_verified` |
| 设备侧版本 | `pm list packages --show-versioncode` → `versionCode:10`；`dumpsys package` → `versionName=1.9 minSdk=26 targetSdk=34` |
| 设备侧 base.apk 摘要 | `sha256sum …/base.apk` → `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b`，与候选逐字符一致 |

证据：`phone_stage_device_identity.txt`、`phone_stage_apk_digest.txt`、`phone_stage_package_dump.txt`。

### 3.2 蓝牙、定位与定位权限

| 项 | 实际结果 | 证据 |
|---|---|---|
| 蓝牙 | `settings get global bluetooth_on` = `1`；状态栏“蓝牙开启” | `phone_stage_radio_wifi_before.txt` |
| 定位开关 | `settings get secure location_mode` = `3`（高精度） | 同上 |
| 定位权限 | 初始 `ACCESS_FINE_LOCATION/COARSE_LOCATION: granted=false`；App 首次“搜索附近网关”触发 MIUI 权限弹窗，按真实用户路径选择**“仅在使用中允许”**后复读为 `granted=true`（fine+coarse） | `phone_stage_package_dump.txt`、`phone_stage_app_ui_8_scan.xml`、`phone_stage_package_dump_after_grant.txt` |

### 3.3 连接 `ZTE_607157` 并确认默认网络

| 步骤/观察 | 实际结果 |
|---|---|
| 打开 Wi-Fi 设置 | `am start -a android.settings.WIFI_SETTINGS`；WLAN 开关 `checked=true`；候选列表含 `ZTE_607157`（安全网络） |
| 密码注入方式 | `android_test ui_set_text` 定位 `com.android.settings:id/password`，`text_env=RESTLESSDREAM_ANDROID_SECRET_WIFI`；**未在任何命令、日志、截图或证据文件中出现明文** |
| 关联结果 | supplicant 状态链 `ASSOCIATING → ASSOCIATED → FOUR_WAY_HANDSHAKE → GROUP_HANDSHAKE → COMPLETED`；`mWifiInfo SSID: ZTE_607157, BSSID 7c:21:d8:60:71:57`；`mNetworkInfo state: CONNECTED/CONNECTED`；DHCP 成功，地址 `192.168.0.103/24`（网关 192.168.0.1） |
| 当前默认网络 | `dumpsys connectivity`：`Active default network: 100`，`NetworkAgentInfo [WIFI] CONNECTED/CONNECTED`，`VALIDATED`，`SSID="ZTE_607157"`，默认路由 `0.0.0.0/0 -> 192.168.0.1 wlan0` |
| 有界可达性 | 手机 `ping 8.140.23.253` 2/2 可达（148/268 ms）；`nc -w 3` 到 TCP 5000/8883 超时（如实记录，未据此断言平台离线） |

证据：`phone_stage_wifi_ui_1.xml`、`phone_stage_wifi_dialog_1.xml`、`phone_stage_wifi_connected.png`、`phone_stage_wifi_after_connect.txt`、`phone_stage_wifi_dump.txt`、`phone_stage_connectivity_dump.txt`、`phone_stage_network_reachability.txt`。

### 3.4 启动守护并保持运行

| 项 | 实际结果 | 证据 |
|---|---|---|
| 首页“启动” | 点击 `startGuardButton` 后按钮变为“停止” | `phone_stage_app_ui_2.xml` → `_3_after_start.xml` |
| as-built 运行态 | `MyPrefs.measure_state=stopping`；`finish_guard_time=2026-11-24 05:29:04`（guardtime 8 h） | `phone_stage_myprefs_*.xml` |
| 服务保持运行 | `dumpsys activity services`：`com.jinyuni.dengbei_care/.MqtttService` `isForeground=true`，进程 4677 存活；至 21:43:50 仍在运行 | `phone_stage_app_state_1.txt`、`phone_stage_final_state.txt` |

### 3.5 网关正常发现、绑定 `uid4`、0xA1 写入与读取终态

| 步骤/观察 | 实际结果 |
|---|---|
| 网关发现 | App“网关与上传设置 → 搜索附近网关”（真实扫描，非手填）：`找到 1 个候选：… 84:C2:E4:03:02:02 RSSI=-50 FFE0`；选中后“保存网关地址” → `GatewayBleSettings: gateway_ble_addr saved=84:C2:E4:03:02:02 (platform mac_addr untouched)`；`MyPrefs.gateway_ble_addr=84:C2:E4:03:02:02` |
| 注 | 保存前对话框显示“正在使用默认网关地址（未确认）”，本轮不使用该默认值作身份；身份取自扫描结果与 App 保存路径。 |
| 注册传感器 | “我的 → 注册传感器”输入 `6A002C00` + 别称 `e2ep0` → `MyPrefs.macid_name_map={"6A002C00":"e2ep0"}`；传感器列表显示“已保存的 ID（1） e2ep0 6A002C00” |
| 读取周期（绑定前） | 21:40:08：`cipher length=160`、`BLE frame rejected: NO_DEVICE (devCount=0)`、`GATEWAY_NO_DEVICE`；随后 `writeCharacteristic(legacy) ok=true, bytes=6`、`Chunk written OK, left=0` |
| 读取周期（绑定后） | 21:43:12：`GwID=B1:17:9C:7A:4E:E0, dev=1, valueSize=160, mtu=240`；`gateway placeholder block skipped (dev=00:00:00:00)`；`link_state=GATEWAY_NO_DATA reason=ALL_PLACEHOLDER`；`BLE batch … dev=1 valid=0 placeholder=1`；再次 `writeCharacteristic(legacy) ok=true, bytes=6` |
| 0xA1 语义 | `0xA1 | count=1 | uid4(4 B)` = **6 B**，与代码 `buildA1Payload4B` 一致；日志 N=1、bytes=6 |
| 无伪造读数 | `valid=0, placeholder=1`；本地无温度库（`databases/` 仅有 `mqttAndroidService.db`，无 `bwtemperature_database1`）；主页/列表无温湿度数值 |

证据：`phone_stage_app_ui_7_gateway.xml`、`phone_stage_app_ui_9_after_grant.xml`、`phone_stage_myprefs_after_gw.xml`、`phone_stage_myprefs_after_register2.xml`、`phone_stage_home_after_bind.png`、`phone_stage_logcat_register.txt`、`phone_stage_logcat_a1_cycle.txt`、`phone_stage_logcat_post_bind_cycle.txt`、`phone_stage_final_state.txt`。

### 3.6 测试方法修正（不是产品缺陷）

首轮注册使用别称 `E2E-P0-6A002C00`，被 App 的别称规则拒绝（`MacIdBook.sanitizeName`：仅允许 ≤3 汉字，或 1–8 位小写字母/数字），出现 Toast 且未写入。核对当前源码后改用合法别称 `e2ep0`，同一路径注册成功。此为测试输入错误，已修正并复测，不记为产品缺陷，也不改判据。

---

## 4. 上传链路资格（如进入上传）

| 项 | 实际结果 |
|---|---|
| `registerphone` | 空（UI“用户：未登录”），无测试手机号，本轮未走注册/短信路径 |
| `mac_addr`（平台网关 MAC） | 空；网关保存日志明确 `platform mac_addr untouched`，App 日志 `mac_addr=000000000000`；无已确认来源，**不猜测** |
| 结论 | 无测试身份 → **P1 上传/持久化段 NOT EXECUTED**（不得伪造，不得改 prefs/DB）；P0 手机段其余项不受影响 |

附带观测（不属 P0 必需）：App 的 MQTT 客户端以 `mac_addr=000000000000` 重连 `ssl://8.140.23.253:8883`，30 s 后 `SocketTimeoutException`（`phone_stage_logcat_cycles.txt`）。由于无测试身份，未把它判为产品缺陷；云端链路由 P1/平台段按授权条件验证。

---

## 5. 判定（本调用范围）

| 段/项 | 判定 | 依据 |
|---|---|---|
| 嵌入式 | 引用 EV-041：TEST_PASS | 非本轮判定，不扩大 |
| 手机：设备/APK/摘要 | PASS | 主机与设备侧 SHA-256 一致，版本一致 |
| 手机：蓝牙/定位/权限 | PASS | BT on、location_mode=3、fine+coarse granted=true |
| 手机：`ZTE_607157` 默认网络 | PASS | supplicant COMPLETED、`Active default network: 100 (WIFI)`、`SSID="ZTE_607157"`、默认路由存在、密码未泄露 |
| 手机：守护运行 | PASS | 按钮“停止”、`measure_state=stopping`、前台服务存活 |
| 手机：网关发现/绑定/0xA1/读取 | PASS | 扫描候选 + 保存路径、MacIdBook 含 `6A002C00`、N=1 bytes=6、dev=0 与 dev=1 placeholder 终态、无伪造读数 |
| 手机：测试身份 | 缺口（允许） | `registerphone`/`mac_addr` 空→ P1 上传/持久化 NOT EXECUTED |
| 平台：只读基线 | 部分执行 | TCP 端口基线已记录；服务状态/日志/连接数/DB 回读 NOT EXECUTED（无授权入口） |
| **手机段本调用** | **TEST_PASS** | 上表手机必需项全部有本轮原始证据 |
| **P0 整体** | **本调用不判定** | 平台段为同一用例的后续独立阶段；本调用不以手机段通过冒充整体通过 |

未发现可归因于产品的实测缺陷（`MEASURED_DEFECT` 不成立）。上一轮的 USB 未授权已由操作者解除，不再是本轮结论。

---

## 6. 平台只读基线（非手机段，随本调用附带记录）

方式：注册的浏览器测试入口（Playwright + Edge，headless）仅 GET + TCP connect；**未 POST、未登录、未写入、未连数据库、未使用 SSH**。

| 探测 | 本轮结果（13:40:55Z–13:41:20Z） |
|---|---|
| TCP 5000 / 1883 / 8883 / 3306 / 18083 / 8083 / 8084 | 全部 `open` |
| `GET /`、`/upload_data`、`/login`、`/setmac`、`/nonexistent-probe-e2e` | 均 `socket hang up`（无应用层响应） |
| 上一轮对照 | 同类 `Empty reply`/`socket hang up`；两轮一致 |
| 手机侧对照 | ICMP 可达；TCP 5000/8883 在 3 s 内超时（记录，不外推平台离线） |
| 服务状态/近期日志/连接数 | **NOT EXECUTED**：`existing_service_ssh_inspection.effective_status=unavailable_for_current_capability`（缺 `inspect_existing_service_readonly`） |
| DB 回读 | **NOT EXECUTED**：无授权 |
| 未修改/未重启/未写入 | 本轮未执行任何写操作 |

证据：`platform_baseline.mjs`、`platform_baseline_result_phone_stage.json`、`platform_baseline_result_first.json`、`platform_root_get.txt`、`platform_head_options.txt`。

---

## 7. 证据清单（本轮新增/更新）

| 文件 | 内容 |
|---|---|
| `phone_stage_device_identity.txt` | 设备型号/API/serial、包版本 |
| `phone_stage_apk_digest.txt` | 设备侧 base.apk 路径与 SHA-256 |
| `phone_stage_package_dump.txt` / `_after_grant.txt` | 权限状态（grant 前后） |
| `phone_stage_radio_wifi_before.txt` | 蓝牙/定位开关 |
| `phone_stage_wifi_ui_1.xml` | Wi-Fi 列表（开关、ZTE_607157） |
| `phone_stage_wifi_dialog_1.xml` | 密码对话框控件（脱敏；`password=true`） |
| `phone_stage_wifi_connected.png` | 连接成功截图（ZTE_607157） |
| `phone_stage_wifi_after_connect.txt` | IP 与接口状态 |
| `phone_stage_wifi_dump.txt` / `phone_stage_connectivity_dump.txt` | 系统原始 SSID/默认网络/路由 |
| `phone_stage_network_reachability.txt` | 手机到平台 ICMP/TCP 有界探测 |
| `phone_stage_final_state.txt` | 默认网络、前台服务、数据库列表 |
| `phone_stage_app_ui_1..16*.xml` / `*.png` | 隐私政策同意、首页启动、网关对话框、扫描候选、权限弹窗、注册、列表 |
| `phone_stage_myprefs_*.xml` | `measure_state`、`gateway_ble_addr`、`macid_name_map`（无凭据） |
| `phone_stage_logcat_app_1.txt` / `_scan_1.txt` / `_cycles.txt` / `_register.txt` / `_a1_cycle.txt` / `_post_bind_cycle.txt` | 本轮 App 原始日志切片（PID 4677） |
| `platform_baseline_result_phone_stage.json` 等 | 平台只读探测原始结果 |
| `p0_correlation_index.md` | 三端身份/网关身份/时间窗/关联键的单一索引 |

> 未入库：跨 3 日的完整 logcat 缓冲区（10.5 MB，含大量无关系统日志）已按当前会话时间窗/PID 切片保留，未把整包缓冲区提交。

---

## 8. 恢复条件与下一步

1. **平台段**：为 `existing_service_ssh_inspection` 提供 `inspect_existing_service_readonly` 授权（只读 `systemctl status`、近期日志、连接数），或由具备该入口的能力执行；DB 回读需单独授权。平台 5000 的 HTTP 无应用层响应需结合实际部署状态核查（本轮不判缺陷）。
2. **P1**：需隔离测试身份（测试手机号 + 已确认的平台网关 `mac_addr`），经 App 既有注册/`setMacAddress` 路径设置；当前缺口保持，P1 上传/持久化段 `NOT EXECUTED`。
3. **测试绑定保留**：`MacIdBook` 已加入 `6A002C00`（别称 `e2ep0`），网关地址 `84:C2:E4:03:02:02` 已保存；无删除授权，保留并在此披露。
4. 复测使用同一用例、同一前置与当前版本；不沿用本轮 PASS 覆盖平台/P1。

## 9. 脱敏声明

Wi-Fi 密码仅经 `android_test ui_set_text` + `text_env=RESTLESSDREAM_ANDROID_SECRET_WIFI` 注入；本证据目录不含密码明文。SSH/DB 凭据未读取、未复制。手机号未涉及（未登录）。网关地址/GwID/`uid4` 为设备标识，非凭据，按用例要求记录。

---

## 10. 勘误（2026-11-23T14:10Z，平台段追加）

- §6 及 §5 表中“TCP 5000/1883/8883/3306/18083/8083/8084 全部 `open`”**不能作为平台服务监听/可达证据**。平台段用非项目控制端口 1/54321/65000 复测，表现完全相同（connect <1 ms、无首字节），且主机默认路由经 `Meta Tunnel`（198.18.0.2，metric 0）；ICMP 物理 RTT 为 34–44 ms。判定为本机 TUN/代理路径伪造 TCP 接受。
- 该误判不影响手机段其余 PASS 项（设备/APK 摘要、Wi-Fi 默认网络、守护运行、网关发现/绑定、0xA1、读取终态）；手机段 TEST_PASS 仅证明这些手机项。
- 平台服务身份/端口/日志/连接数/DB 回读在平台段仍为 **NOT EXECUTED**；详见 `P0_platform_stage_report.md`。

