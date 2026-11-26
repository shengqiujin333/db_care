# E2E-JOINT-P1 受控注入四段一致验证报告（证书修复后，第 1 轮）

- 负责角色：软件测试
- 能力：软件验证
- 用例：`E2E-JOINT-P1`（`TD-E2E-SW-006` §4.3 / `TD-CERT8883-001` §3.6 `C6`；`case_kind=affected_joint_positive_event`）
- 本调用目标：在证书修复后的当前组合版本上，证明 App→EMQX 的 TLS 阻断已解除；并在条件具备时用固件测试构建受控注入 `367`，完成 `uid4+367+h` 的嵌入式判定/冻结/发送 → 网关 → App 读取/解码/本地消费/上传 → 平台受理/回读四段一致。
- 执行时间窗（UTC）：**2026-11-26T04:51:58Z ~ 2026-11-26T05:02:56Z**（CST 12:51:58 ~ 13:02:56）
- 被测对象：既有传感器标准发布构建 `fw_sensor_FD-002r8`（uid4 `6A002C00`，本轮未烧录/未复位）+ 既有网关（BLE `84:C2:E4:03:02:02`，`GwID=B1:17:9C:7A:4E:E0`）+ Android App `com.jinyuni.dengbei_care` vC10/vN1.9（APK SHA-256 `80232c50…de4b`）+ 平台 `8.140.23.253`（EMQX 5.8.0 / Flask `user_script`）。
- **本轮结果：`TEST_FAIL` —— `E2E-JOINT-P1` 整例 `NOT EXECUTED`（P1 不通过、不得部分通过）。**
  - **证书维度（本窗口实测）：PASS** —— App→EMQX 无 SAN/链错误，TLS 完成并进入 MQTT CONNECT；CONNACK 实际结果 `rc=5 无权连接`（认证层，不是 TLS/证书失败）。
  - **P1 四段：全部 `NOT EXECUTED`** —— 缺少固件测试构建（无注入/TINJ）、`mac_addr` 未确认、平台字段级回读无授权；未产生任何 `uid4+367+h` 事件，四段无法成立。
  - 证书维度通过**不**构成 P1 通过；`CERT8883-CLI-02` 的既有 PASS 也未被当作 P1 PASS。

---

## 1. 结论摘要

| 维度 | 实际结果 | 判定 |
|---|---|---|
| 前置 1：P0 已完成 | 引用同版本 `E2E-JOINT-P0` 第 1 轮 PASS 报告与手机/平台同 APK、同网关身份 | 满足（引用） |
| 前置 2：N1 或等价干净基线 | 已记录网关实际缓存值与“无 36.7”证据；板上 `S/G/P r=0/s=0` 无入口观测（等价记录） | 等价记录（非 N1 独立 PASS） |
| 前置 3：固件测试构建（注入 367 / TINJ / 正式排除 / Verify OK） | **不存在**：源码无注入宏/`TINJ`，Keil 仅单 target，发布包仅标准构建且无测试字符串 | **缺失 → P1 NOT EXECUTED** |
| 前置 4：隔离测试身份与已确认 `mac_addr` | `registerphone=14000000001`（模拟）、`mac_addr=0200000000A1`（合成、来源未确认）；EMQX 认证拒绝 | **不满足** |
| 前置 5：平台字段级回读授权 | 无回读接口（仅 POST 路由），无 DB 回读授权；人工延期仅覆盖 `CERT8883-UPLOAD-04`，不含 P1 | **缺失 → 平台段 NOT EXECUTED** |
| 证书维度（同窗口） | 无 SAN/链错误；TLS 完成；CONNACK `rc=5` 实际记录；服务端同一连接 `authentication_failure` | **PASS（仅证书/TLS 维度）** |
| 嵌入式段（第 1/2/3 次 S、TINJ、P、Verify OK） | 无测试构建、无烧录、无 COM43 入口 | **NOT EXECUTED** |
| 手机段（注入后 36.7/`h`/DATA_OK/本地消费/上传） | 未发生注入；仅观测到真实缓存值读取 | **NOT EXECUTED** |
| MQTT 段（`/<mac>/a/r0` 载荷） | 无注入事件；当前合成身份连接被拒（`rc=5`），实际原因已记录 | **NOT EXECUTED** |
| 平台段（`sensor_data` 行回读、幂等） | 无注入事件；无回读授权；仅见缓存值 HTTP 200 | **NOT EXECUTED** |
| 关联 `uid4+367+h` 四段一致 | 无 `367`/`T=36.7` 事件 | 不适用；P1 不通过 |

