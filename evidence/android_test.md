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

---

## AT-007 · ITEM-007（单条读数写入与真实采样时间）

### 1. 环境与代码版本

| 项 | 值 |
|---|---|
| 工作目录 | `D:\mypro\beiwo2\AIP\dengbei_care` |
| 命令 | `./gradlew :app:testDebugUnitTest --offline`、`./gradlew :app:assembleDebug --offline`、`./gradlew :app:compileDebugKotlin --offline --rerun`、`grep -rn` |
| Gradle / AGP / JDK | 8.4（wrapper）/ 8.3.2 / openjdk 21.0.2 |
| 代码状态 | 相对上一提交：`TemperatureDatabaseHelper.kt`（删列表式写入 + 新增单条 API/常量/门控纯函数）、`MqtttService.kt`（存储段改调新 API）；新增 `test/.../db/TemperatureWriteItem007Test.kt` |

### 2. 宿主机单元测试

```
./gradlew :app:testDebugUnitTest --offline
=> BUILD SUCCESSFUL in 3s
```

| 套件 | 项数 | 失败 | 跳过 | 归属 |
|---|---|---|---|---|
| `com.jinyuni.dengbei_care.telemetry.AlarmEvaluatorTest` | 23 | 0 | 0 | ITEM-005 |
| `com.jinyuni.dengbei_care.telemetry.ReadingAttributionTest` | 16 | 0 | 0 | ITEM-003 |
| `com.jinyuni.dengbei_care.protocol.GatewayFrameCodecTest` | 11 | 0 | 0 | ITEM-001 |
| `com.jinyuni.dengbei_care.verification.AlarmEvaluatorItem005VerificationTest` | 10 | 0 | 0 | 软件测试能力独立验证 |
| `com.jinyuni.dengbei_care.db.DatabaseSchemaV3Test` | 9 | 0 | 0 | ITEM-006 |
| `com.jinyuni.dengbei_care.verification.ReadingAttributionItem003VerificationTest` | 8 | 0 | 0 | 软件测试能力独立验证 |
| `com.jinyuni.dengbei_care.verification.GatewayFrameCodecItem001VerificationTest` | 7 | 0 | 0 | 软件测试能力独立验证 |
| `com.jinyuni.dengbei_care.db.TemperatureWriteItem007Test` | 4 | 0 | 0 | 本项实现侧自检（新增） |
| `com.jinyuni.dengbei_care.cloud.CloudConfigTest` | 3 | 0 | 0 | ITEM-004 |
| `com.jinyuni.dengbei_care.verification.GatewayFrameCodecItem001LegacyParityTest` | 3 | 0 | 0 | 软件测试能力独立验证 |
| `com.jinyuni.dengbei_care.ExampleUnitTest` | 1 | 0 | 0 | 既有 |
| **合计** | **95** | **0（0 errors、0 skipped）** | 0 | — |

新增 4 项用例（TD-SW-002 对应）：

| 用例 | 覆盖 | 结果 |
|---|---|---|
| `insertStatement_isReplaceInto_withFourBoundColumns` | 写入语句 = `INSERT OR REPLACE INTO temperature (time, device_id, temperature, humidity) VALUES (?, ?, ?, ?)`（列序即绑定序） | PASS |
| `idempotency_isGuaranteedByCompositePrimaryKeyAndReplace` | `INSERT OR REPLACE` + `PRIMARY KEY(time, device_id)` ⇒ 同 `(dev_id,time)` 不产生重复行 | PASS |
| `writtenTimeIsTheCallerProvidedSampleTime_noOffsetBackfill` | 语句内无任何偏移/回填表达式（无 `60`/`+ `/`- `） | PASS |
| `guardGate_onlyStoreflagOneAllowsWriting` | `isGuardActive(1)=true`，0/-1/2 均为 false ⇒ 守护关闭不写任何行 | PASS |

### 3. 构建与静态核查

```
./gradlew :app:assembleDebug --offline
=> BUILD SUCCESSFUL
```

| 产物 | 大小 |
|---|---|
| `dengbei_care/app/build/outputs/apk/debug/app-debug.apk` | 10,359,865 B（较 ITEM-006 的 10,360,124 B 减少 259 B，为删除列表式写入） |

| 检查 | 命令 | 结果 |
|---|---|---|
| 入库路径 60 s 回填清零 | `grep -rn "60L" app/src/main --include=*.kt` | 仅 `telemetry/AlarmEvaluator.kt`（时间窗换算，非入库路径）；**入库路径 0 命中** |
| 旧列表式 API 清零 | `grep -rn "storeTemperatureData\|averageList0\|averageList1" app/src/main --include=*.kt` | 0 命中 |
| 调用点时间语义 | 读 `MqtttService.processTemperatureHumidityData` | `storeTemperatureReading(this, devId, currentTime, temperature, humidity, humistartflag)`；`currentTime = timeOverride ?: 系统时刻`，MQTT 调用传入 `batch.timeSeconds`（payload 首段） |
| 全量重编译告警 | `./gradlew :app:compileDebugKotlin --offline --rerun` | 11 条，与 ITEM-006 同一集合，无新增 |
| 差异范围 | `git status --short` | 2 个产品文件修改 + 1 个新增测试文件 |

