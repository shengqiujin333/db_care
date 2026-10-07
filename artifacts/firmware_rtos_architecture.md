# 固件运行时与任务架构（RTA-002）

状态：固件方案设计的伴随架构视图（firmware_engineer.firmware_solution_design）
版本：2.0（与 FD-002 rev 2.0 同步重发）
范围：本次需求涉及的传感器（CW32L010Y8M6，裸机）与网关（CH592，TMOS）固件运行时结构。不含 Android/iOS/服务器。
结论先行：**传感器固件保持既有裸机事件驱动（超级循环 + RTC 唤醒深睡）架构，不引入 RTOS**；网关固件保持既有 TMOS 任务模型，本次**无功能变更**。

---

## 1. 传感器运行时结构（CW32L010Y8M6，Cortex-M0+，64 KB Flash / 4 KB SRAM）

### 1.1 调度模型

单线程超级循环 + 中断置标志；除 433 位时钟外不在 ISR 内做业务。状态在深睡期间保留（`SYSCTL_GotoDeepSleep()` = `SLEEPDEEP + WFI`，SRAM/寄存器保持，非复位）。

```text
RTC 1 分钟中断 ──► rtc_tick_cnt++ ; 到 3 → sample_flag=1
                        │
主循环 while(1) ────────┘
  sample_flag? → [T_LIGHT] PB05 高 → 稳定延时 → PB04/AIN11 多次采样 → DARK/LIT
              → [T_MEAS ] GXHT40 0xFD → 等待 tMEAS → 读 6B → CRC → x10
              → [T_DECIDE] report = (下降>0.9℃ 且 DARK) 或 (T>35.0℃)
              → [T_REPORT] 需要时 编码+Feistel+433 有界发送
              → [T_SLEEP ] 无挂起工作 → 深睡
```

### 1.2 逻辑任务表

| 任务 | 触发 | 执行体 | 预计最坏执行时间 | 优先级/约束 |
|---|---|---|---|---|
| T_TICK | RTC 1 min 中断 | `RTC_IRQHandlerCallBack`：计数、置 `sample_flag` | 数 µs | ISR，最高；只置标志 |
| T_LIGHT | 主循环（sample_flag） | PB05 高 → 稳定等待 → ADC 采样 N 次 | 稳定等待 ~100 ms + 采样 ~3 ms | 主循环，先于测温（光敏需上电稳定） |
| T_MEAS | 主循环 | GXHT40 命令/读取/CRC/换算，失败有界重试 | 命令+等待 8.3 ms + 读 6B ~1 ms；重试 ×3 | 主循环；不可与 T_LIGHT 的 PB05 高电平冲突（互不影响，不同引脚） |
| T_DECIDE | 主循环 | 纯函数判定（可宿主机测试） | < 1 µs | 无阻塞、无浮点 |
| T_REPORT | 主循环（report_req） | `encode_frame10` + Feistel + UM2005C 发送 | 帧长 34 B @10 kbps ≈ 27 ms；超时上限 200 ms | 主循环；发射期间 CPU 参与位时钟 |
| T_SLEEP | 主循环末尾 | `SYSCTRL_GotoDeepSleep()` | — | 无挂起工作时立即进入 |

### 1.3 中断边界与优先级

| 中断 | 用途 | 边界约束 |
|---|---|---|
| `RTC_IRQHandler` | 3 分钟采样节拍 | 仅计数 + 置 `sample_flag`，不做 I²C/ADC/发射 |
| `LPTIM_IRQHandler` | 433 位时钟（`app_gtimer_count_irq`） | 仅在发射期间使能；**移除**原 OPTCFG 采样分支 |
| `GPIOB_IRQHandler` | 原霍尔 EXTI | **本次移除**（PB04 改为模拟输入，不再产生 EXTI） |
| `ADC_IRQHandler` | 未使用 | 光照采样走查询模式（`ADC_Isr` 不使能），避免与 433 位时钟争用 |
| `UART1_IRQHandler` | 未使用 | 调试 UART 仅轮询输出，用后关闭 |

中断嵌套：RTC/LPTIM 均为默认优先级、不嵌套；发射期间 433 位时钟优先，业务在主循环。

### 1.4 状态机（测量周期，主循环内）

