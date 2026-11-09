# Android App 方案设计（AA-004）

状态：Android 方案设计（供实现与测试消费；不代表任何设备、网关、云端或真机行为已验证）
版本：3.0（替代 AA-003 rev 2.0；rev 2.0 的四项增量保留，本版依据当前上游证据新增三项，并更正已过期结论）
责任角色：Android 工程
范围：仅 Android App（`dengbei_care`，Kotlin）。不含固件、网关、服务器端代码、iOS 与硬件改动。
工程基线：Git HEAD `9116957fd7d95886db492a62651615c2d7ec21d0`，接手时 `git status --porcelain` 为空；源码 `versionCode 9 / versionName 1.8`，`minSdk 26`、`targetSdk 34`、`compileSdk 34`。
检查时点：2026-11-09（Asia/Shanghai）。本版只做源码/资料静态核对，未构建、未运行测试、未安装、未连接手机/网关/串口/服务器。

输入来源（本版实际读取）：
- `readme.txt`（需求方，权威，八项修改点 + 四项补充测试安排）
- `artifacts/system_analysis_current.md`（系统架构当前分析：缓存复读风险、Android 四项增量未完成、服务端阶段分工）
- `artifacts/backend_feasibility.md`（后端可行性探索 BE-EXP-002：既有服务在线/端口/路由、`/upload_data` 历史 401、资料与部署不一致、端到端 D1–D6）
- `artifacts/firmware_design.md` / `artifacts/firmware_requirements.md`（FD-002 rev 7 / FWR-002 rev 7：升温 >0.9℃ 且无光 / 严格 >35.0℃、180 s、空口/BLE 布局不变、未标定边界）
- `artifacts/interface_contract.md`（IC-002 v3.0：判据、BLE 布局、MQTT 载荷、`/upload_data`、D-03）
- `artifacts/android_feasibility.md`（AF-001：Redmi 8A / API 29 只读事实、F-1 权限门、F-2 网关身份）
- `artifacts/android_implementation.md`（AIMPL-002：历史实现与修复记录，仅作历史）
- `artifacts/codegraph_current_index.md`（三个子工程索引入口；本次按精确 projectPath 的图查询由上游提供，本能力未建索引）
- 当前 Android 源码、网关源码（`CH592EVT/EVT/EXAM/BLE/beiwov2`）、`服务器迁移记录.md`
- `file_manifest.txt`、最近提交与工作区状态

---

## 0. 本版更新（优先于以下历史描述）

1. **AA-003 rev 2.0 的四项增量仍然成立且仍未实现**（本次源码逐条复核）：G-1 权限门、G-2 网关身份硬编码/无发现、G-3 批次只保留最后一笔、G-4 规则文案方向。四项均未在 `dengbei_care` 中找到对应新增模块或接线。
2. **新增 G-5：网关“无数据占位”被当成读数。** 网关在“收到帧但解密/CRC 失败”累计到 `timeoutcnt` 耗尽后把 `sensorres` 整体清零并下发全零设备块（`APP/app_um2006A.c:217–226`）；当前 App 的 `isZeroValue` 连续 3 次门槛（`MqtttService.kt:743–760`）最终会把 `devId=00000000 / T=0.0 / H=0.0` 写入本地库、待发箱并显示为设备，属于确定的数据错误。
3. **新增 G-6：云上传失败不分类，`401/403` 与网络瞬时失败同路。** 既有服务部署日志在 2026-10-22 出现两次 `POST /upload_data` → 401（`backend_feasibility.md` F1），当前 App 不带任何凭证，且 `RetrofitUploadApi` 把“非 2xx”统一当失败退避（`CloudUploadRepository.kt:124–142`、`:200–206`），最坏结果是全部行在约 8 h 后滞留且用户无感。401 的具体机制未定性，本设计不猜测、不新增鉴权协议。
4. **更正 G-3 的第二个缺陷：BLE 转发载荷格式与契约不一致。** 当前构造为 `time + ',' + "humi...,temp..."`（`MqtttService.kt:1288–1291`），而 IC-002 §6 与 App 自身解析器要求 `time:temps:humis`（冒号分隔、温度在前）。该载荷即使被本 App 的 `ReadingAttribution` 收到也会整批拒绝。
5. **上游已解决、本版删除的旧结论**：AA-003 U-6（“固件判据仍为降温/整体排除 35℃”）已被 FD-002 rev 7 与 IC-002 v3.0 取代——当前固件实现升温方向与严格 `>35.0℃`，Android 侧不因此改动门控，也不再把该条列为未决缺陷。
6. **未改变的产品判断**：缓存复读仍按 IC-002 的“接收时间”语义记录为观测（不是物理采样时刻）；本版**不**引入按值去重来改变本地/云端行语义（理由见 D-24 与 §11 U-10），避免破坏 App 自身下降报警的时间窗基准。
7. **DESIGN_READY 仅表示方案可供实现**：真机权限/发现/GATT、真实 `/upload_data` 与 401 定性、持久化回读均未执行，见 §10.3 与 §11。

---

## 1. 本次设计结论

### 1.1 已完成并可作为 as-built 复用（宿主机层已验，设备层未验）

| 能力 | 实现位置 | 证据边界 |
|---|---|---|
| BLE 聚合帧唯一解码（`id4 | hum u16be@4 | temp s16be@6`） | `protocol/GatewayFrameCodec.kt` | 宿主机单测 + 历史构建；真实 GATT 未验 |
| MQTT 批量载荷严格归因（数目不一致拒绝整批） | `telemetry/ReadingAttribution.kt` | 同上；不解决“数目相同但顺序错” |
| 告警时间窗判定（5/10/15 min，基准来自库，30 min 回溯容差） | `telemetry/AlarmEvaluator.kt` + `MqtttService` 接线 | 同上 |
| SQLite v3（`temperature`、`alarm_events`、`pending_uploads`） | `TemperatureDatabaseHelper.kt` | schema/语句级；真机迁移未验 |
| 单条读数写入（真实接收/载荷时间） | `storeTemperatureReading(...)` | 语句级；真机行级未验 |
| 云上传待发箱与编排（幂等、退避、门控、串行） | `cloud/UploadOutbox.kt`、`cloud/CloudUploadRepository.kt`、`cloud/UploadPayload.kt` | 宿主伪实现；真机 SQLite 与 HTTP 未验 |
| 服务器常量单一来源（`8.140.23.253`，8883/5000） | `cloud/CloudConfig.kt` | 静态；TLS/上传可达性未验 |
| 卡片不以数据龄判“离线” | `DeviceState.kt`、`DeviceCardAdapter.kt` | 逻辑层；界面未验 |
| 详情 24 h 范围、高温强调、规则说明 | `ui/detail/DetailPresentation.kt`、`DeviceDetailFragment.kt` | 逻辑层；界面未验 |
| 早报“按条件上报/非等间隔/昨日无上报” | `report/ReportGenerator.kt` | 文本层；通知展示未验 |
| 3 分钟读取节拍（180 s） | `telemetry/SensorCadence.kt` | 常量断言；真机长跑未验 |
| 守护门控（`measure_state == "stopping"` + `registerphone`） | `PrefsUploadGate` | 逻辑层；真机未验 |

