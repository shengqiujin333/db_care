# 传感器/网关固件设计方案（FD-002）

负责角色：固件工程
版本：6.0（替代 rev 5.0）
检查时点：2026-11-08（Asia/Shanghai）
本轮基线：Git `491853165f63713a1dc4df108034859ac9afb6e6`（接手时 `git status --porcelain` 为空）
本能力范围：固件方案设计。本文只做需求核对、模块/任务划分、数据流、时序、资源预算、接口处理、异常策略与实现约束；不实现代码、不构建、不下载、不操作实板、不替验证判定通过。

上游输入：
- `readme.txt`（需求方，权威，修改点 1–8）
- `artifacts/system_analysis.md`（本轮系统分析，已验证交接）
- `artifacts/interface_contract.md` IC-002 v3.0（冻结的采样/上报语义、433/BLE 布局与九组规范向量）
- 硬件事实：`sensor_hardware/pstxnet.dat`、`sensor_hardware/MAIN_BOARD.BOM`、`gateway_board.xml`
- 器件手册：`gxht40.pdf`（GXHT4x，V2.6）
- 既有固件源码（参考输入）：`CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/`、`CH592EVT/EVT/EXAM/BLE/beiwov2/`
- 伴随产物：`artifacts/firmware_requirements.md`（FWR-002 rev 6.0）、`artifacts/firmware_rtos_architecture.md`（RTA-002 rev 6.0）、`artifacts/firmware_tasks.yaml`
- 背景证据（历史，不作本轮结果）：`evidence/test.md`（EV-017 等）、`evidence/build.md`、`evidence/driver_test.md`

---

## 1. 现状基线与差异核对

### 1.1 as-built 事实（本轮逐文件读取，HEAD `4918531`）

| 项 | 现状 | 依据 |
|---|---|---|
| GXHT40 驱动 | 已实现：0x44/0x45 运行期探测与缓存、0xFD 高重复率测量、等 10 ms、读 6 B、温度字/湿度字各一次 CRC-8、整数 x10 换算与有效域、有界读重读（5）与整帧重测（3）；失败不写输出；失败后清缓存、重探两地址并做有界总线恢复（9 个 SCL 脉冲 + STOP） | `USER/src/gxht40.c`、`USER/inc/gxht40.h`、`USER/src/sf_i2c.c:i2c_probe_addr/i2c_bus_recover` |
| 光照通路 | 已结构化：`light_result_t {valid, adc_ok, samples_ok, mean_adc_code, code_min, code_max, dark}`；PB05 供电 → 100 ms → PB04/AIN11 8 次 → PB05 低；全部转换超时 `valid=false` 且不合成满量程；`valid = adc_ok && LIGHT_DARK_CALIBRATED`；`dark = light_is_dark(mean, valid, C_dark)`（uint32 乘法、边界相等为暗） | `USER/src/light.c`、`USER/src/fw_core.c:light_is_dark`、`USER/inc/sensor_config.h` |
| 3 分钟节拍 | RTC 1 分钟节拍累计 3 次置 `sample_flag`；上电 `sample_flag=1` 给首采样机会 | `USER/src/main.c`、`USER/inc/sensor_config.h` |
| 条件上报判定 | **仍是旧语义（与 readme 冲突）**：`cur==350` 直接 `return false`（把恰好 35.0℃ 整体排除）；升温分支实际为 `prev-cur > SENSOR_REPORT_DROP_X10`（降温方向） | `USER/src/fw_core.c:106–122`、`USER/inc/sensor_config.h` |
| 待上报冻结 | **缺失**：`send_data_to_gateway()` 组帧直接读 `s_prev_temp_x10`/`s_last_hum_x10`，没有冻结快照；待上报期间新触发只置 `report_req=1`，不保留原样本 | `USER/src/measure.c:565–591` |
| UART1 调试 | 已实现：BOOT 横幅、`VDD`/`IOTEST`/`IOSIG`/`BUS` 上电行、每周期 `S` 行、失败周期 `G` 行；9600 8N1；打印后关闭串口；`SENSOR_DEBUG_UART` 目标默认 1、宿主默认 0 | `USER/src/debug_trace.c`、`USER/src/main.c` |
| 调试通道等待 | **无界**：`trace_putc()` 等 `TXE`、`debug_trace_flush_close()` 等 `TXE`/`TXBUSY` 均为 `while(...)` 无计数；注释“固定行长保证有界”只约束字符数，不约束硬件标志永不就绪 | `USER/src/debug_trace.c:32–39,143–152` |
| 无光基准 | `LIGHT_DARK_REF_CODE=0`、`LIGHT_DARK_CALIBRATED=0`（未标定占位），升温分支因此始终关闭 | `USER/inc/sensor_config.h` |
| 旧通路退役 | `hall/optcfg/params/history` 源文件已删除；`USER/` 内无功能引用；PB04/PB05 只属于 `light.c`，PB06 未使用 | `USER/src`、`USER/inc`、`MDK/Project.uvprojx`、`EWARM/project.ewp` |
| 空口/BLE 布局 | 未改：433 明文 10 B（uid4/T s16LE/RH u16LE/CRC16LE），BLE 每设备 8 B（id4/RH u16BE/T s16BE）；无光照/原因/时间字段 | `USER/src/encrytogate.c`、`CH592EVT/EVT/EXAM/BLE/beiwov2/APP/{feistel_al.c,bleencrypt.c}` |
| 构建事实（历史） | 上一轮交付件 Keil `0 Error / 1 既有告警`（`main.c` 空循环体），`Code=20100 RO=644 RW=116 ZI=1676`（Flash 20,744 B / RAM 1,792 B）；本轮**未重建**，当前交付构建的尺寸/产物摘要须由下游重新取得 | `evidence/build.md`、`evidence/test.md` |

### 1.2 与 `readme.txt` 的一致性结论

| readme 修改点 | 现状 | 结论 |
|---|---|---|
| 1 温湿度改为 GXHT40 | 驱动、双 CRC、有效域、重探/恢复均已实现；**真实目标采集历史上未成功**（见 §1.3） | 机制在，实板功能未闭环（T4） |
| 2 光敏电阻 + 分压，不改硬件 | 采样、供电时序、均值/有效性、1/3 判据均已实现 | 机制一致；标定未闭环（T5） |
| 3 采集改为 3 分钟 | 1 分钟 ×3 已实现 | 一致（保持） |
| 4 本次比上次**升温 >0.9℃ 且无光**，或 **>35℃** | 方向错误（降温）+ 恰好 35.0℃ 被整体排除 | **必须修正**（T1） |
| 5 服务器 broker | 不属固件域 | 不在本文 |
| 6 同步 Android；不改 iOS；取消光通讯/霍尔 | 固件侧无残留功能引用；Android 属其自身职责 | 固件一致（保持） |
| 7 无光阈值取完全无光的 1/3 | 判据/有效性门禁已实现；`C_dark` 未实测 | 机制在，数值未标定（T5） |
| 8 UART1 初始化并打印调试 | 已实现并有字节冻结格式；**发送等待无界**；当前构建未在本轮复观测 | 机制在，需有界化（T3）+ 当前构建再观测（§13） |
| 「不需要修改硬件内容」 | 不改网表/BOM/空口/BLE | 一致（保持） |

