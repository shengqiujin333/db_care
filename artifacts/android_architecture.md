# Android App 方案设计（AA-002）

状态：Android 设计候选产物（供后续实现 / 测试能力消费）
版本：1.0
能力：`android_engineer.android_design`
范围：本次「GXHT40 替换温湿度传感器 + 新增光敏分压 + 采集周期改为 3 分钟 + 条件上报（温度较上次下降 >0.9℃ 且无光，或温度 >35.0℃）+ 平台服务器信息同步」改动在 **Android App（`dengbei_care`，Kotlin）** 内的设计。不含 iOS、固件、硬件、服务器端代码。

输入来源：
- `readme.txt`（需求方，权威：6 个修改点）
- `artifacts/interface_contract.md`（IC-002，已批准）：BLE 记录布局、433 帧、MQTT 批量格式、Android 消费边界
- `artifacts/firmware_requirements.md` / `artifacts/firmware_design.md`（FWR-002 / FD-002）：传感器判定式、无周期保活、失败不伪造数据
- `服务器迁移记录.md`（§二/五/八）：新服务器 `8.140.23.253`、EMQX 8883、`abr.py` 5000、`POST /upload_data` 与 MySQL `sensor_data`
- 现有 Android 源码（as-built 事实）与 `dengbei_care/_REFACTOR_NOTES.md`（不可改动清单）

---

## 1. 需求到 Android 的落点

| readme 修改点 | Android 侧落点 | 性质 |
|---|---|---|
| 1 温度传感器改 GXHT40 | 空口/BLE 单位不变（0.1℃ / 0.1 %RH），**解析逻辑不需改**；本设计只做回归约束与死代码清理 | 验证 + 清理 |
| 2 新增光敏电阻与分压 | 光照状态**不上空口、不上 BLE**；App 无法也不得推断/显示光照或 lux | 约束 |
| 3 采集时间改为 3 分钟 | 3 分钟是**传感器采样节拍**：App 不再假设固定数据到点节奏；BLE 读取节拍、判活/新鲜度、图表范围、sparkline 容量、SQLite 采样时间戳全部按此重述 | 设计改动 |
| 4 条件上报（下降 >0.9℃ 且无光 / >35.0℃） | 数据密度大幅下降且**事件驱动**：App 不得合成缺失样本、不得把未收到当作 0、不得重算传感器门控；告警判定改为与采样间隔无关的时间窗；卡片"离线"语义修正；上报规则以文案呈现 | 设计改动 |
| 5 broker 服务器信息 | MQTT `serverUri` 与注册/上传 `baseUrl` 切到新服务器；网络明文放行新增主机；证书与认证规则不变 | 设计改动 |
| 6 同步 Android、不改 iOS | 本设计仅覆盖 Android；iOS 侧无需改动（线上格式未变），作为交接登记 | 范围约束 |

---

## 2. As-built 事实（本次设计必须尊重的现状）

工程：`dengbei_care`（Kotlin，AGP 8.3.2 / Gradle 8.4 / JDK 21 可构建；`compileSdk 34`、`minSdk 26`、`versionCode 8`、`versionName 1.7`）。

数据链路（两条入口，汇聚到同一处理函数）：
1. **BLE 路径**：`MqtttService.startBluetoothTask()`（`READ_INTERVAL_MS = 5 * 60 * 1000`）→ `connectToBluetoothDevice()`（5 次重试、2 s 间隔）→ `connectAndReadData()`（GATT FFE0/FFE2 读、MTU 240、AES/ECB/NoPadding 解密）→ `decryptAndParseEcbFrame()` → `parsePlainFrame()` → `interpretPayloadV3`/`DeviceReading` → `processTemperatureHumidityData()`。
2. **MQTT 路径**：`subscribeToTopic(rtopic0 = "/$macAddr/a/r0")` → `messageArrived()` 解析 `time:temp1,...:humi1,...` → 按 `MacIdBook.all()` 顺序位置归因 → `processTemperatureHumidityData(devId, t, h, time)`。

