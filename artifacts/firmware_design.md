# 传感器/中继器固件设计方案（FD-001）

状态：固件设计（firmware_engineer.firmware_solution_design）
版本：1.0
范围：蹬被报警器在既有源码基础上的固件增量设计。**传感器固件（CW32L010Y8M6）为本次主要改动**；**中继器固件（CH592 beiwov2）仅做契约对齐的字段顺序调整**。本方案明确模块/任务划分、数据流、时序、资源预算、接口处理、异常策略与实现约束，**不承担代码实现**（由后续固件实现能力承接）。
固件源码存在且作为参考输入：`CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/`（传感器）、`CH592EVT/EVT/EXAM/BLE/beiwov2/`（中继器）。

上游输入：
- 需求：readme.txt；接口契约 IC-001（artifacts/interface_contract.md）；硬件接口 HWI-001（artifacts/hardware_interface.md）；硬件方案 HW-001（artifacts/hardware_solution.md）；原理图 SCH-001（artifacts/schematic.md）；固件需求 FR-001（artifacts/firmware_requirements.md）；调度架构 RTA-001（artifacts/firmware_rtos_architecture.md）。

---

## 1. 现状基线（源码核定）

### 1.1 传感器（CW32L010Y8M6，64 KB Flash / 4 KB SRAM，MDK `IRAM(0x20000000,0x01000) IROM(0,0x10000)`）

已用 GPIO（源码+原理图一致）：PA0/PA1=主晶振、PA3/PA4=AHT10 软 I2C、PA5/PA6=UART1 调试、PA7/PA8=SWD、PB0/PB1=OSC32、PB2/PB3=UM2005C 433 TWI；**空闲：PA2、PB4、PB5、PB6**（SCH-001 核定：PB04=Hall 输入+中断、PB05=光敏输入、PB06=F2 电源使能）。

关键现状：
- 主循环：`temperature_process()` → `send_data_to_gateway()` → `go_to_sleep()`；深睡由 RTC 唤醒。
- RTC：`RTC_INTERVAL_EVERY_1M`（1 min/拍）；`rtc_set_cnt` 到 2 置 `sample_flag`（现为每 2 min 采样）、到 5 置 `send_flag`（现为每 5 min 上报）。**与需求"1 min 采样 / 1 h 上报"不符，需改**。main.c 中"1s间隔"注释为陈旧注释，与宏语义（1 min）不符。
- 采样：AHT10/AHT21B 软 I2C（地址 0x70/0x71），温度 `tempvalue`=int16×0.1 °C、湿度 `huminityvalue`=uint16×0.1 %RH。
- 上报：`send_data_to_gateway()` 组 10 B 帧 → `encode_frame10()` Feistel 加密 → `app_um2005C_send_data()`（LPTIM 位时钟，20B 前导+4B 同步+10B 数据 @10 kbps，约 30 ms）。
- 433 明文由 `encode_frame10(uid, temp, hum, out)` 内部组包：`p[0..3]=uid_pick、p[4..5]=temp LE、p[6..7]=hum LE、p[8..9]=crc16 LE`，随后 8 轮 Feistel。
- 中断：RTC_IRQHandler（业务节拍）、LPTIM_IRQHandler（433 位时钟）、GPIOA/GPIOB_IRQHandler（空壳，可扩展）。

### 1.2 中继器（CH592 beiwov2）

- 433 接收：UM2006A 驱动 + `app_ev1527`（TMR2 捕获）→ `decode_frame10()` 解密校验 → `app_um2006A.c` 按绑定表写入 `sensorres[i*8+0..7]` → `build_padded_frame_blocks()`（设备块 16 B，前 8 B=`payload8` 直拷）→ AES-128 ECB 加密 → `SimpleProfile_SetParameter(CHAR2)`。
- BLE：服务 FFE0，遥测 CHAR2（FFE2），绑定 CHAR3（FFE3，`0xA1|count|id[count][4]`）。
- 板上 AHT20 测量任务（TMOS，遗留路径）不影响本次功能。

### 1.3 端到端温湿度字段顺序（源码逐级核定，见 §6.4）

