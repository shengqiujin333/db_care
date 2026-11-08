# 固件运行时与任务架构（RTA-002）

负责角色：固件工程
版本：6.0（替代 rev 5.0）
基线：Git `491853165f63713a1dc4df108034859ac9afb6e6`（2026-11-08）
范围：本次需求涉及的传感器（CW32L010Y8M6，裸机）与网关（CH592，TMOS）固件运行时结构。不含 Android/iOS/服务器。

结论先行：**传感器保持既有裸机事件驱动（超级循环 + RTC 唤醒深睡）架构，不引入 RTOS**；网关保持既有 TMOS 任务模型，本次**无功能变更**。本版按当前源码更新 as-built 任务（上电四类诊断行已实现、`P` 行/冻结快照/有界等待待 T2/T3 补齐、判定待 T1 修正），不改变调度模型。

---

## 1. 传感器运行时结构（Cortex-M0+，64 KB Flash / 4 KB SRAM）

### 1.1 调度模型

单线程超级循环 + 中断置标志；除 433 位时钟外不在 ISR 内做业务。状态在深睡期间保留（`SYSCTRL_GotoDeepSleep()` = `SLEEPDEEP + WFI`，SRAM/寄存器保持，非复位）。

```text
上电 ──► 锁存复位来源 → 系统/RTC/UID 初始化 → BOOT 横幅
       → VDD 供电轨测量 → IOTEST I/O 自检 → IOSIG 位签名 → BUS 地址身份诊断
         （均只读，SENSOR_DEBUG_UART=1 时；一次）

RTC 1 分钟中断 ──► rtc_tick_cnt++ ; 到 3 → sample_flag=1
                        │
主循环 while(1) ────────┘
  sample_flag? → [T_LIGHT ] PB05 高 → 稳定 → PB04/AIN11 多次采样 → {valid,mean,dark}
              → [T_MEAS  ] GXHT40 0xFD → 等待 tMEAS → 读 6B → 双字 CRC → x10
                          失败：清缓存/总线恢复/下轮重探 + 诊断快照
              → [T_ADVANCE] 成功才推进前值/最近湿度（含未上报或光照无效）
              → [T_DECIDE ] report = ((cur-prev)>0.9℃ 且 无光) 或 (T>35.0℃)
              → [T_LATCH  ] 触发且无待上报时冻结当次 {temp,hum}
              → [T_REPORT ] 有待上报才 编码+Feistel+433 有界发送（只读冻结快照）
              → [T_TRACE  ] S 行 →（失败周期）G 行 →（有上报决定/发送尝试）P 行
                            → 有界等 TXE/TXBUSY → 关闭 UART1
              → [T_SLEEP  ] 无挂起工作 → 深睡
```

顺序要点：**先光照后温湿度**（光敏需 PB05 上电稳定）；判定使用同一周期内各自结果；诊断行位于业务之后，只读观测。

### 1.2 逻辑任务表（as-built + 本版增量）

| 任务 | 触发 | 执行体 | 最坏执行时间 | 约束 |
|---|---|---|---|---|
| T_TICK | RTC 1 min ISR | 计数、置 `sample_flag` | 数 µs | ISR 最高；只置标志 |
| T_DIAG_VDD | 上电一次 | `light_read_vdd_mv()` + `VDD` 行 | ~1 ms + 行时间 | 只读；超时不伪造 |
| T_DIAG_IO | 上电一次 | `sensor_io_diag_scan()`（拉低/回读/释放/角色对调）+ `IOTEST` 行 | ~2 ms | 只读；不动其它引脚 |
| T_DIAG_SIG | 上电一次 | `sensor_io_sig_scan()`（20 半位回读）+ `IOSIG` 行 | ~1 ms | 只读；软件回读非示波器 |
| T_DIAG_BUS | 上电一次 | `sensor_bus_diag_scan()`：空闲电平 + 0x08..0x77 地址探测 + `BUS` 行 | ≈90–100 ms | 只发地址字节；NACK 后 STOP；不写命令/不读数据 |
| T_LIGHT | 主循环（sample_flag） | PB05 高 → 100 ms → ADC ×8 → PB05 低 → 1/3 判据 | ~103 ms | 主循环；先于测温 |
| T_MEAS | 主循环 | GXHT40 命令/等待/读取/双 CRC/换算；失败有界重试、清缓存、下轮重探 | ~9.3 ms（失败最多约 3×） | 与 T_LIGHT 引脚不冲突 |
| T_ADVANCE | 主循环 | 成功推进前值/最近湿度 | <1 µs | 失败不推进 |
| T_DECIDE | 主循环 | 纯函数判定（int32 差值） | <1 µs | 无阻塞/无浮点 |
| T_LATCH | 主循环 | 触发且无待上报时冻结 `{temp,hum}` | <1 µs | 不覆盖未发送成功的样本（T2） |
| T_REPORT | 主循环（待上报） | `encode_frame10`（只读快照）+ Feistel + UM2005C 发送 | ≈27 ms；单次上限 200 ms，≤3 次 | 成功才清除；用尽放弃 |
| T_TRACE | 主循环（周期末） | UART1 开 → `S` →（失败）`G` →（有上报决定/发送尝试）`P` → 有界排空 → 关 | S ≤100 ms；G/P 各 ≤100 ms | 有界等待（T3）；不改变业务状态 |
| T_SLEEP | 主循环末尾 | `SYSCTL_GotoDeepSleep()` | — | 无 `sample_flag` 且无待上报时进入 |