### 4. 判定

ITEM-007 可在宿主机确定断言的部分全部成立：写入语句为 `INSERT OR REPLACE` 且以 `(time, device_id)` 为幂等键；写入行的 `time` 直接来自调用方传入的真实采样时间，语句内无偏移/回填；守护门控仅 `storeflag == 1` 可写；BLE/MQTT 两条入口的时间来源已固定到调用点（BLE=接收时刻、MQTT=payload 首段）；入库路径无 60 s 回填与旧列表式 API 残留；全量 95 项宿主机用例通过、构建成功。

未覆盖（需 Android 运行时，属软件测试能力）：真实写入行的 `time` 值、重复写入后行数、守护关闭时 0 行 —— TD-SW-002 §4.2 T-SW-L0d-04。本项不声称设备侧行级结果已验证。

另：非编译的 iOS 参考快照仍含旧列表式 API（本轮不改 iOS）；`AlarmEvaluator` 的 `60L` 为窗口换算，与入库无关。

---

## AT-008 · ITEM-008（`/upload_data` 接口与请求体构造）

### 1. 环境与代码版本

| 项 | 值 |
|---|---|
| 工作目录 | `D:\mypro\beiwo2\AIP\dengbei_care` |
| 命令 | `./gradlew :app:testDebugUnitTest --offline`、`./gradlew :app:assembleDebug --offline`、`./gradlew :app:compileDebugKotlin --offline --rerun` |
| Gradle / AGP / JDK | 8.4（wrapper）/ 8.3.2 / openjdk 21.0.2 |
| 代码状态 | 相对上一提交：`ui/zhuce/ApiService.kt` +24/−1（纯追加端点与 DTO）；新增 `cloud/UploadPayload.kt` 与 `test/.../cloud/UploadPayloadTest.kt` |

### 2. 宿主机单元测试

```
./gradlew :app:testDebugUnitTest --offline
=> BUILD SUCCESSFUL in 3s
```

| 套件 | 项数 | 失败 | 跳过 |
|---|---|---|---|
| `com.jinyuni.dengbei_care.telemetry.AlarmEvaluatorTest` | 23 | 0 | 0 |
| `com.jinyuni.dengbei_care.telemetry.ReadingAttributionTest` | 16 | 0 | 0 |
| `com.jinyuni.dengbei_care.protocol.GatewayFrameCodecTest` | 11 | 0 | 0 |
| `com.jinyuni.dengbei_care.cloud.UploadPayloadTest`（本项新增） | 11 | 0 | 0 |
| `com.jinyuni.dengbei_care.verification.AlarmEvaluatorItem005VerificationTest` | 10 | 0 | 0 |
| `com.jinyuni.dengbei_care.db.DatabaseSchemaV3Test` | 9 | 0 | 0 |
| `com.jinyuni.dengbei_care.verification.ReadingAttributionItem003VerificationTest` | 8 | 0 | 0 |
| `com.jinyuni.dengbei_care.verification.GatewayFrameCodecItem001VerificationTest` | 7 | 0 | 0 |
| `com.jinyuni.dengbei_care.db.TemperatureWriteItem007Test` | 4 | 0 | 0 |
| `com.jinyuni.dengbei_care.verification.TemperatureWriteItem007VerificationTest` | 4 | 0 | 0 |
| `com.jinyuni.dengbei_care.cloud.CloudConfigTest` | 3 | 0 | 0 |
| `com.jinyuni.dengbei_care.verification.GatewayFrameCodecItem001LegacyParityTest` | 3 | 0 | 0 |
| `com.jinyuni.dengbei_care.ExampleUnitTest` | 1 | 0 | 0 |
| **合计** | **110** | **0（0 errors、0 skipped）** | 0 |

新增 11 项用例（TD-SW-002 对应）：

| 用例 | 覆盖 | 结果 |
|---|---|---|
| `serializedJson_hasExactContractFieldNames` | L0-11 逐字 JSON：`{"phone":…,"mac":…,"readings":[{"devId":…,"time":…,"temperature":25.5,"humidity":60.0}]}` | PASS |
| `unitsAreCelsiusAndPercentRh_notRawX10` | 单位：×10 原始值已换算为 ℃/%RH（不得再缩放），负温保持 | PASS |
| `timeIsUnixSeconds_passedThroughUnchanged` | `time` 为 Unix 秒整数 | PASS |
| `devIdComesFromAttributedReading_notFromIndex` | `devId` 来自归因结果而非下标 | PASS |
| `multipleReadings_shareBatchTime_andKeepOrder` | 多读数共享批时间、顺序保持 | PASS |
| `emptyReadings_produceNoPayload` | 空读数集不产生请求（两条构造路径均 null） | PASS |
| `build_keepsPerReadingTimes_forOutboxBackfill` | 待发箱路径逐行时间 | PASS |
| `dtoFieldNames_matchServerContract` | DTO 字段名集合逐字比对 | PASS |
| `apiService_keepsFiveExistingEndpoints_andAddsUploadData` | 端点路径集合 = 既有 5 个 + `/upload_data`（无其它变动） | PASS |
| `existingRequestDataClasses_areUnchanged` | 既有 5 个请求数据类字段未变 | PASS |
| `uploadDataEndpoint_usesJsonContentType` | `Content-Type: application/json` + 入参为 `SensorUploadData` | PASS |

