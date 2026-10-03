# 软件验证证据（ST-002）

状态：软件测试执行证据（`software_tester.software_verification`），按 Runtime 逐个指派的队列项追加
对象：Android App `dengbei_care`（Kotlin / AGP 8.3.2 / Gradle 8.4 / JDK 21）
本轮项：队列 **ITEM-001**、**ITEM-002**、**ITEM-003**、**ITEM-004**（逐项分节记录）
依据：`artifacts/software_test_design.md`（TD-SW-002）、`artifacts/software_e2e_plan.md`（E2E-SW-002）、`artifacts/interface_contract.md`（IC-002 §4）、`artifacts/android_architecture.md`（AA-002 §5.1/§10）
被测代码版本：`3e0ed11`（android_engineer.android_implementation 提交）；改动前基线：`fe9a81c`
说明：本文件为本能力**独立执行**的记录；实现侧自检（`evidence/android_test.md` AT-001）仅作为被核对对象，不重复引用为通过依据。

---

## ST-001 · ITEM-001（GatewayFrameCodec 与 MqtttService 接入）

### 1. 环境

| 项 | 值 |
|---|---|
| 工作目录 | `D:\mypro\beiwo2\AIP\dengbei_care` |
| 平台 / JDK | Windows（Git Bash）/ openjdk 21.0.2 |
| 命令 | `./gradlew :app:testDebugUnitTest --offline`、`./gradlew :app:assembleDebug --offline` |
| 被测文件摘要 | `GatewayFrameCodec.kt` sha256[0:16]=`76f669ad4ad88a78`；`MqtttService.kt` sha256[0:16]=`5b17b99829b8f309` |
| 设备需求 | 本项为纯逻辑 + 服务内接线，**不需要真机/BLE/网络**；真机 GATT 路径的端到端确认属后续 REG-02/E2E（见 §7） |

### 2. 适用测试的选择与理由

| TD-SW-002 用例 | 是否适用 | 理由 |
|---|---|---|
| T-SW-L0-01（帧长规则、devCount=0） | 适用 | 本项显式要求 `devCount==0` 与长度不足必须拒绝且不产生读数 |
| T-SW-L0-02（8 B 字段解码/符号/单位/不互换） | 适用 | 本项要求字节偏移/字节序/符号与改动前逐位一致 |
| T-SW-L0-03（多设备归属） | 适用 | 本项要求读数携带自身 ID，不依赖下标 |
| T-SW-L1-01/L1-02（构建、宿主机单测） | 适用 | 改动编译产物与测试基础设施 |
| T-SW-L1-03（单一解析入口） | 适用 | 本项要求"BLE 解析只有该模块一处实现" |
| T-SW-L2-*（MQTT/上传/告警/存储） | 不适用（未实现） | 队列后续项范围，本项未改动；按能力要求不得据此判本项 |
| T-SW-L3-*（UI/文案） | 不适用（未实现） | 同上 |
| T-SW-REG-02（BLE 真机联调） | 本项不适用 | 需真实网关；本项未改 GATT 传输层，只改解密后解码与拒绝分支 |

### 3. 独立验证方法（不复用实现侧自检）

新增两个独立验证测试（`app/src/test/java/com/jinyuni/dengbei_care/verification/`），期望值不引用被测实现：

1. `GatewayFrameCodecItem001LegacyParityTest.kt` —— 内嵌**从 Git 历史逐字提取**的改动前 `parsePlainFrame`/`u16be`/`s16be`/`toHexSep`（`fe9a81c`）作为基准，对每个被接受的帧比较网关 ID、devCount、devIdHex、devIdBytes、湿度、温度，**Double 用 `toRawBits()` 逐位相等**：
   - 边界矩阵：devCount 1..8 × 尾部填充 0..32 × 7 组极值（`0x0000/0xFFFF/0x8000/0x7FFF/0x03E8/0xFF9C/0x00FA`）= **1848 例**；
   - 随机矩阵：600 组（devCount 1..5，湿度/温度取全 16 bit 范围，随机 ID 与尾部填充）；
   - 拒绝边界：长度不足（改动前抛异常）与 `devCount==0`（改动前"空帧成功"）的差异**仅应为显式拒绝**。
2. `GatewayFrameCodecItem001VerificationTest.kt` —— 按 IC-002 §4 自写独立参考：
   - **穷举温度表**：全部 65536 个原始字，期望整数由独立有符号公式给出，并校验整表 FNV-1a 哈希；
   - **穷举湿度表**：全部 65536 个原始字（无符号） + 整表哈希；
   - **穷举设备 ID 字节序**：首字节 × 末字节 = 65536 组；
   - **帧长规则矩阵**：devCount 0..8 × 填充 0..32、长度 −1、明文 0..15 B；
   - **拒绝结果结构**：反射确认 `Result.Rejected` 不含任何 sample/reading 字段或返回 `List` 的方法；
   - **灵敏度负对照**：无符号解释（6543.6）必须与实现（−10.0）不同；截断帧与 `devCount=0` 必须不返回 `Success`。
