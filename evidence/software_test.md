# 软件验证证据（ST-002）

状态：软件测试执行证据（`software_tester.software_verification`），按 Runtime 逐个指派的队列项追加
对象：Android App `dengbei_care`（Kotlin / AGP 8.3.2 / Gradle 8.4 / JDK 21）
本轮项：队列 **ITEM-001**…**ITEM-010**（逐项分节记录；**ST-010 = TEST_FAIL**）
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

---

## ST-005 · ITEM-005（告警时间窗纯逻辑 `telemetry/AlarmEvaluator.kt`）

**被测代码版本**：`a6bfa6d`（android_engineer.android_implementation 提交，相对上一提交 `87b80ab`）
**变更范围**：**仅新增** `telemetry/AlarmEvaluator.kt`（179 行）+ `telemetry/AlarmEvaluatorTest.kt`（308 行）；`git diff --name-only 87b80ab a6bfa6d` 确认**未修改任何现有产品文件**（`MqtttService.kt` 未变）
**任务期望**：时间窗下降判定（5/10/15 min，基准为不晚于 `now-W` 且不早于 `now-W-回溯容差` 的最新样本）；阈值超限与紧急（>65.0℃）语义保持现状；基准缺失/无历史时下降不成立；恰好等于阈值不触发；不写入任何伪造温度/湿度。

### 1. 适用测试的选择与理由

| TD-SW-002 用例 | 是否适用 | 理由 |
|---|---|---|
| T-SW-L0-06（未设置报警） | 适用 | 模块输出契约 |
| T-SW-L0-07（时间窗下降与边界） | 适用 | 本项核心改动，纯逻辑可直接穷举/属性验证 |
| T-SW-L0-08（阈值超限保持现状） | 适用 | 严格比较语义 |
| T-SW-L0-09（优先级与文案保持现状） | 适用 | 两开关都开时的覆盖关系 |
| T-SW-L0-10（紧急 >65.0） | 适用 | 阈值、级别、每次通知标记 |
| T-SW-L2-13/L2-14（告警链路与去抖集成） | **本轮不适用** | 本项**未接线**（`MqtttService` 仍用旧内存窗口逻辑，接线属后续队列项）；去抖/通知/落库均在 Service |

### 2. 独立验证方法

新增 `app/src/test/java/com/jinyuni/dengbei_care/verification/AlarmEvaluatorItem005VerificationTest.kt`（10 项），期望值由本文件按 AA-002 §7 / D-04 / D-05 独立重写参考实现得出：
1. **时间窗边界表**：单一基准位于 `now-35min`（5 min 窗口下界含端点）/`now-36min`/`now-40min`/`now-41min`/`now-45min`（15 min 下界含端点）/`now-46min`（超出回溯）/`now-4min`（过新）/`now`（当前时刻）→ 期望命中窗口或 NONE；
2. **每窗口取最新合格基准**：`now-35min` 与 `now-12min` 同时存在时命中 5 min 窗口（取最新）；
3. **无基准/无历史**：空历史、样本全部过新（“最近 3 个样本”旧语义的**负对照**）、温度上升 → 均不报警；
4. **严格比较**：温度/湿度差恰好等于阈值不触发、再多 0.1 才触发；四个上下限恰好等于不触发、略超才触发；
5. **开关矩阵 ×4 与优先级**：两关→`未设置报警`；只开一个→只判该项；都开且同时成立→超限文案覆盖下降（与改动前一致）；
6. **紧急阈值**：`65.0` 不触发、`>65.0` 触发且覆盖一切（含两开关均关），文案/级别/`notifiesImmediately` 断言；
7. **文案字面值与常量**（兼容面）：五条文案、`DROP_WINDOW_MINUTES=[5,10,15]`、`EMERGENCY_TEMP_C=65.0`、`DEFAULT_LOOKBACK_SLACK_SECONDS=1800`；
8. **纯函数性与无伪造数值**：同输入结果相同、不修改入参列表（快照比对）、`Result` 无任何测量字段（无 Double/Float、无 temp/humi/reading 命名）；
9. **随机属性比对**：4000 例（随机参数含开关组合、0–5 个随机时段样本含窗口边界偏移、随机当前读数）逐例与独立参考比较 outcome/message/severity/window；
10. **异常鲁棒性**：非有限基准不得成为下降依据、非有限当前值不得抛异常。

### 3. 原始结果

```
$ ./gradlew :app:testDebugUnitTest --offline
BUILD SUCCESSFUL in 3s
```

| 套件 | tests | failures | errors | skipped |
|---|---|---|---|---|
| `ExampleUnitTest` | 1 | 0 | 0 | 0 |
| `cloud.CloudConfigTest` | 3 | 0 | 0 | 0 |
| `protocol.GatewayFrameCodecTest` | 11 | 0 | 0 | 0 |
| `telemetry.AlarmEvaluatorTest`（实现侧自检） | 23 | 0 | 0 | 0 |
| `telemetry.ReadingAttributionTest` | 16 | 0 | 0 | 0 |
| `verification.AlarmEvaluatorItem005VerificationTest`（独立，本轮新增） | 10 | 0 | 0 | 0 |
| `verification.GatewayFrameCodecItem001LegacyParityTest` | 3 | 0 | 0 | 0 |
| `verification.GatewayFrameCodecItem001VerificationTest` | 7 | 0 | 0 | 0 |
| `verification.ReadingAttributionItem003VerificationTest` | 8 | 0 | 0 | 0 |
| **合计** | **82** | **0** | **0** | **0** |

本轮独立用例：`dropWindowBoundaries_matchSpec`、`latestEligibleSampleIsChosenPerWindow`、`dropRequiresEligibleBaseline`、`strictComparisons_exactEqualityDoesNotTrigger`、`switchMatrix_andPrecedence_matchLegacy`、`emergencyThreshold_isStrictAndHasPriority`、`messageLiterals_areExact`、`evaluate_isPure_andResultCarriesNoMeasurements`、`randomCases_matchIndependentReference`、`nonFiniteInputs_neverThrow_andNonFiniteBaselineDoesNotAlarm` —— **10/10 PASS**。

构建与回归：

| 检查 | 命令 | 结果 |
|---|---|---|
| 构建/打包 | `./gradlew :app:assembleDebug --offline` | BUILD SUCCESSFUL；`app-debug.apk` 10,359,236 B；`app/build/tmp/kotlin-classes/debug/.../telemetry/AlarmEvaluator.class` 已产出 |
| 前项静态口径 | ITEM-002 / ITEM-004 两个独立脚本 | 均 EXIT=0 |
| 差异范围 | `git diff --name-only 87b80ab a6bfa6d` | 仅新增模块 + 测试 + 文档/清单；**`MqtttService.kt` 等生产文件未被修改** |

### 4. 判定

1. **时间窗判定**：基准选取区间 `[now-W-slack, now-W]`（两端含）、每窗口取最新合格样本、窗口按 5→10→15 升序首个成立者命中；边界逐一验证并与独立参考在 4000 随机例上完全一致。**通过**
2. **无基准不成立**：空历史/样本全过新/超出回溯均不报警；“最近 3 个样本近 1 分钟”负对照不报警（证明已不再按样本序号）。**通过**
3. **阈值与紧急保持现状**：严格比较（恰好等于阈值、恰好 65.0 均不触发）、紧急覆盖一切且与开关无关、>65 每次通知标记正确；文案字面值精确。**通过**
4. **不写入伪造数值**：纯函数、不改入参、`Result` 不含任何测量字段。**通过**
5. **接入与运行行为未变**：`AlarmEvaluator` 被生产代码引用次数 = **0**（除自身文件），`MqtttService` 未改，故本项**不改变现有运行行为**（接线属后续队列项）。**通过**

结论：**TEST_PASS**（本项为模块级交付；不断言“告警链路已改造”）。

### 5. 交接与观察

1. **`matchedWindowMinutes` 语义**：因回溯容差（30 min）远大于窗口差（5 min），同一基准可同时满足多个窗口；模块返回升序首个成立窗口。已实测 `now-35min→5`、`now-36min→10`、`now-41min→15`、`now-46min→null`（与实现记录一致），该字段仅用于日志/证据，不影响报警与否。
2. **`Outcome.NONE` 文案为 `无报警`**（区别于“两开关都关”的 `未设置报警`）：与已批准设计 AA-002 §7 列出的四种结论一致；两者严重级别均为 NORMAL，卡片不展示、不落 `alarm_events`，因此**生产可见行为不变**；具体落库分支需在接线后按 ITEM-010 复核。
3. **接线与基准来源属后续项**：① `processTemperatureHumidityData` 改用本模块（含 `Severity` 映射）与 900 s 去抖/通知保留 → 后续队列项；② 基准样本是否来自本地库（设计 D-05，支持进程重启后仍可判定）→ 库 API 与接线项；TD-SW-002 §8-G4 已登记。
4. **本项不涉及**：SQLite v3 与真实时间戳（ITEM-006/007）、云上传（ITEM-008/009）、UI 呈现（ITEM-011/012）、通知去抖（900 s，仍在 Service）。

---

## ST-006 · ITEM-006（SQLite v3：`pending_uploads` 表与迁移 `TemperatureDatabaseHelper.kt`）

**被测代码版本**：`557cf77`（android_engineer.android_implementation 提交，相对上一提交 `c232105`）
**变更范围**：仅 `app/src/main/java/com/jinyuni/dengbei_care/TemperatureDatabaseHelper.kt`（+62/−5）+ 实现侧宿主机测试；`git diff --name-only c232105 557cf77` 确认无其它产品文件改动
**任务期望**：`DATABASE_VERSION = 3`；新增 `pending_uploads(dev_id, time, temperature, humidity, attempts, next_attempt_at, PK(dev_id,time))` 与其 `next_attempt_at` 索引；迁移对既有 `temperature`/`alarm_events` 结构与数据不动；v2→v3 后历史可查；重复创建不报错；迁移异常按既有回退策略处理且不崩溃。

### 1. 适用测试的选择与理由

| TD-SW-002 用例 | 是否适用 | 本轮结果 |
|---|---|---|
| T-SW-L0d-01（v2→v3 保数据 + 新表/索引/PK） | 适用 | **SQL 语义层已执行**（宿主 SQLite + 从源码抽取的语句）；**Android 运行时经验层未执行**（见 §4 环境阻塞） |
| T-SW-L0d-02（v1→v3） | 适用 | 同上（v1→v2 既有语句逐字转写 + v2→v3） |
| T-SW-L0d-03（迁移异常回退不崩溃） | 适用 | 代码路径静态核查 + 宿主 SQLite 回退序列执行 |
| T-SW-L0d-06（既有查询 API 未破坏） | 适用 | 静态核查 DDL/索引未变（查询实现未改）+ 宿主 SQL 数据保真 |
| T-SW-L0d-04（真实时间戳单条写入） | **不适用** | 本项未改 `storeTemperatureData`（属 ITEM-007） |
| T-SW-L1-01/L1-02（构建、宿主机单测） | 适用 | 已执行（91 项 0 失败） |