另有 IC-002 §2 已冻结、当前实现缺失的一项：**待上报样本在重试期间必须保持同一笔读数**（当前用“前一有效温度/最近湿度”组帧，若待上报跨越新采样会被悄然替换）。列为 T2。

### 1.3 真实目标历史事实与本轮未知

历史证据（**上一轮运行**，仅作背景，不冒充本轮结果）在 `evidence/test.md` EV-017 记录：

- 受测板 `uid=6A002C00`，历史调试口为 COM42（**本轮登记入口为 COM43，不得沿用 COM42**）。
- 7 个连续采样周期全部 `q=0`；失败周期 `G s=2 a44=0 a45=0 rd=0 at=3 raw=------------`；`BUS … ack=none`；`IOTEST … gc=3`；`IOSIG scl=95555 sda=30303`；`VDD ok=1 … mv=3289`。
- 即：主机侧可拉低/释放两线、位序列送到引脚、供电轨约 3.289 V，但**没有任何 7bit 地址收到 ACK**。

竞争假设（**不预设唯一结论**，须由 T4 的当前构建一手数据判别）：

| 假设 | 支持现象 | 反证/待判别 | 可执行的固件侧动作 |
|---|---|---|---|
| H1 U9 焊点/器件本体未连通或损坏 | 所有地址无 ACK、`gc` 无改善、跨 21 分钟稳定 | 需要 U9 引脚连通性/替换后的对照；MCU 侧电压不能证明 U9.3 焊点导通 | 固件侧无法修复；保留失败语义并提交硬件处置与复测条件 |
| H2 总线电气/上拉/短路 | `idle` 出现 0 位或 SDA 被持续拉低 | 历史 `idle=3`、主机可拉低/释放，倾向排除但未做位级电气测量 | 有界恢复后可重试；不得用软件掩盖 |
| H3 固件时序/速率/引脚映射不匹配器件 | ACK 缺失也可能来自时钟过快、建立/保持不足、地址字节错误 | `IOSIG` 仅为软件回读，不是示波器位时序；需降低速率/加长半位与上电余量后对照 | 在器件手册范围内调整软 I²C 半位延时、上电等待、重试上限；不得改判据 |
| H4 供电瞬态/焊接接触 | 静态 VDD 正常但测量瞬间跌落 | 需要测量瞬间的供电观测（当前无电流表/示波器授权入口） | 保持上电余量与有界重试；记录缺失的观测能力 |

本能力不操作设备，因此**不判定 H1–H4 中任一成立**；T4 的任务是取得当前构建的一手数据并按证据收敛，而不是重复已 disproved 的“加打印就能好”。

### 1.4 本轮设计目标与不做项

1. 修正上报判定方向与严格边界（T1）。
2. 补齐待上报冻结快照与 `P` 观测行（T2）。
3. 消除调试通道无界等待，保证诊断故障不阻塞采样/深睡（T3）。
4. 以当前构建实板数据定位并修复 GXHT40 采集失败；失败路径保持不伪造（T4）。
5. 完成完全遮光基准标定并使 1/3 判据生效（T5）。

**不做**：不重写已符合项；不改网关功能、不改 Android/iOS、不改硬件/网表/BOM；不引入 RTOS、动态内存、NVM 记录；不新增心跳/时间戳/协议版本；不为通过而放宽 CRC、量程、判据或标定；不把历史日志当成本轮实板结果。

---

## 2. 硬件接口核定

### 2.1 引脚（传感器）

| 引脚 | 功能 | 方向/配置 | 所有者 |
|---|---|---|---|
| PA03 | GXHT40 SCL | 开漏，R2 4.7 k 上拉至 BAT | 软 I²C 物理层 |
| PA04 | GXHT40 SDA | 开漏，R1 4.7 k 上拉至 BAT | 软 I²C 物理层 |
| PB04 | LIGHT_ADC（AIN11） | 模拟输入 | 仅 `light.c` |
| PB05 | LIGTHT_POWER | 推挽输出；采样高、其余低 | 仅 `light.c` |
| PB02/PB03 | 433 CLK/DATA | 复用，UM2005C | UM2005C 驱动 |
| PA05/PA06 | UART1 RXD/TXD（J3.2/J3.1） | 复用，9600 8N1 | 仅 `debug_trace.c` |
| PA07/PA08 | SWD | 复用，任何关闭路径不得复位 | — |
| PB06 等 | NC | 不配置、不外驱动 | — |

MCU 时钟：内部 HSI（`SYSCTRL_HSIOSC_DIV6` → 8 MHz）。32 MHz 晶振只接 UM2005C。

### 2.2 GXHT40（手册核定）

| 项 | 值 |
|---|---|
| 供电 | 1.6–5.5 V（接 BAT，无稳压） |
| 地址（7 bit） | 0x44 / 0x45（BOM 未定变体 → 运行期探测） |
| 写/读地址字节 | 0x88/0x89、0x8A/0x8B |
| 测量命令 | `0xFD` 高重复率（周期路径只用它） |
| 返回 | 6 B：`T_MSB,T_LSB,T_CRC, RH_MSB,RH_LSB,RH_CRC` |
| tMEAS / tPU | 高重复率典型 6.9 ms、最大 8.3 ms；tPU ≤ 1 ms → 等待 10 ms / 首访 2 ms |
| CRC-8 | poly `0x31`、init `0xFF`、不反转、xorout 0；参考向量 `CRC(0xBEEF)=0x92` |
| 换算 | `T[0.1℃] = -450 + round(1750·S_T/65536)`；`RH[0.1%] = clip(-60 + round(1250·S_RH/65536), 0, 1000)` |
| 不支持 | clock stretching；数据未就绪时对读请求返回 NACK |
| 周期禁用 | 软复位 `0x94`、加热命令 |

### 2.3 光敏分压（不改硬件）