### 3. 构建与告警

```
./gradlew :app:assembleDebug --offline
=> BUILD SUCCESSFUL
```

| 产物 | 大小 |
|---|---|
| `dengbei_care/app/build/outputs/apk/debug/app-debug.apk` | 10,359,867 B |

全量重编译（`--rerun`）共 11 条 warning，与 ITEM-007 同一集合，无新增；`ApiService.kt` diff 为纯追加（既有 5 个端点与 6 个数据类逐字未改，仅原文件末尾换行差异）。

### 4. 判定

ITEM-008 的期望行为成立：`POST /upload_data` 接口与 `SensorReading`/`SensorUploadData` 请求体数据类已就位，Gson 序列化结果与服务器契约逐字一致（字段名、℃/%RH、Unix 秒）；`devId` 来自归因结果；空读数集不产生请求；既有 5 个端点与数据类未变；全量 110 项宿主机用例通过、构建成功。

未覆盖（需真实服务器/网络，属测试能力）：`/upload_data` 实际可达性与服务端 `inserted` 幂等计数（TD-SW-002 T-SW-L2-12）、端到端上传链路（E2E-SW-002）。本项不声称云端落库已验证。

另：本项不含上传编排/待发箱/门控（ITEM-009）与 Service 接线（ITEM-010）。

---

## AT-009 · ITEM-009（待发箱与上传编排）

### 1. 环境与代码版本

| 项 | 值 |
|---|---|
| 工作目录 | `D:\mypro\beiwo2\AIP\dengbei_care` |
| 命令 | `./gradlew :app:testDebugUnitTest --offline`、`./gradlew :app:assembleDebug --offline`、`./gradlew :app:compileDebugKotlin --offline --rerun` |
| Gradle / AGP / JDK | 8.4（wrapper）/ 8.3.2 / openjdk 21.0.2 |
| 代码状态 | 相对上一提交：新增 `cloud/UploadOutbox.kt`、`cloud/CloudUploadRepository.kt`、`test/.../cloud/CloudUploadRepositoryTest.kt`；`app/build.gradle.kts` +5 行（`testOptions.unitTests.isReturnDefaultValues = true`） |

### 2. 宿主机单元测试

```
./gradlew :app:testDebugUnitTest --offline
=> BUILD SUCCESSFUL in 6s
```

| 套件 | 项数 | 失败 | 跳过 |
|---|---|---|---|
| `com.jinyuni.dengbei_care.telemetry.AlarmEvaluatorTest` | 23 | 0 | 0 |
| `com.jinyuni.dengbei_care.telemetry.ReadingAttributionTest` | 16 | 0 | 0 |
| `com.jinyuni.dengbei_care.cloud.CloudUploadRepositoryTest`（本项新增） | 14 | 0 | 0 |
| `com.jinyuni.dengbei_care.protocol.GatewayFrameCodecTest` | 11 | 0 | 0 |
| `com.jinyuni.dengbei_care.cloud.UploadPayloadTest` | 11 | 0 | 0 |
| `com.jinyuni.dengbei_care.verification.AlarmEvaluatorItem005VerificationTest` | 10 | 0 | 0 |
| `com.jinyuni.dengbei_care.db.DatabaseSchemaV3Test` | 9 | 0 | 0 |
| `com.jinyuni.dengbei_care.verification.UploadPayloadItem008VerificationTest` | 9 | 0 | 0 |
| `com.jinyuni.dengbei_care.verification.ReadingAttributionItem003VerificationTest` | 8 | 0 | 0 |
| `com.jinyuni.dengbei_care.verification.GatewayFrameCodecItem001VerificationTest` | 7 | 0 | 0 |
| `com.jinyuni.dengbei_care.db.TemperatureWriteItem007Test` | 4 | 0 | 0 |
| `com.jinyuni.dengbei_care.verification.TemperatureWriteItem007VerificationTest` | 4 | 0 | 0 |
| `com.jinyuni.dengbei_care.cloud.CloudConfigTest` | 3 | 0 | 0 |
| `com.jinyuni.dengbei_care.verification.GatewayFrameCodecItem001LegacyParityTest` | 3 | 0 | 0 |
| `com.jinyuni.dengbei_care.ExampleUnitTest` | 1 | 0 | 0 |
| **合计** | **133** | **0（0 errors、0 skipped）** | 0 |