### 1.2 本轮必须补齐的六项（当前源码确认仍存在）

| 编号 | 缺口 | 当前证据（文件:行/行为，行号为本次检查时点） |
|---|---|---|
| G-1 | BLE 运行时权限模型与请求流程不一致：Android 10（API 29）上直连 GATT 被应用自身要求 `ACCESS_FINE_LOCATION`，而正常启动流程从不请求该权限 → 每 180 s 轮询静默返回 | `MqtttService.kt:552–557`、`:566–569`、`:583–586`；`MainActivity.kt:62–77`、`:149–158` 请求段被注释；`requestBluetoothPermissions()`（`:219–226`）只在定位授权回调分支可达（`:267–284`） |
| G-2 | 网关身份硬编码且无发现/绑定流程，与环境登记“由 App 发现并连接、实际名称与地址未确认”冲突；`mac_addr`（topic/上传）与 BLE 地址互不关联 | `MqtttService.kt:261`（`84:C2:E4:03:02:02`）；全仓无 BLE 扫描实现；`mac_addr` 由 `NotificationsFragment` 扫码/手输（`:81–88`、`:218–233`） |
| G-3 | BLE 读取批次只保留最后一笔用于 MQTT 转发（多设备丢数据/错配），且转发载荷分隔符/字段顺序与契约不一致 | `MqtttService.kt:748–752`（每笔覆盖）、`:1281–1292`（`time,humi...,temp...`，只在 MQTT 连接成功时发布） |
| G-4 | 用户可见上报规则写成“温度较上次**下降**超过 0.9℃”，与 readme/IC-002 冻结的**升温**方向相反 | `ui/detail/DetailPresentation.kt:26–30`；`dengbei_care/用户使用说明书.md`、`dengbei_care/readme.txt` |
| G-5 | 网关全零“无数据占位”被当作读数：连续 3 次后以 `devId=00000000 / 0.0 / 0.0` 入库、入待发箱、生成设备卡片 | `MqtttService.kt:743–760`；网关 `APP/app_um2006A.c:217–226`（`memset(sensorres,0,...)` 后下发） |
| G-6 | 云上传不区分 401/403/4xx/5xx/IO：一律按失败退避，最多 20 次后静默滞留；用户无“被拒绝/需处理”状态 | `cloud/CloudUploadRepository.kt:124–142`；`RetrofitUploadApi`（`:200–206`）只返回 `isSuccessful`；`ApiService.uploadData` 不带任何凭证；BE-EXP-002 F1 观测到 401 |

### 1.3 为什么这六项不能只靠测试或环境解决

- G-1/G-2 是应用自身代码路径的确定性缺陷（不依赖外部条件即可从源码证明）。
- G-3/G-5 是数据正确性缺陷：多设备静默错配/丢失、无数据占位被写成真实读数。
- G-6 是错误可观测性与数据保留缺陷：上传被拒时既不能正确区分，也不能让用户看到并处置；但**鉴权机制本身**属未决项，必须由端到端阶段定性，App 不猜测协议。
- G-4 是用户可见规则与产品需求相反。
- 六项均可通过纯逻辑 + 接线改造消除，并可在宿主机独立断言；真实弹窗、扫描、GATT、401 与落库仍需测试能力在真机/获准服务入口独立验收（§10.3）。

---

## 2. 需求到 Android 的落点

| readme 修改点 | Android 侧落点 | 当前状态 |
|---|---|---|
| 1 GXHT40 替换温湿度器件 | 单位（0.1℃/0.1 %RH）与 BLE 布局不变，解析逻辑不改；只做回归约束 | 已实现（`GatewayFrameCodec`） |
| 2 新增光敏电阻与分压 | 光照不上空口、不上 BLE；App 不推断、不显示光照/照度 | 已实现（无光照字段与展示） |
| 3 采集时间改为 3 分钟 | App 读取周期对齐 180 s；不得把“无上报”当异常 | 已实现（`SensorCadence`、卡片/详情/早报文案） |
| 4 条件上报：本次较上次**升高 >0.9℃ 且无光**，或温度**严格 >35.0℃** | App 不重算门控、不推断光照、不合成样本；规则文案必须与方向、严格边界一致 | 门控实现正确；**文案方向错误（G-4）** |
| 5 平台 broker/服务器信息 | MQTT `ssl://8.140.23.253:8883`、HTTP `http://8.140.23.253:5000`、明文放行、`POST /upload_data` | 已实现；上传可用性未证且存在 401 反向证据（G-6） |
| 6 同步 Android、不改 iOS、退役光通信/霍尔 | 仅 Android；线上格式不变；退役模块不引入 App 逻辑 | 已实现（约束登记） |
| 环境：网关由 App 发现并连接 | 必须有发现/绑定/持久化身份，不得硬编码地址 | **未实现（G-2）** |
| 环境：真机为 Android 10/API 29 | 权限模型按 API 级别分流，连接路径不得依赖未请求的权限 | **未实现（G-1）** |
| 补充：服务器复用、端到端真实上传/幂等/回读 | App 侧按既有契约编码；上传失败必须可区分、可观测、不丢数据；真实写入与回读在端到端阶段执行 | **部分缺失（G-6）** |

---

## 3. As-built 事实（本设计必须尊重的现状）

### 3.1 数据链路

```text
BLE：GATT FFE0/FFE2 读 → AES/ECB 解密 → GatewayFrameCodec（按 4 B ID 归因）
MQTT：/<mac>/a/r0  → ReadingAttribution（按 MacIdBook 顺序，数目不一致拒绝整批）
        ↓（两条入口汇聚）
processTemperatureHumidityData(devId, T, H[, 时间])
  → AlarmEvaluator（时间窗下降 + 阈值 + >65℃ 紧急）
  → alarm_events 落库 / 通知（900 s 去抖，紧急不节流）
  → storeTemperatureReading（接收时间、幂等、守护门控）
  → CloudUploadRepository.enqueueAndUpload（幂等入队、异步上传）
  → 卡片/详情/早报
```

### 3.2 已确认的关键事实

