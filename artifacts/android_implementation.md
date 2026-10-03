# Android 实现记录（AIMPL-002）

状态：Android 实现（`android_engineer.android_implementation`），按 Runtime 逐个指派的队列项推进
依据：`artifacts/android_architecture.md`（AA-002）、`artifacts/android_tasks.yaml`（14 项）、`artifacts/interface_contract.md`（IC-002）、`artifacts/software_test_design.md`（TD-SW-002）、as-built 源码 `dengbei_care/`
范围：仅 Runtime 当前指派的队列项；同队列其它项（ITEM-002…ITEM-014）不在本轮实现，未提前改动。
兼容面：AES 密钥/模式、`assets/jd-ca.crt`、`TLSv1.2`、MQTT 用户名/密码拼接、topic 规则、MTU 240、GATT UUID、`0xA1` 写入头、时间校准 payload、65℃ 阈值、900 s 去抖、报警文案字面值、`MyPrefs` key 名、`MacIdBook`/`DeviceParamsBook` 格式——本轮均未改动。

---

# 任务项 ITEM-001（本轮完成）

**ITEM-001（队列第 1 项）**：新增纯逻辑模块 `protocol/GatewayFrameCodec.kt` 承载 BLE 聚合帧解码（16B 头块 = 网关ID 6B + devCount@6；每设备 16B 块取前 8B = sensor_id 4B 原字节序、`humidity_x10 = u16be@4`、`temperature_x10 = s16be@6` 有符号），并让 `MqtttService` 的解密后解析改为调用该模块。
期望行为：解码结果与改动前的字节偏移、字节序、符号处理逐位一致；`devCount == 0` 或帧长不足 `expected = 16*(1+devCount)` 时明确拒绝该帧且不产生读数、不落库、不上传。

设计映射：AA-002 §4.2（新增 `protocol/GatewayFrameCodec.kt`）、§5.1（BLE 解析契约不变）、D-01（不改字节偏移/符号）、D-02（纯逻辑可独立检查）。

## 1. 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/protocol/GatewayFrameCodec.kt` | 新增 | 纯 Kotlin（无 Android 依赖）聚合帧解码：常量、`u16be`/`s16be`、HEX 格式化、`SensorSample`、`Result.Success/Rejected` + `RejectionReason`、`decodeAggregatedFrame`、`decodeDeviceBlock` |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/MqtttService.kt` | 修改 | ① 新增 `import com.jinyuni.dengbei_care.protocol.GatewayFrameCodec`；② 删除本地 `parsePlainFrame` 与已被其替代的 `DeviceReading`/`ParsedFrame` 类型定义（消除第二套解析实现）；③ `decryptAndParseEcbFrame` 改为返回 `GatewayFrameCodec.Result`；④ `onCharacteristicRead` 按 `Success`/`Rejected` 分支处理 |
| `dengbei_care/app/src/test/java/com/jinyuni/dengbei_care/protocol/GatewayFrameCodecTest.kt` | 新增 | 宿主机 JUnit4 自检 11 项（帧长规则、字段/符号/单位向量、多设备归属、与改动前参考实现固定种子随机比对 500 组） |

关键实现要点（与改动前逐位对齐）：

| 项 | 改动前 | 改动后 |
|---|---|---|
| 头部块 | `gwid6 = plain[0..5]`、`devCount = plain[6] and 0xFF` | 同（`GatewayFrameCodec.decodeAggregatedFrame`） |
| 帧长规则 | `expected = 16*(1+devCount)`，`plain.size >= expected` 才继续；更长时按 devCount 取数并忽略尾部 | 同（`expectedFrameSize`；尾部填充忽略） |
| 每设备块 | `block = plain[off..off+15]`（off 从 16 步进 16），取前 8 B | 同（`HEADER_BLOCK_SIZE`/`DEVICE_BLOCK_SIZE`；`decodeDeviceBlock`） |
| 湿度 | `u16be(p8,4)/10.0` | 同 |
| 温度 | `s16be(p8,6)/10.0`（有符号，负温保留） | 同 |
| 设备 ID | `devIdHex = "0C:11:82:24"`，调用方 `.replace(":", "")` 得 `0C118224` | `SensorSample.devIdHex`（冒号形式，日志不变）+ `SensorSample.devIdCompact`（8 位大写，MacIdBook 键）；调用方不再自行去分隔符 |
| 长度不足 | `require(...)` 抛 `IllegalArgumentException` | 显式 `Result.Rejected(LENGTH_MISMATCH)`，不抛异常 |
| `devCount == 0` | 视作空帧（无读数、不报错） | 显式 `Result.Rejected(NO_DEVICE)`（按本项要求"明确拒绝"） |
| 密文长度非 16 B 对齐 | `aesEcbDecrypt` 内 `require` 抛异常（由调用处 try/catch 吞掉） | 前置 `Rejected(CIPHERTEXT_NOT_ALIGNED)`；解密抛错时 `Rejected(DECRYPT_FAILED)` |

行为变化仅限**拒绝方式**（异常驱动 → 显式结果），解码数值与字节偏移、字节序、符号**未变**；`parseHexData`（小端残留、ITEM-002 范围）本轮**未改动**，BLE/MQTT 两条入口均不经它。

## 2. 预期行为（本轮交付）

1. 解密后的明文按 `16*(1+devCount)` 解码：恰等长度与"更长带尾部填充"均成功，取 `devCount` 条读数，忽略尾部填充。
2. 明文 `< 16 B` → `FRAME_TOO_SHORT`；`devCount == 0` → `NO_DEVICE`；明文 `< expected` → `LENGTH_MISMATCH`；密文非 16 B 对齐 → `CIPHERTEXT_NOT_ALIGNED`；解密抛错 → `DECRYPT_FAILED`。以上一律**不产生读数**，因此不落库、不上传、不报警（调用方 `onCharacteristicRead` 仅记 `Log.w` 后继续收尾流程）。
3. 每条读数携带自身 4 B ID（`devIdHex` 冒号形式供日志、`devIdCompact` 8 位大写供 `MacIdBook` 归因），不对列表下标建立隐含依赖。
4. 温度 `s16be@6/10.0` 有符号、湿度 `u16be@4/10.0` 无符号，单位与改动前一致（℃、%RH），不因 GXHT40 替换而变化。
5. 工程内 BLE 聚合帧解析只有 `GatewayFrameCodec` 一处实现。

## 3. 本轮验证（实现侧自检）

| 检查 | 命令 | 结果 |
|---|---|---|
| 宿主机单元测试 | `./gradlew :app:testDebugUnitTest --offline`（`dengbei_care/`） | `BUILD SUCCESSFUL`；`GatewayFrameCodecTest` 11/11 通过、`ExampleUnitTest` 1/1，共 12 项、0 失败 0 跳过 |
| 构建 | `./gradlew :app:assembleDebug --offline` | `BUILD SUCCESSFUL`；产出 `app/build/outputs/apk/debug/app-debug.apk`（10,285,626 B） |
| 逐位一致 | 测试 `matchesLegacyImplementation_onRandomVectors`（500 组固定种子随机帧，devCount 1..5、湿度 0..1000、温度 −400..1250、随机尾部填充） | 与改动前 `parsePlainFrame` 参考实现在网关 ID、devCount、devIdHex、湿度、温度上逐位相等（含尾部填充场景） |
| 单一实现 | `grep -rn "parsePlainFrame\|ParsedFrame\b" MqtttService.kt` | 本地 `parsePlainFrame` 与 `ParsedFrame`/`DeviceReading` 定义已删除，无残留调用 |

原始输出见 `evidence/android_test.md`（AT-001）。

## 4. 观察与交接

1. **`devCount == 0` 的行为变化属有意为之**（本项要求"明确拒绝"）：改动前是"静默空帧"。网关 `memset(sensorres,0,...)` 的超时路径会发布全 0 记录——按 IC-002 §4/§5，`devCount` 仍为真实设备数，故该变化不影响合法遥测；真实 BLE 联调由后续测试能力确认（TD-SW-002 §6-5）。
2. **`DeviceReadingV3` / `parseUnencryptedFrame` / `interpretPayloadV3` 仍保留**：当前无有效调用点（仅注释残留），但属旧帧解析族的清理范围，未在本项任务描述内，未改动，避免越权扩大 diff。
3. **`parseHexData`（小端）**：已在队列下一项 ITEM-002 中移除（见下节），本项记录保留以说明当时状态。
4. **未修改**：服务器常量、SQLite、告警、UI、`/upload_data`、依赖与 `build.gradle.kts` 均属后续队列项，本轮未动。
5. 本项不涉及真机 BLE 读取行为；BLE 与真实网关的联调属后续测试能力（AA-002 §10、TD-SW-002 §4.6 REG-02）。

---

# 任务项 ITEM-002（本轮完成）

**ITEM-002（队列第 2 项）**：移除 `MqtttService` 中无调用点且与接口契约冲突的小端解析入口 `parseHexData`（`humidity|temperature` 小端解释）。
期望行为：工程内不再存在该小端解释入口与相关注释残留，且 BLE/MQTT 两条入口的解析均只经过 `GatewayFrameCodec` 的唯一实现。

设计映射：AA-002 §4.2/§5.1、D-01（唯一解析实现）、§10（静态检查：不再存在该入口）；TD-SW-002 §2.1-S2、§4.3 T-SW-L1-03。

## 1. 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/MqtttService.kt` | 修改 | ① 删除 `fun parseHexData(hexData: String): Pair<List<Int>, List<Int>>`（按 4 字节分组做 `humidity = b0 or b1<<8`、`temperature = b2 or b3<<8` 的小端解释）；② 删除 `onCharacteristicRead` 中引用它的那段注释掉的旧替代解析路径（HEX 字符串 + 未加密帧 + `payloadsRaw8` 循环），替换为一行退役说明；③ 在原位置留一行说明：该小端入口已移除、BLE 聚合帧解析统一由 `protocol/GatewayFrameCodec` 提供 |

改动仅限删除死代码与注释：**无行为变化**，未触碰任何在用代码路径、常量、协议或兼容面（`parseUnencryptedFrame`/`interpretPayloadV3`/`ParsedFrameRaw`/`DeviceReadingV3` 声明未在本次任务描述范围内，未改动）。

## 2. 预期行为（本轮交付）