### 2. 独立验证方法

新增可复跑独立脚本 `evidence/software_verify_db_migration_item006.py`：
- **语句不手抄**：从被测 Kotlin 源码**抽取** v3 两条 DDL 与 `MIGRATION_V2_TO_V3` 列表（解析 `const val` 拼接与 `$常量` 插值），并与期望形状精确比对；
- **基线不变**：对改动前提交 `c232105` 抽取同一组 DDL 常量并逐字比对（`temperature`/`alarm_events`/两索引）；
- **结构分析**：解析 `onCreate` 与 `if (oldVersion < 3)` 分支，断言 v2→v3 仅含 `CREATE`（回退时仅 DROP 新对象），无对既有表的 `DROP/ALTER/UPDATE/DELETE`；
- **执行语义**：在宿主 SQLite 上以**独立构造的 v2/v1 库与数据**执行抽取出的迁移语句，断言数据保真、表/列/PK/索引、默认值、`OR REPLACE` 幂等、重复键拒绝、重复迁移幂等、冲突对象回退、v1→v3 路径。
另编写**真机 instrumented 独立验证** `app/src/androidTest/java/com/jinyuni/dengbei_care/verification/TemperatureDatabaseItem006VerificationTest.kt`（5 项：v2→v3 保数据+新 schema、幂等键与默认值、重复打开、v1→v3 不崩溃、同名 VIEW 冲突回退不崩溃），使用测试专用库名，不触碰 App 正式库。

### 3. 原始结果

**独立脚本（EXIT=0）**：

```
== A 源码事实 ==
  PASS  DATABASE_VERSION = 3
  PASS  v3 待发箱 DDL 与期望逐字一致（含 PRIMARY KEY(device_id, time)、attempts/next_attempt_at DEFAULT 0）
  PASS  v3 索引 DDL 与期望逐字一致
  PASS  MIGRATION_V2_TO_V3 = ['SQL_CREATE_TABLE_PENDING_UPLOADS', 'SQL_CREATE_INDEX_PENDING_UPLOADS_NEXT']
  PASS  onCreate 含 v3 新表与索引
  PASS  v2→v3 分支仅含 CREATE 与（回退时）新对象 DROP；无对既有表的 DROP/ALTER/UPDATE
== B 既有结构（对照改动前提交 c232105） ==
  PASS  SQL_CREATE_TABLE_TEMPERATURE / SQL_CREATE_TABLE_ALARM_EVENTS / 两条索引 均逐字未变
  INFO  基线 DATABASE_VERSION = 2（本项升为 3）
== C1 v2 + 数据 → 迁移 ==
  PASS  历史温湿度 3 行保真；历史报警记录保真
  PASS  pending_uploads=table；列顺序=device_id,time,temperature,humidity,attempts,next_attempt_at
  PASS  复合主键 = {device_id:1, time:2}；索引 idx_pending_uploads_next(next_attempt_at)
  PASS  既有索引 idx_temperature_device / idx_alarm_device 保留
  PASS  同键 OR REPLACE → 1 行且 attempts=0/next_attempt_at=0；普通重复 INSERT 触发主键约束
== C2 ==  PASS  迁移语句重复执行无报错
== C3 ==  INFO  首次迁移（存在同名 VIEW）报错 views may not be indexed；PASS 按回退序列后 pending_uploads=table 且历史数据仍 3 行
== C4 ==  PASS  v1 两行保留且 device_id 回填；v1→v3 后 alarm_events/pending_uploads 齐备
== RESULT ==  OK
```

**宿主机回归与构建**：

| 检查 | 命令 | 结果 |
|---|---|---|
| 宿主机单测（10 套件） | `./gradlew :app:testDebugUnitTest --offline` | BUILD SUCCESSFUL；**91 tests / 0 failures / 0 errors / 0 skipped**（含实现侧 `DatabaseSchemaV3Test` 9 项） |
| 构建 | `./gradlew :app:assembleDebug --offline` | BUILD SUCCESSFUL；`app-debug.apk` 10,360,124 B |
| 真机测试可编译可打包 | `./gradlew :app:compileDebugAndroidTestKotlin --offline` | BUILD SUCCESSFUL；`app/build/outputs/apk/androidTest/debug/app-debug-androidTest.apk` 686,747 B |
| 前项静态口径 | ITEM-002 / ITEM-004 脚本 | 均 EXIT=0 |

### 4. Android 运行时（设备）经验层：未执行 —— 环境阻塞（已如实记录）

已尝试在已授权目标设备（`android_test devices` → `fy5tibu88pcambxo device`）上运行 instrumented 测试：

| 尝试 | 结果 |
|---|---|
| `./gradlew :app:connectedDebugAndroidTest --offline` | FAILED：`Could not download kotlinx-coroutines-core-jvm-1.6.4.jar … No cached version available for offline mode`（AGP 8.3 unified-test-platform 的 `android-device-provider-ddmlib` 依赖） |
| 同上（联网） | FAILED：`Connect to 127.0.0.1:10808 failed: Connection refused`（本机代理不可用，无外网） |
| `-Pandroid.experimental.androidTest.useUnifiedTestPlatform=false` | 仍要求同一依赖，无法绕过 |
| 本地缓存检查 | `~/.gradle/caches/modules-2/files-2.1/org.jetbrains.kotlinx/` 不存在 |

本能力工具边界仅授权 `android_test`（devices/logcat/screenshot/install/launch/tap）与 `usb_list_devices`，**不含直接 `adb shell am instrument`**，因此不以绕过方式执行。
**结论：设备端经验层（在真实 Android SQLite 上跑 `SQLiteOpenHelper` 升级）本轮未执行**；不得将 §3 的宿主 SQL 语义证据解读为“已在设备上验证”。该层已交接给后续完整回归能力（如具备缓存依赖/网络即可直接跑上述已编译的 instrumented 测试）。

### 5. 判定

1. **版本与新 schema**：`DATABASE_VERSION == 3`；两条新 DDL 与期望逐字一致；`MIGRATION_V2_TO_V3` 恰为这两条；`onCreate` 也建新对象。**通过**
2. **既有结构不变**：`temperature`/`alarm_events` 与两索引的 DDL 与改动前提交逐字相同（仅 `private const val` → `const val` 可见性变化）；v2→v3 分支无任何针对既有表的 DROP/ALTER/UPDATE/DELETE。**通过**
3. **迁移语义（宿主 SQLite，执行抽取出的语句）**：v2 历史温湿度与报警行全部保真；新表列/复合主键/索引正确；默认值 0；同键 `OR REPLACE` 幂等、普通重复 INSERT 被主键拒绝；重复迁移不报错；冲突对象经回退序列恢复为表且历史数据不被重建；v1→v3 后历史行保留且三表齐备。**通过**
4. **不崩溃回退**：v2→v3 分支为 try/catch，首次失败仅清理新同名对象后重试，仍失败只记日志不抛出（静态核查 + 回退序列执行）。**通过**

结论：**TEST_PASS**（范围：schema/迁移语句与既有结构不变、迁移语义的宿主 SQL 执行、宿主机回归与构建；**不含**设备端 Android 运行时经验层——该层因环境阻塞未执行，已登记交接）。

### 6. 交接与观察

1. **设备端经验层待补**：上文 instrumented 测试已就绪且可编译；任何具备 Gradle 缓存依赖或外网的环境执行 `./gradlew :app:connectedDebugAndroidTest` 即可补齐 T-SW-L0d-01/02/03/06 的设备证据。
2. **冲突回退的行为细节**：宿主 SQLite 下首次迁移报 `views may not be indexed`（首条 `CREATE TABLE IF NOT EXISTS` 同名 VIEW 时不报错，随后建索引才报错），回退序列 `DROP VIEW/TABLE IF EXISTS` + 重试后得到正确的表且历史数据完好；Android SQLite 的错误文本可能不同，但回退结构一致。该场景为人为构造的异常预置，生产路径不会创建同名对象。
3. **v1→v3 的 device_id 回填值**：沿用既有 v1→v2 策略（`MacIdBook` 第一条，否则 `UNKNOWN`）；本项未改该策略。
4. **待发箱读写 API 不在本项**：`enqueue/peekBatch/ack/markFailed` 属后续队列项；若迁移最终失败导致表缺失，待发箱访问需容忍（实现侧已登记）。
5. **本项不涉及**：入库真实时间戳与单条写入（ITEM-007）、上传编排（ITEM-009）、Service 接线（ITEM-010）、UI（ITEM-011/012）。

---

## ST-007 · ITEM-007（真实采样时间戳与单条写入 API）

**被测代码版本**：`886dbd1`（android_engineer.android_implementation 提交，相对上一提交 `0bebe43`）
**变更范围**：`TemperatureDatabaseHelper.kt`（列表式 `storeTemperatureData` → 单条 `storeTemperatureReading`，新增 `SQL_INSERT_TEMPERATURE_READING` 与 `isGuardActive`，顶层包装同步）+ `MqtttService.kt`（存储段改调单条 API，+4/−4）+ 实现侧宿主机测试
**任务期望**：新增单条读数写入 API（devId、真实采样时间戳、温度、湿度、守护标志）；BLE 路径用接收时刻、MQTT 路径用 payload 首段时间；不再按 60 s 回填；写入行 `time` 等于真实采样时间；同 `(dev_id, time)` 不产生重复行；`storeflag != 1` 不写入任何行。

### 1. 适用测试的选择与理由

| TD-SW-002 用例 | 是否适用 | 本轮结果 |
|---|---|---|
| T-SW-L0d-04（单条写入真实时间戳/幂等/门控） | 适用 | **语句级与门控级已执行**（源码抽取 + 宿主 SQLite）；**Android 行级经验层未执行**（同 ST-006 环境阻塞） |
| T-SW-L1-04（无 60 s 回填残留） | 适用 | 已执行（静态分布核查） |
| T-SW-L1-01/L1-02（构建、宿主机单测） | 适用 | 已执行（99 项 0 失败） |
| T-SW-L2-*（MQTT/BLE 链路时间语义端到端） | 本轮不适用 | 需真实 broker/网关；入口时间语义已由静态调用点核查覆盖，端到端属 E2E-SW-002 J-1/J-2 |

### 2. 独立验证方法

