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
3. **`parseHexData`（小端）仍在**：由队列下一项 ITEM-002 处理（TD-SW-002 §4.3 T-SW-L1-03 判定口径为"无功能入口"）。
4. **未修改**：服务器常量、SQLite、告警、UI、`/upload_data`、依赖与 `build.gradle.kts` 均属后续队列项，本轮未动。
5. 本项不涉及真机 BLE 读取行为；BLE 与真实网关的联调属后续测试能力（AA-002 §10、TD-SW-002 §4.6 REG-02）。
