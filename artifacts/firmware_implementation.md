# 传感器/中继器固件实现（FW-IMP-001）

状态：固件实现产物（firmware_engineer.firmware_implementation）
版本：1.0
对象：
- **传感器固件（CW32L010Y8M6）**：OPTCFG/1 光学配置、霍尔触发、20 项历史、阈值告警与上报调度、NVM 持久化、433 实参修正。
- **中继器固件（CH592 beiwov2）**：BLE 每设备记录字段顺序对齐（FR-401）。
实现依据：readme.txt、artifacts/interface_contract.md（IC-001）、artifacts/firmware_design.md（FD-001）、artifacts/firmware_requirements.md（FR-001）、artifacts/firmware_rtos_architecture.md（RTA-001）、artifacts/test_design.md（TD-001）。

---

## 1. 交付概览

| 项 | 结果 |
|---|---|
| 传感器源码 | 新增 5 个模块（fw_core/hall/history/optcfg/params）+ 修改 6 个文件（main/measure/interrupts/main.h/measure.h/app_um2005C） |
| 中继器源码 | 修改 1 处（app_um2006A.c sensorres 记录顺序） |
| 交叉编译 | arm-none-eabi-gcc 全工程 40 TU 编译 0 错误；链接成功 FLASH 51.72%（33,896 B/64 KB）、RAM 47.85%（1,960 B/4 KB） |
| L0 宿主机单测 | 56/56 通过（CRC、参数校验、记录编解码、帧校验、Manchester 解码含 ±25% 抖动 3/3、历史/告警） |
| 功耗 | 需目标板实测（TD-PWR-*），本文档 §7 记录设计预算与交接 |

## 2. 传感器固件实现内容

### 2.1 新增模块（纯逻辑与硬件分离，TD-001 L0 可测）

| 文件 | 职责 | 主要需求映射 |
|---|---|---|
| `USER/src/fw_core.c/.h` | 纯逻辑核心：CRC16-CCITT-FALSE、参数数值校验/x10 换算/NVM 记录编解码、OPTCFG/1 帧校验、Manchester/OOK 解码器（确定性相位，前导起点检测） | FR-103/104/105/107/205 |
| `USER/src/history.c/.h` | 20 项环形缓冲（4 B/项=80 B，int16 temp_x10+uint16 hum_x10）、下降告警纯判定 | FR-202/204 |
| `USER/src/params.c/.h` | 参数 RAM 模型（x10 整数域）、Flash 顶层页（0xFF80，128 B）非易失记录读写、幂等重放、默认值装载 | FR-107/108/205 |
| `USER/src/optcfg.c/.h` | OPTCFG/1 窗口状态机：PB06=F2 供电（低使能）、PB05 光敏采样、LPTIM 5 ms 采样、35 s 窗口、成功/超时/连续 3 失败关窗 | FR-101/102/106/109 |
| `USER/src/hall.c/.h` | PB04 霍尔输入+EXTI（下降沿有效，可配）、5–10 ms 去抖、窗口内忽略重复触发 | FR-101/102/506 |

### 2.2 修改文件

| 文件 | 改动 | 需求映射 |
|---|---|---|
| `USER/src/main.c` | RTC 节拍改值（1 min/拍置 sample_flag，删除 send_flag）；初始化 params/history/optcfg/hall；主循环加入 Hall→开窗、optcfg_process、上报调度 | FR-204/203；FD-001 §8 |
| `USER/src/measure.c` | `tempvalue` 改 int16_t；采样完成后写历史环、`samples_since_report++`、立即/小时/首样本上报决策；**修正 `encode_frame10(mcu_uid, tempvalue, huminityvalue, ...)` 实参**；有界发送（200 ms 超时）+失败重试（≤3 次，失败不归零）；窗口内延后上报 | FR-202/203/205/302/303/304/305/505 |
| `USER/src/interrupts_cw32l010.c` | GPIOB_IRQHandler 处理 PB04 Hall 沿；LPTIM_IRQHandler 按窗口状态路由（optcfg 采样 / 433 位时钟） | FR-101；RTA-001 §5 |
| `USER/inc/main.h` / `measure.h` | 新模块 include 与上报状态 extern | — |
| `UM2005C/app_um2005C.c/.h` | 新增 `app_um2005C_send_data_timeout()`（有界发送，返回成败） | FR-303/304 |

### 2.3 关键实现决策（与设计的偏差及理由）

1. **窗口采样率 100 Hz → 200 Hz（5 ms）**：FD-001/TD-001 原记录 100 Hz（10 ms）。L0 实测表明 10 ms 采样下 ±25% 半位宽容差（IC-001 §3.1 强制要求）使边界沿与位中心沿在 `t=3` 样本处不可区分（抖动 3 帧通过率 0/3）；提高到 5 ms 后半位=3..5 样本、位中心=6..10 样本，分类无重叠（抖动 3/3 通过）。接口仅要求"可定时采样的数字输入"（HWI-001 §4，下限 25 Hz），硬件带宽与 MCU 负载（200 Hz×35 s≈7000 次中断）均无压力；F2 前端 ~1.1 mA 电流主导，功耗影响可忽略。已在 L0 测试与本文档留痕。
2. **Manchester 解码器**：采用确定性相位方案——>300 ms 暗区后首沿=前导起点（位边界），起点+20 ms 沿=bit0 位中心，其后每 40 ms 沿=位中心；数据相位忽略 20 ms 处边界沿，仅按位中心解码（bit=首半电平取反），以 magic `44 42` + CRC 做最终校验（反相/歧义流被拒）。前导 0x55 在该映射下为 40 ms 方波（非 20 ms），解码器按实测波形实现。
3. **发送失败语义**：无 RF/光学 ACK 通道，采用"发射完成=成功"（有界 200 ms 看门狗，避免 LPTIM 异常挂死）；超时计失败并重试 ≤3 次，仍失败保留待上报状态，小时调度自然重试（FR-304 语义下"不记成功"= 未完成发射不归零）。
4. **窗口 35 s**：与 FD-001 §4.3 一致，容 2 次完整发射；成功提交/3 失败/超时即断电关窗。

