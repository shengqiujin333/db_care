# Android App 方案设计（AA-003）

状态：DESIGN_READY（方案完成，可交实现；不表示已实现、已构建、已在真机验证或已通过验收）。
版本：3.0
能力：`android_engineer.android_design`
基线：共享仓库 `a1ebcdb`（开始时 `git status` 干净；采用 FULL_HANDOFF，已读 `file_manifest.txt`、`git log` 与相关上游工件）。
范围：仅 Android App（`dengbei_care`，Kotlin，`compileSdk 34` / `minSdk 26` / `targetSdk 34`，现 `versionCode 9`、`versionName 1.8`）。不改 iOS、固件、网关、硬件、服务器实现、密钥、topic、认证、RF/BLE 载荷布局、上传字段。不设计 OTA；不新增网关刷写或绑定协议。

输入与优先级：
- `readme.txt`（根目录，产品修改点 1–8）。
- `artifacts/interface_contract_v4.md` IC-003 v4.0（现行接口契约，替代旧 IC-002；本次输入解析值即此文件）。
- `artifacts/product_requirements.md` PDR-001、`product_specification.md` PSPEC-001、`product_scope.md` PSCOPE-001、`product_acceptance_v2.md` PACC-002。
- `artifacts/system_analysis.md` SA-001、`system_architecture.md` SARCH-001、`system_decomposition.md` SD-001（C08–C12、B04–B07、I05/I06）。
- `artifacts/software_test_design.md` TD-SW-002、`software_e2e_plan.md` E2E-SW-002（消费边界，不替测试执行）。
- 现有 Android 源码与 `dengbei_care/_REFACTOR_NOTES.md`（as-built 事实与兼容面）。

说明：根目录没有 `.codegraph/` 索引，本次未创建索引；代码定位使用限定目录的 `rg`、原生文件查找与定向阅读，未做无界遍历。本能力不操作手机、不运行实验应用、不构建/烧录/改环境；真机安装、交互与独立验收属后续软件测试能力。

---

## 1. 结论摘要

现有 App 已具备本轮的绝大部分结构：BLE 聚合帧按有效前缀解码、MQTT 批量严格归因、5/10/15 分钟时间窗报警、SQLite v3 + `pending_uploads` 待发箱、`/upload_data` 上传、24 h 详情图、无光照展示、无“离线”判定。**本轮不需要重写 App，只需修正与 IC-003 v4/PACC-002 不一致的少量行为并做回归**：

1. **合法 0.0 ℃ / 0.0 %RH 仍被 BLE 路径当“无数据”过滤**（连续 3 次才处理）——违反 IC-003 §5、PA-012/026。
2. **详情/文档把板端上报规则写成“温度较上次下降超过 0.9℃”**，方向错误；应为“本次温度比上次高超过 0.9℃且无光，或超过 35.0℃”——违反 IC-003 §7、PR-008、SD C10。
3. **MQTT 批次按原时间做了整批跳过**（`_bkp_temp_time`），且 UI 对 MQTT 直接使用载荷时间作为“最近上报”；需改为“每次成功归因都是一次接收观察”，并用 App 接收时刻表达新鲜度，同时保留原 `time` 用于存储/上传/报警基准。
4. **无报警基准时清空卡片 sparkline**，会在长时间稀疏上报后抹掉已展示历史；基准缺失只应关闭下降报警，不应清除展示数据。
5. **`SensorReading.time` 注释仍称“真实采样时间”**；需按 IC-003 §8 改成“BLE 接收时刻 / MQTT 载荷首段原时间，非已证实源采样时刻”。
6. **MQTT 重连任务在每次 `onStartCommand` 都新起一个循环**（可能叠加），且 `connectionLost` 未主动触发；需收敛为单一有界重连循环，成功重连后重新订阅。
7. 其余为回归核对：BLE 前缀/拒绝、MQTT 数量不一致整批拒绝、outbox 原 time/退避/2xx、守护+手机号门控、报警文案/优先级/900 s、24 h 图表、`>35.0 ℃` 强调、早起报告稀疏文案。

不改变传感器门控，不在 App 复算光照/上报原因，不合成缺失样本，不把未收到当 0，不做数值/哈希去重，不承诺源样本恰一次。

---

## 2. 现有应用核对（as-built）

### 2.1 构建与入口

| 项 | 现状 | 本设计处置 |
|---|---|---|
| 工程 | `dengbei_care`，Gradle Kotlin DSL，AGP 8.3.2 / Gradle 8.4 / JDK 21 | 保持 |
| SDK/版本 | `compileSdk 34`、`minSdk 26`、`targetSdk 34`、`versionCode 9`、`versionName 1.8` | 收尾升 `10` / `1.9` |
| 前台服务 | `MqtttService`，`foregroundServiceType="dataSync"`，`START_STICKY` | 保持；不新增服务类型 |
| 权限 | INTERNET、通知、`BLUETOOTH_CONNECT/SCAN`、位置（旧版扫描/配网）、前台服务、WAKE_LOCK 等已在 Manifest | 保持；缺权限时降级不崩溃 |
| 网络 | `CloudConfig` 单一常量：`ssl://8.140.23.253:8883`、`http://8.140.23.253:5000`；明文放行仅既有主机 | 保持 |
| 协议常量 | FFE0/FFE2/FFE3、AES/ECB/NoPadding、MTU 240、topic `/<mac>/a/r0|r1|s0` | 保持 |

