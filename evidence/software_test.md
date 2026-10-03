# 软件验证证据（ST-002）

状态：软件测试执行证据（`software_tester.software_verification`），按 Runtime 逐个指派的队列项追加
对象：Android App `dengbei_care`（Kotlin / AGP 8.3.2 / Gradle 8.4 / JDK 21）
本轮项：队列 **ITEM-001**…**ITEM-007**（逐项分节记录）
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
