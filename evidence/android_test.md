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

---

## AT-004 · ITEM-004（服务器常量集中与明文放行）

### 1. 环境与代码版本

| 项 | 值 |
|---|---|
| 工作目录 | `D:\mypro\beiwo2\AIP\dengbei_care` |
| 命令 | `./gradlew :app:testDebugUnitTest --offline`、`./gradlew :app:assembleDebug --offline`、`grep -rn`、`md5sum` |
| Gradle / AGP / JDK | 8.4（wrapper）/ 8.3.2 / openjdk 21.0.2 |
| 代码状态 | 相对上一提交：新增 `cloud/CloudConfig.kt`、`test/.../cloud/CloudConfigTest.kt`；修改 `MqtttService.kt`（+4/−1）、`ZhuCeViewModel.kt`（+2/−1）、`res/xml/network_security_config.xml`（+8/−1） |

### 2. 宿主机单元测试

```
./gradlew :app:testDebugUnitTest --offline
=> BUILD SUCCESSFUL in 4s
```

| 套件 | 项数 | 失败 | 跳过 | 归属 |
|---|---|---|---|---|
| `com.jinyuni.dengbei_care.cloud.CloudConfigTest` | 3 | 0 | 0 | 本项实现侧自检（新增） |
| `com.jinyuni.dengbei_care.telemetry.ReadingAttributionTest` | 16 | 0 | 0 | ITEM-003 实现侧自检 |
| `com.jinyuni.dengbei_care.protocol.GatewayFrameCodecTest` | 11 | 0 | 0 | ITEM-001 实现侧自检 |
| `com.jinyuni.dengbei_care.verification.ReadingAttributionItem003VerificationTest` | 8 | 0 | 0 | 软件测试能力独立验证 |
| `com.jinyuni.dengbei_care.verification.GatewayFrameCodecItem001VerificationTest` | 7 | 0 | 0 | 软件测试能力独立验证 |
| `com.jinyuni.dengbei_care.verification.GatewayFrameCodecItem001LegacyParityTest` | 3 | 0 | 0 | 软件测试能力独立验证 |
| `com.jinyuni.dengbei_care.ExampleUnitTest` | 1 | 0 | 0 | 既有 |
| **合计** | **49** | **0（0 errors、0 skipped）** | 0 | — |

新增 3 项用例（TD-SW-002 对应）：

| 用例 | 覆盖 | 结果 |
|---|---|---|
| `brokerUri_andHttpBaseUrl_pointToNewServer` | L0-13 常量值（`ssl://8.140.23.253:8883`、`http://8.140.23.253:5000`、主机/端口） | PASS |
| `constants_areComposedFromSingleHostAndPorts` | 常量由单一主机+端口组合（避免多处硬编码） | PASS |
| `networkSecurityConfig_permitsNewHost_andKeepsExistingOnes_only` | L1-06 明文放行：含新主机、仍保留既有条目（域名集合仍为 3 个，无其它主机）、三个 `domain-config` 均显式 `cleartextTrafficPermitted="true"` | PASS |

### 3. 构建与静态核查

```
./gradlew :app:assembleDebug --offline
=> BUILD SUCCESSFUL in 1s
```

| 产物 | 大小 |
|---|---|
| `dengbei_care/app/build/outputs/apk/debug/app-debug.apk` | 10,359,236 B（资源 XML 已成功合并） |

| 检查 | 命令 | 结果 |
|---|---|---|
| 旧 IP 清零（Kotlin 源码，含测试） | `grep -rn "117.72.84.210" --include=*.kt app/src` | **0 命中**（修改前 2 处硬编码连接地址） |
| 旧 IP 分布（全 `app/src`） | `grep -rn "117.72.84.210" app/src` | 1 命中：`res/xml/network_security_config.xml:9`（设计“追加而非替换”保留项，非连接目标） |
| 新 IP 集中性 | `grep -rn "8.140.23.253" app/src/main --include=*.kt` | 仅 `cloud/CloudConfig.kt:18` |
| 明文放行主机集合 | 读取 XML | `8.140.23.253`、`117.72.84.210`、`192.168.4.1`（共 3，无其它） |
| TLS 信任锚未变 | `md5sum app/src/main/assets/jd-ca.crt` | `f1a8212e3c690d57894b8a3a5fd901ab`（与 `服务器迁移记录.md` §三 记录一致） |
| 认证/加密/参数未变 | `grep -n "TLSv1.2\|jd-ca.crt\|&^!A:z?\|AES/ECB/NoPadding\|APP_AES_KEY16\|requestMtu(240)\|keepAliveInterval = 20" MqtttService.kt` | 均仍存在且与基线一致（`setupSSL()` 未进入 diff） |
| 编译告警 | `./gradlew :app:compileDebugKotlin --offline` | 仍为既有 5 条，无新增 |
| 差异范围 | `git diff --stat` | 3 个修改文件（共 +14/−3）+ 2 个新增文件；未触碰 `assets/`、`AndroidManifest.xml`、`build.gradle.kts`、`ApiService.kt` |