新增 14 项用例（TD-SW-002 T-SW-L0-12 ①–⑧ + 策略细节）：

| 用例 | 覆盖 | 结果 |
|---|---|---|
| `success_deletesRows_andReportsOutcome` | ① 成功即删除 | PASS |
| `failure_keepsRows_andSchedulesBackoff` | ② 失败保留 + `attempts=1` + `next_attempt_at = now+60` | PASS |
| `retryBecomesDue_afterBackoffElapses_andSucceeds` | ② 退避期内不可取，到期后补传成功 | PASS |
| `maxAttempts_stopsAutoRetry_butKeepsRow` | ③ 达上限后不再自动重试（行保留、请求数停在 3） | PASS |
| `batchLimit_splitsIntoMultipleBatches` | ④ 60 条 → 50 + 10 两批 | PASS |
| `guardClosed_noEnqueue_andNoRequest` | ⑤ 守护关闭：不入队、`gate_closed`、无请求 | PASS |
| `noRegisteredPhone_noEnqueue_andNoRequest` | ⑥ 无手机号：同上 | PASS |
| `duplicateEnqueue_keepsSingleRow_andSecondReturnsFalse` | ⑦ 同 `(devId,time)` 只一行且不改旧值 | PASS |
| `concurrentUploads_areSerialized_secondIsBusy` | ⑧ 并发触发时第二次 `busy`，只 1 次请求 | PASS |
| `backoff_growsExponentially_andCapsAtMaximum` | 退避 60/120/240/480/960/1800 且封顶 | PASS |
| `dueBatch_ordersByNextAttemptAt_thenTime` | 取批排序（next_attempt_at, time, devId） | PASS |
| `emptyOutbox_producesNoRequest` | 空队列 `empty`、无请求 | PASS |
| `payloadFromOutbox_carriesDevIdPerRowTimeAndUnits` | 载荷逐行 devId/time 与单位（含负温） | PASS |
| `enqueueAndUpload_enqueuesAndTriggersAsyncUpload` | 入队 + 异步触发（不阻塞调用方） | PASS |

### 3. 构建与打包

```
./gradlew :app:assembleDebug --offline
=> BUILD SUCCESSFUL
```

| 产物 | 大小 |
|---|---|
| `dengbei_care/app/build/outputs/apk/debug/app-debug.apk` | 10,359,866 B |

| 检查 | 结果 |
|---|---|
| 打包核查（读 APK 多 dex） | `UploadOutbox`、`CloudUploadRepository`、`SqliteUploadOutboxStore`、`PrefsUploadGate`、`RetrofitUploadApi` 均已打包 |
| 全量重编译告警（`--rerun`） | 11 条，与 ITEM-008 同一集合，无新增 |

### 4. 判定

ITEM-009 的期望行为成立：入队按 `(dev_id, time)` 幂等；成功即删除；失败保留并按 `attempts`/`next_attempt_at` 有界退避（参数集中于 `Config`）；达上限不再自动重试但数据保留；单批 50；守护关闭或无手机号不入队不请求；同一时刻串行不重复请求；上传在独立作用域、异常不抛到调用方；待发箱表缺失时降级不崩溃；全量 133 项宿主机用例通过、构建与打包成功。

未覆盖（需设备/网络，属测试能力）：断网→恢复的真实补传与 HTTP 观测（TD-SW-002 T-SW-L2-09/10/11）、服务端 `inserted` 幂等计数（T-SW-L2-12）、真机待发箱行数（T-SW-L0d-05）。本项不声称链路已验证。

另：触发点接线（每轮 BLE 开始/网络恢复/冷启动）属 ITEM-010（TD-SW-002 §8-G3）；本项已提供 `enqueueAndUpload` 与 `triggerUpload()` 供其调用。

---

## AT-010 · ITEM-010（Service 编排接入）

### 1. 环境与代码版本

| 项 | 值 |
|---|---|
| 工作目录 | `D:\mypro\beiwo2\AIP\dengbei_care` |
| 命令 | `./gradlew :app:testDebugUnitTest --offline`、`./gradlew :app:assembleDebug --offline`、`./gradlew :app:compileDebugKotlin --offline --rerun`、`grep -n` |
| Gradle / AGP / JDK | 8.4（wrapper）/ 8.3.2 / openjdk 21.0.2 |
| 代码状态 | 相对上一提交：`MqtttService.kt`（读取周期/告警判定/基准查询/上传触发四处）、`TemperatureDatabaseHelper.kt`（新增 `getSamplesInRange`）；新增 `telemetry/SensorCadence.kt` 与 `test/.../telemetry/SensorCadenceTest.kt` |

### 2. 宿主机单元测试

```
./gradlew :app:testDebugUnitTest --offline
=> BUILD SUCCESSFUL in 3s
```