3. 独立参考哈希脚本：`evidence/software_reference_frame_codec.py`（不依赖被测代码，可复跑）。

### 4. 原始结果

```
./gradlew :app:testDebugUnitTest --offline
=> BUILD SUCCESSFUL in 3s
```

JUnit XML（`dengbei_care/app/build/test-results/testDebugUnitTest/`）：

| 套件 | tests | failures | errors | skipped |
|---|---|---|---|---|
| `com.jinyuni.dengbei_care.ExampleUnitTest` | 1 | 0 | 0 | 0 |
| `com.jinyuni.dengbei_care.protocol.GatewayFrameCodecTest`（实现侧自检） | 11 | 0 | 0 | 0 |
| `com.jinyuni.dengbei_care.verification.GatewayFrameCodecItem001LegacyParityTest`（独立） | 3 | 0 | 0 | 0 |
| `com.jinyuni.dengbei_care.verification.GatewayFrameCodecItem001VerificationTest`（独立） | 7 | 0 | 0 | 0 |
| **合计** | **22** | **0** | **0** | **0** |

独立用例结果：

| 独立用例 | 覆盖 | 结果 |
|---|---|---|
| `acceptedFrames_areBitIdenticalToPreChangeImplementation` | 与改动前逐位一致（1848 边界例） | PASS |
| `randomFrames_areBitIdenticalToPreChangeImplementation` | 与改动前逐位一致（600 随机例） | PASS |
| `rejectionBoundaries_differFromLegacyOnlyByExplicitRejection` | 拒绝边界差异仅为显式拒绝 | PASS |
| `temperature_allRawWords_matchIndependentSpecAndHash` | 65536 温度字 + 表哈希 | PASS |
| `humidity_allRawWords_matchIndependentSpecAndHash` | 65536 湿度字 + 表哈希 | PASS |
| `deviceId_byteOrderAndFormat_exhaustiveFirstAndLastByte` | 65536 组 ID 字节序/格式 | PASS |
| `frameLengthRule_matrix_rejectsExactlyOutsideSpec` | 帧长矩阵 + devCount=0 + <16 B | PASS |
| `rejectedResult_carriesNoReadings` | 拒绝结果不含读数（结构） | PASS |
| `randomFrames_matchIndependentReference_bitForBit` | 400 随机帧与独立参考 | PASS |
| `harnessSensitivity_detectsWrongSignAndWrongLengthRules` | 负对照（灵敏度） | PASS |

独立 Python 参考输出（`python evidence/software_reference_frame_codec.py`）：

```
TEMPERATURE_TABLE_FNV1A = 0x2852B22D36982325     # 与 Kotlin 测试断言常量一致
HUMIDITY_TABLE_FNV1A    = 0x7E93E747630A2325     # 与 Kotlin 测试断言常量一致
temp raw=0x00FA -> 250 (25.0); 0xFF9C -> -100 (-10.0); 0xFE70 -> -400 (-40.0); 0x04E2 -> 1250 (125.0)
```

构建（编译产物由当前源码产生）：

```
./gradlew :app:assembleDebug --offline
=> BUILD SUCCESSFUL （:app:assembleDebug UP-TO-DATE：主源码自 3e0ed11 未变；本能力新增的仅测试源码，不影响 APK）
app/build/outputs/apk/debug/app-debug.apk  10,285,626 B
```

主源码编译由 `:app:testDebugUnitTest` 链路的 `:app:compileDebugKotlin` 承担（输入未变即 UP-TO-DATE，表明当前源码已成功编译）。

### 5. 接线与单一实现的静态核查（独立读取被测源码）