- **BLE 设备块**：前 8 B 为 `sensor_id(4 B) | humidity_x10(u16be@4) | temperature_x10(s16be@6, 有符号)`；帧长规则 `16*(1+devCount)`；`devCount==0`、长度不足、密文未对齐、解密失败均显式拒绝且不产生读数（`GatewayFrameCodec.kt:96–176`）。
- **网关缓存是“最后值寄存器”**：`sensorres[]` 只在收到并成功解码 433 帧时按绑定顺序更新（`app_um2006A.c:180–196`），BLE 读**不消费/不清除**；`decode_frame10` 失败时 `timeoutcnt` 递减（成功解码置 100），耗尽后整体 `memset` 为 0 并下发（`:217–226`）。因此 App 每 180 s 读到的是“当前缓存值”，可能重复、也可能是全零占位。
- **网关广播**：`advertData` 含 16-bit 服务 UUID `0xFFE0`（`peripheral.c:109–123`、`gattprofile.h:36`）；扫描响应本地名为 `"xxx"`（`peripheral.c:86–105`）；GAP 设备名为 `"JYN Peripheral"`（`:126`、`:210`）。以上为源码事实，**不是空中抓包证据**。
- **MQTT 载荷无 ID**：`<epoch_seconds>:<temps csv>:<humis csv>`；数目/数值/时间非法时拒绝整批（`ReadingAttribution.kt:75–130`）。现有 `ReadingAttribution` 检查数目，不解决“数目相同但顺序错”。
- **App 自发布路径**：当 `mac_addr` 非空且不以 `1010` 开头时，App 在 **MQTT 连接成功时**把最后一笔 BLE 读数发布到 `/<mac>/a/r0`；否则订阅该 topic（`MqtttService.kt:1281–1297`）。该分支是既有兼容行为，本版不改路由，只改载荷内容/完整性。
- **本地库 v3**：`temperature(time, device_id, temperature, humidity, PK(time,device_id))`、`alarm_events`、`pending_uploads(device_id,time,...,attempts,next_attempt_at, PK(device_id,time))` + 索引。
- **上传**：`POST {HTTP_BASE_URL}/upload_data`，`phone/registerphone`、`mac/mac_addr`、`readings[{devId,time,temperature,humidity}]`；任何 2xx 视为成功；本地与服务端 `(dev_id,time)` 双重去重；单批 50；退避 60→1800 s、最多 20 次，达上限保留行不再自动重试。
- **上传门控**：守护运行中（`measure_state == "stopping"`）且 `registerphone` 非空。
- **告警**：下降判定用 5/10/15 min 时间窗，基准取自库且允许 30 min 回溯；`>65.0℃` 紧急通知每次触发；其他报警 900 s 去抖；三条文案与 `ALARM_TYPE_*` 为兼容面。
- **BLE 连接目标**：常量 `84:C2:E4:03:02:02`，`getRemoteDevice` 后直接 `connectGatt`；无扫描、无绑定、无地址持久化（`MqtttService.kt:261`、`:600–615`、`:794`）。
- **权限门**：`SDK >= S` 查 `BLUETOOTH_CONNECT`，否则查 `ACCESS_FINE_LOCATION`；门失败只打印一行日志后返回（`:552–557`、`:583–586`）。
- **BLE 零值**：`isZeroValue = (T == 0.0 && H == 0.0)`，连续 3 次才进入处理；计数为全局单值，非按设备（`:743–760`）。
- **启动权限请求**：`MainActivity` 正常流程只（在 API 33+）请求通知权限；定位请求段被注释；`requestBluetoothPermissions()` 仅在定位授权回调内可达（`:62–77`、`:143–158`、`:219–226`、`:267–284`）。
- **设备层未执行项**（沿用 ST-002/AF-001/TD-SW-003）：真机安装/升级、权限弹窗、真实 GATT、MQTT TLS、`/upload_data`、待发箱补传、跨息屏长跑、界面截图、instrumented SQLite 行级。

### 3.3 本次新增的上游事实对 Android 的含义

| 事实 | 来源 | Android 含义 |
|---|---|---|
| 既有服务在线、6 条 HTTP 路由存在、EMQX 5.8.0 | BE-EXP-002 §3/§4 | 端点与端口可继续复用；不改契约 |
| `/upload_data` 历史出现 401（同秒 401/401/200），当前 App 无凭证 | BE-EXP-002 §4.2/F1 | 上传可能被永久拒绝；必须可区分、可观测、保留数据（G-6）；鉴权机制未决 |
| 部署 `abr.py` 晚于迁移资料 26 天、体积变化 | BE-EXP-002 F2 | 资料不是当前部署的准确描述；App 不得据资料推断响应体/鉴权 |
| 近 18 天无 HTTP 业务、近 16 天无 MQTT 连接 | BE-EXP-002 §4.3 | 不能据历史推断上传/幂等/持久化；本设计不宣称上传可用 |
| 固件已实现升温 >0.9℃ 且无光 / 严格 >35.0℃；BLE 布局不变 | FD-002 rev 7、IC-002 v3.0 | Android 不重算门控；只同步文案与消费语义 |
| 光敏未标定、现场无光 | FD-002 rev 7 | 不影响 App（光照不上 BLE）；App 不显示光照状态 |
| 缓存复读、每 180 s 读取上界 480 行/设备/日 | system_analysis_current §7、BE-EXP-002 §10 | App 记录的是“接收时刻观测”，不是物理采样；不按值去重（D-24） |

---

## 4. 设计决策

沿用决策（D-01…D-15 与 AA-002/AA-003 一致，均已落地为 as-built，不再重做）：

| ID | 决策 |
|---|---|
| D-01 | BLE 记录偏移/字节序/符号不变，只做回归约束 |
| D-02 | 解析、归因、告警、上传载荷为无 Android 依赖的纯逻辑，可宿主机断言 |
| D-03 | App 不重算传感器门控、不推断光照、不合成缺失样本、不把未收到当 0 |
| D-04 | 告警下降判定改为时间窗（5/10/15 min），不按样本序号 |
| D-05 | 告警基准样本来自本地库（`getSamplesInRange`），重启后可判定 |
| D-06 | 入库时间戳为真实接收时间（BLE 接收时刻 / MQTT 载荷首段），无 60 s 回填；不得声称是物理采样时刻 |
| D-07 | 上传待发箱以 `(dev_id,time)` 幂等，成功即删，失败有界退避 |
| D-08 | 归因键必须是记录自身 ID；数目不一致拒绝整批，不截断 |
| D-09 | 上传门控 = 守护中且手机号非空 |
| D-10 | 不用数据龄判在线/离线；连通性由链路状态表达 |
| D-11 | BLE 读取周期 180 s，保留手动立即读取 |
| D-12 | 详情图表范围上限 24 h |
| D-13 | 服务器地址集中 `CloudConfig`，其余引用常量 |
| D-14 | `>35.0℃` 仅作呈现强调，App 不据此新增门控；`=35.0` 不强调 |
| D-15 | SQLite v3 仅为增量（新增 outbox 表），既有表结构与语义不动 |

AA-003 引入、本版保留：