1. `dengbei_care/app/src`（含 `app/src/main`）中不存在 `parseHexData` 的定义、调用点或注释残留——即工程内不存在"小端 `humidity|temperature`"这一与 IC-002 §4 的 `id|hum_be|temp_be` 布局冲突的解释入口。
2. 二进制聚合帧解析只有一处实现：`GatewayFrameCodec`，且 BLE 路径只能经 `decryptAndParseEcbFrame` 到达；MQTT 路径解析的是文本载荷（`time:temp1,...:humi1,...`），不解析二进制帧，因此不存在第二套二进制解析实现。
3. 合法帧的解码结果、`devCount==0`/帧长不足的拒绝行为与 ITEM-001 交付完全一致（回归无变化）。

## 3. 本轮验证（实现侧自检）

| 检查 | 命令 | 结果 |
|---|---|---|
| 入口与注释清零 | `grep -rn "parseHexData" dengbei_care/app/src`、`.../app/src/main` | **0 命中**（删除前：1 处定义 + 1 处注释） |
| 宿主机单元测试 | `./gradlew :app:testDebugUnitTest --offline` | `BUILD SUCCESSFUL`；4 个套件共 22 项、0 失败 0 跳过（含软件测试能力独立编写的 `verification/GatewayFrameCodecItem001*` 10 项与实现侧自检 11 项、`ExampleUnitTest` 1 项） |
| 构建 | `./gradlew :app:assembleDebug --offline` | `BUILD SUCCESSFUL`；`app/build/outputs/apk/debug/app-debug.apk` 10,285,626 B（与上一项同尺寸，无新增/删除类） |
| 编译告警 | Kotlin 编译输出 | 仍为改动前既有的 5 条（opt-in/未使用变量/deprecated override），无新增告警 |
| 差异范围 | `git diff --stat` | 仅 `MqtttService.kt`：5 insertions / 36 deletions，全部为死代码与注释 |

原始输出见 `evidence/android_test.md`（AT-002）。

## 4. 观察与交接

1. **仓库全量 grep 仍会命中非编译的参考材料**：`dengbei_care/iOS开发资料/02_Tier1_硬前置/MqtttService.kt`（旧快照副本，第 725/817 行）与 `dengbei_care/iOS开发资料/README.md`（第 67/152 行按旧行号描述该入口）。这两处属 iOS 参考文档包（readme 点 6：本轮不改 iOS；且不属 Android 源码），**有意未改动**；静态核查（TD-SW-002 T-SW-L1-03）应以 Android 源码 `app/src/main` 为范围，或显式排除 `iOS开发资料/`。已在 file_manifest 中登记该目录用途为间接参考。
2. **仍无调用点的声明（本项范围外，未改动）**：`ParsedFrameRaw`、`parseUnencryptedFrame`、`DeviceReadingV3`、`interpretPayloadV3`，以及私有扩展 `ByteArray.toHex`（其唯一引用原先只存在于本次删除的注释中，现已零引用；Kotlin 未报新增告警）。是否清理属后续队列项/维护决定，本能力不越权扩大 diff。
3. ITEM-001 已验证行为（`devCount==0` 拒绝、长度规则、逐位一致）在本项后未变，软件测试能力此前独立编写的验证用例全部继续通过。

---

# 任务项 ITEM-003（本轮完成）

**ITEM-003（队列第 3 项）**：新增纯逻辑模块 `telemetry/ReadingAttribution.kt`，把 MQTT 批量载荷 `time:temp1,...:humi1,...` 的归因从位置截断改为严格映射。
期望行为：设备数、温度个数、湿度个数三者不一致，或任一为空/含非法数值时拒绝整批（不落库、不上传、不报警）；一致时按 `MacIdBook` 顺序得到 `(devId, temperature, humidity)` 列表，且 `devId` 随读数向下游传递而不再依赖下标。

设计映射：AA-002 §5.2（MQTT 归因严格化）、D-08（禁止用列表顺序替代 ID、禁止静默错归因）、§4.2（新增 `telemetry/ReadingAttribution.kt`）；IC-002 §5；TD-SW-002 §2.1-S3、§4.1 T-SW-L0-04/05。

## 1. 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/telemetry/ReadingAttribution.kt` | 新增 | 纯 Kotlin（无 Android 依赖）：`attribute(payload, savedIds)` 解析 `time:temps:humis` 并严格归因；`Reading`/`Batch`/`Result.Success|Rejected`/`RejectionReason`；逗号后空格容忍；非有限值（NaN/±Infinity）视为非法 |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/MqtttService.kt` | 修改 | `messageArrived` 的 MQTT 归因改为调用 `ReadingAttribution.attribute(temppayload, MacIdBook.all().map{it.first})`，按 `Success`/`Rejected` 分支；删除 `minOf(...)` 静默截断与按下标处理的循环；删除仅为旧截断路径服务的重复载荷分段及其孤立字段 `readBackTemperature`/`readBackHuminity`/`currentTime` |
| `dengbei_care/app/src/test/java/com/jinyuni/dengbei_care/telemetry/ReadingAttributionTest.kt` | 新增 | 宿主机 JUnit4 自检 16 项（数目一致归因、负数/0 值、绑定顺序无关性、单设备、空格容忍、数目不一致×3、空序列×2、非法数值、非有限值、时间非法/缺失、无绑定设备、多余段兼容、拒绝结果不携带读数） |

关键实现要点：

| 项 | 改动前 | 改动后 |
|---|---|---|
| 数目不一致 | `minOf(saved, temps, humis)` 静默截断，仅打 warning，然后按下标处理 → **可能把 A 的读数记到 B** | `Rejected(COUNT_MISMATCH)`，整批不处理 |
| 空序列 / 非法数值 | `map { it.toDouble() }` 抛错被 catch → 空列表 → 早退（非法项被静默丢弃） | `Rejected(EMPTY_SERIES / INVALID_NUMBER)`，整批不处理（含 NaN/±Infinity） |
| 时间非法/缺失 | `toLongOrNull()` 失败则**沿用上一条消息的旧 `currentTime`**（旧值可能被当作本条时间写入） | `Rejected(INVALID_TIME)`（含非正时间），不落库、不上传、不报警 |
| 无绑定设备 | 日志后 `return`（跳过时间校准发布） | `Rejected(NO_BOUND_DEVICE)`，不提前 return（流程继续，与旧代码在“数目不一致”情形下的流程一致） |
| 归因载体 | 循环内 `val devId = savedIds[i].first` + 并列的 `doubles0[i]`/`doubles1[i]` | `Reading(devId, temperature, humidity)`，`devId` 随读数一起下沉 |
| 去重键 | `_bkp_temp_time != currentTime`（旧 `currentTime` 字段） | `_bkp_temp_time != batch.timeSeconds`（载荷首段，同一语义，仅在成功后推进） |

`MacIdBook.all()` 仍是唯一顺序真相源：网关 `0xA1` 配置包（`buildAllSavedIdsA1SinglePacket`）与 MQTT 归因现在取自同一个调用（此前归因也是 `MacIdBook.all()`，未改变顺序来源）。

## 2. 预期行为（本轮交付）

1. 设备数、温度个数、湿度个数三者一致且每项均为有限数值、时间为正的长整数时：得到按 `MacIdBook.all()` 顺序的 `(devId, temperature, humidity)` 列表，并逐条交给 `processTemperatureHumidityData(devId, t, h, 载荷时间)`。
2. 任一不一致/为空/非法/时间非法 → **整批拒绝**：不调用 `processTemperatureHumidityData`，因此不落库、不上传、不报警，仅记一条 `Log.w` 携带具体原因与计数。
3. 拒绝结果类型本身不携带任何读数，调用方无法误用部分结果。
4. 数值物理量程校验（>125.0℃、<0 %RH 等）不属本项（AA-002 §8 / TD-SW-002 §8-G2）。
5. 本项不改变告警判定、存储时间戳、上传、UI、服务器常量（属后续队列项）。

## 3. 本轮验证（实现侧自检）

| 检查 | 命令 | 结果 |
|---|---|---|
| 宿主机单元测试 | `./gradlew :app:testDebugUnitTest --offline` | `BUILD SUCCESSFUL`；5 套件共 **38 项、0 失败 0 跳过**（新增 `ReadingAttributionTest` 16 项；实现侧 `GatewayFrameCodecTest` 11 项；软件测试能力独立验证 3+7 项；`ExampleUnitTest` 1 项） |
| 构建 | `./gradlew :app:assembleDebug --offline` | `BUILD SUCCESSFUL`；`app-debug.apk` 10,359,240 B（较上项 +73,614 B，为新增归因模块与其接入代码） |
| 编译告警 | `./gradlew :app:compileDebugKotlin --offline` | 仍为改动前既有的 5 条（opt-in/未使用变量/deprecated override），无新增 |
| 旧截断路径清零 | `grep -n "minOf(savedIds\|using first\|doubles0\|doubles1\|tempDoubles0" MqtttService.kt` | 0 命中 |
| 未破坏前项静态口径 | `python evidence/software_static_check_no_little_endian_parser.py`（软件测试能力的独立脚本） | `RESULT: OK`（A1/A2 = 0；B1a–B1d 与 B2 均 PASS） |
| 差异范围 | `git diff --stat` | 仅 `MqtttService.kt`：21 insertions / 64 deletions；新增 1 个模块 + 1 个测试文件 |

原始输出见 `evidence/android_test.md`（AT-003）。

## 4. 观察与交接

1. **删除重复分段与孤立字段属本项直接后果**：`readBackTemperature`/`readBackHuminity`/`currentTime` 三个字段的唯一读取点就是被替换的截断式解析，替换后它们成为只写死状态，故一并移除（与其一同被移除的重复 `split(":")` 也消除了“同一载荷两处解析”）。未触碰 `readbackdata`（改动前即无使用）等无关遗留。
2. **不再提前 `return`**：拒绝整批时流程继续到时间校准发布块（旧代码仅在“空序列/无绑定设备”两种情形下提前 return；在“数目不一致”情形下本就继续）。若后续希望严格保持“拒绝即不发布校准”，属新行为需另行确认。
3. **设备数 > 50 的边界**：写入网关的 `0xA1` 包最多 50 个 ID（`take(maxCount)`），而归因比较对象是全部已绑定 ID；设备超过 50 时两者数量不等 → 按本项设计**拒绝整批**（安全优先，不错归因）。是否应改为“只比对前 50 个”属产品/协议决定，登记为后续观察项。
4. **本项不涉及**：告警时间窗（ITEM-005）、存储真实时间戳（ITEM-007）、云上传（ITEM-008/009）、UI（ITEM-011/012）。

---