传感器 `encode_frame10(mcu_uid, huminityvalue, tempvalue, ...)` **实参颠倒** → 线上 p[4..5]=湿度、p[6..7]=温度；中继器 `decode_frame10` 解出 `tempervalue`（实际=湿度）、`humivalue`（实际=温度），`sensorres` 记录为 `[id | tempervalue | humivalue]`（大端），即值上 `[id | 湿度_be | 温度_be]`；Android 活跃解析路径 `parsePlainFrame`（BLE FFE2 读取）按 `humi=u16be(p8,4)、temp=s16be(p8,6)` 解析，即期望每设备记录 `[id | humidity_be(2) | temperature_be(2)]`。**现状端到端数值正确（双重交换相互抵消），但字段命名与契约不符。** 契约（IC-001 §5/§6）要求修正传感器实参 + 中继器记录顺序，须同步落地（§6.4）。

---

## 2. 总体架构与模块划分

保持裸机事件驱动（RTA-001），主循环分发 4 个逻辑任务：T_SAMPLE、T_REPORT、T_OPTCFG、T_SLEEP。

### 2.1 文件级模块规划（传感器工程）

| 文件 | 动作 | 职责 |
|---|---|---|
| `USER/src/main.c` | 修改 | 主循环状态机、RTC 节拍改值（1 min 采样、60 拍小时上报）、T_SLEEP、PB04/05/06 初始化 |
| `USER/src/measure.c` | 修改 | 采样流程保持；写历史环；阈值判定；上报调度（立即/小时/首样本）；**修正 `encode_frame10` 实参**；AHT 失败诊断 |
| `USER/src/interrupts_cw32l010.c` | 修改 | GPIOB_IRQHandler 处理 PB04 Hall 有效沿（去抖+置标志）；RTC_IRQHandlerCallBack 节拍调整；LPTIM 窗口采样模式 |
| `USER/src/encrytogate.c` | 不动 | Feistel 加密复用（CRC16-CCITT-FALSE 亦可复用） |
| `USER/src/optcfg.c`（新） | 新增 | OPTCFG/1：F2 供电控制、Manchester/OOK 解码状态机、帧校验、NVM 提交、会话状态机 |
| `USER/src/hall.c`（新） | 新增 | Hall 输入配置、极性/去抖、事件产生 |
| `USER/src/params.c`（新） | 新增 | 参数模型（RAM x10 表示）、Flash 非易失记录读写、编译期默认值 |
| `USER/src/history.c`（新） | 新增 | 20 项环形缓冲（4 B/项）写入与遍历 |
| `USER/inc/*.h` | 修改/新增 | 对应头文件；`main.h` 增加新模块声明与引脚宏 |

> 新增模块 ≤4 个 .c 文件，职责单一；纯函数（CRC、浮点校验、Manchester 解码、帧解析）与硬件访问分离，便于单元测试。

### 2.2 中继器文件级改动

| 文件 | 动作 | 职责 |
|---|---|---|
| `CH592EVT/EVT/EXAM/BLE/beiwov2/APP/app_um2006A.c` | 修改 | `sensorres[i*8+4..7]` 由 `[tempervalue | humivalue]` 调整为 `[humivalue | tempervalue]`（对齐 IC-001 §6 每设备记录 `[id | humidity_be | temperature_be]`） |

---

## 3. 数据流

### 3.1 配置通道（新增）

```text
手机后置闪光灯 OOK/Manchester 光
  → Q1 光敏三极管（F2，PB06 使能供电）
  → PB05 施密特输入（LPTIM 10 ms 采样，100 Hz）
  → optcfg 解码器（前导同步 → Manchester 解码 → 35 B 帧）
  → 帧校验（magic/version/type/len/CRC/数值范围/关系）
  → params 原子提交（RAM → Flash 非易失记录）
  → 会话结束（断电 F2，回深睡）
Hall 磁铁（门磁）→ U8 Hall IC → PB04 EXTI 有效沿 → 去抖 → 开启一次配置会话
```

### 3.2 遥测通道（保持，仅调度与实参变化）

