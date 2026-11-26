# CERT8883-UPLOAD-04 第 5 轮（UPLOAD-04 单项复验）逐项判定

- 角色/能力：软件测试 / 软件验证
- 用例：`CERT8883-UPLOAD-04`（TD-CERT8883-001 §3.4，`case_kind=android_upload_persistence`）
- 执行窗口（UTC）：2026-11-26T04:11:51Z ~ 04:16:43Z（CST 12:11:51 ~ 12:16:43）
- 被测对象（当前版本，未更换）：
  - 平台：`8.140.23.253`（`iZ2zeih0gmrk47og5r4z7sZ`），Flask `user_script.service` PID 312368（启动 2026-11-25 21:59:02 CST，`NRestarts=0`），部署对象 `/root/abr.py` sha256 `a062b05f524dc8f6bce596e4571d339f697ef909b318503ad418ee0add24017c`（= 已批准候选，`X-Upload-Token` 门禁已移除）
  - 客户端：`com.jinyuni.dengbei_care` vC10 / vN1.9，设备侧 `base.apk` SHA-256 `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b`（= 主机 `app-debug.apk`）
  - 手机：Redmi 8A / Android 10 / API 29 / serial `12f046690506`；默认网络 101=WIFI，SSID `ZTE_607157`，wlan0 `192.168.0.103/24`
- 人工范围控制：`deferred_test_items` 记录 `CERT8883-UPLOAD-04`，missing_condition=平台 `sensor_data` 字段级只读回读授权，人工结论“经人工确认，未执行”，原因“不用回读，传上去就行了”，`delivery_status=APPROVED_DEFERRED`。
  - 本轮据此：**执行真实 App 上传受理**（人工明确要求“传上去”），**字段级回读与按回读行数的幂等未执行**（不写成 PASS）。

---

## 0. 测量边界（先声明）

- 手机侧：`adb` 只读采集（身份/prefs/本地库/logcat/UI），App 由既有会话（PID 14224，11:28 启动）运行；本轮**未安装/卸载/清数据/改 prefs**、未伪造身份或成功。
- 平台侧：只读 SSH（`systemctl show`、`stat/sha256sum`、`journalctl`、`ss`）观察服务与 `/upload_data` 访问日志；**未连数据库、未写文件、未重启服务**。
- 写路径：真实 App 自动上传（产品既有 `POST /upload_data`）；另有 1 次**同键幂等重放**（与已上传读数完全相同的 `phone/mac/reading`），属 TD §3.4 步骤 4 允许的同键重试，用于观察服务端响应；不是以重试冒充成功。
- 回读：字段级 `sensor_data` 回读无授权（仅人工批准延期），**未执行**；本轮不以 HTTP 2xx 代替持久化。

---

## 1. 前置逐项记录

| 前置 | 实测 | 证据 | 判定 |
|---|---|---|---|
| 隔离测试身份 `registerphone` 非空，经 App 既有登录路径 | `14000000001`（模拟测试账号；此前的 App“我的→登录”路径设置，非直接改 prefs） | `06_prefs_before.xml`、`15_prefs_after.xml` | 满足（身份为模拟测试号，非真实用户） |
| 已确认来源的平台网关 `mac_addr`（12 位 HEX）经 App 保存路径 | `0200000000A1`（12 位 HEX，经 App 既有保存路径设置；但为**明确合成的测试值**，非来自用户/网关标签/已确认资料） | 同上 | **来源未确认**（披露为缺口；平台上传处理器不读取 `mac`，见失败分析 §4） |
| 至少一条合法读数 | 真实 BLE `DATA_OK`：12:10:45.603 `GwID=B1:17:9C:7A:4E:E0, dev=1`；`id=6A002C00, H=37.6%, T=21.1°C`；12:10:45.626 `TempDb Save reading devId=6A002C00 time=1795666245` | `07_logcat_buffer_start.txt` 第 79470 行附近 | 满足（真实 BLE，不是占位/缓存） |
| 平台写授权（本次 `POST /upload_data`） | 真实 App 上传 + 部署能力交付明确交软件测试执行真实上传 | 见 §2 | 满足（上传受理已执行） |
| 平台按 `(dev_id,time)` 只读回读授权 | 无；人工批准延期“不用回读” | `deferred` 记录 | **未执行（APPROVED_DEFERRED）** |
| 守护运行 | `measure_state=stopping`（守护运行中）；`finish_guard_time=2026-11-26 13:46:12`；UI“停止”按钮 | `06_prefs_before.xml`、`15_prefs_after.xml`、`21_ui_home.png` | 满足 |
| `ZTE_607157` 为默认网络 | 默认网络 101=WIFI；SSID `ZTE_607157`；wlan0 `192.168.0.103/24` | `10_connectivity.txt`、`11_wifi_dump.txt`、`19_wlan0.txt` | 满足 |

---

## 2. 真实 App 上传（受理维度，实测）