已确认的关键事实：
- BLE 记录布局（`parsePlainFrame`）：头块 16 B = 网关 ID(6 B) + devCount(1 B)；其后每设备一块 16 B，**前 8 B 有效** = `sensor_id(4 B, BE)` + `humidity_x10(u16be@4)` + `temperature_x10(s16be@6, 有符号)`。与 IC-002 §4 一致，**无需修改**。
- MQTT 批量载荷**不含 device ID**，只能按 `MacIdBook.all()` 顺序归因；现实现用 `minOf(...)` 静默截断。
- 本地持久化：`TemperatureDatabaseHelper`（`DATABASE_VERSION = 2`），表 `temperature(time, device_id, temperature, humidity, PRIMARY KEY(time, device_id))` + `alarm_events(...)`；写入经 `storeTemperatureData(...)`，其中**按 60 s 回填历史时间戳**：`time = timenow - (size-1-i)*60`。
- 守护门控：`HomeViewModel.startData`（`measure_state`/`finish_guard_time` 持久化）为 1 时才 `storeTemperatureData`（`storeflag == 1`）。
- 告警：`processTemperatureHumidityData()` 用内存 `DeviceHistory` 的 `data5MinAgo/data10MinAgo/data15MinAgo`（实为**最近第 1/2/3 个样本**，不是时间窗）× `tempdrop/humdrop` + `tempmax/tempmin/hummax/hummin`；`>65℃` 紧急报警；通知/震动/铃声按设备 900 s 去抖。报警文案与类型（`drop` / `threshold` / `emergency`）已落 `alarm_events`。
- 展示：`DeviceCardAdapter.bindStatusChip()` 在 `!state.isOnline()`（>30 min 无数据）时显示 **"离线"**；`DeviceState.OFFLINE_THRESHOLD_MS = 30 min`；`HomeViewModel.MAX_SPARKLINE_SIZE = 30`（注释按 5 分钟间隔估算）；`DeviceDetailFragment` 图表范围硬限制 **50 分钟**；`report/` 生成"早起报告"（`DailyStats` + `AlarmEvent`）。
- 网络与密钥：`MqtttService.serverUri = "ssl://117.72.84.210:8883"`（**旧 IP**）；`ZhuCeViewModel.baseUrl = "http://117.72.84.210:5000"`（**旧 IP**）；`res/xml/network_security_config.xml` 仅放行 `117.72.84.210` 与 `192.168.4.1`；`ApiService` 只有 5 个注册/登录端点，**无 `/upload_data`**。
- `MqtttService.parseHexData()`（小端 `humidity | temp` 解释）**无任何有效调用点**（仅存在于注释掉的旧代码），是与 IC-002 冲突的残留解释。

> 注意：`服务器迁移记录.md` §八描述的 App 改动（`serverUri` 切新 IP、`ApiService` 新增 `/upload_data`、`ZhuCeViewModel` 改 baseUrl、明文放行新 IP）在当前仓库代码中**并不存在**。本设计按"待实现"处理，避免重复实现或误判已完成。

---

## 3. 设计决策（Decision Log）