```text
AHT10（软 I2C）→ 1 min 采样 → temp_x10/hum_x10
  → 历史环（20×4 B）写入
  → 阈值判定（越界/下降/小时/首样本）
  → 433 10 B Feistel 帧（实参修正后 温度|湿度 正序）
  → UM2005C 发射 → CH592 中继器解码 → BLE 聚合（记录顺序 hum|temp）→ Android
```

---

## 4. 时序设计

### 4.1 采样与上报节拍（RTC）

- RTC 保持 `RTC_INTERVAL_EVERY_1M`；`rtc_set_cnt` 每拍 +1，**到 1** 置 `sample_flag` 并清零 → 每 1 min 一次采样。
- `samples_since_report`：每成功采样 +1；**成功上报后归零**；≥60 触发小时上报（IC-001 §2"满 60 个采样周期"，即以"上次成功上报"为基准，发送失败不得归零）。
- 首样本上报：重启后首次有效采样完成后立即上报（IC-001 §2）。
- `send_flag` 机制废弃或重构为 `report_req` + `samples_since_report`；`go_to_sleep()` 条件扩展为 `sample_flag==0 && report_req==0 && hall_event==0 && optcfg_active==0`。

### 4.2 测量时序（沿用现有状态机）

现有 `temperature_process()` 4 步状态机（init→0xAC 触发→读取→换算）沿用；每拍唤醒执行一轮。深睡期间 AHT10 断电（现状）。读取失败：不写历史、不判定、`fail_cnt++`（诊断），不伪造 0 值。

### 4.3 光学配置窗口时序（关键论证）

**OPTCFG/1 空中时间核算（40 ms/bit Manchester）**：
- 前导 16×0x55 = 128 bit × 40 ms = **5.12 s/帧**
- 数据帧 35 B = 280 bit × 40 ms = **11.20 s/帧**
- 单次完整发射 = **16.32 s**；帧间暗区 0.5 s；3 帧连发 ≈ **50 s**

**窗口设计决策**：
- 契约名义窗口 30 s 在 40 ms/bit 下最多容纳 1 帧完整 + 1 帧部分（第 2 帧完成于 33.14 s），**无法容纳契约要求的 3 次重发**。
- 本设计将窗口置为 **35 s**（`OPTCFG_WINDOW_MS = 35000`）：恰好容纳 2 次完整发射（16.32×2+0.5=33.14 s），覆盖"首帧失败→第二帧重试"；成功提交时**提前关闭**（典型 ~16.3 s 即结束）。
- "连续 3 帧失败即断电"规则实现为计数：会话内失败帧计数 ≥3 立即断电；35 s 窗口下通常由超时兜底。该计数保留以对齐契约语义。
- **开放风险**：30 s 窗口与 3 帧重发在 40 ms/bit 下不自洽，已记录 §10；Android 侧按"窗口内最多 2 帧、失败后重新贴磁铁再试"交互设计，避免宣称 3 帧必达。

**窗口内时序**：F2 上电稳定（去耦 C18，留 5 ms 稳定期）→ 100 Hz（10 ms）LPTIM 采样 PB05 → 解码器处理 → 窗口结束（成功/超时/3 失败）→ PB06 拉高断电 → 回深睡。窗口内若有待发 433 帧，延后到窗口结束。

### 4.4 433 发送时序

保持现状（LPTIM 位时钟 @10 kbps，约 30 ms 突发）。新增约束：与 OPTCFG 窗口互斥（LPTIM 归属互锁，RTA-001 §5）。

---

## 5. 资源预算

### 5.1 RAM（CW32L010：4 KB，地址 0x20000000）

| 项 | 字节 | 说明 |
|---|---|---|
| 现状基线（g_send_data[1024]、txbf/rxbf/setdatabf/send_data、栈、库） | ~1.6 K | 实测编译后基线 |
| 历史环 history[20] | 80 | `{int16 t; uint16 h;}` 4 B/项 |
| 参数 RAM 模型 params | ~32 | 6×x10 阈值 + version/txn/crc/标志 |
| OPTCFG 解码缓冲 + 状态机 | ~96 | 帧缓冲 64 B + 状态 ~32 B |
| 433 帧（复用 send_data[10]） | 0 | 复用 |
| **新增合计** | **≤ 300** | 目标 <4 KB 的 60% 占用 |
| 优化项（P2） | -960 | `APP_GTIMER_DATA_MAX` 1024→64（实际 TX 仅 34 B）可释放 ~960 B，实现节点按需采纳 |

