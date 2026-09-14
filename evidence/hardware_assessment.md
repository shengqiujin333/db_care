# 硬件评估证据（HA-001）

状态：硬件方案设计产物（evidence）
版本：1.0
能力：hardware_engineer.hardware_solution_design
关联：artifacts/hardware_solution.md（HW-001）、artifacts/hardware_feasibility.md（HFE-001）

## 1. 评估输入与依据（源码/图纸溯源）

| 依据 | 来源 | 用途 |
|---|---|---|
| 需求：增加霍尔检测、光敏接收闪光灯 6 个 float 阈值、1 min 采样、20 min 缓冲、阈值/小时上报 | D:\mypro\beiwo2\AIP\readme.txt | 明确硬件增量目标 |
| 硬件三项要求：Hall 数字输入、可关断光敏前端电源、可定时采样数字输入、CR2032 预算 | artifacts/interface_contract.md（IC-001 §8） | 硬件方案必须满足的接口约束 |
| 现有传感器原理图（MCU=CW32L010Y8M6、AHT10、UM2005C、433 spring 天线、晶振、排针、CR2032） | D:\mypro\beiwo2\AIP\sensor_board.xml | 确定现有器件与空闲引脚 |
| 现有 GPIO 分配（PA3/PA4=AHT10 I2C、PA5/PA6=UART1、PA7/PA8=SWD、PB2/PB3=UM2005C 433） | sensor 固件 USER/src/measure.c、USER/inc/measure.h、UM2005C/um2005C_hal.c | 确定可用空闲脚 PA2/PB4/PB5/PB6 |
| 深睡省电策略（GotoDeepSleep、radio_sleep） | sensor 固件 USER/src/main.c、measure.c、radio.c | 匹配待机暗电流预算 |

## 2. 评估结论

- **可行**：技术成熟、对现有设计侵入低、无需换 MCU。
- **MCU 资源**：存在 ≥3 空闲脚，满足 F1 中断输入 + F2 采样输入 + 电源开关输出。
- **电源**：F2 前端平时断电（暗电流≈0），30 s 窗口持续 <2 mA，符合 CR2032；433 短脉冲为现状。
- **接口满足**：方案提供的 Hall 数字输入、可关断光敏前端电源、可定时采样数字输入，逐一对应 IC-001 §8 要求。

## 3. 未决项（交接后续）

- 器件数据手册精确电流值（schematic 阶段复核）。
- CW32L010 具体封装脚外部中断能力（schematic_design 核定）。
- Hall 静态电流是否触发 GPIO 关断设计（随最终器件选型）。