**一句话**：证书修复后 App→EMQX 的 TLS 阻断在本窗口确已解除（PASS），但 P1 所需固件测试构建、确认身份与平台回读均不具备，四段没有任何一段获得 `uid4+367+h` 的实际证据，因此 `E2E-JOINT-P1` 整例 `NOT EXECUTED`，不判通过。

---

## 2. 被测版本身份（本轮实测）

| 端 | 本轮身份 | 证据 |
|---|---|---|
| 传感器（标准发布构建） | `fw_sensor_FD-002r8.zip` 134,791 B；md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf`；sha256 `16ad3f69…f4d0`；成员 axf md5 `b6a15fed…`、hex md5 `4d728dec…`；源提交 `82a7e74e…`；`BOOT uid=6A002C00` | `evidence/e2e_joint_p0_platform_20261126/44_fw_release_identity.txt`、本轮 `21_fw_testbuild_audit.txt` |
| 传感器运行态 | App 读到 `id=6A002C00`；本轮未烧录/复位；板上镜像身份仅按同版本引用，未重新 Verify | `20_logcat_window.txt` 12:54:37；`37_n1_equivalent_baseline.md` |
| 网关 | BLE `84:C2:E4:03:02:02`；`GwID=B1:17:9C:7A:4E:E0`；`dev=1`、`valueSize=160`、`mtu=240` | `20_logcat_window.txt` 12:54:37 |
| 安卓 App | `com.jinyuni.dengbei_care` vC10 / vN1.9；设备侧 `base.apk` SHA-256 `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b`，10,370,964 B；PID 14224（前后一致） | `05_package_dump.txt`、`06_apk_path.txt`、`08_apk_sha256.txt`、`07_pid.txt`、`36_pid_end.txt` |
| 手机 | Redmi 8A（`olivelite`），Android 10 / API 29，serial `12f046690506` | `01_model.txt`~`04_serial.txt` |
| 网络 | 默认网络 101（WIFI，VALIDATED）；SSID `ZTE_607157`、BSSID `7c:21:d8:60:71:57`、RSSI -44；wlan0 `192.168.0.103/24` | `13_connectivity.txt`、`32_wifi_head.txt`、`33_wlan0.txt`、`14_ip_route.txt` |
| 平台服务 | `user_script` active/running，PID 312368，启动 2026-11-25 21:59:02，`NRestarts=0`；`/root/abr.py` 8,688 B，sha256 `a062b05f524dc8f6bce596e4571d339f697ef909b318503ad418ee0add24017c`；EMQX PID 299216，启动 2026-11-25 10:58:15，`NRestarts=0` | `22_platform_readonly.txt` |
| 8883 证书 | serial `77ADCFD924BA0F87F845C81934D4A3F346D4BB83`；SHA-256 `E7:AD:1E:46:FA:5B:5D:06:02:C5:AD:04:B7:8B:1C:AF:2E:78:EE:47:8A:9F:6A:79:EA:B6:3B:D5:54:BD:88:84`；issuer `CN=Tiktok Root CA`；notAfter `2028-11-24T02:55:16Z`；SAN `IP:8.140.23.253, IP:117.72.84.210` | `24_host_tls_verify_ip.txt`、`24b_host_tls_showcerts.txt`、`24c_host_leaf_fields.txt` |
| App 信任锚 | `dengbei_care/app/src/main/assets/jd-ca.crt`（未更换）；App 连接 `ssl://8.140.23.253:8883` | `24_host_tls_verify_ip.txt`、`CloudConfig.kt:18-27` |

---

## 3. 前置逐项记录（acceptance 第 1 条）

### 3.1 P0 已完成

引用 `artifacts/software_test_report_e2e_joint_p0_r1.md`（UTC 2026-11-26T04:29:43Z~04:40:11Z，`E2E-JOINT-P0` 本调用必需项 PASS）：同 APK SHA-256、同手机、同网关绑定/读取就绪、平台只读基线、`CONNACK rc=5` 分类记录。本轮设备/APK/网关身份与之一致，未使用旧平台段或旧证书结论。

