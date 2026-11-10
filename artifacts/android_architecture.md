# Android App 方案设计（AA-005）

状态：Android 方案设计（供实现与测试消费；不代表任何设备、网关、云端或真机行为已验证）
版本：4.0（替代 AA-004 rev 3.0；AA-004 的六项增量继续保留，本版新增一项容量/完整读取增量，并同步 IC-002 v4.0、当前现场光条件与当前后端证据）
责任角色：Android 工程
范围：仅 Android App（`dengbei_care`，Kotlin）。不含固件、网关、服务器端代码、iOS 与硬件改动。
工程基线：Git HEAD `f81ab04`（接手时 `git status --short` 为空）；源码 `versionCode 9 / versionName 1.8`、`minSdk 26`、`targetSdk 34`、`compileSdk 34`。
检查时点：2026-11-10（Asia/Shanghai）。

本版工作方式：只做源码与工件静态核对。**本能力当前没有可用的设备工具**（`android_app_testing`、`android_readonly_inspection`、`sensor_serial_observation` 等入口对本能力均为 `unavailable_for_current_capability`，`tool_profile` 为空），因此**未安装、未连接手机、未扫描/连接网关、未构建、未运行 Gradle 测试、未写任何数据**。文中所有“当前”均指本次读取的本地源码文本，不是设备行为。

代码定位方法：本次会话未暴露可调用的 `codegraph_explore` MCP 工具，仓库根也无 `.codegraph/` 索引目录；因此按上游索引登记的目标文件，使用限定目录的原生 Grep/Read 核对 Android 关键路径与符号。未建立或升级索引，未伪造图查询结果。网关源码事实沿用 AA-004/IC-002 已核对结论，本次未重读网关文件（已在 §3.2 标注）。

输入来源（本版实际读取）：
- `readme.txt`（需求方，权威：GXHT40、光敏、180 s、升温 >0.9℃ 且无光或 >35℃、服务器资料、只改 Android、当前有光、UART1；不安排调温/改变光强/功耗测试）
- `artifacts/system_analysis_current.md`（当前需求冲突、冻结边界、Android 六项增量仍未完成、上电日志与跨链路阶段划分）
- `artifacts/interface_contract.md`（IC-002 **v4.0**：判据与边界、BLE 布局、**160 B/最多 9 设备完整读取**、占位与合法 0.0、MQTT 三段格式、`/upload_data`、时间语义、错误分类沿用 AA-004）
- `artifacts/backend_feasibility_current.md`（BE-EXP-004：2026-11-10 15:47 只读复核，`abr.py` 未变、6 路由、EMQX 5.8.0、0 连接、POST 仍 14 条且末次 2026-10-22 401/401/200）
- `artifacts/firmware_design.md`（FD-002 rev 8：LIGHT-DEF-2 绝对阈值形制，`T=0` 即未启用，候选 3590 未启用；不启用时升温分支不触发，仅 >35℃ 分支可用）
- `artifacts/firmware_requirements.md` / `artifacts/firmware_tasks.yaml`（FWR-002 rev 8 / 6 项任务：本阶段不安排调温/调光/功耗实验）
- `artifacts/android_feasibility.md`（AF-001：Redmi 8A / Android 10 / API 29 只读事实、F-1 权限门、F-2 网关身份、AVD API 34 对照）
- `artifacts/android_implementation.md`（AIMPL-002：既有 14 项实现记录与已落地模块，作 as-built 历史复用依据）
- `artifacts/software_test_design.md`（TD-SW-004 rev 3.0：现有 G-1…G-6 验收用例 T-SW-L1/L2/L3/REG 编号）
- 当前 Android 源码、网关源码（`CH592EVT/EVT/EXAM/BLE/beiwov2`）、`file_manifest.txt`、最近提交

---

## 0. 本版变更（优先于以下历史描述）

1. **AA-004 的六项增量经本次逐条源码复核仍然存在、仍未实现**（`ble/` 包不存在；`protocol/GatewayIdentity.kt`、`telemetry/BleBatchForwarding.kt`、`telemetry/BleObservation.kt`、`cloud/UploadResult.kt` 均不存在）：
   - G-1 权限门：`MqtttService.kt:552–561` 在 API<31 仍以 `ACCESS_FINE_LOCATION` 作为**直连**前置；`connectToBluetoothDevice()` 在权限缺失时仅一行日志后返回（`:582–586`）；`MainActivity.kt:62–77` 的启动请求段仍被注释，`:219–226` 的 `requestBluetoothPermissions()` 只在定位授权回调（`:267–331`）内可达，且通知授权回调会转而请求定位（`:237–265`）。
   - G-2 网关身份：硬编码常量仍在 `MqtttService.kt:261`，连接目标仍为 `getRemoteDevice(deviceMacAddress)`（`:598`）；全仓无 BLE 扫描实现。`ui/gateway/GatewayFragment.kt` 是既有的 **Wi-Fi/ESP 配网**页面（使用 `ACCESS_FINE_LOCATION`/`NEARBY_WIFI_DEVICES` 扫描 Wi-Fi），**不是**网关 BLE 发现入口，不得复用为网关身份发现。
   - G-3 批次转发：`bleTempData/bleHumiData/bletime/bletimebkp`（`:248–251`）仍在每次读数时覆盖为“最后一笔”（`:743–752`）；发布串仍为 `time + ',' + "humi...,temp..."`（`:1281–1292`）。
   - G-4 规则文案：`DetailPresentation.kt:26–30`、`res/layout/fragment_device_detail.xml:52` 仍为“较上次**下降**超过 0.9℃”；`telemetry/SensorCadence.kt:7` 注释同样写“下降”。
   - G-5 占位/零值：`isZeroValue` 与全局 `bleZeroCount >= 3` 仍在（`:255`、`:743–762`）。
   - G-6 上传分类：`CloudUploadRepository.UploadApi` 仍只返回 `Boolean`，`RetrofitUploadApi` 仍是 `isSuccessful`，非 2xx 统一退避（`CloudUploadRepository.kt`）；`CloudApiClient` 无拦截器注入点。
2. **新增 G-7（本版唯一新增功能增量）：绑定容量与读取完整性。** IC-002 v4.0 §6 明确：FFE2 属性 160 B、设备块 16 B，**完整快照最多 9 个设备**（16+9×16=160；10 个需 176 B）；`MAX_DEV_COUNT=255` 只是构帧上限。Android 必须在绑定与读取两处显式拒绝超出容量，**不得截断或少报后冒称整批**；读取时必须确认实际返回密文覆盖 `16×(1+N)` 全部设备块，不足即整帧拒绝，**不补零、不跨时刻拼接、不裁掉余字节**。
   - 当前源码事实：`MacIdBook` 无数量上限（`MacIdBook.kt`），`NotificationsFragment.kt:91–99` 添加传感器只有“长度 8 + 非空名称”校验；`buildAllSavedIdsA1SinglePacket(..., maxCount = 50)`（`MqtttService.kt:414–418`）按 50 截断，与 150 B `sensorres` 的 9 设备容量不一致；读路径虽已由 `GatewayFrameCodec` 对 `plain < 16×(1+N)` 拒绝，但**没有**容量前置、没有返回字节数/MTU 充足性判定，也没有可读的“读取不完整”状态。