1. 独立脚本 `evidence/software_verify_timestamp_write_item007.py`：从源码**抽取** `SQL_INSERT_TEMPERATURE_READING`/`CREATE TABLE temperature`/`isGuardActive` 与两大调用点形状，断言门控在打开数据库之前、列序=绑定顺序、旧列表式 API 清零、`60L` 仅存于 `AlarmEvaluator` 时间窗口算术；再在**宿主 SQLite** 执行抽取出的插入语句验证时间/幂等/值位置。
2. 宿主机独立单测 `verification/TemperatureWriteItem007VerificationTest.kt`（4 项）：插入语句逐字期望、`isGuardActive` 在 −1000..1000 上仅 1 为真、单条 API 反射形状（5 参数返回 Long）且旧列表式方法不存在、顶层包装仅暴露单条写入。
3. 真机 instrumented 独立验证 `androidTest/.../TemperatureWriteItem007VerificationTest.kt`（3 项）：真实写入行 `time` 恰为传入值且显式排除 ±60/±120 s、值不串位；同 `(dev,time)` 重复写入不增行；`storeflag ∈ {0,2,−1}` 写 0 行且返回 −1。

### 3. 原始结果

**独立脚本（EXIT=0）**：

```
== A 源码事实 ==
  PASS  storeTemperatureReading 参数 = [devId: String, time: Long, temperature: Double, humidity: Double, storeflag: Int]
  PASS  门控位于打开数据库之前（gate@9 < db@80）
  PASS  isGuardActive = storeflag == 1
  PASS  插入语句 = 'INSERT OR REPLACE INTO temperature (time, device_id, temperature, humidity) VALUES (?, ?, ?, ?)'，列序 = [time, device_id, temperature, humidity]
  PASS  旧列表式 API 清除（storeTemperatureData / averageList0 / averageList1 残留=[]）
  PASS  含 60L 的 main Kotlin 文件 = [telemetry/AlarmEvaluator.kt]（仅时间窗口算术）
  PASS  currentTime = timeOverride ?: 接收时刻；BLE 调用不传 timeOverride（2 处）；MQTT 传 batch.timeSeconds；存储调用实参顺序正确
== B 宿主 SQLite 执行抽取出的语句 ==
  PASS  B1/B2 写入行 = [(1700000123, 'A1B2C3D4', 25.5, 60.0)]（time 无偏移、列不串位）
  PASS  B3 同 (time, device_id) 重复写入 → [(1, 26.5, 61.5)]（1 行且为新值）
  PASS  B4 不同 time → 两行
== RESULT ==  OK
```

**宿主机回归与构建**：

| 检查 | 命令 | 结果 |
|---|---|---|
| 宿主机单测（12 套件） | `./gradlew :app:testDebugUnitTest --offline` | BUILD SUCCESSFUL；**99 tests / 0 failures / 0 errors / 0 skipped**（含本轮新增独立 4 项） |
| 构建 | `./gradlew :app:assembleDebug --offline` | BUILD SUCCESSFUL；`app-debug.apk` 10,359,865 B（与实现侧记录一致） |
| 真机测试可编译可打包 | `./gradlew :app:compileDebugAndroidTestKotlin --offline` | BUILD SUCCESSFUL；APK 含 `TemperatureWriteItem007VerificationTest` 与 ITEM-006 验证类（dex 字符串核查） |
| 前项独立脚本 | ITEM-002 / ITEM-004 / ITEM-006 脚本 | 均 EXIT=0 |

### 4. Android 行级经验层：未执行 —— 环境阻塞（同 ST-006）

`./gradlew :app:connectedDebugAndroidTest --offline` 再次失败：`Could not download kotlinx-coroutines-core-jvm-1.6.4.jar … No cached version available for offline mode`（AGP UTP 设备提供者依赖；无外网且缓存缺失）；本能力工具边界不含直接 `am instrument`，故不绕过。**不得将 §3 的宿主 SQL/门控证据解读为“已在设备上写入验证”。**

### 5. 判定

1. **单条 API 与门控**：`storeTemperatureReading(devId, time, temperature, humidity, storeflag): Long` 存在；门控在访问数据库**之前**（`storeflag != 1` 直接返回 −1）；`isGuardActive` 在 −1000..1000 上仅 1 为真；旧列表式 API/临时列表已从 main 清除（含顶层包装）。**通过**
2. **真实时间戳与无回填**：写入行 `time` 恰为传入值；插入语句列序=绑定顺序，宿主执行后值不串位；入库路径无 `60L`（全 main 仅 AlarmEvaluator 时间窗口算术）；BLE 调用不传 `timeOverride`（→接收时刻），MQTT 调用传 `batch.timeSeconds`（payload 首段）。**通过**
3. **幂等**：同 `(time, device_id)`（即 `(dev_id, time)`）重复写入 → 1 行且为新值（`INSERT OR REPLACE` + 复合主键）；不同 time 新增行。**通过**
4. **回归**：99 项宿主机用例全绿、构建成功、前项独立脚本均通过。**通过**

结论：**TEST_PASS**（范围：单条写入 API/门控/幂等/时间语义的语句级与入口级验证 + 宿主机回归与构建；**不含**设备端行级经验层，该层因环境阻塞未执行）。

### 6. 交接与观察

1. **设备端行级层待补**：上文 instrumented 测试已就绪且已打包进测试 APK；具备 Gradle 缓存依赖或外网的环境执行 `./gradlew :app:connectedDebugAndroidTest` 即可补齐 T-SW-L0d-04 设备证据（与 ST-006 同一阻塞）。
2. **`INSERT OR REPLACE` 语义**：同键重复写入会替换原行（rowId 可能变化），满足“不产生重复行”；若后续需要“保留首次值”属新需求。
3. **`60L` 静态口径**：全仓字面清零需显式排除 `AlarmEvaluator` 的时间窗口算术（本项已在脚本中固化该例外）。
4. **iOS 参考快照**仍含旧列表式 API 与 60 s 回填（非编译、不参与构建，readme 点 6 本轮不改 iOS），属文档/iOS 资料维护交接项。
5. **本项不涉及**：待发箱 DAO 与上传编排（ITEM-009）、Service 接入 `AlarmEvaluator`（ITEM-010）、UI（ITEM-011/012）。

---

## ST-008 · ITEM-008（`/upload_data` 端点与请求体构造 `cloud/UploadPayload.kt`）

**被测代码版本**：`0c45960`（android_engineer.android_implementation 提交，相对上一提交 `cec8e01`）
**变更范围**：`ui/zhuce/ApiService.kt`（**纯追加**：`@Headers`+`@POST("/upload_data")` 的 `uploadData` 与两个 DTO）；新增 `cloud/UploadPayload.kt`；实现侧宿主机测试。
**任务期望**：字段名与服务器契约一致（`phone`/`mac`/`readings[{devId,time,temperature,humidity}]`）；`temperature` ℃、`humidity` %RH、`time` Unix 秒；`devId` 来自记录自身归因；既有 5 个端点签名与数据类字段不变。

### 1. 适用测试的选择与理由

| TD-SW-002 用例 | 是否适用 | 本轮结果 |
|---|---|---|
| T-SW-L0-11（上传载荷构造：字段名/单位/时间/devId/空集） | 适用 | 已执行（逐字 JSON + 结构 + 反射 + 静态） |
| T-SW-L0d-07（既有 5 端点契约不变） | 适用 | 已执行（基线行作为有序子序列 + 端点清单） |
| T-SW-L1-01/L1-02（构建、宿主机单测） | 适用 | 已执行（119 项 0 失败） |
| T-SW-L2-12（真实 `/upload_data` 幂等与 `inserted` 计数） | **本轮不适用** | 本能力本次未授权 `api_client` 且本机无外网（代理拒绝）；属 E2E-SW-002 J-1/J-5，不作为本项判定依据 |

### 2. 独立验证方法

1. 独立脚本 `evidence/software_verify_upload_payload_item008.py`：把改动前 `ApiService.kt` 的**每一行**作为有序子序列在现文件中断言（任何既有行被删/改即失败）；端点清单与返回类型；新 DTO 字段名/类型；**单位仅一次换算**链条（非注释代码中 `GatewayFrameCodec` 恰 2 处、`ReadingAttribution`/`UploadPayload` 各 0 处、`MqtttService` 仅存于无调用点的旧解析族）；`UploadPayload` 无副作用依赖且两入口均空集返回 null。
2. 宿主机独立单测 `verification/UploadPayloadItem008VerificationTest.kt`（9 项）：独立写出的**逐字 JSON**、JSON key 集合与类型（`time` 必须是 JSON 数字）、单位不二次换算（含负对照与负温保号）、`time` 非毫秒、`devId` 与数值成对随读数下沉、空集不产生请求体与待发箱逐行时间、新增/既有 DTO 字段与类型、`ApiService` 6 端点（注解路径/suspend/参数类型/JSON 头）。

### 3. 原始结果

**独立脚本（EXIT=0）**：

```
== A 既有端点/数据类未被修改 ==
  PASS  基线 31 行全部按序保留（缺失/被改=0）
  PASS  端点清单 = [/sendVerificationCode, /verifyCode, /register, /login, /setmac, /upload_data]
  PASS  6 个端点均返回 Response（Response<Void>×5 + Response<LoginResponse>）
== B 新增 DTO 字段名/类型 ==
  PASS  SensorReading = [(devId,String),(time,Long),(temperature,Double),(humidity,Double)]
  PASS  SensorUploadData = [(phone,String),(mac,String),(readings,List<SensorReading>)]
== C 单位只换算一次 ==
  PASS  非注释换算计数 = {GatewayFrameCodec:2, ReadingAttribution:0, UploadPayload:0, MqtttService:2}
  PASS  MqtttService: 旧解析族内换算=2、存活路径换算=0、interpretPayloadV3 调用点=0
== D UploadPayload 纯逻辑与空集门控 ==
  PASS  无副作用依赖（getSharedPreferences/Retrofit/OkHttp/HttpURLConnection/writableDatabase 均未命中）
  PASS  两个入口均有空集返回 null
== RESULT ==  OK
```

**宿主机独立单测（9/9）**：`serializedJson_isExactlyTheServerContract`、`jsonStructure_hasExactKeysAndTypes`、`unitsArePassedThrough_soNoDoubleScaling`、`timeIsUnixSeconds_notMillis`、`devIdComesFromEachReading_notFromIndex`、`emptyReadings_produceNoPayload_butOutboxPathKeepsPerRowTimes`、`dtoFieldNamesAndTypes_matchServerContract`、`existingDataClasses_areUnchanged`、`apiService_endpointInventoryAndContentType`。逐字 JSON 期望：

```
{"phone":"13800000000","mac":"1010100000A1","readings":[{"devId":"A1B2C3D4","time":1699999999,"temperature":25.5,"humidity":60.0}]}
```