# 任务项 ITEM-004（本轮完成）

**ITEM-004（队列第 4 项）**：新增 `cloud/CloudConfig.kt` 集中服务器常量，并把 `MqtttService` 的 MQTT broker 地址改为 `ssl://8.140.23.253:8883`、注册/上传 HTTP 基址改为 `http://8.140.23.253:5000`，同时在 `res/xml/network_security_config.xml` 追加放行 `8.140.23.253` 明文。
期望行为：App 内不再出现 `117.72.84.210`，TLS 信任锚仍为 `assets/jd-ca.crt`、`TLSv1.2`、用户名=MAC、密码=MAC+`&^!A:z?` 全部保持原样，资源合并与构建通过。

设计映射：AA-002 §5.6（服务器迁移表）、D-13（地址集中单一常量对象）；readme 修改点 5；`服务器迁移记录.md` §二/§三/§五/§八；TD-SW-002 §2.1-S4、§4.1 T-SW-L0-13、§4.3 T-SW-L1-05/L1-06。

## 1. 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/cloud/CloudConfig.kt` | 新增 | 平台服务器唯一常量来源：`SERVER_HOST="8.140.23.253"`、`MQTT_BROKER_PORT=8883`、`HTTP_PORT=5000`、`MQTT_BROKER_URI="ssl://…:8883"`、`HTTP_BASE_URL="http://…:5000"`；注明信任锚/认证/旧主机退役 |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/MqtttService.kt` | 修改 | `serverUri` 由硬编码旧地址改为 `CloudConfig.MQTT_BROKER_URI`（+ import；`setupSSL()` 未改） |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/ui/zhuce/ZhuCeViewModel.kt` | 修改 | Retrofit `baseUrl` 由硬编码旧地址改为 `CloudConfig.HTTP_BASE_URL`（+ import） |
| `dengbei_care/app/src/main/res/xml/network_security_config.xml` | 修改 | **追加** `8.140.23.253` 明文放行块（保留既有 `117.72.84.210`、`192.168.4.1` 两块，未新增其它主机；补上文件末尾换行） |
| `dengbei_care/app/src/test/java/com/jinyuni/dengbei_care/cloud/CloudConfigTest.kt` | 新增 | 宿主机 JUnit4 自检 3 项：常量值/组合关系；明文放行含新主机且域名集合仍为 3 个（无其它主机）、三个 `domain-config` 均显式允许明文 |

兼容面核查（均未改动，逐项 grep 确认）：`assets/jd-ca.crt`（md5 `f1a8212e3c690d57894b8a3a5fd901ab`，与迁移记录 §三 一致）、`SSLContext.getInstance("TLSv1.2")`、`options.userName = MAC` / `options.password = MAC + "&^!A:z?"`、`AES/ECB/NoPadding` 与 `APP_AES_KEY16`、`requestMtu(240)`、`keepAliveInterval = 20`、topic 规则、`ApiService` 既有 5 个端点。

旧地址字面量的最终分布（`app/src/**`）：**Kotlin 源码（main+test）0 处**；仅 `network_security_config.xml` 保留 1 处——该处是设计明确要求的“追加而非替换”（AA-002 §5.6 / T-SW-L1-06），不属连接目标。

## 2. 预期行为（本轮交付）

1. MQTT 客户端连接 `ssl://8.140.23.253:8883`；TLS 握手仍用 `assets/jd-ca.crt` 作信任锚、`TLSv1.2`、用户名=网关 MAC、密码=MAC+`&^!A:z?`（仅 IP 变化，认证与证书不变）。
2. 注册/登录/后续 `/upload_data` 的 Retrofit 基址为 `http://8.140.23.253:5000`，并已在网络安全配置中允许该主机明文（Android 9+ 默认禁明文）。
3. 服务器地址只在 `CloudConfig` 一处定义；其余代码引用常量，避免“记录说改了、代码没改”的漂移。
4. 旧主机不再作为任何连接目标；明文放行集合不扩大（仍为 3 个主机）。

## 3. 本轮验证（实现侧自检）

| 检查 | 命令 | 结果 |
|---|---|---|
| 宿主机单元测试 | `./gradlew :app:testDebugUnitTest --offline` | `BUILD SUCCESSFUL`；7 套件共 **49 项、0 失败 0 跳过**（新增 `CloudConfigTest` 3 项） |
| 构建/资源合并 | `./gradlew :app:assembleDebug --offline` | `BUILD SUCCESSFUL`；`app-debug.apk` 10,359,236 B（资源 XML 被正常合并） |
| 旧 IP 清零（Kotlin） | `grep -rn "117.72.84.210" --include=*.kt app/src` | **0 命中**（修改前：`MqtttService.kt:98`、`ZhuCeViewModel.kt:11` 两处硬编码连接地址） |
| 旧 IP 分布（全 `app/src`） | `grep -rn "117.72.84.210" app/src` | 1 命中，仅 `res/xml/network_security_config.xml`（设计要求的追加保留项） |
| 新 IP 集中性 | `grep -rn "8.140.23.253" app/src/main --include=*.kt` | 仅 `cloud/CloudConfig.kt:18` |
| 编译告警 | `./gradlew :app:compileDebugKotlin --offline` | 仍为既有 5 条，无新增 |
| 差异范围 | `git diff --stat` | `MqtttService.kt` +4/−1、`ZhuCeViewModel.kt` +2/−1、`network_security_config.xml` +8/−1，加 2 个新文件 |

原始输出见 `evidence/android_test.md`（AT-004）。

## 4. 观察与交接

1. **旧主机明文放行保留**：按设计与测试设计的“追加”口径保留，代价是全仓字面 grep 仍有 1 处命中（仅 XML）。若项目要求“全仓字面清零”，则需把该条目删除（属于设计变更，需上游确认），本能力未自行改判。
2. **真机连通性未验证**（本能力工具边界无网络探测）：`ssl://8.140.23.253:8883` 的 TLS/认证与 `/upload_data` 可达性由测试能力按 TD-SW-002 T-SW-L2-01/L2-12 执行；证书链已由迁移记录 §四 实测（`Verify return code: 0`）。未产生 connectivity 证据。
3. **仍引用旧地址的非编译材料**（未改动，登记交接）：`dengbei_care/_REFACTOR_NOTES.md:39`（描述旧常量）、`dengbei_care/iOS开发所需资料清单.md:16`、`dengbei_care/iOS版功能需求文档.md:157/399/580`、`dengbei_care/iOS开发资料/**`（iOS 参考快照与 README）。文档更新属队列 ITEM-014（`readme.txt`/用户说明书）或 iOS 资料维护方，不在本项。
4. **本项不涉及**：`/upload_data` 端点与上传编排（ITEM-008/009）、SQLite v3（ITEM-006）、告警时间窗（ITEM-005）、UI（ITEM-011/012）、版本/文档（ITEM-014）。

---

# 任务项 ITEM-005（本轮完成）

**ITEM-005（队列第 5 项）**：新增纯逻辑模块 `telemetry/AlarmEvaluator.kt`，把温度湿度下降报警从「最近第 N 个样本」改为基于时间戳的时间窗判定（窗口 5/10/15 分钟，基准为不晚于 `now-窗口` 且不早于 `now-窗口-回溯容差` 的最新样本），阈值超限与紧急（>65.0℃）判定语义保持现状。
期望行为：输入当前读数与候选历史样本即可得到 `未设置报警 / 温度湿度下降报警 / 温湿度超限报警 / 无报警` 与严重级别；基准样本缺失或无历史时下降分支不成立；恰好等于阈值不触发；评估过程不写入任何伪造温度或湿度值。

设计映射：AA-002 §7（告警展示与判定）、D-04（时间窗替代第 N 个样本）、D-05（基准来自库/单一数据源）、§4.2（新增 `telemetry/AlarmEvaluator.kt`）；TD-SW-002 §4.1 T-SW-L0-06..L0-10。

## 1. 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/telemetry/AlarmEvaluator.kt` | 新增 | 纯 Kotlin（无 Android 依赖）：`evaluate(now, temperature, humidity, parameters, history, lookbackSlackSeconds)` → `Result(outcome, message, severity, matchedWindowMinutes)`；`Parameters`/`Sample`/`Severity`/`Outcome`；常量 `EMERGENCY_TEMP_C=65.0`、`DROP_WINDOW_MINUTES=[5,10,15]`、`DEFAULT_LOOKBACK_SLACK_SECONDS=30min`、五条文案字面值 |
| `dengbei_care/app/src/test/java/com/jinyuni/dengbei_care/telemetry/AlarmEvaluatorTest.kt` | 新增 | 宿主机 JUnit4 自检 23 项（L0-06..L0-10 全覆盖 + 窗口选择/边界/纯函数性） |

判定语义（逐条对照）：

| 项 | 改动前（`MqtttService.processTemperatureHumidityData`） | 本模块 |
|---|---|---|
| 下降基准 | 内存 `DeviceHistory.data5MinAgo/data10MinAgo/data15MinAgo`，实为**最近第 1/2/3 个样本** | 每个窗口取 `[now-W-slack, now-W]` 内**时间最新**样本（含两端边界）；无合格基准该窗口不成立 |
| 下降条件 | `base.temp-cur.temp > tempDrop && base.humi-cur.humi > humiDrop` | 同（严格 `>`、AND、两个窗口任一命中即触发） |
| 回溯容差 | 无（任何旧样本都可能当基准；仅 30 min 过期清理） | `DEFAULT_LOOKBACK_SLACK_SECONDS = 30 min`，可传入覆盖 |
| 阈值超限 | `temp>tempmax \|\| temp<tempmin \|\| humi>humimax \|\| humi<humimin` | 同（严格比较） |
| 开关语义 | 两开关都关→`未设置报警`；只开一个时只判该项；都开时**先下降后超限**，同时成立超限文案覆盖 | 同（`NOT_CONFIGURED` / `DROP` / `THRESHOLD`） |
| 紧急 | `temperature > 65f` 覆盖前述结果，文案/级别固定，每次通知 | 同（`EMERGENCY`，`notifiesImmediately=true`），且与开关无关（与改动前一致） |
| “启用但未触发”文案 | 保留变量初值 `未设置报警`（与“两开关都关”同文案） | 按已批准设计/任务描述区分为 `无报警`（`Outcome.NONE`）；两种情形均为 NORMAL，卡片不展示、不落库，**生产可见行为不变** |
| 伪造数据 | — | 纯函数，不修改入参；`Result` 不携带任何温度/湿度字段 |