3. **契约注释同步并入 G-4**（IC-002 v4.0 §7 要求 Android 同步）：`ui/zhuce/ApiService.kt` 的 `SensorReading.time` 注释仍写“真实采样时间”，应改为“BLE 接收 Unix 秒或 MQTT 载荷首段秒；不是物理采样时刻”；`SensorCadence.kt` 的“下降”注释改为“升温”。
4. **后端证据升级为 BE-EXP-004**：`abr.py` 8949 B/mtime 未变、6 条 POST 路由存在、EMQX 5.8.0/0 连接、POST 仍 14 条且末次为 2026-10-22 的 401/401/200，之后无新业务请求。结论不变：**401 机制仍未定性**，App 不猜测鉴权；D-27/D-28 保持，`/upload_data` 的 2xx 只表示 HTTP 受理，不表示持久化。
5. **现场光条件与固件判据状态（影响数据预期，不改 Android 门控）**：用户声明“当前有光”；固件 FD-002 rev 8 的 LIGHT-DEF-2 为 `dark = valid && mean >= T`，当前 `T=0` 表示**未启用**（`valid=false`、`dark=false`），因此**升温分支当前不会触发，只有 `>35.0℃` 分支可能上报**。Android 侧因此必须做到：
   - 不重算、不绕过、不补偿该门控（D-03）；不因“长时间无数据”合成读数或改写入库；
   - 界面不显示光照状态/照度，也不出现“因无光/因升温上报”之类推断（光照不上空口、不上 BLE）；
   - 规则文案仍按需求原文保留“且无光”和“>35.0℃”，不加固件内部状态说明；
   - 真机验收**不得**通过调温或改变光强来制造升温/无光条件；当前阶段只能验证“无新数据属正常”的呈现与链路状态，升温分支、真实占位、真实 0.0/0.0 在无可控真实源时记 NOT EXECUTED（不判 PASS，也不据此退回实现）。
6. **DESIGN_READY 的含义不变**：仅表示方案可供实现。真机权限/发现/GATT、真实 MTU/返回长度、真实 `/upload_data` 与 401 定性、持久化回读均未执行，见 §10.3 与 §11。

---

## 1. 本次设计结论

### 1.1 可复用的 as-built（既有 14 项实现，宿主机层已验，设备层未验）

| 能力 | 实现位置 | 证据边界 |
|---|---|---|
| BLE 聚合帧唯一解码（`id4 \| hum u16be@4 \| temp s16be@6`），`devCount=0`/长度不足/密文未对齐显式拒绝 | `protocol/GatewayFrameCodec.kt` | 宿主机单测 + 历史构建；真实 GATT/MTU 未验 |
| MQTT 批量载荷严格归因（数目不一致整批拒绝） | `telemetry/ReadingAttribution.kt` | 同上；不解决“数目相同但顺序错”（无 ID 载荷的固有限制） |
| 告警时间窗（5/10/15 min，基准来自库） | `telemetry/AlarmEvaluator.kt` + Service 接线 | 同上 |
| SQLite v3（`temperature`、`alarm_events`、`pending_uploads`） | `TemperatureDatabaseHelper.kt` | schema/语句级；真机迁移未验 |
| 单条读数写入（真实接收/载荷时间，`INSERT OR REPLACE` 幂等） | `storeTemperatureReading(...)` | 语句级；真机行级未验 |
| 云上传待发箱与编排（幂等、退避、门控、串行、独立作用域） | `cloud/UploadOutbox.kt`、`cloud/CloudUploadRepository.kt`、`cloud/UploadPayload.kt` | 宿主伪实现；真机 SQLite/HTTP 未验 |
| 服务器常量单一来源（`8.140.23.253`，8883/5000） | `cloud/CloudConfig.kt` | 静态；TLS/上传可达性未验 |
| 卡片不以数据龄判“离线” | `DeviceState.kt`、`DeviceCardAdapter.kt` | 逻辑层；界面未验 |
| 详情 24 h 范围、高温强调、规则说明 | `ui/detail/DetailPresentation.kt`、`DeviceDetailFragment.kt` | 逻辑层；界面未验 |
| 早报“按条件上报/非等间隔/昨日无上报” | `report/ReportGenerator.kt` | 文本层；通知展示未验 |
| 3 分钟读取节拍（180 s） | `telemetry/SensorCadence.kt` | 常量断言；真机长跑未验 |
| 守护门控（`measure_state == "stopping"` + `registerphone`） | `PrefsUploadGate` | 逻辑层；真机未验 |

### 1.2 本轮必须补齐的七项（源码确认仍存在）

| 编号 | 缺口 | 当前证据（本次检查时点） |
|---|---|---|
| G-1 | 权限模型与请求流程不一致：API 29 直连被自身要求定位权限，正常启动流程从不请求 | 见 §0.1 G-1 |
| G-2 | 网关身份硬编码、无发现/绑定；`mac_addr` 与 BLE 地址互不关联 | 见 §0.1 G-2 |
| G-3 | BLE 批次只保留最后一笔，且转发载荷分隔符/字段顺序与契约不符 | 见 §0.1 G-3 |
| G-4 | 用户可见规则文案方向相反；契约时间语义与采样节拍注释未同步 | 见 §0.1 G-4 |
| G-5 | 网关全零占位被当读数；有效 ID 的 0.0/0.0 被“连续 3 次”门槛延迟/丢弃 | 见 §0.1 G-5 |
| G-6 | 上传结果不分类：401/403/4xx 与瞬时失败同路，最多 20 次后静默滞留 | 见 §0.1 G-6 |
| G-7 | 绑定无 ≤9 容量限制；读路径无返回字节/MTU 充足性判定与“读取不完整”状态 | 见 §0.2 |

### 1.3 为什么这七项不能只靠测试或环境解决

- G-1/G-2 是应用自身代码路径的确定性缺陷（不依赖外部条件即可由源码证明）。
- G-3/G-5 是数据正确性缺陷：多设备静默错配/丢失、无数据占位被写成真实读数。
- G-6 是错误可观测性与数据保留缺陷；鉴权机制本身属未决，必须由端到端阶段定性。
- G-4 是用户可见规则与产品需求相反（注释/文档同步属契约一致性）。
- G-7 是协议容量约束：超出 9 台时物理上无法完整传输，截断会制造“看似成功”的错误数据；MTU 不足时部分密文被解码将直接产出错误快照。
- 七项均可通过纯逻辑 + 接线改造消除，并可在宿主机独立断言；真实弹窗、扫描、GATT、MTU、401 与落库仍需测试能力在真机/获准服务入口独立验收（§10.3）。

---

## 2. 需求到 Android 的落点

| readme / 契约要求 | Android 侧落点 | 当前状态 |
|---|---|---|
| 1 GXHT40 替换温湿度器件 | 单位（0.1℃/0.1 %RH）与 BLE 布局不变，解析逻辑不改；只做回归约束 | 已实现 |
| 2 光敏电阻与分压 | 光照不上空口、不上 BLE；App 不推断、不显示光照/照度 | 已实现（无光照字段） |
| 3 采集时间改为 3 分钟 | App 读取周期对齐 180 s；不得把“无上报”当异常 | 已实现（`SensorCadence`、卡片/详情/早报文案） |
| 4 升温 >0.9℃ 且无光，或严格 >35.0℃ 才上报 | App 不重算门控、不推断光照、不合成样本；规则文案方向与严格边界一致 | 门控实现正确；**文案方向错误（G-4）** |
| 5 平台 broker/服务器信息 | MQTT `ssl://8.140.23.253:8883`、HTTP `http://8.140.23.253:5000`、明文放行、`POST /upload_data` | 已实现；401 机制未定性（G-6） |
| 6 只改 Android，不改 iOS；光通信/霍尔退役 | 仅 Android；线上格式不变；不引入光照/原因字段 | 已实现 |
| 网关由 App 发现并连接 | 必须有发现/绑定/持久化身份，不得硬编码地址 | **未实现（G-2）** |
| 真机 Android 10 / API 29 | 权限按 API 与连接方式分流，连接路径不得依赖未请求的权限 | **未实现（G-1）** |
| IC-002 §6：160 B / 最多 9 设备 / 完整读取 | 绑定与读取双处容量保护；不足即整帧拒绝并有可读状态 | **未实现（G-7）** |
| IC-002 §4.2、§6：占位与合法 0.0 | 全零占位是链路状态；有效 ID 的 0.0/0.0 是合法读数 | **未实现（G-5）** |
| IC-002 §8：失败分类/可观测/手动重试 | 2xx / 401-403 / 其他 4xx / 可重试四分类；不丢行、不猜鉴权 | **未实现（G-6）** |
| 服务器复用、端到端真实上传/幂等/回读 | App 侧按既有契约编码；真实写入与回读在端到端阶段执行 | 契约已实现；运行期未证 |

