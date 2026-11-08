# Android App 方案设计（AA-003）

状态：Android 方案设计（供实现与测试消费；不代表任何设备或云端行为已验证）
版本：2.0（替代 AA-002 v1.0；v1.0 的框架迁移部分已落地为 as-built，本版更正上报方向并补齐连接侧未解决项）
责任角色：Android 工程
范围：仅 Android App（`dengbei_care`，Kotlin）。不含固件、网关、服务器端代码、iOS 与硬件改动。
工程基线：Git HEAD `a0342eb` 工作区干净；源码 `versionCode 9 / versionName 1.8`，`minSdk 26`、`targetSdk 34`、`compileSdk 34`。

输入来源：
- `readme.txt`（需求方，权威，八项修改点）
- `artifacts/system_analysis.md`（系统架构当前核对：判据方向、Android 连接前提、服务复用缺口）
- `artifacts/interface_contract.md`（IC-002 v3.0：升温 >0.9℃、严格 >35.0℃、BLE 记录布局、MQTT 批量格式、Android 消费边界、D-03）
- `artifacts/firmware_requirements.md` / `artifacts/firmware_design.md`（FWR-002 / FD-002：传感器采样与上报语义、失败不伪造数据）
- `artifacts/android_feasibility.md`（AF-001：Redmi 8A / API 29 只读事实、F-1 权限门、F-2 网关身份）
- `evidence/software_test.md`（ST-002 及 ST-001…ST-014R：宿主机层已验项、设备层未执行项）
- `artifacts/android_implementation.md`（AIMPL-002：ITEM-001…ITEM-014 实现记录与修复历史）
- `服务器迁移记录.md`（§二/§五/§八：broker、HTTP、`/upload_data` 与 MySQL 幂等声明）
- 当前 Android 源码（as-built 事实）

---

## 1. 本次设计结论

### 1.1 已完成并可作为 as-built 复用（宿主机层已验，设备层未验）

| 能力 | 实现位置 | 证据边界 |
|---|---|---|
| BLE 聚合帧唯一解码（`id4 | hum u16be@4 | temp s16be@6`） | `protocol/GatewayFrameCodec.kt` | 宿主机单测 + 构建；真实 GATT 未验 |
| MQTT 批量载荷严格归因（数目不一致拒绝整批） | `telemetry/ReadingAttribution.kt` | 同上 |
| 告警时间窗判定（5/10/15 min，基准来自库） | `telemetry/AlarmEvaluator.kt` + `MqtttService` 接线 | 同上 |
| SQLite v3（`temperature`、`alarm_events`、`pending_uploads`） | `TemperatureDatabaseHelper.kt` | schema/语句级；真机迁移未验 |
| 真实采样时间戳单条写入 | `storeTemperatureReading(...)` | 语句级；真机行级未验 |
| 云上传待发箱与编排（幂等、退避、门控、串行） | `cloud/UploadOutbox.kt`、`cloud/CloudUploadRepository.kt`、`cloud/UploadPayload.kt` | 宿主伪实现；真机 SQLite 与 HTTP 未验 |
| 服务器常量单一来源（`8.140.23.253`，8883/5000） | `cloud/CloudConfig.kt` | 静态 + 构建；TLS/上传可达性未验 |
| 卡片不再以 30 min 数据龄判“离线” | `DeviceState.kt`、`DeviceCardAdapter.kt` | 逻辑层；界面未验 |
| 详情 24 h 范围、高温强调、规则说明 | `ui/detail/DetailPresentation.kt`、`DeviceDetailFragment.kt` | 逻辑层；界面未验 |
| 早报“按条件上报/非等间隔/昨日无上报” | `report/ReportGenerator.kt` | 文本层；通知展示未验 |
| 3 分钟读取节拍（180 s） | `telemetry/SensorCadence.kt` | 常量断言；真机长跑未验 |

### 1.2 本版必须补齐的四项（当前源码确认仍存在）