### 2.2 数据路径

1. **BLE 路径**：`startBluetoothTask()`（`SensorCadence.SAMPLE_PERIOD_MS = 180_000`，可手动 `wakeReadNow`）→ `connectToBluetoothDevice()`（最多 5 次、间隔 2 s）→ `connectAndReadData()`（GATT FFE2 读、MTU 240、AES 解密）→ `decryptAndParseEcbFrame()` → `GatewayFrameCodec.decodeAggregatedFrame()` → 每设备 `processTemperatureHumidityData()`。
2. **MQTT 路径**：`connectToBroker()`（TLS、用户名=MAC、密码=MAC+既有后缀）→ 订阅 `/mac/a/r0` → `messageArrived()` → `ReadingAttribution.attribute()`（严格数量）→ 逐设备 `processTemperatureHumidityData(…, batch.timeSeconds)`；同一回调还会发布既有时间校准消息。
3. **汇聚**：告警判定（`AlarmEvaluator`）→ 通知/震动/铃声 → 报警事件落库 → 守护中写 `temperature` → 守护且手机号非空时入 `pending_uploads` 并触发上传 → 更新卡片/详情/报告。
4. **上传**：`CloudUploadRepository`（门控、`Mutex`、独立 `SupervisorJob + IO`）→ `UploadOutbox`（`(device_id,time)` 幂等）→ Retrofit `POST /upload_data` → 2xx 删除，失败 `attempts+1` / 60 s 起、1800 s 上限、最多 20 次。
5. **展示**：`HomeViewModel` 单一 `devices: LiveData<Map<devId, DeviceState>>`；卡片相对时间 + 条件上报提示；详情 24 h 图与规则说明；`ReportGenerator` 稀疏/零样本文案。

### 2.3 已与现行契约一致、本轮只做回归的部分

- `GatewayFrameCodec`：头 16 B、`devCount@6`、`devCount>=1`、`size>=16*(1+N)` 才解码，只取 N 块，尾部忽略；密文未 16 B 对齐/解密失败/短帧/`N=0` 显式拒绝；`u16be@4` / `s16be@6`，负温保号，只除一次 10。
- `ReadingAttribution`：`time:temps:humis`；非法时间、空序列、非有限数、数量不一致整批拒绝；不做 `minOf` 截断；devId 随读数下沉。
- `AlarmEvaluator`：5/10/15 min 时间窗、T/RH 双下降、严格比较、`>65.0 ℃` 紧急优先且不节流；普通报警 900 s 节流在调用方；文案字面值不变。
- `TemperatureDatabaseHelper`：`DATABASE_VERSION = 3`；`temperature(time,device_id,…)` 复合主键、`alarm_events` 不变、`pending_uploads(device_id,time,…)` + 到期索引；单条 `INSERT OR REPLACE`，无 60 s 回填。
- `CloudConfig` / `ApiService`：新主机、`/upload_data` 与 `SensorUploadData/SensorReading` 字段已存在；`UploadPayload` 不二次缩放。
- 展示：卡片无“离线”；`>35.0 ℃` 强调且 `=35.0` 不强调；24 h；不显示光照/lux；报告“按条件上报，条数非等间隔”“昨日无上报”。

### 2.4 已确认的偏差与风险（本轮设计必须处理）

