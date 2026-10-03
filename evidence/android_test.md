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