| 编号 | 缺口 | 当前证据（文件:行/行为） |
|---|---|---|
| G-1 | BLE 运行时权限模型与请求流程不一致：Android 10（API 29）上直连 GATT 被应用自身要求 `ACCESS_FINE_LOCATION`，而正常启动流程从不请求该权限 → 每 180 s 轮询静默返回 | `MqtttService.kt:552-590`；`MainActivity.kt:63-77` 与 `:143-156` 请求段被注释；仅 `GatewayFragment` 在“扫描 Wi-Fi”时请求定位 |
| G-2 | 网关身份硬编码且无发现/绑定流程，与环境登记“由 App 发现并连接、实际名称与地址未确认”冲突；`mac_addr`（topic/上传）与 BLE 地址互不关联 | `MqtttService.kt:261`（`84:C2:E4:03:02:02`）；全仓无 BLE 扫描实现；`mac_addr` 由用户在 `NotificationsFragment` 手输 |
| G-3 | BLE 读取批次只保留最后一笔用于 MQTT 转发（多设备时会丢数据/错配），且把 `0.0℃/0.0%RH` 当作“无数据哨兵” | `MqtttService.kt:748-752`（只存最后一笔）、`:1291` 附近转发；`:743-760` 连续 3 次零值才处理 |
| G-4 | 用户可见上报规则写成“温度较上次**下降**超过 0.9℃”，与 readme/IC-002 冻结的**升温**方向相反 | `ui/detail/DetailPresentation.kt:26-30`；`dengbei_care/用户使用说明书.md:110/241`；`dengbei_care/readme.txt:26` |

其余系统级缺口（真实温湿度采集、全暗标定、服务器核验、设备端体验层）不属于 Android 设计范围，见 §11 交接。

### 1.3 为什么这四项不能只靠测试或环境解决

G-1/G-2 是应用自身代码路径的确定性缺陷（不依赖外部条件即可从源码证明），G-3 是数据正确性缺陷（多设备时静默错配/丢失），G-4 是用户可见规则与产品需求相反。四者均可通过本设计的纯逻辑 + 接线改造消除，并可在宿主机独立断言；真实设备上的弹窗、发现、GATT、长跑仍需测试能力独立验收（§10.3）。

---

## 2. 需求到 Android 的落点

| readme 修改点 | Android 侧落点 | 当前状态 |
|---|---|---|
| 1 GXHT40 替换温湿度器件 | 单位（0.1℃/0.1 %RH）与 BLE 布局不变，解析逻辑不需改；只做回归约束 | 已实现（`GatewayFrameCodec`） |
| 2 新增光敏电阻与分压 | 光照不上空口、不上 BLE；App 不推断、不显示光照/照度 | 已实现（无光照字段与展示） |
| 3 采集时间改为 3 分钟 | 传感器采样节拍；App 读取周期对齐 180 s；不得把“无上报”当异常 | 已实现（`SensorCadence`、卡片/详情/早报文案） |
| 4 条件上报：本次较上次**升高大于 0.9℃且无光**，或温度**超过 35.0℃** | App 不重算该门控、不推断光照、不合成样本；规则文案必须与方向、严格边界一致 | 门控实现正确，**文案方向错误（G-4）** |
| 5 平台 broker/服务器信息 | MQTT `ssl://8.140.23.253:8883`、HTTP `http://8.140.23.253:5000`、明文放行、`POST /upload_data` | 已实现（`CloudConfig`、`ApiService`） |
| 6 同步 Android、不改 iOS、退役光通信/霍尔 | 仅 Android；线上格式未变；退役模块不引入 App 逻辑 | 已实现（约束登记） |
| 环境：网关由 App 发现并连接 | 必须有发现/绑定/持久化身份，不得硬编码地址 | **未实现（G-2）** |
| 环境：真机为 Android 10/API 29 | 权限模型须按 API 级别分流，连接路径不得依赖未请求的权限 | **未实现（G-1）** |

---

## 3. As-built 事实（本次设计必须尊重的现状）

数据链路：

```text
BLE：GATT FFE0/FFE2 读 → AES/ECB 解密 → GatewayFrameCodec（按 4 B ID 归因）
MQTT：/<mac>/a/r0  → ReadingAttribution（按 MacIdBook 顺序，数目不一致拒绝整批）
        ↓（两条入口汇聚）
processTemperatureHumidityData(devId, T, H[, 时间])
  → AlarmEvaluator（时间窗下降 + 阈值 + >65℃ 紧急）
  → alarm_events 落库 / 通知（900 s 去抖，紧急不节流）
  → storeTemperatureReading（真实时间戳、幂等、守护门控）
  → CloudUploadRepository.enqueueAndUpload（幂等入队、异步上传）
  → 卡片/详情/早报
```

已确认的关键事实：

