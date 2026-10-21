# 固件运行时与任务架构（RTA-003）

状态：`DESIGN_INPUT_READY`（运行时架构设计完成；未实现、未实板验证、未验收）
能力：`firmware_engineer.firmware_solution_design`
基线：Git `12bb5b2`；产品源码未改
输入：FWR-003、FDS-003、PDR-001、IC-003、PACC-002、FEX-001

本文件描述传感器固件的**裸机运行时**结构与预算；不引入 RTOS，不新增任务/线程/中间件。

---

## 1. 执行模型

```text
Reset
  → 锁存复位来源
  → 系统时钟/GPIO/RTC 初始化
  → 读取 MCU UID
  → UART1 BOOT
  → 上电诊断 VDD → IOTEST → IOSIG → BUS
  → if UID 无效: 关闭 UART → 真正深睡循环（不再采样）
  → sample_flag = 1（首次机会）
  → 主循环:
       if sample_flag: 执行一次采样事务（光照→温湿度→判定→冻结→发送→S/G）
       关闭 UART
       if 无待发/无标志: 深睡
  → RTC 1 min 中断累计 3 次 → sample_flag = 1 → 唤醒
```

- 唯一调度者是主循环 + RTC 中断；没有抢占式任务。
- RTC ISR 只做：`rtc_tick_total++`、`rtc_set_cnt++`、达到 3 置 `sample_flag`、清中断。
- 不在 ISR 中执行 I²C/ADC/UART/RF/Flash。
- `sample_flag`、`report_req`/`pending.active` 是主循环与 ISR 的唯一共享控制状态；均为 8-bit 对齐变量，单次读写原子；不在 ISR 中读取多字节业务快照。

---

## 2. 逻辑任务与边界

| 逻辑任务 | 执行上下文 | 职责 | 不得做 |
|---|---|---|---|
| T_BOOT | 主循环（一次） | 时钟/GPIO/RTC/UID；BOOT/VDD/IOTEST/IOSIG/BUS；致命 UID 处理 | 不在 UID 无效时进入采样/发送；不空转 |
| T_SAMPLE | 主循环 | `light_sample()` → `gxht40_measure()`；保存本周期结果与驱动诊断 | 不在一个机会内重复完整采样；不在失败时构造样本 |
| T_DECIDE | 主循环 | 判定前快照 prev/have；调用纯逻辑门控；成功后推进 prev/湿度 | 不用推进后的 prev 判定；不在失败时判定 |
| T_FREEZE_SEND | 主循环 | 触发时冻结 10 B 帧；单周期最多 3 次×200 ms 发送同一帧；成功/用尽清除 | 不跨周期保留；不重采样；不重复编码改值 |
| T_TRACE | 主循环 | 发送结束后打印唯一 S（失败追加 G）；关闭 UART | 不在发送重试前打印；不因打印改变业务 |
| T_SLEEP | 主循环 | PB05 低、UART 关、I²C 释放、深睡 | 不在有待发/采样标志时睡；不无限等待 |

---

## 3. 状态机

```text
BOOT
  └─(UID ok)─> IDLE ──(sample_flag)──> LIGHT ─> TH_MEASURE
                   ^                         │
                   │                    ┌────┴────┐
                   │                  fail        ok
                   │                    │          │
                   │                    │        DECIDE
                   │                    │       ┌──┴──┐
                   │                    │     no-rep  rep
                   │                    │       │      │
                   │                    │       │   FREEZE+SEND(≤3)
                   │                    │       │      │
                   │                    └───────┴──TRACE(S/G)
                   │                               │
                   └──────────── SLEEP <───────────┘
```

- `IDLE` 只由 RTC tick 置位；一个机会消费一次 `sample_flag`。
- `TH_MEASURE` 失败仍进入 `TRACE`（q=0 + G），不进入 `DECIDE`。
- `FREEZE+SEND` 期间不回到 `LIGHT/TH`；发送尝试在同一次 `T_FREEZE_SEND` 内完成。
- `TRACE` 后若 `pending.active==0 && sample_flag==0` 才 `SLEEP`；否则继续主循环（防御性）。

---

## 4. 时间预算（单次机会，设计估算）