| ID | 决策 |
|---|---|
| D-16 | 权限按**连接方式与 API 级别**判定：API 26–30 直连 GATT 只需清单中的普通蓝牙权限（安装时授予），**不再要求 `ACCESS_FINE_LOCATION`**；API 31+ 直连需 `BLUETOOTH_CONNECT`；仅实际使用 BLE 扫描时 API 26–30 才要求 `ACCESS_FINE_LOCATION` 且定位总开关开启，API 31+ 才要求 `BLUETOOTH_SCAN` |
| D-17 | 新增唯一“网关连接身份” `gateway_ble_addr`（冒号 MAC，MyPrefs）：由**有界扫描 + 用户确认**或手动输入产生并持久化；首次升级默认沿用既有 `84:C2:E4:03:02:02`，但该值只出现在**一处迁移常量**，不得再作为连接路径硬编码；带迁移标记，清除后不得静默回退 |
| D-18 | `mac_addr` 语义不变（MQTT topic + 上传 `mac`）；`gateway_ble_addr` 只用于 GATT。两者在设置页与连接日志中分别展示，不做自动换算 |
| D-19 | 权限缺失、蓝牙关闭、无已选网关、扫描无结果都是**一等可观测状态**：首页状态区 + 明确日志（含原因与所需动作），不得静默返回 |
| D-20 | 一次 BLE 读取的所有设备读数作为**不可拆分母批次**：本地按 ID 逐条处理；向 MQTT 转发只在“批次数 == 当前绑定集合数且 ID 与绑定顺序逐项匹配”时发布完整批次，否则拒绝转发并记录原因（本地处理与上传不受影响） |
| D-21 | 取消“`0.0℃/0.0%RH` = 无数据”哨兵：带**有效绑定 ID** 的 `0.0/0.0` 是合法读数，是否有效只由帧/记录有效性决定 |
| D-22 | App 自身的“温度湿度下降报警”是**独立功能**，保留；上报规则文案只描述传感器“升温 >0.9℃ 且无光 / >35.0℃”，两处措辞不得互相污染 |
| D-23 | 本轮所有逻辑改造必须可在宿主机（JVM）断言；需要真实蓝牙/系统 UI 的部分只做接线与状态输出，由测试能力独立验收 |

本版新增：

| ID | 决策 | 理由 / 备选否决 |
|---|---|---|
| D-24 | **BLE 观测按“接收时刻观测”记录，不做按值去重**：每次成功读取（非占位）都按接收时间入库并进入上传编排；重复读到同一缓存值不改变这一语义 | IC-002 §6 明确“BLE 记录时间取 App 接收 Unix 秒”，并禁止声称是物理采样时刻；App 的下降报警依赖 5/10/15 min 时间窗内的历史基准（`AlarmEvaluator` 30 min 回溯），若按值去重会让长时间稳定后的一次下降失去基准，破坏核心报警。否决“相同值不落库/不上传”：无法区分合法重复上报（>35℃ 分支可重复同值）且会牺牲报警与图表连续性。该语义的代价与容量上界见 §11 U-10，交产品/测试阶段决定是否另立压缩需求 |
| D-25 | **网关全零占位是链路状态，不是读数**：设备块 8 B 全零（ID `00000000` 且 `T=0.0` 且 `H=0.0`）判定为“网关暂无传感器数据”，不产生读数、不入库、不入待发箱、不生成/更新设备卡片、不触发报警；链路状态显示“网关暂无数据（等待传感器上报）”。带有效绑定 ID 的 `0.0/0.0` 仍按 D-21 正常处理 | 网关源码 `app_um2006A.c:217–226` 明确以全零块表示“no sensors”；`00000000` 不是有效设备身份。否决“继续用连续 3 次零值门槛”：会把占位写成 `00000000` 的假读数，并延迟真实 `0.0/0.0` |
| D-26 | **MQTT 转发载荷逐字按契约构造**：`<epoch_seconds>:<temp1,temp2,...>:<humi1,humi2,...>`，冒号分隔、温度在前、逗号分隔、无空格；数值格式 `%.1f` 且固定 `Locale.US`（小数点必须为 `.`）；时间取该次读取的接收 Unix 秒 | IC-002 §6 与 App 自身 `ReadingAttribution` 均按此解析；当前 `time,humi...,temp...` 会被自身解析器整批拒绝。单设备且 ID 匹配时与既有订阅端格式逐字一致 |
| D-27 | **上传结果四分类**：`Success`（2xx）、`AuthRejected`（401/403）、`RequestRejected`（其余 4xx，408/429 除外）、`Retryable`（5xx/408/429/超时/IO/解析异常）。`Retryable` 沿用既有退避；`AuthRejected`/`RequestRejected` **不自动重试**，保留全部行，置全局“上传被拒”状态并在设置页展示原因码/时间/待发条数，提供用户手动“重试上传” | 401 已被真实观测（BE-EXP-002 F1）；把永久拒绝当瞬时失败会耗尽 20 次后静默滞留。否决“继续统一退避”：用户无感且数据看似已处理 |
| D-28 | **不猜测鉴权协议**：本版不新增 `Authorization`/token/签名头，不改 `ApiService` 契约；只在 `CloudApiClient` 预留**唯一** OkHttp 拦截器注入点（当前为空实现），待端到端 D1 定性后由后续任务决定 | 401 机制（token/归属/注册态）未知；`LoginResponse.token` 当前未被使用，不能据字段存在推断用法。避免把未证实假设写成产品规则 |
| D-29 | **网关发现以 FFE0 过滤为主、显式无过滤为回退**：`ScanFilter` 用 `0000ffe0-0000-1000-8000-00805f9b34fb`；无结果时由用户再次点击“扩大搜索（不过滤）”执行无过滤扫描；两者均为 10 s 上限、必停 | 网关 `advertData` 含 16-bit `0xFFE0`（源码事实，未空中验证）；不同批次/固件可能不广播，保留回退。否决“只做无过滤扫描”：会把周围所有 BLE 设备暴露给用户，增加误选风险 |
| D-30 | **观测语义与界面措辞**：BLE 读到的值是“网关当前缓存值”，卡片相对时间标注为“最近读取/更新”，不写成“最近上报”；详情页说明“接收时间不是传感器采样时刻” | D-06 与 IC-002 §6；避免把缓存复读呈现为新采样 |

---

## 5. 目标架构

### 5.1 分层与状态归属

```text
[Transport]                                     [Pure logic]                          [Domain/State]         [Persistence]        [Cloud]
MqtttService(BLE GATT FFE0/FFE2) ─┐
                                  ├─► protocol/GatewayFrameCodec ─┐
MqtttService(MQTT messageArrived)─┘   telemetry/ReadingAttribution┤
                                                                  ├─► telemetry/AlarmEvaluator ─► DeviceState/UI ─► TemperatureDatabaseHelper ─► cloud/UploadOutbox ─► CloudUploadRepository
                                                                  ├─► telemetry/BleBatchForwarding（本轮新增）
                                                                  └─► protocol/GatewayIdentity（本轮新增）
ble/BlePermissionPolicy（本轮新增，纯逻辑） ◄── MainActivity / MqtttService / NotificationsFragment（网关设置区）
ble/GatewayDiscovery（Android BLE 扫描，本轮新增） ◄── protocol/GatewayIdentity
cloud/UploadResult（本轮新增，纯逻辑分类） ◄── CloudUploadRepository / RetrofitUploadApi
```

状态所有权（单一所有者）：

