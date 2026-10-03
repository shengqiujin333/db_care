# Android 测试证据（AT-002）

状态：Android 实现侧自检证据（`android_engineer.android_implementation`）
对象：Android App `dengbei_care`（Kotlin / AGP 8.3.2 / Gradle 8.4 / JDK 21）
本轮项：队列 ITEM-001（BLE 聚合帧解码抽为纯逻辑模块 `protocol/GatewayFrameCodec.kt` 并接入 `MqtttService`）
对应：AA-002 §5.1/§10、TD-SW-002 §4.1 T-SW-L0-01..03、§4.3 T-SW-L1-01/02
说明：本文件只记录实现侧可在本机执行的确定性检查；真机 GUI / 真机 BLE / broker / 云端链路用例由独立测试能力执行。

---

## AT-001 · ITEM-001（GatewayFrameCodec）

### 1. 环境与代码版本

| 项 | 值 |
|---|---|
| 工作目录 | `D:\mypro\beiwo2\AIP\dengbei_care` |
| 命令 | `./gradlew :app:testDebugUnitTest --offline`、`./gradlew :app:assembleDebug --offline` |
| Gradle / AGP / JDK | 8.4（wrapper）/ 8.3.2 / openjdk 21.0.2 |
| 平台 | Windows 11 amd64 |
| 代码状态 | 相对上一提交：`MqtttService.kt` 修改；新增 `protocol/GatewayFrameCodec.kt`、`test/.../protocol/GatewayFrameCodecTest.kt` |

### 2. 宿主机单元测试

```
./gradlew :app:testDebugUnitTest --offline
=> BUILD SUCCESSFUL in 7s
```

JUnit XML：`dengbei_care/app/build/test-results/testDebugUnitTest/TEST-com.jinyuni.dengbei_care.protocol.GatewayFrameCodecTest.xml`

| 汇总 | 值 |
|---|---|
| 套件 | `com.jinyuni.dengbei_care.protocol.GatewayFrameCodecTest` 11 项；`com.jinyuni.dengbei_care.ExampleUnitTest` 1 项 |
| 合计 | 12 tests / 0 failures / 0 errors / 0 skipped |
| 套件耗时 | 0.087 s |

| 用例 | 覆盖（TD-SW-002 对应） | 结果 |
|---|---|---|
| `exactLength_isAccepted_withDevCountSamples` | L0-01 恰等长度 | PASS |
| `longerThanExpected_isAccepted_andExtraTailIgnored` | L0-01 更长带尾部填充 | PASS |
| `shorterThanExpected_isRejected_withoutSamples` | L0-01 截断必须拒绝、无读数 | PASS |
| `shorterThanHeaderBlock_isRejected` | L0-01 明文 < 16 B 拒绝（15 B、0 B） | PASS |
| `zeroDeviceCount_isRejected_withoutSamples` | L0-01 `devCount == 0` 必须拒绝 | PASS |
| `temperatureVectors_areSignedAndScaledBy10` | L0-02 +25.0/−10.0/0.0/125.0/−40.0 | PASS |
| `humidityVectors_areUnsignedAndScaledBy10` | L0-02 0/60.0/100.0 | PASS |
| `humidityAndTemperatureAreNotSwapped` | L0-02 不得互换 | PASS |
| `deviceId_formatsAndByteOrderArePreserved` | L0-02 devIdHex/devIdCompact 与原字节序 | PASS |
| `multiDevice_samplesCarryTheirOwnId` | L0-03 多设备归属随块内 ID，不按下标 | PASS |
| `matchesLegacyImplementation_onRandomVectors` | 与改动前 `parsePlainFrame` 逐位一致（500 组固定种子随机帧） | PASS |

逐位一致用例的构造：500 组向量，`devCount ∈ 1..5`、`humidity_x10 ∈ 0..1000`、`temperature_x10 ∈ −400..1250`、随机设备 ID 4 B、随机尾部填充 `0..15 B`；期望值由测试内**逐字复刻的改动前 `parsePlainFrame`/`u16be`/`s16be`** 计算，与 `GatewayFrameCodec.decodeAggregatedFrame` 结果的 `gatewayIdHex`、`devCount`、`devIdHex`、`humidityPct`、`temperatureC` 全量比较（严格相等，无容差）。

### 3. 构建

```
./gradlew :app:assembleDebug --offline
=> BUILD SUCCESSFUL in 11s
```

| 产物 | 大小 |
|---|---|
| `dengbei_care/app/build/outputs/apk/debug/app-debug.apk` | 10,285,626 B |

Kotlin 编译仅输出改动前既有的 warning（`ExperimentalCoroutinesApi` opt-in、未使用变量/参数、deprecated override），无新增 error/warning 与本项相关。

### 4. 判定

ITEM-001 的宿主机检查全部通过：帧长规则（含 `devCount == 0` 与截断拒绝且无读数）、字段偏移/字节序/符号/单位、多设备归属、与改动前实现的逐位一致、构建与资源合并均成立。