- 拓扑：`PB05(VDD) → R3 5 MΩ → PB04(AIN11) → 光敏电阻 → GND`。
- 12 bit、VDD 参考（比率式）：`code = 4095·R_photo/(R_photo+5 MΩ)`。
- 极性：**无光 ⟺ 光敏阻值大 ⟺ 读数高**。
- 采样窗口：PB05 上电后 ≥100 ms + 8 次转换取成功样本均值 + PB05 回低。
- 判据：`dark = valid && (uint32)3·mean >= C_dark`，即 `mean >= ceil(C_dark/3)`，边界相等为暗；每笔独立、无滞回。

### 2.4 UART1 调试串口

| 项 | 值 |
|---|---|
| 控制器/引脚 | UART1：PA06=TXD（J3.1）、PA05=RXD（J3.2）、J3.3=GND |
| 参数 | 9600 8N1、无流控、PCLK 8 MHz、轮询发送、不用中断 |
| 当前登记入口 | **COM43/9600 8N1**（`runtime/environment.yaml`）；COM26 是调试器串口，不是传感器调试口 |
| 所有权 | UART1 与 PA05/PA06 仅归 `debug_trace.c`；关闭只复位/关闭 UART1 并把两脚置输入，**不得**复位 GPIOA 或关 GPIOA 时钟 |
| 生命周期 | 复位后打印横幅与上电诊断；每采样周期在判定/发送后打印 `S`（必要时 +`G`/`P`）→ 排空 → 关闭；唤醒后重开 |
| 编译开关 | `SENSOR_DEBUG_UART`：目标默认 1，宿主默认 0，可 `-D` 覆盖；关闭时全部空实现 |
| 行预算 | `S` 最坏 92 B；新增/既有诊断行各自 ≤ `SENSOR_DEBUG_UART_MAXLINE=96`（含 CRLF） |

---

## 3. 模块划分与改动面

### 3.1 保持不动

`USER/src/main.c`（保持现有调用顺序与深睡路径，不为 `P` 行新增业务逻辑）、`USER/src/interrupts_cw32l010.c`、`USER/src/encrytogate.c`、`USER/src/light.c`/`light.h`、`UM2005C/*`、`COMMON/*`、标准外设库、网关固件、`MDK`/`EWARM` 工程源文件列表（无新增源文件时不改）。

### 3.2 本版决定的改动面

| 文件 | 动作 | 内容 | 增量 |
|---|---|---|---|
| `USER/src/fw_core.c`、`USER/inc/fw_core.h` | 修改 | `sensor_decide_report` 更正为升温方向 + 严格 >35.0℃（不再整体排除 350）；差值用 int32 | T1 |
| `USER/inc/sensor_config.h` | 修改 | `SENSOR_REPORT_DROP_X10` → `SENSOR_REPORT_RISE_X10`；新增调试发送等待上限常量；保持 `LIGHT_DARK_REF_CODE`/`LIGHT_DARK_CALIBRATED` | T1/T3 |
| `USER/src/measure.c`、`USER/inc/measure.h` | 修改 | 待上报冻结快照 `report_temp_x10`/`report_hum_x10`；`send_data_to_gateway()` 只读快照；新增本周期发送尝试标志供 `P` 行使用 | T2 |
| `USER/src/debug_trace.c`、`USER/inc/debug_trace.h` | 修改 | `P` 行；`TXE`/`TXBUSY` 等待改为有界并可在超时后安全关闭/返回；既有 `BOOT`/`VDD`/`IOTEST`/`IOSIG`/`BUS`/`S`/`G` 字节格式不变 | T2/T3 |
| `test/host_sensor_core_test.c`、`test/host_fw_core_pure_check.c`、`test/host_measure_flow_check.c`、`test/host_debug_trace_check.c`（实现侧自检） | 修改/新增 | 更新判定向量为升温/严格边界；冻结快照重试期不变；`P` 行与有界等待（mock UART 标志永不就绪）；`S` 行冻结格式回归 | T1/T2/T3 |
| `test/build_test.sh` | 修改 | 将上述自检纳入默认阶段 | T1/T2/T3 |

> 测试方自有 `test/*_verify_ev.c`、`test/mock_*_ev/`、`evidence/embedded_verify_*.py` 属验证角色资产，**实现侧不得修改**；其旧判定/快照断言须由验证角色按 IC-002 更新。已知受影响资产：`host_fw_core_verify_ev.c`（按 as-built 降温规则写死的参考哈希与注释）、`host_measure_flow_verify_ev.c`（引用 `SENSOR_REPORT_DROP_X10` 与降温向量）、`host_rf_report_verify_ev.c`（断言 `encode_frame10(mcu_uid, prev, last_hum)` 而非冻结快照）、`host_light_verify_ev.c`（已陈旧、按 bool API 编写，需重写）、诊断行解析/预算类资产（新增 `P` 行与 `BUS/G` 解析扩展）。详见 §14。

### 3.3 已退役（保持移除）

`USER/{src,inc}/{hall,optcfg,params,history}.{c,h}`：新板无对应硬件，保持删除；回归只核查无功能引用、PB04/PB05/PB06 无旧用途。

---

## 4. 数据流

```text
上电复位
  └─ 锁存复位来源 → 系统/RTC/UID 初始化 → BOOT 横幅
     → VDD 供电轨测量 → IOTEST I/O 自检 → IOSIG 位签名 → BUS 地址身份诊断
        （全部只读观测；BUS 只发地址字节、NACK 后释放总线；一次）

[RTC 1 min ×3] → sample_flag=1
      ▼
light_sample()   PB05=1 → ≥100 ms → ADC(AIN11)×8 → PB05=0
                 adc_ok = samples_ok>0
                 valid  = adc_ok && LIGHT_DARK_CALIBRATED
                 dark   = valid && 3*mean >= C_dark
      ▼
gxht40_measure(&t,&h)  probe 0x44/0x45 → 0xFD → 等 ≥10 ms → 读 6B
                       → 双字 CRC → x10/有效域            ── 成功
                       └─ 失败：清缓存、总线恢复、下轮重探；本周期不上报/不推进/不冻结
      ▼ 成功
sensor_decide_report(prev, have_prev, cur, dark)
        report = valid && ((have_prev && int32(cur-prev) > 9 && dark) || cur > 350)
      │
      ├─ report && report_req==0 → 冻结 {temp,hum} → report_req=1     [T2]
      ├─ report && report_req==1 → 保留原冻结样本，不覆盖              [T2]
      ▼
prev=cur; have_prev=true; last_hum=cur_hum（成功即推进；快照不受影响）
      ▼
send_data_to_gateway()  [仅 report_req==1]
        encode_frame10(uid, frozen_temp, frozen_hum) → Feistel → 433 有界发送
        成功 → report_req=0, retry=0 ；失败 → retry++（用尽则放弃，不伪造成功）
      ▼
debug_trace_sample()  S 行 →（失败周期）G 行 →（有上报决定/发送尝试）P 行
      → 有界等 TXE/TXBUSY → 关闭 UART1 → 深睡
```