---

## 3. As-built 事实（本设计必须尊重的现状）

### 3.1 数据链路

```text
BLE：GATT FFE0/FFE2 读 → AES/ECB 解密 → GatewayFrameCodec（按 4 B ID 归因）
MQTT：/<mac>/a/r0 → ReadingAttribution（按 MacIdBook 顺序，数目不一致拒绝整批）
        ↓（两条入口汇聚）
processTemperatureHumidityData(devId, T, H[, 时间])
  → AlarmEvaluator（时间窗下降 + 阈值 + >65℃ 紧急）
  → alarm_events 落库 / 通知（900 s 去抖，紧急不节流）
  → storeTemperatureReading（接收时间、幂等、守护门控）
  → CloudUploadRepository.enqueueAndUpload（幂等入队、异步上传）
  → 卡片/详情/早报
```

### 3.2 已确认的关键事实（本次逐条读取）

- **BLE 设备块**：前 8 B 为 `sensor_id(4 B) | humidity_x10(u16be@4) | temperature_x10(s16be@6, 有符号)`；帧长规则 `16*(1+devCount)`；`devCount==0`、长度不足、密文未对齐、解密失败均显式拒绝且不产生读数（`GatewayFrameCodec.kt`）。`SensorSample` 目前只有 `humidityPct/temperatureC`，**没有 x10 整数字段**。
- **容量**：FFE2 属性 160 B、网关 `sensorres` 150 B（IC-002 §6），完整快照 ≤9 设备；App 侧无任何容量约束（`MacIdBook` 无上限，`0xA1` 包按 50 截断）。
- **网关缓存是“最后值寄存器”**（沿用 AA-004/IC-002 已核对的源码结论，本次未重读网关文件）：`sensorres[]` 只在收到并成功解码 433 帧时按绑定顺序更新（`app_um2006A.c:180–196`），BLE 读不消费/不清除；`decode_frame10` 失败时 `timeoutcnt` 递减（成功置 100），耗尽后整体 `memset` 为 0 并下发（`:217–226`）。因此 App 每 180 s 读到的是“当前缓存值”，可能重复、也可能是全零占位。
- **网关广播**（同上，源码结论而非空中抓包）：`advertData` 含 16-bit 服务 UUID `0xFFE0`；扫描响应本地名为 `"xxx"`；GAP 设备名为 `"JYN Peripheral"`。
- **MQTT 载荷无 ID**：`<epoch_seconds>:<temps csv>:<humis csv>`；数目/数值/时间非法时拒绝整批（`ReadingAttribution.kt`）。现有实现检查数目，不解决“数目相同但顺序错”。
- **App 自发布路径**：当 `mac_addr` 非空且不以 `1010` 开头时，App 在 MQTT 连接成功时把最后一笔 BLE 读数发布到 `/<mac>/a/r0`；否则订阅该 topic（`MqtttService.kt:1281–1297`）。本设计不改 topic 路由，只改载荷内容/完整性与发布时机。
- **绑定顺序**：`MacIdBook.all()` 按 `name → id` 排序（`MacIdBook.kt`），与 `0xA1` 配置包同一顺序；`sanitizeId12` 实际要求 **8** 位 HEX。
- **本地库 v3**：`temperature(time, device_id, temperature, humidity, PK(time,device_id))`、`alarm_events`、`pending_uploads(...)` + `next_attempt_at` 索引。
- **上传**：`POST {HTTP_BASE_URL}/upload_data`，`phone/mac/readings[{devId,time,temperature,humidity}]`；任何 2xx 视为成功；单批 50；退避 60→1800 s、最多 20 次，达上限保留行不再自动重试。
- **上传门控**：守护运行中（`measure_state == "stopping"`）且 `registerphone` 非空。
- **告警**：下降判定用 5/10/15 min 时间窗，基准取自库且允许 30 min 回溯；`>65.0℃` 紧急通知每次触发；其他报警 900 s 去抖；三条文案与 `ALARM_TYPE_*` 为兼容面。
- **BLE 连接**：MTU 请求 240（`MqtttService.kt:667`），协商结果落到 `negotiatedMtu`（`:675–676`）但**只用于日志**；随后 `readCharacteristic`（`:688`），`onCharacteristicRead` 直接取 `characteristic.value` 解密，**未校验返回字节数**（长度不足时由 codec 拒绝）。
- **权限门**：`SDK >= S` 查 `BLUETOOTH_CONNECT`，否则查 `ACCESS_FINE_LOCATION`；门失败打印一行后返回；`isBluetoothBusy()` 在权限缺失时返回 `true`（视为忙）。
- **BLE 零值**：`isZeroValue = (T == 0.0 && H == 0.0)`，连续 3 次才进入处理；计数为全局单值，非按设备。
- **启动权限请求**：正常流程只在隐私政策同意后请求通知权限（API 33+）；定位/蓝牙请求段被注释；`requestBluetoothPermissions()` 仅在定位授权回调内可达。
- **规则文案**：`DetailPresentation.REPORT_RULE_TEXT` 与布局 `tools:text` 均为“下降”；`SensorCadence` 注释亦为“下降”。
- **`GatewayFragment` 是 Wi-Fi 配网页**：使用 ESP Provisioning + Wi-Fi 扫描，权限为 `NEARBY_WIFI_DEVICES`（API 33+）或 `ACCESS_FINE_LOCATION`（API<33）。**因此 `ACCESS_FINE_LOCATION` 仍需保留在清单**；G-1 只要求删除“BLE 直连的前置依赖”，不是删除权限声明。
- **设备层未执行项**（沿用 AF-001 / TD-SW-004）：真机安装/升级、权限弹窗、真实 GATT/MTU、MQTT TLS、`/upload_data`、待发箱补传、跨息屏长跑、界面截图、instrumented SQLite 行级。

### 3.3 当前上游事实对 Android 的含义

| 事实 | 来源 | Android 含义 |
|---|---|---|
| 现场**有光**；LIGHT-DEF-2 未启用（`T=0`）→ 升温分支不触发 | readme 7；FD-002 rev 8 §0.4 | App 不感知、不显示、不补偿；只保留需求文案与“可能长时间没有新数据”的提示；本阶段不得要求调温/改变光强验证升温分支 |
| 严格 >0.9℃ 在 x10 下最低为升温 1.0℃；`>35.0` 严格；`=35.0` 不触发 | IC-002 §3.1 | 文案表述必须保持严格边界；App 不加门控计算 |
| 完整快照 ≤9 设备（160 B 物理上界） | IC-002 §6 | 绑定与读取双处拒绝超容量；不截断、不部分转发（G-7） |
| 密文返回必须覆盖 `16×(1+N)`；MTU 23 不足 | IC-002 §6 | 校验协商 MTU 与返回字节数；不足→整帧拒绝 + 可读状态；不补零/不拼接（G-7） |
| 全零占位 vs 合法 0.0/0.0；不用连续 3 次门槛 | IC-002 §6 | G-5 语义直接落地 |
| MQTT 生产者只发 3 段，接收端数目/非法值整批拒绝；顺序错无法自检 | IC-002 §7 | G-3 生产者按绑定顺序构造完整批次；顺序错的风险登记为未决 |
| 上传：2xx=受理≠持久化；401/403 与其他 4xx 暂停自动重试；5xx/408/429/IO 可重试 | IC-002 §8（沿用 AA-004） | G-6 四分类；`inserted` 字段不建模、不新增鉴权 |
| 服务部署元数据未变、仍 0 业务流量、10-22 401/401/200 | BE-EXP-004 §9 | 不据历史推断上传可用；真实受理/幂等/回读属端到端 |

