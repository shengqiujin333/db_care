# 传感器/中继器固件并发与调度架构（RTA-001）

状态：固件设计产物（firmware_engineer.firmware_solution_design）
版本：1.0
对象：传感器（CW32L010Y8M6）与中继器（CH592 beiwov2）固件的任务/调度/中断/资源模型。
上游输入：artifacts/firmware_requirements.md（FR-001）、artifacts/interface_contract.md（IC-001）、artifacts/hardware_interface.md（HWI-001）、artifacts/schematic.md（SCH-001）。

## 1. 平台判定与 RTOS 决策

两个 MCU 当前均为**裸机事件驱动**（无 RTOS），本次不引入 RTOS：
- **传感器 CW32L010**：4 MHz 级内核、4 KB SRAM、单任务顺序主循环 + RTC/LPTIM/GPIO 中断。并发需求仅为"深睡唤醒→短时测量/上报/配置窗口"，周期和抢占要求可由 RTC 时间片 + ISR 标志满足。
- **中继器 CH592**：使用厂商 TMOS（协作式任务调度器），已有 temperature/433/BLE 任务；本功能不改调度器。

引入 FreeRTOS/TMOS 之外的新 RTOS 会增加 4 KB RAM 压力且无必要。本架构以"逻辑任务 + 事件标志 + 主循环分发"形式给出与 RTOS 等价的职责划分、优先级与资源预算，供实现与测试节点直接使用。

## 2. 逻辑任务划分（传感器）

| 任务 | 触发 | 周期/持续 | 职责 | 对应文件（现状/新增） |
|---|---|---|---|---|
| **T_SAMPLE** 采样任务 | RTC 1 min 中断置 `sample_flag` | 1 min，活跃约 100–200 ms | AHT20 软 I2C 测量 → 写 20 项历史环 → 阈值判定 → 决策上报 | measure.c（改）/ history.c（新） |
| **T_REPORT** 433 上报任务 | 立即告警 / 满 60 样本 / 首样本，置 `report_req` | 突发约 30 ms | 组 10 B Feistel 帧 → UM2005C 发射 → 成功计数管理 | measure.c / app_um2005C.c（复用） |
| **T_OPTCFG** 光学配置任务 | Hall 有效沿置 `hall_event` | 一次性 ≤35 s（100 Hz 采样） | F2 供电控制、Manchester/OOK 解码、帧校验、NVM 提交 | optcfg.c（新）/ hall.c（新） |
| **T_SLEEP** 深睡/空闲 | 无待办 | 常态 | 进入/退出深睡，RTC/GPIO 唤醒管理 | main.c（改） |
| T_DEADLINE 监控（P2） | 采样失败计数 | 随采样 | 诊断状态维护，不伪造遥测 | measure.c |

主循环为协作式分发器：`while(1){ if(sample_flag) T_SAMPLE; if(report_req) T_REPORT; if(hall_event) T_OPTCFG; T_SLEEP; }`。任务之间无抢占，数据经单一写者 + 易失标志交接。

## 3. 中断服务（ISR）边界与优先级

| 中断 | 触发 | 动作（ISR 内必须短且无阻塞） | 边界 |
|---|---|---|---|
| RTC_IRQHandler | 1 min 间隔 | 置 `sample_flag`，维护上报节拍计数 | 现有 RTC_IRQHandlerCallBack 扩展；只置标志 |
| GPIOB_IRQHandler | PB04 Hall 有效沿 | 去抖后置 `hall_event`；窗口活跃时忽略 | 新增；不得在 ISR 内开窗口/上电 |
| LPTIM_IRQHandler | 433 位时钟（现有）/ OPTCFG 10 ms 采样（窗口内） | 433：现有 app_gtimer 位输出；OPTCFG：采样 PB05 并喂解码器 | 两模式互斥（模式互锁，见 §5） |
| UART/其它 | 调试 | 保持现有 | 不参与业务时序 |

优先级说明（协作式 + 关中断临界区）：RTC/GPIO 唤醒中断优先级高于业务处理；LPTIM 位时钟中断只写寄存器；所有跨任务数据以 `volatile` 单标志 + 显式临界区（`__disable_irq/__enable_irq` 或关对应 NVIC）保护。ISR 内禁止 I2C/Flash/433 等慢操作。

## 4. 调度与同步规则

- **节拍源**：RTC LSI，`RTC_INTERVAL_EVERY_1M`（1 min/拍）。`rtc_set_cnt` 每拍 +1，到 1 置 `sample_flag` 并清零（当前代码为 2/5 阈值，需改为 1/复位），实现 1 min 采样。
- **上报节拍**：`samples_since_report` 每成功采样 +1；成功上报归零；达到 60 触发小时上报（IC-001 §2"满 60 个采样周期"）。
- **同步原语**：事件标志（`volatile uint8_t`）单写者；历史环单写者（T_SAMPLE）只读他人；433 发送缓冲（`g_send_data`）单写者（T_REPORT）；解码缓冲单写者（T_OPTCFG）。
- **互斥**：LPTIM 归属模式互锁（433 发送 / OPTCFG 采样二选一）；433 与 OPTCFG 不得并发（延迟上报 + 模式切换）。
- **功耗调度**：无待办即深睡；RTC 定时唤醒 + GPIO 外部中断唤醒；深睡期间保留 GPIO 状态（PB06 保持 F2 断电）。

## 5. LPTIM 资源归属（关键约束）

LPTIM 同时服务于现有 433 位时钟（app_gtimer，10 kbps）与新增 OPTCFG 10 ms 采样时钟：
- 进入 433 发送：按现状 `LPTIM_Configuration` 重配为位时钟，`app_gtimer_init(bps)` 复用。
- 进入 OPTCFG 窗口：将 LPTIM 重配为 10 ms（100 Hz）采样触发；窗口结束恢复。
- 模式切换集中在 `radio_into_tx`/窗口状态机两处，任何时刻仅一种模式占用 LPTIM。实现节点须复核两处重配的一致性与中断使能/禁止配对。

## 6. 时序与资源预算

| 项 | 预算 | 说明 |
|---|---|---|
| 1 min 采样活跃 | ~100–200 ms | AHT20 启动+读取+判定 |
| 433 单次发射 | ~30 ms | 20B 前导+4B 同步+10B 数据 @10 kbps |
| OPTCFG 窗口 | ≤35 s，100 Hz 采样 | 见 FD-001 §4.3 时序论证 |
| RAM 新增 | ≤300 B | 见 FD-001 §5 |
| 深睡电流 | <10 µA | F1 常供 + 全断电 |

## 7. 中继器（CH592 beiwov2）任务现状

中继器沿用 TMOS 协作调度，已有任务：`temperature_task`（AHT20 板上测量，可选/遗留）、`get_temperature_task`（UM2006A 433 接收 → decode_frame10 → BLE CHAR2 聚合）、`app_ev1527`（TMR2 捕获解码）。本功能仅需在 433→BLE 记录构造处调整字段顺序（FR-401），不改任务划分与调度；TMOS 事件、优先级、睡眠保持现状。

## 8. 交接

- 传感器实现节点：按 §2/§3/§4 落地任务与中断；务必完成 LPTIM 模式互锁与 RTC 节拍改值。
- 嵌入式测试节点：按 §6 时序与 §4 同步规则编写并发/时序验证用例（采样节拍、窗口与上报互斥、深睡唤醒）。
