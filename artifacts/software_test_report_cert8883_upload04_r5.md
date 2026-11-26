# CERT8883-UPLOAD-04 复验报告（第 5 轮：真实 App 上传受理 + 人工批准延期的字段级回读）

- 负责角色：软件测试
- 能力：软件验证
- 用例：`CERT8883-UPLOAD-04`（TD-CERT8883-001 §3.4，`case_kind=android_upload_persistence`）
- 执行窗口（UTC）：2026-11-26T04:11:51Z ~ 04:16:43Z（CST 12:11:51 ~ 12:16:43）
- 本轮性质：独立复验。**未修改产品源码/APK/平台配置**；未重部署、未重启服务；未登录 Dashboard、未连接数据库；手机侧仅启动态只读采集与真实 App 上传观察，未安装/卸载/清数据/改 prefs。
- 运行依据：`artifacts/software_test_design_cert8883.md` §3.4、`artifacts/interface_contract.md` IC-002 v4.0 §8、`artifacts/software_failure_analysis_cert8883_upload401.md`、`evidence/e2e_p0/devops_platform_upload_contract_20261125/DEPLOYMENT_EVIDENCE.md`。

---

## 1. 人工范围决定与本项处理（先声明）

上一轮人工问题请求“平台 `sensor_data` 字段级只读回读授权”或明确跳过。人工回复：**“无法准备，先跳过 [不用回读，传上去就行了]。”**

| 项 | 稳定 ID | 人工结论 | 本轮处理 |
|---|---|---|---|
| 平台 `sensor_data` 字段级只读回读 / 按行数幂等 | `CERT8883-UPLOAD-04` | “经人工确认，未执行”，`APPROVED_DEFERRED`；“不用回读，传上去就行了” | 字段级回读与按行数幂等 **NOT EXECUTED**，不写成 PASS；按人工明确要求执行**真实 App 上传受理** |

因此本轮把本项拆成两部分判定：**上传受理（执行）** 与 **字段级持久化回读/行数幂等（未执行）**。本报告不以 HTTP 2xx 代替持久化。

---

## 2. 结果摘要

| 判定部分 | 结果 | 说明 |
|---|---|---|
| 真实 App 上传受理（`CloudUpload`/`POST /upload_data`，载荷含 devId/time/temp/humi，未 gated off） | **PASS（本轮实测）** | 12:10:45 真实 BLE 读数入队并上传；平台 12:10:48 `POST /upload_data 200`；待发箱清零、`upload_last_status=OK` |
| 同 `(dev_id,time)` 重试不新增（服务端响应维度，补充） | 已观测 `inserted=0` | 服务端实际响应 `{"inserted":0,"message":"ok"}`；**非**行数回读 |
| 平台 `sensor_data` 字段级只读回读、`temperature`/`humidity` 逐项一致 | **NOT EXECUTED（APPROVED_DEFERRED）** | 无回读授权；不写成 PASS |
| 按 `(dev_id,time)` 回读行数证明“重试不增行” | **NOT EXECUTED（随回读延期）** | 同缺回读入口 |

**一句话**：在修复后的当前平台版本上，真实 App 用真实 BLE 读数走产品上传路径，平台 `POST /upload_data` 返回 HTTP 200，待发箱清零；同一 `(dev_id,time)` 重放服务端返回 `inserted=0`。字段级 MySQL 回读按人工决定未执行，本轮不声称持久化字段值已核对。

**本项判定表述（按人工确认范围）**：`CERT8883-UPLOAD-04` 的**上传受理维度 = PASS（本轮实测）**；**字段级持久化回读 / 按行数幂等 = NOT EXECUTED（APPROVED_DEFERRED，不写成 PASS）**。两者分别记录，不以 200 代替持久化。

---

## 3. 被测版本身份（当前，未更换）