### 3.2 N1 或等价干净基线（`clean_or_explained_gateway_cache_baseline`）

- 已记录网关实际缓存状态与可区分性：App 在 12:46:27 读到 `23.5°C / 35.6%`，12:54:37 读到 `22.4°C / 36.4%`；本地库 140 行中 **无 `36.6–36.8` 温度行**；窗口日志 **`T=36.7` = 0、`TINJ` = 0**。
- 缓存复读语义已披露：App 每个读取周期对当前缓存值生成新的 `receiveTime` 并上传（12:54:37.635 `TempDb time=1795668877`，12:54:37.834 `CloudUpload uploaded 1 reading(s)`，平台 12:54:40 `POST /upload_data 200`）；该上传对应缓存值，不是新物理采样，更不是 P1 注入。
- 板上 `S/G/P r=0/s=0` 因本能力无 COM43/烧录入口未观测；按 `e2e_joint_test_design.md` §4.2“无法执行则记录实际网关缓存状态并可区分”处理为**等价基线**。
- 详见 `37_n1_equivalent_baseline.md`。**这不是 `E2E-JOINT-N1` 的独立 PASS。**

### 3.3 固件测试构建（注入 `367`，`firmware_test_build_injection`）——**缺失**

实测审计（`21_fw_testbuild_audit.txt`，只读）：

| 检查 | 结果 |
|---|---|
| 源码 `USER/` 下 `SENSOR_TEST_INJECT` / `TINJ` | **0 个文件**（`measure.c` 注入点附近无测试代码） |
| `measure_sample()` 之后、`sensor_decide_report()` 之前注入 | 源码中不存在该测试分支 |
| Keil 工程 target | 仅一个 `Project` target；`Define` 条目为空；工程文本不含 `SENSOR_TEST_INJECT`/`TINJ` |
| 发布包 `fw_sensor_FD-002r8.zip` 成员 | 仅 `axf`、`hex`、`PACKAGE.txt`；二进制内 `TINJ`/`SENSOR_TEST_INJECT`/`INJECT` 命中 **NONE** |
| 仓库内固件 zip | 仅 `fw_release/fw_sensor_FD-002r8.zip` 一个，无测试构建包 |
| 本能力设备工具 | 当前有效工具仅 `android_test`、`usb_list_devices`；无 `mdk_flash` / `serial_capture` 入口，也未执行烧录或 Verify OK |

结论：**没有可供 P1 使用的测试构建**，因此不存在 `TINJ real=<real> inj=367`、`S t=367 q=1 r=1 s=1`、`P t=367 h=<h>`、第 3 次 `t≈real p=367 r=0` 的任何观察。责任方：**固件实现**（按 `test_design.md` §6.2 提供默认关闭、仅测试启用、正式构建排除的测试构建与观测埋点；板上烧录/串口观测由嵌入式能力执行）。

### 3.4 隔离测试身份与已确认 `mac_addr` ——**不满足**

- `registerphone=14000000001`：模拟测试账号，非真实用户。
- `mac_addr=0200000000A1`：12 位 HEX，但为**明确合成的测试值**，来源未确认，不是用户/网关标签/已确认资料；EMQX 服务端对该 username 返回 `reason: not_authorized`（认证库无匹配用户/凭据；未直连数据库核实）。
- 网关真实标签为 `B1:17:9C:7A:4E:E0`（`GwID`），BLE 地址 `84:C2:E4:03:02:02`；App 的 `mac_addr` 与二者均不同，不能自动互相替代。
- 因此“已确认来源的 `mac_addr`”缺失；该缺口同时阻断 MQTT 业务发布（见 §4.3）。责任方：身份/测试账号提供方（产品/实现或用户确认），测试侧不得伪造或改 prefs。

### 3.5 平台字段级回读授权 ——**缺失**

- 部署的 `/root/abr.py` 只读路由清单：`/sendVerificationCode`、`/verifyCode`、`/register`、`/login`、`/setmac`、`/upload_data`，**全部 POST，无回读/查询路由**。
- `upload_data` 实现为 `INSERT IGNORE INTO sensor_data`；无 GET 回读。
- 本轮仍只有“只读 SSH（不含 DB 查询、无 DB 直连授权）”；人工延期项仅 `CERT8883-UPLOAD-04`（“不用回读，传上去就行了”），**不覆盖 `E2E-JOINT-P1`**；按 Runtime 规则人工跳过只排除列出的具体用例。
- 责任方：平台/运维（提供只读回读入口或授权 DB 只读查询）。