| 检查 | 命令 | 结果 |
|---|---|---|
| 宿主机单测（14 套件） | `./gradlew :app:testDebugUnitTest --offline` | BUILD SUCCESSFUL；**119 tests / 0 failures / 0 errors / 0 skipped** |
| 构建 | `./gradlew :app:assembleDebug --offline` | BUILD SUCCESSFUL；`app-debug.apk` 10,359,867 B（与实现侧一致） |
| 前项独立脚本 | ITEM-002/004/006/007 脚本 | 均 EXIT=0 |
| 差异范围 | `git diff --name-only cec8e01 0c45960` | 仅 `ApiService.kt`（纯追加）、新增 `UploadPayload.kt`、实现侧测试 + 文档/清单 |

### 4. 判定

1. **字段名与单位**：序列化结果与服务器契约**逐字一致**；JSON key 集合精确；`time` 为 Unix 秒（JSON 数字、非毫秒）；`temperature`/`humidity` 为 ℃/%RH，且**不再二次换算**（换算只发生在 `GatewayFrameCodec` 的 2 处；`ReadingAttribution`/`UploadPayload` 为 0）；负温保号。**通过**
2. **`devId` 归属**：`readings` 的 `devId` 来自读数自身（打乱顺序后 `devId` 与数值仍成对），不依赖下标。**通过**
3. **空集**：两个入口均返回 null（不产生请求）。**通过**
4. **既有端点/数据类不变**：基线 31 行全部按序保留（含 5 个端点与 6 个既有数据类），端点清单恰为 5 旧 + `/upload_data`，返回类型与签名未变；新增端点为 JSON POST 且参数为 `SensorUploadData`。**通过**
5. **回归**：119 项宿主机用例全绿、构建成功、前项独立脚本均通过。**通过**

结论：**TEST_PASS**（契约级：字段/单位/归属/幂等键形式与既有端点兼容性；不含真实服务器调用）。

### 5. 交接与观察

1. **真实 `/upload_data` 调用未执行**：本次未授权 `api_client` 且本机无外网（代理 127.0.0.1:10808 拒绝）；T-SW-L2-12 的 HTTP 状态/`inserted` 幂等核对属 E2E-SW-002 J-1/J-5。本项为端点/载荷构造交付，契约级证据已充分（不声称服务端已联调）。
2. **响应体未建模**：接口用 `Response<Void>`（仅 2xx 判定）；服务端实际返回 `inserted` 计数。若后续需在 App 侧读取，属新增需求（一处类型改动）。
3. **手机号/守护门控**（空手机号不入队、守护关闭不入队）属后续队列项（ITEM-009/D-09）；`UploadPayload` 只做结构组装，不读 `SharedPreferences`（已静态核查）。
4. **旧解析族的重复换算**：`MqtttService.interpretPayloadV3`（无调用点）内仍有 2 处 `/10.0`，与存活路径无关（已核实调用点=0）；清理属后续维护项，已在脚本中固化该例外以便回归。
5. **本项不涉及**：待发箱 DAO/重试与上传编排（ITEM-009）、Service 接线（ITEM-010）、UI（ITEM-011/012）。

---

## ST-009 · ITEM-009（待发箱与上传编排 `UploadOutbox.kt` / `CloudUploadRepository.kt`）

**被测代码版本**：`0951920`（android_engineer.android_implementation 提交，相对上一提交 `e155553`）
**变更范围**：新增 `cloud/UploadOutbox.kt`、`cloud/CloudUploadRepository.kt`；`app/build.gradle.kts` +5（`testOptions.unitTests.isReturnDefaultValues`，仅影响宿主机单测的 android.jar 桩）；实现侧宿主机测试。
**任务期望**：入队按 `(dev_id, time)` 幂等；成功（HTTP 2xx）即删除；失败保留并按有界次数与退避重试（单批/退避上下限/最大次数集中于一处常量）；上传在独立作用域、不阻塞 BLE 与 MQTT 主链路；断网期间不丢、恢复后不重复；守护关闭或未注册手机号时不入队也不发请求。

### 1. 适用测试的选择与理由

| TD-SW-002 用例 | 是否适用 | 本轮结果 |
|---|---|---|
| T-SW-L0-12 ①–⑧（成功删除/失败退避/上限/分批/门控×2/幂等/串行） | 适用 | **宿主纯逻辑层已执行**（独立替身 11 项）；**SQLite 适配器经验层未执行**（同 ST-006/007 环境阻塞） |
| T-SW-L0-11（载荷与逐行时间/单位联通） | 适用 | 已执行（载荷逐行映射与负温） |
| T-SW-L0d-03-邻域（表缺失降级不崩溃） | 适用 | 静态核查 `runCatching`×5 + 真机测试类（已编译待跑） |
| T-SW-L1-01/L1-02（构建、宿主机单测） | 适用 | 已执行（144 项 0 失败） |
| T-SW-L2-09/10（真机断网累积→恢复补传与退避时序） | **本轮不适用** | 需真实设备/网络与 ITEM-010 接线；属 E2E-SW-002 J-1/J-3 |

### 2. 独立验证方法

1. 宿主机独立单测 `verification/CloudUploadItem009VerificationTest.kt`（11 项，**自带**内存待发箱/伪 Api/伪门控）：常量与退避曲线、门控三态、幂等入队、成功删除+载荷逐行映射、失败退避+到期重试、`maxAttempts` 保留不重试、单批上限分批、Api 异常不抛出、并发串行、**入队不阻塞调用方**（`@Test(timeout)` + 阻塞式 Api：若同步等待则测试超时）、`dueBatch` 过滤排序。
2. 独立脚本 `evidence/software_verify_cloud_upload_item009.py`（A–F）：参数集中与无散落魔数；SQL 适配器语义（`INSERT OR IGNORE`、到期过滤 `next_attempt_at<=? AND attempts<?`、排序 `next_attempt_at,time,device_id`、`LIMIT`、按 `(device_id,time)` 删除/更新、`runCatching` 降级）；门控顺序与 `isGateOpen` 语义；失败/并发控制（try-catch、`tryLock`+`finally unlock`、独立 `SupervisorJob+IO` 作用域）；生产接线尚未启用（`MqtttService` 引用=0）；生产客户端与门控键。
3. 真机 instrumented 独立验证 `androidTest/.../UploadOutboxItem009VerificationTest.kt`（5 项）：真实 SQLite 上的幂等插入、到期过滤/排序/LIMIT、重试与删除只影响目标行、策略+SQL 端到端幂等/ack、表缺失降级不崩溃。

### 3. 原始结果

**独立脚本（EXIT=0）**：

```
== A 参数集中 ==  PASS batch 50 / backoff 60→1800 / maxAttempts 20；业务逻辑无散落魔数
== B SQLite 适配器 ==  PASS INSERT OR IGNORE；WHERE next_attempt_at<=? AND attempts<?；ORDER BY next_attempt_at,time,device_id；LIMIT ?；按 (dev,time) 删除/更新；先查后插（事务）；runCatching×5+getOrElse 降级
== C 门控顺序 ==  PASS enqueueAndUpload：门控→入队→触发；uploadPendingOnce：门控→tryLock→取批；isGateOpen=守护中&&手机号非空
== D 失败与并发 ==  PASS api.upload 包 try/catch；Mutex.tryLock；finally unlock；CoroutineScope(SupervisorJob()+Dispatchers.IO)；scope.launch 触发；逐行 markFailed(backoffSeconds(attempts+1))；成功逐行 ack
== E 生产接线 ==  PASS MqtttService 引用数 = 0（本项不改变运行行为）
== F 客户端与门控键 ==  PASS CloudConfig.HTTP_BASE_URL；uploadData(...).isSuccessful；measure_state==stopping/registerphone/mac_addr
== RESULT ==  OK
```

**宿主机独立单测（11/11）**：

| 用例 | 断言要点 | 结果 |
|---|---|---|
| `configDefaults_andBackoffCurve_areBounded` | 50/60/1800/20；退避序列 `60,120,240,480,960,1800,1800,1800`；单调不减且 ≤1800 | PASS |
| `gateClosed_neverEnqueuesNorRequests` | 守护关闭 / 手机号空 / 全空白 → 不入队、0 行、不发请求、`gate_closed` | PASS |
| `enqueue_isIdempotentByDevIdAndTime` | 同 `(devId,time)` 第二次 false 且不覆写既有值 | PASS |
| `success_deletesRows_andPayloadCarriesRowValues` | 2 行→删除；一次请求；`phone/mac` 正确；`devId/time/temperature`（含负温）逐行映射不串位 | PASS |
| `failure_retainsRowWithBackoff_thenSucceedsWhenDue` | attempts=1、next=now+60、数据不丢；未到期不再请求；到期后重试成功并删除 | PASS |
| `maxAttempts_stopsAutoRetry_butKeepsRow` | 达 2 次后第三次 `empty`、行保留 attempts=2 | PASS |
| `batchLimit_splitsRequests_andProcessesAllRows` | 5 行 / 上限 2 → 请求大小 `[2,2,1]`、最终清空 | PASS |
| `apiException_isTreatedAsFailure_withoutThrowing` | IOException → `upload_failed`、attempts=1、无异常抛出 | PASS |
| `concurrentUploads_areSerialized` | 第一次挂起时第二次 `busy`，仅 1 次 HTTP | PASS |
| `enqueueAndUpload_returnsWithoutWaitingForHttp` | 阻塞式 Api 下调用立即返回（同步实现会超时失败），随后异步完成并清空 | PASS |
| `dueBatch_filtersByAttemptsAndDueTime_andOrders` | 到期/attempts 过滤与 `(next,time,dev)` 排序、LIMIT | PASS |

| 检查 | 命令 | 结果 |
|---|---|---|
| 宿主机单测（15 套件） | `./gradlew :app:testDebugUnitTest --offline` | BUILD SUCCESSFUL；**144 tests / 0 failures / 0 errors / 0 skipped** |
| 构建 | `./gradlew :app:assembleDebug --offline` | BUILD SUCCESSFUL；`app-debug.apk` 10,359,866 B（与实现侧一致） |
| 真机测试可编译 | `./gradlew :app:compileDebugAndroidTestKotlin --offline` | BUILD SUCCESSFUL（新增 ITEM-009 验证类） |
| 前项独立脚本 | ITEM-002/004/006/007/008 | 均 EXIT=0 |

### 4. Android SQLite 经验层：未执行 —— 环境阻塞（同 ST-006/007）

`./gradlew :app:connectedDebugAndroidTest --offline` 仍失败：`Could not download kotlinx-coroutines-core-jvm-1.6.4.jar … No cached version available for offline mode`；无外网且工具边界不含直接 `am instrument`，故不绕过。**SQL 语句语义已由独立脚本静态核实（§3-B），行级行为待后续环境补跑。**