**本项仅交付模块与自检**：未修改 `MqtttService`——把 `processTemperatureHumidityData` 改为调用本模块属队列 **ITEM-010**（AA-002 §4.2 的“Service 编排接入”），因此当前生产路径仍用旧的内存窗口逻辑，本项不影响运行行为。

## 2. 预期行为（本轮交付）

1. `evaluate` 的输出是 `Outcome`（`NOT_CONFIGURED`/`NONE`/`DROP`/`THRESHOLD`/`EMERGENCY`）+ 文案 + `Severity`（`NORMAL`/`WARNING`/`ALARM`）+ 命中的下降窗口（分钟，未命中为 `null`）。
2. 下降仅在存在合格基准且温度与湿度**同时**严格超过阈值时成立；无历史/样本全部过早/样本全部过新均不成立。
3. 阈值超限与紧急语义与改动前逐字一致；恰好等于阈值/65.0℃ 不触发。
4. 输出不含任何温度/湿度数值，不产生伪造样本；同一输入多次调用结果相同且不修改入参列表。

## 3. 本轮验证（实现侧自检）

| 检查 | 命令 | 结果 |
|---|---|---|
| 宿主机单元测试 | `./gradlew :app:testDebugUnitTest --offline` | `BUILD SUCCESSFUL`；8 套件共 **72 项、0 失败 0 跳过**（新增 `AlarmEvaluatorTest` 23 项） |
| 构建/打包 | `./gradlew :app:assembleDebug --offline` | `BUILD SUCCESSFUL`；`app-debug.apk` 10,359,236 B；直接读 APK 多 dex 确认 `AlarmEvaluator`/`ReadingAttribution`/`CloudConfig`/`GatewayFrameCodec` 均已打包 |
| 编译告警 | `./gradlew :app:compileDebugKotlin --offline` | 仍为既有 5 条，无新增 |
| 差异范围 | `git status --short` | 仅新增 1 个模块 + 1 个测试文件（未改任何现有产品文件） |

原始输出见 `evidence/android_test.md`（AT-005）。

## 4. 观察与交接

1. **窗口重叠与 `matchedWindowMinutes` 语义**：由于回溯容差 30 min 远大于窗口差（5 min），一个较旧样本可同时满足多个窗口；本模块返回**最先满足**（W 递增）的窗口。判定结果（是否报警）不受影响，该字段仅供日志/证据使用。实测边界：`now-35min` → 5 分钟窗口（下界含边界）、`now-36min` → 10 分钟、`now-41min` → 15 分钟、`now-46min` → 全部不成立。
2. **“启用但未触发”文案改为 `无报警`**（与两开关都关的 `未设置报警` 区分）：与已批准设计/任务描述一致；因两者均 `NORMAL`，卡片不显示且不落 `alarm_events`，生产可见行为不变。若上游要求严格保持旧占位文案，仅需改一处常量。
3. **接线与基准来源属后续项**：① `processTemperatureHumidityData` 改用本模块 + 把 `Severity` 映射到 `com.jinyuni.dengbei_care.Severity` → ITEM-010；② 基准样本的来源（设计 D-05 要求从本地库按 devId+时间范围查询，以支持进程重启后仍可判定）→ ITEM-006/007（库 API）与 ITEM-010（接线）；TD-SW-002 §8-G4 已登记该缺口。
4. **本项不涉及**：SQLite v3 与真实时间戳（ITEM-006/007）、云上传（ITEM-008/009）、UI 呈现（ITEM-011/012）、通知去抖（900 s，仍在 Service，属 ITEM-010 保持不变）。

---

# 任务项 ITEM-006（本轮完成）

**ITEM-006（队列第 6 项）**：扩展 `TemperatureDatabaseHelper` 到 `DATABASE_VERSION = 3`：新增 `pending_uploads(dev_id, time, temperature, humidity, attempts, next_attempt_at, PRIMARY KEY(dev_id, time))` 表与其 `next_attempt_at` 索引；迁移对既有 `temperature` / `alarm_events` 结构与数据保持不动。
期望行为：v2 数据库升级到 v3 后历史温湿度与报警记录仍可查；重复创建不报错；迁移异常时按既有回退策略处理且不崩溃。

设计映射：AA-002 §5.4（SQLite v3 schema 与迁移）、D-07（待发箱幂等键）、D-15（升版本但既有表结构与语义不动）、§4.2；TD-SW-002 §4.2 T-SW-L0d-01/03/06。

## 1. 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/TemperatureDatabaseHelper.kt` | 修改 | ① `DATABASE_VERSION` 2 → 3；② 新增 `TABLE_PENDING_UPLOADS`/`COLUMN_PENDING_ATTEMPTS`/`COLUMN_PENDING_NEXT_ATTEMPT_AT` 与两条 DDL（`CREATE TABLE IF NOT EXISTS`、`CREATE INDEX IF NOT EXISTS`）；③ 新增 `MIGRATION_V2_TO_V3` 步骤列表；④ `onCreate` 新装也建待发箱表与索引；⑤ `onUpgrade` 新增 `oldVersion < 3` 块；⑥ 既有四条 DDL 由 `private` 改为公开常量（仅可见性，内容逐字未变，便于确定性核查） |
| `dengbei_care/app/src/test/java/com/jinyuni/dengbei_care/db/DatabaseSchemaV3Test.kt` | 新增 | 宿主机 JUnit4 自检 9 项（见 §3） |

新增表结构（与设计 §5.4 一致）：

```sql
CREATE TABLE IF NOT EXISTS pending_uploads (
  device_id TEXT, time INTEGER, temperature REAL, humidity REAL,
  attempts INTEGER DEFAULT 0, next_attempt_at INTEGER DEFAULT 0,
  PRIMARY KEY(device_id, time))
CREATE INDEX IF NOT EXISTS idx_pending_uploads_next ON pending_uploads(next_attempt_at)
```

迁移与回退策略：

| 场景 | 行为 |
|---|---|
| 新装（`onCreate`） | 建 `temperature`、`alarm_events`、两索引 + `pending_uploads` 与 `next_attempt_at` 索引 |
| v2 → v3 | 仅执行 `MIGRATION_V2_TO_V3`（两个 `IF NOT EXISTS` 语句）；**不** DROP/ALTER/UPDATE 任何既有表 |
| v1 → v3 | 先跑既有 v1→v2 路径（含其原有重建回退），再跑 v2→v3 块（`IF NOT EXISTS` 重复执行无害） |
| 重复创建/重复迁移 | `IF NOT EXISTS` 幂等，不报错 |
| 迁移异常 | 第一次失败：记录日志 → 清理同名冲突对象（`DROP VIEW/TABLE IF EXISTS pending_uploads`）→ 重试；仍失败：记录日志并继续（**不抛异常**，App 不崩溃，既有温湿度/报警读写保持可用）。**不**采用 v1→v2 那种“重建全部表”的破坏性回退，因为那会丢历史数据，与本项“既有结构与数据不动”相矛盾 |

## 2. 预期行为（本轮交付）

1. `DATABASE_VERSION == 3`；新装与 v2 升级后 `pending_uploads` 表及其 `next_attempt_at` 索引均存在，且 `(device_id, time)` 为复合主键（保证同一设备同一时刻只入队一条）。
2. `temperature` 与 `alarm_events` 的 DDL、索引与语义逐字未变；v2→v3 迁移语句不含对既有表的任何 DROP/ALTER/UPDATE，因此历史温湿度与报警记录仍可查（设备侧实测属测试能力，见 §4）。
3. 迁移可重复执行（`IF NOT EXISTS`）；异常时不死锁不崩溃，最坏情况下待发箱不可用但既有功能正常。

## 3. 本轮验证（实现侧自检）

| 检查 | 命令 | 结果 |
|---|---|---|
| 宿主机单元测试 | `./gradlew :app:testDebugUnitTest --offline` | `BUILD SUCCESSFUL`；10 套件共 **91 项、0 失败 0 跳过**（新增 `DatabaseSchemaV3Test` 9 项） |
| 构建 | `./gradlew :app:assembleDebug --offline` | `BUILD SUCCESSFUL`；`app-debug.apk` 10,360,124 B |
| 全量重编译告警 | `./gradlew :app:compileDebugKotlin --offline --rerun` | 11 条 warning 全部位于改动前既有位置（`MqtttService` 5、`TemperatureDatabaseHelper.storeAlarmEvent` 的 `rowId` 冗余初值 1（未改动函数）、`MacIdBox`/`VibrationPlayer`/UI 4），**无一条指向本项新增代码** |
| 既有调用点 | `grep -rn "DATABASE_VERSION\|pending_uploads" app/src --include=*.kt`（除本文件与新增测试） | 无其它引用；`DatabaseHelperInstance` 与既有查询 API 未变 |
| 差异范围 | `git status --short` | 仅 `TemperatureDatabaseHelper.kt` 修改 + 1 个新增测试文件 |

新增 9 项用例：`databaseVersion_isThree`、`pendingUploadsTable_hasRequiredColumnsAndCompositePrimaryKey`、`pendingUploadsIndex_coversNextAttemptAt_andIsIdempotent`、`tableAndColumnNames_areStable`、`temperatureTable_ddlIsUnchangedFromV2`、`alarmEventsTable_ddlIsUnchangedFromV2`、`existingIndexes_areUnchangedFromV2`、`migrationV2ToV3_onlyCreatesNewObjects`、`migrationV2ToV3_neverReferencesLegacyTables` —— **9/9 PASS**。

原始输出见 `evidence/android_test.md`（AT-006）。

## 4. 观察与交接

1. **设备侧迁移未在本轮实测**：`SQLiteOpenHelper` 的真实升级路径（v2→v3 保数据、冲突对象回退、既有查询 API 结果一致）需要 Android 运行时与 debug 包，属软件测试能力按 TD-SW-002 T-SW-L0d-01/02/03/06 执行；本项只提供宿主机可确定断言的 schema/迁移形状证据。
2. **待发箱的读写 API 不在本项**：本项只建表与索引（任务描述范围）。`enqueue/peekBatch/ack/markFailed` 等 DAO 与上传编排属 ITEM-009（AA-002 §4.2 `cloud/UploadOutbox.kt`）；若迁移最终失败导致表缺失，待发箱访问需容忍该情况（已在本节登记，供 ITEM-009 处理）。
3. **入库真实时间戳与单条写入 API**属 ITEM-007（本项未改 `storeTemperatureData` 的 60 s 回填行为）。
4. **本项不涉及**：上传编排（ITEM-009）、Service 接线（ITEM-010）、UI（ITEM-011/012）、版本/文档（ITEM-014）。