| ID | 位置（符号/文件） | 事实 | 影响 | 处置 |
|---|---|---|---|---|
| G-01 | `MqtttService` `bleZeroCount`、`onCharacteristicRead` 中 `isZeroValue` 分支 | BLE 读到 `0.0/0.0` 时前两次不进入业务，第三次起才处理 | 合法零值被当失败哨兵；违反 IC-003 §5、PA-026 | 删除该过滤；每个成功解码设备块首次即进入业务 |
| G-02 | `DetailPresentation.REPORT_RULE_TEXT`、`layout/fragment_device_detail.xml` 预览、`dengbei_care/readme.txt` V1.8 第 2 条、`用户使用说明书.md` §5.1/Q2 | 写成“温度较上次**下降**超过 0.9℃” | 与 readme 第 4 点、PR-008、IC-003 §7 方向相反 | 改为“本次温度比上次**高**超过 0.9℃且无光，或超过 35.0℃”；同步全部用户可见文本与测试预期 |
| G-03 | `MqtttService._bkp_temp_time` | 与上一条 MQTT 载荷首段相同时整批跳过 | 同时间不同内容会丢；与“不按数值/哈希去重、每次接收都是观察”不一致 | 删除整批跳过；按 `(devId, originalTime)` 由存储主键幂等，不做值/哈希去重 |
| G-04 | `MqtttService` 调用 `updateDeviceReading(..., currentTime)`，MQTT 的 `currentTime` 是载荷时间 | UI“最近上报”对 MQTT 表达的是载荷时间，不是 App 接收时间 | PA-027/IC-003 §7 的接收语义不成立 | 业务入口分离 `originalTime`（存储/上传/报警基准）与 `receivedAt`（App 收到秒），UI 用 `receivedAt` |
| G-05 | `MqtttService` 在 `baselines.isEmpty()` 时 `homeViewModel.clearDevice(devId)` | 超过 45 min 无历史/首条读数时清空卡片 sparkline | 稀疏上报下抹掉已展示趋势；与 24 h/稀疏展示语义冲突 | 基准为空只关闭下降分支，不清展示历史；不补点 |
| G-06 | `ApiService.SensorReading.time` 注释“真实采样时间” | 与 BLE=接收、MQTT=载荷原时间的契约不一致 | 后续误把接收时间称采样时间 | 仅改注释/KDoc 语义，字段名与类型不变 |
| G-07 | `MqtttService.scheduleReconnect()` 在 `onStartCommand` 每次调用都 `launch`；`connectionLost` 中重连被注释 | 可能叠加多个 300 s 重连循环；断线后依赖既有循环 | 重复连接/资源泄漏；恢复时延不确定 | 收敛为单个受管 Job（或 `AtomicBoolean` 守卫），断线后由同一循环重连并重订阅 |
| G-08 | `MqtttService` `bletime/bleTempData/bleHumiData` 旧“BLE→MQTT 末设备”发布 | 只保留最后一个设备、逗号格式 | 与 MQTT 冒号批量输入不是同一协议链，可能被误当桥接 | 保留现状兼容，但明确不属于本轮输入/归因契约；不扩展、不复制末设备给所有 ID |
| G-09 | `MqtttService` 未按 GXHT40 有效域过滤 | 任意有限 `Double` 进入业务 | 无源 CRC/序号，无法判断真实采样；过滤会与合法零冲突 | 不新增值域过滤/钳位；保持“收到即作为接收观察”，不伪造/不推断 |

> 以上 G-01..G-09 是代码/文案事实，不是新增产品需求。修复依据为现行 IC-003/PACC-002/PDR；不得借修复改变报警文案、阈值、优先级、900 s、协议布局或服务器字段。

---

## 3. 需求到 Android 的落点

| 需求/验收 | Android 可观察行为 | 落点 |
|---|---|---|
| readme 点 1（GXHT40） | BLE/RF 布局不变；负温、0 ℃、35.0/35.1 ℃、0/100 %RH 正确 | `GatewayFrameCodec`、单测/回归 |
| readme 点 2（光敏） | App 不显示、不推断光照；规则文案只复述条件 | `DetailPresentation`、卡片/详情；无光照字段 |
| readme 点 3（3 分钟） | BLE 轮询 180 s；手动立即读保留；不按 3 分钟补点 | `SensorCadence`、`MqtttService` |
| readme 点 4（升温/无光/>35 ℃） | 只展示规则；不重算；`>35.0` 强调、`=35.0` 不强调 | `DetailPresentation`、详情布局 |
| readme 点 5（服务器） | MQTT/HTTP 新主机；TLS/证书/认证不变；旧 IP 不作目标 | `CloudConfig`、`network_security_config.xml` |
| readme 点 6（只改 Android） | iOS/固件/网关/硬件无改动；协议布局不变 | 兼容面清单 |
| PR-007 / PA-012/028 | BLE 有效前缀；非法帧拒绝；尾部不产读数 | `GatewayFrameCodec` |
| PR-007 / PA-026 | 合法 0/0 首次即有效；失败不落 0 | 删除 G-01 过滤 |
| PR-007 / PA-013 | MQTT 数量不符整批拒绝；顺序错不可检测需记录 | `ReadingAttribution` + 手册 |
| PR-007/008 / PA-027 | 接收观察与源样本区分；不去重丢失同值新报 | `receivedAt`/`originalTime` 分离；删除 G-03/G-04 |
| PR-008 / PA-014 | 不判离线；相对时间 + 条件上报；24 h；高温强调 | 卡片/详情/`DeviceState` |
| PR-008 / PA-015 | 5/10/15 min 双下降、严格阈值、>65 紧急、900 s | `AlarmEvaluator` + 调用方 |
| PR-008 / PA-030 | 只对已收到样本报警；不补造基准；覆盖限制写入文档 | 设计/手册 |
| PR-009 / PA-016/029 | 守护+手机号门控；2xx 成功；原 time 重试；同键一行 | `CloudUploadRepository`/`UploadOutbox` |
| PR-009 / PA-023 | 旧 IP 不再作为连接目标；字段/单位/密钥不变 | 静态检查/兼容面 |
| PR-014 / PA-021/022/024/025 | App 可部署、可排障、证据可复现 | 版本/文档/日志；真机与总测由后续能力执行 |

---

## 4. 目标架构

### 4.1 分层与单一真相