未覆盖（属后续能力）：真机 BLE GATT 读取与真实网关帧（TD-SW-002 §4.6 REG-02 / §6-5）、MQTT 链路、SQLite、UI 展示——这些既有本项未改动的部分，也有后续队列项与独立测试能力承接。

---

## AT-002 · ITEM-002（移除小端解析入口）

### 1. 环境与代码版本

| 项 | 值 |
|---|---|
| 工作目录 | `D:\mypro\beiwo2\AIP\dengbei_care` |
| 命令 | `grep -rn`（静态）、`./gradlew :app:testDebugUnitTest --offline`、`./gradlew :app:assembleDebug --offline` |
| Gradle / AGP / JDK | 8.4（wrapper）/ 8.3.2 / openjdk 21.0.2 |
| 代码状态 | 相对上一提交：仅 `MqtttService.kt` 修改（5 insertions / 36 deletions，删除 `parseHexData` 与其注释残留） |

### 2. 静态核查（小端入口清零）

```
$ grep -rn "parseHexData" dengbei_care/app/src
(无输出，退出码 1)
$ grep -rn "parseHexData" dengbei_care/app/src/main
(无输出，退出码 1)
```

| 项 | 删除前 | 删除后 |
|---|---|---|
| `MqtttService.kt` 定义 | `fun parseHexData(...)`（小端 `humidity|temperature`） | 已删除 |
| `MqtttService.kt` 注释残留 | `onCharacteristicRead` 内注释掉的旧替代解析路径（引用该入口） | 已删除，改为一行退役说明（不含该标识符） |
| `app/src` 命中数 | 2 | **0** |

补充：仓库全量 grep 仍会命中 `dengbei_care/iOS开发资料/`（旧快照副本与 README 的旧行号描述）——非编译的 iOS 参考材料，不在 Android 源码范围且 readme 点 6 明确本轮不改 iOS，故未改动（已在 `artifacts/android_implementation.md` ITEM-002 §4.1 登记交接）。

### 3. 宿主机单元测试（回归）

```
./gradlew :app:testDebugUnitTest --offline
=> BUILD SUCCESSFUL in 5s
```

| 套件 | 项数 | 失败 | 归属 |
|---|---|---|---|
| `com.jinyuni.dengbei_care.protocol.GatewayFrameCodecTest` | 11 | 0 | 实现侧自检（ITEM-001） |
| `com.jinyuni.dengbei_care.verification.GatewayFrameCodecItem001VerificationTest` | 7 | 0 | 软件测试能力独立验证（上一提交） |
| `com.jinyuni.dengbei_care.verification.GatewayFrameCodecItem001LegacyParityTest` | 3 | 0 | 软件测试能力独立验证（上一提交） |
| `com.jinyuni.dengbei_care.ExampleUnitTest` | 1 | 0 | 既有 |
| **合计** | **22** | **0（0 errors、0 skipped）** | — |

删除死代码后既有验证全部继续通过 ⇒ ITEM-001 的帧长规则、`devCount == 0` 拒绝、字段/符号/单位与逐位一致行为未受影响。

### 4. 构建

```
./gradlew :app:assembleDebug --offline
=> BUILD SUCCESSFUL in 1s
```

| 产物 | 大小 |
|---|---|
| `dengbei_care/app/build/outputs/apk/debug/app-debug.apk` | 10,285,626 B（与 ITEM-001 同尺寸） |

Kotlin 编译告警与改动前一致（5 条既有：`ExperimentalCoroutinesApi` opt-in、`_name` 未使用、`closed` 未使用、`negotiatedMtu` 冗余初值、deprecated override），无新增告警。

### 5. 判定

ITEM-002 的期望行为成立：Android 源码内不存在小端 `parseHexData` 入口及其注释残留；二进制聚合帧解析的唯一实现为 `protocol/GatewayFrameCodec`（BLE 路径经 `decryptAndParseEcbFrame` 到达；MQTT 路径解析文本载荷，无第二套二进制解析）；构建与全部 22 项宿主机用例通过。

未覆盖（属后续能力）：真机 BLE/网关联调、MQTT/broker/云上传链路、SQLite、UI 展示。

---

## AT-003 · ITEM-003（MQTT 批量载荷严格归因）

### 1. 环境与代码版本

| 项 | 值 |
|---|---|
| 工作目录 | `D:\mypro\beiwo2\AIP\dengbei_care` |
| 命令 | `./gradlew :app:testDebugUnitTest --offline`、`./gradlew :app:assembleDebug --offline`、`python evidence/software_static_check_no_little_endian_parser.py` |
| Gradle / AGP / JDK | 8.4（wrapper）/ 8.3.2 / openjdk 21.0.2 |
| 代码状态 | 相对上一提交：新增 `telemetry/ReadingAttribution.kt`、`test/.../telemetry/ReadingAttributionTest.kt`；`MqtttService.kt` 修改 21 insertions / 64 deletions |