详见 `22_platform_readonly.txt`（`handler_routes_readonly`、`handler_upload_readback_search`、`upload_http_routes`）。

### 3.6 `wifi_ZTE_607157_default_network`

满足：默认网络 101（WIFI，VALIDATED），SSID `ZTE_607157`，wlan0 `192.168.0.103/24`（`13_connectivity.txt:3`、`32_wifi_head.txt`、`33_wlan0.txt`）。

---

## 4. 证书维度（本窗口实测 PASS，acceptance“证书维度”）

### 4.1 主机侧：用项目信任锚校验证书与 IP SAN

```
openssl s_client -connect 8.140.23.253:8883 -servername 8.140.23.253 \
  -CAfile dengbei_care/app/src/main/assets/jd-ca.crt \
  -verify_ip 8.140.23.253 -verify_return_error
→ verify_return_error_rc=0
```

- 叶子 `CN=8.140.23.253`，SAN 含 `IP:8.140.23.253` 与 `IP:117.72.84.210`，链到 App 信任锚 `Tiktok Root CA`；serial/指纹见 §2。
- 证据：`24_host_tls_verify_ip.txt`、`24b_host_tls_showcerts.txt`、`24c_host_leaf_fields.txt`、`24d_host_tls_plain.txt`。

### 4.2 App 侧：无 SAN/链错误，TLS 完成并进入 CONNECT

窗口 12:52:00–13:02:56 全量 logcat 计数（`38_log_final_scan.txt` + 定向计数）：

| 签名 | 次数 | 结论 |
|---|---|---|
| `SSLHandshakeException` | **0** | 无握手异常 |
| `No subjectAltNames` | **0** | 无 SAN 不匹配 |
| `PKIX` / `Trust anchor` / `CertPathValidatorException` / `SSLPeerUnverifiedException` | **0** | 无链/信任锚错误 |
| `Attempting to reconnect to broker` | 2（12:53:54.990、12:58:55.004） | TLS 侧连接已建立并进入 MQTT 流程 |
| `无权连接 (5)` / `MqttException` | 2（12:53:56.005、12:58:56.024） | 收到 CONNACK 后由 Paho 抛错；栈在 `ClientState.notifyReceivedAck → CommsReceiver.run`，不在 TLS 校验层 |

### 4.3 服务端同事件（同一连接到达 EMQX）

`emqx.log.1`（只读）：

```
2026-11-26T12:53:57.824178+08:00 [warning] tag: AUTHN, clientid: android_client,
  msg: authentication_failure, peername: 27.128.89.184:22687,
  username: 0200000000A1, reason: not_authorized
2026-11-26T12:58:57.854433+08:00 [warning] tag: AUTHN, clientid: android_client,
  msg: authentication_failure, peername: 27.128.89.184:22847,
  username: 0200000000A1, reason: not_authorized
```

- 与 App 两次 CONNACK 失败按 `username=0200000000A1 + clientid=android_client + 同一约 5 min 重连周期` 配对；两次服务端时间均比手机日志晚约 **1.82 s**，与本轮实测“手机时钟比主机慢约 3 s”一致（`20_logcat_window.txt` 与主机 UTC 对比），不是两个不同事件。
- 证据：`26_emqx_window_1254.txt`、`31_emqx_window_1259.txt`。

**证书维度结论**：本轮同窗口内 App→EMQX 的 TLS 阻断已解除；`CONNACK rc=5 not authorized` 是认证层结果（合成 `mac_addr` 无匹配认证凭据；未直连数据库核实），不是证书/SAN/链缺陷。该 PASS **仅**证明证书维度，**不**证明 P1 或 MQTT 业务发布通过。

---

## 5. 四段判定（acceptance“嵌入式/手机/MQTT/平台段”）

> 由于没有固件测试构建、没有注入 `367`、没有 `TINJ`，本轮不存在 `uid4+367+h` 事件。以下四段均不能成立；已观测到的真实缓存读取只作附录，不替代 P1 事件。