| 检查 | 命令 | 结果 | 判定 |
|---|---|---|---|
| 不再有本地 `parsePlainFrame` 功能实现 | `grep -rn "parsePlainFrame" app/src/main` | 仅 1 处命中，位于 `GatewayFrameCodec.kt` 的 KDoc 注释（说明历史来源），无代码实现 | 通过 |
| `decryptAndParseEcbFrame` 只经 codec | `grep -rn "decryptAndParseEcbFrame" app/src/main` | 定义 1 处（`MqtttService.kt:459`，返回 `GatewayFrameCodec.Result`）+ 唯一调用 1 处（`MqtttService.kt:722`） | 通过 |
| 拒绝路径不产生读数/不落库/不上传 | 读取 `MqtttService.kt:722-733` | `Result.Rejected` 分支仅 `Log.w(...)`，不调用 `processTemperatureHumidityData`（亦无 `storeTemperatureData`/上传调用） | 通过 |
| 成功路径行为未变 | 读取 `MqtttService.kt:734-765` 与 `git diff fe9a81c 3e0ed11` | 逐样本日志、`isZeroValue` 连续 3 次门控、`bleTempData/bleHumiData/bletime`、`processTemperatureHumidityData(devId, ...)` 调用与改动前一致（devId 取值由 `devIdHex.replace(":","")` 改为 `devIdCompact`，等价值） | 通过 |
| 字节布局/符号/单位与改动前一致 | `git show fe9a81c:...MqtttService.kt` 对比 | 头块 `plain[0..5]` + `devCount=plain[6]&0xFF`；`expected=16*(1+devCount)`；块内 `p8=block[0..7]`、`id=p8[0..3]`、`hum=u16be(p8,4)/10.0`、`temp=s16be(p8,6)/10.0`；旧 `u16be/s16be/toHexSep` 源文本与 codec 新实现等价 | 通过 |
| 本项范围外的旧解析族未被动用 | `grep -rn "ParsedFrame" app/src/main`、`grep -rn "parseHexData" app/src/main` | 仅 `ParsedFrameRaw`/`parseUnencryptedFrame`（无有效调用点）与 `parseHexData`（+1 条注释）仍存在，与改动前一致，属后续队列项（ITEM-002 等） | 通过（未越权改动） |

### 6. 回归覆盖（变更影响面）

| 回归项 | 方法 | 结果 |
|---|---|---|
| 实现侧原自检未被破坏 | `GatewayFrameCodecTest` 11/11 PASS | 通过 |
| 既有最小单测仍可运行 | `ExampleUnitTest` 1/1 PASS | 通过 |
| 主源码编译未破坏 | `:app:compileDebugKotlin` 成功、`assembleDebug` BUILD SUCCESSFUL | 通过 |
| MQTT 路径未被改动 | diff 仅涉及 `MqtttService.kt` 的解密/解析调用段，MQTT `messageArrived` 归因段未出现在本提交 diff 中 | 通过 |
| 兼容面未被改动 | diff 未触及 AES 密钥/模式、TLS/证书、topic、MTU、UUID、`0xA1`、时间校准 payload、阈值/去抖/文案、`MyPrefs` key | 通过 |

### 7. 判定

ITEM-001 的期望行为全部成立：

1. **逐位一致**：对全部被接受的帧，网关 ID、devCount、每设备 devId、湿度、温度与改动前实现**逐位相等**（含 Double 位级比较），覆盖 1848 组边界 + 600 组随机 + 400 组独立参考 + 2×65536 穷举表（表哈希与独立 Python 参考一致）。**通过**
2. **拒绝语义**：`devCount == 0`、明文 `< 16 B`、明文 `< 16*(1+devCount)` 均返回显式 `Rejected` 且**不产生任何读数**（类型上不含读数，调用点不进入处理/落库/上传）。**通过**（其中 `devCount==0` 由"空帧成功"变为"显式拒绝"，是本项要求的唯一有意语义变化）
3. **单一实现**：`MqtttService` 不再保留本地帧解析实现，BLE 聚合帧解析只有 `protocol/GatewayFrameCodec` 一处。**通过**

结论：**TEST_PASS**。

未覆盖（不构成本项失败，已登记）：
- 真机 BLE GATT 读取与真实网关帧（需真实网关，属 TD-SW-002 §4.6 REG-02 / E2E-SW-002 J-2）；
- MQTT 归因严格化、`parseHexData` 移除、服务器常量、SQLite v3、上传链路、UI/文案 —— 均为队列后续项，本项未实现（`devCount==0` 之外的行为变化未引入）。

### 8. 交接与观察

1. 本项在 `onCharacteristicRead` 中保留的 `isZeroValue` 连续 3 次门控**未改动**（与改动前相同），其在条件上报下的取舍见 TD-SW-002 §8-G1，仍待实现/需求方确认。
2. `ParsedFrameRaw`/`parseUnencryptedFrame`/`interpretPayloadV3`/`DeviceReadingV3`/`parseHexData` 仍为无调用点遗留；清理属后续队列项（`parseHexData` 为 ITEM-002 明确范围），本能力不越权改动。
3. 新增的独立验证测试与参考脚本保留在仓库中，供后续项与本项回归复用；`file_manifest.txt` 已同步登记。

---

## ST-002 · ITEM-002（移除小端解析入口 `parseHexData`）

**被测代码版本**：`d1758ec`（android_engineer.android_implementation 提交，相对上一提交 `0075b09`）；对象文件 `MqtttService.kt`（**5 insertions / 36 deletions**，全部为死代码与注释）
**任务期望**：工程内不再存在小端 `humidity|temperature` 解释入口 `parseHexData` 及相关注释残留；BLE/MQTT 两条入口的解析均只经 `GatewayFrameCodec` 的唯一实现。

### 1. 适用测试的选择与理由