| 状态 | 所有者 | 说明 |
|---|---|---|
| 告警参数 | `MyPrefs`（全局）+ `DeviceParamsBook`（每设备覆盖） | 不变 |
| 守护开关 | `HomeViewModel.startData` / `MyPrefs.measure_state` | 不变 |
| 设备展示状态 | `HomeViewModel.devices` | 不变；新增链路/上传状态字段（见 §6.6、§7.7） |
| 样本历史（告警基准与图表） | SQLite `temperature` | 不变；每次成功读取按接收时间写入 |
| 待上传队列 | SQLite `pending_uploads` | 不变 |
| **网关连接身份** | `MyPrefs.gateway_ble_addr`（本轮新增） | 只用于 GATT |
| **网关平台身份** | `MyPrefs.mac_addr`（不变） | topic + `/upload_data.mac` |
| **链路/权限状态** | `MqtttService` 运行态 → 首页状态区（本轮新增可见性） | 不再由数据龄推导 |
| **上传阻塞状态** | `MyPrefs`（`upload_blocked_reason`、`upload_blocked_at`、`upload_last_status`）+ `NotificationsViewModel` | 本轮新增；不改变 outbox 表结构 |

### 5.2 模块清单

已存在（as-built，本轮不改结构）：`protocol/GatewayFrameCodec.kt`、`telemetry/ReadingAttribution.kt`、`telemetry/AlarmEvaluator.kt`、`telemetry/SensorCadence.kt`、`cloud/CloudConfig.kt`、`cloud/UploadPayload.kt`、`cloud/UploadOutbox.kt`、`cloud/CloudUploadRepository.kt`、`ui/detail/DetailPresentation.kt`、`ui/home/DeviceState.kt` 等。

本轮新增：

| 文件 | 职责 | 依赖 |
|---|---|---|
| `ble/BlePermissionPolicy.kt` | 纯逻辑：输入 `sdkInt`、各权限授予状态、`needScan`、适配器/定位开关状态，输出“缺失权限列表、是否需定位总开关、可否直连、可否扫描、用户提示类别”；不含 Android 类型 | 无（可 JVM 测试） |
| `protocol/GatewayIdentity.kt` | 纯逻辑：MAC 规范化（大小写/分隔符/12 位 HEX）、合法性校验、扫描候选去重与排序、绑定状态、迁移默认值判定 | 无 |
| `ble/GatewayDiscovery.kt` | Android 侧有界扫描（`BluetoothLeScanner`）：FFE0 过滤优先、显式无过滤回退；10 s 上限；停止/取消/异常路径必停；产出 `GatewayCandidate(mac, name, rssi, serviceUuids)` | Android BLE |
| `telemetry/BleBatchForwarding.kt` | 纯逻辑：把一次读取的有效读数（已排除占位）与绑定 ID 顺序匹配，输出 `Forward(payload)` 或 `Refuse(reason)`；按 D-26 构造载荷 | 无 |
| `telemetry/BleObservation.kt` | 纯逻辑：判定占位块（D-25）、判定有效读数、把 `SensorSample` 归一为 `{devId, tX10, hX10}`；供读取路径与测试共用 | 无 |
| `cloud/UploadResult.kt` | 纯逻辑：HTTP 状态/异常 → `Success/AuthRejected/RequestRejected/Retryable`；含状态码与可读原因 | 无 |

本轮修改：`MqtttService.kt`（权限门、地址来源、批次转发、占位与零值、上传结果处理、状态输出、发布时机）、`MainActivity.kt`（按 API 请求权限与拒绝处理）、`ui/notifications/NotificationsFragment.kt`（网关 BLE 发现入口、`gateway_ble_addr` 展示/清除、上传状态与手动重试）、`ui/home/HomeFragment.kt` + `HomeViewModel.kt`（链路状态区）、`ui/detail/DetailPresentation.kt`（规则文案）、`cloud/CloudUploadRepository.kt` + `cloud/CloudApiClient`（结果分类与拦截器注入点）、`res/layout`/`res/values`（状态区与候选列表）、文档与版本。

---

## 6. 关键流程与状态机

### 6.1 守护启动与权限

```text
用户开启守护（HomeFragment）
  → BlePermissionPolicy.evaluate(sdkInt, granted, needScan=false, adapterEnabled, locationEnabled)
      ├─ canConnect=true  → 启动/恢复 180 s 轮询与 MQTT
      └─ canConnect=false → 首页状态区显示缺失项 + “去授权”
                            → 用户授权后重新评估；拒绝/不再询问 → 跳系统应用设置
  → 若用户进入“选择网关”（需要扫描）→ evaluate(needScan=true)
      → API<31 额外要求 ACCESS_FINE_LOCATION 且定位总开关开启（提示去开启）
      → API31+ 要求 BLUETOOTH_SCAN
```

规则：
- API < 31 的**直连**分支不读取 `ACCESS_FINE_LOCATION`；`isBluetoothBusy()`/`connectToBluetoothDevice()` 使用同一策略对象，避免两处口径漂移。
- 权限检查失败时写入一次可读状态（如“蓝牙权限未授予：需要附近设备权限”），供首页展示；日志含 `sdkInt`、缺失权限、`needScan`、`adapterEnabled`。
- 权限请求只请求清单已声明的权限；`BLUETOOTH_CONNECT/SCAN` 仅在 `SDK >= 31` 请求，`POST_NOTIFICATIONS` 仅在 `SDK >= 33` 请求；API<31 的扫描才请求定位。
- `MqtttService` 在权限恢复后不重启进程即可继续下一轮（不清空既有库/队列）。
- 蓝牙关闭：状态“蓝牙未开启”，不空转刷日志；适配器开启后下一轮恢复。

### 6.2 网关发现、绑定与连接

状态：`未配置（仅迁移默认值）→ 扫描中 → 有候选/无候选 → 已保存 → 连接中 → 已连接/连接失败`

1. 读取 `gateway_ble_addr`；为空则使用迁移默认值（既有常量），并在网关设置区标注“使用默认网关地址（未确认）”。迁移默认只在键从未存在且迁移标记未置位时生效；用户清除后写“已清除”标记，不回退。
2. 用户点“搜索附近网关”：`GatewayDiscovery.start()`（10 s 上限）→ 候选列表（名称、MAC、RSSI、是否含 FFE0）；无结果时给出“未找到，可再试 / 扩大搜索（不过滤）/ 手动输入 MAC”，**不自动连接**任何未确认地址。
3. 用户确认候选或手动输入 → `GatewayIdentity.normalize/validate` → 写入 `gateway_ble_addr` → 停止扫描 → 下一轮读取使用该地址。
4. 连接日志同条输出 `gateway_ble_addr` 与 `mac_addr`，分别标注“BLE 连接目标 / 平台身份”，不做换算。
5. 清除身份：回到“未配置”状态并显示动作；不静默回退到旧常量。

扫描取消/页面销毁/超时/蓝牙关闭/权限撤销 → 必须 `stopScan` 且释放回调；任何异常都不得留下持续扫描。

### 6.3 一次 BLE 读取

```text
读 FFE0/FFE2 → 解密 → GatewayFrameCodec
  → Rejected：记录原因，不产生读数（不转发、不入库、不上传）
  → Success：
      for each sample：
          BleObservation.classify(sample)
            ├─ Placeholder（8 B 全零）→ 记录“网关暂无数据”，跳过
            └─ Reading：
                processTemperatureHumidityData(devId, T, H, 接收时间)   // 告警、入库、上传、卡片
      BleBatchForwarding.plan(readings, MacIdBook.all(), 接收时间)   // readings 已排除占位块
          → Forward(payload) ：按 D-26 构造 `<time>:<temps>:<humis>` 并发布（见 6.4）
          → Refuse(reason)   ：不发布，记录原因（本地处理已完成；含占位的批次按 COUNT_MISMATCH 拒绝）
```

