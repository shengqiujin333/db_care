# 嵌入式验证证据（EV-002 / ITEM-001）

状态：embedded_tester.embedded_verification 独立验证证据
本轮验证对象：任务项 **ITEM-001**（新增 `sensor_config.h` 作为唯一配置点；加入后工程仍可交叉编译）
被测提交：`ab5c5b2`（firmware_engineer.firmware_implementation）
验证基线：`9e90eb5`（本次实现之前，已含 TD-002 测试设计）
结论：**TEST_PASS**（ITEM-001 的声明行为全部独立复现；另发现 1 项与本项无关的既有 Keil 工具链环境问题，见 §6）

上游输入：`artifacts/firmware_implementation.md`（FWI-002 声明）、`artifacts/test_design.md`（TD-002）、`artifacts/firmware_design.md`（FD-002）、IC-002、任务清单 ITEM-001。

---

## 1. 验证范围与选测说明（为什么选这些）

| 选测项 | 理由 |
|---|---|
| 交付物存在性与常量值（TD-002 T-L1-05/T-L1-08 的配置点部分） | ITEM-001 的唯一验收物就是该头文件与其常量 |
| 头文件自包含 / 编译期断言 | 证明常量可被后续 TU 直接使用且取值正确，不靠人工目测 |
| GNU 交叉编译 + 与基线逐字节比对（TD-002 T-L1-02） | 直接检验「加入该头文件后工程仍可交叉编译」 |
| 生产编译器 ARM Compiler 5（Keil AC5）+ 工程 include 路径 | 交叉编译的真实量产工具链是 Keil MDK / IAR，需证明头文件对 AC5 与两个工程都可用 |
| 宿主机既有 L0 回归（TD-002 T-L0-06 的回归部分） | 本次改动不涉及被判定的既有纯逻辑，需确认无回归 |
| 变更范围审查（git） | 确认未夹带无关改动，且 .c 未被改动（支持"无行为变化"结论） |

**不适用（本轮未实现，按能力约定不要求、也不作为本项通过条件）**：TD-002 中 T-L0-01…T-L0-05（CRC-8/换算/光照/上报判定纯逻辑）、T-L0i（空口互操作）、T-L2/T-L3/T-L4/T-L5/T-L6/T-L7/T-L8（I²C、光照、节拍、条件上报、失败路径、端到端、功耗）——这些行为集中在后续 ITEM-002…ITEM-012，本轮不存在可测对象。仅对既有保留逻辑做回归。

## 2. 环境与工具（本轮实际调用）

| 工具 | 版本/路径 | 用途 |
|---|---|---|
| `arm-none-eabi-gcc` | 10.3.1 20210824（Cortex-M0+） | 工程交叉编译、编译期断言 TU |
| ARM Compiler 5 | `C:/Keil_v5/ARM/ARM_Compiler_5.06u7/Bin/armcc.exe` | 量产编译器单 TU 兼容性验证 |
| MinGW-w64 gcc | 12.2.0（`C:/ProgramData/chocolatey/.../mingw64/bin/gcc.exe`） | 宿主机 L0 回归 |
| `mdk_build`（Runtime 工具） | 配置的 Keil 工程 | 尝试真实 Keil 工程构建（结果见 §6） |
| 逻辑分析仪/示波器/J-Link/串口/烧录 | — | **本轮未使用**：ITEM-001 只新增宏定义，无可观测的板上行为；无需也不可能用仪器验证 |

## 3. 检查明细与结果

### 3.1 交付物存在与内容（通过）

`CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/inc/sensor_config.h` 存在，135 行，含 6 组配置。独立核对：