| 汇总 | 值 |
|---|---|
| 套件数 | 15（含实现侧自检与软件测试能力独立验证） |
| 合计 | **146 tests / 0 failures / 0 errors / 0 skipped** |
| 本项新增 | `SensorCadenceTest` 2 项（180 s / 180000 ms 一致；非旧 5 分钟/1 分钟） |

回归意义：本项改动了 Service 的判定/存储/上传编排，所有既有模块级用例（帧解码/归因/告警判定/载荷/待发箱/存储 schema）均继续通过。

### 3. 构建与告警

```
./gradlew :app:assembleDebug --offline
=> BUILD SUCCESSFUL
```

| 产物 | 大小 |
|---|---|
| `dengbei_care/app/build/outputs/apk/debug/app-debug.apk` | 10,360,647 B |

全量重编译（`--rerun`）共 11 条 warning，与 ITEM-009 同一集合，无新增。

### 4. 接线与兼容面静态核查（独立读取源码）

| 检查 | 结果 |
|---|---|
| 读取周期 = 3 分钟节拍 | `MqtttService.kt:253 READ_INTERVAL_MS = SensorCadence.SAMPLE_PERIOD_MS`；`SensorCadence.SAMPLE_PERIOD_MS == 180000` |
| 手动立即读取保留 | `wakeReadNow` 通道与 `select { onTimeout(...); wakeReadNow.onReceiveCatching }` 未改动 |
| 判定接入 | `840 val verdict = AlarmEvaluator.evaluate(...)`；`Severity` 枚举映射函数 `toAppSeverity()` |
| 基准来自本地库 | `950 loadBaselineSamples` → `955 getSamplesInRange(devId, from, now)`；查询失败→空列表（不报警、不伪造） |
| 真实采样时间戳入库 | `901 storeTemperatureReading(..., currentTime, ...)`（BLE=接收时刻、MQTT=payload 首段，ITEM-007） |
| 上传触发点（4） | 入队后 `911 enqueueAndUpload`；每轮 BLE `388 triggerUpload`；网络恢复 `1244 triggerUpload`（`connectToBroker.onSuccess`）；冷启动 `369 triggerUpload`（`onCreate`） |
| 900 s 去抖保留 | `925 if (now - history.lastAlarmTime > (900 * 1000))` |
| >65℃ 每次通知保留 | `AlarmEmergent` 分支直接震动/铃声/通知（不进去抖通道）；阈值 `AlarmEvaluator.EMERGENCY_TEMP_C = 65.0` |
| 报警文案字面值 | `grep -c "温度湿度下降报警\|温湿度超限报警\|紧急报警，温度超过65度，谨防火灾"` = 3（`when` 映射处逐字保留） |
| 报警事件落库 | `storeAlarmEvent` + `ALARM_TYPE_DROP/THRESHOLD/EMERGENCY` 未变 |
| 旧“第 N 个样本”窗口残留 | `grep -n "data5MinAgo\|data10MinAgo\|data15MinAgo\|isSignificantChange"` = 0 命中 |

### 5. 判定

ITEM-010 的期望行为在代码层面成立：读取周期与 3 分钟采样节拍对齐（手动立即读取保留）；告警判定改由 `AlarmEvaluator`（基准来自本地库，重启后可判定）；入库使用真实采样时间戳；入库成功后幂等入队并异步上传，四个触发点齐备；900 s 去抖、>65℃ 每次通知、报警事件落库与字面值均保留；旧窗口逻辑无残留；全量 146 项宿主机用例通过、构建成功。

未覆盖（需设备/网络，属测试能力）：真机 BLE/MQTT 读数全链路与卡片/通知展示（T-SW-L2-13/14/15、L3-01..03）、断网补传与 HTTP 观测（L2-09..12）、真机待发箱行数（L0d-05）、端到端（E2E J-1/J-5）。本项不声称真机链路已验证。

---

## AT-010R · ITEM-010 修复（缺陷 D-010-1）

### 1. 背景与修复范围

软件测试能力 ST-010 判 **TEST_FAIL**（缺陷 D-010-1：单层 try 覆盖全函数，告警副作用异常会跳过入库/入队上传/卡片更新，涉及 >65℃ 紧急路径）。本轮按修复要求完成：逐步独立保护 + 两个播放器内部降级；**不改变**文案/阈值/去抖/门控/时间戳/上传触发/180 s 周期，也不改变已验证的 12 步编排顺序。

| 文件 | 改动 |
|---|---|
| `MqtttService.kt` | 各副作用步骤加独立 `try/catch`（清 sparkline、紧急通知块、卡片告警、事件落库、入库、入队+上传、卡片读数、去抖告警）；函数内 `try` 块 1 → 9 |
| `RingtonePlayer.kt` | `startAlarm`/`stopAlarm` 不向外抛（URI 为空、`create` 返回 null、`start()`/`stop()` 异常均降级并释放） |
| `VibrationPlayer.kt` | `vibratePhone`/`stopVibration` 不向外抛（服务缺失用 `as?`、`vibrate`/`cancel` 异常降级） |