| TD-SW-002 用例 | 是否适用 | 理由 |
|---|---|---|
| T-SW-L1-03（单一解析入口） | 适用 | 本项核心期望；判定口径为"无功能入口与注释残留" |
| T-SW-L1-01/L1-02（构建、宿主机单测） | 适用 | 删除后须可编译且既有/新增用例不回归 |
| T-SW-L2/L3（链路、UI） | 不适用（未实现） | 后续队列项范围；本项为纯死代码删除，无行为变化 |

判定范围（A 判定域）：**Android 编译源码 `dengbei_care/app/src/**`**。依据：本项对象是 Android 的 `MqtttService`；`readme.txt` 点 6 明确本轮不改 iOS，`dengbei_care/iOS开发资料/` 在 `file_manifest.txt` 中登记为"间接参考、不参与编译"。全仓其它命中按 §4 记录，不计入判定。

### 2. 独立验证方法

新增可复跑的独立静态核查脚本 `evidence/software_static_check_no_little_endian_parser.py`（不依赖实现侧声明）：
- A1：`app/src/**` 中 `parseHexData` 出现次数必须为 0（定义、调用、注释一律不允许）；
- A2：`app/src/**` 中不得存在小端 16 bit 组合写法 `X or (Y shl 8)`（允许大端 `(X shl 8) or Y`）；
- B1：BLE 在用入口经 `decryptAndParseEcbFrame` 且按 `GatewayFrameCodec.Result.Rejected/Success` 分支、无本地帧解析实现；
- B2：`decodeAggregatedFrame` 在 `app/src/main` 中只有 `MqtttService` 解密入口一处调用点；
- C：仓库其它命中仅作信息输出。

### 3. 原始结果

```
$ python evidence/software_static_check_no_little_endian_parser.py   # EXIT=0
== A1 parseHexData in app/src (expect 0) ==
  0 hits
== A2 little-endian combine `X or (Y shl 8)` in app/src (expect 0) ==
  0 hits
== B 在用入口 ==
  PASS  B1a uses decryptAndParseEcbFrame
  PASS  B1b handles Result.Rejected
  PASS  B1c handles Result.Success
  PASS  B1d no local frame parse implementation
== B2 decodeAggregatedFrame call sites in app/src/main ==
  MqtttService.kt:477: return GatewayFrameCodec.decodeAggregatedFrame(plain)
== RESULT ==
  OK: 小端解析入口已从 Android 源工程移除；在用入口只经 GatewayFrameCodec
```

回归与构建（独立重跑）：

| 检查 | 命令 | 结果 |
|---|---|---|
| 宿主机单测（含本能力 ITEM-001 的 10 项独立用例） | `./gradlew :app:testDebugUnitTest --offline` | BUILD SUCCESSFUL；4 套件 **22 tests / 0 failures / 0 errors / 0 skipped**（ExampleUnitTest 1、实现侧自检 11、独立验证 3+7） |
| 构建 | `./gradlew :app:assembleDebug --offline` | BUILD SUCCESSFUL；`app-debug.apk` 10,285,626 B（与删除前同尺寸） |
| 全量重编译告警 | `./gradlew :app:compileDebugKotlin --offline --rerun` | BUILD SUCCESSFUL；12 条 warning 全部位于与本项无关的历史位置（MacIdBox/TemperatureDatabaseHelper/VibrationPlayer/UI/opt-in 等），**无一条指向被删代码或其注释**，无新增 error |
| 差异范围 | `git diff --numstat 0075b09 d1758ec -- .../MqtttService.kt` | `5 insertions / 36 deletions`，仅删除 `parseHexData` 定义、引用它的整段注释旧路径，并留退役说明；无其它产品文件改动 |

删除内容的性质（读取 diff 确认）：被删的 `onCharacteristicRead` 段落原本**整段处于注释状态**（`//` 前缀，含 `parseHexData`、`parseUnencryptedFrame`、`payloadsRaw8` 调用），未参与任何在用路径；被删的 `parseHexData` 定义在删除前已无调用点。因此本项**无行为变化**，与 APK 同尺寸、用例全绿互相印证。

### 4. 全仓残留（不计入 A 判定，如实记录）

| 位置 | 性质 | 处置 |
|---|---|---|
| `dengbei_care/iOS开发资料/02_Tier1_硬前置/MqtttService.kt:725,817` | iOS 参考资料包中的**旧快照副本**（非 Android 编译源，不参与构建） | 未改动（readme 点 6：本轮不改 iOS；该目录为间接参考）。若项目要求全仓字面清零，属文档/iOS 资料维护项 |
| `dengbei_care/iOS开发资料/README.md:67,152` | 对旧行号的描述文字 | 同上 |
| `dengbei_care/app/build/**`（release 类文件/dex/Kotlin 缓存） | `.gitignore` 覆盖的**陈旧构建产物** | 非交付源；不修改 |
| `artifacts/`、`evidence/`、`file_manifest.txt` | 本仓库文档对**历史状态/需求/静态检查口径**的描述 | 正常，不需清理 |
| `dengbei_care/.codegraph/codegraph.db` | 被忽略的本地代码索引 | 非交付源 |