- BLE 设备块前 8 B：`sensor_id(4 B) | humidity_x10(u16be@4) | temperature_x10(s16be@6, 有符号)`；帧长规则 `16*(1+devCount)`；`devCount==0`、长度不足、密文未对齐、解密失败均显式拒绝且不产生读数。
- MQTT 载荷无 ID：`<epoch_seconds>:<temps csv>:<humis csv>`；数目/数值/时间非法时拒绝整批。
- 本地库 v3：`temperature(time, device_id, temperature, humidity, PK(time,device_id))`、`alarm_events`、`pending_uploads(device_id,time,...,attempts,next_attempt_at, PK(device_id,time))` + 索引。
- 上传：`POST {HTTP_BASE_URL}/upload_data`，`phone/registerphone`、`mac/mac_addr`、`readings[{devId,time,temperature,humidity}]`；2xx 视为成功；本地与服务端 `(dev_id,time)` 双重去重。
- 上传门控：守护运行中（`measure_state == "stopping"`）且 `registerphone` 非空。
- 告警去抖 900 s；`>65.0℃` 每次通知；三条报警文案与 `ALARM_TYPE_*` 为兼容面。
- 服务器地址只在 `cloud/CloudConfig.kt` 定义；`network_security_config.xml` 仍保留旧主机明文放行块（历史条目，非连接目标）。
- 当前 BLE 连接目标为常量 `84:C2:E4:03:02:02`，`getRemoteDevice` 后直接 `connectGatt`；无扫描、无绑定、无地址持久化。
- 当前权限门：`SDK >= S` 查 `BLUETOOTH_CONNECT`，否则查 `ACCESS_FINE_LOCATION`；`connectToBluetoothDevice()` 在门失败时只打印一行日志后返回。
- 当前 BLE 分支在遍历本次读数时把 `bletime/bleTempData/bleHumiData` 覆盖为**最后一笔**，供 `connectToBroker` 转发；`isZeroValue` 需连续 3 次才进入处理。
- `MainActivity` 启动只（在 API 33+）请求通知权限；定位权限请求段被注释；`requestBluetoothPermissions()` 仅在定位权限授权回调的分支内被调用，正常流程不可达。
- 设备层未执行项（沿用 ST-002/AF-001）：真机安装/升级、权限弹窗、真实 GATT、MQTT TLS、`/upload_data`、待发箱补传、跨息屏长跑、界面截图、instrumented SQLite 行级。

---

## 4. 设计决策

沿用决策（D-01…D-15 与 AA-002 一致，均已落地为 as-built，不再重做）：

| ID | 决策 |
|---|---|
| D-01 | BLE 记录偏移/字节序/符号不变，只做回归约束 |
| D-02 | 解析、归因、告警、上传载荷为无 Android 依赖的纯逻辑，可宿主机断言 |
| D-03 | App 不重算传感器门控、不推断光照、不合成缺失样本、不把未收到当 0 |
| D-04 | 告警下降判定改为时间窗（5/10/15 min），不按样本序号 |
| D-05 | 告警基准样本来自本地库（`getSamplesInRange`），重启后可判定 |
| D-06 | 入库时间戳为真实采样时间（BLE 接收时刻 / MQTT 载荷首段），无 60 s 回填 |
| D-07 | 上传待发箱以 `(dev_id,time)` 幂等，成功即删，失败有界退避 |
| D-08 | 归因键必须是记录自身 ID；数目不一致拒绝整批，不截断 |
| D-09 | 上传门控 = 守护中且手机号非空 |
| D-10 | 不用数据龄判在线/离线；连通性由链路状态表达 |
| D-11 | BLE 读取周期 180 s，保留手动立即读取 |
| D-12 | 详情图表范围上限 24 h |
| D-13 | 服务器地址集中 `CloudConfig`，其余引用常量 |
| D-14 | `>35.0℃` 仅作呈现强调，App 不据此新增门控；`=35.0` 不强调 |
| D-15 | SQLite v3 仅为增量（新增 outbox 表），既有表结构与语义不动 |

本版新增：

