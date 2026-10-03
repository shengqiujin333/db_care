# 软件验证证据（ST-002）

状态：软件测试执行证据（`software_tester.software_verification`），按 Runtime 逐个指派的队列项追加
对象：Android App `dengbei_care`（Kotlin / AGP 8.3.2 / Gradle 8.4 / JDK 21）
本轮项：队列 **ITEM-001**（BLE 聚合帧解码抽为纯逻辑模块 `protocol/GatewayFrameCodec.kt` 并接入 `MqtttService`）
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