约束：
- 批次不得按“最后一笔”覆盖；不得只转发首台；不得在数目/ID/顺序不符时发布部分批次。
- 占位块不参与本地处理与转发；若整批均为占位 → 状态“网关暂无传感器数据”，不发布。
- 单设备且 ID 匹配时，载荷字符串与既有格式逐字一致（`<time>:<temp>:<humi>`）。
- 时间取该次读取的接收 Unix 秒，不声称是物理采样时刻（D-06/D-30）。

### 6.4 MQTT 转发时机

- 每次成功读取并得到 `Forward` 后，若 MQTT 已连接 → 立即发布；若未连接 → 只保留**最近一批** `pendingForward`（不建无界队列），在 MQTT 连接成功回调中发布一次并清空。
- 发布失败：记录日志与状态，不阻塞本地处理；下一批成功读取会覆盖 `pendingForward`（最新优先）。
- 兼容：保留既有 topic 选择分支（`mac_addr` 非空且不以 `1010` 开头时发布到 `/<mac>/a/r0`，否则订阅），不改 topic 规则。

### 6.5 本地入库与上传

不变：`ReadingAttribution` 严格归因 → 同一处理函数 → 入库/告警 → 待发箱 → `/upload_data`。传输失败只影响上传，不影响读数处理。

新增：
- 每次成功读取（非占位）按接收时间入库并进入上传编排（D-24，不去重）。
- 上传结果按 D-27 分类：
  - `Success` → 逐行 `ack`。
  - `Retryable` → 逐行 `markFailed(backoff)`，保留状态“重试中”。
  - `AuthRejected` / `RequestRejected` → **不**增加 `attempts` 到 20 次退避；把所有本次批次行置为“暂停自动重试”（实现可用 `markFailed` 到上限或新增 `park` 语义，但不得删除行），置全局阻塞状态（原因码、HTTP 状态、时间、待发条数），在设置页展示并提供“重试上传”。
  - 用户点“重试上传” → 清除阻塞状态、把被暂停的到期行恢复为可重试（`attempts=0`/`next_attempt_at=0` 的批量 UPDATE，不改表结构），立即触发一次上传。
- 上传阻塞不得影响 BLE/MQTT/告警/入库；不得删除本地行。

### 6.6 链路状态机（用户可见）

```text
UNKNOWN → (评估) → PERMISSION_MISSING | BLUETOOTH_OFF | GATEWAY_UNSET | GATEWAY_DEFAULT_UNCONFIRMED
        → CONNECTING → CONNECTED → (读取成功) → DATA_OK | GATEWAY_NO_DATA（全零占位）
                                   → (读取失败/超时) → CONNECT_FAILED
        → (权限撤销/蓝牙关闭) → 对应状态
```

状态文本只陈述可观测原因，不使用数据龄；`GATEWAY_NO_DATA` 与“连接失败”“设备离线”分开。

---

## 7. 接口与契约

### 7.1 本地偏好键

| 键 | 语义 | 变化 |
|---|---|---|
| `gateway_ble_addr` | GATT 连接目标（冒号 MAC，大写规范化） | 新增 |
| `gateway_ble_addr_cleared` | 用户已显式清除，禁止回退迁移默认 | 新增（布尔） |
| `mac_addr` | 网关平台身份：topic `/<mac>/a/r0` 与 `/upload_data.mac` | 语义不变 |
| `upload_blocked_reason` / `upload_blocked_at` / `upload_last_status` | 上传被拒/最近一次结果（原因码、HTTP 状态、时间） | 新增 |
| `registerphone` / `measure_state` / 报警参数键 | 现状 | 不变 |

迁移：`gateway_ble_addr` 缺失且 `gateway_ble_addr_cleared=false` 时读取默认常量并视作“未确认”；首次用户选择后写入。不得把 `mac_addr` 自动写入 `gateway_ble_addr`。

### 7.2 权限策略（纯逻辑）

```kotlin
enum class PermissionNeed { BLUETOOTH_CONNECT, BLUETOOTH_SCAN, FINE_LOCATION, POST_NOTIFICATIONS }

data class Policy(
    val canConnect: Boolean,
    val canScan: Boolean,
    val missing: List<PermissionNeed>,
    val requiresLocationServices: Boolean,   // 仅 needScan 且 API<31
    val reason: LinkReason,                  // 一等状态（见 6.6）
    val userMessage: String                  // 面向用户的一句原因与动作
)

fun evaluate(
    sdkInt: Int,
    granted: Set<PermissionNeed>,
    needScan: Boolean,
    adapterEnabled: Boolean,
    locationEnabled: Boolean
): Policy
```

语义：`sdkInt >= 31` 直连缺 `BLUETOOTH_CONNECT` → 不可连接；`sdkInt in 26..30` 直连恒 `canConnect = true`（普通权限安装时授予），但 `adapterEnabled=false` → 不可连接且状态为 `BLUETOOTH_OFF`；`needScan` 时 `sdkInt >= 31` 需 `BLUETOOTH_SCAN`，`< 31` 需 `FINE_LOCATION` 且 `requiresLocationServices = true`（`locationEnabled=false` → 不可扫描并提示）。

### 7.3 网关身份（纯逻辑）

```kotlin
fun normalize(raw: String): String?            // "84:c2:e4:3:2:2" → "84:C2:E4:03:02:02"
fun isValid(raw: String): Boolean
fun rank(candidates: List<Candidate>): List<Candidate>   // 含 FFE0 优先，其次 RSSI 降序，MAC 去重
fun planBinding(selected: String?, boundIds: List<String>): BindingState
fun isMigrationDefault(selected: String?, cleared: Boolean): Boolean
```

### 7.4 批次转发（纯逻辑）

```kotlin
sealed interface Plan {
    data class Forward(val payload: String) : Plan
    data class Refuse(val reason: RefuseReason) : Plan
}

enum class RefuseReason { NO_BOUND_DEVICE, COUNT_MISMATCH, UNKNOWN_ID, DUPLICATE_ID, ORDER_MISMATCH, ALL_PLACEHOLDER }

fun plan(readings: List<Reading>, boundOrder: List<String>, timeSeconds: Long): Plan
```

载荷：`"$timeSeconds:${temps.joinToString(",") { "%.1f".format(it) }}:${humis.joinToString(",") { "%.1f".format(it) }}"`（温度在前、湿度在后、冒号三段、逗号分隔、无空格）。

### 7.5 观测分类（纯逻辑）

```kotlin
/** 已归一的有效读数（x10 整数用于精确比较/载荷，浮点用于既有处理链路） */
data class Reading(
    val devId: String,
    val tX10: Int,
    val hX10: Int,
    val temperatureC: Double,
    val humidityPct: Double
)

sealed interface Observation {
    data object Placeholder : Observation     // 8 B 全零（ID 00000000 且 T=0.0 且 H=0.0）
    data class Valid(val reading: Reading) : Observation
}

fun classify(sample: SensorSample): Observation
```