### 5.2 Flash（64 KB）

| 项 | 估算 |
|---|---|
| 新增代码（optcfg/hall/params/history + 解码表） | ~3–5 KB，含 CRC/校验 |
| 参数非易失记录 | 1 页（128 B/页，CW32L010 共 512 页） |
| 预留 | 余量充足 |

### 5.3 功耗

| 状态 | 预算 | 依据 |
|---|---|---|
| 深睡 | <10 µA（新增 F1 约 8 µA + F2 断电≈0） | HW-001 §4/SCH-001 §4 |
| 配置窗口 | ≤35 s × ~1.1 mA ≈ 10.7 µAh/次 | SCH-001 §3.2（R6=2.2 kΩ） |
| 采样 | 1 min × ~200 ms 数百 µA | 现状 |
| 433 突发 | 10–30 mA × ~30 ms | 现状，CR2032 支持短脉冲 |

---

## 6. 接口处理

### 6.1 引脚与初始化（SCH-001 核定）

| 信号 | 脚 | 方向/模式 | 初始化约束 |
|---|---|---|---|
| HALL_DOUT | PB04 | 输入 + EXTI（上升/下降沿按 Hall 极性） | 上拉/下拉按器件；去抖 5–10 ms；深睡可唤醒 |
| OPT_IN | PB05 | 输入（施密特） | 窗口内才采样；平时不使能 |
| F2_PWR_EN | PB06 | 推挽输出，**低=使能** | 上电/复位默认高（F2 断电，R4 保证安全）；深睡保持 |

Hall 极性以编译期宏（如 `HALL_ACTIVE_EDGE = GPIO_IT_FALLING`）配置；GPIOB_IRQHandler 中仅置 `hall_event` 标志（ISR 内不开窗、不上电）。

### 6.2 OPTCFG/1 光学解码（新增 optcfg.c）

- **采样**：窗口内 LPTIM 10 ms（100 Hz）采样 PB05 电平，喂软件 Manchester 解码器。25 Hz（40 ms/位）为接口最低要求，本设计在活跃窗口内用 100 Hz 过采样 + 边沿/相位重同步，以覆盖 ±25% 半位宽容差与长帧累计漂移。
- **解码状态机**：IDLE（长暗区 >300 ms 复位）→ PREAMBLE_SYNC（检测 0x55 方波，锁定位相位）→ BIT_DECODE（每位 4 样本，首半位电平=位值，校验半位过渡）→ FRAME（收满 35 B，MSB-first）→ VALIDATE → COMMIT / DISCARD。
- **相位歧义**：前导 0x55 为纯方波存在半位相位歧义，以帧首 magic `0x44 0x42` 校验消除（两相位不同时通过）。
- **校验顺序**：magic → version → message_type → payload_len → **CRC16-CCITT-FALSE**（覆盖 0..32，poly 0x1021、init 0xFFFF、无反射、xorout 0、结果 big-endian）→ 6×binary32 LE 解析 → NaN/±Inf/负零拒绝 → 范围（temp_drop 0..20、hum_drop 0..100、temp ±40..85、hum 0..100）→ 关系（temp_low<temp_high、hum_low<hum_high）。全通过才提交，任一失败整帧丢弃。
- **幂等**：transaction_id（uint32 LE）与最近成功值相同 → 不写 Flash，但结束会话（IC-001 §3.2）。
- **字节序**：所有多字节字段显式 LE/BE 处理，禁止结构体裸拷贝。

### 6.3 参数模型与 NVM（新增 params.c）