### 5. 判定

1. **小端解析入口已移除**：Android 编译源码中 `parseHexData` 定义/调用/注释均为 0；且不存在任何小端 16 bit 组合写法。**通过**
2. **BLE/MQTT 两条在用入口**：BLE 读取的唯一二进制解析路径为 `decryptAndParseEcbFrame → GatewayFrameCodec.decodeAggregatedFrame`（`app/src/main` 中唯一调用点）；MQTT 入口解析文本载荷（`time:temp1,...:humi1,...`），不解析二进制帧，不存在第二套二进制解析实现。**通过**
3. **无行为回归**：删除内容为注释旧路径与无调用点函数；22 项宿主机用例全部通过、构建成功、APK 尺寸不变。**通过**

结论：**TEST_PASS**。

### 6. 交接与观察（本项范围外，不构成本项失败）

1. **遗留无调用点的大端解析族**：`ParsedFrameRaw`、`DeviceReadingV3`、`parseUnencryptedFrame`、`interpretPayloadV3` 仍在 `MqtttService.kt` 中存在（`app/src/main` 内零外部调用点；其内部使用 `u16be/s16be/toHexSep`，均为**大端**，与 IC-002 §4 不冲突），也因此使遗留私有助手 `u16be/s16be/toHexSep` 仍被引用（无 unused 告警）。它们是"潜在重复实现"而非"在用入口"，清理属后续队列项/维护决定；本条已在 ITEM-001 证据中登记，此处延续。
2. **`ByteArray.toHex`（第 480 行）现为零引用**（其唯一引用原先只在被删注释中）；属可选清理项，不属本项描述范围。
3. 若后续能力/评审把"工程内"理解为"整个仓库字面清零"，则 §4 中的 iOS 参考快照与 README 描述需由文档维护方更新；本能力不越权改动 iOS 参考资料。

---

## ST-003 · ITEM-003（MQTT 批量载荷严格归因 `telemetry/ReadingAttribution.kt`）

**被测代码版本**：`b96df49`（android_engineer.android_implementation 提交，相对上一提交 `15af1d9`）
**变更范围**：新增 `telemetry/ReadingAttribution.kt` + `telemetry/ReadingAttributionTest.kt`；`MqtttService.kt` **21 insertions / 64 deletions**（删除 `minOf` 静默截断循环、重复载荷分段与孤立字段 `readBackTemperature`/`readBackHuminity`/`currentTime`，改为调用归因模块）
**任务期望**：设备数/温度个数/湿度个数三者不一致或任一为空/含非法数值时拒绝整批（不落库、不上传、不报警）；一致时按 `MacIdBook` 顺序得到 `(devId, temperature, humidity)` 列表，且 `devId` 随读数向下传递而不再依赖下标。

### 1. 适用测试的选择与理由

| TD-SW-002 用例 | 是否适用 | 理由 |
|---|---|---|
| T-SW-L0-04（归因：数目一致） | 适用 | 本项核心期望之一（严格映射与 devId 下沉） |
| T-SW-L0-05（归因：拒绝整批，6 类情形） | 适用 | 本项核心期望之二 |
| T-SW-L1-01/L1-02（构建、宿主机单测） | 适用 | 新增模块接入后的编译与回归 |
| T-SW-L2-03/L2-04（MQTT 数目不一致/非法数值注入） | **本轮不适用** | 需真实 broker/设备链路；其“拒绝整批→不落库/不上传/不报警”的下游后果已由调用点静态核查覆盖（§4），真机链路由后续 E2E（E2E-SW-002 J-1）承接 |
| T-SW-L2-05/L2-06（0 值/越界物理量程） | 不适用 | 量程校验明确不属本项（AA-002 §8 / TD-SW-002 §8-G2） |

### 2. 独立验证方法

新增 `app/src/test/java/com/jinyuni/dengbei_care/verification/ReadingAttributionItem003VerificationTest.kt`（8 项），期望值由本文件按 IC-002 §5 重新推导（独立解析 + 独立分类），不复用被测代码或其自检：
1. **穷举计数矩阵**：id 数 0..4 × 温度个数 0..4 × 湿度个数 0..4 = 125 组，逐组与独立参考比对“接受/拒绝”以及接受时的**逐条映射**；
2. **数值 token 分类**：接受类（`24.5`/`-3.2`/`0`/`0.0`/`125.0`/带空格/`1e2`/`.5`）与拒绝类（`abc`/空/空白/`1.5.5`/`0x10`/`NaN`/`Infinity`/`-Infinity`/`1,`/`,1`/`1,,2`/`--1`/`1e`）；
3. **时间与分段数分类**：合法/带空格时间接受；`0`/负/非数字/空/小数/分段不足拒绝；
4. **拒绝结果不携带读数**（反射：无 reading/batch 字段、无返回 List 的方法）**且绝不部分成功**；
5. **与改动前截断实现对照**：一致输入映射逐条相同（回归安全）；不一致输入旧实现“静默丢值/部分处理”，新实现必须 `COUNT_MISMATCH` 拒绝；
6. **devId 随读数下沉**：同一批值配不同绑定顺序 → 归属随之改变（证明归属来自映射表而非“下标即身份”）；
7. **无状态性**：合法→非法→合法 三次调用结果不变（不沿用上一条消息的时间/数值）；
8. **模糊测试**：3000 例（30% 纯随机字符 + 70% 结构化 `time:temps:humis`，含非法 token 与错位计数），要求**不得抛异常**且每例与独立参考一致。