要点：
1. 光照与温湿度同周期各取一次，判定与轨迹共用同一份结果。
2. 诊断行只携带观测事实（空闲电平/ACK 地址/结果码/原始字节/冻结样本），不参与判定、不进入空口。
3. 冻结快照只在「触发且当前无待上报」时写入；发送路径只读快照。
4. 失败路径的副作用集合为空：不写空口、不推进前值、不新建/覆盖冻结样本、不构造 0 值。

---

## 5. 时序

### 5.1 采样周期（3 分钟）

```text
t=0        RTC 1 min → 深睡
t=60 s     RTC → 深睡
t=120 s    RTC(3) → sample_flag=1
t=120 s    PB05=1 → 稳定 100 ms
t=120.1 s  ADC ×8 ≈3.2 ms → PB05=0
t=120.1 s  GXHT40：0xFD → 等 10 ms → 读 6 B ≈1 ms（失败时有界重试）
t=120.1 s  判定 →（触发且无待上报）冻结
t=120.1 s  433 发射 ≈27 ms（单次上限 200 ms；同一唤醒内最多 3 次尝试）
t=120.1 s  调试：UART1 开 → S（+G/P）→ 有界排空 → 关
t≈120.2 s  深睡 → 下一轮 t=300 s
```

### 5.2 上电一次性诊断

```text
复位 → BOOT（≤约 40 B）
     → VDD（BGR1.2 V 反推供电，超时不伪造）
     → IOTEST（拉低/回读/释放/角色对调探测）
     → IOSIG（一次地址事务的 20 个半位回读签名）
     → BUS（空闲电平 + 0x08..0x77 有界地址扫描，约 ≤100 ms）
     → 首次采样（sample_flag=1）
```

约束：诊断行每次上电只打印一遍；探测只发地址字节，不写器件命令、不读数据；NACK 后必须 STOP 释放总线；不修改 PB04/PB05/PB02/PB03、UART、TIM 配置；不改变采样/判定/上报状态。

### 5.3 节拍精度

RTC 源为 LSI，3 分钟为「1 分钟节拍 ×3」，有 LSI 容差；readme 未给精度要求，验收以 `S` 行 `k` 的差值 3 与相邻样本实际间隔在容差内为准，不把 LSI 偏差当固件缺陷。

---

## 6. 关键算法与格式（实现须逐位一致）

### 6.1 CRC 与换算（保持）

- CRC-8：`poly 0x31, init 0xFF, 无反转, xorout 0`；`fw_crc8_gxht({0xBE,0xEF},2)==0x92`；温度字/湿度字各校验一次，任一失败丢弃整帧。
- CRC16-CCITT-FALSE：`0x1021/init 0xFFFF/无反射/xorout 0`，仅前 8 B。
- 换算必须是整数，不得浮点；温度有效域 `-400..1250`，湿度截断 `0..1000`。

### 6.2 无光判据与全暗基准（判据保持，T5 回填数值）

```c
bool light_is_dark(uint16_t mean_adc_code, bool valid, uint16_t c_dark)
{
    if (!valid) return false;                                   /* 无效不能证明无光 */
    return ((uint32_t)3u * (uint32_t)mean_adc_code) >= (uint32_t)c_dark;
}
```

规则：
1. `valid = adc_ok && LIGHT_DARK_CALIBRATED`；ADC 全部超时 → `valid=false`、`dark=false`，**不得**合成满量程暗态。
2. `C_dark` = 完全遮光、供电/ADC 参考/建立时间/平均策略与量产一致的实板有效均值码（1..4095）；阈值 = `ceil(C_dark/3)`；`uint32` 乘法；边界相等为暗。
3. 每笔独立、无滞回、无历史暗态；不得保留旧 350/250。
4. 未标定时 `valid=false` → 升温分支关闭，超温分支不受影响；不得宣称无光已验收。

标定方法（现场最小必要操作，不设无依据门槛）：

| 步骤 | 内容 | 证据类别 |
|---|---|---|
| 1 | 未标定构建在有光环境观测 `S` 的 `V/o/n/x/a/D` | 串口原始字节 |
| 2 | 用完全不透光遮蔽物包住 J4 光敏器件及其受光路径（不按压器件、不遮挡 SWD 排针），跨过下一次采样机会，并再保持至少一个采样周期确认稳定 | 遮光方式 + 时间戳 |
| 3 | 记录遮光期间各周期 `o/n/x/a`；候选 `C_dark` = 实测全暗有效样本（均值或稳健中位；样本分散时记录 min/max 并说明取值理由） | `S` 行原始字节 |
| 4 | 移开遮蔽，确认 `o/n/x/a` 恢复有光水平 | `S` 行原始字节 |
| 5 | 回填 `LIGHT_DARK_REF_CODE`、置 `LIGHT_DARK_CALIBRATED=1`，重新构建 | 配置版本 + 构建产物摘要 |
| 6 | 标定后三态复测：有光 `V=1 D=0` 且 `mean < ceil(C_dark/3)`；完全遮光 `V=1 D=1` 且 `3·mean >= C_dark`；移开恢复 `D=0` | 串口原始字节 |

标定记录必须含：板件标识（`BOOT` 横幅 UID）、供电（`VDD` 行）、遮光方式、原始样本分布（min/max/mean）、实际取样次数（如实记录，不设门槛）、配置版本与构建摘要。取得 1 笔有效全暗样本即可作为候选值；不得以样本数不足为由延长现场操作或加仪器门槛。

### 6.3 条件上报判定（T1 修正后冻结）

```c
/* valid_sample（温湿度均有效）由调用方保证；light_valid 已折算进 dark
 * （light_is_dark 在 !valid 时返回 false），签名保持 (prev, have_prev, cur, dark)。 */
bool sensor_decide_report(int16_t prev_temp_x10, bool have_prev,
                          int16_t cur_temp_x10, bool dark)
{
    if ((int32_t)cur_temp_x10 > (int32_t)SENSOR_REPORT_HIGH_X10) {
        return true;                              /* 严格 > 35.0℃：不受光照/前值限制 */
    }
    if (!have_prev) return false;                 /* 无前一有效温度：升温分支恒假 */
    if (!dark)      return false;                 /* 需同时无光（含光照无效） */
    return ((int32_t)cur_temp_x10 - (int32_t)prev_temp_x10) > (int32_t)SENSOR_REPORT_RISE_X10;
}
```