### 4. 判定

ITEM-004 的期望行为成立：MQTT broker 与 HTTP 基址指向 `8.140.23.253`（8883/5000）且集中于 `cloud/CloudConfig.kt`；明文放行追加新主机且未扩大其它主机；Kotlin 源码中旧地址清零（仅 XML 按设计保留 1 处）；TLS 信任锚、TLS 版本、用户名/密码规则、AES 密钥与模式、MTU、keepAlive 均未变；资源合并与构建通过、全量 49 项宿主机用例通过。

未覆盖（属后续能力/队列项）：真机 MQTT TLS 连接与认证、`/upload_data` 可达性（TD-SW-002 T-SW-L2-01/L2-12，需真实网络）、`/upload_data` 端点与上传编排、SQLite、告警、UI。

---

## AT-005 · ITEM-005（报警判定纯逻辑 `telemetry/AlarmEvaluator.kt`）

### 1. 环境与代码版本

| 项 | 值 |
|---|---|
| 工作目录 | `D:\mypro\beiwo2\AIP\dengbei_care` |
| 命令 | `./gradlew :app:testDebugUnitTest --offline`、`./gradlew :app:assembleDebug --offline` |
| Gradle / AGP / JDK | 8.4（wrapper）/ 8.3.2 / openjdk 21.0.2 |
| 代码状态 | 相对上一提交：**仅新增** `telemetry/AlarmEvaluator.kt`（7,375 B）与 `telemetry/AlarmEvaluatorTest.kt`（13,753 B）；未修改任何现有产品文件（生产路径仍用旧内存窗口逻辑，接线属 ITEM-010） |

### 2. 宿主机单元测试

```
./gradlew :app:testDebugUnitTest --offline
=> BUILD SUCCESSFUL in 3s
```

| 套件 | 项数 | 失败 | 跳过 | 归属 |
|---|---|---|---|---|
| `com.jinyuni.dengbei_care.telemetry.AlarmEvaluatorTest` | 23 | 0 | 0 | 本项实现侧自检（新增） |
| `com.jinyuni.dengbei_care.telemetry.ReadingAttributionTest` | 16 | 0 | 0 | ITEM-003 实现侧自检 |
| `com.jinyuni.dengbei_care.protocol.GatewayFrameCodecTest` | 11 | 0 | 0 | ITEM-001 实现侧自检 |
| `com.jinyuni.dengbei_care.verification.ReadingAttributionItem003VerificationTest` | 8 | 0 | 0 | 软件测试能力独立验证 |
| `com.jinyuni.dengbei_care.verification.GatewayFrameCodecItem001VerificationTest` | 7 | 0 | 0 | 软件测试能力独立验证 |
| `com.jinyuni.dengbei_care.cloud.CloudConfigTest` | 3 | 0 | 0 | ITEM-004 实现侧自检 |
| `com.jinyuni.dengbei_care.verification.GatewayFrameCodecItem001LegacyParityTest` | 3 | 0 | 0 | 软件测试能力独立验证 |
| `com.jinyuni.dengbei_care.ExampleUnitTest` | 1 | 0 | 0 | 既有 |
| **合计** | **72** | **0（0 errors、0 skipped）** | 0 | — |

新增 23 项用例（TD-SW-002 对应）：