| 要求组（ITEM-001 原文） | 实际定义 | 判定 |
|---|---|---|
| PB04/PB05 引脚 | `LIGHT_POWER_PORT=CW_GPIOB`、`LIGHT_POWER_PIN=GPIO_PIN_5`（PB05）；`LIGHT_ADC_PORT=CW_GPIOB`、`LIGHT_ADC_PIN=GPIO_PIN_4`（PB04） | PASS |
| ADC 通道 | `LIGHT_ADC_INPUT_CHANNEL=ADC_InputCH11`（=11）；`LIGHT_ADC_FULL_SCALE=4095`；`LIGHT_ADC_CLK_DIV=ADC_Clk_Div8`；`LIGHT_ADC_SAMPLE_TIME=ADC_SampTime390Clk`；`LIGHT_ADC_SAMPLES=8` | PASS |
| GXHT40 地址/命令/重试/等待 | 0x44/0x45；写读字节 0x88/0x89/0x8A/0x8B；`0xFD`（高重复率，另存备用 0xF6/0xE0 与软复位 0x94 并注明周期路径禁用）；`GXHT40_RESULT_LEN=6`；`GXHT40_MEASURE_WAIT_MS=10`；`GXHT40_READ_RETRY=5`+`1 ms`；`GXHT40_MEAS_RETRY=3`；`GXHT40_POWER_ON_WAIT_MS=2`；CRC8 poly 0x31/init 0xFF；换算常量 `-450/1750/-60/1250/65536/32768` 与量程 `-400..1250`、`0..1000` | PASS |
| 光照明暗阈值与滞回 | `LIGHT_DARK_ENTER=350`、`LIGHT_DARK_EXIT=250`、`LIGHT_DARK_HYSTERESIS_STEP=100`；`LIGHT_DARK_CALIBRATED=0`（明确标注"未标定，须实板标定后回填"） | PASS（并未把未标定默认值谎称已验收） |
| 3 分钟采样计数值 | `SENSOR_RTC_TICK_PERIOD_MIN=1`、`SENSOR_SAMPLE_TICKS=3` | PASS |
| 0.9 ℃ 下降 / 35.0 ℃ 超温上报常量 | `SENSOR_REPORT_DROP_X10=9`、`SENSOR_REPORT_HIGH_X10=350`，注释写明严格大于语义 | PASS |

附带核对：`SENSOR_RF_FRAME_LEN=10`、`SENSOR_RF_TX_TIMEOUT_MS=200`、`SENSOR_RF_TX_RETRY=3`、调试钩子 `SENSOR_TEST_TRACE` 默认 0。

**独立交叉核对（来源可信度）**：
- PB04 = AIN11 = `ADC_InputCH11`：由厂商库头 `CW32L010_StandardPeripheralLib_V1.0.5/Libraries/inc/cw32l010_adc.h` 的注释与宏独立确认（`ADC_InputCH11 = 0x0B`，注释「通道11输入PB04」），非仅引用设计文档。
- PB04 模拟功能宏 `PB04_ANALOG_ENABLE()` 存在于 `cw32l010_gpio.h`。

### 3.2 头文件自包含 + 编译期断言（通过）

自写独立验证 TU（`sensor_config.h` 为唯一项目头包含，另仅 `_Static_assert`），43 条断言覆盖上表全部常量与关系（`EXIT<ENTER`、`ENTER-EXIT==STEP`、`MEASURE_WAIT_MS>=9`、地址派生、通道=11 等）：

```
arm-none-eabi-gcc -mcpu=cortex-m0plus -mthumb -Wall -Wextra -std=c11 \
  -I USER/inc -I COMMON -I UM2005C -I ../../../Libraries/inc \
  -I <CMSIS 5.9.0 Core Include> -fsyntax-only <verify_tu.c>
=> ASSERT_TU_OK: all 43 static assertions compiled cleanly（0 警告，0 错误）
```

结论：头文件自包含（仅 `<stdint.h>`/`<stdbool.h>` + CW32 库头），所有断言值正确。

### 3.3 唯一配置点（通过，局部）

- 在目标工程范围内（`USER/`、`UM2005C/`、`COMMON/`、`test/`、`gcc/`、`MDK/`、`EWARM/`）检索 `GXHT40_*`/`LIGHT_*`/`SENSOR_RTC_*`/`SENSOR_SAMPLE_*`/`SENSOR_REPORT_*`/`SENSOR_RF_*`/`SENSOR_TEST_*` 的 `#define`：**除 `sensor_config.h` 外为 0 处**，无重复定义/冲突。
- 实际被构建使用（非死文件）：预处理确认 `USER/src/main.c` 与 `USER/src/interrupts_cw32l010.c` 经 `main.h` 包含该头文件；`measure.c` 目前未包含（本次任务未要求，后续 ITEM 接入）。
- 说明：`SENSOR_*` 同名前缀在仓库其它无关示例（`Examples/sensoraiwrite/...`）与 CH592 官方库中存在，但不在本工程构建范围内，不构成冲突。

### 3.4 GNU 交叉编译 + 与基线逐字节比对（通过，核心）

命令：`sh gcc/build.sh`（工程自带脚本，Cortex-M0+，`-O1 -Wall -Wextra`，全工程 TU）。