---

# 任务项 ITEM-007（本轮完成）

**ITEM-007（队列第 7 项）**：修正本地入库时间戳：新增单条读数写入 API（devId、真实采样时间戳、温度、湿度、守护标志），BLE 路径使用接收时刻、MQTT 路径使用 payload 首段网关时间，并让现有调用点不再按 60 秒间隔回填历史时间戳。
期望行为：写入行的 `time` 等于该读数的真实采样时间；同一 `(dev_id, time)` 重复写入不产生重复行；守护关闭（`storeflag != 1`）时不写入任何行。

设计映射：AA-002 §5.4（写入：单条读数使用真实采样时间；`INSERT OR REPLACE` 幂等）、D-06（移除 60 s 回填）、§4.2；TD-SW-002 §4.2 T-SW-L0d-04、§4.3 T-SW-L1-04。

## 1. 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/TemperatureDatabaseHelper.kt` | 修改 | ① 删除列表式 `storeTemperatureData(temperatureData, humidityData, timenow, devId, storeflag)`（内含 `time = timenow - ((size-1-i) * 60L)` 回填）；② 新增 `storeTemperatureReading(devId, time, temperature, humidity, storeflag): Long`（单条、真实时间戳、`INSERT OR REPLACE` 幂等、门控返回 -1）；③ 新增公开常量 `SQL_INSERT_TEMPERATURE_READING` 与纯函数 `isGuardActive(storeflag)`；④ 顶层包装函数改为 `storeTemperatureReading(...)` |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/MqtttService.kt` | 修改 | `processTemperatureHumidityData` 的存储段：删除 `averageList0/averageList1` 临时列表，改调 `storeTemperatureReading(this, devId, currentTime, temperature, humidity, humistartflag)`；`currentTime` 即真实采样时间（BLE 无 `timeOverride` → 接收时刻；MQTT 传 `batch.timeSeconds` = payload 首段） |
| `dengbei_care/app/src/test/java/com/jinyuni/dengbei_care/db/TemperatureWriteItem007Test.kt` | 新增 | 宿主机 JUnit4 自检 4 项（见 §3） |

关键点：

| 项 | 改动前 | 改动后 |
|---|---|---|
| 写入形态 | 列表 + `for` 循环，按 `60L` 倒推历史时间戳 | 单条 + 显式 `time` 参数（调用方传入真实采样时间） |
| 时间戳来源 | `timenow - (size-1-i)*60`（列表长度 >1 时产生虚假采样时刻） | BLE：接收时刻；MQTT：payload 首段（网关时间）——与 AA-002 §8 一致 |
| 幂等 | `INSERT OR REPLACE` + `PK(time, device_id)`（保留） | 同（现由 `SQL_INSERT_TEMPERATURE_READING` 常量承载） |
| 守护门控 | `if (storeflag != 1) return` | `if (!isGuardActive(storeflag)) return -1L`（语义不变，纯函数可独立断言） |

注：实际调用点在 `processTemperatureHumidityData` 中本就只传单个元素列表，因此**生产行为等价**；本项消除的是“列表式 API 潜在回填 + 两条入口时间语义不明”的隐患，并把时间语义固定到调用点。

## 2. 预期行为（本轮交付）

1. `storeTemperatureReading` 写入行的 `time` 就是传入的 `time`（真实采样时间），不存在 `time-60`/`time-120` 类偏移。
2. 同一 `(dev_id, time)` 重复写入由 `INSERT OR REPLACE` + 复合主键 `(time, device_id)` 幂等，行数不增加。
3. `storeflag != 1`（守护关闭）时**不写入任何行**并返回 -1。
4. BLE 与 MQTT 两条入口的时间语义：BLE = 接收时刻，MQTT = payload 首段网关时间（由调用点传入，无额外改写）。
5. 既有查询 API（图表范围/最新值/sparkline/日报/报警）未变。

## 3. 本轮验证（实现侧自检）

| 检查 | 命令 | 结果 |
|---|---|---|
| 宿主机单元测试 | `./gradlew :app:testDebugUnitTest --offline` | `BUILD SUCCESSFUL`；11 套件共 **95 项、0 失败 0 跳过**（新增 `TemperatureWriteItem007Test` 4 项） |
| 构建 | `./gradlew :app:assembleDebug --offline` | `BUILD SUCCESSFUL`；`app-debug.apk` 10,359,865 B（较 ITEM-006 的 10,360,124 B 减少 259 B，为删除列表式写入） |
| 60 s 回填清零 | `grep -rn "60L" app/src/main --include=*.kt` | 仅 `telemetry/AlarmEvaluator.kt`（时间窗算术，非入库路径）；**入库路径 0 命中** |
| 旧 API 清零 | `grep -rn "storeTemperatureData\|averageList0\|averageList1" app/src/main --include=*.kt` | 0 命中 |
| 全量重编译告警 | `./gradlew :app:compileDebugKotlin --offline --rerun` | 11 条，与 ITEM-006 同一集合（无新增；`TemperatureDatabaseHelper` 仅 `storeAlarmEvent` 的既有 `rowId` 冗余初值） |
| 差异范围 | `git status --short` | 仅 `TemperatureDatabaseHelper.kt`、`MqtttService.kt` 修改 + 1 个新增测试文件 |

新增 4 项用例：`insertStatement_isReplaceInto_withFourBoundColumns`、`idempotency_isGuaranteedByCompositePrimaryKeyAndReplace`、`writtenTimeIsTheCallerProvidedSampleTime_noOffsetBackfill`、`guardGate_onlyStoreflagOneAllowsWriting` —— **4/4 PASS**。

原始输出见 `evidence/android_test.md`（AT-007）。

## 4. 观察与交接

1. **设备侧行级验证未在本轮执行**：真实写入行的时间值、重复写入后行数、守护关闭时 0 行 —— 需 Android 运行时，属软件测试能力 T-SW-L0d-04 范围；本项提供语句级/门控级确定性证据。
2. **`INSERT OR REPLACE` 的语义**：同 `(time, device_id)` 重复写入会替换原行（行 id 可能变化），满足“不产生重复行”；若后续需要“保留首次值”，属新需求需另行确认。
3. **`AlarmEvaluator` 中的 `60L`** 为时间窗分钟→秒换算，与入库回填无关；若后续静态门禁采用全仓 `60L` 字面清零，需显式排除该处。
4. **iOS 参考快照**（`dengbei_care/iOS开发资料/03_Tier2_行为对齐/TemperatureDatabaseHelper.kt` 等）仍含旧列表式 API 与 60 s 回填：非编译、不参与构建，本轮不改 iOS（readme 点 6），登记为交接项。
5. **本项不涉及**：待发箱 DAO 与上传编排（ITEM-009）、`processTemperatureHumidityData` 接入 `AlarmEvaluator`（ITEM-010）、UI（ITEM-011/012）。

---

# 任务项 ITEM-008（本轮完成）

**ITEM-008（队列第 8 项）**：在 `ui/zhuce/ApiService.kt` 新增 `POST /upload_data` 接口与对应请求体数据类（`phone`、`mac`、`readings[{devId,time,temperature,humidity}]`），并新增纯逻辑模块 `cloud/UploadPayload.kt` 构造该请求体。
期望行为：字段名与服务器契约一致；`temperature` 为摄氏度、`humidity` 为 %RH、`time` 为 Unix 秒（由 0.1℃/0.1 %RH 原始值除以 10 得到）；`readings` 中的 `devId` 来自记录自身归因结果；既有 5 个端点签名与数据类字段不变。

设计映射：AA-002 §5.5（`/upload_data` 契约、单位、归属、幂等）、§4.2（新增 `cloud/UploadPayload.kt`）；`服务器迁移记录.md` §八；TD-SW-002 §4.1 T-SW-L0-11、§4.2 T-SW-L0d-07。

## 1. 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/ui/zhuce/ApiService.kt` | 修改（纯追加） | 新增 `@Headers("Content-Type: application/json")` + `@POST("/upload_data")` 的 `suspend fun uploadData(@Body data: SensorUploadData): Response<Void>`；新增 `SensorReading(devId, time, temperature, humidity)` 与 `SensorUploadData(phone, mac, readings)`；**既有 5 个端点与 6 个既有数据类逐字未改** |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/cloud/UploadPayload.kt` | 新增 | 纯 Kotlin（无 Android 依赖）：`fromAttributed(phone, mac, timeSeconds, readings)`（生产路径：已归因读数 + 批时间）与 `build(phone, mac, readings)`（待发箱补传路径：逐行时间）；空读数集返回 null |
| `dengbei_care/app/src/test/java/com/jinyuni/dengbei_care/cloud/UploadPayloadTest.kt` | 新增 | 宿主机 JUnit4 自检 11 项（见 §3） |

契约与单位：

```json
{"phone":"13800000000","mac":"1010100000A1",
 "readings":[{"devId":"A1B2C3D4","time":1699999999,"temperature":25.5,"humidity":60.0}]}
```

| 项 | 处理 |
|---|---|
| 字段名 | DTO 属性名与契约逐字一致（`phone`/`mac`/`readings`；`devId`/`time`/`temperature`/`humidity`），Gson 直接序列化即得契约 JSON（无 `@SerializedName` 需求） |
| 单位 | `temperature` ℃、`humidity` %RH。线上原始值是 ×10 整数，**除以 10 的换算已在解码/归因阶段完成**（`GatewayFrameCodec` → `ReadingAttribution`）；`UploadPayload` 不做第二次缩放（避免双重换算），负温符号保持 |
| `time` | Unix 秒；生产路径用批时间（MQTT = payload 首段、BLE = 接收时刻），待发箱路径逐行时间 |
| `devId` | 来自 `ReadingAttribution.Reading.devId`（即记录自身的 4 B ID），不依赖列表下标 |
| 幂等 | 服务端 `UNIQUE(dev_id, time)` + `INSERT IGNORE`；App 侧由待发箱 `(dev_id, time)` 去重（ITEM-009 实现） |
| 空集 | 返回 null ⇒ 不产生请求（与 TD-SW-002 T-SW-L0-11 一致） |

## 2. 预期行为（本轮交付）