| ID | 决策 | 理由 / 备选否决 |
|---|---|---|
| D-16 | 权限按**连接方式与 API 级别**判定，而不是按“可能的扫描需求”一刀切：API 26–30 直连 GATT 只需清单中的普通权限（安装时授予），**不再要求 `ACCESS_FINE_LOCATION`**；API 31+ 直连需 `BLUETOOTH_CONNECT`；仅当本次流程真的使用 BLE 扫描时，API 26–30 才要求 `ACCESS_FINE_LOCATION` 且需定位总开关开启，API 31+ 才要求 `BLUETOOTH_SCAN` | AF-001 F-1：Android 10 上 `connectGatt` 本身不要求定位权限，阻断完全来自应用自身误判；否决“继续请求定位以沿用旧门”（用户拒绝或无感时轮询静默失效，且剥夺最小权限原则） |
| D-17 | 新增唯一“网关连接身份” `gateway_ble_addr`（冒号 MAC，MyPrefs）：由**有界扫描 + 用户确认**或手动输入产生并持久化；首次升级默认沿用既有 `84:C2:E4:03:02:02` 以保证老用户行为不变，但该默认值只出现在**一处迁移常量**，不得再作为连接路径中的硬编码 | AF-001 F-2 与环境登记“App 发现并连接”要求；否决“继续硬编码”与“用 `mac_addr` 反推 BLE 地址”（topic 身份可能不等于 BLE 地址，未证实前不互相推导） |
| D-18 | `mac_addr` 语义不变（MQTT topic + 上传 `mac`，由注册/绑定流程维护）；`gateway_ble_addr` 只用于 GATT 连接。两者在设置页与连接日志中**同时展示**，不做自动换算 | 不改变既有 topic/schema 与服务器契约；避免把未证实的“同一地址”假设写成规则 |
| D-19 | 权限缺失、蓝牙关闭、无已选网关、扫描无结果都是**一等可观测状态**：卡片/首页横幅 + 明确日志（含原因与所需动作），不得静默返回 | 现状仅一行“Bluetooth permission not granted”，用户无感；D-10 要求不用数据龄表达链路状态，链路状态必须由真实原因填充 |
| D-20 | 一次 BLE 读取的所有设备读数作为**不可拆分母批次**：本地按 ID 逐条处理；向 MQTT 转发只在“批次数 == 当前绑定集合数且 ID 与绑定顺序逐项匹配”时发布完整批次，否则**拒绝转发**并记录原因（本地处理与上传不受影响） | IC-002 D-03：不得只保留最后一笔、不得静默错配；否决“单笔转发 + 只转发首台”（会让其余设备永久无云记录且无法察觉） |
| D-21 | 取消“0.0℃/0.0%RH = 无数据”哨兵：`0.0/0.0` 是合法读数，是否有效只由帧/记录有效性决定 | 设计 §8 原有要求；GXHT40 有效域含 0℃；以数值充当哨兵会延迟或丢弃真实数据 |
| D-22 | App 自身的“温度湿度下降报警”是**独立功能**，保留；上报规则文案只描述传感器“升温 >0.9℃ 且无光 / >35.0℃”，两处措辞不得互相污染 | readme 点 4 的上报规则与 App 报警不是同一语义；避免修复方向时误改用户既有报警能力 |
| D-23 | 本轮所有逻辑改造必须可在宿主机（JVM）断言；需要真实蓝牙/系统 UI 的部分只做接线与状态输出，由测试能力独立验收 | 设计阶段无设备授权；与 AF-001/ST-002 的证据边界一致 |

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
ble/BlePermissionPolicy（本轮新增，纯逻辑） ◄── MainActivity / MqtttService / GatewayPicker
ble/GatewayDiscovery（Android BLE 扫描，本轮新增） ◄── protocol/GatewayIdentity
```

状态所有权（单一所有者）：

| 状态 | 所有者 | 说明 |
|---|---|---|
| 告警参数 | `MyPrefs`（全局）+ `DeviceParamsBook`（每设备覆盖） | 不变 |
| 守护开关 | `HomeViewModel.startData` / `MyPrefs.measure_state` | 不变 |
| 设备展示状态 | `HomeViewModel.devices` | 不变 |
| 样本历史（告警基准与图表） | SQLite `temperature` | 不变 |
| 待上传队列 | SQLite `pending_uploads` | 不变 |
| **网关连接身份** | `MyPrefs.gateway_ble_addr`（本轮新增） | 只用于 GATT |
| **网关平台身份** | `MyPrefs.mac_addr`（不变） | topic + `/upload_data.mac` |
| 链路/权限状态 | `MqtttService` 运行态 → 首页状态区（本轮新增可见性） | 不再由数据龄推导 |

### 5.2 模块清单

已存在（as-built，本轮不改结构）：`protocol/GatewayFrameCodec.kt`、`telemetry/ReadingAttribution.kt`、`telemetry/AlarmEvaluator.kt`、`telemetry/SensorCadence.kt`、`cloud/CloudConfig.kt`、`cloud/UploadPayload.kt`、`cloud/UploadOutbox.kt`、`cloud/CloudUploadRepository.kt`、`ui/detail/DetailPresentation.kt`、`ui/home/DeviceState.kt` 等。

本轮新增：

| 文件 | 职责 | 依赖 |
|---|---|---|
| `ble/BlePermissionPolicy.kt` | 纯逻辑：输入 `sdkInt` 与各权限授予状态、`needScan`，输出“缺失权限列表、是否需要定位总开关、可否直连、用户提示类别”；不含 Android 类型 | 无（可 JVM 测试） |
| `protocol/GatewayIdentity.kt` | 纯逻辑：MAC 规范化（大小写/分隔符/12 位 HEX）、合法性校验、扫描候选去重与排序、已选身份与绑定集合的一致性判断、持久化值到展示文本的格式化 | 无 |
| `ble/GatewayDiscovery.kt` | Android 侧有界扫描（`BluetoothLeScanner`）：优先 `ScanFilter` 服务 UUID `0000ffe0-0000-1000-8000-00805f9b34fb`，无结果时按用户显式操作退化为无过滤扫描；10 s 上限；停止/取消/异常路径必停；产出 `GatewayCandidate(mac, name, rssi, serviceUuids)` | Android BLE |
| `telemetry/BleBatchForwarding.kt` | 纯逻辑：把一次读取的 `List<SensorSample>` 与当前绑定 ID 顺序做匹配，输出 `Forward(batch)` 或 `Refuse(reason)`；构造 MQTT 批量载荷字符串 | 无 |

本轮修改：`MqtttService.kt`（权限门、地址来源、批次转发、零值哨兵、状态输出）、`MainActivity.kt`（按 API 请求权限与拒绝处理）、`ui/gateway/GatewayFragment.kt` 或新增网关选择页（扫描/候选/手输/保存）、`ui/detail/DetailPresentation.kt`（规则文案）、`res/layout`/`res/values`（状态横幅与选择列表）、文档与版本。

---

## 6. 关键流程与状态机

### 6.1 守护启动与权限

```text
用户开启守护（HomeFragment）
  → BlePermissionPolicy.evaluate(sdkInt, granted, needScan=false)
      ├─ canConnect=true  → 启动/恢复 180 s 轮询与 MQTT
      └─ canConnect=false → 首页状态区显示缺失项 + “去授权”
                            → 用户授权后重新评估；拒绝/不再询问 → 跳系统应用设置
  → 若用户进入“选择网关”（需要扫描）→ evaluate(needScan=true)
      → API<31 额外要求 ACCESS_FINE_LOCATION 且定位总开关开启（提示去开启）
      → API31+ 要求 BLUETOOTH_SCAN