- RAM 表示：阈值换算为 x10 整数（`temp_low_x10`、`hum_low_x10`、`temp_high_x10`、`hum_high_x10`、`temp_drop_x10`、`hum_drop_x10`，int16/uint16 域内），判定全整数域完成；6 个 float 原始值仅存于 NVM 记录与校验路径。
- Flash 记录（1 页 128 B）：`magic(2) | version(1) | transaction_id(4) | float×6(24) | crc16(2) | pad`。写入流程：`FLASH_UnlockPage` → 擦页 → 写记录（带 CRC）→ 锁页。掉电中断写入 → CRC 失败 → 上电装载默认值并标记"未配置"。
- 装载：上电读页 → magic+version+CRC+范围校验全通过才装载；否则编译期安全默认（建议默认 temp_drop=2.0°C、hum_drop=10%RH、temp_low=28°C、hum_low=30%RH、temp_high=36°C、hum_high=85%RH，与 IC-001 §7 互操作向量一致，具体默认值由实现节点与产品确认）。
- **风险**：现有 `FLASH_SetReadOutLevel(FLASH_RDLEVEL2)` 每次上电执行；实现节点须验证 RDP2 下应用态页擦写可用，否则调整保护策略或持久化方案。

### 6.4 433/BLE 端到端字段顺序（跨设备协同，见 §1.3）

| 节点 | 现状 | 契约要求 | 所需改动 |
|---|---|---|---|
| 传感器 `encode_frame10` 调用 | `(huminityvalue, tempvalue)` → 线上温度字段=湿度 | `(tempvalue, huminityvalue)` | 修正实参（FR-302） |
| 中继器 `decode_frame10` 解读 | `tempervalue` 实为湿度 | 字段名与实际一致 | 无需改（解码本身按契约） |
| 中继器 `sensorres`/BLE 记录 | `[id | tempervalue | humivalue]`（temp 前） | `[id | humidity_be | temperature_be]`（IC-001 §6） | 交换 4..7 字节（FR-401） |
| Android `parsePlainFrame` | 每设备记录 `[id | hum_be | temp_be]`（u16be@4/s16be@6） | 与 IC-001 §6 一致 | 保持，复核端到端数值 |

> **同步约束**：传感器实参修正与中继器记录顺序调整必须**同批落地并一起联调**；只改一侧会令 Android 读到交换值。此项为跨设备实现交接重点（§9）。

### 6.5 433 帧（保持格式）

`encode_frame10` 内部明文布局（p[0..3]=uid_pick、p[4..5]=temp LE、p[6..7]=hum LE、p[8..9]=crc16 LE）与 `decode_frame10` 一致，仅调用实参修正；CRC16-CCITT-FALSE 复用 encrytogate.c 既有实现。

### 6.6 BLE（中继器）

保持 FFE0/FFE2/FFE3 兼容；绑定写入校验（`0xA1|count|id[count][4]`，`count≤50`，长度 `2+4*count`）不变；不承担光学配置转发。

---

## 7. 异常与容错策略

| 场景 | 策略 |
|---|---|
| AHT 读取失败 | 不写历史、不判定、不伪造 0 值；`fail_cnt++` 供诊断；连续失败不影响节拍与深睡 |
| 433 发送失败 | 有界重试（如 3 次×1 s 退避）；仍失败保留待上报状态，`samples_since_report` 不归零，小时调度自然重试；**失败不记成功** |
| Flash 写失败/掉电中断 | 记录含 CRC；上电仅装载校验通过记录，否则默认值；写失败保持 RAM 旧配置 |
| 解码帧校验失败 | 整帧丢弃、解码器复位；失败帧计数，≥3 或超时断电关窗 |
| Hall 噪声/窗口内重复触发 | 去抖 5–10 ms；窗口活跃时忽略新沿 |
| LPTIM 模式冲突 | 433 与 OPTCFG 互斥；窗口内待发帧延后 |
| 未知 version/message_type | 拒绝整帧，不猜测解析 |
| 密钥日志 | 任何日志不含系统密钥 |

---

## 8. 实现约束（交实现节点）