| 项 | HEAD（ab5c5b2） | 基线（9e90eb5，独立 git worktree 构建） |
|---|---|---|
| 编译/链接 | 0 error | 0 error |
| warning 数 | 29（全部来自既有代码/厂商库） | （同源代码，未变） |
| sensor_config.h 相关告警 | **0** | 不适用 |
| FLASH | 33,896 B（51.72%） | 33,896 B（51.72%） |
| RAM used | 1,960 B（47.85%） | 1,960 B（47.85%） |
| `arm-none-eabi-size` text/data/bss | 33896 / 84 / 1876 | 33896 / 84 / 1876 |
| `sensor_fw.bin` MD5 | `46727d6446edb4adb0c87d26c7933f01` | `46727d6446edb4adb0c87d26c7933f01` |
| `sensor_fw.hex` MD5 | `0da7d963b50d16c2e1bb4fc5d80ce9da` | `0da7d963b50d16c2e1bb4fc5d80ce9da` |

基线为在临时 worktree（`git worktree add --detach <tmp> 9e90eb5`）中独立构建，未复用实现者产物。**固件二进制逐字节相同**，独立证明「加入该头文件后工程可交叉编译且不产生任何代码/数据变化」。

### 3.5 生产编译器与工程 include 路径（通过）

- Keil MDK 工程 include 路径含 `..\USER\inc`（`MDK/Project.uvprojx: <IncludePath>...;..\USER\inc;..\UM2005C;..\COMMON`）。
- IAR 工程 include 路径含 `$PROJ_DIR$\..\USER\inc`（`EWARM/*.ewp`）。
- 用 ARM Compiler 5.06u7 + 工程固定的 CMSIS 5.9.0 单独编译实际解析该头文件的 TU：

```
armcc --cpu=Cortex-M0+ --c99 -I ../../../Libraries/inc -I USER/inc -I UM2005C -I COMMON \
      -I ../../IdeSupport/MDK -I <CMSIS 5.9.0 Core Include> -c <file>
USER/src/main.c                 : 0 error（经 main.h 包含 sensor_config.h）
USER/src/interrupts_cw32l010.c  : 0 error（经 main.h 包含 sensor_config.h）
USER/src/sf_i2c.c               : 0 error
USER/src/measure.c              : 0 error, 1 warning（既有代码）
```

结论：新头文件对量产 ARM Compiler 5 兼容，未破坏既有 TU 编译。

### 3.6 宿主机 L0 既有回归（通过）

```
CC=<mingw64 gcc 12.2.0> sh test/build_test.sh
==== result: 56 passed, 0 failed ====
```

本项未改动 `fw_core.c`/`history.c`/`host_sensor_core_test.c`，56/56 与基线一致。

### 3.7 变更范围审查（通过）

`git show --stat ab5c5b2`：仅 5 个文件 —— `USER/inc/sensor_config.h`（新增）、`USER/inc/main.h`（+1 行 include，位于既有 include 段）、`artifacts/firmware_implementation.md`、`evidence/build.md`、`file_manifest.txt`。**无任何 .c/.ld/工程文件被修改**，无夹带无关改动，与"仅新增配置头"的声明一致。

## 4. 反证与反向检查

| 检查 | 结果 |
|---|---|
| 是否存在与声明不符的"假验证"（例如头文件未被任何 TU 使用） | 否：`main.c`/`interrupts_cw32l010.c` 预处理确认包含 |
| 是否把未标定阈值当作已验收 | 否：`LIGHT_DARK_CALIBRATED=0` 且注释明确要求实板标定后回填（FWR-OPEN-1） |
| 是否引入新的编译告警 | 否：0 条告警指向该头文件；交叉编译告警均来自既有代码/厂商库 |
| 是否存在同值常量重复定义（破坏"唯一配置点"） | 本工程范围内为 0 |
| 二进制是否真的无变化 | 是：与基线 `.bin`/`.hex` MD5 完全相同 |

## 5. 覆盖边界声明（本轮未验证的内容）

- 未在真实传感器板上做任何 I²C/光照/节拍/上报/功耗验证（无非硬件可测对象，且本项不涉及行为）。
- 未烧录、未使用串口/逻辑分析仪/示波器；`mdk_flash` 未调用（无需下载，且本项无板上行为可观测）。
- Keil 完整工程构建未成功（见 §6），因此"MDK 工程端到端编译通过"**未**被证明；已用 GNU 交叉编译 + AC5 单 TU 编译替代覆盖，并如实标注。
- 光照阈值、GXHT40 驱动、条件上报判定等属后续 ITEM，本证据不作为其通过依据。