| ID | 决策 | 依据 / 理由 | 备选与否决 |
|---|---|---|---|
| D-01 | **不修改** BLE 帧解析的字节偏移与符号（`id(4) | hum u16be@4 | temp s16be@6`），只做回归约束 | IC-002 §4/§5；固件 FWR-105 保证布局不变 | 否决"重新定义记录顺序"——会破坏已验证的空口/网关兼容性 |
| D-02 | 把帧解析、归因、告警判定、上传载荷构造提取为**不依赖 Android 的纯 Kotlin 逻辑**（`protocol/`、`telemetry/`、`cloud/`），`MqtttService` 只做编排 | 本能力工具面是 `gradle` + `unit_test`，纯逻辑可在宿主机 JUnit 下独立验证，避免真机依赖；也降低 1580 行 Service 的改动风险 | 否决"直接在 Service 内改逻辑"——不可独立检查 |
| D-03 | App **不重算**传感器条件门控（下降 >0.9℃ / 无光 / >35.0℃），**不推断光照**，**不合成**缺失样本，**不**把未收到视为 0 | IC-002 §2/§5；光照未上线上、App 无此信息 | 否决"在 App 复现 0.9℃+无光判据"——光照不可得，且会产生与传感器不一致的第二套判定 |
| D-04 | 告警判定从"最近第 N 个样本"改为**基于时间戳的时间窗**（5/10/15 min），并对基准样本设上界回溯容差；基准缺失时该窗口不成立 | 3 分钟条件上报后样本稀疏，按样本序号比较已无时间含义 | 否决"按固定采样间隔×N 直接换算"——条件上报下样本间隔不固定 |
| D-05 | 判定基准样本从**本地库**读取（`[now-W-容差, now-W]`），内存历史仅作缓存 | 服务重启/进程被杀后仍可判定；单一数据源 | 否决"仅内存 DeviceHistory"——重启即失效 |
| D-06 | 本地入库时间戳使用**该读数真实时间**（BLE 用接收时刻、MQTT 用 payload `parts[0]`），**移除 60 s 回填** | 稀疏事件驱动数据下回填会造成错误时间点（并与 `PRIMARY KEY(time, device_id)` 去重交互） | 否决"保留 60 s 回填"——会产生虚假采样时刻 |
| D-07 | 新增**上传待发箱（outbox）表**，`PRIMARY KEY(dev_id, time)`，成功即删、失败有界重试/退避；依赖服务端 `UNIQUE(dev_id, time)` 实现端到端幂等 | 迁移记录 §八要求即时上传；移动网络下必须可重试且不重复 | 否决"直接 fire-and-forget 单次调用"——断网即丢，且无重试可见性 |
| D-08 | 云上传的**归属键是 BLE 记录自身的 4 B sensor_id**（MQTT 批量路径按 `MacIdBook` 顺序，但**数目不一致时拒绝整批**而非截断） | IC-002 §4/§5：禁止用列表顺序替代 ID、禁止静默错归因 | 否决"沿用 minOf 截断"——会把 A 的读数记为 B |
| D-09 | 上传门控与本地存储门控一致（守护中才入队上传），且**手机号为空时不上传也不入队** | 与现状守护开关语义一致；避免无归属用户的隐私数据外发 | 否决"无条件上传所有读数"——破坏守护开关语义与隐私边界；列为未决项 U-2 |
| D-10 | 卡片状态不再用 30 min 数据龄显示"离线"，改为**相对时间 + 条件上报提示**；连接/断网状态仍由既有 `_mqtt_broker_state` 表达 | 条件上报下长时间无数据是**正常产品行为**，显示"离线"是错误信息 | 否决"把 30 min 改成更大常数"——仍是把数据龄当连通性；保活心跳属固件新需求（FWR-OPEN-3） |
| D-11 | BLE 读取周期常量与采样节拍对齐为 **3 min**（`SENSOR_SAMPLE_PERIOD`），保留手动"立刻读"（`wakeReadNow`） | 显示新鲜度不应系统性落后一个采样周期以上；读取周期是展示/轮询参数，不改协议 | 否决"保持 5 min"——展示最坏延迟 >1 个采样周期 |
| D-12 | 设备详情图表范围上限由 50 min 放宽到 **24 h**（与报警列表一致），并提示"按条件上报，数据点可能稀疏" | 50 min 窗口在新上报规则下经常为空，图表基本不可用 | 否决"保持 50 min"——功能实际失效 |
| D-13 | 服务器地址集中在**单一常量对象**（`cloud/CloudConfig.kt`）并保留 `jd-ca.crt`、`TLSv1.2`、用户名/密码拼接规则、AES 密钥、UUID、MTU、topic 规则不变 | `_REFACTOR_NOTES.md` 不可改动清单；迁移记录 §五"除 IP 外无需改动" | 否决"散落硬编码新 IP"——避免再次出现"记录说改了、代码没改"的漂移 |
| D-14 | `>35.0℃` 在 App 侧只做**呈现强调**（高温样式/文案），不新增门控逻辑；`=35.0℃` 不视为"超过" | readme 点 4 的"超过 35 度"为严格大于；App 已有 `tempmax=35` 的超限报警 | 否决"新增 App 侧 35℃ 上报判定"——违反 D-03 |
| D-15 | SQLite 升 `DATABASE_VERSION = 3`：保留 `temperature` / `alarm_events` 结构不动，仅新增 outbox 表与索引；迁移失败回退策略沿用现状（重建） | 现状 v1→v2 迁移已有回退分支；本次不改既有列语义 | 否决"在 temperature 表加 uploaded 列"——会改动既有主键语义与历史行含义 |

