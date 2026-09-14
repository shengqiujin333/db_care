# 原理图电气规则自检证据（ERC-001）

状态：hardware_engineer.schematic_design 产出（evidence）
版本：1.0
能力：hardware_engineer.schematic_design
关联：artifacts/schematic.md（SCH-001）、artifacts/hardware_solution.md（HW-001）、artifacts/hardware_interface.md（HWI-001）、artifacts/interface_contract.md（IC-001 §8）

> 说明：现有原理图为 Cadence OrCAD dsn XML（sensor_board.xml），本版本以手工增量规格交付原理图（artifacts/schematic.md）。本证据为对新增 F1/F2 网络的**静态 ERC 自检**，逐项核对电气规则；Cadence 在线 ERC 工具在原理图落库后由后续环节复核。

## 1. 接线完整性（connectivity）

| 检查项 | 结果 |
|---|---|
| 每颗新器件所有引脚均有网络（无悬空） | ✅ U8/U9/Q1 各脚、R3/R4/R5/R6/C17/C18 各脚均列入 §2.2 网表 |
| 单网络唯一性、无短路重名 | ✅ `HALL_DOUT`/`F2_PWR_EN`/`OPT_IN`/`F2_VCC` 为新增唯一网络，不与现有 VDD/GND 冲突 |
| 与现有电路连接点明确 | ✅ PB04→HALL_DOUT、PB05→OPT_IN、PB06→F2_PWR_EN、VDD/GND 接入全局轨 |

## 2. 引脚冲突（pin conflict）

| 检查项 | 结果 |
|---|---|
| 使用的 MCU 脚未被现有占用 | ✅ PA02/PB04/PB05/PB06 空闲（HW-001 §5 核定；PA3/4=AHT10、PA5/6=UART、PB2/3=UM2005C、PA7/8=SWD、PB0/1=OSC32） |
| 无同名网络接不同电平 | ✅ 无 |
| 无输出对输出直接短接 | ✅ 每个 MCU 脚仅一个驱动方向：PB04 输入、PB05 输入、PB06 输出 |

## 3. 电源与极性（power / polarity）

| 检查项 | 结果 |
|---|---|
| 各 IC 供电范围覆盖 3.0V | ✅ U8 Hall 2.5–5.5V、U9 P-MOS 3V 逻辑、Q1 3V |
| 极性正确 | ✅ Q1 集电极→F2_VCC（高）、发射极→R6→GND；C17/C18 无极性（陶瓷） |
| P-MOS 高边拓扑正确 | ✅ Source=VDD、Drain=F2_VCC、Gate 低有效；R4 保证默认关断 |
| 无电源短路 | ✅ U9 关断时 F2_VCC=0V，Q1 无直通 VDD→GND 路径 |

## 4. 负载与带宽（loading / bandwidth）

| 检查项 | 结果 |
|---|---|
| Hall 输出负载 | ✅ R3(10 kΩ) 上拉，开漏可驱动 |
| 光敏前端电流 <2 mA | ✅ 受光 ≈1.1 mA（(3V−Vce)/2.2k），30 s 窗口内，符合 CR2032 持续限制 |
| 25 Hz 带宽裕量 | ✅ R6=2.2 kΩ 与节点电容 RC << 半位宽 20 ms，无失真 |
| MCU 输入钳位/过压 | ✅ OPT_IN/HALL_DOUT 电平在 0–3V，无超压 |

## 5. 去耦（decoupling）

| 检查项 | 结果 |
|---|---|
| U8 Hall VDD 去耦 | ✅ C17(100 nF) 就近放置 |
| F2_VCC 上电瞬间去耦 | ✅ C18(100 nF)（HW-001 §6 约束 4） |

## 6. 待机暗电流预算（idle current budget）

| 检查项 | 结果 |
|---|---|
| 常态新增电流 <10 µA | ✅ Hall ~8 µA + U9 泄漏 <1 µA + Q1/R6 断电≈0，符合 HW-001 §4 |
| F2 前端默认断电 | ✅ R4 上拉 → U9 关断 → F2_VCC=0V |

## 7. ERC 结论

- **通过**：新增 F1/F2 网络静态 ERC 全部通过，无接线悬空、无引脚冲突、电源/极性正确、负载与带宽裕量充足、去耦到位、暗电流符合预算。
- **待后续复核**：最终 Hall 料号静态电流（>5 µA 则启用 GPIO 关断可选改动，见 SCH-001 §5）；Cadence 在线 ERC/DRC 在原理图落库与 PCB 阶段复核。