### 2. 原始结果

```
$ python evidence/software_verify_orchestration_item010.py
...
== F 步骤隔离 ==
  INFO  函数内 try 块数 = 9
  PASS  单一全函数 try=False；通知块与入库之间独立保护=True
  PASS  报警事件落库与入库之间独立保护=True
== RESULT ==
  OK
EXIT=0
```

| 检查 | 结果 |
|---|---|
| A1–A5（周期对齐/手动读取保留） | PASS |
| B 编排顺序 12 项 | PASS（12/12，顺序未变） |
| C1–C11（文案/类型/去抖/门控/旧窗口清零/阈值只在 evaluator） | PASS |
| D1–D5（四个触发点 + 仅守护中入队，triggerUpload 计数 3） | PASS |
| E（库基准范围/降级/SQL 语义 + 宿主 SQLite 执行） | PASS |
| F（步骤隔离） | PASS |

```
./gradlew :app:testDebugUnitTest --offline  => BUILD SUCCESSFUL ; 146 tests / 0 failures / 0 errors / 0 skipped
./gradlew :app:assembleDebug --offline       => BUILD SUCCESSFUL ; app-debug.apk 10,361,755 B
./gradlew :app:compileDebugKotlin --offline --rerun => 11 warnings（与修复前同一集合）
```

其余独立门禁回归（均 **EXIT=0**）：`software_verify_timestamp_write_item007.py`、`software_verify_cloud_upload_item009.py`、`software_verify_upload_payload_item008.py`、`software_verify_db_migration_item006.py`、`software_static_check_no_little_endian_parser.py`、`software_static_check_server_migration.py`。

### 3. 判定

D-010-1 的两条修复要求均已满足：① 每一步副作用独立保护（通知/震动/铃声失败不再跳过入库、入队上传与卡片更新；反之亦然）；② 两个播放器内部降级不向外抛。兼容面与编排顺序未变（测试门禁 A–F 全 PASS），构建与 146 项宿主机用例无回归。

未覆盖：真机链路/通知/待发箱/HTTP（属 E2E-SW-002，需与生产隔离的测试环境）。

---

## AT-011 · ITEM-011（卡片状态：去掉“离线”，改为相对时间 + 条件上报提示）

### 1. 环境与代码版本

| 项 | 值 |
|---|---|
| 工作目录 | `D:\mypro\beiwo2\AIP\dengbei_care` |
| 命令 | `./gradlew :app:testDebugUnitTest --offline`、`./gradlew :app:assembleDebug --offline`、`grep -n` |
| Gradle / AGP / JDK | 8.4（wrapper）/ 8.3.2 / openjdk 21.0.2 |
| 代码状态 | 相对上一提交：`DeviceState.kt`（删离线判定 + 新增相对时间）、`DeviceCardAdapter.kt`（chip 四态 + `reportHint`）、`item_device_card.xml`（新增提示行）、`HomeViewModel.kt`（仅注释）；新增 `test/.../ui/home/DeviceStateTest.kt` |

### 2. 宿主机单元测试

```
./gradlew :app:testDebugUnitTest --offline
=> BUILD SUCCESSFUL in 3s
```

| 汇总 | 值 |
|---|---|
| 套件数 | 16 |
| 合计 | **155 tests / 0 failures / 0 errors / 0 skipped** |
| 本项新增 | `DeviceStateTest` 7 项 |

新增 7 项用例（TD-SW-002 T-SW-L0-14）：

| 用例 | 覆盖 | 结果 |
|---|---|---|
| `neverReported_isNeverReported_andHasNoAgeLabel` | `latestTime == 0` → 未收到语义，无相对时间 | PASS |
| `ageLabel_buckets` | 刚刚 / 1 分钟前 / 40 分钟前 / 59 分钟前 / 1 小时前 / 23 小时前 / 1 天前 / 3 天前（含 59s/60s/59min/60min/23h/24h 边界） | PASS |
| `staleDevice_getsRelativeLabel_notOffline` | 40 分钟无数据 → “40 分钟前”，且标签不含“离线” | PASS |
| `noOnlineOrOfflineThresholdEntryPoints` | 反射：`isOnline` 方法已删、无 OFFLINE 常量 | PASS |
| `displayName_prefersAlias_thenDevId` | 显示名规则不变 | PASS |
| `severity_isCarriedThroughUnchanged` | NORMAL/WARNING/ALARM 透传不变 | PASS |
| `emptyPlaceholder_matchesNeverReportedSemantics` | `empty()` 占位即未收到语义、温湿度 0、sparkline 空 | PASS |

### 3. 构建与静态核查

```
./gradlew :app:assembleDebug --offline
=> BUILD SUCCESSFUL
```