---

## 4. 目标架构

### 4.1 分层与状态归属

```text
[Transport]                                  [Pure logic]                     [Domain / State]          [Persistence]         [Cloud]
MqtttService(BLE GATT FFE0/FFE2) ─┐
                                  ├─► protocol/GatewayFrameCodec  ─┐
MqtttService(MQTT messageArrived)─┘   telemetry/ReadingAttribution─┤
                                                                   ├─► telemetry/AlarmEvaluator ─► DeviceState(HomeViewModel)─► UI(卡片/详情/通知/报告)
                                                                   ├─► 采样时间戳 ────────────────► TemperatureDatabaseHelper(temperature, alarm_events)
                                                                   └─► 归属后的读数 ─────────────► cloud/UploadOutbox ─► cloud/CloudUploadRepository ─► ApiService POST /upload_data
```

状态所有权（单一所有者，避免重复真相）：
- **告警参数**：`MyPrefs` 扁平 key（全局）+ `DeviceParamsBook`（每设备覆盖）——现状不变，App 只读。
- **守护开关**：`HomeViewModel.startData`（`measure_state` / `finish_guard_time` 持久化）。
- **设备展示状态**：`HomeViewModel.devices: LiveData<Map<devId, DeviceState>>`。
- **样本历史（判定与展示基准）**：本地 SQLite `temperature` 表（时间序、按 devId 归属）。
- **待上传队列**：SQLite outbox 表（唯一持久化队列，进程重启不丢）。
- **连通性状态**：`NotificationsViewModel._mqtt_broker_state`（MQTT/BLE 链路），**不再**由数据龄推导。

### 4.2 模块清单（新增 / 修改）

新增（纯逻辑，无 Android 依赖，可 host 单测）：
| 文件 | 职责 |
|---|---|
| `app/src/main/java/com/jinyuni/dengbei_care/protocol/GatewayFrameCodec.kt` | BLE 聚合帧解码：头块校验、devCount、每设备 8 B → `SensorSample(devIdHex, humidityPct, temperatureC)`；提供 `u16be`/`s16be`/ID 格式化（含 `devIdHex` 去分隔符） |
| `app/src/main/java/com/jinyuni/dengbei_care/telemetry/ReadingAttribution.kt` | MQTT 批量载荷 → `List<AttributedReading>`：严格长度校验（数目不一致→拒绝整批）、按 `MacIdBook` 顺序映射、空/非法数值处理 |
| `app/src/main/java/com/jinyuni/dengbei_care/telemetry/AlarmEvaluator.kt` | 纯函数判定：输入 `(devId, now, temp, humi, params, historySamples)` → `AlarmOutcome(message, severity, type)`；时间窗下降 + 阈值超限 + 紧急（>65℃）；无基准/无样本时返回 NORMAL |
| `app/src/main/java/com/jinyuni/dengbei_care/cloud/CloudConfig.kt` | 唯一服务器常量：`MQTT_BROKER_URI`、`HTTP_BASE_URL`、时间/重试常量；替代散落硬编码 |
| `app/src/main/java/com/jinyuni/dengbei_care/cloud/UploadPayload.kt` | `/upload_data` 请求体构造（`phone`、`mac`、`readings[{devId,time,temperature,humidity}]`），单位与字段名契约 |
| `app/src/main/java/com/jinyuni/dengbei_care/cloud/UploadOutbox.kt` | outbox 表 DAO：`enqueue`（幂等去重）、`peekBatch`、`ack`(删)、`markFailed`（次数/下次重试时间） |
| `app/src/main/java/com/jinyuni/dengbei_care/cloud/CloudUploadRepository.kt` | 上传编排：守护开关/手机号门控、批大小、超时、失败退避、成功后删除；接口化依赖（Api + Outbox）便于独立检查 |