`GatewayFrameCodec.SensorSample` 增加 `temperatureX10: Int`、`humidityX10: Int`（additive，不改变现有字段/偏移）。

### 7.6 上传结果分类（纯逻辑）

```kotlin
sealed interface UploadResult {
    data object Success : UploadResult
    data class AuthRejected(val httpStatus: Int) : UploadResult
    data class RequestRejected(val httpStatus: Int) : UploadResult
    data class Retryable(val httpStatus: Int?, val cause: String) : UploadResult
}

fun fromHttp(status: Int?): UploadResult        // 2xx→Success；401/403→AuthRejected；4xx(除408/429)→RequestRejected；5xx/408/429→Retryable
fun fromError(cause: Throwable): UploadResult   // 超时/IO/解析异常→Retryable
```

### 7.7 用户可见文本（逐字冻结）

详情页规则说明：

> 采样周期 3 分钟。仅当温度较上次**升高**超过 0.9℃且无光，或温度超过 35.0℃ 时才上报；未满足条件时不会上报，因此可能长时间没有新数据。

- 不得出现“较上次下降超过 0.9℃”的表述；App 不显示光照状态/照度/推断的上报原因。
- App 自身报警面板的“温度湿度下降报警”保留原措辞（D-22）。
- 用户说明书与 App 变更记录同步为“升高”，并注明严格边界（0.9℃ 需超过、35.0℃ 需超过、等于不触发）。
- 卡片/详情相对时间标注为“最近读取/更新”；详情说明“接收时间为手机读取网关缓存的时刻，不是传感器采样时刻”（D-30）。

链路状态文案（示例，实现可按现有 UI 风格微调，但原因不得合并）：
- “蓝牙权限未授予：需要附近设备权限（去授权）”
- “蓝牙未开启（去开启）”
- “未配置网关地址（使用默认地址，未确认 / 搜索附近网关）”
- “网关连接失败（重试 / 检查网关电源）”
- “网关暂无传感器数据（等待传感器上报）”
- “云端上传被拒绝（HTTP 401/403），数据已保留（重试上传）”
- “云端上传请求被拒绝（HTTP 4xx），数据已保留”
- “云端上传失败，重试中（待发 N 条）”

### 7.8 其余接口

SQLite v3 schema、`/upload_data` 请求体、MQTT topic 与载荷格式、BLE 记录布局、AES/证书/UUID/MTU 均不变（见 §9）。

---

## 8. 异常与边界

| 场景 | 设计行为 |
|---|---|
| 直连权限缺失（API 31+） | 不发起连接；首页显示缺失权限与“去授权”；日志含 SDK 与权限名 |
| API<31 且定位权限未授予 | **直连照常**；仅“搜索附近网关”（扫描）被拦并提示 |
| 定位总开关关闭（API<31 扫描） | 扫描前提示开启；不请求无效扫描；不因此阻断直连 |
| 蓝牙适配器关闭 / 未授权 | 状态“蓝牙未开启”；提示开启；轮询不空转刷日志 |
| 未选择网关（仅默认值） | 允许尝试默认地址但界面标注未确认；用户可立即搜索/手输 |
| 扫描无结果 / 超时 / 取消 | 停止扫描，显示“未找到网关”，保留手动输入与“扩大搜索”；不自动连接其他设备 |
| 多台候选 | 用户显式选择；不自动挑选 |
| 保存的地址非法或无法 `getRemoteDevice` | 回到“未配置”状态并提示重新选择；不崩溃 |
| BLE 帧被拒绝 | 不产生读数、不转发、不入库、不上传（现状保持） |
| 设备块 8 B 全零 | 判为占位：跳过；整批占位 → “网关暂无传感器数据”；不生成 `00000000` 设备 |
| 批次与绑定不符（数量/ID/顺序） | 拒绝转发 + 记录原因；本地按 ID 处理与上传仍执行 |
| 带有效 ID 的 `0.0℃/0.0%RH` | 视为合法读数，正常判定/入库/上传/展示 |
| MQTT 数目不一致 / 非法值 / 非法时间 | 拒绝整批（现状保持） |
| 上传成功 / 网络失败 / 5xx | 2xx 删除待发行；其余按退避重试（现状保持） |
| 上传 401/403 | 不自动重试；保留全部行；全局“上传被拒绝”状态 + 手动重试；不影响主链路 |
| 上传其他 4xx | 不自动重试；保留行；状态“请求被拒绝” + 手动重试 |
| 守护关闭 / 未注册手机号 | 不入库（守护关闭）、不入队、不发请求 |
| 长时间无新数据 | 正常（条件上报）；只显示相对读取时间与提示，不显示“离线” |
| 权限在运行中被撤销 | 下一轮检测到即停止连接并显示状态；恢复后继续，不清数据 |
| MQTT 未连接时的 BLE 读取 | 本地照常处理；仅保留最近一批待转发，连接成功后发布一次 |

---

## 9. 兼容面（不得改动）

协议与安全：BLE 记录布局/字节序/符号、帧长规则、AES 密钥与模式、`assets/jd-ca.crt`、`TLSv1.2`、MQTT 用户名/密码拼接、topic 规则（`/<mac>/a/r0|r1|s0`）、MQTT 批量载荷格式（`time:temps:humis`）、MTU 240、GATT UUID、`0xA1` 写入头与分块、时间校准载荷、`mac_addr` 语义、`/upload_data` 字段与单位、SQLite 既有表结构与语义、65℃ 阈值、900 s 去抖、三条报警文案、`ALARM_TYPE_*`、`MyPrefs` 既有键名、`MacIdBook`/`DeviceParamsBook` 存储格式与 8 位 HEX 校验。

允许变更：新增 `gateway_ble_addr` 与网关选择 UI/状态展示、权限请求与提示、批次转发内部实现、占位判定与零值哨兵移除、上传结果分类与阻塞状态、规则文案、版本号与文档。

---

## 10. 验证策略

### 10.1 宿主机可独立执行（实现阶段必须给出）

| 目标 | 断言要点 |
|---|---|
| 权限策略 | SDK 26/29/30/31/33 矩阵 × 直连/扫描 × 授予/拒绝 × 适配器/定位开关：API<31 直连 `canConnect=true` 且 `missing` 不含定位；`needScan` 时 API<31 需定位、API31+ 需 SCAN；`BLUETOOTH_OFF`/`LOCATION_OFF` 状态与提示一致 |
| 网关身份 | 规范化/校验（大小写、单双位段、分隔符、非法长度/字符、空值）；候选排序（FFE0 优先、RSSI、去重）；迁移默认与“已清除”状态；未选择/已选择 |
| 批次转发 | 单设备→`<time>:<temp>:<humi>` 逐字一致；多设备按绑定顺序；数量不符/未知 ID/重复 ID/顺序不符/全占位全部 Refuse 且不产出部分载荷；空批次不转发 |
| 占位与零值 | 8 B 全零 → Placeholder（不处理）；`ID=有效, T=0.0, H=0.0` → Reading（正常处理）；无“连续 3 次”门槛 |
| 上传分类 | 200/201/204→Success；401/403→AuthRejected；400/404/413/422→RequestRejected；408/429/500/502→Retryable；异常→Retryable |
| 上传阻塞与恢复 | 401 后不再自动请求、行不删除、状态写入；手动重试清除阻塞并恢复到期行；网络失败仍按退避；成功逐行删除 |
| 规则文案 | 含“升高”、不含“较上次下降超过 0.9”；0.9/35.0 严格表述；无光照/照度/lux 与推断原因 |
| 回归 | 现有宿主机用例全部通过；`assembleDebug` 成功；无新增编译告警 |