### 5.1 嵌入式段 —— **NOT EXECUTED**

| P1 要求 | 实际 |
|---|---|
| 测试构建身份且 `Verify OK` | 无测试构建；未烧录；无 Verify OK |
| 第 1 次真实 `S r=0` | 无 COM43 观测入口；无本轮 `S` 原文 |
| 第 2 次 `TINJ real=<real> inj=367`，`S t=367 q=1 r=1 s=1`，`P t=367 h=<h>` | 源码/构建无注入；日志 `TINJ` = 0 |
| 第 3 次 `t≈real p=367 r=0` | 无 |
| `uid4` 可读、帧/CRC/Feistel/发送逻辑未改 | App 侧读到 `6A002C00`；仅能引用标准发布构建“未改”，不能替代测试构建证据 |

责任方：固件实现（测试构建与注入接口）+ 嵌入式能力（烧录与 COM43 观测）。本能力固定任务不承担 MCU/固件板上验证，且当前无 `mdk_flash`/`serial_capture` 入口。

### 5.2 手机段 —— **NOT EXECUTED（P1 事件）**

P1 要求 `T=36.7`、`H=h`、`DATA_OK`、本地行 `(uid4, receiveTime, 36.7, h)`、上传发出；本轮：

- 未发生 36.7 注入；窗口 `T=36.7` = 0、`H=36.7` = 0、`36.7°C` = 0。
- 附录（**不是 P1 事件**）：12:54:36.871 `MTU negotiated: mtu=240 (status=0), expectedDevices=1, requiredMtu=35`；12:54:37.614 `GwID=B1:17:9C:7A:4E:E0, dev=1, valueSize=160, mtu=240`（160 = 10×16 B，16 B 对齐）；12:54:37.615 `id=6A002C00, H=36.4%, T=22.4°C`；12:54:37.645 `link_state=DATA_OK`；12:54:37.635 `TempDb Save reading devId=6A002C00 time=1795668877`；本地库最新行 `(1795668877, 6A002C00, 22.4, 36.4)`；`pending_uploads=0`。
- 12:57:37–12:58:35 另有一次 BLE 连接重试 5 次后 `CONNECT_FAILED`（既有链路间歇）；本轮未取得该周期读数。
- “读取不完整时整帧拒绝”分支未被本次注入刺激覆盖；不能以缓存读取成功推及该分支。

### 5.3 MQTT 段 —— **NOT EXECUTED**

- P1 要求 `/<mac>/a/r0` 载荷 `<epoch>:36.7:<h>`；无注入事件，故无该载荷。
- 当前身份分支（实际记录）：`mac_addr=0200000000A1` 不以 `1010` 开头，`shouldPublishBle()` 为真（`MqtttService.kt:1030-1032`），App 应发布；但连接在 CONNACK 阶段被拒 `rc=5 not_authorized`。App 日志 12:54:37.646 `MQTT offline: keep latest BLE batch (bounded=1)` 表明离线时保留批次而非伪造发布。
- 该失败属认证/身份缺口，**不得用 HTTP 成功替代 MQTT 发布**；也不能据此判证书缺陷。

### 5.4 平台段 —— **NOT EXECUTED**

- P1 要求 `sensor_data` 行 `(phone, dev_id=uid4, time=epoch, temperature=36.7, humidity=h)` 字段一致，且同 `(dev_id,time)` 重试不新增；本轮无 `36.7` 事件，也无字段级回读授权/接口。
- 附录（**不是 P1 事件**）：12:54:40 平台只读日志有 `POST /upload_data HTTP/1.1 200`，对应 App 12:54:37 的缓存值上传；HTTP 2xx 仅证明受理，不证明行存在或字段一致。

---

## 6. 逐条 acceptance 对照