| 时间（CST） | 观测 | 原始证据 |
|---|---|---|
| 12:10:44–45 | 真实 BLE 连接并读取 `84:C2:E4:03:02:02`，`cipher length=160`，`GwID=B1:17:9C:7A:4E:E0, dev=1, valueSize=160, mtu=240` | `07_logcat_buffer_start.txt` |
| 12:10:45.604 | `MqttService: id=6A002C00, H=37.6%, T=21.1°C` | 同上 |
| 12:10:45.626 | `TempDb: Save reading devId=6A002C00 time=1795666245` | 同上 |
| 12:10:45.640 | `link_state=DATA_OK msg=网关已连接（最近读取 12:10）`；`BLE batch: valid=1 placeholder=0` | 同上 |
| 12:10:46.015 | `CloudUpload: uploaded 1 reading(s)`（门控开，未出现 `upload gated off`） | 同上 |
| 12:10:48 | 平台访问日志：`27.128.89.184 - - "POST /upload_data HTTP/1.1" 200 -` | `13_platform_window_upload.txt` |
| 12:14 | App prefs：`upload_last_status=OK`、`upload_blocked_reason` 空、`upload_pending_count=0` | `15_prefs_after.xml` |
| 12:13 | 本地库：`temperature` 135 行；`pending_uploads` 0 行；含 `(1795666245,'6A002C00',21.1,37.6)` | `16_local_db_after.db`、`analyze_db.py` |
| 12:14 | UI：e2ep0 `21.1°C / 38%`，“最近读取 刚刚”，守护运行 | `21_ui_home.png`、`22_ui_dump.xml` |

**载荷字段关联**：上传请求体由 `UploadPayload.build(phone, mac, readings=[SensorReading(devId,time,temperature,humidity)])` 组装（`cloud/UploadPayload.kt`、`ui/zhuce/ApiService.kt`）。本轮对应载荷为
`{"phone":"14000000001","mac":"0200000000A1","readings":[{"devId":"6A002C00","time":1795666245,"temperature":21.1,"humidity":37.6}]}`，
与 BLE 解码值、`TempDb` 行、本地库最新行逐项一致。App 未打印 JSON 原文，字段来自同一 `(devId,time,temp,humi)` 数据流，非事后编造。

**门控/异常检索**：全量 logcat 缓冲中 `CloudUpload` 仅出现 `uploaded N reading(s)`，无 `upload gated off`、无 `upload failed for`、无 `upload rejected for`；本轮无 401/403/其他 4xx。

---

## 3. 幂等（同 `(dev_id,time)` 重试）— 补充证据，非回读行数

- 12:10:45 的真实读数已由 App 上传。12:13:54Z（CST 12:13:54）以完全相同的 `(phone=14000000001, mac=0200000000A1, devId=6A002C00, time=1795666245, 21.1, 37.6)` 重放一次：
  - `http_status=200 elapsed_ms=9226 body={"inserted":0,"message":"ok"}`（`14_idempotency_replay.txt`）
  - 平台访问日志 12:14:03 再次 `POST /upload_data 200 -`（`18_platform_window_replay.txt`）
- **判读**：服务端返回 `inserted=0`（观测到的实际响应，不预设字段），与“同键未新增”一致；但这是**服务端响应维度**，**不是**按回读行数核对。按回读行数的幂等属回读范围，**未执行**（APPROVED_DEFERRED）。

---

## 4. 未执行项（显式列出，不写成 PASS）

| 子项 | 状态 | 缺失条件/责任方/恢复后步骤 |
|---|---|---|
| 平台 `bwusernamebase.sensor_data` 按 `(phone, dev_id, time)` 字段级只读回读、`temperature`/`humidity` 逐项一致 | **NOT EXECUTED（APPROVED_DEFERRED / 经人工确认，未执行）** | 缺：平台 `sensor_data` 字段级只读回读授权（只读 SSH 入口不含数据库查询，平台无回读接口）。责任方：平台/运维（提供只读查询入口或等效回读结果）。恢复后：以 `(phone=14000000001, dev_id=6A002C00, time=1795666245)` 回读该行，核对 `temperature≈21.1`、`humidity≈37.6`（记录实际表示），并统计该键行数确认未增行。人工原因：“不用回读，传上去就行了”。 |
| 按回读行数验证同键重试不增行 | **NOT EXECUTED（随回读延期）** | 同上；当前仅有服务端 `inserted=0` 响应，不替代行数回读。 |

---

## 5. 未伪造 / 未写本地状态 / 数据披露

- 未写入 SharedPreferences/本地数据库/待发箱；只读采集。App 自身写入属正常业务，不由测试人员构造。
- 未伪造身份：`registerphone=14000000001` 为模拟测试账号（此前经 App 登录路径设置），非真实用户；`mac_addr=0200000000A1` 为明确合成测试值，非真实网关，**来源未经确认**（已披露）。
- 未伪造成功：上传成功以真实 App 产品路径 + 平台访问日志 200 为准，非手工构造。
- 测试行保留：无删除授权，**未清理**。可识别信息：`dev_id=6A002C00`（uid）、`time=1795666245`、`phone=14000000001`、`mac=0200000000A1`、窗口 2026-11-26 12:10 CST。平台保留该模拟账号与该测试设备读数。

---

## 6. 结论（本项）

- **真实 App 上传受理：实测通过**（真实 BLE 读数 → `CloudQueue/CloudUpload` 上传 → 平台 `POST /upload_data` HTTP 200 → 待发箱清零、`upload_last_status=OK`；无门控关闭、无 4xx、无伪造）。
- **字段级持久化回读 / 按行数幂等：NOT EXECUTED（人工批准延期）**；本轮不以 200 代替持久化，不写成 PASS。
- 本结论只覆盖 `CERT8883-UPLOAD-04` 的已执行上传受理范围与当前版本，不代表 MySQL 字段值核对通过，也不代表 P1/FINAL-05 或完整产品验收通过。