### 1.3 中断边界与优先级

| 中断 | 用途 | 边界约束 |
|---|---|---|
| `RTC_IRQHandler` | 3 分钟节拍 | 仅计数 + 置 `sample_flag`，不做 I²C/ADC/发射 |
| `LPTIM_IRQHandler` | 433 位时钟 | 仅在发射期间使能；无旧 OPTCFG 分支 |
| `GPIOB_IRQHandler` | 原霍尔 EXTI | **已移除**（PB04 为模拟输入，不产生 EXTI） |
| `ADC_IRQHandler` | 未使用 | 光照/VDD 走查询模式（有界 EOC 轮询） |
| `UART1_IRQHandler` | 未使用 | 调试 UART 仅轮询；等待有界；打印后关闭；唤醒后重开；`debug_trace` 是 PA05/PA06/UART1 唯一所有者 |

中断嵌套：RTC/LPTIM 默认优先级、不嵌套；发射期间 433 位时钟优先，业务在主循环。

### 1.4 测量周期状态机

```text
IDLE(深睡) --RTC×3--> SAMP_REQUESTED
SAMP_REQUESTED --T_LIGHT--> LIGHT_DONE {valid,mean,dark}   (ADC 全超时 => valid=false,dark=false)
LIGHT_DONE --T_MEAS 成功--> MEAS_OK --T_ADVANCE--> prev=cur --T_DECIDE--> {LATCHED->REPORT | IDLE}
LIGHT_DONE --T_MEAS 失败(重试用尽)--> MEAS_FAIL(诊断快照) --(不推进/不上报/不冻结)--> TRACE_SG -> IDLE
REPORT --T_REPORT 成功--> IDLE ; --3 次用尽--> DISCARD(不放行假成功) --> IDLE
```

上电一次：`BOOT -> VDD -> IOTEST -> IOSIG -> BUS -> (首采样)`。

### 1.5 深睡保留状态

| 变量 | 大小 | 语义 |
|---|---|---|
| `tempvalue` / `huminityvalue` | 2+2 B | 最近有效温湿度 |
| `s_prev_temp_x10` / `s_have_prev` | 2+1 B | 前一**有效**采样温度与存在标志（跨失败周期保留） |
| `s_last_hum_x10` | 2 B | 最近有效湿度 |
| `report_req` + `report_temp_x10` + `report_hum_x10` | 1+2+2 B | 待上报标志与**冻结样本快照**（T2） |
| `report_retry` | 1 B | 本轮发送重试计数 |
| `s_last_send_result` + 本周期发送尝试标志 | 1+1 B | 调试轨迹用；不参与判定 |
| `sample_flag` / `rtc_tick_cnt` | 1+1 B | 采样请求与 1 分钟节拍 |
| GXHT40 诊断快照（结果码/地址 ACK/重读/重测/原始 6 B） | ≤12 B | 仅本周期观测 |
| 打印快照（`debug_trace_sample_t` + 诊断字段 + P 字段） | ≈35 B | 打印窗口内使用；不参与判定 |

不新增 NVM 记录；所有状态在深睡期间由 SRAM 保持，复位后回到 `have_prev=false`。光照结果 `{valid,mean,dark}` 为单周期量，无跨周期状态（无滞回）。

