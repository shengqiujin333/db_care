# 蹬被报警器接口契约（IC-001）

状态：候选接口架构契约  
版本：1.0  
范围：Android 闪光灯参数下发、CW32 传感器参数接收/测量/告警上报、CH592 中继器透传、Android 读取。本文定义接口，不包含原理图、固件或 App 实现。

## 1. 系统边界与数据流

```text
Android 参数页
  └─ 后置闪光灯 OOK/Manchester ─> 光敏接收 ─> CW32 传感器
       （仅 Hall 磁铁触发的配置窗口内供电）       │
                                              ├─ 1 min 温湿度采样
                                              └─ 433 MHz 10 B 加密帧
                                                    ↓
                                              CH592 中继器
                                                    └─ BLE FFE0/FFE2 读取 ─> Android

Android ── BLE FFE0/FFE3 写入 ──> CH592（现有传感器 ID 绑定接口，保持不变）
```

配置通道不经过中继器。Hall 事件只授权一次有界配置会话，不改变正常遥测路径。

## 2. 参数模型与约束

六个参数使用 IEEE-754 binary32、little-endian，按下表固定排序。接收端必须先完整校验整帧，再以一次原子操作替换当前配置；失败时保留旧配置。

| 序号 | 字段 | 单位 | 合法范围 | 关系约束 |
|---:|---|---|---:|---|
| 0 | `temp_drop_c` | °C | 0.0..20.0 | 非负 |
| 1 | `humidity_drop_pct` | %RH | 0.0..100.0 | 非负 |
| 2 | `temp_low_c` | °C | -40.0..85.0 | `< temp_high_c` |
| 3 | `humidity_low_pct` | %RH | 0.0..100.0 | `< humidity_high_pct` |
| 4 | `temp_high_c` | °C | -40.0..85.0 | `> temp_low_c` |
| 5 | `humidity_high_pct` | %RH | 0.0..100.0 | `> humidity_low_pct` |

NaN、正负 Infinity、负零和超出范围的值均拒绝。比较采用严格越界：`current < low`、`current > high`；下降告警为最近 20 个有效的一分钟样本中任一样本与当前值之差满足 `temp_drop_c` 或 `humidity_drop_pct`。任一条件成立即立即上报；否则距离上次成功上报满 60 个采样周期时上报。传感器重启后首次有效样本立即上报。

## 3. Android → 传感器光学配置协议 OPTCFG/1

### 3.1 会话与物理编码

- Hall 有效沿触发后，传感器给光敏前端供电并开启 30 s 接收窗口；窗口超时、成功提交或连续 3 帧失败即断电。Hall 未触发时忽略光信号。
- Android 应要求用户将后置闪光灯对准光敏器件，在 30 s 内发送同一帧 3 次，帧间暗区 500 ms；收到端接受首个通过全部校验的帧。
- OOK 定义：亮=`1`，灭=`0`。位编码为 Manchester：每位 40 ms，`0=亮20 ms+灭20 ms`，`1=灭20 ms+亮20 ms`。接收端允许半位宽 ±25%。
- 每帧前导为 16 次 `0x55`，随后发送下述字节帧，所有字节均 MSB-first。发送前、结束后保持灭灯至少 500 ms。

### 3.2 字节帧（35 B）

| 偏移 | 长度 | 名称 | 定义 |
|---:|---:|---|---|
| 0 | 2 | magic | `0x44 0x42`（ASCII `DB`） |
| 2 | 1 | version | `0x01` |
| 3 | 1 | message_type | `0x01` = SET_THRESHOLDS |
| 4 | 1 | payload_len | `0x18`（24 B） |
| 5 | 4 | transaction_id | uint32 little-endian；Android 每次设置递增，0 禁用 |
| 9 | 24 | payload | 六个 binary32，顺序见第 2 节 |
| 33 | 2 | crc16 | CRC-16/CCITT-FALSE，覆盖偏移 0..32；poly=`0x1021`、init=`0xFFFF`、refin/refout=false、xorout=`0x0000`；结果 big-endian |

接收端仅在 magic、版本、类型、长度、CRC、数值范围和关系全部通过后提交。与最近成功 `transaction_id` 相同的帧视为幂等重放：不得重复写 Flash，但可结束会话。更新须写入带版本与 CRC 的非易失记录；掉电恢复只能装载校验通过的完整记录，否则装载编译期安全默认值并标记“未配置”。本版没有光学回传，因此 Android 的“发送完成”只表示三帧已发出，不得宣称传感器已确认。