```

规则：
- API < 31 的**直连**分支不读取 `ACCESS_FINE_LOCATION`；`isBluetoothBusy()`/`connectToBluetoothDevice()` 使用同一策略对象，避免两处口径漂移。
- 权限检查失败时写入一次可读状态（"蓝牙权限未授予：需要附近设备权限" 等），供首页展示；日志含 `sdkInt`、缺失权限、`needScan`。
- 权限请求只请求清单已声明的权限；`BLUETOOTH_CONNECT/SCAN` 仅在 `SDK >= 31` 请求，`POST_NOTIFICATIONS` 仅在 `SDK >= 33` 请求。
- `MqtttService` 在权限恢复后不重启进程即可继续下一轮（不清空既有库/队列）。

### 6.2 网关发现、绑定与连接

状态：`未配置（仅迁移默认值）→ 扫描中 → 有候选/无候选 → 已保存 → 连接中 → 已连接/连接失败`

1. 读取 `gateway_ble_addr`；为空则使用迁移默认值（既有常量），并在首页标注“使用默认网关地址（未确认）”。
2. 用户点“查找网关”：`GatewayDiscovery.start()`（10 s 上限）→ 候选列表（名称、MAC、RSSI、是否含 FFE0 服务）；无结果时给出“未找到，可再试或手动输入 MAC”，**不自动连接**任何未确认地址。
3. 用户确认候选或手动输入 → `GatewayIdentity.normalize/validate` → 写入 `gateway_ble_addr` → 停止扫描 → 下一轮读取使用该地址。
4. 连接日志输出所使用地址与 `mac_addr`（两者分别标注），便于与网关侧记录对照。
5. 修改/清除身份：设置页可重新选择；清除后回到“未配置”状态，不静默回退到旧常量。

扫描取消/页面销毁/超时/蓝牙关闭/权限撤销 → 必须 `stopScan` 且释放回调；任何异常都不得留下持续扫描。

### 6.3 一次 BLE 读取

```text
读 FFE0/FFE2 → 解密 → GatewayFrameCodec
  → Rejected：记录原因，不产生读数（不转发、不入库、不上传）
  → Success：
      for each sample（含 0.0/0.0）
          processTemperatureHumidityData(devId, T, H)   // 告警、入库、上传、卡片
      BleBatchForwarding.plan(samples, MacIdBook.all())
          → Forward(payload) ：按绑定顺序构造 `now:temps:humis` 并发布
          → Refuse(reason)   ：不发布，记录原因（本地处理已完成）