### 5. 判定

1. **幂等入队**：`(dev_id, time)` 同键只一行且不覆写（策略层执行 + SQL `INSERT OR IGNORE`/先查后插静态核实 + 真机测试类待跑）——**通过**
2. **成功删除 / 失败保留+有界退避**：2xx 逐行 ack；失败逐行 `attempts+1` 且 `next = now + backoff(attempts+1)`；未到期不再请求；达 `maxAttempts` 保留但不重试——**通过**
3. **参数集中**：50 / 60→1800 / 20 仅在 `Config` 常量声明处，业务逻辑无散落魔数——**通过**
4. **门控**：守护关闭或无手机号时 `enqueueAndUpload` 不入队、`uploadPendingOnce` 不发请求（`gate_closed`）——**通过**
5. **不阻塞与串行**：上传在独立作用域 `scope.launch`（阻塞式 Api 下调用方仍立即返回）；`Mutex.tryLock` 保证同一时刻单请求（第二次 `busy`）——**通过**
6. **异常不崩溃**：Api 异常按失败记账不抛出；SQLite 适配器全操作 `runCatching` 降级——**通过**（适配器真机层待跑）
7. **未改变现有运行行为**：`MqtttService` 未引用新模块（触发点接线属后续队列项）——**通过**

结论：**TEST_PASS**（范围：队列/编排/门控/退避/并发/异常处理的宿主执行与静态核查 + 构建回归；**不含**真机 SQLite 行级与真机断网补传时序，已登记）。

### 6. 交接与观察

1. **触发点接线属后续队列项**：`enqueueAndUpload`/`triggerUpload` 的四个触发点（入队后、每轮 BLE、网络恢复、冷启动）未接入，本项不改变运行行为；TD-SW-002 §8-G3 已登记。真机断网累积→恢复补传属 E2E-SW-002 J-1/J-3。
2. **`measure_state` 语义依赖 as-built 约定**：`"stopping"` = 守护运行中、`"starting"` = 已停止（`HomeFragment.restoreGuardState`）。若后续改用 `homeViewModel.startData.value`，需同步核对（语义等价由接线项确认）。
3. **整批记账**：一次请求失败整批 `attempts+1`；因服务端 `UNIQUE(dev_id,time)` + `INSERT IGNORE`，重试不产生重复；部分成功粒度属新需求。
4. **达 `maxAttempts` 的数据保留但无自动清理/手动入口**；如需保留期策略属新需求。
5. **`app/build.gradle.kts` 的 `testOptions` 改动仅影响宿主机单测的 android.jar 桩**，不改变生产行为（已核查差异范围）。
6. **真实 HTTP 未执行**：本次未授权 `api_client` 且无外网，`inserted` 幂等核对归 E2E。

---

## ST-010 · ITEM-010（MqtttService 编排接入）—— 结论 **TEST_FAIL**

**被测代码版本**：`aba90a9`（android_engineer.android_implementation 提交，相对上一提交 `26bc2ff`）
**变更范围**：`MqtttService.kt`（181 行改动：周期对齐、AlarmEvaluator 接入、库基准、入库后上传触发、四个触发点）；新增 `telemetry/SensorCadence.kt` + 其宿主机测试；`TemperatureDatabaseHelper.kt` 新增 `getSamplesInRange`（关闭 G4）。

### 1. 适用测试与结果汇总

| 验收点 | 方法 | 结果 |
|---|---|---|
| 读取周期与 3 分钟采样对齐、手动立即读取保留 | 独立脚本 A1–A5 | **PASS** |
| 单次读数编排顺序（真实时间→参数→库基准→AlarmEvaluator→级别映射→紧急直接通知→卡片告警→事件落库→真实时间戳入库→入队+上传触发→卡片读数→900 s 去抖） | 独立脚本 B（逐项位置排序） | **PASS**（12/12 顺序正确） |
| 900 s 去抖、去抖仅守护中、>65℃ 每次通知、三条文案字面值、`ALARM_TYPE_*`、`AlarmEmergent`/`_bkp_all_time`、旧内存窗口清零、Service 侧不再做阈值比较 | 独立脚本 C1–C11 | **PASS**（11/11） |
| 四个上传触发点（冷启动/每轮 BLE/网络恢复/入库后）与仅守护中入队 | 独立脚本 D1–D5 + 调用计数 | **PASS** |
| 告警基准来自本地库（D-05/G4）：范围、降级为空表、SQL 语义 | 独立脚本 E + **宿主 SQLite 执行** `getSamplesInRange` 的 SQL | **PASS** |
| **任何一步失败都不影响其他步骤**（验收文字） | 独立脚本 F（结构分析） | **FAIL → 缺陷 D-010-1** |
| 宿主机回归与构建 | `testDebugUnitTest` / `assembleDebug` | 146 tests / 0 failures；APK 10,360,647 B（与实现侧一致） |
| 真机 BLE/MQTT 链路、通知/卡片、待发箱补传与 HTTP | — | **未执行**（环境阻塞 + 为避免生产副作用未安装启动，见 §3） |

### 2. 缺陷 D-010-1（阻断项）

**现象**：`processTemperatureHumidityData` 全函数体只有**一层** try/catch；紧急通知块（`MqtttService.kt:872-874`：`VibrationPlayer.vibratePhone` / `RingtonePlayer.startAlarm`）与报警事件落库（`:883` `storeAlarmEvent`）位于**单条入库（`:901`）与上传触发（`:911`）之前**，且两者之间没有任何独立保护。因此这些步骤抛异常时，同一读数的 **入库、入队与上传触发、卡片更新均被跳过**。

**可抛点（已逐文件核实）**：
- `RingtonePlayer.kt`：**全文件 0 个 `catch`**；`startAlarm` 中 `MediaPlayer.create(context, alarmUri).apply { … start() }`——`getDefaultUri(TYPE_ALARM)`/`TYPE_NOTIFICATION` 均可能为空且 `create` 失败时返回 null（平台类型），`apply` 于 null 上抛 NPE；`start()` 也可能抛 `IllegalStateException`。
- `VibrationPlayer.kt`：**全文件 0 个 `catch`**；`vibratePhone` 做服务转换后直接 `vibrate()`。
- `TemperatureDatabaseHelper.storeAlarmEvent` 与 `storeTemperatureReading`：均仅 `try/finally`、**无 catch**（`catch=0`），数据库异常直接向上抛。

**影响**：最严重的是 **>65℃ 紧急报警路径**（安全相关）：若铃声/震动环节抛出异常，该条超温读数将被“静默丢失”（不入库、不上传、卡片不更新、无报警事件），而外层 catch 只记日志；后台循环与其它读数不受影响（这一点符合设计）。

**依据**：任务验收文字“一次 BLE 或 MQTT 读数会依次完成归因、告警判定、去重落库与上传触发，**任何一步失败都不影响其他步骤**与后台循环”。当前实现只做到“读数级隔离 + 循环存活”，未做到“步骤间隔离”。（对比：已批准设计 AA-002 §8 只明写“上传失败不阻塞 BLE/MQTT 主链路”，已由 repository 内部吞错满足；本项 FAIL 针对的是任务验收文字中更强的步骤隔离要求。）

**修复要求（属实现能力范围，非本能力修改）**：
1. 把“本地入库 + 入队上传触发”与告警副作用（震动/铃声/通知/事件落库）**各自保护**，推荐顺序：先单条入库 → 入队上传 → 再报警事件落库/通知/震动/铃声（或为每步加 try/catch）；
2. `RingtonePlayer.startAlarm` 与 `VibrationPlayer.vibratePhone` 内部需对 null/异常降级（不得向外抛）；
3. **不得改变**：三条报警文案字面值、`ALARM_TYPE_*`、`>65℃` 语义、900 s 去抖、守护门控、真实采样时间戳与上传触发语义、周期 180 s。

### 3. 未执行层（不作为 FAIL 依据，但必须登记）

1. `connectedDebugAndroidTest` 仍因 `kotlinx-coroutines-core-jvm:1.6.4` 缺失且无外网而失败（同 ST-006/007/009）。
2. 本项的真机链路（BLE 读取、MQTT 消息、通知/卡片、待发箱补传、`/upload_data`）需要设备 + 真实链路；且启动当前 debug 包会连到**生产 broker/服务器并可能写入真实数据**（本机测试账号未与生产隔离），故本能力**未执行 install/launch**，以免产生生产副作用。该层属 E2E-SW-002 J-1/J-2/J-3；若需在设备上完成，请提供与生产隔离的测试环境/账号。

### 4. 结论

除缺陷 D-010-1 外，本项的周期对齐、编排顺序、告警语义与兼容面、四个上传触发点、库基准查询（G4）均已核实通过，且无构建/既有用例回归。因验收文字中的**步骤隔离**未满足（且涉及 >65℃ 紧急路径的数据保留），结论为 **TEST_FAIL**。修复后应重跑：`evidence/software_verify_orchestration_item010.py`（期望 EXIT=0）、`./gradlew :app:testDebugUnitTest --offline` 与 `assembleDebug`。

### 5. 本能力验证资产的同步更新（透明记录）

ITEM-010 使两个早先的回归门禁脚本出现“预期变化导致的失败”，本能力已按新事实**收紧/校正**其断言（仍为可复跑门禁）：

| 脚本 | 原断言（ITEM-007/009 当时） | 现断言（ITEM-010 后） | 理由 |
|---|---|---|---|
| `software_verify_timestamp_write_item007.py` | `60L` 仅允许出现在 `AlarmEvaluator.kt` | `60L` 仅允许出现在三处非入库语义（AlarmEvaluator 窗口 / CloudUploadRepository 退避常量 / MqtttService 基准窗口算术），并**新增**“数据库辅助类无 `dataSize`/`(size-1-i)`/`* 60` 固定间隔回填”断言 | ITEM-009 的退避常量与 ITEM-010 的基准窗口算术合法使用 `60L`；入库回填不变量改为直接断言 |
| `software_verify_cloud_upload_item009.py` | `MqtttService` 引用 = 0（未接线） | 断言接线**形状**：仅经 `uploadRepository.enqueueAndUpload/triggerUpload`，导入四个生产实现类，**不得**直接 `Retrofit.Builder`/`CloudApiClient`，入队受守护门控 | ITEM-010 按设计完成接线；门禁改判为“接线必须经 repository 且门控” |

校正后各门禁均回到 EXIT=0；与 ITEM-010 无关的断言未被放宽。

---

## ST-010R · ITEM-010 复验（缺陷 D-010-1 修复后）—— 结论 **TEST_PASS**