---

## 4. 设计决策

沿用决策（D-01…D-15，已落地为 as-built，不重做）：D-01 BLE 布局/字节序/符号不变；D-02 纯逻辑可宿主机断言；D-03 不重算门控/不推断光照/不合成样本；D-04 告警时间窗；D-05 告警基准来自库；D-06 入库时间=真实接收（BLE）/载荷首段（MQTT），不声称物理采样时刻；D-07 待发箱 `(dev_id,time)` 幂等；D-08 归因键必须是记录自身 ID；D-09 上传门控；D-10 不用数据龄判离线；D-11 180 s 读取节拍；D-12 详情 24 h；D-13 服务器常量集中；D-14 `>35.0℃` 仅呈现强调；D-15 SQLite 仅增量。

AA-003/AA-004 引入、本版保留：D-16 权限按连接方式与 API 判定（API<31 直连不要求定位；API31+ 直连需 `BLUETOOTH_CONNECT`；仅扫描时 API<31 需定位+总开关、API31+ 需 `BLUETOOTH_SCAN`）；D-17 唯一 `gateway_ble_addr`（有界扫描+用户确认或手输，迁移默认仅一处常量，清除后不回退）；D-18 `mac_addr` 语义不变，与 BLE 地址互不换算；D-19 权限/蓝牙/未选网关/扫描无结果均为一等可观测状态；D-20 一次读取为不可拆分母批次（本地按 ID 处理；转发仅当数量/ID/顺序全部匹配）；D-21 取消“0.0/0.0=无数据”哨兵；D-22 上报规则文案与 App 自身“下降报警”措辞互不污染；D-23 改造必须可宿主机断言；D-24 观测按接收时刻记录、不按值去重；D-25 全零占位是链路状态；D-26 MQTT 载荷逐字 `time:temps:humis`，`Locale.US`；D-27 上传四分类 + 全局阻塞状态 + 手动重试；D-28 不猜鉴权，只留唯一空拦截器注入点；D-29 发现以 FFE0 过滤为主、显式无过滤回退，10 s 上限必停；D-30 观测语义与界面措辞（“最近读取/更新”，接收时间≠采样时刻）。

本版新增：

| ID | 决策 | 理由 / 备选否决 |
|---|---|---|
| D-31 | **容量硬上限 = 9 台**：新增纯逻辑 `protocol/GatewayReadGuard.kt`（或等价命名）暴露 `MAX_DEVICES = 9`、`MAX_FRAME_BYTES = 160`、`expectedFrameSize(n)`、`capacityVerdict(boundCount)`、`mtuVerdict(negotiatedMtu, devCount)`、`lengthVerdict(returnedBytes, devCount)`。绑定入口（`MacIdBook.add` 的唯一调用方/绑定 UI）在已达 9 台时拒绝新增并给出明确提示；读路径在 `boundCount > 9` 时**不发起读取**并置状态。 | IC-002 §6 的物理上界（160 B 属性）。否决“截断到前 9 台”：会造成第 10 台起永久静默缺失且用户以为已绑定；否决“按 50 台继续写入 `0xA1`”：网关 150 B 缓存无法容纳，帧与绑定必然不一致。 |
| D-32 | **读取完整性以“协商 MTU + 实际返回字节数”双重判定**：仅当 `negotiatedMtu >= 16×(1+N) + 3` 才发起读；`onCharacteristicRead` 记录实际 `value.size`，要求 `value.size >= 16×(1+N)` 且为 16 的整数倍；不满足则整帧拒绝、置状态“读取不完整（MTU/长度不足）”，**不补零、不拼接跨次快照、不裁掉余字节**。MTU 协商失败时先重连一次并重试协商；仍失败则本轮记失败，不无限重试。 | IC-002 §6 明确“默认 MTU 23 不能证明足够、需确认实际返回长度”。现有 `negotiatedMtu` 只写日志、读路径不看返回长度；部分密文若恰好 16 字节对齐（例如 16 B 头块）会被解码为 `LENGTH_MISMATCH`，虽然不会产出读数，但用户与日志都无法区分“帧坏”与“传输不足”。 |
| D-33 | **当前阶段的数据预期写入设计而非产品文案**：在文档/测试说明中记录“现场有光 + 判据未启用 ⇒ 升温分支不触发，只有 >35℃ 可能上报”，避免把“长时间无新数据”误判为 App/链路故障；**不**在 App 界面加入固件判据状态或光照推断。 | 需求 7 与 FD-002 rev 8 §0.4 的直接后果。否决“在 App 里显示‘光照判据未启用’”：该状态不上线、会随固件演进而过期，且把内部实现暴露给用户；也否决“临时放宽上报条件”：篡改产品需求。 |
| D-34 | **契约注释与文档同步属功能一致性**：`ApiService.SensorReading.time` 注释改为“BLE 接收 Unix 秒 / MQTT 载荷首段秒；不是物理采样时刻”；`SensorCadence` 注释改为“升温 >0.9℃ 且无光，或 >35℃”；`RESERVED` 不新增协议字段。 | IC-002 §7 明确要求 Android 同步该注释；注释是后续实现/测试判断时间语义的唯一就地依据，错误注释已导致一次口径漂移。 |

---

## 5. 目标架构

### 5.1 分层与状态归属

```text
[Transport]                                     [Pure logic]                          [Domain/State]         [Persistence]        [Cloud]
MqtttService(BLE GATT FFE0/FFE2) ─┐
                                  ├─► protocol/GatewayFrameCodec ─┐
MqtttService(MQTT messageArrived)─┘   telemetry/ReadingAttribution┤
                                  ├─► protocol/GatewayReadGuard ──┤
                                  │     （容量/MTU/返回长度）      │
                                                                  ├─► telemetry/AlarmEvaluator ─► DeviceState/UI ─► TemperatureDatabaseHelper ─► cloud/UploadOutbox ─► CloudUploadRepository
                                                                  ├─► telemetry/BleBatchForwarding（本轮新增）
                                                                  └─► protocol/GatewayIdentity（本轮新增）
ble/BlePermissionPolicy（本轮新增，纯逻辑） ◄── MainActivity / MqtttService / 网关设置区
ble/GatewayDiscovery（Android BLE 扫描，本轮新增） ◄── protocol/GatewayIdentity
cloud/UploadResult（本轮新增，纯逻辑分类） ◄── CloudUploadRepository / RetrofitUploadApi
```

状态所有权（单一所有者）：

| 状态 | 所有者 | 说明 |
|---|---|---|
| 告警参数 | `MyPrefs`（全局）+ `DeviceParamsBook`（每设备覆盖） | 不变 |
| 守护开关 | `HomeViewModel.startData` / `MyPrefs.measure_state` | 不变 |
| 设备展示状态 | `HomeViewModel.devices` | 不变；新增链路/上传状态字段（§6.6、§7.7） |
| 样本历史（告警基准与图表） | SQLite `temperature` | 不变；每次成功读取按接收时间写入 |
| 待上传队列 | SQLite `pending_uploads` | 不变 |
| **网关连接身份** | `MyPrefs.gateway_ble_addr`（新增） | 只用于 GATT |
| **网关平台身份** | `MyPrefs.mac_addr`（不变） | topic + `/upload_data.mac` |
| **链路/权限/读取完整性状态** | `MqtttService` 运行态 → 首页状态区 | 新增可见性；不由数据龄推导 |
| **上传阻塞状态** | `MyPrefs`（`upload_blocked_reason`、`upload_blocked_at`、`upload_last_status`）+ `NotificationsViewModel` | 新增；不改 outbox 表结构 |