| # | acceptance | 实际结果 | 判定 |
|---|---|---|---|
| 1 | 前置逐项：P0；N1/等价基线；测试构建+引入位置+TINJ+正式排除+恢复 Verify OK；隔离身份+确认 `mac_addr`；平台回读授权 | P0 引用；等价基线已记录；**测试构建缺失**；身份为模拟且 `mac_addr` 未确认；回读无授权 | **不满足 → 整例 NOT EXECUTED** |
| 2 | 嵌入式段：测试构建 Verify OK；第 1/2/3 次 S/TINJ/P；uid4；逻辑未改 | 无测试构建、无 TINJ、无 COM43/烧录 | **NOT EXECUTED** |
| 3 | 手机段：MTU≥35、valueSize≥32 且 16B 对齐、devCount=1、id=uid4、T=36.7、H=h、DATA_OK；本地行；上传；不完整拒绝 | 无 36.7 事件；仅有缓存值 22.4/36.4 的读取/本地行/上传；不完整拒绝分支未刺激 | **NOT EXECUTED**（缓存读取仅附录） |
| 4 | MQTT 段：载荷 `<epoch>:36.7:<h>`；`1010` 分支 N/A；实际原因记录 | 无注入载荷；`shouldPublishBle()` 为真但 `rc=5 not_authorized` 实际原因已记录 | **NOT EXECUTED** |
| 5 | 平台段：HTTP 2xx + `sensor_data` 行字段一致 + 同键不新增；仅 2xx 不通过 | 无注入事件；无回读授权/接口；仅见缓存值 200 | **NOT EXECUTED** |
| 6 | 证书维度：同窗口无 SAN/链错误、TLS 完成进入 CONNECT、CONNACK 实际记录 | 0 个 SAN/链错误；2 次 CONNACK `rc=5`；服务端同连接 `authentication_failure` | **PASS（仅证书维度）** |
| 7 | 关联与披露：`uid4+367+h` 四段一致；缓存复读披露；测试构建排除与恢复证据 | 无 `367` 事件；缓存复读已披露；无测试构建可排除/恢复 | **不适用；P1 不通过** |
| 8 | 任一段缺失/不一致 → 该段 FAIL/NOT EXECUTED；P1 不通过、不得部分通过；缺测试构建/身份/回读 → 整例 NOT EXECUTED 并写责任方 | 四段均缺失；责任方已列 §7 | **符合：TEST_FAIL** |

---

## 7. 失败责任、恢复条件与最小复现

| 缺项 | 责任方 | 恢复条件（完成后再重做 P1 同一用例/同版本） |
|---|---|---|
| 固件测试构建：默认 0、仅测试启用；`measure_sample()` 成功后、`sensor_decide_report()` 前注入 `367`；`TINJ` 观测；正式构建排除；Keil 测试 target/配置 | 固件实现 | 提供测试构建与构建身份；嵌入式能力烧录并取得 `Verify OK`、`BOOT`、`TINJ`、`S/P` 原文；P1 后恢复标准构建并再次 `Verify OK`、确认无 `TINJ` |
| 已确认来源的 `mac_addr` 与可用测试身份（EMQX 认证库可接受） | 身份/测试账号提供方（产品/实现或用户确认） | 提供可追溯的 12 位 HEX `mac_addr` 与对应认证凭据/注册；不得用合成值替代后声称业务发布 |
| 平台 `sensor_data` 字段级只读回读（或等效回读结果）与同键行数幂等 | 平台/运维 | 提供只读回读入口或授权 DB 只读查询；按 `(dev_id,time)` 核对 `(phone,dev_id,time,temperature=36.7,humidity=h)` 与行数 |
| 板上 `S/G/P r=0/s=0` 基线观测 | 嵌入式能力 | 与测试构建同一 COM43 窗口取得第 1 次真实 `S r=0`、第 2 次注入、第 3 次恢复 |

**最小复现输入（当前版本）**：标准构建 + `mac_addr=0200000000A1` + 平台无回读 → 无 `TINJ`/`367`，四段无事件；App 重连仅在 CONNACK 被拒。该结果不是产品功能缺陷的直接证据，而是**该用例前置交付缺失**。

---

## 8. 未执行项与边界（不写成 PASS）