1. `uploadData` 以 `application/json` POST 到 `{CloudConfig.HTTP_BASE_URL}/upload_data`，请求体字段名与服务器契约逐字一致。
2. 载荷中的温度/湿度为 ℃/%RH（不是 ×10 原始值），负温保留符号；`time` 为 Unix 秒整数。
3. `readings` 的 `devId` 来自归因结果；空读数集不构造请求体。
4. 既有 5 个端点路径/方法/数据类字段与 `LoginResponse` 未变（纯追加）。

## 3. 本轮验证（实现侧自检）

| 检查 | 命令 | 结果 |
|---|---|---|
| 宿主机单元测试 | `./gradlew :app:testDebugUnitTest --offline` | `BUILD SUCCESSFUL`；13 套件共 **110 项、0 失败 0 跳过**（新增 `UploadPayloadTest` 11 项） |
| 构建 | `./gradlew :app:assembleDebug --offline` | `BUILD SUCCESSFUL`；`app-debug.apk` 10,359,867 B |
| 全量重编译告警 | `./gradlew :app:compileDebugKotlin --offline --rerun` | 11 条（与 ITEM-007 同一集合），无新增 |
| 差异范围 | `git diff --stat` | `ApiService.kt` +24/−1（仅末尾换行差异）；新增 2 个文件 |

新增 11 项用例：`serializedJson_hasExactContractFieldNames`（逐字 JSON 断言）、`unitsAreCelsiusAndPercentRh_notRawX10`（含负温）、`timeIsUnixSeconds_passedThroughUnchanged`、`devIdComesFromAttributedReading_notFromIndex`、`multipleReadings_shareBatchTime_andKeepOrder`、`emptyReadings_produceNoPayload`、`build_keepsPerReadingTimes_forOutboxBackfill`、`dtoFieldNames_matchServerContract`、`apiService_keepsFiveExistingEndpoints_andAddsUploadData`、`existingRequestDataClasses_areUnchanged`、`uploadDataEndpoint_usesJsonContentType` —— **11/11 PASS**。

原始输出见 `evidence/android_test.md`（AT-008）。

## 4. 观察与交接

1. **响应体未建模**：接口用 `Response<Void>`（既有风格，仅需 2xx 判定成功）。服务端实际返回 `inserted` 计数（迁移记录 §八）；若后续需在 App 侧读取该计数，属新增需求（一行类型改动），当前 T-SW-L2-12 的 `inserted` 核对由测试能力直接 `curl` 完成。
2. **手机号/守护门控不在本项**：`phone` 与 `mac` 由调用方（ITEM-009 编排）从 `registerphone`/`mac_addr` 取值；空手机号不入队属 ITEM-009（D-09）。本模块只做结构组装（不读 SharedPreferences）。
3. **`UploadPayload` 依赖 `ui.zhuce` 的 DTO**：因任务要求数据类放在 `ApiService.kt`，模块引用该 DTO 属于可接受的单向依赖（DTO 无 Android 依赖）；若后续将 DTO 下沉到 `cloud/`，属结构优化，需同步 `ApiService` 导入。
4. **本项不涉及**：待发箱 DAO/重试与上传编排（ITEM-009）、Service 接线（ITEM-010）、UI（ITEM-011/012）、版本/文档（ITEM-014）。

---

# 任务项 ITEM-009（本轮完成）

**ITEM-009（队列第 9 项）**：新增 `cloud/UploadOutbox.kt` 与 `cloud/CloudUploadRepository.kt` 实现待发箱与上传编排：入队按 `(dev_id, time)` 幂等，成功（HTTP 成功）即删除，失败保留并按有界次数与退避重试（单批条数、退避上下限、最大尝试次数集中于一处常量），上传在独立作用域执行不阻塞 BLE 与 MQTT 主链路。
期望行为：断网期间读数进入待发箱且不丢失；网络恢复后按 `(dev_id, time)` 不重复地补传；重复上传由服务端唯一键与本地待发箱双重去重；守护关闭或本机未注册手机号时不入队也不发起请求。

设计映射：AA-002 §5.5（上传编排/重试/幂等/门控）、§6（独立作用域与触发点）、D-07（待发箱幂等）、D-09（守护/手机号门控）、§4.2；TD-SW-002 §4.1 T-SW-L0-12（①–⑧）、§8-G3。

## 1. 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/cloud/UploadOutbox.kt` | 新增 | `UploadOutboxRow`、`UploadOutboxStore` 接口、`UploadOutbox` 策略（幂等入队/到期取批/ack 删除/失败记账）；`SqliteUploadOutboxStore` 适配 ITEM-006 的 `pending_uploads` 表（表缺失或数据库异常时降级为空操作并记日志，不崩溃） |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/cloud/CloudUploadRepository.kt` | 新增 | 编排：`UploadApi`/`UploadGate` 抽象、`Config`（**单批 50 / 退避 60s→1800s / 最多 20 次**集中一处）、`isGateOpen()`、`enqueueAndUpload()`、`triggerUpload()`（独立 `CoroutineScope`）、`suspend uploadPendingOnce()`（`Mutex.tryLock` 串行）；生产适配 `PrefsUploadGate`（`MyPrefs`）、`CloudApiClient`（基址取自 `CloudConfig`）、`RetrofitUploadApi`（`POST /upload_data`，2xx = 成功） |
| `dengbei_care/app/src/test/java/com/jinyuni/dengbei_care/cloud/CloudUploadRepositoryTest.kt` | 新增 | 宿主机 JUnit4 自检 14 项（内存 Outbox + 伪 Api，见 §3） |
| `dengbei_care/app/build.gradle.kts` | 修改 | 新增 `testOptions { unitTests.isReturnDefaultValues = true }`，使宿主机单测可加载含 `android.util.Log` 的云上传类（AGP 标准做法，仅影响单元测试） |

行为与判定点：

| 判定点（T-SW-L0-12） | 实现 |
|---|---|
| ① 成功（2xx）→ 删除 | `uploadPendingOnce` 成功后逐行 `outbox.ack`（DELETE） |
| ② 失败（超时/5xx）→ 保留 + `attempts+1` + `next_attempt_at` 退避 | 失败逐行 `markFailed`，退避 = `Config.backoffSeconds(attempts)` |
| ③ 达最大尝试次数后不再自动重试 | 取批条件 `attempts < maxAttempts`；行保留在待发箱 |
| ④ 单批条数上限（50）分批 | `Config.batchLimit` + `LIMIT ?` |
| ⑤ 守护关闭 → 不入队不发请求 | `isGateOpen()`（`measure_state == "stopping"`） |
| ⑥ 手机号为空 → 不入队不发请求 | `isGateOpen()` 要求 `registerphone` 非空 |
| ⑦ 同 `(dev_id,time)` 重复入队只保留一条 | `UploadOutboxStore.insertIfAbsent`（先查后 `INSERT OR IGNORE`，事务内） |
| ⑧ 串行：不产生并发重复请求 | `Mutex.tryLock()`，重入返回 `skipped="busy"` |
| 失败不抛异常到调用方 | `api.upload` 包 try/catch；上传在 `scope` 上执行（不阻塞 BLE/MQTT 循环） |

幂等与补传：本地待发箱 `(dev_id, time)` 去重 + 服务端 `UNIQUE(dev_id, time)` + `INSERT IGNORE`；因此“请求超时但服务端已入库”的重试也不会产生重复数据。断网期间读数先入库入队，网络恢复后由后续触发点补传（触发点接线见 ITEM-010，TD-SW-002 §8-G3）。

## 2. 预期行为（本轮交付）

1. 门控关闭（守护关闭或无手机号）时 `enqueueAndUpload` 返回 false 且不入队；`uploadPendingOnce` 返回 `skipped="gate_closed"` 且不产生请求。
2. 门控开启时读数幂等入队并异步触发上传；上传在独立作用域，不阻塞调用方。
3. 成功即删除；失败保留并推后退避；到 `maxAttempts` 后不再自动重试但数据不丢。
4. 同一时刻只有一个上传在飞（串行）；批量受 `batchLimit` 限制。
5. 待发箱表缺失/异常时降级为空操作，App 不崩溃（与 ITEM-006 的迁移回退衔接）。

## 3. 本轮验证（实现侧自检）

| 检查 | 命令 | 结果 |
|---|---|---|
| 宿主机单元测试 | `./gradlew :app:testDebugUnitTest --offline` | `BUILD SUCCESSFUL`；14 套件共 **133 项、0 失败 0 跳过**（新增 `CloudUploadRepositoryTest` 14 项） |
| 构建 | `./gradlew :app:assembleDebug --offline` | `BUILD SUCCESSFUL`；`app-debug.apk` 10,359,866 B |
| 全量重编译告警 | `./gradlew :app:compileDebugKotlin --offline --rerun` | 11 条（与 ITEM-008 同一集合），无新增 |
| 打包核查 | 直接读 APK 多 dex | `UploadOutbox`/`CloudUploadRepository`/`SqliteUploadOutboxStore`/`PrefsUploadGate`/`RetrofitUploadApi` 均已打包 |
| 差异范围 | `git status --short` | 新增 2 个模块 + 1 个测试文件；`app/build.gradle.kts` +5 行（testOptions） |

新增 14 项用例：`success_deletesRows_andReportsOutcome`（①）、`failure_keepsRows_andSchedulesBackoff`（②）、`retryBecomesDue_afterBackoffElapses_andSucceeds`（②）、`maxAttempts_stopsAutoRetry_butKeepsRow`（③）、`batchLimit_splitsIntoMultipleBatches`（④）、`guardClosed_noEnqueue_andNoRequest`（⑤）、`noRegisteredPhone_noEnqueue_andNoRequest`（⑥）、`duplicateEnqueue_keepsSingleRow_andSecondReturnsFalse`（⑦）、`concurrentUploads_areSerialized_secondIsBusy`（⑧）、`backoff_growsExponentially_andCapsAtMaximum`、`dueBatch_ordersByNextAttemptAt_thenTime`、`emptyOutbox_producesNoRequest`、`payloadFromOutbox_carriesDevIdPerRowTimeAndUnits`、`enqueueAndUpload_enqueuesAndTriggersAsyncUpload` —— **14/14 PASS**。

原始输出见 `evidence/android_test.md`（AT-009）。

## 4. 观察与交接