- 方向为升温：`本次 − 前次 > 9`（0.1℃ 量化下即 ≥ +1.0℃ 才触发）；恰好 +0.9℃ 不触发；降温恒不触发。
- 差值必须用 int32，防止 int16 边界溢出。
- **不得**再对 `cur == 350` 特判整体排除：恰好 35.0℃ 时超温分支为假，但若同时满足升温分支（如 34.0→35.0、无光）仍须上报。
- 常量更名 `SENSOR_REPORT_RISE_X10`（唯一配置点）。
- 前值语义：`prev` = 上一次**有效采集**温度（含未上报或光照无效的采样）；失败周期不推进；每次成功采样均推进。

IC-002 §2 规范向量（实现与验证必须逐条覆盖）：

| 条件 | 前值 / 本次（℃） | 光照 | 有前值 | 上报 |
|---|---|---|---|---|
| D=+0.8 | 30.0 / 30.8 | 暗 | 是 | 否 |
| D=+0.9 | 30.0 / 30.9 | 暗 | 是 | 否 |
| D=+1.0 | 30.0 / 31.0 | 暗 | 是 | 是 |
| D=+1.0 | 30.0 / 31.0 | 明或无效 | 是 | 否 |
| D=−1.0 | 30.0 / 29.0 | 暗 | 是 | 否 |
| 35.0℃ 升温 | 34.0 / 35.0 | 暗 | 是 | 是 |
| 35.0℃ 升温 | 34.0 / 35.0 | 明 | 是 | 否 |
| 独立超温 | 无 / 35.1 | 明或无效 | 否 | 是 |
| 首笔非超温 | 无 / 35.0 | 暗 | 否 | 否 |

### 6.4 待上报冻结（T2）

```
触发且 report_req==0 → report_temp_x10=cur_temp; report_hum_x10=cur_hum; report_req=1
触发但 report_req==1 → 不改写快照（保留原冻结样本），仅推进 prev（判定用）
组帧：encode_frame10(mcu_uid, report_temp_x10, report_hum_x10, send_data)
成功：report_req=0, report_retry=0
失败：report_retry++；达到 SENSOR_RF_TX_RETRY → report_req=0（放弃本轮，不伪造成功）
```

- 重试期间（含若待上报跨越采样周期）帧内容必须与快照逐字节一致；不受新采样、前值推进、最近有效湿度变化影响。
- 快照仅在「无待上报时的新触发」处被覆盖；不得覆盖尚未发送成功的那一笔。
- 帧布局/字节序/单位不变；不新增光照/原因/时间/序号字段。
- ID 用 `mcu_uid`（常量），无需另存。

### 6.5 GXHT40 失败诊断与修复策略（T4）

诊断数据已实现，验收时直接消费，不重复制作：

| 观测量 | 含义 | 判别用途 |
|---|---|---|
| `BUS idle=` PA03/PA04 空闲电平 | 任何 I²C 事务前的 SCL/SDA | 有 0 位 → 总线被拉低/上拉/短路 |
| `BUS ack=` 地址集合 | 0x08..0x77 中收到 ACK 的 7bit 地址 | `none` → 无器件应答；出现非 0x44/0x45 → 变体/装配需核对 |
| `IOTEST` | 主机拉低/回读、释放、引脚角色对调 | 区分“拉不低/读不到”与“引脚映射错误” |
| `IOSIG` | 一次地址事务的 20 半位回读 | 判别位序列是否真的送到引脚（软件回读级，不是示波器） |
| `VDD` | BGR1.2 V 反推供电 | 排除/确认供电轨异常；不证明 U9 焊点导通 |
| `G s=` | 本轮结果码：1=入参/2=无器件/3=读失败/4=CRC 错/5=量程无效 | 区分失败环节 |
| `G a44= a45=` | 两候选地址本轮是否 ACK | 与 `BUS ack=` 交叉确认 |
| `G rd=` | 最近一次读事务消耗的重读次数 | 读 NACK 频繁 → 未就绪/等待/速率 |
| `G at=` | 本轮整帧重测次数（1..3） | 偶发 vs 持续 |
| `G raw=` | 最近一次成功读回的 6 字节或 12 个 `-` | CRC 错时可独立复算 |

判别与处置（先取得数据再定结论）：

| 现象组合 | 判定环节 | 允许的固件侧动作 |
|---|---|---|
| `idle` 有 0 位 | 总线电气/上拉/短路 | 只做有界恢复；登记硬件处置，不用软件掩盖 |
| `ack=none`、`a44=a45=0`、`idle=3`，且 `IOTEST/ IOSIG/VDD` 正常 | 器件未应答（供电/焊接/器件/装配映射） | 核对引脚映射与上电时序；降速/加余量后复测；仍为 none 则提交硬件判别证据与处置，不得改判据或伪造 |
| `ack` 有 0x44/0x45 但 `s=3`、`rd` 用满 | 读未就绪/等待不足/速率 | 在手册范围内提高等待余量、降低软 I²C 速率；保持 NACK 重读有界 |
| `ack` 有、`s=4`、`raw` 有值 | CRC 或位时序/采样点 | 独立复算 CRC；修位时序/字段顺序，不得放宽校验 |
| `ack` 有、`s=5` | 量程/换算无效 | 核对换算与器件状态；不得放宽有效域 |
| 偶发单周期失败 | 噪声/时序边界 | 保持有界重试；不改变判定/上报语义 |

修复边界：
1. 保留双字 CRC-8、整数换算与有效域、有界重读/重测；不得为通过而放宽或绕过校验。
2. 周期路径不得使用软复位 `0x94` 或加热命令；不得每周期复位从机。
3. 任一失败周期：不上报、不推进前值、不构造 0 值、不冻结快照。
4. 任何失败后下一周期必须重探两候选地址，并在重探前做有界总线恢复（已实现，保持）。
5. 地址/速率/等待只能通过 `sensor_config.h` 常量调整，不散落魔法数。
6. 修复必须以**当前构建的实板一手数据**为依据；不得用宿主 mock 或历史结论冒充。

### 6.6 调试轨迹格式

**`S` 行（15 字段，字节冻结，保持 as-built）**：

```text
S k=<tick> p=<prev> H=<have> V=<lvalid> o=<ok> n=<min> x=<max> a=<mean>
  D=<dark> t=<temp> h=<hum> q=<sample_ok> r=<report> s=<send> y=<retry>
```