修改：
| 文件 | 改动 |
|---|---|
| `MqtttService.kt` | `serverUri` 取 `CloudConfig.MQTT_BROKER_URI`；BLE 读取周期改 `SENSOR_SAMPLE_PERIOD`；解析改调 `GatewayFrameCodec`；MQTT 归因改调 `ReadingAttribution`（拒绝整批）；`processTemperatureHumidityData()` 改调 `AlarmEvaluator` + 真实采样时间戳入库 + 触发上传；**删除**无调用点且与 IC-002 冲突的 `parseHexData()`；`DeviceHistory` 降级为缓存（判定基准来自库查询） |
| `TemperatureDatabaseHelper.kt` | `DATABASE_VERSION = 3`；新增 outbox 表 + 索引；新增 `storeTemperatureReading(devId, time, temp, humi, storeflag)`（单条、真实时间戳）；保留既有查询 API；`getRecentSamples(devId, fromTime, toTime)` 供判定基准 |
| `DeviceState.kt` | 移除"30 min ⇒ 离线"语义，改为 `lastReportAgeLabel()` + 条件上报提示所需的字段/常量（`SENSOR_SAMPLE_PERIOD_SEC = 180` 引用） |
| `ui/home/DeviceCardAdapter.kt` + `res/layout/item_device_card.xml` | 状态 chip：未收到 / 正常 / 警告 / 紧急 + 最近上报相对时间；不再出现"离线" |
| `ui/detail/DeviceDetailFragment.kt` + `res/layout/fragment_device_detail.xml` + `res/values/strings.xml` | 采样周期与条件上报规则说明区；图表范围上限 24 h；高温（>35.0℃）呈现强调 |
| `ui/zhuce/ApiService.kt` | 新增 `POST /upload_data` + `SensorReading` / `SensorUploadData` 数据类（既有 5 个端点签名不动） |
| `ui/zhuce/ZhuCeViewModel.kt` | `baseUrl` 改用 `CloudConfig.HTTP_BASE_URL`（不再硬编码旧 IP） |
| `res/xml/network_security_config.xml` | 新增放行 `8.140.23.253` 明文（保留既有两项） |
| `report/ReportGenerator.kt` / `report/DailyReportWorker.kt` | 报告文案补充"按条件上报，采样条数非等间隔"；0 样本时明确"昨日无上报"而非"无数据"歧义 |
| `app/build.gradle.kts` | `versionCode` / `versionName` 递增 |
| `dengbei_care/readme.txt`、`dengbei_care/用户使用说明书.md` | 变更记录与新上报规则/3 分钟采集的用户说明 |

---

## 5. 接口设计

### 5.1 BLE → App（保持不变，仅回归约束）
- 头块 16 B：`gwId(6)` + `devCount@6`；`expected = 16 * (1 + devCount)`，不足即拒绝该帧（不上报、不落 0）。
- 每设备块 16 B，取前 8 B：`sensor_id(4 B 原字节顺序)` → 8 位大写 HEX 无分隔（作为 `MacIdBook` 键）；`humidity_x10 = u16be@4`；`temperature_x10 = s16be@6`（**负温必须保留符号**）；换算 `/10.0`。
- 解密：`AES/ECB/NoPadding` + 16 B 密钥（不变）；解密后按 `devCount` 截断（不变）。
- 不得出现第二套解析（小端）入口。

### 5.2 MQTT → App（归因严格化）
- 载荷格式不变：`<epoch_seconds>:<t1,t2,...>:<h1,h2,...>`。
- 解析失败/空列表：丢弃该批，不落库、不上传、不报警。
- `savedIds.size != temps.size || temps.size != humis.size` → **拒绝整批**并记录日志（现状为截断，须改）。
- 归因得到的 `devId` 与读数一起向下游传递；**下游任何环节不得再依赖列表顺序**。

### 5.3 规则呈现（App 侧）
- 只读展示"采样周期 3 分钟；仅当温度较上次下降超过 0.9℃且无光，或温度超过 35.0℃ 时才上报"。
- 明确不显示光照/无光/lux，不显示"因下降 x℃ 上报"之类的推断结论（D-03/D-14）。
- 用户配置的 App 报警参数（下降阈值、上下限、延时、震动/铃声）继续生效，语义见 §7。