---

## 2. 网关运行时结构（CH592，TMOS）

保持既有结构：RF 接收（UM2006A + TMR2 捕获）→ `decode_frame10` 解密校验 → 绑定表归集 → `build_padded_frame_blocks` → AES-128-ECB → BLE `SimpleProfile_SetParameter(CHAR2, FFE2)`。

本次**不改任务划分、优先级、特征值长度、服务/记录布局**。约束：网关不得依据自身温度历史对传感器上报做二次门控（门控在传感器完成）。与产品无关的遗留 TMOS 测量任务保持不动。

---

## 3. 资源预算

### 3.1 存储

| 项 | 预算 | 依据 |
|---|---|---|
| Flash | 64 KB；历史交付件 20,744 B（Code 20,100 / RO 644）；本版增量 <1 KB（实际构建为准） | MDK `IROM(0,0x10000)`、`gcc/cw32l010.ld` |
| SRAM | 4 KB；历史 1,792 B（RW 116 + ZI 1,676）；本版 +≤8 B | MDK `IRAM(0x20000000,0x01000)` |
| 栈/堆 | 无动态分配；软 I²C 无缓冲拷贝；不得裸拷贝结构体到空口 | 实现约束 |

### 3.2 时间预算（单周期）

| 阶段 | 时间 |
|---|---|
| PB05 稳定 | ~100 ms |
| 光照 ADC ×8 | ~3.2 ms |
| GXHT40 测量（含典型重读） | ~9.3 ms（失败最多约 3×） |
| 判定 + 冻结 | <0.01 ms |
| 433 发射（仅触发） | ~27 ms（上限 200 ms×≤3） |
| `S` 行 / `G` 或 `P` 行 | 各 ≤100 ms（有界） |
| **合计（正常 / 上报 / 失败）** | **≈213 / ≈340 / ≈340 ms**（`SENSOR_DEBUG_UART=0` 约 113/140/140 ms）；若 433 重试至上限，另加 ≤400 ms |
| 上电四类诊断 | ≈100–150 ms（一次性，仅调试开启） |

### 3.3 功耗预算（估算，须实测）

| 项 | 估算 | 假设 |
|---|---|---|
| MCU 深睡（含 LSI+RTC） | ~1.5 µA | 待实测 |
| GXHT40 待机 | 0.1 µA（手册） | 手册 |
| GXHT40 测量 | ~0.55 µAh/天 | 500 µA × 8.3 ms × 480 次 |
| 分压电流 | <0.02 µAh/天 | 仅采样约 103 ms |
| MCU 活动 | ~46 µAh/天 | 113 ms × 3 mA × 480 次 |
| 433 发射 | ~0.17 µAh/次 | 27 ms × 20 mA；按 20 次/天 ≈3.3 µAh |
| **合计** | **≈100 µAh/天（≈36 mAh/年）** | CR2032 容量与峰值电流另行实测 |

调试开启时按 §3.2 增加活动时间，量产可关。该表是设计假设，不是实测证据；`radio_sleep()` 后 UM2005C 静态电流、MCU 深睡电流必须实测回填，不据此判通过。

---

## 4. 不变量

1. 单次采样周期内温湿度测量最多执行一次。
2. 任何失败路径都不得写空口、不得推进「前值」、不得新建/覆盖待上报冻结样本、不得伪造 0 值。
3. 光照必须显式区分「有效」与「无效」；ADC 全超时不得合成为暗态；无有效全暗基准时不得声称无光。
4. 待上报样本在重试期间必须与冻结快照逐字节一致，且不得被新触发覆盖。
5. 深睡是默认态；只有 `sample_flag`、待上报或发射挂起时才保持唤醒。
6. 433 发射必须有界超时，失败不阻塞下一次 3 分钟节拍。
7. 调试通道不得改变业务：诊断行位于判定/发送之后，不修改 `report_req`/前值/冻结快照/发送结果；关闭串口只碰 UART1/PA05/PA06，不得复位 GPIOA；`SENSOR_DEBUG_UART=0` 时为零开销空实现；`TXE`/`TXBUSY` 等待必须有界。
8. 诊断观测只用于定位，不得放宽 CRC/量程/判据/标定，也不得进入空口或 BLE。