### 3. 原始结果

```
$ ./gradlew :app:testDebugUnitTest --offline
BUILD SUCCESSFUL in 3s
```

| 套件 | tests | failures | errors | skipped |
|---|---|---|---|---|
| `ExampleUnitTest` | 1 | 0 | 0 | 0 |
| `protocol.GatewayFrameCodecTest`（实现侧自检） | 11 | 0 | 0 | 0 |
| `telemetry.ReadingAttributionTest`（实现侧自检） | 16 | 0 | 0 | 0 |
| `verification.GatewayFrameCodecItem001LegacyParityTest`（独立） | 3 | 0 | 0 | 0 |
| `verification.GatewayFrameCodecItem001VerificationTest`（独立） | 7 | 0 | 0 | 0 |
| `verification.ReadingAttributionItem003VerificationTest`（独立，本轮新增） | 8 | 0 | 0 | 0 |
| **合计** | **46** | **0** | **0** | **0** |

本轮新增独立用例：`countMatrix_matchesIndependentReference`、`numericTokenClasses_matchSpec`、`timeAndSegmentClasses_matchSpec`、`rejectedResult_carriesNoReadings_andNeverPartiallySucceeds`、`legacyComparison_consistentIdentical_inconsistentDiverges`、`mappingFollowsSavedIdOrder_andDevIdIsCarried`、`attribute_isStateless_acrossCalls`、`fuzz_neverThrows_andAgreesWithReference` —— **8/8 PASS**。

构建：`./gradlew :app:assembleDebug --offline` → BUILD SUCCESSFUL；`app-debug.apk` **10,359,240 B**（较上项 +73,614 B，为新增模块与接入代码；与实现侧记录一致）。

### 4. 接入点与下游后果的静态核查（独立读取被测源码）

| 检查 | 证据 | 判定 |
|---|---|---|
| 旧截断路径清零 | `grep -n "minOf(savedIds\|doubles0\|doubles1\|tempDoubles0\|readBackTemperature\|readBackHuminity\|using first" MqtttService.kt` → **0 命中** | 通过 |
| 归因入口与顺序来源 | `MqtttService.kt:1360` `val savedIds = MacIdBook.all(applicationContext).map { it.first }`；`1361` 调用 `ReadingAttribution.attribute(temppayload, savedIds)` | 通过（与网关 `0xA1` 配置包 `394: MacIdBook.all().map{...}.take(maxCount)` 同源顺序） |
| 拒绝整批 → 不落库/不上传/不报警 | `1362-1367` `Rejected` 分支仅 `Log.w(...)`；不调用 `processTemperatureHumidityData`（落库/告警/后续上传的唯一入口） | 通过 |
| 成功 → 逐条带 devId 下沉 | `1368-1377` `Success` 分支 `batch.readings.forEach { processTemperatureHumidityData(r.devId, r.temperature, r.humidity, batch.timeSeconds) }` | 通过 |
| 去重键仅在成功后推进 | `1370-1371` `_bkp_temp_time` 赋值位于 `Success` 分支内（拒绝批不会污染去重状态，后续同时间合法批仍可处理） | 通过 |
| 前项静态口径未破坏 | `python evidence/software_static_check_no_little_endian_parser.py` → `EXIT=0`（A1/A2=0，B1a–B1d、B2 全 PASS） | 通过 |

### 5. 判定

1. **严格映射**：计数一致时按 `MacIdBook.all()` 顺序得到 `(devId, temperature, humidity)`，`devId` 随读数下沉；同值集不同绑定顺序归属随之变化（穷举矩阵 125 组 + 随机/模糊比对与独立参考完全一致）。**通过**
2. **拒绝整批**：计数不一致、序列为空、含非法/非有限数值、时间非法或缺失、无绑定设备 —— 均返回 `Rejected` 且结构上不携带读数；调用点不进入落库/告警/上传。一致性输入与改动前映射逐条相同，不一致输入由“静默丢值/部分处理”变为整批拒绝。**通过**
3. **无行为回归**：46 项用例全绿、构建成功、前两项静态口径未破坏。**通过**