### 5.4 本地 SQLite（v3）
```text
temperature(time INTEGER, device_id TEXT, temperature REAL, humidity REAL, PRIMARY KEY(time, device_id))   -- 结构不变
alarm_events(id INTEGER PK AUTOINCREMENT, time, device_id, type, message)                                  -- 结构不变
pending_uploads(dev_id TEXT, time INTEGER, temperature REAL, humidity REAL,
                attempts INTEGER DEFAULT 0, next_attempt_at INTEGER DEFAULT 0,
                PRIMARY KEY(dev_id, time))                                                                 -- 新增
INDEX idx_pending_uploads_next ON pending_uploads(next_attempt_at)
```
- 写入：单条读数使用**真实采样时间**；`INSERT OR REPLACE` 保证 `(dev_id,time)` 幂等。
- 迁移：`oldVersion < 3` 时仅 `CREATE TABLE IF NOT EXISTS pending_uploads` + 建索引；失败按现状回退（记录日志，不崩溃）。
- 清理：`temperature` 保留策略现状不变（无自动删除）；outbox 成功即删，失败记录保留且有界重试。

### 5.5 `/upload_data`（云上传）
- 端点：`POST {HTTP_BASE_URL}/upload_data`，`Content-Type: application/json`。
- 请求体：`{"phone":"<registerphone>","mac":"<mac_addr 网关MAC>","readings":[{"devId":"A1B2C3D4","time":1699999999,"temperature":25.5,"humidity":60.0}]}`
- 单位：`temperature` ℃（`x10/10.0`）、`humidity` %RH（`x10/10.0`）、`time` Unix 秒。
- 归属：`devId` 来自 §5.1/§5.2 的归属结果，非列表下标。
- 幂等：服务端 `UNIQUE(dev_id, time)` + `INSERT IGNORE`；App 侧 outbox 也以 `(dev_id,time)` 去重 ⇒ 重试/补传不产生重复。
- 门控（D-09）：仅守护中（`startData == 1`）且 `registerphone` 非空时入队；否则不入队、不请求。
- 重试：单批上限（设计初值 50 条）、网络失败指数退避（1 min 起步、上限 30 min）、`attempts` 上限（设计初值 20 次）后保留在 outbox 等待手动/下次触发；仅当 HTTP 2xx 视为成功并删除。
- 失败不阻塞 BLE/MQTT 主链路：上传在独立协程作用域执行。

### 5.6 服务器迁移（readme 点 5）
| 项 | 现值（as-built） | 目标 |
|---|---|---|
| MQTT broker | `ssl://117.72.84.210:8883` | `ssl://8.140.23.253:8883` |
| 注册/上传 HTTP | `http://117.72.84.210:5000` | `http://8.140.23.253:5000` |
| 明文放行 | `117.72.84.210`、`192.168.4.1` | 追加 `8.140.23.253` |
| TLS 信任锚 / 认证 / AES / UUID / MTU / topic | 现状 | **不变**（`assets/jd-ca.crt`、`TLSv1.2`、用户名=MAC、密码=MAC+`&^!A:z?`） |

---

## 6. 生命周期与并发

- `MqtttService` 前台服务：BLE 轮询循环（3 min + 手动唤醒）与 MQTT 订阅并存；`serviceScope`(IO) 承担上报编排；新增上传使用**独立 SupervisorJob 子作用域**，单次失败不影响循环。
- 上传触发点：① 新读数入 outbox 后立即尝试；② 每次 BLE 读取轮次开始；③ 网络恢复（`connectToBroker` 成功回调）；④ App 冷启动（`MainActivity.onCreate`）。
- 同一 `(dev_id,time)` 的并发入队由 SQLite 主键保证不重复；同一批上传由单 worker 串行（`Mutex`/单线程 dispatcher），避免重复请求。
- 守护状态切换（`START_STOP_GUARD`）：即刻停止新入队；已入队未发送的记录仍可发送（已产生数据），不因关闭守护而丢失。
- 报告 `DailyReportWorker`（WorkManager）与 `alarm_events` 查询保持现状，仅文案调整。