### 5.2 模块清单

已存在（as-built，不改结构）：`protocol/GatewayFrameCodec.kt`、`telemetry/ReadingAttribution.kt`、`telemetry/AlarmEvaluator.kt`、`telemetry/SensorCadence.kt`、`cloud/CloudConfig.kt`、`cloud/UploadPayload.kt`、`cloud/UploadOutbox.kt`、`cloud/CloudUploadRepository.kt`、`ui/detail/DetailPresentation.kt`、`ui/home/*`、`MacIdBook.kt`、`TemperatureDatabaseHelper.kt`。

本轮新增：

| 文件 | 职责 | 依赖 |
|---|---|---|
| `ble/BlePermissionPolicy.kt` | 纯逻辑：`sdkInt`、授予集合、`needScan`、适配器/定位开关 → `canConnect/canScan/missing/reason/userMessage` | 无（JVM 可测） |
| `protocol/GatewayIdentity.kt` | 纯逻辑：MAC 规范化/校验、候选去重排序、绑定状态、迁移默认判定 | 无 |
| `ble/GatewayDiscovery.kt` | Android 有界扫描（FFE0 过滤优先、无过滤回退，10 s 上限，所有结束路径 `stopScan`），产出 `GatewayCandidate(mac,name,rssi,serviceUuids)` | Android BLE |
| `telemetry/BleBatchForwarding.kt` | 纯逻辑：读数 + 绑定顺序 → `Forward(payload)` / `Refuse(reason)`；按 D-26 构造载荷 | 无 |
| `telemetry/BleObservation.kt` | 纯逻辑：占位判定（x10 整数）、有效读数归一 `{devId,tX10,hX10}` | 无 |
| `protocol/GatewayReadGuard.kt` | 纯逻辑：容量（≤9）与读取完整性（MTU/返回字节/期望帧长）判定 | 无 |
| `cloud/UploadResult.kt` | 纯逻辑：HTTP 状态/异常 → `Success/AuthRejected/RequestRejected/Retryable` | 无 |

本轮修改：`MqtttService.kt`（权限门、地址来源、读前容量/MTU 校验与返回长度校验、批次转发、占位与零值、上传结果处理、状态输出、发布时机）、`MainActivity.kt`（按 API 请求权限与拒绝处理）、`ui/notifications/NotificationsFragment.kt`（网关 BLE 发现入口、`gateway_ble_addr` 展示/清除、上传状态与手动重试、绑定数量提示）、`ui/home/HomeFragment.kt` + `HomeViewModel.kt`（链路状态区）、`ui/detail/DetailPresentation.kt`（规则文案）、`ui/zhuce/ApiService.kt`（仅时间注释）、`cloud/CloudUploadRepository.kt` + `CloudApiClient`（结果分类与拦截器注入点）、`telemetry/SensorCadence.kt`（注释）、`res/layout`/`res/values`（状态区与候选列表）、文档与版本。

---

## 6. 关键流程与状态机

### 6.1 守护启动与权限

```text
用户开启守护（HomeFragment）
  → BlePermissionPolicy.evaluate(sdkInt, granted, needScan=false, adapterEnabled, locationEnabled)
      ├─ canConnect=true  → 启动/恢复 180 s 轮询与 MQTT
      └─ canConnect=false → 首页状态区显示缺失项 + “去授权”
                            → 用户授权后重新评估；拒绝/不再询问 → 跳系统应用设置
  → 进入“选择网关”（需要扫描）→ evaluate(needScan=true)
      → API<31 额外要求 ACCESS_FINE_LOCATION 且定位总开关开启
      → API31+ 要求 BLUETOOTH_SCAN
```

规则：
- API<31 的**直连**分支不读取 `ACCESS_FINE_LOCATION`；`checkBluetoothPermission()`、`isBluetoothBusy()`、`connectToBluetoothDevice()` 共用同一策略结果，避免口径漂移。
- `ACCESS_FINE_LOCATION` **继续保留在 Manifest**（既有 Wi-Fi 配网页 `GatewayFragment` 仍需要），只是不再作为 BLE 直连前置。
- 权限检查失败写入一次可读链路状态（原因与动作）；日志含 `sdkInt`、缺失权限、`needScan`、`adapterEnabled`；不存在“仅一行 `Bluetooth permission not granted` 后返回”的路径。
- 权限请求只请求清单已声明的权限：`BLUETOOTH_CONNECT/SCAN` 仅 `SDK>=31`，`POST_NOTIFICATIONS` 仅 `SDK>=33`，定位仅 API<31 的扫描流程；不出现不可达分支与“拒绝后无限重复请求”。
- 权限恢复后不重启进程即可继续下一轮；不清空既有库/队列。

### 6.2 网关发现、绑定与连接

状态：`未配置（仅迁移默认值）→ 扫描中 → 有候选/无候选 → 已保存 → 连接中 → 已连接/连接失败`

1. 读取 `gateway_ble_addr`；为空且未被显式清除时使用迁移默认（既有常量），界面标注“使用默认网关地址（未确认）”。
2. “搜索附近网关”→ `GatewayDiscovery.start()`（10 s 上限）→ 候选列表（名称、MAC、RSSI、是否含 FFE0）；无结果给出“未找到 / 扩大搜索（不过滤）/ 手动输入 MAC”，**不自动连接**。
3. 用户确认候选或手输 → `GatewayIdentity.normalize/validate` → 写入 `gateway_ble_addr` → 停止扫描 → 下一轮使用。
4. 连接日志同条输出 `gateway_ble_addr` 与 `mac_addr`，分别标注“BLE 连接目标 / 平台身份”，不做换算。
5. 清除后写“已清除”标记，不回退默认值。
6. 扫描取消/页面销毁/超时/蓝牙关闭/权限撤销 → 必须 `stopScan` 并释放回调。

### 6.3 一次 BLE 读取（含容量与完整性门）

```text
绑定集合 = MacIdBook.all()（顺序即网关 0xA1/回传顺序）
  → GatewayReadGuard.capacityVerdict(boundCount)
        ├─ EXCEEDED（>9）→ 不发起读取；状态“已绑定 N 台，超过网关完整传输上限 9 台”；不写入 0xA1
        └─ OK → 连接 → 协商 MTU（请求 240）
              → GatewayReadGuard.mtuVerdict(mtu, devCount=N)
                    ├─ MTU 不足（< expected+3）→ 不读；重连重试一次；仍不足 → “读取不完整（MTU=x）”
                    └─ 足够 → readCharacteristic
                          → onCharacteristicRead：记录 value.size
                              → GatewayReadGuard.lengthVerdict(value.size, N)
                              ├─ size < expected 或 size % 16 != 0 → 整帧拒绝；“读取不完整（返回 y/需 z 字节）”
                              └─ 通过 → 解密 → GatewayFrameCodec
                                    ├─ Rejected → 记录原因，不产生读数
                                    └─ Success：
                                        for each sample：
                                            BleObservation.classify(sample)
                                              ├─ Placeholder（8 B 全零）→ 记录“网关暂无数据”，跳过
                                              └─ Reading → processTemperatureHumidityData(devId,T,H,接收时间)
                                        BleBatchForwarding.plan(readings, boundOrder, 接收时间)
                                            → Forward(payload) / Refuse(reason)
```