结论：**TEST_PASS**。

### 6. 交接与观察（本项范围外，不构成失败）

1. **绑定设备 > 50 时的拒绝风险**：网关 `0xA1` 配置包取 `MacIdBook.all()` 的**前 50 个**（`take(maxCount=50)`），而归因比较对象是**全部**已绑定 ID；若绑定数 >50，数量不等将导致**每批都被拒绝**（安全优先，符合 D-08“拒绝而非错归因”）。这是实现侧已登记的观察项；是否改为“只比对前 50 个”或限制绑定上限，属产品/协议决定，需需求方确认，不在本能力内自行改判。
2. **拒绝后不再提前 `return`**：旧代码仅在“空序列/无绑定设备”两种情形提前 return（“数目不一致”情形本就继续），新代码统一继续到后续时间校准发布块。行为差异仅出现在旧代码的两种提前返回情形；若产品要求“拒绝即不发布校准”，属新行为需另行确认。
3. **本项未涉及**：告警时间窗（ITEM-005）、入库真实时间戳（ITEM-007）、云上传与待发箱（ITEM-008/009）、UI（ITEM-011/012）；真机 MQTT 链路的“拒绝整批”端到端确认留在 E2E-SW-002 J-1。

---

## ST-004 · ITEM-004（服务器常量集中与地址迁移 `cloud/CloudConfig.kt`）

**被测代码版本**：`5090f5d`（android_engineer.android_implementation 提交，相对上一提交 `e4982e5`）
**变更范围**：新增 `cloud/CloudConfig.kt` + `cloud/CloudConfigTest.kt`；`MqtttService.kt` +4/−1（broker URI 改引常量）；`ui/zhuce/ZhuCeViewModel.kt` +3/−1（baseUrl 改引常量）；`res/xml/network_security_config.xml` +8/−1（**追加**新主机放行）；`ApiService.kt` **未改动**
**任务期望**：App 内不再出现 `117.72.84.210`；TLS 信任锚仍为 `assets/jd-ca.crt`、`TLSv1.2`、用户名=MAC、密码=MAC+`&^!A:z?` 全部保持原样；资源合并与构建通过。

### 1. 适用测试的选择与理由

| TD-SW-002 用例 | 是否适用 | 理由 |
|---|---|---|
| T-SW-L0-13（服务器常量与旧 IP 清零） | 适用 | 本项核心期望（常量值 + 旧主机不再作为连接目标） |
| T-SW-L1-05（旧 IP 清零） | 适用 | 同上 |
| T-SW-L1-06（明文放行最小化） | 适用 | 追加新主机且不得新增其它主机 |
| T-SW-L1-07（兼容面 diff） | 适用 | 证书/TLS/认证/密钥/MTU/既有端点必须不变 |
| T-SW-L1-01/L1-02（构建、宿主机单测） | 适用 | 资源合并与回归 |
| T-SW-L2-01/L2-12（新 broker 连通、`/upload_data` 契约） | **本轮不适用** | 本能力工具边界无网络探测（工具仅 `android_test`/`usb_list_devices`）；属 E2E-SW-002 J-5，且本项期望本身为配置级 |

**判定口径说明（重要）**：任务描述的“App 内不再出现 `117.72.84.210`”与已批准设计 AA-002 §5.6 / D-13 的“明文放行**追加**而非替换”存在字面差异。本能力按下列口径判定并在 §5 如实记录：
- **连接目标**：App 的 Kotlin/Java 源码中旧 IP 必须 0 命中（已成立）；
- **明文放行**：按已批准设计与 TD-SW-002 T-SW-L1-06 的口径，允许 `network_security_config.xml` 保留该历史条目（**非连接目标**），但域名集合不得扩大。

### 2. 独立验证方法

新增可复跑独立静态核查 `evidence/software_static_check_server_migration.py`（EXIT 0/1，不依赖实现侧声明）：A 常量值精确比对（含 URI/基址拼接结果）；B 旧主机分布（Kotlin 源码 0、非 Kotlin 仅允许 1 处且必须是该 XML）；C 新主机集中性 + `app/src/main/**/*.kt` 的 IPv4 字面量全集；D XML 可解析、`domain-config` 恰 3 个且均 `cleartextTrafficPermitted=true`、域名集合精确相等；E 兼容面（证书 md5、TLSv1.2、信任锚、认证拼接、MTU、AES、`ApiService` 5 端点）。
另做**产物级**核查：直接解析 `app-debug.apk` 内 `res/xml/network_security_config.xml`（二进制 AXML）的字符串池与 `assets/jd-ca.crt` 摘要。

### 3. 原始结果