| 产物 | 大小 |
|---|---|
| `dengbei_care/app/build/outputs/apk/debug/app-debug.apk` | 10,361,689 B |

| 检查 | 结果 |
|---|---|
| 旧判定入口 | `grep -rn "isOnline\|OFFLINE_THRESHOLD" DeviceState.kt ui/home/` → 0 命中 |
| chip 四态文案 | `未收到`/`紧急`/`警告`/`正常` 齐备；无 `chip.text = "离线"` |
| 配色语义 | 未收到=brand_secondary、紧急=brand_error、警告=brand_warning、正常=brand_ok（与改动前一致） |
| 温湿度占位 | `isNeverReported()` → `--`（未改） |
| 提示行接线 | `item_device_card.xml` 含 `reportHint`，sparkline 顶部约束改为 `@id/reportHint` |
| 编译告警 | 无新增（既有 11 条不变） |

### 4. 判定

ITEM-011 的期望行为成立：长时间无新数据的设备不再显示“离线”，改为相对时间 + “按条件上报，可能长时间无上报”提示；未收到数据的设备仍为“未收到”且温湿度 `--`；报警级别文案与配色不变；旧离线判定入口（`isOnline`/30 min 阈值）已彻底移除；全量 155 项宿主机用例通过、构建成功。

未覆盖（需设备，属测试能力）：卡片实际渲染/配色/截图为 T-SW-L3-01/02/03；`bindStatusChip` 位于 Adapter（需 Android 视图），其分支由静态核查 + 设备侧用例覆盖。

---

## AT-012 · ITEM-012（详情页规则呈现、24h 范围、高温强调）

### 1. 环境与代码版本

| 项 | 值 |
|---|---|
| 工作目录 | `D:\mypro\beiwo2\AIP\dengbei_care` |
| 命令 | `./gradlew :app:testDebugUnitTest --offline`、`./gradlew :app:assembleDebug --offline`、`grep -rn`、七个独立门禁脚本 |
| Gradle / AGP / JDK | 8.4（wrapper）/ 8.3.2 / openjdk 21.0.2 |
| 代码状态 | 相对上一提交：`DeviceDetailFragment.kt`（规则绑定 + 最新读数高温强调 + 35℃ 参考线 + 24h 范围）、`fragment_device_detail.xml`（两个新 TextView）；新增 `ui/detail/DetailPresentation.kt` 与 `test/.../ui/detail/DetailPresentationTest.kt` |

### 2. 宿主机单元测试

```
./gradlew :app:testDebugUnitTest --offline
=> BUILD SUCCESSFUL in 3s
```

| 汇总 | 值 |
|---|---|
| 套件数 | 17 |
| 合计 | **168 tests / 0 failures / 0 errors / 0 skipped** |
| 本项新增 | `DetailPresentationTest` 8 项 |

新增 8 项用例（TD-SW-002 L3-04/05/06）：

| 用例 | 覆盖 | 结果 |
|---|---|---|
| `ruleText_matchesReadme` | 规则文案含 3 分钟 / 0.9 / 无光 / 35.0 / 才上报 | PASS |
| `ruleText_containsNoAmbientLightOrIlluminanceInfo` | 文案不含光照/照度/lux/有光 | PASS |
| `ruleText_givesNoAppInferredReportingReason` | 无“因下降…”/“上报原因”类推断结论 | PASS |
| `chartRangeLimit_isTwentyFourHours` | 24h = 86,400,000 ms | PASS |
| `rangeLimitMessage_matchesLimit_andHasNoFiftyMinuteWording` | 提示文案含“24 小时”，不含“50”/“分钟以内” | PASS |
| `highTemp_isStrictlyAbove35` | 35.1/36/100 强调；**35.0**/34.9/0/-10 不强调 | PASS |
| `highTempSuffix_onlyForHighReadings` | 强调后缀 | PASS |
| `highTempThreshold_equalsReadmeValue` | 阈值 = 35.0 | PASS |

### 3. 构建与静态核查

```
./gradlew :app:assembleDebug --offline
=> BUILD SUCCESSFUL
```

| 产物 | 大小 |
|---|---|
| `dengbei_care/app/build/outputs/apk/debug/app-debug.apk` | 10,391,985 B |

| 检查 | 结果 |
|---|---|
| 旧范围上限清零 | `grep -rn "50 分钟\|49 \* 60" app/src/main` → 0 命中 |
| 环境光/照度词元 | detail 包 + 详情布局 `grep -rni "lux\|光照\|照度"` → 0 命中（规则文案保留 readme 原文的“无光”条件） |
| 规则与上限同源 | `DeviceDetailFragment` 引用 `DetailPresentation.REPORT_RULE_TEXT` / `MAX_CHART_RANGE_MS` / `rangeLimitMessage()` / `isHighTemperature()` |
| 七个独立门禁 | 全部 **EXIT=0**（`software_verify_orchestration_item010` / `timestamp_write_item007` / `cloud_upload_item009` / `upload_payload_item008` / `db_migration_item006` / `static_check_no_little_endian_parser` / `static_check_server_migration`） |
| 编译告警 | 无新增 |