## 4. 传感器内部采样接口

- AHT 温湿度每 60 s 产生一个有效样本；温度内部标准值为有符号 0.1 °C，湿度为无符号 0.1 %RH。
- 历史环形缓冲固定 20 项，每项建议 4 B（`int16 temp_x10` + `uint16 humidity_x10`），共 80 B；仅成功测量写入。无需保存 20 份 float。
- 读取失败不写缓冲、不进行阈值判断；连续失败属于诊断状态，不能伪造 0 值遥测。
- “立即上报”指完成当前测量和帧编码后启动 433 MHz 发送，不等待小时定时器；发送失败策略由后续固件实现定义，但不得把失败记作成功上报。

## 5. 传感器 → 中继器 433 MHz 接口（兼容现有 10 B 空口）

本次保持现有 10 B Feistel 帧及射频时序，不把六个配置参数放入遥测帧。加密前明文固定为：

| 偏移 | 长度 | 字段 | 编码 |
|---:|---:|---|---|
| 0 | 4 | `sensor_id` | MCU UID 选取的 4 B，字节序保持现网 |
| 4 | 2 | `temperature_x10` | int16 little-endian，单位 0.1 °C |
| 6 | 2 | `humidity_x10` | uint16 little-endian，单位 0.1 %RH |
| 8 | 2 | `crc16` | CRC-16/CCITT-FALSE(偏移 0..7)，little-endian |

随后使用现有 8 轮 10 B Feistel、系统密钥、S-box 和置换表加密。中继器必须按相同字段和单位解码，CRC 失败丢弃。特别注意：当前传感器调用 `encode_frame10(mcu_uid, huminityvalue, tempvalue, ...)` 与函数签名 `(temp, hum)` 相反；实现节点必须修正调用参数，不能把这一现状当成契约。

## 6. 中继器 → Android BLE 接口（兼容现有 GATT）

- 服务 UUID：`0000FFE0-0000-1000-8000-00805F9B34FB`。
- 遥测读取特征：FFE2，READ，当前固定值区 160 B。Android 读取后按现有 AES-128 ECB 聚合帧解密；明文为 `gateway_id[6] | device_count[1] | repeated(sensor_id[4] | humidity_x10_be[2] | temperature_x10_be[2])`，其余块填充规则保持现有实现。每个读数 8 B。
- 绑定写入特征：FFE3，WRITE。载荷保持 `0xA1 | count:u8 | sensor_id[count][4]`；`count <= 50` 且总长度必须为 `2 + 4*count`。长度、命令或数量不合法时中继器不得更新绑定表。
- FFE1/FFE4/FFE5 不承载本次新增配置语义。现有源码注释中的 FFFx 与实际宏 FFE0..FFE5 冲突时，以宏和 Android 使用的 FFE UUID 为准。
- 本版 Android 使用 READ 而非通知；若后续改为通知，需另立协议版本并完成 CCCD/分片契约，不在本能力范围内。

## 7. 兼容、错误与验收规则

- OPTCFG/1 是新增接口；未知 `version` 或 `message_type` 必须拒绝，不能猜测解析。
- 433 MHz 与 BLE 格式保持现有宽度，旧 Android/中继器不因参数配置功能而失效。
- 所有跨 MCU 多字节字段均按本契约显式字节序处理，不得用结构体裸拷贝。
- 日志不得记录系统密钥；当前源码硬编码密钥属于安全技术债，密钥轮换/设备认证需单独安全设计，不由本接口节点实现。
- 最小互操作向量：六个值 `[2.5, 10.0, 28.0, 30.0, 36.0, 85.0]`，`transaction_id=1`；payload 的前 8 B 应为 `00 00 20 40 00 00 20 41`。实现验证必须覆盖 CRC 错误、截断、NaN、上下限倒置、重复 transaction、掉电中断写入、Hall 未触发、20 样本回绕、立即/小时上报，以及 433 温湿度字段未交换。

## 8. 实现交接

- 硬件设计需提供 Hall 数字输入、可关断的光敏前端电源控制和可定时采样的数字输入，并确认 CR2032 峰值电流与暗电流预算。
- 传感器固件实现 OPTCFG/1、原子持久化、20 项缓冲和上报调度，并修正 `encode_frame10` 实参次序。
- Android 实现参数校验、OPTCFG/1 编码和相机闪光灯时序；UI 明示“单向发送、无设备确认”。
- 中继器仅需保持现有 433 解码与 BLE 聚合接口兼容；不得擅自承担光学配置转发。