```
$ python evidence/software_static_check_server_migration.py   # EXIT=0
== A CloudConfig 常量 ==           SERVER_HOST / MQTT_BROKER_PORT=8883 / HTTP_PORT=5000 / MQTT_BROKER_URI / HTTP_BASE_URL 全 PASS
  解析结果: ssl://8.140.23.253:8883 ; http://8.140.23.253:5000
== B 旧主机 117.72.84.210 ==      Kotlin/Java 命中 0；非 Kotlin 命中 1（network_security_config.xml:9，设计允许保留项）
== C 新主机集中性 ==               含新主机 main Kotlin 文件 = [cloud/CloudConfig.kt]；main Kotlin IPv4 字面量全集 = [8.140.23.253]
== D 明文放行 ==                   domain-config=3，cleartext=[true,true,true]，domains=[8.140.23.253, 117.72.84.210, 192.168.4.1]
== E 兼容面 ==                     E1..E9 全 PASS（jd-ca.crt md5=f1a8212e3c690d57894b8a3a5fd901ab）
== RESULT ==                       OK
```

产物级核查（直接读 APK，非源码）：

```
res/xml/network_security_config.xml (844 B, 头部 03000800=AXML) 字符串池：
  8.140.23.253: utf8=True   117.72.84.210: utf8=True   192.168.4.1: utf8=True   9.9.9.9(负对照): False
assets/jd-ca.crt: 1512 B, md5 f1a8212e3c690d57894b8a3a5fd901ab（与迁移记录 §三 一致）
```

构建与回归：

| 检查 | 命令 | 结果 |
|---|---|---|
| 宿主机单测 | `./gradlew :app:testDebugUnitTest --offline` | BUILD SUCCESSFUL；7 套件 **49 tests / 0 failures / 0 errors / 0 skipped**（本轮新增 `CloudConfigTest` 3 项全过） |
| 构建与资源合并 | `./gradlew :app:assembleDebug --offline` | BUILD SUCCESSFUL；`app-debug.apk` 10,359,236 B |
| 前项静态口径 | `python evidence/software_static_check_no_little_endian_parser.py` | EXIT=0（ITEM-002 门禁未被破坏） |
| 差异范围 | `git diff --name-only e4982e5 5090f5d` | 仅上列 4 个产品文件 + 2 个新增文件 + 文档/清单；**`ApiService.kt` 未改** |

### 4. 判定

1. **地址迁移与集中**：`CloudConfig` 5 个常量精确等于约定值（`ssl://8.140.23.253:8883`、`http://8.140.23.253:5000`）；`app/src/main` 中唯一 IPv4 字面量就在该文件；broker URI 与 Retrofit baseUrl 均引用该常量。**通过**
2. **旧主机不再是连接目标**：Kotlin/Java 源码 0 命中；全 `app/src` 仅 XML 保留 1 处（设计要求的追加保留项，非连接目标）。按 §1 口径 **通过**；字面差异见 §5-1。
3. **明文放行最小化**：源码与**打包后**的 AXML 字符串池都恰含 3 个主机（含负对照验证），无新增主机，均允许明文。**通过**
4. **兼容面未变**：`jd-ca.crt` md5 一致（源码与 APK 内均一致）、`TLSv1.2`、`CertificateFactory`+jd-ca.crt 信任锚、`options.userName=MAC`、`options.password=MAC+"&^!A:z?"`、`requestMtu(240)`、`APP_AES_KEY16`、`ApiService` 既有 5 端点均原样。**通过**
5. **资源合并与构建**：`assembleDebug` 成功，合并后的二进制 XML 已含新主机。**通过**

结论：**TEST_PASS**（依据 §1 判定口径：连接目标清零 + 明文放行按设计追加且不扩大）。

### 5. 交接与观察

1. **字面差异（需产品/上游确认，不影响本项功能判定）**：任务描述要求“App 内不再出现 `117.72.84.210`”，而已批准设计 AA-002 §5.6/D-13 与 TD-SW-002 T-SW-L1-06 要求“追加而非替换”；实现按设计保留 XML 条目。若项目要求字面清零（含已退役主机的明文放行条目），属设计变更（删 1 个 `domain-config` 块），需上游确认后由实现能力处理；本能力未自行改判。
2. **运行期连通性/TLS 未在本轮验证**：本能力工具边界内无网络探测；`ssl://8.140.23.253:8883` 的握手/认证与 `http://8.140.23.253:5000` 可达性属 E2E-SW-002 J-5（T-SW-L2-01/L2-12）。本项期望为配置级，已由上述静态与产物级证据充分覆盖；不把静态结论当作链路连通性结论。
3. **仍引用旧地址的非编译材料**（未改动，属文档/iOS 资料维护或 ITEM-014）：`_REFACTOR_NOTES.md`、`iOS开发所需资料清单.md`、`iOS版功能需求文档.md`、`iOS开发资料/**`。
4. **本项未涉及**：`/upload_data` 端点与上传编排（ITEM-008/009）、SQLite v3（ITEM-006）、告警时间窗（ITEM-005）、UI（ITEM-011/012）、版本/文档（ITEM-014）。