```

约束：
- 批次不得按“最后一笔”覆盖；不得只转发首台；不得在数目不符时发布部分批次。
- 单设备且 ID 匹配时，载荷字符串与既有格式逐字一致（兼容面）。
- 时间取该次读取的接收 Unix 秒（与 IC-002 §6 一致），不声称是物理采样时刻。

### 6.4 MQTT 与上传

不变：`ReadingAttribution` 严格归因 → 同一处理函数 → 入库/告警 → 待发箱 → `/upload_data`。传输失败只影响上传，不影响读数处理。

---

## 7. 接口与契约

### 7.1 本地偏好键

| 键 | 语义 | 变化 |
|---|---|---|
| `gateway_ble_addr` | GATT 连接目标（冒号 MAC，大写规范化） | 新增 |
| `mac_addr` | 网关平台身份：topic `/<mac>/a/r0` 与 `/upload_data.mac` | 语义不变 |
| `registerphone` / `measure_state` / 报警参数键 | 现状 | 不变 |

迁移：`gateway_ble_addr` 缺失时读取默认常量并视作“未确认”；首次用户选择后写入。不得把 `mac_addr` 自动写入 `gateway_ble_addr`。

### 7.2 权限策略（纯逻辑）

```kotlin
enum class PermissionNeed { BLUETOOTH_CONNECT, BLUETOOTH_SCAN, FINE_LOCATION, POST_NOTIFICATIONS }
data class Policy(
    val canConnect: Boolean,
    val missing: List<PermissionNeed>,
    val requiresLocationServices: Boolean,   // 仅 needScan 且 API<31
    val userMessage: String                  // 面向用户的一句原因与动作
)
fun evaluate(sdkInt: Int, granted: Set<PermissionNeed>, needScan: Boolean): Policy
```

语义：`sdkInt >= 31` 直连缺 `BLUETOOTH_CONNECT` → 不可连接；`sdkInt in 26..30` 直连恒 `canConnect = true`（普通权限安装时授予，仍需适配器开启）；`needScan` 时 `sdkInt >= 31` 需 `BLUETOOTH_SCAN`，`< 31` 需 `FINE_LOCATION` 且 `requiresLocationServices = true`。

### 7.3 网关身份（纯逻辑）

```kotlin
fun normalize(raw: String): String?            // "84:c2:e4:3:2:2" → "84:C2:E4:03:02:02"
fun isValid(raw: String): Boolean
fun rank(candidates: List<Candidate>): List<Candidate>   // 含 FFE0 优先，其次 RSSI 降序，MAC 去重
fun planBinding(selected: String?, boundIds: List<String>): BindingState
```

### 7.4 批次转发（纯逻辑）

```kotlin
sealed interface Plan { data class Forward(val payload: String): Plan; data class Refuse(val reason: RefuseReason): Plan }
fun plan(samples: List<Sample>, boundOrder: List<String>, timeSeconds: Long): Plan
```

拒绝原因至少区分：`NO_BOUND_DEVICE`、`COUNT_MISMATCH`、`UNKNOWN_ID`、`DUPLICATE_ID`、`ORDER_MISMATCH`。

### 7.5 用户可见文本（逐字冻结）

详情页规则说明：

> 采样周期 3 分钟。仅当温度较上次**升高**超过 0.9℃且无光，或温度超过 35.0℃ 时才上报；未满足条件时不会上报，因此可能长时间没有新数据。

- 不得出现“较上次下降超过 0.9℃”的表述；App 不显示光照状态/照度/推断的上报原因。
- App 自身报警面板的“温度湿度下降报警”保留原措辞（D-22）。
- 用户说明书与 App 变更记录同步为“升高”，并注明严格边界（0.9℃ 需超过、35.0℃ 需超过、等于不触发）。

### 7.6 其余接口

SQLite v3 schema、`/upload_data` 请求体、MQTT topic 与载荷格式、BLE 记录布局、AES/证书/UUID/MTU 均不变（见 §9）。

---

## 8. 异常与边界

| 场景 | 设计行为 |
|---|---|
| 直连权限缺失（API 31+） | 不发起连接；首页显示缺失权限与“去授权”；日志含 SDK 与权限名 |
| API<31 且定位权限未授予 | **直连照常**；仅“查找网关”（扫描）被拦并提示 |
| 定位总开关关闭（API<31 扫描） | 扫描前提示开启；不请求无效扫描；不因此阻断直连 |
| 蓝牙适配器关闭 / 未授权 | 状态“蓝牙未开启”；提示开启；轮询不空转刷日志 |
| 未选择网关（仅默认值） | 允许尝试默认地址但首页标注未确认；用户可立即“查找网关” |
| 扫描无结果 / 超时 / 取消 | 停止扫描，显示“未找到网关”，保留手动输入；不自动连接其他设备 |
| 多台候选 | 用户显式选择；不自动挑选，避免误连他人网关 |
| 保存的地址非法或无法 `getRemoteDevice` | 回到“未配置”状态并提示重新选择；不崩溃 |
| BLE 帧被拒绝 | 不产生读数、不转发、不入库、不上传（现状保持） |
| 批次与绑定不符 | 拒绝转发 + 记录原因；本地按 ID 处理与上传仍执行 |
| `0.0℃/0.0%RH` | 视为合法读数，正常判定/入库/上传 |
| MQTT 数目不一致 / 非法值 / 非法时间 | 拒绝整批（现状保持） |
| 上传失败 / 重复上传 | 待发箱保留 + 退避；(dev_id,time) 本地与服务端双重去重（现状保持） |
| 守护关闭 / 未注册手机号 | 不入库（守护关闭）、不入队、不发请求 |
| 长时间无新数据 | 正常（条件上报）；只显示相对时间与提示，不显示“离线” |
| 权限在运行中被撤销 | 下一轮检测到即停止连接并显示状态；恢复后继续，不清数据 |

---

## 9. 兼容面（不得改动）

协议与安全：BLE 记录布局/字节序/符号、帧长规则、AES 密钥与模式、`assets/jd-ca.crt`、`TLSv1.2`、MQTT 用户名/密码拼接、topic 规则（`/<mac>/a/r0|r1|s0`）、MQTT 批量载荷格式、MTU 240、GATT UUID、`0xA1` 写入头与分块、时间校准载荷、`mac_addr` 语义、`/upload_data` 字段与单位、SQLite 既有表结构、65℃ 阈值、900 s 去抖、三条报警文案、`ALARM_TYPE_*`、`MyPrefs` 键名（除新增键）、`MacIdBook`/`DeviceParamsBook` 存储格式与 8 位 HEX 校验。

允许变更：新增 `gateway_ble_addr` 与网关选择 UI/状态展示、权限请求与提示、批次转发内部实现、零值哨兵移除、规则文案、版本号与文档。

---

## 10. 验证策略

### 10.1 宿主机可独立执行（实现阶段必须给出）

| 目标 | 断言要点 |
|---|---|
| 权限策略 | SDK 26/29/30/31/33 矩阵：API<31 直连 `canConnect=true` 且 `missing` 不含定位；`needScan` 时 API<31 需定位、API31+ 需 SCAN；提示文案与缺失项一致 |
| 网关身份 | 规范化/校验（大小写、单双位段、非法长度/字符、空值）；候选排序（FFE0 优先、RSSI、去重）；未选择/已选择状态 |
| 批次转发 | 单设备→与既有载荷逐字一致；多设备按绑定顺序；数量不符/未知 ID/重复 ID 全部 Refuse 且不产出部分载荷；空批次不转发 |
| 零值读数 | `0.0/0.0` 进入正常处理路径，不被门槛拦截 |
| 规则文案 | 含“升高”、不含“较上次下降超过 0.9”；0.9/35.0 严格表述；无光照/照度/lux 与推断原因 |
| 回归 | 现有 188 项宿主机用例全部通过；`assembleDebug` 成功；无新增编译告警 |

### 10.2 静态核查

- 直连路径不再出现 `ACCESS_FINE_LOCATION` 作为 API<31 连接前置（扫描路径除外）。
- `84:C2:E4:03:02:02` 只允许出现在迁移默认常量一处。
- 不存在 `bleTempData = listOf(...)`（最后一笔覆盖）与 `isZeroValue` 哨兵分支。
- 项目内不再存在“较上次下降超过 0.9”的规则文案。
- 服务器地址仍只在 `CloudConfig` 定义。

### 10.3 必须由测试能力在真机独立验收（本设计不执行）

1. Android 10（Redmi 8A / API 29）上开启守护：无定位授权时仍能发起 GATT 连接；不再出现仅一行 `Bluetooth permission not granted` 的静默路径。
2. 网关发现：有界扫描、候选呈现、保存后重连使用已保存地址；扫描必停；蓝牙关闭/无候选的界面状态。
3. API 31+ 设备（或模拟器）上 `BLUETOOTH_CONNECT/SCAN`、通知权限的请求与拒绝路径。
4. 真实 GATT 读取多设备帧：本地逐条入库 + 批次转发与绑定顺序一致；数目不符时拒绝转发。
5. 3 分钟轮询跨息屏/充电/省电长跑、前台服务存活。
6. MQTT TLS、`/upload_data` 幂等、待发箱补传、界面与设备层 SQLite 升级。

设备侧 instrumented 测试的依赖缺失（UTP/无外网）属测试环境问题，不由本设计用宿主机结论替代。

---

## 11. 未决项与交接

| ID | 内容 | 影响与处置 |
|---|---|---|
| U-1 | 网关 BLE 广播内容（是否含 FFE0 服务 UUID、设备名）未证实 | 设计以“FFE0 过滤优先 + 无结果时用户显式无过滤扫描”兼容两种情形；扫描结果与真实网关身份由测试能力对照 |
| U-2 | `mac_addr`（topic/上传）与 BLE MAC 是否同一物理地址未证实 | 设计保持两者独立（D-17/D-18），不互相推导；由真机联调记录确认后另行决定是否合并展示 |
| U-3 | 默认迁移值 `84:C2:E4:03:02:02` 对应的真实网关是否存在/可达 | 保留为兼容默认 + “未确认”标注；用户可通过发现流程替换 |
| U-4 | MIUI 12.5 后台/自启动对 180 s 轮询的影响 | 属真机长跑验收（测试能力）；设计只保证节拍与前台服务声明不变 |
| U-5 | 服务器 `/upload_data`、MySQL 持久化与回读未在当前轮核验 | 不在 Android 设计范围；App 侧按契约编码与幂等重试，云端结论由获准服务入口核验 |
| U-6 | 固件当前判据仍与需求冲突（前值减本次、35.0℃ 整体排除） | 固件责任；App 不变更门控，也不据此调整文案方向 |
| U-7 | 设备端 instrumented/SQLite 行级验证受阻（依赖缺失、无外网） | 交测试能力；不视为 App 逻辑已验 |
| U-8 | 全暗标定、真实温湿度采集、光照证据 | 固件/嵌入式与硬件责任；App 无光照字段，不受影响 |
| U-9 | `artifacts/software_test_design.md`（T-SW-L3-04）与 `artifacts/software_e2e_plan.md` 的用例文字仍写“下降超过 0.9℃” | 属测试能力工件，需由该能力同步为升温；本设计不修改他人工件，且不得据此把 App 文案改回“下降” |

---

## 12. 自检

- [x] 仅覆盖 Android（`dengbei_care`），未改固件/网关/服务器/iOS/硬件
- [x] 上报方向按 readme 与 IC-002 v3 更正为“升温 >0.9℃ 且无光，或 >35.0℃”，严格边界未放宽
- [x] App 仍不重算传感器门控、不推断光照、不合成样本
- [x] 四个缺口均给出可独立检验的功能增量（状态/接口/预期行为），并落到具体文件
- [x] 未把模拟器或宿主机结论当作真机事实；设备层验收明确交测试能力
- [x] 兼容面与不可改动清单完整，含 `mac_addr` 语义与 topic/上传契约
- [x] 未决项保留未决，未以“外部环境”为由删除目标

任务清单见 `artifacts/android_tasks.yaml`。