## 6. 发现的非本项问题（环境/工具链，登记为交接，不计入本项缺陷）

`mdk_build`（action=build）在**当前环境**下失败，日志摘录：

```
*** Using Compiler 'V5.06 update 7 (build 960)'
compiling system_cw32l010.c...
C:\Users\kason\AppData\Local\Arm\Packs\ARM\CMSIS\6.3.0\CMSIS\Core\Include\cmsis_compiler.h(279): error: #35: #error directive: Unknown compiler.
".\output\exe\Project.axf" - 13 Error(s) ...
```

定位（独立复现，证明与 ITEM-001 无关）：

| 复现 | 结果 |
|---|---|
| 未改动的厂商文件 `Libraries/src/system_cw32l010.c` + CMSIS **6.3.0** 头 | 同样 `Unknown compiler` 错误 |
| 同一文件 + 工程记录的固定包 CMSIS **5.9.0** 头 | 0 错误，产出 .o |
| 本工程 `git show HEAD:MDK/output/exe/Project.build_log.htm`（导入基线，早于本次改动） | 上次成功构建使用 `ARM.CMSIS.5.9.0`，`0 Error(s)`；本次 UV4 解析到了 6.3.0 |

即：Keil RTE/pack 解析在本机选到 CMSIS 6.3.0，而 AC5 不被 CMSIS ≥6 支持，导致**所有** MDK 构建失败，与 `sensor_config.h` 无关。
处置建议（交接，非本能力执行）：在 Keil 工程中固定 CMSIS 5.9.0 包，或改用 ARM Compiler 6（AC6）并同步工程选项；修复前 `mdk_flash`/`mdk_build` 不能作为量产构建证据。

**仓库卫生**：`mdk_build` 生成的构建产物（`MDK/output/exe/*` 被跟踪文件）与 `MDK/build.log`、`test/host_sensor_core_test.exe` 已在验证后清理/还原，验证结束时 `git status` 为空。

## 7. 结论

ITEM-001 的声明行为经独立复现全部成立：配置头存在且取值正确、自包含、对 AC5 与 GNU 交叉编译均可用、为工程实际包含、无重复配置点，且固件二进制与改动前**逐字节一致**。既有 L0 回归无退化。

**判定：TEST_PASS**。残留项为 §6 的既有 Keil/CMSIS 工具链环境问题（已登记交接，不影响本项判定）与后续 ITEM 的行为验证（不在本项范围）。

---

# ITEM-002 验证（`sf_i2c` 无寄存器地址总线原语）

验证对象：任务项 **ITEM-002**（命令写 + 连续读原语，ACK 失败可判断；既有 i2c 函数语义与调用方式不变）
被测提交：`306c63d`；源码基线：`5a6210a`
结论：**TEST_PASS**

## A. 选测说明

| 选测项 | 理由 |
|---|---|
| 源码 diff 纯增量审查 | 直接检验「既有 i2c 函数语义与调用方式不变」 |
| 独立 mock 总线协议测试（自研，不复用实现者测试） | 检验「命令写 / 连续读」的线上字节、START/STOP 边界、主机 ACK/NACK 位置与返回值 |
| 既有函数机器码逐指令比对（改动前后两个 elf） | 比 diff 更强的回归证据：证明既有函数编译结果未被改变 |
| GNU 交叉编译 + 符号/体积差分 | 可编译、增量只来自新增函数、无既有代码膨胀 |
| ARM Compiler 5（AC5）单 TU 编译 | 量产工具链兼容性（sf_i2c.c 本次被修改） |
| 宿主机既有 L0 回归 | 无附带回归 |

不适用（后续 ITEM 才存在可测对象）：GXHT40 驱动、光照、节拍、条件上报、端到端与功耗用例。真实电气/时序（上升沿、上拉、时钟频率）、GXHT40 tMEAS 等待与器件地址变体属 TD-002 T-L2-01/02/03/07，须在真实传感器板上用逻辑分析仪完成，本轮**不具备**该条件，不在此判定。

## B. 源码变更范围（通过）