```text
[输入适配]                         [纯逻辑]                          [业务/状态]                 [持久化]               [云]
BLE GATT FFE2 ──AES──► GatewayFrameCodec ─┐
                                          ├─► ReadingObservation ─► AlarmEvaluator ─► HomeViewModel/UI
MQTT /a/r0 ─────────► ReadingAttribution ─┘        │                 (时间窗/阈值/紧急)      │
                                                  │                                          ├─► temperature / alarm_events
                                                  └─► 守护/手机号门控 ─────────────────────► └─► pending_uploads ─► POST /upload_data
```

状态所有权（不新增第二套真相）：
- **绑定与身份**：`MacIdBook`（设备 ID/名称）、`MyPrefs.mac_addr`（网关 MAC）、`registerphone`；只读复用。
- **守护开关**：`HomeViewModel.startData` + `MyPrefs.measure_state`（`"stopping"`=运行中，既有约定）。
- **展示状态**：`HomeViewModel.devices`（不可变 `DeviceState` map）。
- **历史/报警/待发**：SQLite v3（唯一持久队列）。
- **链路状态**：MQTT/BLE 连接结果与日志；**不**由数据龄推导。
- **报警参数**：`MyPrefs` + `DeviceParamsBook`（按设备覆盖）；语义不变。

### 4.2 模块职责（复用为主）

| 模块 | 职责 | 本轮动作 |
|---|---|---|
| `protocol/GatewayFrameCodec.kt` | BLE 聚合帧前缀解码、显式拒绝、ID/端序/符号 | 保留；补边界回归（160 B 尾部、N 上限、块不足） |
| `telemetry/ReadingAttribution.kt` | MQTT 批量严格归因 | 保留；回归数量/空/非法/顺序限制 |
| `telemetry/AlarmEvaluator.kt` | 时间窗下降、严格阈值、紧急 | 保留；回归边界与优先级 |
| `telemetry/SensorCadence.kt` | 180 s 常量（轮询/文案同源） | 保留 |
| `MqtttService.kt` | 连接、读取、解密、归因、编排、通知、重连 | 修 G-01/G-03/G-04/G-05/G-07；其余保持 |
| `TemperatureDatabaseHelper.kt` | v3 schema、单条写入、区间查询、报警事件 | 保留；回归迁移/主键/守护门控 |
| `cloud/CloudConfig.kt` | 连接常量唯一来源 | 保留 |
| `cloud/UploadPayload.kt` / `ApiService.kt` | 请求体/DTO | 仅修 G-06 注释；字段不变 |
| `cloud/UploadOutbox.kt` / `CloudUploadRepository.kt` | 幂等入队、到期批、ack/退避、门控、串行 | 保留；回归 `false` 语义/原 time/独立作用域 |
| `DeviceState.kt` / `ui/home/DeviceCardAdapter.kt` | 卡片状态、相对时间 | 修接收语义文案与 G-05 展示历史 |
| `ui/detail/DetailPresentation.kt` / `DeviceDetailFragment.kt` / 布局 | 规则文案、24 h、高温 | 修 G-02；同步 `tools:text` |
| `report/ReportGenerator.kt` | 稀疏/零样本文案 | 保留；回归 |
| `dengbei_care/readme.txt`、`用户使用说明书.md` | 用户文档 | 修正方向、接收语义、云同步说明；升版本 |

### 4.3 设计决定