约束：
- 批次不得按“最后一笔”覆盖；不得只转发首台；不得在数目/ID/顺序不符时发布部分批次。
- 占位块不参与本地处理与转发；整批占位 → “网关暂无传感器数据”，不发布。
- 单设备且 ID 匹配时载荷与既有格式逐字一致（`<time>:<temp>:<humi>`）。
- 时间取该次读取的接收 Unix 秒，不声称物理采样时刻。
- 读取完整性失败不产生任何读数、不写库、不入队、不转发；**不得**用上次成功快照或补零继续。

### 6.4 MQTT 转发时机

- 每次成功读取得到 `Forward` 后：MQTT 已连接 → 立即发布；未连接 → 只保留**最近一批** `pendingForward`（有界为 1），连接成功回调中发布一次并清空。
- 发布失败：记录日志与状态，不阻塞本地处理；下一批成功读取覆盖 `pendingForward`。
- 兼容：保留既有 topic 选择分支（`mac_addr` 非空且不以 `1010` 开头时发布到 `/<mac>/a/r0`，否则订阅）。

### 6.5 本地入库与上传

不变：`ReadingAttribution` 严格归因 → 同一处理函数 → 入库/告警 → 待发箱 → `/upload_data`。

新增：
- 每次成功读取（非占位）按接收时间入库并进入上传编排（D-24，不去重）。
- 上传结果按 D-27 分类：`Success` 逐行 ack；`Retryable` 逐行 `markFailed(backoff)`；`AuthRejected`/`RequestRejected` **不**进入自动退避，保留全部行、置全局阻塞状态（原因码/HTTP 状态/时间/待发条数），设置页展示并提供“重试上传”；手动重试清除阻塞、恢复被暂停的到期行（批量 UPDATE，不改表结构）并立即上传一次。
- 上传阻塞不得影响 BLE/MQTT/告警/入库；不得删除本地行。

### 6.6 链路状态机（用户可见）

```text
UNKNOWN → (评估) → PERMISSION_MISSING | BLUETOOTH_OFF | GATEWAY_UNSET | GATEWAY_DEFAULT_UNCONFIRMED
        → CONNECTING → CONNECTED → (读取成功) → DATA_OK | GATEWAY_NO_DATA（全零占位）
                                   → (读取失败/超时/长度不足) → CONNECT_FAILED | READ_INCOMPLETE
        → BINDING_CAPACITY_EXCEEDED（已绑定 >9 台，不读取）
        → (权限撤销/蓝牙关闭) → 对应状态
```

状态文本只陈述可观测原因，不使用数据龄；`GATEWAY_NO_DATA`、`READ_INCOMPLETE` 与“连接失败”“设备离线”分开。

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

迁移：`gateway_ble_addr` 缺失且 `gateway_ble_addr_cleared=false` 时读取默认常量并视作“未确认”；首次用户选择后写入；不得把 `mac_addr` 自动写入 `gateway_ble_addr`。

### 7.2 权限策略（纯逻辑）

```kotlin
enum class PermissionNeed { BLUETOOTH_CONNECT, BLUETOOTH_SCAN, FINE_LOCATION, POST_NOTIFICATIONS }

data class Policy(
    val canConnect: Boolean,
    val canScan: Boolean,
    val missing: List<PermissionNeed>,
    val requiresLocationServices: Boolean,   // 仅 needScan 且 API<31
    val reason: LinkReason,
    val userMessage: String
)

fun evaluate(sdkInt: Int, granted: Set<PermissionNeed>, needScan: Boolean,
             adapterEnabled: Boolean, locationEnabled: Boolean): Policy
```

语义：`sdkInt >= 31` 直连缺 `BLUETOOTH_CONNECT` → 不可连接；`sdkInt in 26..30` 直连恒 `canConnect = true`（普通权限安装时授予），但 `adapterEnabled=false` → 不可连接且状态 `BLUETOOTH_OFF`；`needScan` 时 API31+ 需 `BLUETOOTH_SCAN`，API<31 需 `FINE_LOCATION` 且 `requiresLocationServices=true`（定位关闭 → 不可扫描并提示）。

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

载荷：`"$timeSeconds:${temps.joinToString(",") { formatOneDecimal(it) }}:${humis.joinToString(",") { formatOneDecimal(it) }}"`（温度在前、湿度在后、冒号三段、逗号分隔、无空格、`Locale.US`）。

### 7.5 观测分类（纯逻辑）

```kotlin
data class Reading(val devId: String, val tX10: Int, val hX10: Int,
                   val temperatureC: Double, val humidityPct: Double)

sealed interface Observation {
    data object Placeholder : Observation     // 8 B 全零（ID 00000000 且 T_x10=0 且 H_x10=0）
    data class Valid(val reading: Reading) : Observation
}

fun classify(sample: SensorSample): Observation
```

`GatewayFrameCodec.SensorSample` 增加 `temperatureX10: Int`、`humidityX10: Int`（additive，不改变现有字段/偏移）；占位判定以 x10 整数为准，不用浮点比较。

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

### 7.7 容量与读取完整性（纯逻辑）

```kotlin
object GatewayReadGuard {
    const val MAX_DEVICES = 9              // 160 B / 16 B 设备块 −> 9
    const val MAX_FRAME_BYTES = 160
    const val ATT_OVERHEAD = 3             // GATT 读：MTU − 3

    fun expectedFrameSize(devCount: Int): Int = 16 * (1 + devCount)

    sealed interface Capacity { object Ok; data class Exceeded(val bound: Int) }
    sealed interface MtuVerdict { object Proceed; data class Refuse(val requiredMtu: Int, val actualMtu: Int) }
    sealed interface LengthVerdict {
        data class Proceed(val expectedBytes: Int) : LengthVerdict
        data class Refuse(val reason: ReadRefuseReason, val expectedBytes: Int, val actualBytes: Int) : LengthVerdict
    }
    enum class ReadRefuseReason { MTU_TOO_SMALL, LENGTH_TOO_SMALL, LENGTH_NOT_ALIGNED }

    fun capacityVerdict(boundCount: Int): Capacity
    fun mtuVerdict(negotiatedMtu: Int, devCount: Int): MtuVerdict
    fun lengthVerdict(returnedBytes: Int, devCount: Int): LengthVerdict
}
```

判定优先级（避免歧义）：`lengthVerdict` 先判 `returnedBytes % 16 != 0` → `LENGTH_NOT_ALIGNED`，再判 `returnedBytes < expected` → `LENGTH_TOO_SMALL`，否则 `Proceed`。`mtuVerdict` 仅在 `negotiatedMtu >= expected + ATT_OVERHEAD` 时 `Proceed`。

### 7.8 用户可见文本（逐字冻结）

详情页规则说明：

> 采样周期 3 分钟。仅当温度较上次**升高**超过 0.9℃且无光，或温度超过 35.0℃ 时才上报；未满足条件时不会上报，因此可能长时间没有新数据。

- 不得出现“较上次下降超过 0.9℃”；不显示光照状态/照度/lux，也不显示 App 推断的上报原因。
- App 自身报警面板的“温度湿度下降报警”保留原措辞（D-22）。
- 用户说明书与 App 变更记录同步为“升高”，并注明严格边界（0.9℃ 与 35.0℃ 均需“超过”，等于不触发）。
- 卡片/详情相对时间标注为“最近读取/更新”；详情说明“接收时间为手机读取网关缓存的时刻，不是传感器采样时刻”。