**复验对象**：`489ed62`（android_engineer.android_implementation 修复提交，相对上一提交 `21f4b53`）
**上轮结论**：`aba90a9` → `TEST_FAIL`（缺陷 D-010-1：步骤隔离不足）
**修复差异范围**（`git diff --name-only 21f4b53 489ed62`）：仅 `MqtttService.kt`、`RingtonePlayer.kt`、`VibrationPlayer.kt` 三个产品文件 + 文档/清单。

### 1. 复验方法（独立，较上轮加强）

在上轮门禁基础上**新增 F2**（逐步保护与辅助类不外抛）并新增宿主机可执行用例：

1. `evidence/software_verify_orchestration_item010.py` 的 F/F2：逐步骤断言“该步骤之后出现 `catch`（步骤闭合）”；核对 `RingtonePlayer`/`VibrationPlayer` 的 try/catch、`as?` 判空、`MediaPlayer.create` 判空、stop 系列的 try/catch+finally 置空、两文件无显式 `throw`。
2. **新增宿主机可执行验证** `verification/PlayerDegradationItem010VerificationTest.kt`（2 项）：`RingtonePlayer.stopAlarm()` 可重复调用不抛；`VibrationPlayer.vibratePhone(...)` 在系统服务不可用（mockable android.jar 下返回默认 null）时静默降级，`stopVibration` 不抛。
3. 全量回归：7 个独立门禁 + 宿主机单测 + 构建。

### 2. 原始结果

```
$ python evidence/software_verify_orchestration_item010.py        # EXIT=0
  A1-A5 PASS（周期 180s 派生、READ_INTERVAL_MS 引用、手动立即读取保留）
  B  12/12 PASS（编排顺序未因修复而改变）
  C  C1-C11 PASS（三条文案字面值/ALARM_TYPE_*/900s 去抖仅在守护中/>65℃ 只在 AlarmEvaluator/旧窗口清零）
  D  D1-D5 PASS（四个触发点 + triggerUpload×3 + 入队受守护门控）
  E  PASS（库基准范围与失败降级；宿主 SQLite 执行 getSamplesInRange 过滤+升序正确）
  F  PASS  函数内 try 块数 = 9；通知块↔入库独立保护=True；报警事件落库↔入库独立保护=True
  F2 PASS  8/8 步骤闭合 + 8/8 辅助类断言（startAlarm/ create 判空 / URI 空降级 / stopAlarm / vibratePhone / as? 判空 / stopVibration / 无 throw）
  RESULT: OK
```

| 检查 | 结果 |
|---|---|
| ITEM-010 门禁（上轮 EXIT=1） | **EXIT=0** |
| 其余 6 个独立门禁（ITEM-002/004/006/007/008/009） | 全部 **EXIT=0**（无回归） |
| 宿主机单测（含本轮新增 2 项） | **148 tests / 0 failures / 0 errors / 0 skipped** |
| 构建 | BUILD SUCCESSFUL；`app-debug.apk` 10,361,755 B（与修复记录一致） |
| 辅助类可执行降级 | `PlayerDegradationItem010VerificationTest` 2/2 PASS |

### 3. 判定

1. **步骤隔离（原缺陷）**：函数由 1 个 try 变为 **9 个 try**，8 个副作用步骤（清 sparkline / 紧急通知块 / 卡片告警 / 报警事件落库 / 单条入库 / 入队上传 / 卡片读数 / 900 s 去抖副作用）**各自被 catch 闭合**；紧急通知块与报警事件落库不再可能跳过入库、入队上传与卡片更新。**通过**
2. **辅助类不外抛**：`RingtonePlayer.startAlarm` 现对 URI 为空与 `MediaPlayer.create` 返回 null 显式降级，并对 `start()` 异常兜底；`VibrationPlayer.vibratePhone` 用 `as?` 判空并对 `vibrate()` 兜底；stop 系列均 try/catch+finally 释放置空；两文件无显式 `throw`；宿主机可执行用例证明服务不可用时静默降级。**通过**
3. **兼容面未因修复改变**：编排顺序 12/12、三条文案字面值、`ALARM_TYPE_*`、`>65℃` 语义（仍只在 `AlarmEvaluator`）、900 s 去抖且仅守护中、守护门控、真实采样时间戳、四个上传触发点、周期 180 s 全部与上轮一致。**通过**
4. **无回归**：7 个门禁 + 148 项宿主机用例 + 构建全部通过；修复差异仅 3 个产品文件。**通过**

结论：**TEST_PASS**（缺陷 D-010-1 已修复并经独立复验；设备端经验层仍未执行，见 §4）。

### 4. 仍存在的限制与观察（不构成失败）

1. **设备端经验层仍未执行**：`connectedDebugAndroidTest` 受 UTP 依赖缺失 + 无外网阻塞；且启动 debug 包会连生产 broker/服务器并可能写入真实数据（无隔离测试环境），故未执行 install/launch。真机 BLE/MQTT 链路、通知/卡片、待发箱补传与 `/upload_data` 属 E2E-SW-002 J-1/J-2/J-3；若需设备验证，请提供隔离的测试账号/环境。
2. **入库与上传现为“各自保护”而非“入库成功才入队”**：由于选择“保持顺序 + 逐步 try/catch”，当**本地入库**抛出时仍会执行入队上传（两者均以 `(dev_id, time)` 幂等，待发箱是持久队列，服务端 `UNIQUE` 兜底），正常路径顺序仍为“入库 → 入队上传”。若产品要求严格的“入库失败则不上传”，属语义细化，需上游确认（当前实现与验收文字“任一步失败不影响其他步骤”一致）。
3. `stopAlarm`/`stopVibration` 现在无论是否在播放都会释放资源并置空引用（原仅在播放中释放）；属健壮性改进，用户可见行为不变。

---

## ST-011 · ITEM-011（卡片状态展示：去除 30 分钟“离线”）—— 结论 **TEST_PASS**

**被测代码版本**：`47176a4`（android_engineer.android_implementation 提交，相对上一提交 `6f12fd4`）
**变更范围**：`DeviceState.kt`（删除 `OFFLINE_THRESHOLD_MS`/`isOnline()`，新增 `isNeverReported()`/`lastReportAgeLabel()`）、`ui/home/DeviceCardAdapter.kt`（chip 去“离线”分支 + `reportHint`）、`res/layout/item_device_card.xml`（新增 `reportHint`，sparkline 约束改挂）、`HomeViewModel.kt`（仅注释）+ 实现侧测试。

### 1. 适用测试与结果

| 验收点 | 方法 | 结果 |
|---|---|---|
| 不再存在 30 分钟数据龄判定入口 | 独立脚本 A（**只看代码**，注释中的历史说明不计）+ JVM 反射 | **PASS**（`isOnline`/`OFFLINE_THRESHOLD` 命中 0；卡片/状态文件代码与布局中“离线”命中 0） |
| chip 仍区分 未收到/正常/警告/紧急 且配色语义不变 | 独立脚本 B（与基线 `6f12fd4` 逐三元组比对） | **PASS**（4 组文案+配色+字色逐字一致；相对基线仅移除“离线”分支） |
| 长时间无数据不再显示离线，改为相对时间 + 条件上报提示 | 独立脚本 C + JVM 分桶边界 | **PASS**（`最近上报 <label> · 按条件上报，可能长时间无上报`；40 分钟 → `40 分钟前` 且不含“离线”） |
| 未收到仍为“未收到”且温度/湿度为 `--` | 独立脚本 C4 + JVM `isNeverReported` | **PASS** |
| 相对时间分桶精确 | JVM 边界：0/59/60 s、59/60 min、23h59m/24h、3d | **PASS**（恰好 60 s、60 min、24 h 均正确跳出上一档） |
| 布局接线 | 独立脚本 D（reportHint 位于 tempValue 下；sparkline 顶部约束指向 reportHint） | **PASS** |
| 无关配色资源未被改动 | 独立脚本 E（`colors.xml` 与基线逐字比对） | **PASS** |
| 连通性不由数据龄推断 | 独立脚本 F | **PASS**（卡片/状态类未引入链路状态推断） |
| 兼容面：DeviceState 其余字段未变 | JVM 反射（8 字段齐全） | **PASS** |
| 宿主机回归与构建 | `testDebugUnitTest` / `assembleDebug` | **160 tests / 0 failures**；APK 10,361,689 B（与实现侧一致） |
| 真机界面截图/配色（TD-SW-002 T-SW-L3-01/02/03） | — | **未执行**（见 §2） |

### 2. 独立验证方法摘要

1. 宿主机独立单测 `verification/DeviceStateItem011VerificationTest.kt`（5 项，期望值自写）：`isNeverReported` 语义与空标签；分桶边界表（含“恰好等于”负向断言）；40 分钟陈旧设备只得相对时间；反射断言 `isOnline` 方法与 `OFFLINE_*` 字段不存在且其余 8 字段齐全；级别与 `displayName` 原样传递。
2. 独立脚本 `evidence/software_verify_card_status_item011.py`（A–F）：代码层旧判定清零（注释不计，且校验非卡片文件中既有的“离线”文案未新增）；chip 三元组与基线逐项比对 + 仅允许移除“离线”分支；两种提示文案与 `--` 占位；布局约束；`colors.xml` 未变；无连通性推断。
3. 全量回归：8 个独立门禁（ITEM-002/004/006/007/008/009/010/011）+ 160 项宿主机用例 + 构建。

### 3. 判定

长时间无新数据的设备不再被判定/显示为“离线”，只展示“最近上报 N 分钟/小时/天前”与“按条件上报，可能长时间无上报”提示；从未收到数据的设备仍为“未收到”且数值为 `--`；紧急/警告/正常 三档文案与配色（含字色）与改动前逐字一致；30 分钟数据龄判定入口已彻底移除且未在其它文件新增“离线”文案。结论：**TEST_PASS**。

### 4. 未执行层与交接

1. **真机界面未验证**：卡片截图与配色观感（T-SW-L3-01/02/03）需设备；`connectedDebugAndroidTest` 受 UTP 依赖缺失 + 无外网阻塞，且启动 debug 包会连**生产** broker/服务器（无隔离测试环境），故未执行 install/launch/screenshot。该层属 E2E-SW-002 与后续 L3 设备用例。
2. **`ReportGenerator` 的既有“设备可能离线过”建议文案**（`ReportGenerator.kt:109`）在本项**未被改动**（已核实为基线既有，非本项新增）。该文案按“预期约 288 条/天”的固定节奏推断，与条件上报语义已不符——属**后续队列项（早报文案）**范围，已交接，本能力不越权修改。
3. `HomeViewModel` 仅改注释（sparkline 容量 `MAX_SPARKLINE_SIZE = 30` 未变，仅修正 3 分钟节拍说明）。
4. 提示文案为硬编码中文（与卡片既有“未收到/正常”等风格一致）；本地化属资源化重构。
5. `bindStatusChip` 位于 Adapter（需 Android 视图）未纳入宿主机测试，其四分支由静态核查（含基线配色比对）与后续设备侧 L3 用例覆盖。