| 端 | 实测身份 | 证据 |
|---|---|---|
| 平台服务 | `user_script.service` ActiveState=active、SubState=running、PID 312368、启动 2026-11-25 21:59:02 CST、`NRestarts=0` | `13_platform_window_upload.txt`、`18_platform_window_replay.txt` |
| 部署对象 | `/root/abr.py` 8,688 B，sha256 `a062b05f524dc8f6bce596e4571d339f697ef909b318503ad418ee0add24017c`（= 已批准候选，`X-Upload-Token` 门禁已移除） | 同上 |
| EMQX/端口 | 8883 `beam.smp` pid 299216；5000/8883/1883/18083/3306 均 LISTEN | 同上 |
| App | `com.jinyuni.dengbei_care` vC10 / vN1.9；设备侧 `base.apk` SHA-256 `80232c50f5b132da8747d389fb11402e5110faaf0f740107da202ef8e7d3de4b`（= 主机 `app-debug.apk`）；PID 14224，11:28 启动 | `00_utc_start.txt`、`01_dumpsys_package.txt`、`04_device_apk_sha256.txt`、`25_pid_end.txt` |
| 手机 | Redmi 8A / Android 10 / API 29 / serial `12f046690506` | `01_dumpsys_package.txt` |
| 网络 | 默认网络 101=WIFI；SSID `ZTE_607157`；wlan0 `192.168.0.103/24` | `10_connectivity.txt`、`11_wifi_dump.txt`、`19_wlan0.txt` |

---

## 4. 前置逐项记录

| 前置 | 实测 | 判定 |
|---|---|---|
| `registerphone` 非空，经 App 既有登录路径设置 | `14000000001`（模拟测试账号，非真实用户；此前经 App 登录路径设置，非直接改 prefs） | 满足（身份为模拟测试号） |
| 已确认来源的平台网关 `mac_addr`（12 位 HEX），经 App 保存路径 | `0200000000A1`（12 位 HEX，经 App 保存路径设置，但为**明确合成的测试值**，非用户/网关标签/已确认资料） | **来源未确认（披露）** |
| 至少一条合法读数 | 真实 BLE `DATA_OK`：`id=6A002C00, H=37.6%, T=21.1°C`，`TempDb time=1795666245` | 满足 |
| 平台写授权（本次上传） | 真实 App 自动上传；部署能力明确交软件测试执行真实上传 | 满足（上传已执行） |
| 平台只读回读授权 | 无；人工批准延期“不用回读” | **未执行（APPROVED_DEFERRED）** |
| 守护运行 | `measure_state=stopping`；`finish_guard_time=2026-11-26 13:46:12`；UI“停止”按钮 | 满足 |
| `ZTE_607157` 默认网络 | 默认网络 101=WIFI；SSID `ZTE_607157` | 满足 |

> `mac_addr` 缺口说明：平台上传处理器不读取 `mac`（见失败分析 §4），故其不阻断上传受理；但它不满足“已确认来源的平台网关 MAC”这一前置，如实披露，不由测试人员擅自改为真实网关值。

---

## 5. 真实 App 上传受理（实测 PASS）

| 时间（CST） | 观测 | 证据 |
|---|---|---|
| 12:10:44–45 | 真实 BLE 连接读取 `84:C2:E4:03:02:02`，`GwID=B1:17:9C:7A:4E:E0, dev=1, valueSize=160, mtu=240` | `07_logcat_buffer_start.txt` |
| 12:10:45.604 | `MqttService: id=6A002C00, H=37.6%, T=21.1°C` | 同上 |
| 12:10:45.626 | `TempDb: Save reading devId=6A002C00 time=1795666245` | 同上 |
| 12:10:45.640 | `link_state=DATA_OK`；`BLE batch: valid=1 placeholder=0` | 同上 |
| 12:10:46.015 | `CloudUpload: uploaded 1 reading(s)`（门控开；无 `upload gated off`） | 同上 |
| 12:10:48 | 平台 `27.128.89.184 - - "POST /upload_data HTTP/1.1" 200 -` | `13_platform_window_upload.txt` |
| 12:14 | prefs：`upload_last_status=OK`、`upload_blocked_reason` 空、`upload_pending_count=0` | `15_prefs_after.xml` |
| 12:13 | 本地库：`temperature` 135 行；`pending_uploads` 0 行；含 `(1795666245,'6A002C00',21.1,37.6)` | `16_local_db_after.db`、`analyze_db.py` |
| 12:14 | UI：e2ep0 `21.1°C / 38%`，“最近读取 刚刚”，守护运行 | `21_ui_home.png`、`22_ui_dump.xml` |