| 用例 | 覆盖 | 结果 |
|---|---|---|
| `bothSwitchesOff_isNotConfigured` | L0-06 两开关都关 → `未设置报警`/NORMAL | PASS |
| `enabledButNothingMatches_isNoAlarm` | 启用但无命中 → `无报警`/NORMAL | PASS |
| `baselineInsideFiveMinuteWindow_triggersDrop` | L0-07① 5 分钟窗内基准触发（WARNING、窗口=5） | PASS |
| `baselineInsideWindow_triggersDrop_andSmallestMatchingWindowIsReported` | 窗口选择：`now-5/8/10/15min`→5；`-36min`→10；`-41min`→15；`-46min`→无 | PASS |
| `exactlyAtDropThreshold_doesNotTrigger` | L0-07② 恰好等于 tempDrop / humiDrop 不触发 | PASS |
| `dropRequiresBothTemperatureAndHumidity_strictAnd` | L0-07③ 仅温度满足 / 仅湿度满足 / 湿度上升 → 不触发 | PASS |
| `baselineOlderThanAllWindowSlack_doesNotTrigger` | L0-07④ `now-50min` 超出全部回溯容差；`now-35min` 含边界命中 5 分钟 | PASS |
| `windowSlackIsInclusiveAtBothBounds` | 窗口上下界含边界 | PASS |
| `samplesTooRecent_doNotTrigger_negativeControl` | L0-07⑤ 3 个样本但均在 1 分钟内 → 不触发（证明已非“第 N 个样本”） | PASS |
| `noHistory_doesNotTriggerAndDoesNotThrow` | L0-07⑥ 无历史 → 不触发且不抛错 | PASS |
| `newestSampleInWindowIsUsedAsBaseline` | 窗口内取时间最新样本作为基准 | PASS |
| `customSlackNarrowsTheWindow` | 回溯容差可传入覆盖（slack=0 时边界外样本不参与） | PASS |
| `thresholdExceeded_triggersWithStrictComparison` | L0-08 36 / 28.9 / 76 / 39.9 触发 | PASS |
| `exactlyAtThreshold_doesNotTrigger` | L0-08 恰好 35.0/29.0/75.0/40.0 与范围内 34.9/74.9 均不触发 | PASS |
| `thresholdOnlyWhenEnabled` | 只开下降 → 超限不报；只开超限 → 下降条件不报下降 | PASS |
| `bothEnabled_bothHit_thresholdMessageWins` | L0-09 同时成立 → 超限文案覆盖下降 | PASS |
| `bothEnabled_dropOnly_dropMessage` / `bothEnabled_thresholdOnly_thresholdMessage` | L0-09 单独成立 → 各自文案 | PASS |
| `emergencyOnlyAboveSixtyFive` | L0-10 仅 `>65.0` 触发（65.0/64.9 不紧急；65.1/100.0 紧急） | PASS |
| `emergency_overridesEverything_andNotifiesImmediately` | 紧急覆盖且与开关无关、`notifiesImmediately=true` | PASS |
| `evaluate_isPure_doesNotMutateHistory_andIsDeterministic` | 纯函数：确定性 + 不修改入参列表 | PASS |
| `result_carriesNoTemperatureOrHumidityValues` | 输出不携带任何温度/湿度数值 | PASS |
| `nonFiniteHistorySamples_areIgnored` | NaN/±Infinity 历史样本被忽略，不污染判定 | PASS |

### 3. 构建与打包

```
./gradlew :app:assembleDebug --offline
=> BUILD SUCCESSFUL
```

| 产物 | 大小 |
|---|---|
| `dengbei_care/app/build/outputs/apk/debug/app-debug.apk` | 10,359,236 B（多 dex） |

产物级核查（直接读 APK 的 dex 字节流）：`AlarmEvaluator`、`ReadingAttribution`、`CloudConfig`、`GatewayFrameCodec` 均已打包。编译告警仍为既有 5 条，无新增。

### 4. 判定

ITEM-005 的期望行为成立：下降判定改为基于时间戳的 5/10/15 分钟窗口（基准取窗口内最新样本，含回溯容差，无基准不成立）；阈值超限与紧急（>65.0℃）语义与改动前一致（严格比较、恰好等于不触发、超限文案优先、紧急覆盖且不受去抖限制）；四类文案与严重级别齐备；纯函数、不修改入参、输出不含温度/湿度数值；全量 72 项宿主机用例通过、构建成功。

未覆盖（属后续能力/队列项）：① 与 `MqtttService.processTemperatureHumidityData` 的接线及基准样本来自本地库（ITEM-010，需 ITEM-006/007 的库查询 API）；② 真机链路与 UI 展示（TD-SW-002 T-SW-L2-13/L2-14/L2-15、T-SW-L3-01..03，需设备）。本项为纯逻辑层，不声称已改变运行行为。

---

## AT-006 · ITEM-006（SQLite v3 与待发箱表）

### 1. 环境与代码版本

| 项 | 值 |
|---|---|
| 工作目录 | `D:\mypro\beiwo2\AIP\dengbei_care` |
| 命令 | `./gradlew :app:testDebugUnitTest --offline`、`./gradlew :app:assembleDebug --offline`、`./gradlew :app:compileDebugKotlin --offline --rerun` |
| Gradle / AGP / JDK | 8.4（wrapper）/ 8.3.2 / openjdk 21.0.2 |
| 代码状态 | 相对上一提交：`TemperatureDatabaseHelper.kt` 修改（版本 2→3、新增待发箱表/索引/迁移块、既有 DDL 由 private 改公开常量）；新增 `test/.../db/DatabaseSchemaV3Test.kt` |

### 2. 宿主机单元测试

```
./gradlew :app:testDebugUnitTest --offline
=> BUILD SUCCESSFUL in 887ms
```