---

## ST-012 · ITEM-012（设备详情：规则呈现 / 24 h 图表范围 / 高温强调）—— 结论 **TEST_PASS**

**被测代码版本**：`5ccfc1d`（android_engineer.android_implementation 提交，相对上一提交 `f6d26dc`）
**变更范围**（`git diff --name-only`）：新增 `ui/detail/DetailPresentation.kt`（纯逻辑常量与判定）；`DeviceDetailFragment.kt`（规则文案绑定、最新读数与高温强调、35℃ 参考线、范围上限 24 h）；`res/layout/fragment_device_detail.xml`（新增两个只读 TextView）+ 实现侧测试。

### 1. 适用测试与结果

| 验收点 | 方法 | 结果 |
|---|---|---|
| 规则文案与 readme 一致（3 分钟 / 0.9℃ / 无光 / 35.0℃ / 才上报） | 独立脚本 A + 宿主机测试 | **PASS**（从源码抽取的文案与独立写出的 readme 规则**逐字相等**） |
| 不显示环境光/照度信息 | 独立脚本 A（去注释后扫描详情 UI 三个文件） | **PASS**（`lux`/`照度`/`光照`/`勒克斯` 命中 0） |
| 不给出 App 推断的上报原因 | 独立脚本 A（推断词表） | **PASS**（`因下降`/`因为温度`/`由于温度`/`上报原因`/`推测`/`触发上报的原因` 命中 0） |
| 图表范围上限 50 min → 24 h | 独立脚本 B（常量、派生、调用点、全 main 无“50 分钟”残留、基线对照） | **PASS**（`MAX_CHART_RANGE_HOURS=24`；`MAX_CHART_RANGE_MS = 24 * HOUR_MS = 86,400,000`；提示文案取自常量且不含“50/分钟”；基线确实为 `49*60*1000`+“50 分钟”） |
| 高温强调严格 `> 35.0℃`（=35.0 不强调） | 独立脚本 C + 宿主机测试（35.0/34.9/0/-10 不强调；35.1/36/100/35.0+ε 强调） | **PASS**（`isHighTemperature` 用 `>`；无 `>= 35` 写法） |
| 只读呈现 | 独立脚本 E（两 TextView 存在、未绑点击、由常量赋值） | **PASS** |
| 呈现层不参与判定 | 独立脚本 F（`DetailPresentation` 仅被详情页引用；主链路未引用其阈值） | **PASS** |
| 宿主机回归与构建 | `testDebugUnitTest` / `assembleDebug` | **173 tests / 0 failures**；APK 10,391,985 B（与实现侧一致） |
| 真机截图（T-SW-L3-04/05/06） | — | **未执行**（见 §3） |

### 2. 独立验证方法摘要

1. 宿主机独立单测 `verification/DetailPresentationItem012VerificationTest.kt`（5 项）：规则文案逐字相等 + 必需词元 + 禁词（lux/照度/光照/勒克斯、推断词）+ “无光”仅作上报条件出现一次；24 h 常量与消息（无“50/分钟”）；高温阈值边界；后缀与阈值一致。
2. 独立脚本 `evidence/software_verify_detail_presentation_item012.py`（A–F，去注释后扫描）：规则文案与禁词；范围常量/调用点/全 main 残留与基线对照；严格 `>` 与常量来源；详情页未内联门控常量；布局与只读性；呈现层不被主链路引用。
3. 全量回归：9 个独立门禁（ITEM-002/004/006/007/008/009/010/011/012）+ 173 项宿主机用例 + 构建。

### 3. 判定与口径说明

1. **规则呈现与 readme 一致、且无光照/推断信息**：`REPORT_RULE_TEXT` 与 readme 逐字一致；详情 UI 不含 `lux/照度/光照/勒克斯`，不含推断结论词；`DetailPresentation` 仅被详情页引用，主链路（上传/告警）未使用其呈现阈值（不存在第二套门控）——**通过**
2. **图表范围 24 h**：上限常量集中且由小时派生，调用点与提示文案同源，全 `app/src/main` 无“50 分钟”残留——**通过**
3. **高温强调严格大于 35.0℃**：`=35.0` 与更低值不强调，`>35.0` 强调（颜色 + 文本后缀 + 图表参考线，均仅呈现）——**通过**
4. **只读、不重算门控**：规则文本未绑点击，详情页未内联 `0.9`/`35.0` 门控常量——**通过**

**口径说明（重要，任务文字内部张力）**：任务同时要求“规则与 readme 一致（含‘且无光’）”与“不显示任何光照、无光或 lux 信息”。二者无法字面同时成立。本能力按**首要要求（规则忠于 readme）+ 设计 AA-002 §5.3 的本意（不显示环境光状态/照度数值、不显示推断结论）**判定并核实：
- 允许：“无光”作为 readme 上报条件出现在规则文案中——经核实仅出现 2 处，且都在含“0.9℃”的规则文案内（Kotlin 常量 + 布局 `tools:text` 预览）；
- 禁止项全部为 0：无 `lux/照度/光照/勒克斯`，无环境光状态或读数展示，无“因下降 x℃ 上报”类推断结论。
若上游要求连规则原文的“无光”也不得出现，属产品表述变更（改一处文案常量，需上游确认），本能力不自行改判。

结论：**TEST_PASS**。

### 4. 未执行层与交接

1. **真机界面未验证**：规则文案/高温强调/24 h 查询属 TD-SW-002 T-SW-L3-04/05/06（需设备截图与配色观感）；`connectedDebugAndroidTest` 受 UTP 依赖缺失 + 无外网阻塞，且启动 debug 包会连**生产** broker/服务器（无隔离测试环境），故未执行 install/launch/screenshot。该层属 E2E-SW-002 与后续 L3 设备用例。
2. **高温强调形式**：颜色（`brand_error`）+ 文本后缀“ · 高温”+ 温度轴 35℃ 虚线参考线；`= 35.0℃` 不强调。若产品希望图表逐点着色，属新需求（MPAndroidChart 需自定义渲染）。
3. **与 ITEM-007 门禁的相互作用**：新增范围常量采用 `MAX_CHART_RANGE_HOURS * HOUR_MS`（`HOUR_MS = 3_600_000L`），未引入 `60L` 字面量；ITEM-007 门禁复跑仍 EXIT=0（未放宽任何断言）。
4. **本项不涉及**：早报文案（ITEM-013）、版本与文档（ITEM-014）。

---

## ST-013 · ITEM-013（早起报告文案与零样本表述）—— 结论 **TEST_PASS**

**被测代码版本**：`ccc59d9`（android_engineer.android_implementation 提交，相对上一提交 `7e168bd`）
**变更范围**（`git diff --name-only`）：`report/ReportGenerator.kt`（3 处文案）、`res/layout/activity_report.xml`（仅 `tools:text` 预览）、实现侧测试。**`DailyReportWorker.kt` 未改动**。

### 1. 适用测试与结果

| 验收点 | 方法 | 结果 |
|---|---|---|
| 零样本设备显示「昨日无上报」而非「无数据」 | 独立脚本 A + 宿主机测试 | **PASS**（文本含“昨日无上报”，无“无数据”；零样本设备无统计行） |
| 说明“按条件上报、条数非等间隔”，不暗示固定 3 分钟一条 | 独立脚本 A + 宿主机测试 | **PASS**（含“按条件上报/条数非等间隔”；`每 3 分钟`/`288`/`共采集`/`预期`/`离线` 在用户可见文本中命中 **0**） |
| 同一批数据统计值与改动前一致 | 宿主机测试（**从 Git 历史逐字重建的改动前生成器**作参照，同一夹具对比） | **PASS**（数值差异**仅**为被有意删除的 `288`；数值序列与 `采样/温度/湿度/报警` 行**逐字相同**；行级差异恰为 3 处措辞） |
| 统计字段与报警次数结构不变 | 宿主机测试（行结构/边界）+ 独立脚本 B/D | **PASS**（标题、4 类统计行格式、`报警 N 次`、建议阈值 `>0 / ≥3 / ≥5 / <24` 与基线一致；`DailyStats` 字段与 `getDailyStats` SQL 逐字未变） |
| 低条数建议不再作离线/异常推断 | 独立脚本 A + 宿主机测试 | **PASS**（文案为“按条件上报且非等间隔，条数偏少不一定异常”，无“288/预期/离线”） |
| `DailyReportWorker` 与缓存读取结构未变 | 独立脚本 C（与基线逐字比对）+ 差异范围 | **PASS**（逐字一致；仍调用 `ReportGenerator.build(...).toText()`） |
| 布局仅预览文案变化 | 独立脚本 F（剔除 `tools:text` 行后与基线比对） | **PASS** |
| 宿主机回归与构建 | `testDebugUnitTest` / `assembleDebug` | **188 tests / 0 failures**；APK 10,391,985 B（与实现侧一致） |
| 真机报告页/通知展示（T-SW-L3-07） | — | **未执行**（见 §3） |

### 2. 独立验证方法摘要

1. 宿主机独立单测 `verification/ReportGeneratorItem013VerificationTest.kt`（7 项）：**基线参照对比**（数据夹具含 0 条/7 条/30 条设备、负温、紧急 1 次、下降 3 次、超限 5 次；断言数值差异仅 `288`、统计行逐字相同、改写行恰 3 处）；零样本表述与禁止位；确定性（两次生成逐字相同）；建议阈值边界（下降 3/2、超限 5/4、紧急 ≥1、条数 23/24 与 24 不提示）；空设备列表与 `报警 N 次` 行结构；`DeviceReportSection` 字段结构。
2. 独立脚本 `evidence/software_verify_report_wording_item013.py`（A–F，注释不计入用户可见文本）：新旧文案与禁止位；统计行格式/标题/建议阈值/报警类型常量与基线逐字比对；`DailyReportWorker.kt` 与基线**逐字一致**；`DailyStats` 字段与 `getDailyStats` SQL 与基线一致；基线确实含旧文案（证明已改）；布局仅 `tools:text` 变化。
3. 全量回归：10 个独立门禁（ITEM-002/004/006/007/008/009/010/011/012/013）+ 188 项宿主机用例 + 构建。

### 3. 判定

1. **零样本表述**：某设备当日无上报 → “昨日无上报”，且全文不再出现“无数据”——**通过**
2. **条件上报语义**：摘要与低条数建议明确“按条件上报、条数非等间隔”，不再出现固定节拍（“每 3 分钟”）或“预期约 288 条”的推断——**通过**
3. **统计值与结构不变**：以改动前生成器为参照，同一批数据的统计数值、`采样/温度/湿度/报警` 行、标题、报警次数结构与建议阈值全部一致（唯一数值差异是删除固定的 `288` 预期）——**通过**
4. **未越权改动**：`DailyReportWorker`/`DailyStats`/`getDailyStats`/`ReportActivity` 缓存逻辑与布局非预览内容均未改动——**通过**