### 2. 宿主机单元测试

```
./gradlew :app:testDebugUnitTest --offline
=> BUILD SUCCESSFUL in 2s
```

| 套件 | 项数 | 失败 | 跳过 | 归属 |
|---|---|---|---|---|
| `com.jinyuni.dengbei_care.telemetry.ReadingAttributionTest` | 16 | 0 | 0 | 本项实现侧自检（新增） |
| `com.jinyuni.dengbei_care.protocol.GatewayFrameCodecTest` | 11 | 0 | 0 | ITEM-001 实现侧自检 |
| `com.jinyuni.dengbei_care.verification.GatewayFrameCodecItem001VerificationTest` | 7 | 0 | 0 | 软件测试能力独立验证 |
| `com.jinyuni.dengbei_care.verification.GatewayFrameCodecItem001LegacyParityTest` | 3 | 0 | 0 | 软件测试能力独立验证 |
| `com.jinyuni.dengbei_care.ExampleUnitTest` | 1 | 0 | 0 | 既有 |
| **合计** | **38** | **0（0 errors、0 skipped）** | 0 | — |

新增 16 项用例（TD-SW-002 对应）：

| 用例 | 覆盖 | 结果 |
|---|---|---|
| `consistentBatch_isAttributedInSavedIdOrder` | L0-04 数目一致 + 顺序 | PASS |
| `negativeAndZeroValues_areAccepted` | L0-04 负温/0 值 | PASS |
| `devIdTravelsWithReading_notWithIndex` | L0-04 devId 随读数下沉 | PASS |
| `singleDevice_isAccepted` | L0-04 单设备 | PASS |
| `spacedSeparators_areTolerated` | 逗号后空格容忍（App 自发布格式） | PASS |
| `extraTemperature_rejectsWholeBatch` / `missingHumidity_rejectsWholeBatch` / `deviceCountDiffersFromSeries_rejectsWholeBatch` | L0-05 数目不一致→拒绝整批 | PASS |
| `emptyTemperatureSeries_rejectsWholeBatch` / `emptyHumiditySeries_rejectsWholeBatch` | L0-05 空序列→拒绝整批 | PASS |
| `nonNumericToken_rejectsWholeBatch`（abc / 空项 / 尾部逗号） | L0-05 非法数值→拒绝整批 | PASS |
| `nonFiniteValue_rejectsWholeBatch`（NaN / ±Infinity） | 非法数值口径 | PASS |
| `invalidOrMissingTime_rejectsWholeBatch`（abc / 空 / 0 / 负 / 仅两段） | L0-05 ⑥ 时间非法或缺失→拒绝整批 | PASS |
| `noBoundDevice_rejectsWholeBatch` | 无绑定设备→拒绝整批 | PASS |
| `extraColonSegments_areIgnored_likeBefore` | 与旧实现一致的多余段口径 | PASS |
| `rejectedResult_neverCarriesReadings` | 拒绝结果不携带部分读数 | PASS |

### 3. 构建与静态核查

```
./gradlew :app:assembleDebug --offline
=> BUILD SUCCESSFUL in 1s
```

| 产物 | 大小 |
|---|---|
| `dengbei_care/app/build/outputs/apk/debug/app-debug.apk` | 10,359,240 B（较 ITEM-002 的 10,285,626 B 增加 73,614 B，为新增归因模块与其接入） |

| 检查 | 命令 | 结果 |
|---|---|---|
| 旧截断路径清零 | `grep -n "minOf(savedIds\|using first\|doubles0\|doubles1\|tempDoubles0" MqtttService.kt` | 0 命中 |
| 编译告警 | `./gradlew :app:compileDebugKotlin --offline` | 仍为既有 5 条，无新增 |
| 前项静态口径回归 | `python evidence/software_static_check_no_little_endian_parser.py` | `RESULT: OK`（A1=0、A2=0、B1a–B1d/B2 均 PASS）——新模块未引入小端写法，也未新增 `decodeAggregatedFrame` 调用点 |
| 差异范围 | `git diff --stat` | 仅 `MqtttService.kt`（21/64），加 2 个新文件 |

### 4. 判定

ITEM-003 的期望行为成立：MQTT 批量载荷不再按位置截断；数目不一致/空序列/非法数值/时间非法时整批拒绝（不落库、不上传、不报警，仅 `Log.w`）；一致时按 `MacIdBook.all()` 顺序产出携带 `devId` 的读数并交给下游。构建成功、全量 38 项宿主机用例通过、前项静态口径未回归。

未覆盖（属后续能力/队列项）：真机 MQTT 注入与 broker 链路（TD-SW-002 T-SW-L2-03/04）、告警时间窗、存储时间戳、云上传、UI。