`git show --numstat 306c63d`：`USER/src/sf_i2c.c` **62 insertions / 0 deletions**；`USER/inc/sf_i2c.h` **8 / 0**。即对既有函数无任何行修改，仅在文件尾部新增 `i2c_write_cmd` / `i2c_read_bytes` 及声明。

## C. 独立 mock 总线协议测试（通过，核心）

自研位级 mock 总线 + mock 从机，直接链接**未打桩的** `USER/src/sf_i2c.c`（与实现者的 `test/host_sf_i2c_bus_check.c` 相互独立）。harness 入库于 `.../gpio_input_output/test/host_sf_i2c_verify_ev.c`。

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output
<mingw64 gcc> -std=c11 -Wall -Wextra -I USER/inc test/host_sf_i2c_verify_ev.c \
              USER/src/sf_i2c.c -o <tmp>/i2c_verify.exe && <tmp>/i2c_verify.exe
=> ==== result: 35 passed, 0 failed ====   (编译 0 警告)
```

原始线上转录（本 harness 输出）：

```
T1  i2c_write_cmd(0x88,0xFD) 有从机 : START TX(88) ACK TX(FD) ACK STOP
T2  i2c_write_cmd 无从机应答       : START TX(88) NACK STOP            -> SF_I2C_TIMEOUT
T6b 地址 ACK、命令字节 NACK         : START TX(88) ACK TX(FD) NACK STOP STOP -> SF_I2C_TIMEOUT
T3  i2c_read_bytes(0x89,6)         : START TX(89) ACK RX(61) M-ACK RX(9C) M-ACK RX(2B) M-ACK RX(8A) M-ACK RX(44) M-ACK RX(75) M-NACK STOP
T4  i2c_read_bytes 无从机应答       : START TX(89) NACK STOP            -> SF_I2C_TIMEOUT
T6  i2c_read_bytes 传写形式地址 0x88: START TX(89) ACK RX(11) M-NACK STOP
T7  回归 i2c_write_multi_byte       : START TX(70) ACK TX(AC) ACK TX(33) ACK TX(00) ACK STOP
T8  回归 i2c_read_multi_byte        : START TX(70) ACK TX(71) ACK START TX(71) ACK RX(A0) M-ACK ... RX(A4) M-NACK STOP
```

| 断言组 | 结果 |
|---|---|
| 命令写：1×START/1×STOP、线上 `0x88`→`0xFD`（无寄存器地址）、两字节均 ACK、从机仅收到命令字节、无读字节、结束后总线空闲高 | PASS |
| 命令写失败：地址 NACK → `SF_I2C_TIMEOUT`、已发 STOP、**命令字节不再发送**（快速失败）、总线释放 | PASS |
| 命令写失败：命令字节 NACK → `SF_I2C_TIMEOUT`、总线释放 | PASS |
| 连续读：1×START/1×STOP、首字节 `0x89`、6 字节与从机数据逐字节一致、**前 5 字节主机 ACK、末字节 NACK**、总线释放 | PASS |
| 连续读失败：读地址 NACK → `SF_I2C_TIMEOUT`、无数据字节、已发 STOP（不卡总线） | PASS |
| `length==0`：返回 SUCCESS 且**无任何总线动作** | PASS |
| 地址形式：`I2C_READ()` 掩码使写形式 0x88 与读形式 0x89 均得 0x89；与 `sensor_config.h` 的 `GXHT40_ADDR_READ_A=0x89` 兼容 | PASS |
| 回归：既有 `i2c_write_multi_byte` / `i2c_read_multi_byte`（含重复 START）字节序列与末字节 NACK 不变 | PASS |

实现者自带 harness 也被独立执行确认：`gcc ... test/host_sf_i2c_bus_check.c USER/src/sf_i2c.c` → **15 passed, 0 failed**（不以其自报为唯一依据）。

## D. 既有函数机器码等价（通过，最强回归证据）

在临时 worktree 中独立构建基线 `5a6210a`，与 HEAD 构建产物逐函数比对（`arm-none-eabi-objdump -d --disassemble=<fn>`，归一化重定位地址后逐指令比较）：

| 既有函数 | 结果 |
|---|---|
| `i2c_init` / `i2c_start` / `i2c_stop` / `i2c_write_byte` / `i2c_read_byte` | 指令完全相同 |
| `i2c_write_multi_byte` / `i2c_read_multi_byte` | 指令完全相同 |
| `i2c_write_multi_byte_16bit` / `i2c_read_multi_byte_16bit` / `i2c_obj_find` | 指令完全相同（仅重定位目标地址随链接位置变化） |

`arm-none-eabi-nm --print-size` 对照：上述函数**大小逐一相等**；新增符号只有 `i2c_write_cmd`（0x32=50 B）与 `i2c_read_bytes`（0x72=114 B）。

## E. 构建与体积（通过）

| 项 | 基线 5a6210a | HEAD 306c63d |
|---|---|---|
| 交叉编译 | 0 error / 29 warning | 0 error / 29 warning（无新增） |
| FLASH | 33,896 B | 34,060 B（**+164 B = 50+114，恰为两个新函数**） |
| RAM | 1,960 B | 1,960 B（不变） |
| `sf_i2c.c` 相关告警 | 2 条既有 `err may be used uninitialized`（`i2c_write_multi_byte`/`_16bit`，line 295/417，非本次引入） | 同左，行号与数量不变 |

AC5（`armcc --cpu=Cortex-M0+ --c99`，工程固定 CMSIS 5.9.0）单 TU 编译：`sf_i2c.c` 0 错误、`main.c` 0 错误、`measure.c` 0 错误（1 既有告警）。宿主机 L0 回归 `test/build_test.sh` → **56 passed, 0 failed**。

## F. 观察项（非缺陷，记录备查）

1. **NACK 失败路径会出现两个 STOP**（`i2c_wait_ack` 超时内已发一次，调用方再发一次；T6b 转录 `... NACK STOP STOP`）。I²C 总线上空闲态重复 STOP 无副作用，且与既有 `i2c_write_multi_byte` 的写法一致；本项不判缺陷，后续如需精简可随 GXHT40 驱动一并评估。
2. **`length==0` 短路返回 `SF_I2C_SUCCESS` 且不发总线**：任务未定义该边界，实现选择明确且可测；记录为行为约定。
3. 调用方需注意 `i2c_read_bytes` 的地址参数可用写/读形式任一种（内部掩码），与 `sensor_config.h` 的 `0x88/0x89` 常量兼容——后续 ITEM-003 接入时不应出现地址形式错用。

## G. ITEM-002 判定

「命令写（START→地址+W→命令→STOP）」「连续读（START→地址+R→N 字节→末字节 NACK→STOP）」「从机 ACK 失败作为可判断返回值」「既有 i2c 函数语义与调用方式不变」四项声明均经独立复现成立，且既有函数机器码逐指令未变。

**判定：TEST_PASS**（真实板电气/时序与 GXHT40 器件交互不在本项范围，交接 TD-002 T-L2-01/02/03/07）。

---

# ITEM-003 验证（GXHT40 驱动 `gxht40.c/.h`）

验证对象：任务项 **ITEM-003**（地址探测 0x44/0x45、`0xFD` 高重复率测量、tMEAS 等待、读 6 字节、温度/湿度字分别 CRC-8 校验、x10 换算与截断、失败返回失败码且不修改输出）
被测提交：`3bbd292`；源码基线：`1712162`
结论：**TEST_PASS**

## A. 选测说明与变更范围

| 选测项 | 理由 |
|---|---|
| 新增文件 diff 审查 | `git show --numstat 3bbd292`：`gxht40.h` 48/0、`gxht40.c` 183/0、`test/host_gxht40_check.c` 308/0，**均为纯新增，无既有源码修改** |
| 独立参考帧生成（Python 独立实现 CRC/换算） | 不让被测驱动的 CRC/换算参与构造期望值，避免同源验证 |
| 自研 mock I²C + mock GXHT40 器件（链接真实 `gxht40.c`/`sf_i2c.c`） | 独立复现地址探测/命令/帧顺序/双字 CRC/重试/失败语义 |
| 失败不改输出（逐失败码） | 任务硬性要求，需直接断言 |
| GNU 交叉编译 + AC5 单 TU 编译 | 可编译性与量产编译器兼容（gxht40.c 尚未加入 Keil/IAR 源列表，属 ITEM-011） |

不适用（后续 ITEM 才有可测对象）：光照通路、采样节拍、条件上报判定、端到端与功耗。**真实 tMEAS 等待下界、地址变体实物确认、电气与真实器件 CRC 实读**属 TD-002 T-L2-01/02/03/06/07，本轮无逻辑分析仪/实物板，不在此判定。

## B. 独立参考帧与 CRC 变体确认（通过）

用独立 Python 实现（poly 0x31、init 0xFF、MSB-first、无反转、xorout 0x00）生成期望值：

```
CRC-8({0xBE,0xEF}) = 0x92   （与 gxht40.pdf / FD-002 §6.1 参考向量一致）
CRC-8("123456789") = 0xF7   （即 CRC-8/NRSC-5 标准校验值，确认变体无歧义）
换算：raw_t=0x0000 -> -450（超范围）; 0xFFFF -> 1300（超范围）; raw_h=0 -> 0; 0xFFFF -> 1190 -> 截断 1000
```

用于 mock 器件的 6 字节帧（**由上述独立脚本生成后硬编码，harness 不调用驱动的 CRC/换算**）：

| 向量 | 6 字节 | 期望结果 |
|---|---|---|
| 手册参考 0xBEEF/0x1234 | `BE EF 92 12 34 37` | OK，temp=855，hum=29 |
| 常温 25.0 ℃/50 %RH | `66 66 93 72 B0 DC` | OK，250 / 500 |
| 负温 −10.0 ℃/0 % | `33 33 88 00 00 81` | OK，−100 / 0 |
| 湿度越界（raw 0xFFFF） | `66 66 93 FF FF AC` | OK，hum 截断 1000 |
| 湿度 raw 0 | `66 66 93 00 00 81` | OK，hum 0 |
| 量程低（raw_t 0） | `00 00 81 72 B0 DC` | `GXHT40_ERR_RANGE`（换算 −450） |
| 量程高 125.1 ℃ | `F8 CA 32 72 B0 DC` | `GXHT40_ERR_RANGE`（换算 1251） |
| 温度字 CRC 错 | `66 66 00 72 B0 DC` | `GXHT40_ERR_CRC` |
| 湿度字 CRC 错 | `66 66 93 72 B0 00` | `GXHT40_ERR_CRC` |

字节序独立验证：若温度字字节序颠倒，0xEFBE 会得到 1189 而非 855；若湿度字颠倒，0x3412 会得到 194 而非 29——实际得到 855/29，证明 `T_MSB,T_LSB,T_CRC,RH_MSB,RH_LSB,RH_CRC` 顺序正确。

## C. 独立 mock 器件功能测试（通过，核心）

harness 入库于 `.../gpio_input_output/test/host_gxht40_verify_ev.c`（链接**未打桩的** `gxht40.c` + `sf_i2c.c`；`delay_ms` 为计数测试桩以断言 tMEAS 等待）。

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test
<mingw64 gcc> -std=c11 -Wall -Wextra -Wno-int-to-pointer-cast \
  -I../USER/inc -I../COMMON -I../../../../Libraries/inc -I<CMSIS 5.9.0 Core Include> \
  host_gxht40_verify_ev.c ../USER/src/gxht40.c ../USER/src/sf_i2c.c -o gxht40_verify && ./gxht40_verify
=> ==== result: 42 passed, 0 failed ====   (0 编译告警)
```