链路状态文案（示例，实现可按现有 UI 风格微调，但原因不得合并）：
- “蓝牙权限未授予：需要附近设备权限（去授权）”
- “蓝牙未开启（去开启）”
- “未配置网关地址（使用默认地址，未确认 / 搜索附近网关）”
- “网关连接失败（重试 / 检查网关电源）”
- “网关暂无传感器数据（等待传感器上报）”
- “读取不完整：蓝牙传输长度不足（MTU=x，需要 y 字节）”
- “已绑定 N 台设备，超过网关完整传输上限 9 台（请先删除多余设备）”
- “云端上传被拒绝（HTTP 401/403），数据已保留（重试上传）”
- “云端上传请求被拒绝（HTTP 4xx），数据已保留”
- “云端上传失败，重试中（待发 N 条）”

### 7.9 其余接口

SQLite v3 schema、`/upload_data` 请求体、MQTT topic 与载荷格式、BLE 记录布局、AES/证书/UUID 均不变（见 §9）。`ApiService.SensorReading.time` 仅改注释。

---

## 8. 异常与边界

| 场景 | 设计行为 |
|---|---|
| 直连权限缺失（API 31+） | 不发起连接；首页显示缺失权限与“去授权”；日志含 SDK 与权限名 |
| API<31 且定位未授予 | **直连照常**；仅“搜索附近网关”（扫描）被拦并提示 |
| 定位总开关关闭（API<31 扫描） | 扫描前提示开启；不请求无效扫描；不阻断直连 |
| 蓝牙适配器关闭 | 状态“蓝牙未开启”；不空转刷日志；开启后下一轮恢复 |
| 未选择网关（仅默认值） | 允许尝试默认地址但标注未确认；可立即搜索/手输 |
| 扫描无结果/超时/取消 | 停止扫描，显示“未找到网关”，保留手输与“扩大搜索”；不自动连接其他设备 |
| 多台候选 | 用户显式选择；不自动挑选 |
| 保存地址非法/无法解析 | 回到“未配置”并提示重新选择；不崩溃 |
| 绑定第 10 台 | 拒绝新增并提示上限 9；既有 9 台不受影响；不截断写入 `0xA1` |
| 已绑定 >9（历史数据） | 不发起读取；状态 `BINDING_CAPACITY_EXCEEDED`；提示删除多余设备；不读部分 |
| MTU 协商失败/不足 | 重连重试协商一次；仍不足则本轮不读，状态 `READ_INCOMPLETE`；不循环重试 |
| 返回密文长度不足/非 16 对齐 | 整帧拒绝；记录实际/期望字节；无读数、不入库、不入队、不转发 |
| BLE 帧被拒绝（长度/头/N=0/解密） | 不产生读数、不转发、不入库、不上传（现状保持） |
| 设备块 8 B 全零 | 判为占位：跳过；整批占位 → “网关暂无传感器数据”；不生成 `00000000` 设备 |
| 批次与绑定不符（数量/ID/顺序） | 拒绝转发 + 记录原因；本地按 ID 处理与上传仍执行 |
| 带有效 ID 的 `0.0℃/0.0%RH` | 合法读数，正常判定/入库/上传/展示 |
| MQTT 数目不一致/非法值/非法时间 | 拒绝整批（现状保持） |
| MQTT 未连接时的 BLE 读取 | 本地照常处理；仅保留最近一批待转发，连接成功后发布一次 |
| 上传 2xx | 逐行 ack 删除 |
| 上传 401/403 | 不自动重试；保留全部行；全局“上传被拒绝”状态 + 手动重试；不影响主链路 |
| 上传其他 4xx | 不自动重试；保留行；状态“请求被拒绝” + 手动重试 |
| 上传 5xx/408/429/超时/IO | 按既有退避；达 20 次保留行不再自动重试 |
| 守护关闭/未注册手机号 | 不入库（守护关闭）、不入队、不发请求 |
| 长时间无新数据 | 正常（当前现场有光且判据未启用时仅超温分支可能上报）；只显示相对读取时间与提示，不显示“离线”，不认定故障 |
| 权限运行中撤销 | 下一轮检测到即停止连接并显示状态；恢复后继续，不清数据 |

---

## 9. 兼容面（不得改动）

协议与安全：BLE 记录布局/字节序/符号、帧长规则、AES 密钥与模式、`assets/jd-ca.crt`、`TLSv1.2`、MQTT 用户名/密码拼接、topic 规则（`/<mac>/a/r0|r1|s0`）、MQTT 批量载荷格式（`time:temps:humis`）、GATT UUID、`0xA1` 写入头与分块、时间校准载荷、`mac_addr` 语义、`/upload_data` 字段与单位、SQLite 既有表结构与语义、65℃ 阈值、900 s 去抖、三条报警文案、`ALARM_TYPE_*`、`MyPrefs` 既有键名、`MacIdBook`/`DeviceParamsBook` 存储格式与 8 位 HEX 校验。

允许变更：新增 `gateway_ble_addr` 与网关选择 UI/状态展示、权限请求与提示、批次转发内部实现、占位判定与零值哨兵移除、容量与读取完整性守卫、上传结果分类与阻塞状态、规则文案与注释、版本号与文档。

---

## 10. 验证策略

### 10.1 宿主机可独立执行（实现阶段必须给出）

| 目标 | 断言要点 |
|---|---|
| 权限策略 | SDK 26/29/30/31/33 矩阵 × 直连/扫描 × 授予/拒绝 × 适配器/定位开关：API<31 直连 `canConnect=true` 且 `missing` 不含定位；`needScan` 时 API<31 需定位、API31+ 需 SCAN；`BLUETOOTH_OFF`/`LOCATION_OFF` 状态与提示一致 |
| 网关身份 | 规范化/校验（大小写、单双位段、分隔符、非法长度/字符、空值）；候选排序（FFE0 优先、RSSI、去重）；迁移默认与“已清除”；未选择/已选择 |
| 批次转发 | 单设备→`<time>:<temp>:<humi>` 逐字一致；多设备按绑定顺序；数量不符/未知 ID/重复 ID/顺序不符/全占位全部 Refuse 且不产出部分载荷；空批次不转发 |
| 占位与零值 | 8 B 全零（x10 整数判定）→ Placeholder（不处理）；`ID=有效, T_x10=0, H_x10=0` → Reading（正常处理）；无“连续 3 次”门槛 |
| 容量 | `capacityVerdict(0..9)=Ok`、`(10+)=Exceeded(bound)`；`expectedFrameSize(n)=16*(1+n)`；绑定 UI 拒绝第 10 台 |
| 读取完整性 | `mtuVerdict`：MTU 23/N=1（期望 32，需 35）→ `Refuse`；MTU 240/N=9 → `Proceed`；`lengthVerdict`：返回 20/N=1 → `LENGTH_NOT_ALIGNED`；返回 32/N=3（期望 64）→ `LENGTH_TOO_SMALL`；返回 48/N=2 → `Proceed`；返回 96/N=2（期望 48，含多余 16 B 对齐余量）→ `Proceed` 并只消费 N 块 |
| 上传分类 | 200/201/204→Success；401/403→AuthRejected；400/404/413/422→RequestRejected；408/429/500/502→Retryable；异常→Retryable |
| 上传阻塞与恢复 | 401 后不再自动请求、行不删除、状态写入；手动重试清除阻塞并恢复到期行；网络失败仍按退避；成功逐行删除 |
| 规则文案 | 含“升高”、不含“较上次下降超过 0.9”；0.9/35.0 严格表述；无光照/照度/lux 与推断原因 |
| 回归 | 现有宿主机用例全部通过；`assembleDebug` 成功；无新增编译告警 |

### 10.2 静态核查