**载荷字段关联**：上传体由 `UploadPayload.build(phone, mac, readings=[SensorReading(devId,time,temperature,humidity)])` 组装（`cloud/UploadPayload.kt`、`ui/zhuce/ApiService.kt`）。本轮对应载荷为
`{"phone":"14000000001","mac":"0200000000A1","readings":[{"devId":"6A002C00","time":1795666245,"temperature":21.1,"humidity":37.6}]}`。
App 不打印 JSON 原文；`devId/time/temperature/humidity` 与 BLE 解码值、`TempDb` 行、本地库最新行逐项一致，来源为同一真实数据流，非事后编造。

**门控/异常检索**：全量 logcat 缓冲 `CloudUpload` 仅 `uploaded N reading(s)`；无 `upload gated off`、无 `upload failed for`、无 `upload rejected for`；本轮无 401/403/其他 4xx。

---

## 6. 幂等（补充，服务端响应维度）

12:13:54Z 以与 12:10 真实上传完全相同的 `(phone=14000000001, mac=0200000000A1, devId=6A002C00, time=1795666245, 21.1, 37.6)` 重放一次：

```
http_status=200 elapsed_ms=9226 body={"inserted":0,"message":"ok"}
```

平台访问日志 12:14:03 再次 `POST /upload_data 200 -`（`18_platform_window_replay.txt`）。观测到的响应为 `inserted=0`（不预设字段），与“同键未新增”一致；但这是**服务端响应**，不是按回读行数核对。行数幂等属回读范围，**未执行**。

---

## 7. 未执行项（显式列出，不写成 PASS）

| 子项 | 稳定 ID | 状态 | 缺失条件 / 责任方 / 恢复后必须重做 |
|---|---|---|---|
| 平台 `sensor_data` 按 `(phone, dev_id, time)` 字段级只读回读，`temperature`/`humidity` 逐项一致 | CERT8883-UPLOAD-04 | **NOT EXECUTED（APPROVED_DEFERRED / 经人工确认，未执行）** | 缺：字段级只读回读授权（只读 SSH 不含 DB 查询，平台无回读接口）。责任方：平台/运维。恢复后：以 `(phone=14000000001, dev_id=6A002C00, time=1795666245)` 回读该行，核对 `temperature≈21.1`、`humidity≈37.6`（记录实际表示），并统计该键行数确认未增行。人工原因：“不用回读，传上去就行了”。 |
| 同 `(dev_id,time)` 重试不新增（按回读行数） | CERT8883-UPLOAD-04 | **NOT EXECUTED（随回读延期）** | 同上；现有 `inserted=0` 仅为服务端响应，不替代行数回读。 |

---

## 8. 未伪造/未写本地状态与测试数据披露

- 未写入 SharedPreferences/本地数据库/待发箱（只读采集）。App 自身写入属正常业务，测试人员未构造。
- 未伪造身份：`registerphone=14000000001` 为模拟测试账号，`mac_addr=0200000000A1` 为明确合成测试值（来源未确认，已披露）；未使用真实用户/真实网关凭据。
- 未伪造成功：上传成功以真实 App 产品路径与平台访问日志 200 为准。
- 测试行**保留未清理**（无删除授权）。可识别信息：`dev_id=6A002C00`（uid）、`time=1795666245`、`phone=14000000001`、`mac=0200000000A1`、窗口 2026-11-26 12:10 CST。平台保留该模拟账号与该测试设备读数。
- 未发送短信；未直连数据库；未登录 Dashboard；未改服务端配置。