| ID | 决定 | 依据/理由 | 否决项 |
|---|---|---|---|
| AA3-D01 | BLE 每个成功解码设备块**无条件**进入业务；不按 `0/0`、数值、哈希做过滤或去重 | IC-003 §5；PA-026/027；失败由帧级校验表达，不由数值猜 | 保留 `bleZeroCount`；用 0 当“无数据”哨兵 |
| AA3-D02 | MQTT 每次成功归因的批次都是一次**接收观察**；不因与上一批同 `time` 整批跳过 | 避免同时间不同内容丢失；PA-027 允许记录多次接收、只禁止虚假“源样本”声明 | 保留 `_bkp_temp_time`；加数值/哈希去重 |
| AA3-D03 | 业务入口分离 `originalTime` 与 `receivedAt`：`originalTime` 用于报警基准/本地写入/上传（BLE=接收秒，MQTT=载荷首段）；`receivedAt` 只用于 UI 新鲜度 | IC-003 IF06；PA-027；MQTT 载荷时间不能被宣称为 App 接收时间 | 把载荷时间当“最近接收到”；把接收时间写入上传 time |
| AA3-D04 | 时间窗基准缺失只关闭下降分支，**不清空** sparkline/历史 | 稀疏上报是正常产品行为；PA-014/030；不补点 | `clearDevice` 作为“无参照清屏” |
| AA3-D05 | 规则文案使用“本次温度比上次高超过 0.9℃且无光，或温度超过 35.0℃才上报；可能长时间没有新数据” | readme 点 4；IC-003 §7；PR-008；SD C10 | 沿用“下降超过 0.9℃” |
| AA3-D06 | UI 保留“最近上报”产品词，但明确标注“App 接收时间，非传感器采样时刻”；卡片状态不再推断离线 | PDR PR-008；IC-003 §7 允许诚实接收语义；FB-13 最终文案仍待产品 | 把相对时间称为精确源上报时间；显示“离线” |
| AA3-D07 | 不上传空批次；仅在守护中且手机号非空时入队；本地历史与云端门控分离 | PR-009；SARCH §4.2 | 无条件上传；因云失败跳过本地报警/入库 |
| AA3-D08 | 待发箱只以 `(device_id,time)` 幂等；同键 `false` 不覆盖既有行；业务 time 重试不变；2xx 删除；失败退避 60→1800 s、最多 20 次后保留 | IC-003 IF07；PA-016/029 | 用重试调度时间替换业务 time；达到上限清库 |
| AA3-D09 | BLE 有效长度 `16*(1+N)`；承载更长时只取前 N 块；`N=0`/短帧/未对齐/解密失败拒绝 | IC-003 IF04；PA-012/028；SARCH AD05 | 严格等长拒绝固定 160 B；解析尾部 |
| AA3-D10 | MQTT 数量不一致/非法时间/空/非有限值整批拒绝；同数量顺序错不可检测，写入用户手册 | IC-003 IF05；PA-013 | 静默截断；用顺序证明身份 |
| AA3-D11 | 重连使用**单一**受管循环；断线后由该循环重连并重新订阅；失败不阻塞 BLE/上传；不做无界紧循环 | 可靠性/资源边界；不新增产品 SLA | 每次 `onStartCommand` 叠加循环；无限立即重试 |
| AA3-D12 | 不新增值域过滤/钳位；未知 BLE ID 保持原 ID，不映射到其他已绑定设备 | 无协议 CRC/序号，值域过滤会与合法零/边界冲突；PR-007 只禁止错配 | 把越界值/未知 ID 静默改写成别台设备 |

---

## 5. 接口与数据处理设计

### 5.1 BLE（IF04）

- 密文长度必须为 16 B 整数倍；AES/ECB/NoPadding、既有密钥不变；解密失败拒绝。
- 明文 ≥16 B；`N = plain[6] & 0xFF`；`N>=1`；`plain.size >= 16*(1+N)`；否则拒绝。
- 只消费前 `16*(1+N)`；每设备块偏移 `16+16*i`，前 8 B：`id[0..3]` 原字节、`humidity_x10=u16be@4`、`temperature_x10=s16be@6`；`/10.0` 一次；负温保号。
- 成功帧内每个设备块生成一条 `ReadingObservation`（`transport=BLE`、`originalTime=receivedAt=App 接收 Unix 秒`、`devId=8 位大写 HEX`、`sourceSampleTime=unknown`）。
- 拒绝时：不产生读数、不写库、不入队、不报警；仅记录 `reason/detail` 日志。
- 多设备逐块归因；未知 ID 保留自身，不改写为其他已绑定 ID。
- 承载尾部（如固定 160 B 属性）不是设备数据，不产生读数，也不作为完整性证明。

### 5.2 MQTT（IF05）

- 主题沿用 `/<gateway_mac>/a/r0` 等；不改订阅/发布分工，不新增桥接服务。
- 载荷 `<original_epoch_seconds>:<t1,t2,...>:<h1,h2,...>`；时间必须为正 Long；序列非空、每项有限；`savedIds.size == temps.size == humis.size`；任一失败整批拒绝（不写库/上传/报警）。分段多于三段时沿用现有兼容行为（只消费前三段，后续忽略），不新增“多冒号必拒”。
- 按同一 `MacIdBook` 顺序归因；`devId` 随读数下沉；数量相同但顺序错无法由格式检测，手册/验证保留限制。
- `originalTime = parts[0]`；`receivedAt = Service 当前 Unix 秒`；两者不混用。
- 每次成功归因批次都是一次接收观察；不按数值/哈希/同时间整批去重；同 `(devId, originalTime)` 的重复由 `temperature` 主键与 outbox 主键自然收敛，不宣称源样本恰一次。
- 既有“BLE 末设备转 MQTT”发布保留兼容，但不作为本契约的输入链，不扩展、不复制给所有 ID。

### 5.3 业务记录（IF06，内部语义，不新增持久字段）

```text
ReadingObservation:
  devId           8 位大写 HEX（BLE 帧内 ID / MQTT 绑定顺序对应 ID）
  temperatureC    Double，℃（只除一次 10）
  humidityPct     Double，%RH（只除一次 10；0/100 合法）
  transport       BLE | MQTT
  originalTime    BLE=接收秒；MQTT=载荷首段；用于存储/上传/报警时间窗
  receivedAt      App 收到秒；仅用于 UI 新鲜度/相对时间
  sourceSampleTime = unknown（不显示为精确采样时刻）
```