---

## 7. 告警展示设计

判定输入与次序（沿用现状次序，避免行为漂移）：
1. `AlarmEvaluator` 产出 `未设置报警 / 温度湿度下降报警 / 温湿度超限报警 / 无报警`。
2. 紧急：`temperature > 65.0` → `紧急报警，温度超过65度，谨防火灾`（每次通知，不受去抖限制）。
3. 落库：`drop`/`threshold`/`emergency` → `alarm_events`（`NORMAL`/`未设置报警` 不落库）。
4. 通知/震动/铃声：按设备 900 s 去抖（`dengbeialarm`/`yuzhialarm`/`zhendongalarm`/`xianglingalarm` 开关不变）。

时间窗下降判定（替代"最近第 N 个样本"，D-04/D-05）：
```text
对 W ∈ {5, 10, 15} 分钟:
  baseline = 库中该 devId 满足 (time <= now - W) 且 (time >= now - W - LOOKBACK_SLACK) 的最新样本
  if baseline 存在 and (baseline.temp - cur.temp) > tempdrop and (baseline.humi - cur.humi) > humiDrop
       → 温度湿度下降报警（WARNING）
若无任何窗口成立 → 不产生下降报警
```
- `LOOKBACK_SLACK` 设计初值 30 min（避免与远古样本比较）；常量集中于一处。
- 无样本/无基准：不报警、不落 0、不推进历史（与固件 FWR-107 的"失败不污染状态"精神一致）。
- 阈值超限判定（`tempmax/tempmin/hummax/hummin`）与紧急判定**保持现状语义**。
- 文案字面值、65℃ 阈值、900 s 去抖、报警类型常量均为兼容面，**不得改**（`_REFACTOR_NOTES.md`）。

展示面：
- 卡片（`item_device_card.xml`）：温度/湿度（未收到显示 `--`）、状态 chip（未收到/正常/警告/紧急）、最近上报相对时间、条件上报提示、sparkline（容量按 3 min 节拍重新表述，保留 `MAX_SPARKLINE_SIZE` 数量上限）。
- 详情（`fragment_device_detail.xml`）：图表（范围上限 24 h）、报警事件列表（24 h）、规则说明 + 高温强调。
- 通知（`sendAlarmNotification`）：文案与标题现状不变，仍含设备名与报警文案。
- 早起报告：`DailyStats`/`AlarmEvent` 现状结构不变，文案补充"按条件上报"。

---

## 8. 异常与边界

| 场景 | 设计行为 |
|---|---|
| BLE 帧长度不足 / 解密失败 | 丢弃该帧，记录日志；不落库、不上传、不报警（现状保持） |
| BLE 读到 0/0 | 现状为连续 3 次才处理；**改为不再以 0 值作为有效读数下发**（0 ℃/0 %RH 是合法物理值，不能作为"无数据"哨兵），仅按帧有效性判断 |
| MQTT 数目不一致 | 拒绝整批（D-08） |
| MQTT 数值解析失败 | 丢弃该批 |
| 读数温度/湿度越界（>125.0℃、<0 %RH 等） | 记录日志并丢弃该条，不入库/不上传（GXHT40 有效域见 FWR-101） |
| 上传失败 | outbox 保留 + 退避重试；对 UI 不弹错（可在详情页显示"待同步 N 条"作为非阻塞提示） |
| 重复上传 | 服务端 `UNIQUE(dev_id,time)` + App outbox 双保险 |
| 手机号为空 | 不入队、不上传（D-09） |
| 守护关闭 | 不入库、不入队；已上传历史不回溯删除 |
| 时钟偏差 | 以 payload `parts[0]`（网关时间）优先，缺失时用系统时间；不做时间戳改写 |
| 长时间无数据 | 属正常（条件上报）；卡片只显示相对时间，不显示"离线" |

---

## 9. 兼容面（不得改动清单）