1. **触发点接线属 ITEM-010**：本项提供 `enqueueAndUpload`（入队后触发）与 `triggerUpload()`（供“每轮 BLE 开始/网络恢复/冷启动”调用），四个触发点的实际接入、以及把 `processTemperatureHumidityData` 的落库结果接到上传，由 ITEM-010 完成（TD-SW-002 §8-G3）。
2. **`measure_state` 语义**：as-built 约定为 `"stopping"` = 守护运行中、`"starting"` = 已停止（`HomeFragment.restoreGuardState`）；`PrefsUploadGate` 按此实现。ITEM-010 若改用内存中的 `homeViewModel.startData.value`，语义等价（`startData == 1`）。
3. **失败整批记账**：一次 HTTP 请求对应一批，失败时整批 `attempts+1`；因服务端幂等，重试不会重复入库；若后续需要“部分成功”粒度，属新需求。
4. **达到 `maxAttempts` 的数据保留但不自动重试**（设计 §5.5），目前无手动清理入口；若需要清理策略（如保留 N 天）属新需求。
5. **`testOptions.isReturnDefaultValues` 为构建脚本改动**：仅为让宿主机单测能加载含 `android.util.Log` 的类；生产行为不变（仅影响 unit test 的 android.jar 桩）。
6. **本项不涉及**：Service 接线（ITEM-010）、UI（ITEM-011/012）、版本/文档（ITEM-014）。

---

# 任务项 ITEM-010（本轮完成）

**ITEM-010（队列第 10 项）**：在 `MqtttService` 中完成编排接入：① 设备读取周期常量与传感器 3 分钟采样节拍对齐（保留手动立即读取）；② `processTemperatureHumidityData` 改用 `AlarmEvaluator` 的判定结果；③ 使用真实采样时间戳入库；④ 入库成功后按待发箱与上传编排触发云上传；同时保留 900 秒报警去抖、>65℃ 每次通知、报警事件落库与既有报警文案字面值。
期望行为：一次 BLE 或 MQTT 读数依次完成归因、告警判定、去重落库与上传触发；任何一步失败都不影响其他步骤与后台循环；上报失败不产生用户可见的阻塞或崩溃。

设计映射：AA-002 §4.1（数据流与状态归属）、§5.1–§5.5、§6（四个上传触发点）、§7（告警展示）、D-04/D-05（时间窗 + 库基准）、D-07/D-09（待发箱/门控）、D-11（读取周期对齐）、§4.2（Service 编排）；TD-SW-002 §8-G3/G4。

## 1. 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/telemetry/SensorCadence.kt` | 新增 | 采样节拍常量（180 s / 180000 ms），供 BLE 读取周期与后续界面文案共用，避免多处魔数 |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/MqtttService.kt` | 修改 | ① `READ_INTERVAL_MS = SensorCadence.SAMPLE_PERIOD_MS`（原 5 分钟；手动 `wakeReadNow` 通道保留）；② `processTemperatureHumidityData` 的整个 if/else 下降/超限判定与紧急块改为调用 `AlarmEvaluator.evaluate` + 枚举映射；③ 下降基准改为 `loadBaselineSamples`（本地库 `[now-15min-回溯容差, now]`，D-05）；④ `DeviceHistory` 仅保留 `lastAlarmTime`（删除第 1/2/3 个样本窗口字段）；⑤ 入库后 `uploadRepository.enqueueAndUpload`；⑥ 四个上传触发点：入队后、每轮 BLE 开始、broker 连接成功（网络恢复）、`onCreate`（冷启动）；⑦ 新增 `uploadRepository`（`RetrofitUploadApi` + `SqliteUploadOutboxStore` + `PrefsUploadGate`） |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/TemperatureDatabaseHelper.kt` | 修改 | 新增 `getSamplesInRange(devId, fromTime, toTime)`（时间升序，供告警时间窗基准，关闭 G4 的查询 API 缺口） |
| `dengbei_care/app/src/test/java/com/jinyuni/dengbei_care/telemetry/SensorCadenceTest.kt` | 新增 | 宿主机 JUnit4 自检 2 项（节拍常量） |

编排后的单次读数流程（顺序与任务描述一致）：

```text
读入(BLE 解密→GatewayFrameCodec / MQTT→ReadingAttribution 严格归因)
  → loadDeviceParams(devId)                        每设备报警参数
  → loadBaselineSamples(devId, now)                本地库基准样本（D-05；失败→空列表）
  → AlarmEvaluator.evaluate(...)                    时间窗下降 / 阈值超限 / 紧急 >65.0℃
  → updateDeviceAlarm + 报警事件落库（三个字面值映射）
  → 紧急？直接震动/铃声/通知（每次，不受去抖）
  → storeTemperatureReading(devId, now, t, h, guard)   真实采样时间、幂等
  → 守护中？uploadRepository.enqueueAndUpload(...)     幂等入队 + 异步上传
  → updateDeviceReading（卡片）
  → 900 s 去抖后震动/铃声/通知（仅下降/超限，且守护中）
```

保留的兼容面：三个报警文案字面值、`ALARM_TYPE_*` 常量、`>65℃` 阈值、`900 * 1000` 去抖、`homeViewModel.startData` 守护门控、`updateDeviceAlarm`/`updateDeviceReading` 调用、`AlarmEmergent` 字段、`_bkp_all_time`。

行为差异（均为有意且有依据）：

| 项 | 改动前 | 改动后 | 依据 |
|---|---|---|---|
| 下降基准 | 内存“最近第 1/2/3 个样本” + 30 min 过期 | 本地库 `[now-W-slack, now-W]` 最新样本 | D-04/D-05（样本间隔不再固定；重启后可判定） |
| “无参照点则清屏” | 内存 5/10/15 均为空时清 sparkline | 库中窗口内无任何样本时清 sparkline | 语义平移（原判定依据已被替换） |
| 紧急报警与下降同时成立 | 可能“直接触发一次 + 去抖再触发一次” | 紧急走直接通道，不再进入去抖通道 | 与 T-SW-L2-14“紧急不受去抖限制”一致，避免双次动作 |
| 触发上报的前提 | — | 仅守护中（`startData == 1`）才入队；无手机号由门控拦住 | D-09 |

## 2. 预期行为（本轮交付）

1. BLE 轮询周期为 3 分钟（与传感器采样节拍一致），手动“立即读取”仍可用。
2. 告警判定结果（文案 + 级别）与 `AlarmEvaluator` 一致；阈值/紧急语义、优先级与字面值不变；无基准不产生下降报警。
3. 入库行的 `time` 为真实采样时间；守护关闭不写入、不入队、不上传。
4. 入库成功后读数幂等入队并异步上传；四个触发点覆盖冷启动/每轮 BLE/网络恢复/入队后；上传失败不抛异常、不阻塞读数处理与后台循环。
5. 900 s 去抖、`>65℃` 每次通知、报警事件落库均保持。

## 3. 本轮验证（实现侧自检）

| 检查 | 命令 | 结果 |
|---|---|---|
| 宿主机单元测试 | `./gradlew :app:testDebugUnitTest --offline` | `BUILD SUCCESSFUL`；全部套件共 **146 项、0 失败 0 跳过**（新增 `SensorCadenceTest` 2 项） |
| 构建 | `./gradlew :app:assembleDebug --offline` | `BUILD SUCCESSFUL`；`app-debug.apk` 10,360,647 B |
| 全量重编译告警 | `./gradlew :app:compileDebugKotlin --offline --rerun` | 11 条（与 ITEM-009 同一集合），无新增 |
| 接线静态核查 | `grep -n`（见下表） | 全部命中 |
| 旧窗口残留 | `grep -n "data5MinAgo\|data10MinAgo\|data15MinAgo\|isSignificantChange"` | 0 命中 |
| 兼容面字面值 | `grep -c "温度湿度下降报警\|温湿度超限报警\|紧急报警，温度超过65度，谨防火灾"` | 3（`when` 映射处，逐字保留） |

| 核查点 | 命中 |
|---|---|
| 读取周期对齐 | `MqtttService.kt:253 READ_INTERVAL_MS = SensorCadence.SAMPLE_PERIOD_MS` |
| 判定接入 | `840 val verdict = AlarmEvaluator.evaluate(` |
| 基准来自库 | `950 loadBaselineSamples` / `955 getSamplesInRange` |
| 真实时间戳入库 | `901 storeTemperatureReading(... currentTime ...)` |
| 上传触发（入队后） | `911 uploadRepository.enqueueAndUpload(...)` |
| 上传触发（每轮 BLE） | `388 uploadRepository.triggerUpload()` |
| 上传触发（网络恢复） | `1244 uploadRepository.triggerUpload()`（`connectToBroker.onSuccess`） |
| 上传触发（冷启动） | `369 uploadRepository.triggerUpload()`（`onCreate`） |
| 900 s 去抖 | `925 if (now - history.lastAlarmTime > (900 * 1000))` |

原始输出见 `evidence/android_test.md`（AT-010）。

## 4. 观察与交接

1. **真机行为未在本轮验证**：BLE/MQTT 链路、卡片/通知/报告展示、待发箱补传与 HTTP 观测需设备与网络，属测试能力（TD-SW-002 T-SW-L2-13/14/15、L3-01..03、E2E J-1）。本项只提供接线静态证据 + 构建/回归。
2. **基准查询在回调线程执行**：`loadBaselineSamples` 是单条带索引查询（与既有 `MacIdBook.all()`/入库写在同一线程），未引入新的阻塞类别；若后续需要把整段处理移入 IO 调度器，属性能优化项。
3. **G4 已关闭**：设计 D-05 要求的“按 devId + 时间范围查样本”查询 API 已由 `TemperatureDatabaseHelper.getSamplesInRange` 提供并被判定链路使用（重启后可判定）。
4. **G3 已确认**：四个上传触发点均已在代码中就位（入队后/每轮 BLE/网络恢复/冷启动），可供测试能力逐点验证。
5. **sparkline 清屏语义平移**：由“内存窗口为空”改为“库中窗口内无样本”；若产品要求保留原频率的清屏行为，属新需求。
6. **本项不涉及**：UI 卡片/详情/报告文案（ITEM-011/012/013）、版本与文档（ITEM-014）。

---

# 任务项 ITEM-010 修复（缺陷 D-010-1，本轮完成）

**来源**：软件测试能力独立验证（`evidence/software_test.md` ST-010，结论 **TEST_FAIL**）——缺陷 **D-010-1**：`processTemperatureHumidityData` 全函数只有一层 try/catch，紧急通知块（震动/铃声/通知）与报警事件落库位于单条入库/上传触发**之前**且无独立保护，因此这些步骤抛异常时会跳过**入库、入队上传、卡片更新**（涉及 >65℃ 紧急路径的数据保留，安全相关）。