- 守护门控：`startData == 1` 才写本地历史；关闭不写、不补历史。
- 上传门控：守护中且 `registerphone` 非空才入队；本地历史写失败不冒充成功，仍独立尝试 outbox（若门控通过）；outbox 失败不影响报警/展示。
- 报警：始终对已收到样本评估；`>65.0 ℃` 紧急优先、每次通知、不受 900 s；其余 900 s 节流；阈值严格；5/10/15 min 双下降；无基准不报警、不补造。普通报警的震动/铃声/通知仍保持既有守护条件与 900 s 行为，不借本轮修正扩大或缩小既有启用条件。
- 展示：接收时间 + 条件上报说明；不显示光照/照度；不推断上报原因；不把“没新数据”推断为离线/环境稳定。

### 5.4 本地持久化（SQLite v3）

```text
temperature(time INTEGER, device_id TEXT, temperature REAL, humidity REAL,
            PRIMARY KEY(time, device_id))                    -- 现有结构不变
alarm_events(id INTEGER PK AUTOINCREMENT, time, device_id, type, message) -- 不变
pending_uploads(device_id TEXT, time INTEGER, temperature REAL, humidity REAL,
                attempts INTEGER DEFAULT 0, next_attempt_at INTEGER DEFAULT 0,
                PRIMARY KEY(device_id, time))                -- v3 新增
INDEX idx_pending_uploads_next(next_attempt_at)
```

- 写入：单条 `INSERT OR REPLACE`，`time = originalTime`；不按 60 s 回填；同键重复收敛为一行（同秒真实观察碰撞不承诺两行）。
- 迁移：`oldVersion < 3` 只建 outbox 表/索引；异常沿用既有回退（记录日志、不崩溃）。
- 查询：`getSamplesInRange(devId, from, to)` 供 5/10/15 时间窗；范围 `[now-15min-slack, now]`，`slack=30min`。
- 失败隔离：历史写、报警事件、outbox、UI 更新各自 try/catch；任一失败不跳过其他步骤。

### 5.5 `/upload_data`（IF07）

```json
{"phone":"已注册手机号","mac":"网关MAC","readings":[
  {"devId":"01020304","time":1700000000,"temperature":-5.0,"humidity":60.0}]}
```

- `devId` 为 8 位大写 HEX；`time` 为 `originalTime`；温度 ℃、湿度 %RH；不二次缩放；空 readings 不发。
- 本地 outbox 同键不覆盖；到期批默认 50；失败 attempts+1、`now+backoff`（60 s 起、1800 s 上限），达到 20 次保留但不再自动重试；HTTP 2xx 删除本批；非 2xx/异常/超时/未知响应保留原项。
- 上传在独立 `SupervisorJob + IO`，`Mutex` 串行；失败不阻塞 BLE/MQTT；触发点：入队后、每轮 BLE 开始、MQTT 连接成功（网络恢复）、Service 冷启动。
- 幂等只在 `(dev_id,time)` 重试范围；不同接收秒可形成不同键；同秒不同真实观察可能碰撞；不承诺源样本恰一次。HTTP 明文/TLS 主机名校验风险不在 App 内改造。

---

## 6. 页面状态与用户路径

### 6.1 卡片墙（`HomeFragment` / `DeviceCardAdapter`）

状态 chip（既有，保持）：
- **未收到**：从未收到任何接收观察；温度/湿度显示 `--`；提示“尚未收到上报 · 按条件上报，可能长时间无上报”。
- **正常**：有接收且无报警；显示最新值 + 相对时间 + 条件上报提示。
- **警告**：下降/超限；显示既有报警文案。
- **紧急**：`>65.0 ℃`；红色；既有文案。

新鲜度：
- 保留“最近上报 X 分钟/小时/天前”产品表述，追加“（App 接收时间）”限定；**不使用“离线”**；不以 30 min 数据龄推导连通性。
- 相对时间基于 `receivedAt`（G-04）；未收到不显示 0/0 为有效读数。
- 合法 `0.0 ℃/0.0 %RH` 显示为 `0.0 °C / 0 %`，不显示 `--`（`latestTime != 0` 即已收到）。

### 6.2 设备详情（`DeviceDetailFragment`）

- 规则只读说明（逐字目标）：
  `采样周期 3 分钟。仅当本次温度比上次高超过 0.9℃且无光，或温度超过 35.0℃ 时才上报；未满足条件时不会上报，因此可能长时间没有新数据。`
- 最新读数 + 最近接收相对时间（注明 App 接收时间，非采样时刻）。
- 24 h 图表（DB 查询；稀疏点不补零、不插值）；`>35.0 ℃` 红色 + “· 高温”，`=35.0 ℃` 不强调。
- 报警记录 24 h；不显示光照/无光状态/lux；不显示“因下降 x℃ 上报”等推断。
- 历史为空/无基准：图表为空是合法展示，不写“离线/故障”。

### 6.3 通知、报警、报告

- 通知标题/正文、报警文案、优先级、900 s、紧急不节流：保持既有字面值。
- 报警事件类型 `drop/threshold/emergency` 与入库结构不变；`NORMAL/未设置报警` 不落库。
- 早起报告：`按条件上报，条数非等间隔`；无样本“昨日无上报”；不把零样本写成设备异常。

### 6.4 网关/绑定/发现