协议与安全：BLE 帧/记录布局与字节序、AES 密钥与模式、`jd-ca.crt`、`TLSv1.2`、MQTT 用户名/密码拼接、topic 规则（`/$macAddr/a/r0|r1|s0`）、MTU 240、GATT UUID、`0xA1` 写入头、时间校准 payload 模板、65℃ 阈值、900 s 去抖、报警文案字面值、`MyPrefs` key 名、`MacIdBook`/`DeviceParamsBook` 存储格式与校验规则（8 位 HEX）、`ApiService` 既有 5 个端点签名与数据类字段。

允许变更：仅 IP/端口相关常量、新增 `/upload_data` 端点与数据类、新增 outbox 表与纯逻辑模块、展示文案与布局、读取周期/图表范围等展示参数。

---

## 10. 验证策略（实现阶段可独立检查）

- 宿主机单元测试（`app/src/test`，JUnit4，`./gradlew :app:testDebugUnitTest` 已验证可离线运行）覆盖纯逻辑：帧解码（正/负温度、devCount、长度不足、字节序）、归因（数目一致/不一致/空）、告警时间窗（边界与"无基准"）、上传载荷构造（字段名/单位/幂等键）、`CloudConfig` 常量断言。
- 构建检查：`./gradlew :app:assembleDebug`（含资源合并，可捕获 `network_security_config.xml` 语法问题）。
- 静态检查：代码中不再存在 `117.72.84.210`；不再存在 `parseHexData` 调用；不再存在 `* 60L` 回填。
- 真机 GUI / 端到端（连接真实网关、真实 broker、云端落库）属 Android 测试能力，不在本设计阶段执行。

---

## 11. 任务分解

见 `artifacts/android_tasks.yaml`（按前置依赖排序，供 `android_engineer.android_implementation` 消费）。任务顺序：纯逻辑基础（协议/归因/告警/上传载荷）→ 服务器常量迁移 → 持久化（v3 + outbox + 真实时间戳）→ 上传链路 → Service 编排接入 → UI 呈现 → 文档与版本。

---

## 12. 未决项与交接

| ID | 内容 | 处置 |
|---|---|---|
| U-1 | 服务器端 `/upload_data`、`sensor_data` 表与 `UNIQUE(dev_id,time)` 是否已在目标环境生效（迁移记录称已部署，但 App 侧代码缺失） | 实现阶段按契约编码；端到端验证由测试能力在真实环境确认；若服务端未就绪，App 侧失败退避不阻塞主链路 |
| U-2 | 云上传是否应覆盖"守护关闭"期间收到的读数 | 本设计取 D-09（不入队）。若产品要求全量上云，属新需求，须需求方确认后调整一处门控 |
| U-3 | App 侧"温度湿度下降报警"在传感器已按 0.9℃ 门控上报后是否仍有意义（可能出现"每条上报都触发下降报警"） | 本设计保留该用户可配功能并改造为时间窗判定，不删除用户可见能力；是否退役/调整默认阈值须产品确认 |
| U-4 | 是否需要设备"保活/心跳"以支持在线判定 | 固件侧明确无周期保活（FWR-OPEN-3）；本设计改为不依赖数据龄判在线 |
| U-5 | 光照阈值实板标定（IC-002 §7-1） | 与 App 无关（光照不上线）；若未来上线光照字段，需重新评估 BLE 记录布局与 App 解析 |
| U-6 | iOS 侧 | 线上格式未变，本轮不改；作为跨角色交接登记（readme 点 6） |
| U-7 | `服务器迁移记录.md` §八与仓库代码不一致（记录称 App 已改，实际未改） | 本设计按待实现处理；建议后续由文档责任方纠正，避免再次误判 |

---

## 13. 自检

- [x] 仅覆盖 Android（`dengbei_care`），未改固件/硬件/iOS/服务器代码
- [x] 未改变产品需求（3 分钟采集、0.9℃/无光/35℃ 条件上报、服务器信息同步均为 readme 原文）
- [x] 未在 App 侧重算传感器门控、未推断光照、未合成样本
- [x] 明确列出兼容面与不可改动清单
- [x] 每个改动点均指向具体文件与行为，可由实现能力独立检查
- [x] 阻塞/未决项已登记，未静默自行决定产品语义