### 10.2 静态核查

- 直连路径不再出现 `ACCESS_FINE_LOCATION` 作为 API<31 连接前置（扫描路径除外）。
- Android 应用源码（`dengbei_care/app/src/main`）内，`84:C2:E4:03:02:02` 只允许出现在迁移默认常量一处；历史说明文档与 `iOS开发资料/` 下的参考副本不属于 Android 运行时代码，不在此静态门禁范围。
- 不存在 `bleTempData = listOf(...)` / `bleHumiData = listOf(...)` 的“最后一笔”转发状态与 `bleZeroCount`/`isZeroValue` 哨兵分支。
- 转发载荷构造只在一处，且为 `time:temps:humis`；不存在 `bletime + ',' + ...` 的拼接。
- 项目内不再存在“较上次下降超过 0.9”的规则文案。
- 服务器地址仍只在 `CloudConfig` 定义；`ApiService` 未新增未证实鉴权头。
- `pending_uploads` 表结构未变；上传阻塞状态只落在 `MyPrefs`。

### 10.3 必须由测试能力在真机独立验收（本设计不执行）

1. Android 10（Redmi 8A / API 29）开启守护：无定位授权时仍能发起 GATT 连接；不再出现仅一行 `Bluetooth permission not granted` 的静默路径。
2. 网关发现：有界扫描、候选呈现、保存后重连使用已保存地址；扫描必停；蓝牙关闭/无候选的界面状态；记录网关实际是否广播 FFE0/名称。
3. API 31+ 设备（或模拟器）上 `BLUETOOTH_CONNECT/SCAN`、通知权限的请求与拒绝路径。
4. 真实 GATT 读取多设备帧：本地逐条入库 + 批次转发与绑定顺序一致；数目/ID 不符时拒绝转发；全零占位不产生 `00000000` 设备。
5. 3 分钟轮询跨息屏/充电/省电长跑、前台服务存活。
6. MQTT TLS、`/upload_data` 幂等、待发箱补传、界面与设备层 SQLite 升级。
7. 上传 401/403 的真实表现与“手动重试”路径；401 机制定性属端到端 D1，不在本设计内。

设备侧 instrumented 测试的依赖缺失（UTP/无外网）属测试环境问题，不由本设计用宿主机结论替代。

---

## 11. 未决项与交接

| ID | 内容 | 影响与处置 |
|---|---|---|
| U-1 | 网关 BLE 广播是否在真实空中包含 FFE0/名称（源码显示含 FFE0、名为 `xxx`） | 设计以“FFE0 过滤优先 + 用户显式无过滤回退”兼容；由测试能力记录实际广播 |
| U-2 | `mac_addr`（topic/上传）与 BLE MAC 是否同一物理地址 | 设计保持两者独立（D-17/D-18），不互相推导 |
| U-3 | 默认迁移值 `84:C2:E4:03:02:02` 对应的真实网关是否存在/可达 | 保留为兼容默认 + “未确认”标注；用户可通过发现流程替换 |
| U-4 | MIUI 12.5 后台/自启动对 180 s 轮询的影响 | 属真机长跑验收（测试能力） |
| U-5 | `/upload_data` 的鉴权机制（401 的具体原因）与真实持久化/回读 | 端到端 D1–D6 定性；Android 侧只提供可观测阻塞与唯一注入点（D-27/D-28），不猜测协议 |
| U-6 | ~~固件判据仍与需求冲突~~ | **已由 FD-002 rev 7 / IC-002 v3.0 取代**：当前固件实现升温与严格 >35.0℃；App 不变更门控 |
| U-7 | 设备端 instrumented/SQLite 行级验证受阻（依赖缺失、无外网） | 交测试能力；不视为 App 逻辑已验 |
| U-8 | 全暗标定、真实温湿度采集、光照证据 | 固件/嵌入式与硬件责任；App 无光照字段，不受影响 |
| U-9 | `artifacts/software_test_design.md` 的追踪表仍按旧 ITEM-001..004 编号，且 U-6 仍引用“固件判据冲突”旧结论 | 属测试能力工件，需由该能力把新增两项（观测有效性/占位、上传分类与 401）纳入追踪并同步 U-6；本设计不修改他人工件。其已有 T-SW-L1-13/19、L2-26..31、REG-09 可直接覆盖本版多项验收 |
| U-10 | 缓存复读的行语义：每 180 s 观测 → 本地/云端约 480 行/设备/日（1000 设备约 12–26 GB/年）；是否另立“按值压缩/去重”需求 | 本设计按 D-24 保持观测语义（保护下降报警基准与合法重复上报）；容量上界由 BE-EXP-002 §10 记录。若产品决定压缩，需单独需求并重新评估报警基准，不在本能力擅自改变 |
| U-11 | 部分设备无数据（网关部分占位）时整批不转发，其他手机收不到该批 | D-20 冻结格式无“无数据”占位语义；设计保持整批拒绝，本地处理不受影响。若需部分转发，需协议变更，属产品决策 |
| U-12 | 网关/手机跨链路“同一笔”对照 | 需真实网关、手机与受光/遮光/升温条件；由嵌入式测试与联调阶段执行 |
| U-13 | 上传 4xx 的响应体/错误码含义 | 未读取部署源码（BE-EXP-002 未授权）；Android 只按 HTTP 状态分类，不解析未证实响应体 |

---

## 12. 自检

- [x] 仅覆盖 Android（`dengbei_care`），未改固件/网关/服务器/iOS/硬件
- [x] 上报方向按 readme 与 IC-002 v3.0 为“升温 >0.9℃ 且无光，或 >35.0℃”，严格边界未放宽
- [x] App 仍不重算传感器门控、不推断光照、不合成样本；不引入按值去重改变观测语义
- [x] 六项缺口均给出可独立检验的功能增量（状态/接口/预期行为），并落到具体文件
- [x] 未把宿主机/模拟器结论当作真机事实；设备层验收明确交测试能力
- [x] 兼容面与不可改动清单完整，含 `mac_addr` 语义、topic/上传契约与 SQLite 表结构
- [x] 未猜测 401 鉴权协议；未把 HTTP 2xx 当作持久化通过
- [x] 未决项保留未决，未以“外部环境”为由删除目标
- [x] 未修改其他角色工件；测试/后端待同步项已登记为交接

任务清单见 `artifacts/android_tasks.yaml`（6 项，按前置依赖排序）。