- 网关身份由既有正常流程确认：用户配网/绑定得到 MAC（`mac_addr`），设备 ID 列表由 `MacIdBook` 管理；App 不扫描猜测 BLE 网关、不猜 MAC、不改 `0xA1` 绑定载荷。
- 未知 BLE ID：保留原 ID，不映射到已绑定设备；不自动写绑定。
- 不设计 OTA/固件刷写/网关管理入口；无授权入口。

---

## 7. 连接、权限与重连

| 项 | 设计行为 |
|---|---|
| BLE 连接 | 用已保存网关 MAC；`connectGatt` 后 5 次重试、间隔 2 s；每轮读 FFE2 后关闭；下一轮 180 s 或手动 `wakeReadNow`；失败只记录并等下一轮，不紧循环 |
| BLE 读取 | MTU 240；FFE2 读；AES 解密；长度/数量/解密失败拒绝；读取成功≠传感器在线/新源样本 |
| MQTT 连接 | `ssl://8.140.23.253:8883`、TLS 1.2、`jd-ca.crt`、用户名=MAC、密码=MAC+既有后缀；订阅 `/mac/a/r0`；不改 topic/认证 |
| MQTT 重连 | 单一受管循环（G-07）：断线/初始失败后按有界间隔重试（保留既有 300 s 级，不紧循环），成功重连后重新订阅；重复 `onStartCommand` 不叠加循环；重连失败不影响 BLE/outbox |
| 上传重试 | 独立于 MQTT/BLE；见 §5.5；网络恢复触发 |
| 权限（Android 13+） | 通知权限缺失：报警仍判定/入库，通知降级并提示；不崩溃 |
| 权限（Android 12+） | `BLUETOOTH_CONNECT/SCAN` 缺失：BLE 路径不可用并提示，MQTT/历史/上传不受影响；不伪造数据 |
| 位置权限 | 仅既有配网/旧版扫描需要；拒绝时提示，不把缺少位置权限当成设备离线 |
| 进程/服务重启 | `START_STICKY`；恢复 MQTT/BLE 循环与 outbox；内存历史不恢复，时间窗基准从 DB 取；不恢复伪造状态 |

---

## 8. 异常与恢复矩阵

| 场景 | 行为 | 不得发生 |
|---|---|---|
| 密文未对齐/解密失败/短帧/N=0/块不足 | 整帧拒绝；日志 reason；无读数 | 产生读数、落库、上传、报警 |
| 160 B 承载、N=1/2/9 | 只取 `16*(1+N)`；尾部忽略 | 把尾部当设备/读数；拒绝合法前缀 |
| BLE 合法 0/0 | 首次即作为接收观察进入业务 | 连续 3 次门槛；把 0 当失败 |
| BLE 未知 ID | 保留原 ID，不映射 | 错配到其他已绑定设备 |
| MQTT 数量不符/空/非法时间/NaN/Inf | 整批拒绝；日志 | 截断、补零、部分归因 |
| MQTT 同时间重复/同值新报 | 作为接收观察；存储同键收敛；报警按规则 | 值/哈希去重丢新报；声称源样本恰一次 |
| 无下降基准/无历史 | 不触发下降；不清 sparkline | 补造基准、按样本序号假装分钟、清空已展示历史 |
| 守护关闭 | 不写历史、不入队；报警仍对已收到样本评估 | 产生云数据；因关闭跳过紧急报警 |
| 未注册手机号 | 不入队、不发请求；本地历史按守护门控 | 上传无归属数据 |
| HTTP 非 2xx/超时/未知响应 | 保留 outbox，attempts+1，退避 | 用调度时间替换业务 time；清库 |
| 达到 20 次 | 保留待发项，不再自动重试；日志可见 | 无限重试；静默删除 |
| 数据库写失败 | 记录失败；不冒充成功；其他步骤继续 | 吞掉 outbox 可独立持久化项 |
| 铃声/震动/通知失败 | 捕获；不跳过入库/上传 | 因播放失败丢数据 |
| MQTT 断线 | 单一循环重连并重订阅；BLE/上传继续 | 多循环叠加；紧循环 |
| 时钟偏差 | `originalTime` 按来源；`receivedAt` 用系统时间；不互相覆盖 | 把接收时间当源采样时刻 |
| 长时间无新数据 | 正常；显示相对时间与条件提示 | 判“离线”；推断无人蹬被/光照 |

---

## 9. 兼容面（不可改动）

- BLE：FFE0/FFE2/FFE3 UUID、AES 密钥/ECB/NoPadding、16 B 对齐、头/设备块布局、`u16be/s16be`、8 位 HEX ID 形式、MTU 240。
- MQTT：主机/端口、TLS 1.2、`jd-ca.crt`、用户名/密码拼接、topic 规则、时间校准消息模板、认证。
- HTTP：既有 5 个端点签名/DTO、`/upload_data` 字段名与类型；不新增认证/管理凭据。
- 报警：5/10/15 min、T/H 双下降、严格比较、`>65.0`、900 s、文案字面值、`alarm_events` 类型。
- 存储：`temperature`/`alarm_events` 既有列与主键；`MyPrefs` 键名、`MacIdBook`/`DeviceParamsBook` 格式；守护键值约定。
- 展示：不显示光照；`=35.0` 不强调；不把未收到当 0；不补点。
- 范围：不改 iOS/固件/网关/硬件/服务器代码；不新增协议字段、心跳、序号、源时刻。