1. **不改动** AHT10 软 I2C（PA3/PA4）、UM2005C 433（PB2/PB3）、晶振、SWD、CR2032 供电与去耦、现有 Feistel/CRC 算法。
2. PB04 配输入+EXTI（极性宏）；PB05 配输入；PB06 配推挽输出且默认高；**深睡前保持 PB06 高**。
3. RTC 节拍：`rtc_set_cnt` 到 1 置 `sample_flag` 并清零；新增 `samples_since_report`（成功上报归零，≥60 小时上报，首样本立即上报）。
4. 修正 `encode_frame10(mcu_uid, tempvalue, huminityvalue, send_data)`。
5. 新模块命名与职责见 §2.1；纯函数与硬件访问分离，便于单元测试。
6. 窗口 35 s（`OPTCFG_WINDOW_MS=35000`），100 Hz 采样，成功提前关窗；与 433 发送互斥。
7. 深睡唤醒源：RTC（定时）+ GPIOB EXTI（Hall）；唤醒后按标志分发。
8. 调试：保留 UART1 打印（不含密钥），提供阈值/状态观测点（如打印 `samples_since_report`、解码失败计数）。
9. 不引入 RTOS；不修改中继器调度与 TMOS 任务划分。
10. 温度内部值按契约为有符号 0.1 °C：当前 `measure.c` 中 `tempvalue` 为 `uint16_t`，实现节点改为 `int16_t` 并同步历史环（int16）与 433 帧组包，避免负温回绕。

---

## 9. 验证与验收映射（IC-001 §7 最小互操作 + 错误规则）

传感器实现节点须完成/覆盖：

| 用例 | 预期 |
|---|---|
| 互操作向量 [2.5,10,28,30,36,85]、txn=1 | payload 前 8 B = `00 00 20 40 00 00 20 41`；提交正确 |
| CRC 错误 / 截断 / 未知 version / 未知 type | 整帧拒绝，旧配置保留 |
| NaN / ±Inf / 负零 / 越界 / 上下限倒置 | 拒绝 |
| 重复 transaction | 幂等，不二次写 Flash，会话结束 |
| 掉电中断写入 | 上电装载默认/上次完整记录 |
| Hall 未触发 | 忽略光信号，F2 断电 |
| 20 样本回绕 | 下降告警按最近 20 项判定 |
| 立即 / 小时 / 首样本上报 | 三条路径时序正确 |
| 433 温湿度字段 | 实参修正后正序（联调中继器验证） |
| Manchester 相位/抖动 | ±25% 半位宽容差下 3 帧至少 1 帧通过 |

嵌入式测试节点联调验证：端到端 `传感器→中继器→Android` 数值一致；深睡电流 <10 µA；窗口与上报互斥。

---

## 10. 开放风险与决策记录

| # | 风险/决策 | 说明 | 状态 |
|---|---|---|---|
| R1 | 30 s 窗口 vs 3 帧重发不自洽 | 40 ms/bit 下 3 帧≈50 s；本设计取窗口 35 s（容 2 帧）+ Android 重贴磁铁重试 | 设计决策，需 Android/架构知会 |
| R2 | `RTC_INTERVAL_EVERY_1M` 语义 | 宏=1 min/拍；main.c 注释"1s"为陈旧注释 | 已按 1 min 设计，实现节点复核寄存器手册 |
| R3 | RDP2 下 Flash 应用态擦写 | 现有代码每次上电 `FLASH_SetReadOutLevel(2)`；需验证页擦写可用 | 实现节点验证 |
| R4 | 深睡 GPIO 状态保持 | PB06 断电态与 EXTI 唤醒在深睡下保持 | 实现节点验证 |
| R5 | LPTIM 双模式共享 | 433 位时钟与 OPTCFG 采样互斥重配 | 实现节点落地互锁 |

---

## 11. 跨节点交接

- **固件实现（传感器）**：按 §2.1/§6/§8 落地；新增 optcfg/hall/params/history 模块；修正实参；RTC 节拍改值；LPTIM 互锁。
- **固件实现（中继器）**：`app_um2006A.c` 中 `sensorres` 记录字节交换（§6.4），与传感器修正同批联调。
- **Android 实现**：按 IC-001 实现闪光灯 OPTCFG/1 编码与 30 s 交互（窗口实际 35 s，最多 2 帧，失败重贴磁铁）；`parseHexData` 保持 hum-first 并复核端到端数值。
- **嵌入式测试**：按 §9 验收矩阵与 RTA-001 §8 并发时序用例执行。
- **PCB/结构**：PB04/05/06 接线与 Hall/光敏朝向按 SCH-001，固件无额外约束。