| 字段 | 含义 |
|---|---|
| `k` | RTC 累计 1 分钟节拍（△k=3 即一次采样） |
| `p` / `H` | 判定用前一有效温度 x10 / 是否存在前值 |
| `V` | 光照有效性 `adc_ok && LIGHT_DARK_CALIBRATED` |
| `o` / `n` / `x` / `a` | 成功样本数 / 最小 / 最大 / 均值 ADC 码（`o=0` 时均为 0） |
| `D` | 无光判定结果 |
| `t` / `h` | 本周期温度 x10（有符号）/ 湿度 x10；失败周期保持最近有效值并由 `q=0` 标记 |
| `q` | 本周期温湿度采样是否有效 |
| `r` | 本周期条件上报判定结果 |
| `s` | 本行打印时最近一次 433 发送结果：0 未发送 / 1 成功 / 2 失败（首次尝试失败仅为中间态，见 `P` 行约束 7） |
| `y` | 待上报重试计数 |

最坏 92 B ≤ 96 B。字段集合/顺序/语义不得改变（既有验证脚本按此解析）。

**既有诊断行（保持字节格式）**：

```text
VDD ok=<0|1> code=<n> bgrmv=<n> mv=<n>
IOTEST sda_lo=<0|1> scl_lo=<0|1> idle=<0..3> swap=<addr7[,addr7]|none> gc=<n>
IOSIG scl=<5 hex> sda=<5 hex>
BUS idle=<0..3> scl=<0|1> sda=<0|1> ack=<addr7[,addr7...]|none[+]>
G s=<1..5> a44=<0|1> a45=<0|1> rd=<0..5> at=<1..3> raw=<12 hex | ------------>
```

**新增 `P` 行（T2）**：

```text
P rq=<0|1> s=<0|1|2> y=<0..3> t=<frozen temp x10> h=<frozen hum x10>
```

| 字段 | 含义 |
|---|---|
| `rq` | 本条 `P` 行打印时是否仍待上报（1=本轮未结束/仍待发，0=已清除或已放弃） |
| `s` / `y` | 打印时最近一次发送尝试的结果 / 待上报重试计数（同 `S` 行语义） |
| `t` / `h` | 组帧实际使用的冻结快照（有符号温度 x10 / 湿度 x10） |

打印时机与约束：
1. `P` 行仅在本周期「产生上报判定（`r=1`）」或「发生过至少一次 433 发送尝试」时打印，紧跟 `S` 行（若同时有 `G`，在 `G` 之后）。
2. 打印位于判定与发送之后、深睡之前；打印不得改变 `report_req`/前值/冻结快照/发送结果。
3. 成功且未上报周期不得新增任何字节；`G`/`P` 只在相应条件周期出现；采样间期无任何字节。
4. 仅整数/十六进制格式化，无 `%f`/浮点、无动态内存；每行 ≤96 B。
5. 诊断信息不进入 433/BLE/平台任何载荷。
6. `SENSOR_DEBUG_UART=0` 时全部诊断为空实现。
7. `P` 行每采样周期最多一条，在首次发送尝试后打印。若首次尝试失败，`rq=1`/`s=2`/`y≥1` 只表示本轮尚未结束；同一唤醒内的后续重试不再重复打印，不得把该中间态当作最终失败。最终是否送达以 `report_req` 是否清除、宿主序列结果与手机实收判断。

**`BOOT` 行保持 as-built**（含 `fw=FD-002r4` 字符串与 `uart=9600`）。该字符串是既有字节冻结标识，已被验证脚本解析；本版不因文档版本变化改动它，避免无业务收益的解析破坏。

### 6.7 调试发送等待有界化（T3）

```c
static bool trace_wait_flag(uint32_t flag, FlagStatus want)
{
    uint32_t guard = SENSOR_DEBUG_TX_GUARD;      /* 单一配置点, 按 9600 8N1 行长推算并留余量 */
    while (UART_GetFlagStatus(DEBUG_UARTx, flag) != want) {
        if (guard-- == 0u) return false;         /* 超时: 放弃本行剩余输出 */
        /* 允许编译为空体 */
    }
    return true;
}
```

- `trace_putc()`：等待 `TXE` 有界；超时返回失败，`trace_*` 各行提前结束（可能截断该行），但不得死等。
- `debug_trace_flush_close()`：等待 `TXE` 与 `TXBUSY` 各有界；超时仍执行关闭流程（复位/关闭 UART1、PA05/PA06 置输入），保证下一次采样/深睡继续。
- 超时不得修改任何业务状态（`report_req`/前值/快照/发送结果），不得触发复位或关 GPIOA。
- 正常硬件路径输出与既有字节格式逐字节相同（用现有格式化自检回归）。
- 上限常量放 `sensor_config.h`，依据：9600 bps 下每字符约 1.04 ms；单行 ≤96 B 约 100 ms；上限须显著大于正常排空时间且保持有限。

---

## 7. 资源预算

| 项 | 预算/现状 | 说明 |
|---|---|---|
| Flash | 64 KB 可用；历史交付件 Code 20,100 B（总 Flash 20,744 B）；本版预计增量：判定/冻结/`P` 行/有界等待 < 1 KB | 以实际构建为准，不把历史尺寸当本轮构建结果 |
| RAM | 4 KB 可用；历史 ZI 1,676 + RW 116 = 1,792 B；本版新增冻结快照 4 B + `P` 取数标志约 2 B | 在余量内 |
| 动态内存/浮点 | 无 | 换算/判据/格式化均整数 |
| 单周期活动时间 | 不上报 ≈113 ms；上报 ≈140 ms；`SENSOR_DEBUG_UART=1` 另加 ≤100 ms（S）+ ≤100 ms（G/P）；433 重试至上限另加 ≤400 ms | 最坏仍 <1 s，3 分钟占空比 <0.6% |
| 上电诊断 | VDD/IOTEST/IOSIG/BUS ≈100–150 ms（一次性） | 仅调试开启 |
| 深睡 | MCU 深睡 + GXHT40 待机 + PB05 低 + UART 关闭 + ADC 关闭 | 实际电流/CR2032 寿命须实测，本文不判通过 |

功耗与寿命为设计估算，必须由低功耗/嵌入式实测回填；当前无电流表/示波器授权入口，不伪造数值。

---

## 8. 接口处理（不改变）

### 8.1 传感器 → 网关（433 MHz）

| 偏移 | 长度 | 字段 | 编码 |
|---:|---:|---|---|
| 0 | 4 | `sensor_id` | `mcu_uid[0..3]` 原字节 |
| 4 | 2 | `temperature_x10` | int16 二补码，**小端** |
| 6 | 2 | `humidity_x10` | uint16，**小端** |
| 8 | 2 | `crc16` | 对前 8 B 计算 CRC16-CCITT-FALSE，**小端** |