### 4. 判定

ITEM-012 可在宿主机确定断言的部分全部成立：规则文案与 readme 一致（含“无光”条件、不含环境光/照度信息与推断结论）；图表范围上限 24 小时（旧 50 分钟字面量已清零）；高温强调仅 `>35.0℃`（`=35.0℃` 不强调，未收到显示 `--`）；七个独立门禁无回归；全量 168 项宿主机用例通过、构建成功。

未覆盖（需设备，属测试能力）：详情页截图与文案核对、24h 查询出图、35.0/35.1 强调对比 —— TD-SW-002 T-SW-L3-04/05/06。

### 5. 需上游确认的口径差异

任务描述/TD-SW-L3-04 同时要求“规则含‘且无光’”与“不出现无光”。本实现按“规则忠于 readme”处理（规则文案含“无光”；界面不显示环境光状态/照度，无推断结论）。若上游要求连规则原文也不得出现该词，属文案变更（改一处常量）。

---

## AT-013 · ITEM-013（早起报告文案与零样本表述）

### 1. 环境与代码版本

| 项 | 值 |
|---|---|
| 工作目录 | `D:\mypro\beiwo2\AIP\dengbei_care` |
| 命令 | `./gradlew :app:testDebugUnitTest --offline`、`./gradlew :app:assembleDebug --offline`、`grep -rn`、七个独立门禁脚本 |
| Gradle / AGP / JDK | 8.4（wrapper）/ 8.3.2 / openjdk 21.0.2 |
| 代码状态 | 相对上一提交：`report/ReportGenerator.kt`（3 处文案：零样本行/摘要/低条数建议）、`res/layout/activity_report.xml`（`tools:text` 预览）；新增 `test/.../report/ReportGeneratorTest.kt` |

### 2. 宿主机单元测试

```
./gradlew :app:testDebugUnitTest --offline
=> BUILD SUCCESSFUL in 2s
```

| 汇总 | 值 |
|---|---|
| 套件数 | 18 |
| 合计 | **181 tests / 0 failures / 0 errors / 0 skipped** |
| 本项新增 | `ReportGeneratorTest` 8 项 |

新增 8 项用例（TD-SW-002 T-SW-L3-07）：

| 用例 | 覆盖 | 结果 |
|---|---|---|
| `deviceWithoutReports_showsNoReportYesterday_notNoData` | 零样本设备 → `  - 昨日无上报`；全文无“无数据” | PASS |
| `summary_statesConditionalReportingAndNonEqualIntervals` | 摘要含“按条件上报”/“非等间隔”/“共上报 N 条”/“触发 M 次报警”；不含“每 3 分钟”/“288”/“共采集” | PASS |
| `statsLines_andAlarmCountStructure_unchanged` | 逐行格式不变（采样/温度/湿度/报警），段内顺序不变 | PASS |
| `sameInput_producesIdenticalStatistics` | 同批数据两次生成逐字相同；负温与一位小数格式保留（`-5.0°C`/`12.3°C`） | PASS |
| `lowSampleCountSuggestion_hasNoCadenceOrOfflineInference` | 低条数建议含“按条件上报”，不含“离线”/“288”/“预期” | PASS |
| `enoughSamples_producesNoLowCountSuggestion` | 条数 ≥24 不产生低条数建议 | PASS |
| `alarmBasedSuggestions_stillEmitted` | 紧急/下降≥3/超限≥5 建议结构不变 | PASS |
| `noDevices_showsPlaceholder` | 无设备时“暂无设备数据。” | PASS |

### 3. 构建与静态核查

```
./gradlew :app:assembleDebug --offline
=> BUILD SUCCESSFUL
```

| 产物 | 大小 |
|---|---|
| `dengbei_care/app/build/outputs/apk/debug/app-debug.apk` | 10,391,985 B（仅文案改动，与上一项同尺寸） |

| 检查 | 结果 |
|---|---|
| 旧文案清零 | `grep -rn "无数据\|288\|共采集\|离线" report/ activity_report.xml` → 0 命中 |
| `DailyReportWorker` | 无代码改动（已核对）：统计查询与通知均委托 `ReportGenerator.build(...).toText()`，文案随之生效 |
| 七个独立门禁 | 全部 **EXIT=0** |
| 编译告警 | 无新增 |

### 4. 判定

ITEM-013 可在宿主机确定断言的部分全部成立：零样本显示“昨日无上报”；报告说明按条件上报且条数非等间隔；同一批数据的统计数值与逐行格式逐字不变；低条数建议不再暗示固定节拍或推断离线；全量 181 项宿主机用例通过、构建成功；七个独立门禁无回归。

未覆盖（需设备，属测试能力）：报告页/通知实际展示与截图 —— TD-SW-002 T-SW-L3-07。