| 项 | 状态 | 原因/缺失条件 | 恢复条件 |
|---|---|---|---|
| `E2E-JOINT-P1` 四段 | **NOT EXECUTED** | 固件测试构建/注入缺失；`mac_addr` 未确认；平台回读无授权 | 见 §7 |
| 嵌入式板上烧录/COM43、`S/G/P/TINJ` | **NOT EXECUTED** | 本能力不承担 MCU 板上验证；无 `mdk_flash`/`serial_capture`；无测试构建 | 由固件/嵌入式能力执行 |
| 平台 `sensor_data` 字段级回读、行数幂等 | **NOT EXECUTED** | 无回读接口/授权；人工延期仅覆盖 `CERT8883-UPLOAD-04` | 平台/运维提供回读入口 |
| MQTT `/<mac>/a/r0` 发布 | **NOT EXECUTED** | 无注入事件；当前合成身份 `rc=5` | 确认身份与认证后重测 |
| 读取不完整整帧拒绝分支 | **NOT EXECUTED** | 未施加不完整帧刺激 | 需注入/构造受控 BLE 不完整帧 |
| 真实物理 >35℃ / 升温>0.9℃ / 光照双态 / 功耗 / 433 电气 | **NOT EXECUTED** | 用户已明确不能调温/调光/测功耗；无仪器 | 人工条件具备后另项 |

---

## 9. 证据索引（`evidence/e2e_joint_p1_20261126/`）

| 文件 | 内容 |
|---|---|
| `00_utc_start.txt` | 起止 UTC/CST、git HEAD、起始状态 |
| `01_model.txt`~`04_serial.txt` | 设备型号/Android/API/serial |
| `05_package_dump.txt` / `06_apk_path.txt` / `08_apk_sha256.txt` | App 版本、APK 路径与 SHA-256 |
| `07_pid.txt` / `36_pid_end.txt` | App PID（前后一致 14224） |
| `09_prefs_list.txt` / `15_prefs_before.xml` / `29_prefs_after.xml` | 身份、守护、上传状态前后 |
| `10_db_list.txt` / `16_local_db_before.db` / `28_local_db_after.db` | 本地库前后快照 |
| `19_local_db_summary.json` / `30_local_db_after_summary.json` | 本地库行数/最新行/36.7 检索 |
| `11_services.txt` / `35_services_end.txt` | App 前台服务 |
| `12_logcat_buffer_start.txt` / `20_logcat_window.txt` | 起始缓冲与 12:52–13:02 实时窗口原文 |
| `23_log_signature_scan.txt` / `27_log_signature_scan_late.txt` / `38_log_final_scan.txt` | TLS/MQTT/BLE/上传签名扫描 |
| `13_connectivity.txt` / `14_ip_route.txt` / `32_wifi_head.txt` / `33_wlan0.txt` | 默认网络 101、`ZTE_607157`、wlan0 |
| `17_ui_dump.xml` / `18_ui_screen.png` | 当前 UI（12:52） |
| `21_fw_testbuild_audit.txt` / `audit_fw_test_build.py` | 固件测试构建/注入/发布排除审计 |
| `24_host_tls_verify_ip.txt` / `24b_host_tls_showcerts.txt` / `24c_host_leaf_fields.txt` / `24d_host_tls_plain.txt` | 主机侧 TLS/SAN/链验证 |
| `22_platform_readonly.txt` / `25_platform_window_1254.txt` / `p1_platform_ssh.py` | 平台只读服务/路由/回读检索/上传日志 |
| `26_emqx_window_1254.txt` / `31_emqx_window_1259.txt` / `emqx_window.py` | EMQX 同窗口 `authentication_failure` 原文 |
| `34_crash_buffer.txt` | crash buffer（仅 11-25 旧崩溃，当前 PID 无新增） |
| `37_n1_equivalent_baseline.md` | N1 等价干净基线记录 |
| `scan_p1_log.py` / `analyze_db.py` | 可复跑只读脚本 |

---

## 10. 脱敏与边界声明

- 未修改产品源码/APK/固件/平台配置；未安装/卸载/清数据/改 prefs；未烧录/复位；未重启/部署服务；未直连数据库；未调用平台写接口（`POST /upload_data` 来自真实 App 产品路径）。
- SSH 仅执行只读命令（systemctl show、ss、journalctl、docker ps、localhost GET、只读路由/函数清单）；未使用 sudo；凭据未回显/未落盘。
- 本轮 PASS 仅指“证书/TLS 维度同窗口无 SAN/链错误且进入 CONNECT”；`E2E-JOINT-P1` 整例 **NOT EXECUTED / 不通过**。不得把 `CERT8883-CLI-02` 或 `E2E-JOINT-P0` 的既有 PASS 当作 P1 PASS。