兼容向量：ID=`01 02 03 04`、T=−5.0℃、RH=60.0% → 前 8 B = `01 02 03 04 CE FF 58 02`。不新增字段，不新增 ACK/序号/采样时间/光照/原因。

### 8.2 网关 → 手机（BLE）

每设备前 8 B：`id(4) | humidity_x10 大端(2) | temperature_x10 大端(2)`；网关 `decode_frame10` 取 `temp=p[4..5]`、`hum=p[6..7]`；Android 按 `u16be@4`/`s16be@6` 解析。本版不改网关与 Android；用「同一笔上报的传感器 `t/h` 与手机显示一致」做跨链路对照。

---

## 9. 网关固件影响评估（结论：无功能改动）

依据 IC-002 §4/§5 与既有源码（`APP/feistel_al.c:decode_frame10()`、`APP/app_um2006A.c` 记录组装、`APP/bleencrypt.c:build_device_block()`）：帧布局、字节序、记录宽度与本设计 §8 一致；GXHT40 替换与光照判据不改变单位、帧长或字段。网关**不得**：新增光照/原因字段、改变服务/特征/记录宽度、按自身温度历史二次过滤。实现阶段只做一次只读核对；如发现不一致，登记接口交接，不在固件域改网关。

---

## 10. 异常策略

| 场景 | 行为 |
|---|---|
| GXHT40 无器件/无地址 ACK | 两候选依次探测；本周期失败 → `G` 行记录结果码/ACK/原始字节 → 不上报/不推进前值/不构造 0 值；下一周期重探 + 有界恢复 |
| 读请求 NACK（未就绪） | 有界重读 ≤5（每次等 1 ms），次数入 `G rd=` |
| CRC 错 | 丢弃整帧，有界整帧重测 ≤3，`G raw=` 保留原始字节供独立复算 |
| 温度超有效域 | 视为无效测量（失败路径），不进入上报 |
| 总线被从机拉低卡死 | 有界恢复（9 个 SCL 脉冲 + STOP）后重探；仍失败按失败处理；不无限等待 |
| 缓存地址读失败 | 清缓存并重探两地址；不得永久锁定 |
| 光照 ADC 全超时 | `valid=false`、`dark=false`；不合成满量程；升温分支关闭 |
| 光照部分成功 | 均值取成功样本，记录 `o`；仍按 1/3 判据 |
| 未标定 | `valid=false`、`dark=false`；升温分支不触发；超温分支独立 |
| 有光与阈值分离不足 | 如实记录原始数值，作为阈值/器件问题交接；不得改判据或期望 |
| 433 发送超时 | 有界重试 ≤3；重试期间帧内容 = 冻结快照；用尽则放弃本轮，不伪造成功 |
| 待上报未清除期间再次触发 | 保留原冻结样本，不覆盖 |
| 调试 UART 标志永不就绪 | 有界等待超时 → 放弃本行剩余输出/执行关闭流程；业务状态不变；采样与深睡继续 |
| 深睡唤醒后 UART1 未就绪 | 打印前重新打开；关闭只碰 UART1/PA05/PA06 |
| 无 USB-TTL 接线 | 打印有界（超时返回）；`SENSOR_DEBUG_UART=0` 可整体关闭；任何情况不改变业务结果 |

---

## 11. 实现约束

1. **单一配置点**：引脚、阈值、等待、重试、扫描范围、调试发送上限集中在 `sensor_config.h`，不散落魔法数。
2. **整数运算**：判定、换算、1/3 判据均为整数；`uint32` 乘法，不得先整除截断。
3. **不改冻结接口**：`S` 行字段集合/顺序/语义；既有上电诊断行字节格式；433/BLE 布局；GXHT40 API 失败不写输出语义。
4. **诊断只读**：所有诊断行只格式化观测事实，不参与判定、不改变状态、不进入空口。
5. **失败无副作用**：失败路径不写空口、不推进前值、不覆盖/新建冻结快照。
6. **有界性**：地址扫描、总线恢复、读重读、整帧重测、433 重试、UART 等待全部有上限。
7. **不伪造**：不得为通过而放宽 CRC/量程/判据/标定；不得用 mock、历史日志、估算值或满量程/亮态读数冒充实板结果。
8. **引脚所有权唯一**：PB04/PB05 仅 `light.c`；PA03/PA04 仅 I²C 物理层；PB02/PB03 仅 433；PA05/PA06 与 UART1 仅 `debug_trace.c`；PA07/PA08 任何路径不得改动。
9. **串口关闭语义**：只复位/关闭 UART1 与改写 PA05/PA06；绝不复位 GPIOA/关 GPIOA 时钟；关闭前排空（有界）。
10. **幂等初始化**：`bsp_i2c_init()` 幂等，供上电诊断与采样共用。
11. **可测试性**：判定、光照判据、换算、CRC 保持纯函数（无寄存器依赖）。
12. **工程一致性**：MDK 与 IAR 工程、`gcc/build.sh` 均须通过；新增/改名源文件同步工程列表。
13. **不恢复读保护**：保持 `main.c` 中 Flash 读保护两行仍为注释。
14. **不扩张范围**：不实现 Android/iOS/网关/硬件改动；不引入 RTOS、NVM、动态内存；不改 `BOOT` 行的 `fw=` 字符串。

---

## 12. 实现增量划分（任务清单来源）

任务按先决顺序组织；「文件编辑/配置/构建」是增量内部步骤，不单列任务。

| 增量 | 预期行为（可观察） | 前置 |
|---|---|---|
| **T1 条件上报判定更正** | 升温 `cur-prev>9` 且 `dark`，或 `cur>350`；+0.9℃ 不报、+1.0℃ 报、降温不报、恰好 35.0℃ 可经升温分支上报；无前值仅超温；光照无效只关升温分支；IC-002 九组向量逐条一致；宿主机自检全过 | 无（纯代码/宿主机） |
| **T2 待上报冻结与 `P` 行** | 触发时冻结 `{temp,hum}`，组帧只读快照；重试/跨采样期间帧逐字节不变；新触发不覆盖原待上报；仅成功清除，用尽放弃不伪造；布局不变；`P` 行显示冻结样本与 `rq/s/y`，≤96 B | T1（判定置位语义确定） |
| **T3 调试发送等待有界化** | 正常输出逐字节不变；mock `TXE`/`TXBUSY` 永不就绪时函数在有限循环内返回并完成关闭；采样/判定/发送/深睡不受影响；无无界 `while` 等待 | 无（可与 T1 并行） |
| **T4 GXHT40 实板采集恢复** | 当前构建下载后：每个 3 分钟周期 `q=1`、`t/h` 在有效域且与环境相符、连续多周期稳定；失败周期仍不上报/不推进/不构造 0；失败后重探两地址 + 有界恢复；判别证据（BUS/IOTEST/IOSIG/VDD/G）入库；不得放宽 CRC/量程 | T1–T3（候选身份）；实板观测与下载授权 |
| **T5 全暗基准标定与 1/3 生效** | 未标定门禁保持；取得完全遮光原始样本并记录；回填 `C_dark`+标志后重建；有光 `V=1 D=0` 且 `mean<ceil(C/3)`；遮光 `V=1 D=1` 且 `3·mean>=C`；移开恢复 `D=0`；ADC 全超时不成暗 | 无功能前置（建议与 T1–T3 同一候选构建；与 T4 独立）；完全遮光条件与实板观测 |