**修复要求（测试能力给出，属实现范围）**：① 各步各自保护（推荐顺序：先入库→入队上传→再告警副作用；**或**保持顺序但逐步 try/catch）；② `RingtonePlayer.startAlarm` 与 `VibrationPlayer.vibratePhone` 内部降级，不得向外抛；③ 不得改变：三条文案字面值、`ALARM_TYPE_*`、`>65℃` 语义、900 s 去抖、守护门控、真实采样时间戳与上传触发语义、周期 180 s。

## 1. 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/MqtttService.kt` | 修改 | 保留已验证的 12 步编排顺序（测试门禁 B 逐项断言），为**每一步副作用加独立 `try/catch`**：清 sparkline、紧急通知块、卡片告警、报警事件落库、单条入库、入队+上传触发、卡片读数、900 s 去抖；函数内 `try` 块由 1 → 9 |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/RingtonePlayer.kt` | 修改 | `startAlarm`：URI 为空 / `MediaPlayer.create` 返回 null / `start()` 抛错 → 记日志并降级（失败时释放并置空）；`stopAlarm`：异常不外抛，始终释放并置空 |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/VibrationPlayer.kt` | 修改 | `vibratePhone`：系统服务用 `as?` 判空、`vibrate()` 异常记日志降级；`stopVibration`：异常不外抛并置空 |

选择“保持顺序 + 逐步保护”而非重排顺序：测试门禁 B 对 12 步顺序逐项断言（当前 12/12 PASS），而验收缺陷的本体是**步骤隔离**（任一步失败不影响其它步骤）——逐步 try/catch 已完整满足，且不引入新的顺序变更风险。

## 2. 预期行为（修复后）

1. 铃声/震动/通知（紧急或去抖通道）任何失败都**不再跳过**入库、入队上传与卡片更新；反之，入库/上传/落库失败也不跳过告警呈现。
2. `RingtonePlayer`/`VibrationPlayer` 自身不向外抛异常（null 媒体播放器、缺失系统服务、`start`/`vibrate` 异常均降级）。
3. 兼容面不变：三条文案字面值、`ALARM_TYPE_*`、`>65℃` 仅由 `AlarmEvaluator` 比较、900 s 去抖、守护门控、真实采样时间戳、四个上传触发点、周期 180 s。

## 3. 本轮验证

| 检查 | 命令 | 结果 |
|---|---|---|
| 测试能力门禁（缺陷项） | `python evidence/software_verify_orchestration_item010.py` | **EXIT=0**：A1–A5、B 顺序 12/12、C1–C11、D1–D5、E（含宿主 SQLite 执行）、F（函数内 try=9；通知块↔入库独立保护=True；落库↔入库独立保护=True）全 PASS |
| 其余 6 个独立门禁 | `python evidence/software_verify_{timestamp_write_item007,cloud_upload_item009,upload_payload_item008,db_migration_item006,static_check_no_little_endian_parser,static_check_server_migration}.py` | 全部 **EXIT=0**（无回归） |
| 宿主机单元测试 | `./gradlew :app:testDebugUnitTest --offline` | `BUILD SUCCESSFUL`；**146 项、0 失败 0 跳过** |
| 构建 | `./gradlew :app:assembleDebug --offline` | `BUILD SUCCESSFUL`；`app-debug.apk` 10,361,755 B |
| 全量重编译告警 | `./gradlew :app:compileDebugKotlin --offline --rerun` | 11 条（与修复前同一集合），无新增 |
| 差异范围 | `git diff --stat` | `MqtttService.kt` / `RingtonePlayer.kt` / `VibrationPlayer.kt` 三个文件，未触碰其它产品文件 |

## 4. 观察与交接

1. **真机层仍未执行**（测试能力已登记）：BLE/MQTT 链路、通知/卡片、待发箱补传、`/upload_data` 需设备与网络；且启动 debug 包会连生产 broker/服务器（无隔离测试环境），属 E2E-SW-002。
2. **修复未改变**任何告警文案/阈值/去抖/门控/上传语义（测试门禁 C/D/E 全 PASS 可证）。
3. `stopAlarm`/`stopVibration` 现总释放资源并置空引用（原仅在播放中释放）：属健壮性改进，行为对用户可见面不变。

---

# 任务项 ITEM-011（本轮完成）

**ITEM-011（队列第 11 项）**：调整卡片状态展示：`DeviceState` 与 `ui/home/DeviceCardAdapter.kt` 不再以「30 分钟无数据」判定并显示离线，改为展示最近上报相对时间与按条件上报（可能长时间无上报）的提示；状态 chip 仍区分未收到/正常/警告/紧急。
期望行为：长时间无新数据的设备不再显示为离线；未收到任何数据的设备仍显示为未收到且温度湿度为 `--`；报警级别配色语义不变。

设计映射：AA-002 §7（卡片展示）、D-10（不再用 30 min 数据龄表示离线，连通性由链路状态表达）、§4.2（`DeviceState.kt`/`DeviceCardAdapter.kt`/`item_device_card.xml`）；IC-002 §5；TD-SW-002 §4.1 T-SW-L0-14、§4.5 T-SW-L3-01/02/03。

## 1. 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/DeviceState.kt` | 修改 | 删除 `OFFLINE_THRESHOLD_MS`（30 min）与 `isOnline()`；新增 `isNeverReported()` 与 `lastReportAgeLabel(nowMillis)`（刚刚 / N 分钟前 / N 小时前 / N 天前；从未上报返回空串）；保留 `displayName()`、`empty()` 与字段语义 |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/ui/home/DeviceCardAdapter.kt` | 修改 | 状态 chip 去掉 `!isOnline()` → “离线”分支（现仅四种：未收到/紧急/警告/正常，配色与文案不变）；新增 `reportHint` 文本：“尚未收到上报 · 按条件上报，可能长时间无上报”或“最近上报 X · 按条件上报，可能长时间无上报” |
| `dengbei_care/app/src/main/res/layout/item_device_card.xml` | 修改 | 新增 `reportHint` TextView（全宽、LabelSmall、次要色、最多 2 行）置于数值下方；sparkline 的顶部约束改挂到 `reportHint` 下 |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/ui/home/HomeViewModel.kt` | 修改（仅注释） | sparkline 容量注释由“约 2.5 小时 @ 5 分钟间隔”改为“约 1.5 小时 @ 3 分钟采样节拍；实际由条件上报决定” |
| `dengbei_care/app/src/test/java/com/jinyuni/dengbei_care/ui/home/DeviceStateTest.kt` | 新增 | 宿主机 JUnit4 自检 7 项（见 §3） |

状态 chip 的四种情形（与改动前一致，仅删去“离线”）：

| 条件 | 文案 | 配色 |
|---|---|---|
| `isNeverReported()` | 未收到 | `brand_secondary` + 黑字 |
| `alarmSeverity == ALARM` | 紧急 | `brand_error` + 白字 |
| `alarmSeverity == WARNING` | 警告 | `brand_warning` + 黑字 |
| 其它 | 正常 | `brand_ok` + 白字 |

温度/湿度仍为：`isNeverReported()` → `--`，否则 `%.1f°C` / `%.0f%%`（未改）。

## 2. 预期行为（本轮交付）

1. 长时间无新数据的设备**不再出现“离线”**；卡片只显示“最近上报 N 分钟/小时/天前”与“按条件上报，可能长时间无上报”提示。
2. 从未收到数据的设备仍显示“未收到”且温度/湿度为 `--`。
3. 报警级别（紧急/警告/正常）文案与配色不变；报警文案显示逻辑不变。
4. 连通性不由数据龄推断（链路状态仍由 `NotificationsViewModel._mqtt_broker_state` 表达，本项未改）。
5. 相对时间标签分桶：<60 s “刚刚”；<60 min “N 分钟前”；<24 h “N 小时前”；否则“N 天前”。

## 3. 本轮验证（实现侧自检）

| 检查 | 命令 | 结果 |
|---|---|---|
| 宿主机单元测试 | `./gradlew :app:testDebugUnitTest --offline` | `BUILD SUCCESSFUL`；全部套件共 **155 项、0 失败 0 跳过**（新增 `DeviceStateTest` 7 项） |
| 构建 | `./gradlew :app:assembleDebug --offline` | `BUILD SUCCESSFUL`；`app-debug.apk` 10,361,689 B（含新布局） |
| 编译告警 | `./gradlew :app:compileDebugKotlin --offline` | 无新增（既有 11 条不变） |
| 旧判定入口清零 | `grep -rn "isOnline\|OFFLINE_THRESHOLD" DeviceState.kt ui/home/` | 0 命中（仅 vendor paho 自身同名方法，与本项无关） |
| 卡片四状态文案 | `grep -n "未收到\|正常\|警告\|紧急" DeviceCardAdapter.kt` | 四种齐备，无“离线”分支 |
| 布局接线 | `grep -n "reportHint\|sparkline" item_device_card.xml` | `reportHint` 存在且 sparkline 顶部约束指向它 |

新增 7 项用例：`neverReported_isNeverReported_andHasNoAgeLabel`、`ageLabel_buckets`（含 59s/60s/59min/60min/23h/24h/3d 边界）、`staleDevice_getsRelativeLabel_notOffline`（40 分钟无数据 → “40 分钟前”且不含“离线”）、`noOnlineOrOfflineThresholdEntryPoints`（反射断言 `isOnline` 与 OFFLINE 常量已不存在）、`displayName_prefersAlias_thenDevId`、`severity_isCarriedThroughUnchanged`、`emptyPlaceholder_matchesNeverReportedSemantics` —— **7/7 PASS**。

原始输出见 `evidence/android_test.md`（AT-011）。

## 4. 观察与交接

1. **真机界面未在本轮验证**：卡片截图/配色属测试能力 T-SW-L3-01/02/03（需设备）；本项提供逻辑层断言 + 布局静态核查。
2. **chip 映射的宿主机可测性**：`bindStatusChip` 位于 Adapter（需 Android 视图），未纳入宿主机测试；其四种分支由上述静态核查 + 设备侧 L3 用例覆盖。
3. **提示文案为硬编码中文**：与既有卡片文案（“未收到”“正常”等）风格一致；若后续需要本地化，属资源化重构（需同步 `strings.xml`）。
4. **`HomeViewModel` 仅改注释**：sparkline 容量数值上限（`MAX_SPARKLINE_SIZE = 30`）未改，仅修正节拍说明。
5. **本项不涉及**：详情页规则说明/图表范围/高温强调（ITEM-012）、早报文案（ITEM-013）、版本与文档（ITEM-014）。