结论：**TEST_PASS**。

### 4. 未执行层与交接

1. **真机报告页/通知展示未执行**（T-SW-L3-07）：`connectedDebugAndroidTest` 受 UTP 依赖缺失 + 无外网阻塞，且启动 debug 包会连**生产** broker/服务器（无隔离测试环境），故未执行 install/launch/screenshot；属 E2E-SW-002 与后续 L3 设备用例。
2. **低条数建议阈值仍为 `< 24`**（结构保持不变）：在条件上报下会较频繁触发；因文案已明确“偏少不一定异常”，不再构成误导。若产品希望改为“多日无上报”判定，属新需求。
3. **摘要用词由“共采集”改为“共上报”**：与 readme/IC-002 的条件上报语义一致，统计数值与报警次数不变。
4. **关闭 ITEM-011 交接项**：`ReportGenerator` 中原有的“预期约 288 条 / 设备可能离线过”误导文案已由本项删除（ST-011 §4-2 登记项已闭合）。

---

## ST-014 · ITEM-014（收尾：版本递增 / 变更记录与说明书 / 清单维护）—— 结论 **TEST_FAIL**（缺陷 D-014-1）

**被测代码版本**：`de8c4cb`（android_engineer.android_implementation 提交，相对上一提交 `2ff9128`）
**变更范围**（`git diff --name-only`）：`app/build.gradle.kts`（版本）、`readme.txt`（V1.8 条目）、`用户使用说明书.md`、`file_manifest.txt`。**本项无新增产品文件**。

### 1. 适用测试与结果

| 验收点 | 方法 | 结果 |
|---|---|---|
| `versionCode`/`versionName` 递增 | 独立脚本 A（源码 + 构建产物三级核实） | **PASS**：源码 `8/1.7 → 9/1.8`；AGP `output-metadata.json` = `9/"1.8"`；合并清单 `versionCode="9"/versionName="1.8"`；**APK 二进制清单字符串池含 UTF-16LE `1.8` 且不含 `1.7`** |
| Gradle 构建与宿主机单测通过 | `assembleDebug` / `testDebugUnitTest` | **PASS**：BUILD SUCCESSFUL；**188 tests / 0 failures / 0 errors / 0 skipped**；APK 10,313,613 B |
| 文档描述与实现一致 | 独立脚本 B（**代码事实 × 文档断言交叉核对**） | **PASS**：13 项代码事实（180 s 采样、0.9℃/无光/35.0℃、24 h、35.0 高温阈值、5/10/15 分钟窗、65.0 紧急、新服务器 URI、待发箱表、DB v3、上传门控、900 s 节流、早报文案、卡片相对时间）与 16 项文档断言（readme 7 项 + 说明书 9 项）全部成立 |
| 旧口径清零 | 独立脚本 C | **PASS**：无 `版本1.7`/`不上传到云端`/`288`/`每 3 分钟上报|上传|一条`；`50 分钟`仅出现在“由 50 分钟放宽到 24 小时”的**历史变更语境**；`每 3 分钟`仅接“采样”；`离线`仅在否定/说明语境（越界 0） |
| `file_manifest.txt` 条目已更新且覆盖完整 | 独立脚本 D | **PASS**：`build.gradle.kts`（versionCode/versionName）、`readme.txt`、`用户使用说明书.md` 条目均已更新；本工作流全部 **53** 个产品变更文件均在清单中（精确路径或粗粒度父目录条目） |
| **清单条目路径真实有效** | 独立脚本 D3（本项新增/修改的 `active` 条目必须存在于磁盘） | **FAIL → 缺陷 D-014-1** |
| 真机安装/升级路径 | — | **未执行**（见 §4） |

### 2. 缺陷 D-014-1（阻断项）

**现象**：本项在 `file_manifest.txt` 中新增了一行路径被 **git 八进制转义未还原** 的条目（第 109 行）：

```
dengbei_care//347/224/250/346/210/267/344/275/277/347/224/250/350/257/264/346/230/216/344/271/246.md | active | 由 android_engineer.android_implementation 新增或修改的项目相关文件
```

该路径**在磁盘上不存在**（`ls` 报 No such file or directory），实为 `dengbei_care/用户使用说明书.md` 经 `git diff`（`core.quotepath=true`）转义后的文本被直接粘贴。

**核查范围与唯一性**：对整份清单（190+ 行）逐条检查 `active` 条目路径是否存在——**全清单仅此 1 条无效**，且由本项引入；真实条目 `dengbei_care/用户使用说明书.md`（第 180 行）本身已正确更新为 1.8 说明。

**影响**：`file_manifest.txt` 是项目级索引（Must Rules 4/6：所有 Agent 共同维护、提交前必须同步），一条指向不存在文件的假记录会误导后续 Agent/工具解析（例如按清单核对产物、生成交接视图）；本项验收文字明确要求“同步维护仓库根目录 file_manifest.txt 的新增与修改记录”，记录必须指向真实文件。

**修复要求（属实现能力范围，本能力不修改）**：删除该行（1 行删除）；保留第 180 行真实条目；清单其余内容不动。修复后应重跑 `python evidence/software_verify_release_item014.py`（期望 **EXIT=0**）与 `./gradlew :app:testDebugUnitTest --offline`。

### 3. 判定

除 D-014-1 外，本项全部通过：版本在源码与**三级构建产物**（AGP 元数据 / 合并清单 / APK 二进制清单）中一致落地为 `9 / 1.8`；构建与 188 项宿主机用例通过；readme 变更记录与用户说明书的内容与实现**逐项交叉一致**（采样节拍、条件上报、卡片不再“离线”、时间窗下降报警、24 h 图表、高温强调、云同步与幂等门控、新服务器地址、早报文案）；旧口径（50 分钟/1.7/不上传云端/固定节拍）已清零。因 `file_manifest.txt` 引入了一条指向不存在文件的假记录，结论为 **TEST_FAIL**。

### 4. 未执行层与交接

1. **真机安装/升级路径未执行**：版本号已在构建产物中核实（含 APK 二进制清单），但“安装新版本覆盖旧版本、数据保留”需设备；`connectedDebugAndroidTest` 受 UTP 依赖缺失 + 无外网阻塞，且启动 debug 包会连**生产** broker/服务器（无隔离测试环境），故未 install/launch。属 E2E-SW-002 与后续设备用例。
2. **文档中的“无光”措辞**沿用 ITEM-012 的口径裁定（规则忠于 readme；“无光”仅为上报条件，App 不展示环境光状态/照度/推断结论）——已在 ST-012 §3 登记。
3. **队列完成状态**：本工作流的 ITEM-001…ITEM-014 已全部经过本能力的独立验证（其中 ITEM-010 首轮 TEST_FAIL 经修复复验通过）；本项为唯一待修复项（清单 1 行）。后续路由/完成判定由 Runtime 负责。

---

## ST-014R · ITEM-014 复验（缺陷 D-014-1 修复后）—— 结论 **TEST_PASS**

**复验对象**：`08f4592`（android_engineer.android_implementation 修复提交，相对上一提交 `fd1f9cc`）
**上轮结论**：`de8c4cb` → `TEST_FAIL`（缺陷 D-014-1：`file_manifest.txt` 新增了一行 git 八进制转义未还原的假路径条目）
**修复差异**（`git diff --name-only fd1f9cc 08f4592`）：`file_manifest.txt`（**−1 行**）、`artifacts/android_implementation.md`、`evidence/android_test.md`——**未改动任何产品源码**。

### 1. 复验原始结果

| 检查 | 结果 |
|---|---|
| 假路径条目已删除 | **PASS**：`grep -c "^dengbei_care//[0-9]"` = **0**（修复提交 diff 仅 `-dengbei_care//347/224/…246.md`） |
| 真实条目保留且已更新 | **PASS**：`dengbei_care/用户使用说明书.md | active` 存在且含 1.8 说明（未被误删） |
| **全清单有效性** | **PASS**：逐条扫描全部 `active` 条目 → 路径不存在者 **0**（修复前为 1） |
| ITEM-014 独立门禁 | **EXIT=0**：A 版本三级核实（源码 9/1.8 严格递增、AGP output-metadata、合并清单、APK 二进制清单含 1.8 不含 1.7）；B 13 项代码事实 × 16 项文档断言全部一致；C 旧口径清零（`50 分钟`仅历史变更语境、`每 3 分钟`仅接“采样”、`离线`仅否定语境、无 1.7/不上传云端/288）；D 条目已更新 + 全工作流 53 个产品文件覆盖；**D3 本项改动条目路径不存在者 = 0** |
| 其余 10 个独立门禁 | 全部 **EXIT=0**（ITEM-002/004/006/007/008/009/010/011/012/013） |
| 构建与宿主机单测 | BUILD SUCCESSFUL；**188 tests / 0 failures / 0 errors / 0 skipped**；APK 10,313,613 B（与修复前一致，符合“未改产品源码”） |

### 2. 判定

1. **缺陷 D-014-1 已修复**：假路径条目删除，真实条目完好，全清单不存在无效 `active` 路径——**通过**
2. **原验收点复验保持通过**：版本递增（源码 + AGP 元数据 + 合并清单 + APK 二进制清单三级一致 `9 / 1.8`）、构建与 188 项用例、文档与实现交叉一致、旧口径清零、清单覆盖完整（53 个产品文件）——**通过**
3. **修复范围最小且无副作用**：仅清单 1 行删除（+ 实现记录/证据文档），产品源码零改动，构建产物尺寸不变——**通过**

结论：**TEST_PASS**（缺陷 D-014-1 已修复并经独立复验）。

### 3. 仍存在的限制（不构成失败）

1. **真机安装/升级路径仍未执行**：版本号已在三级构建产物中核实（含 APK 二进制清单），但“覆盖安装、数据保留”需设备；`connectedDebugAndroidTest` 受 UTP 依赖缺失 + 无外网阻塞，且启动 debug 包会连**生产** broker/服务器（无隔离测试环境），故未 install/launch。属 E2E-SW-002 与后续设备用例。
2. 文档中“无光”措辞沿用 ST-012 的口径裁定（规则忠于 readme；App 不展示环境光状态/照度/推断结论）。
3. **队列状态**：ITEM-001…ITEM-014 均已由本能力独立验证（ITEM-010 与 ITEM-014 各经一次修复后复验通过）；后续路由与项目完成判定由 Runtime 负责。