| 段 | 估算 | 依据/说明 |
|---|---:|---|
| 光照建立 + 8 次 ADC | ~104 ms | 100 ms 建立 + 8×390 µs 采样 + 轮询开销 |
| GXHT40 地址/命令/读（正常） | ~25–30 ms | FEX 实测 SCL 位周期约 214 µs（4.7 kHz）；2 B 命令/地址 + 6 B 读 + 10 ms 等待 |
| GXHT40 读重试/整帧重测最坏 | ~220–250 ms | 5 次读重读 ×3 次重测 + 等待；均为配置上限，非实测最坏 |
| 一次 433 发送 | 既有等待预算 ≤200 ms；不含 `radio_init` 前置配置 | 既有 API；init 开销未测 |
| 3 次发送 | 既有等待预算 ≤600 ms；不是完整上界 | 单周期最多 3 次；init 开销另计 |
| UART S+G（≤96 B/行） | ~100–150 ms | 9600 8N1 线时；有界等待上限另计 |
| 上电诊断（一次） | ~0.2–0.6 s | BUS 112 地址探测 + 5 行串口；不是周期开销 |
| **单周期最坏（设计上限）** | **上述估算 <1.5 s，不含 `radio_init` 未测开销** | 远小于 180 s；不新增额外采样机会；不作为硬上界 |

上述为设计与静态/局部测量估算；不以估算替代 PA-007 的主机时间戳实测。UART 有界等待若触发，增加时间按实现/测试校准（设计目标约 250–300 ms/行），不是硬实时上界；仍远小于 180 s。

---

## 5. 资源预算

| 资源 | 当前构建（FEX/EV-017 同镜像） | 设计增量预算 | 约束 |
|---|---:|---:|---|
| Flash | Code 20,100 + RO 644 + RW 116 = 20,860 B / 64 KB | 新增逻辑/测试常量 < 2 KB | 不引入新协议/大表 |
| RAM | RW 116 + ZI 1,676 = 1,792 B / 4 KB | 新增静态状态 < 128 B（帧 10 B、快照/轨迹 < 64 B） | 不新增跨周期队列/动态分配 |
| 栈 | 未测峰值 | 不新增 >32 B 局部数组；主循环 trace 结构约数十 B | 不做大缓冲/递归 |
| 外设 | ADC、GPIOA/B、RTC、LPTIM、UART1、软 I²C | 不新增外设/中断 | 不与 RF/LPTIM/UART 冲突 |
| 引脚 | PA03/04、PA05/06、PB04/05、PB02/03、PA07/08 | 不变 | 不复位 GPIOA；不改 PB06 |

Flash/RAM 数值来自 FEX 对 `8ae4a33`/`5fc16235…` 镜像的构建记录；新构建必须重新记录，不能把旧数值当最终。

---

## 6. 中断、睡眠与恢复不变量

1. RTC 1 min 中断是唯一周期源；不在主循环用 `delay` 造节拍。
2. `sample_flag` 只在 RTC ISR 置位、主循环消费；不在 ISR 清业务状态。
3. 深睡前：`pending.active==0`、`sample_flag==0`、UART 已关闭/排空（有界）、PB05 低、I²C 两线释放。
4. 唤醒后：重新按需打开 UART；I²C 端口保持已初始化；不执行 GPIOA 复位。
5. 复位后：`have_prev=false`、`pending.active=0`、`report_req=0`、`sample_flag=1`；不恢复 Flash 历史参数。
6. 任何失败路径都必须回到主循环并最终可睡；不得因等 UART/等 ACK/等 RF 而永久阻塞。
7. 不在 ISR 中调用 `delay_ms`、`light_sample`、`gxht40_measure`、`app_um2005C_send_data_timeout` 或任何打印。

---

## 7. 故障隔离层级

| 故障 | 运行时行为 | 不变量 |
|---|---|---|
| GXHT40 无 ACK/CRC/超域 | 本周期 `q=0`，G 行；不判定/不发送；下周期重探 | 不伪造、不推进 prev |
| 光照 ADC 全失败 | `valid=false`；温湿度仍可采样；超温可报 | 不合成暗态 |
| 未标定 | `valid=false`；升温分支关 | 不把未标定当已验收 |
| RF 3 次失败 | 本周期 `s=2`；清除 pending；睡眠 | 不跨周期、不伪造成功 |
| UART 等待超时 | 中止本行；关闭 UART；继续业务/睡眠 | 不阻塞业务 |
| UID 无效 | BOOT/诊断后深睡 | 不空转、不采样/发送 |
| RTC 丢失/异常 | 无新机会；不补采样 | 不周期心跳、不补点 |

---

## 8. 可测试性边界

- 纯逻辑（门控/CRC/换算/光照判据）与 MCU 无关，可宿主机编译。
- `measure.c` 通过 mock 的 `light_sample`/`gxht40_measure`/`app_um2005C_send_data_timeout`/UART 验证控制流；不得在生产代码中加入测试注入分支。
- 独立测试能力拥有的 `*_verify_ev.c` 不被实现能力修改；实现只更新自有 `host_*_check.c` 与 `build_test.sh`。
- 运行时预算中的栈峰值、深睡电流、LSI 容差、RF 峰值压降不在本文件宣称通过，交独立测试/硬件测试。