T4 与 T5 相互独立：若实板/遮光窗口安排不同，可先做已具备条件的一项；不得以另一项未完成拒绝执行。

---

## 13. 当前构建的验证与再观测要求

历史证据只作背景；下列观测必须由具备设备工具的责任方在**当前交付构建**上重新取得原始证据（调试入口 **COM43/9600 8N1**，不得沿用历史 COM42）：

| 观测点 | 方法（现有资源） | 通过判定 |
|---|---|---|
| 构建/下载身份 | Keil 重建 + 下载校验（须见 Verify OK）；记录产物摘要 | 0 Error；下载校验成功；证据绑定提交与构建摘要 |
| 横幅与复位来源 | COM43 复位捕获 | `BOOT fw=… uid=… rst=非零且随复位原因变化 uart=9600` |
| 总线身份 | 上电捕获 `VDD/IOTEST/IOSIG/BUS` | 各行字段与本板实际自洽；`BUS ack=` 与 `G a44/a45` 不矛盾 |
| 温湿度采集 | 每周期 `S` + 失败周期 `G` | `q=1`、`t/h` 有效域内且与环境相符；失败周期 `G` 给出可判别的一手数据 |
| 光照标定 | 有光/完全遮光/恢复三态 + `o/n/x/a` | 未标定 `V=0`；标定后 `V=1`、三态 `D` 正确、有光 `mean<ceil(C/3)` |
| 判定与上报 | `S` 行 `r/s/y` + `P` 行 + 手机端读数 | 判据与九组向量一致；上报周期首次尝试成功时 `s=1`（首次失败按 §6.6-7 解释为中间态）；手机显示与该周期 `t/h`/冻结 `P` 样本一致 |
| 3 分钟节拍 | `S` 行 `k` 差值 | △k=3；相邻样本实际间隔≈180 s（LSI 容差内） |
| 采样间期零输出 | 长时串口捕获 | 采样间期无任何字节（UART1 已关闭） |
| 关闭语义不破坏软 I²C/SWD | 连续多周期 `o=8`/`q=1` 且可再次下载 | 每周期软 I²C 与再次下载均正常 |
| 退役/引脚所有权 | 源码静态核查 + 运行观测 | 无 hall/optcfg 残留功能引用；PB04/PB05/PB06 未被其它模块配置 |
| Flash 读保护 | 源码静态核查 | 两行仍为注释 |

方法边界（不夸大证据力）：
- UART 轨迹只能证明软件可见状态与串口可观测行为；I²C 位级电气/时序、433 空口波形、供电电流与 CR2032 寿命需逻辑分析仪/电流表/示波器。逻辑分析仪在本能力不可用；下游若启用，按登记入口 `fx2lafw:conn=2.31`、D0=SCL/D1=SDA、共地，重插后需更新地址配置。
- 数字波形不能证明电平兼容或上拉质量；`IOSIG` 是软件回读，不是位时序测量。
- 手机端一致性需要真实 BLE/RF 链路；本能力不安排也不声称已通过。

---

## 14. 交接、未决项与停止条件

| 项 | 内容 | 归属 |
|---|---|---|
| T4 一手数据 | 当前构建 COM43 的 `VDD/IOTEST/IOSIG/BUS/S/G` 原始字节、构建摘要、下载校验记录，按周期入库 | 嵌入式测试取证 → 固件实现定位/修复 |
| T5 标定数据 | 完全遮光 `S` 行原始字节 + 板件/供电/遮光方式/样本分布/取样次数/配置版本 | 嵌入式测试取证 → 固件实现回填 |
| 验证资产同步（跨职责） | 验证方自有资产仍按旧语义断言，必须由验证角色按 IC-002 更新，实现侧不得修改：`test/host_fw_core_verify_ev.c`（降温规则的参考哈希/注释）、`test/host_measure_flow_verify_ev.c`（`SENSOR_REPORT_DROP_X10` 与降温向量）、`test/host_rf_report_verify_ev.c`（组帧入参为 prev/last_hum 而非冻结快照）、`test/host_light_verify_ev.c`（已陈旧、bool API，需重写）、`test/host_diag_line_verify_ev.c` 与 `test/host_diag_probe_verify_ev.c`（新增 `P` 行与诊断解析/预算）、`evidence/embedded_verify_*.py`（S/G/P 行解析与判定扩展） | 验证角色 |
| 历史入口 | `evidence/test.md` 的 COM42、逻辑分析仪 2.19 均为历史记录；本轮只用 COM43 与当前配置 `2.31` | 全体 |
| 硬件候选原因 | `U9` 焊点连通性/器件本体、上拉/装配映射等需要硬件判别；固件不据此宣告不可修复，也不据此伪造 `q=1` | 硬件/固件联合，证据驱动 |
| 网关/Android | 布局不变；Android 的升温文案与多设备归因属其自身职责 | 对应职责 |
| 服务端 | 复用既有部署；本能力不涉及 | 对应职责 |
| 未决项 | `C_dark` 实测值（T5）；有光读数与阈值的分离是否足够（T5 实测判定）；U9 无应答的最终原因（T4 收敛）；CR2032 寿命/睡眠电流/433 峰值电流（实测回填）；`FWR-OPEN-2` 0.9℃ 是否含等于（已按 IC-002 严格解释冻结） | 见上 |

**停止条件**：本能力在 `artifacts/firmware_design.md`（rev 6.0）、`artifacts/firmware_requirements.md`（rev 6.0）、`artifacts/firmware_rtos_architecture.md`（rev 6.0）、`artifacts/firmware_tasks.yaml`（T1–T5 结构化任务）已写入并同步 `file_manifest.txt` 后返回 `DESIGN_READY`。代码实现、构建、下载、实板诊断/标定、网关联调与 Android 改动均不在本能力内；`DESIGN_READY` 不代表产品功能通过。