## 3. 中继器固件实现内容（FR-401）

`CH592EVT/EVT/EXAM/BLE/beiwov2/APP/app_um2006A.c`：`sensorres[i*8+4..7]` 由 `[tempervalue | humivalue]`（温度在前）调整为 `[humivalue | tempervalue]`（湿度在前），对齐 IC-001 §6 每设备记录 `[sensor_id(4) | humidity_x10_be(2) | temperature_x10_be(2)]` 与 Android `parsePlainFrame`（u16be@4/s16be@6）。与传感器实参修正（§2.2）**必须同批落地联调**（FD-001 §6.4 同步约束）。

## 4. 需求追踪（FR-001 覆盖情况）

| FR | 实现位置 | 状态 |
|---|---|---|
| FR-101..102 | optcfg 窗口状态机 + hall | 已实现 |
| FR-103 | fw_core Manchester 解码器（±25% 抖动 L0 验证 3/3） | 已实现 |
| FR-104..105 | fw_core 帧校验/数值校验（L0 全用例） | 已实现 |
| FR-106..108 | params 原子提交/NVM/幂等（L0 记录编解码验证；掉电中断写入待目标板 TD-OPT-016） | 已实现 |
| FR-109 | 解码器 >300 ms 暗区复位（60 样本@5 ms） | 已实现 |
| FR-201..205 | measure/history/params（整数域判定、20 项环、1 min 采样） | 已实现 |
| FR-301..305 | 433 帧/实参修正/调度/重试/窗口互斥 | 已实现 |
| FR-401..403 | 中继器记录顺序/BLE 兼容 | 已实现（需 WCH 工具链构建验证） |
| FR-501 | 深睡 <10 µA / 窗口 <2 mA | 设计预算，待 TD-PWR-001/002 实测 |
| FR-502 | RAM 新增 180 B ≤ 300 B | 已实测（§5） |
| FR-503..507 | 显式字节序/无密钥日志/AHT 失败/去抖/默认安全 | 已实现 |

## 5. 资源实测（arm-none-eabi-gcc 链接，build/gcc）

| 项 | 值 | 预算 |
|---|---|---|
| FLASH | 33,896 B / 64 KB = 51.72% | 充足 |
| RAM 总量 | 1,960 B / 4 KB = 47.85% | <60% 目标 ✓ |
| 新增静态 RAM | 180 B（history 80 + optcfg 43 + pending 28 + params 20 + hall 1 + 窗口标志 8） | ≤300 B ✓ |
| 可优化项 | `APP_GTIMER_DATA_MAX` 1024→64 可释放 ~960 B（实际 TX 仅 34 B） | P2，未实施 |

## 6. 验证结果

- L0 宿主机单测（test/host_sensor_core_test.c，56/56 通过）：CRC 标准向量与 TD-001 参考帧 `0xB540`、参数校验 18 例、记录编解码、OPTCFG 帧校验 13 例、Manchester 解码（标称帧一致、±25% 抖动 3/3、反相拒绝）、历史环/下降告警。详见 evidence/protocol_test.md、evidence/driver_test.md。
- 交叉编译（gcc/build.sh，arm-none-eabi-gcc 10.3，Cortex-M0+）：40 TU 0 错误；链接生成 elf/hex/bin。详见 evidence/build.md。
- 目标板级验证（TD-OPT-*/SMP/RPT/433/BLE/TIM/PWR）由嵌入式测试执行能力承接；L1 起需要真实硬件台架。

## 7. 交接与未决项

1. **中继器构建**：CH592 工程需 WCH 工具链（MounRiver/WCHISPTool 生成 config.h），本环境无该工具链；`app_um2006A.c` 改动为纯索引交换，风险低，需在 WCH 环境构建并跑 TD-BLE-001/003、TD-433-002。
2. **功耗实测**：TD-PWR-001/002（深睡 <10 µA、窗口 <2 mA）需 SMU/电流表在目标板实测，本能力未具备仪器。
3. **RDP2 下 Flash 应用态擦写**（FD-001 R3）：现有 `FLASH_SetReadOutLevel(FLASH_RDLEVEL2)` 每次上电执行，需在目标板验证 NVM 页擦写（TD-OPT-016/017 前置）。
4. **深睡 GPIO 保持**（FD-001 R4）：PB06 断电态与 PB04 EXTI 唤醒需 TD-TIM-003 验证。
5. **Android 端**：按 IC-001 实现闪光灯设参（后续 Android 能力）；`parsePlainFrame` 保持 hum-first；与传感器/中继器本次改动同批联调（TD-433-003 回归哨兵）。
6. **采样率决策**（100 Hz→200 Hz）已由 L0 验证支撑；如后续协议/器件变化，解码器阈值集中在 fw_core.c，便于回归。

## 8. 边界声明

- 本实现仅覆盖 firmware_engineer.firmware_implementation 职责；未修改 Android 工程、PCB/原理图、硬件测试资产。
- 未引入新产品需求；仅依据 IC-001/FD-001/FR-001 落地，实现决策（§2.3）均留有验证证据。