| 组 | 独立断言 | 结果 |
|---|---|---|
| 手册向量与帧封装 | OK；855/29；命令恰为 0xFD 一次；写地址 `0x88`、读地址 `0x89`；探测地址缓存 0x44；每次测量一次 10 ms tMEAS 等待 | PASS |
| 换算 | 25.0 ℃/50 %→250/500；−10.0 ℃/0 %→−100/0（符号保留）；hum raw 0xFFFF→1000；raw 0→0 | PASS |
| 地址探测与缓存 | 器件仅在 0x45：先 `0x88`（无应答）再 `0x8A`（成功）、读 `0x8B`；缓存 0x45；第二次测量直接用 `0x8A`，**不再探测 0x88** | PASS |
| 读 NACK 重试 | 前 2 次读地址 NACK、第 3 次成功；**命令未重发**（转换中）；三次读尝试；两次 1 ms 重试等待 | PASS |
| 读 NACK 用尽 | `GXHT40_ERR_IO`；输出不变；命令尝试 ≤ `GXHT40_MEAS_RETRY`；读尝试 ≤ `MEAS_RETRY×READ_RETRY` | PASS |
| CRC 错（温度字/湿度字） | 均 `GXHT40_ERR_CRC`；输出不变；**恰好重测 `GXHT40_MEAS_RETRY`(=3) 次** | PASS |
| 超范围（低/高） | 均 `GXHT40_ERR_RANGE`；输出不变 | PASS |
| 无器件 | `GXHT40_ERR_NO_DEVICE`；`0x88` 与 `0x8A` 均被探测；输出不变；探测次数有界 | PASS |
| 入参 | NULL 温度指针 / NULL 湿度指针 / 未绑定总线 均 `GXHT40_ERR_PARAM` | PASS |
| 命令白名单 | 全程共 23 次已发命令**全部为 `0xFD`**（无 `0x94` 软复位、无 `0x39` 加热器、无 `0xE0/0xF6` 低/中重复率） | PASS |