---

## 9. 边界与未执行

1. 字段级持久化回读/行数幂等：NOT EXECUTED（人工批准延期，见 §1/§7）。
2. `mac_addr` 来源未确认（合成测试值）。
3. 真实物理 >35℃/升温+无光/光强/功耗/433 电气：无条件，未执行。
4. release/生产签名、iOS、API 31+：不在本轮。
5. E2E-JOINT-P1、FINAL-05：不在本项范围；本报告结论仅覆盖 `CERT8883-UPLOAD-04` 的上传受理维度与当前版本。
6. 既有跨组件观察（非本轮引入）：MQTT `CONNACK rc=5`（认证层缺口）；BLE 约存在周期失败；App 11-25 22:00:24 导航崩溃（当前 PID 14224 无新增崩溃/ANR，`23_crash_buffer.txt` 仅旧崩溃）。

---

## 10. 结论

1. **真实 App 上传受理：本轮实测 PASS**。真实 BLE 读数 `(6A002C00, 1795666245, 21.1, 37.6)` 经产品既有 `enqueueAndUpload` 路径上传，平台 `POST /upload_data` HTTP 200，待发箱清零、`upload_last_status=OK`；无 `upload gated off`、无 4xx、无伪造。证书修复后的平台上传受理在当前版本有效。
2. **字段级持久化回读 / 按行数幂等：NOT EXECUTED（人工批准延期）**。本轮不以 200 代替持久化，不写成 PASS。
3. 本报告只覆盖 `CERT8883-UPLOAD-04` 的已执行上传受理范围与当前版本；不代表 MySQL 字段值核对通过，也不代表 P1/FINAL-05 或完整产品验收通过。

---

## 11. 证据索引（`evidence/software_verify_cert8883_upload04_r5_20261126/`）

| 文件 | 内容 |
|---|---|
| `00_utc_start.txt` | 起始 UTC 与 git HEAD |
| `01_dumpsys_package.txt` | App vC10/vN1.9、`lastUpdateTime`、DEBUGGABLE、权限 |
| `02_pid.txt` / `25_pid_end.txt` | App PID（14224，前后一致） |
| `03_apk_path.txt` / `04_device_apk_sha256.txt` | 设备侧 base.apk SHA-256（=`80232c50…de4b`） |
| `05_prefs_list.txt` / `06_prefs_before.xml` / `15_prefs_after.xml` | 身份（registerphone/mac_addr）、守护、上传状态前后 |
| `07_logcat_buffer_start.txt` / `17_logcat_buffer_end.txt` | 全量 logcat（含 12:10:45 真实 BLE 与上传、无 gated off） |
| `08_db_list.txt` / `09_local_db_before.db` / `16_local_db_after.db` | 本地库快照（前后） |
| `10_connectivity.txt` / `11_wifi_dump.txt` / `19_wlan0.txt` | 默认网络 101、SSID `ZTE_607157`、wlan0 地址 |
| `12_services.txt` | App 服务状态 |
| `13_platform_window_upload.txt` | 平台只读窗口：12:02/12:10 `POST /upload_data 200` |
| `14_idempotency_replay.txt` / `idempotency_replay.py` | 同键重放 `200 {"inserted":0}`（补充） |
| `18_platform_window_replay.txt` | 平台只读窗口：12:14 重放 `200` |
| `21_ui_home.png` / `22_ui_dump.xml` | 首页 UI（21.1℃/38%、最近读取 刚刚、守护运行） |
| `23_crash_buffer.txt` | crash buffer（仅 11-25 22:00 旧崩溃，当前 PID 无新增） |
| `analyze_db.py` | 本地库解析脚本（temperature/pending 行数与最新行） |
| `30_upload04_r5_judgement.md` | 本项逐项判定 |

清单说明：仓库 `scripts/` 目录当前仍无 `check_file_manifest.py`，无法运行该机械检查；本轮已用只读路径存在性自检确认新增文件全部存在，并同步 `file_manifest.txt`。