- 直连路径不再出现 `ACCESS_FINE_LOCATION` 作为 API<31 连接前置（扫描与既有 Wi-Fi 配网页除外）。
- `app/src/main` 内 `84:C2:E4:03:02:02` 只允许出现在迁移默认常量一处。
- 不存在 `bleTempData/bleHumiData/bletime` 的“最后一笔”转发状态与 `bleZeroCount/isZeroValue` 哨兵分支。
- 转发载荷构造只在一处，且为 `time:temps:humis`；不存在 `bletime + ',' + ...` 或湿度在前的拼接。
- 工程内不再存在“较上次下降超过 0.9”的规则文案；`SensorCadence` 注释为升温。
- `MacIdBook` 新增路径与读路径均引用同一容量常量；不存在 `take(50)` 式的静默截断写入。
- 读路径引用 `GatewayReadGuard`，且 `onCharacteristicRead` 使用实际 `value.size` 判定（不是只依赖 codec 的长度错误）。
- 服务器地址仍只在 `CloudConfig` 定义；`ApiService` 未新增未证实鉴权头；`pending_uploads` 表结构未变。

### 10.3 必须由测试能力在真机独立验收（本设计不执行）

1. Android 10（Redmi 8A / API 29）开启守护：无定位授权时仍能发起 GATT 连接；不出现仅一行日志的静默路径。
2. 网关发现：有界扫描、候选呈现、保存后重连使用已保存地址；扫描必停；蓝牙关闭/无候选的界面状态；记录网关实际是否广播 FFE0/名称。
3. API 31+ 设备（或模拟器）上 `BLUETOOTH_CONNECT/SCAN`、通知权限的请求与拒绝路径。
4. 真实 GATT 读取：协商 MTU 与返回字节数是否覆盖完整快照；多设备帧本地逐条入库 + 批次转发与绑定顺序一致；数量/ID 不符拒绝转发；全零占位不产生 `00000000`；**10 台绑定被拒绝/不读取**（可在受控绑定数据下验证）。
5. 3 分钟轮询跨息屏/充电/省电长跑、前台服务存活。
6. MQTT TLS、`/upload_data` 幂等、待发箱补传、界面与设备层 SQLite 升级。
7. 上传 401/403 的真实表现与“手动重试”路径；401 机制定性属端到端 D1。

**当前阶段的验证边界（不得越界）**：现场有光且固件判据未启用，**不得**通过调温或改变光强来制造升温/无光条件；不得用模拟/注入替代真实 BLE 结论。真实占位、真实 0.0/0.0、升温分支如无合法条件，记 `NOT EXECUTED` 及原因，不写 PASS、不据此退回实现。

设备侧 instrumented 测试的依赖缺失（UTP/无外网）属测试环境问题，不由本设计用宿主机结论替代。

---

## 11. 未决项与交接

| ID | 内容 | 影响与处置 |
|---|---|---|
| U-1 | 网关是否在真实空中广播 FFE0/名称（源码显示含 FFE0、名为 `xxx`） | 设计以“FFE0 过滤优先 + 显式无过滤回退”兼容；由测试能力记录实际广播 |
| U-2 | `mac_addr`（12 位平台 ID）与 BLE MAC 是否同一物理地址 | 设计保持两者独立（D-17/D-18），不互相推导 |
| U-3 | 迁移默认值 `84:C2:E4:03:02:02` 对应的真实网关是否存在/可达 | 保留为兼容默认 + “未确认”标注；可通过发现流程替换 |
| U-4 | MIUI 12.5 后台/自启动对 180 s 轮询的影响 | 属真机长跑验收（测试能力） |
| U-5 | `/upload_data` 的鉴权机制（401 的具体原因）与真实持久化/回读 | 端到端 D1–D6 定性；Android 只提供可观测阻塞与唯一注入点（D-27/D-28），不猜协议 |
| U-6 | ~~固件判据仍与需求冲突~~ | **已由 FD-002 rev 7/8 与 IC-002 v4.0 取代**：固件实现升温与严格 >35.0℃；无光判据形制/取值另有未决项，不归 Android |
| U-7 | 设备端 instrumented/SQLite 行级验证受阻（依赖缺失、无外网） | 交测试能力；不视为 App 逻辑已验 |
| U-8 | 无光判据数值（LIGHT-DEF-2 的 `T`）未接受、当前 `T=0` 未启用 | 固件/需求责任；App 不感知光照、不显示、不补偿（D-33） |
| U-9 | TD-SW-004 只追踪 G-1…G-6 与 ITEM-001…006 | 本版新增 G-7（容量/完整性）与 G-4 的注释同步项，需软件测试能力在 TD-SW-004 增补用例与追踪（本设计不代写） |
| U-10 | 缓存复读的行语义：每 180 s 观测 → 本地/云端约 480 行/设备/日（条件性量级，不是硬上界） | 按 D-24 保持观测语义；容量上界见 BE-EXP-004 §9；若产品决定压缩需另立需求 |
| U-11 | 混合占位快照整批不转发（其他手机收不到该批） | D-20 冻结格式无“无数据”占位语义；若需部分转发需协议变更 |
| U-12 | 网关/手机跨链路“同一笔”对照 | 需真实网关、手机与合法触发条件；由嵌入式测试与联调阶段执行 |
| U-13 | 上传 4xx 的响应体/错误码含义 | 未读取部署源码；Android 只按 HTTP 状态分类，不解析未证实响应体 |
| U-14 | 真实 MTU/FFE2 返回长度未测（源码只请求 240，协商结果未用于判定） | 由软件测试在真机读取并记录 `mtu`/`value.size`；设计已给拒绝与状态路径（D-32） |
| U-15 | 绑定 9 台的实际上限只来自 160 B 静态容量，未经 9 设备联调 | 9 是“必须拒绝超过”的上界，不是“已支持 9 台”的验收结论；真实多设备由联调阶段确认 |
| U-16 | MQTT 订阅侧“数目相同但顺序错”无法自检 | 无 ID 载荷固有限制；由生产者（G-3）与绑定上下文保证，登记风险不新增协议字段 |
| U-17 | 当前现场有光 + 判据未启用 ⇒ 升温分支不触发 | 产品数据预期写入设计（D-33）；本阶段不得调温/改变光强验证该分支；不得据此判 App 故障 |

---

## 12. 自检

- [x] 仅覆盖 Android（`dengbei_care`），未改固件/网关/服务器/iOS/硬件
- [x] 未连接手机/网关、未构建、未运行测试、未写数据（本能力无设备工具入口）
- [x] 上报方向按 readme 与 IC-002 v4.0 为“升温 >0.9℃ 且无光，或 >35.0℃”，严格边界未放宽
- [x] App 不重算传感器门控、不推断光照、不合成样本；未引入按值去重
- [x] 七项缺口均有可独立检验的功能增量（状态/接口/预期行为）并落到具体文件
- [x] 新增容量/完整读取约束（≤9 台、`16×(1+N)`、MTU/返回字节），未截断、未补零、未拼接
- [x] 兼容面完整，含 `mac_addr` 语义、topic/上传契约、SQLite 表结构与既有 Wi-Fi 配网页
- [x] 未猜测 401 鉴权协议；未把 HTTP 2xx 当作持久化通过
- [x] 当前现场“有光 + 判据未启用”写入数据预期与验证禁令，未修改产品门控
- [x] 未修改其他角色工件；测试/后端待同步项登记为交接（U-9 等）
- [x] 未决项保留未决，未以“外部环境”为由删除必需目标

任务清单见 `artifacts/android_tasks.yaml`（7 项，按前置依赖排序；第 1–6 项与 AA-004/TD-SW-004 的 ITEM-001…006 一一对应，第 7 项为本版新增的绑定容量/读取完整性）。