| 套件 | 项数 | 失败 | 跳过 | 归属 |
|---|---|---|---|---|
| `com.jinyuni.dengbei_care.db.DatabaseSchemaV3Test` | 9 | 0 | 0 | 本项实现侧自检（新增） |
| `com.jinyuni.dengbei_care.telemetry.AlarmEvaluatorTest` | 23 | 0 | 0 | ITEM-005 实现侧自检 |
| `com.jinyuni.dengbei_care.telemetry.ReadingAttributionTest` | 16 | 0 | 0 | ITEM-003 实现侧自检 |
| `com.jinyuni.dengbei_care.protocol.GatewayFrameCodecTest` | 11 | 0 | 0 | ITEM-001 实现侧自检 |
| `com.jinyuni.dengbei_care.verification.AlarmEvaluatorItem005VerificationTest` | 10 | 0 | 0 | 软件测试能力独立验证 |
| `com.jinyuni.dengbei_care.verification.ReadingAttributionItem003VerificationTest` | 8 | 0 | 0 | 软件测试能力独立验证 |
| `com.jinyuni.dengbei_care.verification.GatewayFrameCodecItem001VerificationTest` | 7 | 0 | 0 | 软件测试能力独立验证 |
| `com.jinyuni.dengbei_care.cloud.CloudConfigTest` | 3 | 0 | 0 | ITEM-004 实现侧自检 |
| `com.jinyuni.dengbei_care.verification.GatewayFrameCodecItem001LegacyParityTest` | 3 | 0 | 0 | 软件测试能力独立验证 |
| `com.jinyuni.dengbei_care.ExampleUnitTest` | 1 | 0 | 0 | 既有 |
| **合计** | **91** | **0（0 errors、0 skipped）** | 0 | — |

新增 9 项用例（TD-SW-002 对应）：

| 用例 | 覆盖 | 结果 |
|---|---|---|
| `databaseVersion_isThree` | 版本号升到 3 | PASS |
| `pendingUploadsTable_hasRequiredColumnsAndCompositePrimaryKey` | 列齐备（device_id/time/temperature/humidity/attempts/next_attempt_at）+ `PRIMARY KEY(device_id, time)` | PASS |
| `pendingUploadsIndex_coversNextAttemptAt_andIsIdempotent` | `idx_pending_uploads_next(next_attempt_at)` + `IF NOT EXISTS` | PASS |
| `tableAndColumnNames_areStable` | 表/列名常量 | PASS |
| `temperatureTable_ddlIsUnchangedFromV2` | 既有表 DDL 逐字未变（T-SW-L0d-06 前提） | PASS |
| `alarmEventsTable_ddlIsUnchangedFromV2` | 同上 | PASS |
| `existingIndexes_areUnchangedFromV2` | 两个既有索引逐字未变 | PASS |
| `migrationV2ToV3_onlyCreatesNewObjects` | 迁移仅 2 步且均为 `IF NOT EXISTS`，不含 DROP/ALTER/UPDATE | PASS |
| `migrationV2ToV3_neverReferencesLegacyTables` | 迁移语句不引用 `temperature(...)`/`alarm_events` | PASS |

### 3. 构建与告警

```
./gradlew :app:assembleDebug --offline
=> BUILD SUCCESSFUL
```

| 产物 | 大小 |
|---|---|
| `dengbei_care/app/build/outputs/apk/debug/app-debug.apk` | 10,360,124 B（较 ITEM-005 的 10,359,236 B 增加 888 B） |

全量重编译（`--rerun`）共 11 条 warning，全部位于改动前既有位置（`MqtttService` 5 条、`TemperatureDatabaseHelper.storeAlarmEvent` 的 `rowId` 冗余初值 1 条（未改动函数）、`MacIdBox`/`VibrationPlayer`/`DashboardFragment`/`NotificationsFragment`/`ZhuCeFragment` 各 1 条），**无新增 warning 指向本项新增代码**。

### 4. 判定

ITEM-006 可在宿主机确定断言的部分全部成立：`DATABASE_VERSION = 3`；`pending_uploads` 表含全部要求列与 `(device_id, time)` 复合主键；`next_attempt_at` 索引存在且带 `IF NOT EXISTS`；既有 `temperature`/`alarm_events` 的 DDL 与索引逐字未变；v2→v3 迁移只新建新对象、不触碰既有表；迁移异常回退不抛异常（不采用破坏性重建）；全量 91 项宿主机用例通过、构建成功。

未覆盖（需 Android 运行时，属软件测试能力）：v2→v3 真实升级后历史行可查、v1→v3 路径、冲突对象导致的异常回退实测、既有查询 API 结果一致 —— TD-SW-002 §4.2 T-SW-L0d-01/02/03/06。本项不声称设备侧迁移已验证。

另：待发箱 DAO 与上传编排（`enqueue/peekBatch/ack/markFailed`）属 ITEM-009；入库真实时间戳属 ITEM-007。