实现者自带 harness 也被独立执行确认：`host_gxht40_check.c` → **27 passed, 0 failed**（不以其自报为唯一依据）。

## D. 失败不修改输出（逐失败码，通过）

每类失败均先写入哨兵值（`temp=0x1111`、`hum=0x2222`，或 `0x7F7F`）再调用：`ERR_IO`、`ERR_CRC`（温度字/湿度字）、`ERR_RANGE`（低/高）、`ERR_NO_DEVICE`、`ERR_PARAM` 全部保持哨兵值不变。代码层核对：输出指针仅在完全成功分支末尾写入。

## E. 构建与对现网固件的影响（通过）

| 项 | 结果 |
|---|---|
| GNU 交叉编译（`gcc/build.sh`，含 `USER/src/gxht40.c`） | 0 error；29 warning（与基线同数，**无一条指向 `gxht40.c`**） |
| `gxht40.o` | text 460 B / data 0 / bss 5；符号 `gxht40_init`、`gxht40_measure`、`gxht40_detected_addr7`（`gxht40_crc8`、`gxht40_start_and_read` 为 static） |
| 固件镜像 | FLASH 34,060 B、RAM 1,960 B；`sensor_fw.bin` MD5 **与 ITEM-002 构建完全相同**（`67917abd…`）——模块尚未被调用，链接器 `--gc-sections` 丢弃，对现网固件零影响 |
| AC5（`armcc --cpu=Cortex-M0+ --c99`，CMSIS 5.9.0） | `gxht40.c` 0 error；`main.c` 0 error |
| 宿主机 L0 回归 | `test/build_test.sh` → 56 passed, 0 failed（未涉及，回归确认） |