```text
IDLE(深睡) --RTC×3--> SAMP_REQUESTED
SAMP_REQUESTED --T_LIGHT 完成--> LIGHT_DONE
LIGHT_DONE --T_MEAS 成功--> MEAS_OK --T_DECIDE--> {REPORT | IDLE}
LIGHT_DONE --T_MEAS 失败(重试用尽)--> MEAS_FAIL --(不更新前值/不上报)--> IDLE
MEAS_OK/REPORT --T_REPORT 成功或放弃--> IDLE
```

## 2. 网关运行时结构（CH592，TMOS）

- 保持既有结构：RF 接收（UM2006A + TMR2 捕获 `app_ev1527`）→ `decode_frame10` 解密校验 → 绑定表归集 → `build_padded_frame_blocks` → AES-128-ECB → BLE `SimpleProfile_SetParameter(CHAR2, FFE2)`。
- 本次**不改任务划分、不改优先级、不改特征值长度**。
- 约束：网关不得依据自身温度历史对传感器上报做二次门控（门控已在传感器完成，IC-002 §3）。
- 遗留的板上 AHT20 TMOS 测量任务与本产品功能无关，保持不动（不在本次范围）。

## 3. 资源预算

### 3.1 传感器 MCU 存储

| 项 | 预算 | 依据 |
|---|---|---|
| Flash | 64 KB 可用；本次相对基线为**净减少**（移除 OPTCFG/参数 NVM/历史环，新增 GXHT40/light 驱动与配置头） | MDK `IROM(0,0x10000)`、`gcc/cw32l010.ld` |
| SRAM | 4 KB 可用；本次为净减少（移除 20×4 B 历史环、参数模型、OPTCFG 解码缓冲 35 B+；新增前值 2 B、光照均值/滑窗 < 16 B） | MDK `IRAM(0x20000000,0x01000)` |
| 栈/堆 | 无动态分配；软件 I²C 无缓冲拷贝；不得裸拷贝结构体到空口 | 实现约束 |

### 3.2 时间预算（单个 3 分钟周期，典型）

| 阶段 | 时间 |
|---|---|
| PB05 上电稳定等待 | ~100 ms（覆盖 R3=5 MΩ 与 ADC 采样电容的 RC 建立及光敏器件响应） |
| 光照 ADC（N=8，1 MHz ADC，390 clk 采样） | ~3.2 ms |
| GXHT40 命令+等待+读取（高重复率） | ~9.3 ms |
| 判定 | < 0.01 ms |
| 433 发射（仅满足判据时） | ~27 ms（上限 200 ms 有界） |
| **合计（不上报 / 上报）** | **≈ 113 ms / ≈ 140 ms** |

### 3.3 功耗预算（估算，须实测确认）

| 项 | 估算 | 假设 |
|---|---|---|
| MCU 深睡（含 LSI+RTC） | ~1.5 µA | 待实测（低功耗能力/硬件测试） |
| GXHT40 待机 | 0.1 µA（手册） | 手册 §1 |
| GXHT40 测量 | 0.55 µAh/天 | 500 µA avg × 8.3 ms × 480 次 |
| 分压电流 | < 0.02 µAh/天 | 仅采样 103 ms 内 ~0.6 µA |
| MCU 活动 | ~46 µAh/天 | 113 ms × 3 mA × 480 次 |
| 433 发射 | ~0.17 µAh/次 | 27 ms × 20 mA；按每天 20 次 ≈ 3.3 µAh |
| **合计** | **≈ 100 µAh/天（≈ 36 mAh/年）** | CR2032 可用容量与峰值电流约束另行评估 |

> 该预算是固件设计假设，不是实测证据；`radio_sleep()` 后 UM2005C 实际静态电流、MCU 深睡电流必须由低功耗/嵌入式测试实测回填。

## 4. 不变量

1. 单次采样周期内温湿度测量最多执行一次（避免 I²C/ADC 重复唤醒）。
2. 任何失败路径都不得写空口、不得推进「前值」、不得伪造 0 值。
3. 深睡是默认态；只有 `sample_flag`、`report_req` 或发射挂起时才保持唤醒。
4. 433 发射必须有界超时，失败不阻塞下一次 3 分钟节拍。