允许变更：删除 G-01/G-03 过滤、接收/原时间分离、规则与接收文案、注释语义、单例重连、sparkline 保留、版本号与用户文档、补充回归测试。

---

## 10. 验证策略（设计层，供实现/测试消费）

实现阶段应可独立检查：
1. **宿主机 JUnit**（`./gradlew :app:testDebugUnitTest`）：
   - 帧：正/负温、0/0、35.0/35.1、0/100 %RH、N=1/2/9、160 B 尾部、短帧、未对齐、N=0、块不足、解密失败。
   - 归因：数量一致/不一致、空、非法时间、NaN/Inf、顺序反例（同数量不可检测需记录）。
   - 报警：5/10/15 边界、双下降 AND、严格阈值、>65 优先级、900 s 调用方节流。
   - outbox：同键不覆盖、到期批、attempts/backoff/上限、2xx 删除。
   - 展示：规则逐字方向、`>35` 边界、接收时间语义、无光照字段。
2. **构建**：`./gradlew :app:assembleDebug`（资源合并 + Kotlin 编译）。
3. **静态检查**：无 `117.72.84.210` 连接目标；无 `bleZeroCount/isZeroValue`；用户可见文本无“下降超过 0.9”；无 60 s 回填；无“离线”状态分支；`ApiService.time` 注释不再称真实采样。
4. **仪器化**（环境具备时）：SQLite v2→v3、单条写入主键、outbox 幂等/退避；当前环境登记 `android_app_testing` 对当前能力不可用，真机安装/交互/截图/SQL 读回由后续软件测试能力在获授权条件执行，不能以本设计或模拟结果替代。
5. **E2E**：真实网关+broker+服务器+手机+暗/光条件由总测能力执行；App 设计只保证接口与状态可观察，不宣称已通过。

---

## 11. 未决项与交接

| ID | 内容 | 对 Android 的影响 | 处置 |
|---|---|---|---|
| FB-13 / U03 | “最近上报”最终产品文案与源样本语义 | 本设计保留产品词 + 接收时间限定；若产品改文案，只改展示层 | 产品/验收决定；不在实现中扩大承诺 |
| FB-14 / U07 | 同键幂等 vs 源样本恰一次 | 仅承诺 `(devId,time)` 重试；不承诺源去重 | 文档/验收保留限制 |
| FB-18 / U06 | MQTT 发布端、绑定顺序、原 time 来源 | App 严格拒绝数量不符；同数量顺序错不可检测 | 发布端/运维给原文；手册记录 |
| U02 | 真实 MTU/长读/160 B 承载 | 本设计按前缀兼容；不按 9/255 宣称容量 | 软件/嵌入式联调实测 |
| U-2（旧） | 守护关闭期间是否应上传 | 本设计维持“关闭不入队” | 产品确认后才改一处门控 |
| 服务器在线/唯一键 | `/upload_data`、`sensor_data`、`UNIQUE` 是否生效 | 2xx 只代表 HTTP 成功；SQL 读回另验 | 运维/独立验证 |
| 环境 | `android_app_testing` 当前不可用；`android_readonly_inspection` 仅只读 | 真机测试不在本能力执行 | 后续测试能力在授权条件执行 |
| G-08 | 旧 BLE→MQTT 末设备发布 | 可能被误认为桥接；只保留兼容 | 不在本轮扩展；需要时由产品/接口另立需求 |

---

## 12. 任务分解摘要

见 `artifacts/android_tasks_v3.yaml`（按前置依赖排序，供 `android_engineer.android_implementation` 消费）。顺序：输入语义修正（零值/同时间去重/接收时间/基准保留）→ 协议与归因回归 → 报警/持久化/outbox 回归 → 重连/展示/文案/注释 → 文档版本 → 构建与静态/单测自检。每项均给出可独立检查的预期行为；真机与端到端验证不写入实现任务冒充通过。

---

## 13. 自检

- [x] 仅设计 Android；未改 iOS/固件/网关/硬件/服务器/协议布局。
- [x] 依据当前 IC-003 v4、PACC-002、PDR/PSPEC/SARCH/SD，不沿用旧 IC-002 或旧验收结论。
- [x] 明确了合法零、BLE 前缀/尾部、MQTT 严格归因、接收观察与源样本、同键幂等边界。
- [x] 保留了既有报警阈值/文案/优先级/900 s、服务器字段、密钥/topic/认证。
- [x] 未在 App 复算光照/上报原因，未补点/补零，未做值/哈希去重。
- [x] 每项偏差有文件/符号定位、影响与可检查目标；未把“设计完成”写成“实现/真机通过”。
- [x] 未执行设备操作、构建、烧录或网络/SQL 请求；真机与总测责任已交接。