## F. 观察项与交接

1. **CRC/换算实现会重复（交 ITEM-005）**：`gxht40.c` 内 `gxht40_crc8` / `gxht40_conv_*` 为 static；而 ITEM-005 要求在 `fw_core.c` 提供可宿主机直测的 `fw_crc8_gxht` / `gxht40_raw_to_x10` 等纯函数。若两份实现并存且不互调，存在后续漂移风险。建议 ITEM-005 完成后让 `gxht40.c` 复用 `fw_core.c` 的纯函数（或至少由 ITEM-005 的宿主测试锁定两者一致性）。**本项不因此判失败**（当前单份实现自洽且经独立验证）。
2. **Keil/IAR 源文件列表尚未包含 `gxht40.c`**（`Project.uvprojx`/`project.ewp` 中 `gxht40` 匹配数为 0）。这属 ITEM-011（“MDK/IAR 工程源文件列表与新增/移除源文件保持一致”）的范围，故在 ITEM-003 不判缺陷；但意味着**当前 Keil/IAR 构建不会编译该驱动**，ITEM-011 必须补上。
3. **无器件时的探测次数**：缓存失效后每次外层尝试会依次试 0x44/0x45，最坏 3×2=6 次命令（每次 10 ms 等待）≈ 60 ms 额外活动；有界且仅发生在器件缺失/故障场景，记录备查。
4. **真实 tMEAS 等待与地址变体**未在实物验证（无板/无逻辑分析仪）：软件侧已确认“命令→等待 10 ms→读”顺序与“读 NACK 不重发命令”，但实测下界（≥8.3 ms）与 0x44/0x45 实物确认仍需 TD-002 T-L2-01/02/03。

## G. ITEM-003 判定

地址探测（0x44/0x45，8 位 0x88/0x8A 写、0x89/0x8B 读）、`0xFD` 高重复率测量与 tMEAS 等待、6 字节读取与温度/湿度字分别 CRC-8（poly 0x31/init 0xFF）校验、x10 整数换算（含负温、0..1000 截断、−40.0..125.0 ℃ 有效域）、有界重试、失败返回失败码且不修改输出——全部经**独立于被测实现的参考帧与 mock 器件**复现成立。

**判定：TEST_PASS**。
