# 固件实现（FWI-002）

状态：固件实现（firmware_engineer.firmware_implementation），按任务队列逐项推进
依据：FD-002 `artifacts/firmware_design.md`、FWR-002 `artifacts/firmware_requirements.md`、IC-002、`artifacts/test_design.md`、`artifacts/firmware_tasks.yaml`
范围：仅 Runtime 逐次指派的当前任务项；未指派的后续项不在本轮实现。

---

# 任务项 ITEM-001（已完成，独立验证 TEST_PASS）

**ITEM-001**：新增 `sensor_config.h` 作为唯一配置点：定义 PB04/PB05 引脚与 ADC 通道、GXHT40 地址/命令/重试/等待参数、光照明暗阈值与滞回、3 分钟采样计数值、0.9℃ 下降与 35.0℃ 超温上报常量；加入该头文件后工程仍可交叉编译。

设计映射：FD-002 §3.1（新增文件）、§11.1/§11.4（单一配置点、引脚所有权）。

## 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/inc/sensor_config.h` | 新增 | 传感器固件唯一配置点（见下表） |
| `.../USER/inc/main.h` | 修改 | 增加 `#include "sensor_config.h"`，使工程翻译单元实际解析该头文件（证明「加入后仍可交叉编译」非空验证） |

### `sensor_config.h` 定义的配置（只定义，未接入业务逻辑）

| 组 | 宏 | 值 | 来源 |
|---|---|---|---|
| 节拍 | `SENSOR_RTC_TICK_PERIOD_MIN` / `SENSOR_SAMPLE_TICKS` | 1 / 3（=3 分钟） | readme 修改点 3；FD-002 §5.2 |
| GXHT40 地址 | `GXHT40_ADDR_7BIT_A/B`、`GXHT40_ADDR_WRITE_A/READ_A/WRITE_B/READ_B` | 0x44/0x45 → 0x88/0x89/0x8A/0x8B | gxht40.pdf；FD-002 §2.2 |
| GXHT40 命令 | `GXHT40_CMD_MEASURE_HIGH_REP` = 0xFD（另备 0xF6/0xE0；软复位 0x94 标注周期路径禁用） | — | 手册表 9；FD-002 §11.10 |
| GXHT40 时序/重试 | `GXHT40_MEASURE_WAIT_MS`=10、`GXHT40_READ_RETRY`=5（间隔 1 ms）、`GXHT40_MEAS_RETRY`=3、`GXHT40_POWER_ON_WAIT_MS`=2 | — | 手册 tMEAS.H max 8.3 ms；FD-002 §10 |
| GXHT40 校验/换算 | `GXHT40_CRC8_POLY`=0x31、`GXHT40_CRC8_INIT`=0xFF、换算分子/偏移/取整/量程常量 | — | 手册 §7.3/§7.5；FD-002 §6.1/§6.2 |
| 光照引脚/ADC | `LIGHT_POWER_PORT/PIN`=PB05、`LIGHT_ADC_PORT/PIN`=PB04、`LIGHT_ADC_INPUT_CHANNEL`=`ADC_InputCH11`、`LIGHT_ADC_FULL_SCALE`=4095、`LIGHT_ADC_CLK_DIV`=`ADC_Clk_Div8`、`LIGHT_ADC_SAMPLE_TIME`=`ADC_SampTime390Clk`、`LIGHT_ADC_SAMPLES`=8 | — | 网表；`cw32l010_adc.h`（PB04=AIN11）；FD-002 §2.3 |
| 光照阈值/滞回 | `LIGHT_DARK_ENTER`=350、`LIGHT_DARK_EXIT`=250、`LIGHT_DARK_HYSTERESIS_STEP`=100、`LIGHT_DARK_CALIBRATED`=0 | — | FD-002 §6.3；FWR-OPEN-1（默认值待实板标定回填） |
| 上报判定 | `SENSOR_REPORT_DROP_X10`=9（0.9℃）、`SENSOR_REPORT_HIGH_X10`=350（35.0℃） | — | readme 修改点 4；IC-002 §2；FD-002 §6.4 |
| 433 链路 | `SENSOR_RF_FRAME_LEN`=10、`SENSOR_RF_TX_TIMEOUT_MS`=200、`SENSOR_RF_TX_RETRY`=3 | — | FD-002 §8.1/§10 |
| 调试钩子 | `SENSOR_TEST_TRACE` 默认 0（可 `-D` 覆盖） | — | test_design.md §2 观测钩子请求 |

### 预期行为

1. 上述常量只在 `sensor_config.h` 定义一次；其它文件只引用宏（FD-002 §11.1）。
2. 头文件自包含，可被后续 `gxht40.c`/`light.c`/`measure.c` 直接包含。
3. 加入头文件后，工程交叉编译与链接结果与改动前**逐字节一致**（宏不产生代码）。
4. `SENSOR_TEST_TRACE` 默认关闭，量产构建行为不变。

### 验证

交叉编译 0 错误、FLASH/RAM 与基线逐字节一致（33,896 B / 1,960 B）；配置头 43 条编译期断言通过；宿主机 L0 回归 56/56。详见 `evidence/build.md`，独立验证结论见 `evidence/test.md`（embedded_verification，TEST_PASS）。

---

# 任务项 ITEM-002（已完成，独立验证 TEST_PASS）

**ITEM-002**：扩展 sf_i2c 总线原语：提供不带寄存器地址的命令写（START→地址+W→命令→STOP）与连续读（START→地址+R→N 字节→NACK→STOP），并把从机 ACK 失败作为可判断的返回值；既有 i2c 函数语义与调用方式不变。

设计映射：FD-002 §3.2（`sf_i2c` 增加两个总线原语）、§2.2（GXHT40 无 clock stretching、数据未就绪返回 NACK）、§10（异常策略）。测试映射：TD-002 T-L2-01/T-L2-02/T-L2-03/T-L2-07 所需的软件侧时序。

## 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `.../USER/inc/sf_i2c.h` | 修改（+8 行） | 声明 `i2c_write_cmd()` 与 `i2c_read_bytes()`，注释说明序列与返回值语义 |
| `.../USER/src/sf_i2c.c` | 修改（+62 行，**0 行删除**） | 实现两个原语；既有函数逐字节未改动 |

### `i2c_write_cmd(dev, slave_addr, cmd)` → `sf_i2c_err`

- 序列：`START → I2C_WRITE(slave_addr) → cmd → STOP`（**不发送寄存器地址**，适配 GXHT40 命令 0xFD）。
- 地址字节或命令字节未获 ACK → 返回 `SF_I2C_TIMEOUT`；`i2c_wait_ack()` 超时路径已发 STOP 释放总线，故不再重复 STOP。
- 成功返回 `SF_I2C_SUCCESS`。

### `i2c_read_bytes(dev, slave_addr, buf, length)` → `sf_i2c_err`

- 序列：`START → I2C_READ(slave_addr) → length 字节（前 length-1 字节主机 ACK，末字节主机 NACK）→ STOP`。
- 读地址字节未获 ACK（GXHT40 转换未完成时返回 NACK）→ 返回 `SF_I2C_TIMEOUT`，总线已释放。
- `length == 0` 或空指针 → 直接返回 `SF_I2C_SUCCESS`，**不产生任何总线动作**。
- 成功返回 `SF_I2C_SUCCESS`。

### 预期行为（可检验）

1. 两个原语都不发送寄存器地址，字节序为标准 I²C 地址头 + 数据。
2. 从机 ACK 结果作为返回值可判断：`SF_I2C_SUCCESS` / `SF_I2C_TIMEOUT`。
3. 连续读末字节发 NACK 后 STOP（与 TD-002 T-L2-03 一致）。
4. 既有 `i2c_*` 函数源码 0 行删除，语义与调用方式不变。
5. 工程交叉编译通过，新增符号进入镜像，`-Wall -Wextra` 无新增告警。

**本项不包含**：GXHT40 协议层（地址探测、0xFD 命令调用、CRC、换算）属 ITEM-003；光照采样属 ITEM-004；板上总线波形验证属嵌入式测试（TD-002 T-L2）。

## 验证（本轮实际执行）

- 交叉编译：`gcc/build.sh` → **0 错误**；`FLASH 34,060 B (51.97%)`（较 ITEM-001 +164 B）、`RAM 1,960 B`（不变）。无新增告警指向新代码（既有 `i2c_write_multi_byte`/`_16bit` 的 `err may be used uninitialized` 告警为改动前既有，未触碰）。
- 符号：`arm-none-eabi-nm` 显示 `i2c_write_cmd`、`i2c_read_bytes` 均为全局符号并已进入 `sensor_fw.elf`；既有符号地址集合未消失。
- 变更范围：`git diff --numstat` 为 `62 insertions / 0 deletions`（sf_i2c.c）、`8 insertions`（sf_i2c.h），既有函数未被改动。
- 宿主机 mock 总线自检：15/15 通过，覆盖命令写序列、连续读 6 字节（5×ACK + 末字节 NACK）、从机不响应返回 `SF_I2C_TIMEOUT` 且释放总线、`length==0` 无总线动作。详见 `evidence/driver_test.md`。
- 宿主机 L0 既有回归：`test/build_test.sh` → 56 passed / 0 failed（未触碰被测算例）。

## 交接与依赖

- ITEM-003（GXHT40 驱动）将直接调用本原语；`i2c_read_bytes` 的 NACK 返回即"转换未完成需等待后重读"的判据（FD-002 §10）。
- 板上 I²C 电气/时序（地址字节、tMEAS 间隔、6 字节顺序、总线卡死恢复）仍须按 TD-002 T-L2-01/02/03/07 由嵌入式测试在真实硬件上验证；本项只提供软件侧序列与返回值。

---

# 任务项 ITEM-003（已完成，独立验证 TEST_PASS）

**ITEM-003**：实现 GXHT40 驱动 `gxht40.c/.h`：上电探测 0x44/0x45 地址（8 位写 0x88/0x8A、读 0x89/0x8B），发送 0xFD 高重复率测量命令，按 tMEAS 上限等待后读回 6 字节，用 CRC-8（poly 0x31、init 0xFF、无反转、xorout 0x00）分别校验温度字与湿度字，输出 `temp_x10`（int16，−400..1250）与 `hum_x10`（uint16，0..1000，越界截断）；失败返回失败码且不修改输出。

设计映射：FD-002 §3.1（新增 `gxht40.c/.h`）、§6.1/§6.2（CRC-8 与整数换算）、§10（异常策略）、§11.3/§11.9/§11.10（无 clock stretching、不用加热器、周期路径不软复位）。

## 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `.../USER/inc/gxht40.h` | 新增 | `gxht40_status_t` 结果码、`gxht40_init()`、`gxht40_measure()`、`gxht40_detected_addr7()` |
| `.../USER/src/gxht40.c` | 新增 | 地址探测与缓存、`0xFD` 命令、tMEAS 等待、6 字节读取（读 NACK 有界重读）、双字 CRC-8、整数换算与量程判定 |
| `.../test/host_gxht40_check.c` | 新增（测试载体） | 宿主机 mock 总线自检，27 项 |

无既有文件被修改。

### 行为要点

1. **地址探测与缓存**：首次测量依次试 `0x88`→`0x8A`（地址字节未被 ACK 即认为该地址无器件）；命中后缓存 7bit 地址，后续直接使用；缓存地址失效时重新探测。
2. **时序**：命令写（START→地址+W→`0xFD`→STOP）→ `delay_ms(GXHT40_MEASURE_WAIT_MS=10ms)`（≥ tMEAS.H max 8.3 ms）→ 连续读 6 字节（START→地址+R→前 5 字节主机 ACK + 末字节 NACK→STOP）。
3. **读 NACK = 转换未完成**：等待 1 ms 后重读，最多 `GXHT40_READ_RETRY=5` 次；不重发命令。
4. **双字 CRC**：`crc8(buf[0..1])==buf[2]` 且 `crc8(buf[3..4])==buf[5]`（poly 0x31、init 0xFF、无反转、xorout 0x00；手册参考 `CRC(0xBEEF)=0x92`）。CRC 错丢弃整帧并按 `GXHT40_MEAS_RETRY=3` 重测。
5. **换算（整数，无浮点）**：`temp_x10 = -450 + round(1750*S_T/65536)`；`hum_x10 = -60 + round(1250*S_RH/65536)` 后截断 0..1000。
6. **量程**：温度结果超出 −400..1250（即 −40.0..125.0 ℃）视为无效测量。
7. **失败不改输出**：仅在完全成功时写入 `*temp_x10`/`*hum_x10`；否则返回 `GXHT40_ERR_PARAM/NO_DEVICE/IO/CRC/RANGE` 之一，输出保持调用前值。
8. **命令白名单**：周期路径只发 `0xFD`；不发 `0x94` 软复位、不发加热器命令。

**本项不包含**：把驱动接入采样流程（ITEM-006）、光照采样（ITEM-004）、纯逻辑抽取到 `fw_core.c`（ITEM-005）、MDK/IAR 工程源文件列表注册（ITEM-011）。

## 验证（本轮实际执行）

- 交叉编译：`gcc/build.sh` → **0 错误，无告警指向 `gxht40.c`**；工程 FLASH/RAM 与 ITEM-002 相同（34,060 / 1,960 B），因为 `gxht40.o` 尚未被引用（链接器未拉入，接线属 ITEM-006）；`obj/gxht40.o` = text 460 B / bss 5 B。
- 驱动级自检：`test/host_gxht40_check.c` → **27 passed / 0 failed**，覆盖手册 CRC 参考向量、常温/负温换算、地址探测与缓存、读 NACK 重读、CRC 错重测上限、无器件、超范围、失败不改输出、命令白名单。详见 `evidence/driver_test.md`。
- 宿主机 L0 回归：`test/build_test.sh` → 56 passed / 0 failed。

## 交接与依赖

- ITEM-004（光照）与 ITEM-006（采样流程）将调用 `gxht40_measure()`；接线时需先 `bsp_i2c_init()` 并把 `i2c_obj_find("i2c0")` 结果交给 `gxht40_init()`（与现有 `measure.c` 用法一致）。
- ITEM-005 将把 CRC-8 与 x10 换算抽为 `fw_core.c` 的纯函数供宿主机直测；届时 `gxht40.c` 应改为调用该纯函数（避免两份实现）。
- 真实 tMEAS 下界、地址变体实物确认、电气与 CRC 实读仍属 TD-002 T-L2-01/02/03/06/07，由嵌入式测试在真实硬件验证。

---

# 任务项 ITEM-004（已完成，独立验证 TEST_PASS）

**ITEM-004**：实现光照通路 `light.c/.h`：采样时 PB05 输出高、稳定延时后用 PB04/AIN11 多次取样求均值，按配置阈值与滞回输出 DARK/LIT（无光=读数≥进入阈值），采样结束 PB05 置低。

设计映射：FD-002 §2.3（分压拓扑与极性）、§3.1（新增 `light.c/.h`）、§6.3（滞回判定）、§10（光敏失效处理）、§11.4/§11.5（引脚所有权、空闲电平）。

## 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `.../USER/inc/light.h` | 新增 | `light_init()`、`light_reset_state()`、`light_code_is_dark(code, prev_dark)`、`light_sample()`、`light_last_code()` |
| `.../USER/src/light.c` | 新增 | PB05 推挽输出(初始低) + PB04 模拟输入(AIN11) + ADC 单次转换配置；采样序列与滞回判定 |
| `.../USER/inc/sensor_config.h` | 修改（+2 行） | 新增 `LIGHT_ADC_EOC_GUARD`（ADC EOC 轮询上限，防转换挂死）；其余配置不变 |
| `.../test/host_light_check.c` | 新增（测试载体） | 宿主机滞回判定自检，17 项 |

### 行为要点

1. **引脚所有权**（FD-002 §11.4）：PB04 仅在本模块被配置为模拟输入（`GPIO_Init(..., GPIO_MODE_ANALOG)` 置 ANALOG 位，等价于 `PB04_ANALOG_ENABLE()`）；PB05 仅在本模块被配置为推挽输出。
2. **采样序列**：`PB05 = 高` → `delay_ms(LIGHT_SETTLE_MS=100ms)`（分压 RC + 光敏器件响应）→ `ADC_Enable()` → `LIGHT_ADC_SAMPLES=8` 次单次转换（`ADC_SoftwareStartConvCmd` + 轮询 EOC）→ `ADC_Disable()` → `PB05 = 低`（`LIGHT_IDLE_POWER_OFF`）。
3. **ADC 配置**：`ADC_Clk_Div8`（ADCCLK = PCLK/8 = 1 MHz）、`ADC_SampTime390Clk`（390 µs 采样保持，适配最高 5 MΩ 源阻抗）、通道 `ADC_InputCH11`（PB04）。
4. **均值**：对成功的转换取算术平均（`sum / ok`）；`light_last_code()` 保留最近均值供调试跟踪。
5. **滞回判定**（FD-002 §6.3）：已明时 `code >= LIGHT_DARK_ENTER(350)` 转 DARK；已暗时 `code <= LIGHT_DARK_EXIT(250)` 转 LIT；滞回带内保持原状态。语义：**无光 = code ≥ 进入阈值**（暗时节点电压高）。
6. **异常不阻塞**：单次 ADC 转换超时（`LIGHT_ADC_EOC_GUARD` 用尽）被丢弃；全部超时按满量程（器件开路 = 无光）处理，倾向上报且不死等。
7. **低功耗**：非采样期 PB05 = 低（分压无电流）、ADC 使能位关闭。

**本项不包含**：把光照接入采样流程（ITEM-006）、纯逻辑抽取到 `fw_core.c`（ITEM-005）、旧 hall/OPTCFG 对 PB04/PB05 的退役（ITEM-009）、MDK/IAR 工程源文件列表注册（ITEM-011）、光照阈值实板标定（FWR-OPEN-1 / TD-002 T-L3-03）。

## 验证（本轮实际执行）

- 交叉编译：`gcc/build.sh` → **0 错误**；总告警 29 条（与基线同数，**无一条指向 `light.c`**）；`obj/light.o` = text 364 B / bss 4 B。工程 FLASH/RAM 与 ITEM-003 相同（34,060 / 1,960 B），因 `light.o` 尚未被引用（链接器未拉入，接线属 ITEM-006）。
- 纯逻辑自检：`test/host_light_check.c` → **17 passed / 0 failed**，覆盖阈值/滞回方向与端点（0/349/350/351/4095 与 0/249/250/251/351/4095）与带内不抖动。详见 `evidence/driver_test.md`。
- 宿主机 L0 回归：`test/build_test.sh` → 56 passed / 0 failed。

## 交接与依赖

- ITEM-005 将把 `light_code_is_dark` 抽为 `fw_core.c` 纯函数供宿主机直测；届时 `light.c` 应改为调用该纯函数（避免两份实现），并从 `light.c` 移除同名定义。
- ITEM-006 接线时先 `light_init()`，采样周期内调用 `light_sample()`；需与 ITEM-009 配合：旧 `hall.c`/`optcfg.c` 目前仍会配置 PB04/PB05，必须在接线前退役，否则引脚所有权冲突。
- 光照阈值 `LIGHT_DARK_ENTER/EXIT` 仍为未标定默认值（`LIGHT_DARK_CALIBRATED=0`），按 TD-002 T-L3-03 实板标定后回填。

---

# 任务项 ITEM-005（已完成，独立验证 TEST_PASS）

**ITEM-005**：在 `fw_core.c` 中实现并保持不依赖 MCU 寄存器的纯逻辑：`fw_crc8_gxht`（参考向量 CRC(0xBEEF)=0x92）、GXHT40 原始字到 x10 的整数换算（含负温与 0-100%RH 截断）、`light_code_is_dark` 滞回判定、`sensor_decide_report` 判定 `((prev-cur)>9 且 DARK) 或 (cur>350)`（无前值时下降分支恒假）。

设计映射：FD-002 §3.2（fw_core 纯逻辑）、§6.1/§6.2/§6.3/§6.4（算法）、§11.1（单一配置点）。测试映射：TD-002 T-L0-01…T-L0-05。

## 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `.../USER/inc/fw_core.h` | 修改 | 新增 7 个纯函数声明（CRC-8、温度/湿度换算、量程判定、组合换算、光照滞回、上报判定） |
| `.../USER/src/fw_core.c` | 修改 | 实现上述纯函数；仅依赖 `sensor_config.h` 的纯数值段（定义 `SENSOR_CONFIG_NO_MCU`，不引 MCU 头） |
| `.../USER/inc/sensor_config.h` | 修改 | 拆为「纯数值段」+ `#ifndef SENSOR_CONFIG_NO_MCU` 的「MCU 引脚/外设段」；数值不变，仅重排 |
| `.../USER/src/gxht40.c` | 修改 | 删除内置 `gxht40_crc8`/`gxht40_conv_*`，改用 `fw_crc8_gxht`/`gxht40_raw_to_x10`（消除双份实现，落实 ITEM-003 观察项 F1） |
| `.../USER/inc/light.h`、`.../USER/src/light.c` | 修改 | `light_code_is_dark` 移入 `fw_core.c`；`light.c` 改为包含 `fw_core.h` 调用（消除双份实现） |
| `.../test/host_fw_core_pure_check.c` | 新增（测试载体） | 宿主机纯逻辑自检 36 项 |
| `.../test/host_gxht40_check.c`、`host_light_check.c` | 修改 | 链接新增 `fw_core.c`（驱动/光照已改为调用纯函数） |

### 接口（fw_core.h）

```c
uint8_t  fw_crc8_gxht(const uint8_t *p, uint16_t n);          /* poly 0x31, init 0xFF, 空长度 0xFF */
int16_t  gxht40_temp_raw_to_x10(uint16_t raw);                 /* -450 + round(1750*S/65536) */
uint16_t gxht40_hum_raw_to_x10(uint16_t raw);                  /* -60 + round(1250*S/65536), 截断 0..1000 */
bool     gxht40_temp_x10_valid(int16_t temp_x10);              /* -400..1250 */
bool     gxht40_raw_to_x10(uint16_t rt, uint16_t rh, int16_t *t, uint16_t *h);  /* 有效才写输出 */
bool     light_code_is_dark(uint16_t code, bool prev_dark);    /* ≥ENTER 转暗, ≤EXIT 转明 */
bool     sensor_decide_report(int16_t prev, bool have_prev, int16_t cur, bool dark);
```

### 行为要点

1. **无 MCU 依赖**：`fw_core.c` 只包含 `fw_core.h`/`params.h` 与 `sensor_config.h`（纯数值段），因此 `test/build_test.sh` 的 `gcc -I../USER/inc` 无需追加 MCU 头路径仍可编译（已回归 56/56）。
2. **上报判定**：`cur > 350` 优先返回 true；`cur == 350` 返回 false（IC-002 §2 的 `T<35` 与 `T>35` 在恰好 35.0 ℃ 均不成立）；无前值或非 DARK 返回 false；否则 `(prev-cur) > 9`。
3. **温度有效域**：`gxht40_raw_to_x10` 在温度超 -40.0..125.0 ℃ 时返回 false 且不写输出；`gxht40.c` 据此返回 `GXHT40_ERR_RANGE`（行为与 ITEM-003 一致，重测逻辑不变）。
4. **单一配置点保持**：阈值仍只在 `sensor_config.h` 定义；新增的 `SENSOR_CONFIG_NO_MCU` 只控制是否引入 MCU 头。

**本项不包含**：退役 OPTCFG/params/history 纯逻辑与其测试用例（ITEM-009/ITEM-010）、采样流程接线（ITEM-006）、工程文件注册（ITEM-011）。

## 验证（本轮实际执行）

- 交叉编译：`gcc/build.sh` → **0 错误、29 条告警（与基线同数）**；FLASH 34,352 B（较 ITEM-004 **+292 B**：新增纯函数已随 `fw_core.o` 入镜像）、RAM 1,960 B（不变）。
- 纯逻辑自检：`test/host_fw_core_pure_check.c` → **36 passed / 0 failed**，逐条覆盖 TD-002 T-L0-01…T-L0-05（含 T-L0-02 的 S=0/0xFFFF 无效域、T-L0-03 的 0..1000 截断、T-L0-05 的 35.0 ℃ 边界）。详见 `evidence/driver_test.md`。
- 回归自检：`host_gxht40_check.c` 27/27、`host_light_check.c` 17/17、`host_sf_i2c_bus_check.c` 15/15（后两者已改为链接 `fw_core.c`）。
- 既有宿主机 L0 测试：`test/build_test.sh` → 56 passed / 0 failed（未改动被测算例；退役与新增用例属 ITEM-010）。

## 交接与依赖

- **T-L0-05 用例数值与规则不一致（需测试侧确认）**：TD-002 第②行写作 `prev=300,cur=290`（下降 10 = 1.0 ℃）期望 **false**，但同一行又要求「与 FD-002 §6.4 逐条一致」；按任务书/FD-002/IC-002 的 `(prev-cur)>9`，下降 10 应触发。实现按 `>9`（下降 9 不触发、下降 10 触发），即「恰好 0.9 ℃」对应 `cur=291`；已在自检中同时固定 9/10/11 三个边界。请测试侧在 T-L0-05 中校正该行数值。
- ITEM-006 采样流程将调用 `sensor_decide_report()`；`prev_temp_x10` 由测量成功后更新，失败不更新（ITEM-006 实现）。
- ITEM-009/010 将移除 `fw_core.c` 中的 OPTCFG/params 纯逻辑与 `host_sensor_core_test.c` 对应用例，并把本项纯逻辑用例并入 `host_sensor_core_test.c`/`build_test.sh`。
- **跨角色交接（embedded_verification）**：本项把 `light_code_is_dark` 移入 `fw_core.c`、并让 `gxht40.c`/`light.c` 依赖 `fw_core.c`。因此 `test/host_light_verify_ev.c` 与 `host_gxht40_verify_ev.c`（嵌入式测试的 ITEM-004/ITEM-003 独立 harness）需在链接目标中补上 `USER/src/fw_core.c`（`host_light_verify_ev.c` 还需包含 `fw_core.h`）；本能力未修改这两个跨角色验证文件，仅登记交接。

---

# 任务项 ITEM-006（已完成，独立验证 TEST_PASS）

**ITEM-006**：改造 `measure.c` 采样流程：每个采样周期先取光照再取温湿度；GXHT40 读 NACK 与 CRC 错按配置上限有界重试；整周期失败时不上报、不更新前一有效温度、不构造 0 值；成功后更新前一有效温度与最近有效湿度。

设计映射：FD-002 §4（数据流）、§6.4（上报判定）、§10（异常策略）、§3.2（纯逻辑在 fw_core）。测试映射：TD-002 T-L5/T-L6（实板/集成）。

## 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `.../USER/src/measure.c` | 重写采样部分 | 删除 AHT21 步进状态机与 `alarm_triggered()`；新增 `measure_sample()`（`light_sample()` → `gxht40_measure()`）与新的 `temperature_process()`；保留软 I2C 端口与 `send_data_to_gateway()`/`go_to_sleep()` 框架 |
| `.../USER/inc/measure.h` | 修改 | 移除已废弃的 `samples_since_report`/`first_sample_reported` 声明 |
| `.../USER/src/fw_core.c` | 微调 | `SENSOR_CONFIG_NO_MCU` 定义加 `#ifndef` 守卫（避免命令行 `-D` 时重定义告警；不改变行为） |
| `.../test/host_measure_flow_check.c`、`test/mock_measure_mcu/` | 新增（测试载体） | 宿主机 mock MCU 影子头 + 可控 GXHT40/光照桩，编译**真实 measure.c**，15 项 |

### 行为要点

1. **顺序**：`measure_sample()` 先 `light_sample()`（PB05 供电→稳定→ADC 均值→PB05 低），再 `gxht40_measure()`。
2. **有界重试**：读 NACK（`GXHT40_READ_RETRY`×`GXHT40_READ_RETRY_DELAY_MS`）与 CRC 错（`GXHT40_MEAS_RETRY`）均在 `gxht40_measure()` 内按 `sensor_config.h` 上限完成；`measure.c` 只调用一次并按返回码分支。
3. **失败（`GXHT40_ERR_*`）**：`measure_sample()` 在写 `tempvalue`/`huminityvalue` **之前** 返回 false；`temperature_process()` 失败分支不置 `report_req`、不推进 `s_prev_temp_x10`/`s_have_prev`/`s_last_hum_x10`，也不写 0 值（`tempvalue`/`huminityvalue` 保持上次有效值）。
4. **成功**：先用**更新前**的 `prev` 调 `sensor_decide_report()` 置 `report_req`，再推进 `s_prev_temp_x10 = tempvalue`、`s_have_prev = true`、`s_last_hum_x10 = huminityvalue`。
5. **待上报不丢**：无上报条件的成功周期不清除已挂起的 `report_req`（发送失败重试语义保留）。
6. **初始化**：首次采样惰性执行 `bsp_i2c_init()` + `gxht40_init(temp_ptr)` + `light_init()`（`s_sensor_ready` 一次性）。
7. **发送路径**：帧布局/加密不变；仅把硬编码的 200 ms/3 次改为引用 `SENSOR_RF_TX_TIMEOUT_MS`/`SENSOR_RF_TX_RETRY`，并移除已废弃的 `samples_since_report` 归零。完整对齐属 ITEM-008。

**本项不包含**：3 分钟节拍（ITEM-007）、发送路径完整对齐（ITEM-008）、旧 `hall/OPTCFG/params/history` 退役（ITEM-009）、宿主机测试合并（ITEM-010）。

## 验证（本轮实际执行）

- 交叉编译：`gcc/build.sh` → **0 错误、28 条告警**（基线 29；旧 AHT21 序列移除后少 1 条，无新增告警）；FLASH **35,884 B**（较 ITEM-005 +1,532 B，新驱动/光照模块被引用入镜像；`history.o` 不再被引用）、RAM **1,896 B**（-64 B）。`arm-none-eabi-nm` 确认 `gxht40_measure`/`light_sample`/`sensor_decide_report` 已入镜像。
- 采样流程自检：`test/host_measure_flow_check.c` → **15 passed / 0 failed**，覆盖顺序、9/10 下降边界、需 DARK、待上报保持、失败不改前值/不置位/不写 0、失败后前值判定、超温分支、`sample_flag==0` 不采样。详见 `evidence/driver_test.md`。
- 回归：`host_fw_core_pure_check.c` 36/36、`host_gxht40_check.c` 27/27、`host_light_check.c` 17/17、`host_sf_i2c_bus_check.c` 15/15；既有 `test/build_test.sh` 56/56。

## 交接与依赖

- **报告判定位置**：本项已在采样成功分支调用 `sensor_decide_report()` 并置 `report_req`；ITEM-008 据此完成/校对发送路径（`encode_frame10` + `app_um2005C_send_data_timeout` + 成功清除/失败重试）。
- **引脚所有权冲突仍未消除**：`main.c` 仍调用 `optcfg_init()`/`hall_init()`（PB05 数字输入/PB04 EXTI/PB06），可能在运行中重新配置光照引脚；`light_init()` 在首次采样时会重新配置，但完整退役属 ITEM-009。在 ITEM-009 完成前，TD-002 的 T-L3/T-L5/T-L6 实板结论不具备有效前提。
- 宿主机 mock（`test/mock_measure_mcu/`）只影子 `cw32l010_gpio/sysctrl/uart`；`gxht40_measure`/`light_sample` 以桩替换，未在本 harness 中执行其真实实现（其行为已由 ITEM-003/004 harness 单独验证）。

---

# 任务项 ITEM-007（已完成，独立验证 TEST_PASS）

**ITEM-007**：将采样节拍改为 3 分钟：RTC 1 分钟中断累计到配置的 3 次才置采样标志，一个采样周期内只执行一次测量，并移除原小时上报与首样本强制上报逻辑。

设计映射：readme 修改点 3；FD-002 §5.1/§5.2（采样周期与 RTC 节拍）；RTA-002 §1.2（T_TICK）。测试映射：TD-002 T-L4-01（板级长时基）。

## 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `.../USER/src/main.c` | 修改 | `RTC_IRQHandlerCallBack()` 阈值 `rtc_set_cnt >= 1` → `>= SENSOR_SAMPLE_TICKS`（=3）；删除未使用的 `temp_cnt`/`work_period_flag`；更新陈旧注释（原“每拍一次采样/小时上报由 samples_since_report 计数”）与 `RTC_SetInterval` 的“1s”注释 |

`measure.c` 的小时/首样本上报逻辑已在 ITEM-006 移除（`first_sample_reported`/`samples_since_report` 均无引用）。本项未再改 `measure.c`。

### 行为要点

1. **3 分钟节拍**：RTC 仍为 `RTC_INTERVAL_EVERY_1M`（1 分钟/拍）；`RTC_IRQHandlerCallBack()` 每拍 `rtc_set_cnt++`，达到 `SENSOR_SAMPLE_TICKS`（`sensor_config.h` = 3）时置 `sample_flag = 1` 并清零计数。
2. **一个周期一次测量**：`temperature_process()` 在 `sample_flag != 0` 时执行一次 `measure_sample()`，随后 `sample_flag = 0`；下一次置位需再累计 3 拍。
3. **无强制上报**：上报完全由 `sensor_decide_report` 条件门控；无小时上报、无首样本强制上报。
4. **上电即采一次**：`sample_flag` 初值为 1，上电后立即执行一个采样周期；因无前值，首样本不会触发下降分支（与 FWR-108 一致），超温分支仍可上报。

**本项不包含**：发送路径对齐（ITEM-008）、旧 `hall/OPTCFG/params/history` 退役（ITEM-009）、板级 3 分钟长时基测量（TD-002 T-L4-01，需嵌入式测试）。

## 验证（本轮实际执行）

- 交叉编译：`gcc/build.sh` → **0 错误、28 条告警**（与 ITEM-006 同数；`main.c` 仅有改动前既有的 4 条：未用参数 `handle`/`file`/`line`、`main` 返回类型）；FLASH 35,884 B、RAM 1,896 B（不变，节拍常量不产生代码）。
- 源码结构确定性检查（命令与输出见 `evidence/driver_test.md`）：`main.c` 阈值 = `SENSOR_SAMPLE_TICKS`；全工程无 `rtc_set_cnt >= 1`/`first_sample_reported`/`samples_since_report` 残留；`measure.c` 消费标志（`sample_flag = 0`）；配置值 1 min × 3 = 3 min。
- 回归：`host_fw_core_pure_check.c` 36/36、`host_measure_flow_check.c` 15/15、`host_gxht40_check.c` 27/27、`host_light_check.c` 17/17、`host_sf_i2c_bus_check.c` 15/15、`test/build_test.sh` 56/56。

## 交接与依赖

- **板级节拍**：3 分钟实际间隔（含 LSI 容差与离散度）需 TD-002 T-L4-01 用长时基/电流波形在真实板记录；本能力无硬件，未做实测。
- **引脚所有权冲突仍未消除**：`main.c` 仍调用 `optcfg_init()`/`hall_init()`（PB05/PB04/PB06），完整退役属 ITEM-009。
- 观测钩子 `SENSOR_TEST_TRACE` 仍未接入打印点（TD-002 §2 交接项）；可随后续采样/上报任务或测试需求接入。

---

# 任务项 ITEM-008（已完成，独立验证 TEST_PASS）

**ITEM-008**：实现条件上报：满足 `sensor_decide_report` 时调用 `encode_frame10(uid, temp_x10, hum_x10)` 组 10 字节帧并经 `app_um2005C_send_data_timeout` 有界发送，帧布局、字节序与 Feistel 加密保持不变，仅在发送成功后清除待上报状态，发送失败按上限重试后放弃本轮。

设计映射：readme 修改点 4；IC-002 §2/§3；FD-002 §6.4/§8.1/§10。测试映射：TD-002 T-L0i-01（空口往返）、T-L5/T-L6-03（板级上报/发送失败）。

## 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `.../USER/src/measure.c` | 修改 `send_data_to_gateway()` | 删除被 `encode_frame10()` 覆盖的冗余 `send_data[0..7]` 手写组帧；改用**最近一次有效样本** `s_prev_temp_x10`/`s_last_hum_x10`（同时消除 ITEM-006 遗留的 `s_last_hum_x10` 只写不读）；长度改用 `SENSOR_RF_FRAME_LEN` |
| `.../USER/src/main.c` | 注释修正 | 主循环调用处陈旧注释（“1 min 采样”“立即/小时/首样本上报”）改为“3 分钟一拍”“条件上报: sensor_decide_report 门控” |
| `.../test/host_rf_frame_check.c` | 新增（测试载体） | 传感器 `encrytogate.c` ↔ 网关 `feistel_al.c` 往返互操作 14 项 |
| `.../test/host_measure_flow_check.c` | 扩展 | 新增发送路径 9 项（门控/样本选择/长度/成功清除/失败重试/放弃） |

### 行为要点

1. **门控**：`report_req == 0` 直接返回（不上报）；`report_req` 由 `temperature_process()` 按 `sensor_decide_report()` 置位。
2. **组帧**：完全由 `encode_frame10()` 完成：`uid_pick(4) | temp_x10_LE(2) | hum_x10_LE(2) | crc16_LE(2)` 后 8 轮 Feistel；本次未修改 `encrytogate.c`，帧布局/字节序/加密不变。
3. **发送**：`app_um2005C_send_data_timeout(send_data, SENSOR_RF_FRAME_LEN, SENSOR_RF_TX_TIMEOUT_MS)`（有界）。
4. **成功**：`report_req = 0` 并复位重试计数；**失败**：`report_retry++`，达到 `SENSOR_RF_TX_RETRY`（3）后清除 `report_req` 放弃本轮（下个 3 分钟周期自然重试）。
5. **样本一致性**：发送最近一次有效样本（与触发判定的样本一致）；失败周期不会用 0/脏值覆盖它。

**本项不包含**：旧 `hall/OPTCFG/params/history` 退役（ITEM-009，包括 `send_data_to_gateway()` 中遗留的 `optcfg_window_active()` 延后门与 `go_to_sleep()` 的旧门控）、宿主机测试合并（ITEM-010）、工程文件注册（ITEM-011）。

## 验证（本轮实际执行）

- 交叉编译：`gcc/build.sh` → **0 错误、28 条告警**（与 ITEM-007 同数；`measure.c` 0 告警，`main.c` 仅既有 4 条）；FLASH **35,856 B**（较 ITEM-007 −28 B，删除冗余组帧代码）、RAM 1,896 B（不变）。
- 空口往返（协议级）：`test/host_rf_frame_check.c`（传感器 encode ↔ 网关 decode）→ **14 passed / 0 failed**：T-L0i-01 向量、128 组随机、负温/极值、字段位置与小端序、密文篡改必拒、编码确定性。详见 `evidence/protocol_test.md`。
- 发送路径（流程级）：`test/host_measure_flow_check.c` → **24 passed / 0 failed**（含 9 项发送路径）。详见 `evidence/driver_test.md`。
- 回归：`host_fw_core_pure_check.c` 36/36、`host_gxht40_check.c` 27/27、`host_light_check.c` 17/17、`host_sf_i2c_bus_check.c` 15/15、`test/build_test.sh` 56/56。

## 交接与依赖

- **板级上报链路**：真实 433 发射/网关接收、发送失败重试上限（TD-002 T-L6-03）与端到端时间戳需嵌入式测试在真实硬件完成；且 ITEM-009 退役 `hall/OPTCFG` 前，PB04/PB05 引脚所有权冲突使实板采样/上报验证不具备有效前提。
- **`optcfg_window_active()` 遗留门**：`send_data_to_gateway()` 仍会在配置窗口内延后上报；因硬件已无 OPTCFG，该门恒为 false，不影响行为；完整移除属 ITEM-009。
- `SENSOR_TEST_TRACE` 观测钩子仍未接入（TD-002 §2 交接项）。

---

# 任务项 ITEM-009（已完成，独立验证 TEST_PASS）

**ITEM-009**：退役与新硬件冲突的旧通路：从工程与构建中移除 hall/OPTCFG/params/history 模块及其对 PB04/PB05/PB06 的初始化，移除 GPIOB 霍尔 EXTI 分支与 LPTIM 中的 OPTCFG 分支；PB04 仅作 AIN11 模拟输入、PB05 仅作光照供电输出、PB06 不外驱动。

设计映射：FWR-111；FD-002 §1.2/§3.3/§11.4；网表（PB04=LIGHT_ADC、PB05=LIGHTPOWER、PB06=NC、无霍尔器件）。测试映射：TD-002 T-L0-06（退役引用为 0）、T-L1-07（工程源文件列表）。

## 实际改动

**删除文件（8 个）**：`USER/src/hall.c`、`USER/inc/hall.h`、`USER/src/optcfg.c`、`USER/inc/optcfg.h`、`USER/src/params.c`、`USER/inc/params.h`、`USER/src/history.c`、`USER/inc/history.h`。

| 文件 | 动作 | 内容 |
|---|---|---|
| `.../USER/inc/main.h` | 修改 | 移除 `params.h`/`history.h`/`optcfg.h`/`hall.h` 包含 |
| `.../USER/src/main.c` | 修改 | 移除 `params_init/history_init/optcfg_init/hall_init` 与主循环内 Hall 去抖/开窗、`optcfg_process()` 分支 |
| `.../USER/src/interrupts_cw32l010.c` | 修改 | 移除 `hall.h`/`optcfg.h` 包含；`GPIOB_IRQHandler` 删除霍尔 EXTI 分支（PB04 已为模拟输入）；`LPTIM_IRQHandler` 删除 OPTCFG 采样分支，仅保留 433 位时钟 |
| `.../USER/src/measure.c` | 修改 | 移除 `optcfg.h`/`hall.h` 包含；`send_data_to_gateway()` 删除配置窗口延后门；`go_to_sleep()` 仅按 `report_req`/`sample_flag` 门控 |
| `.../USER/inc/fw_core.h`、`.../USER/src/fw_core.c` | 修改 | 移除参数(NVM 阈值)纯函数与 OPTCFG/1 解码器/帧解析；**保留** CRC16-CCITT-FALSE（通用纯工具，仍由测试覆盖）与本次新增的 CRC-8/换算/滞回/上报纯逻辑；不再包含 `params.h`/`string.h`/`math.h`（改 `<stddef.h>`） |
| `.../test/host_sensor_core_test.c`、`test/build_test.sh` | 修改 | 移除已退役的 params/OPTCFG/history 用例与 `history.c` 链接，仅保留 CRC16 回归（新用例属 ITEM-010） |
| `.../test/host_measure_flow_check.c` | 修改 | 移除不再被引用的 `optcfg_window_active`/`hall_event_pending` 桩 |

### 行为要点

1. **引脚所有权唯一**：PB04 仅由 `light.c` 配为模拟输入（AIN11）；PB05 仅由 `light.c` 配为推挽输出；PB06 无任何代码引用（DEBUG UART 用的是 PA05/PA06）。
2. **无遗留引用**：全工程源码中已无 hall/OPTCFG/params/history 的代码引用（仅保留说明性注释）；固件镜像中无相关符号。
3. **构建自动收敛**：`gcc/build.sh` 以 `USER/src/*.c` 通配编译，删除后自动不再编译；MDK/IAR 工程源文件列表本就未登记这些模块（也尚未登记 gxht40.c/light.c/fw_core.c），列表一致性属 ITEM-011。
4. **功能不变**：采样→判定→上报→深睡主流程不受影响（已由既有 harness 回归）。

**本项不包含**：把 gxht40.c/light.c/fw_core.c 加入 MDK/IAR 源列表（ITEM-011）、新增纯逻辑测试用例（ITEM-010）、网关固件核对（ITEM-012）。

## 验证（本轮实际执行）

- 交叉编译：`gcc/build.sh` → **0 错误、28 条告警**（与 ITEM-008 同数，均为既有模板/库告警）；FLASH 35,856 → **32,208 B**（−3,648 B）、RAM 1,896 → **1,712 B**（−184 B）。`arm-none-eabi-nm` 确认镜像中**无** hall/optcfg/params/history 符号，`gxht40_measure`/`light_sample`/`sensor_decide_report`/`fw_crc8_gxht` 均在。
- 退役引用检查：`grep -rn "hall|optcfg|params_|history|OPTCFG_|PARAMS_" USER/` → 仅 3 处说明性注释；MDK/IAR 工程文件本就无这些模块条目。
- 引脚所有权：PB04/PB05 仅出现在 `light.c`/`sensor_config.h`（owner）；PB06 无引用。
- 回归：`test/build_test.sh`（CRC16）2/2、`host_fw_core_pure_check.c` 36/36、`host_measure_flow_check.c` 24/24、`host_gxht40_check.c` 27/27、`host_light_check.c` 17/17、`host_sf_i2c_bus_check.c` 15/15、`host_rf_frame_check.c` 14/14。

## 交接与依赖

- **跨角色交接（embedded_verification）**：`test/host_measure_flow_verify_ev.c`、`host_rf_report_verify_ev.c`、`host_rtc_cadence_verify_ev.c` 仍引用已退役的 `optcfg.h`/`hall.h`/`params.h`/`history.h` 或其符号；本能力未修改这些跨角色验证文件，需测试侧适配（移除相应桩/包含）。
- **MDK/IAR 源列表（ITEM-011）**：删除的 8 个文件未在这些工程中登记（无需移除）；但 `gxht40.c`/`light.c`/`fw_core.c` 也未登记，ITEM-011 必须补上，否则 MDK/IAR 链接会缺少驱动/纯逻辑。
- **新纯逻辑用例（ITEM-010）**：本项已将 `host_sensor_core_test.c` 收敛为 CRC16 回归，ITEM-010 需补充 CRC-8/换算/光照滞回/上报判定用例。
- 板级引脚/功耗验证（TD-002 T-L3/T-L4）仍需真实硬件；本项完成后 PB04/PB05 引脚冲突已消除，实板验证前提具备。

---

# 任务项 ITEM-010（已完成，独立验证 TEST_PASS）

**ITEM-010**：更新宿主机纯逻辑测试 `test/host_sensor_core_test.c` 与 `test/build_test.sh`：覆盖 CRC-8 参考向量、换算边界（负温、0%/100% 截断）、光照阈值与滞回、上报判定边界（恰好 0.9℃、恰好 35.0℃、无前值、非暗、失败不更新），移除已退役的 OPTCFG/params 用例，脚本在本机运行通过。

设计映射：TD-002 T-L0-01…T-L0-06、T-L6-01；FD-002 §6。

## 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `.../test/host_sensor_core_test.c` | 重写为综合纯逻辑套件 | 保留 CRC16 回归，新增 T-L0-01 CRC-8、T-L0-02 温度换算/合法域、T-L0-03 湿度换算/截断、组合换算、T-L0-04 光照滞回、T-L0-05 上报判定共 38 项 |
| `.../test/build_test.sh` | 扩展为两阶段 | 阶段 1：fw_core 纯逻辑（`host_sensor_core_test.c`）；阶段 2：采样/上报流程（`host_measure_flow_check.c` + mock MCU），覆盖“测量失败不更新前值” |

已退役的 OPTCFG/params/history 用例已在 ITEM-009 移除；本项不再涉及旧模块。

### 覆盖对应（TD-002）

| 用例 | 断言要点 | 结果 |
|---|---|---|
| T-L0-06 | CRC16 `123456789`→0x29B1、参考帧 33B→0xB540 | PASS ×2 |
| T-L0-01 | CRC-8 `{BE,EF}`→0x92、`{00}`→0xAC、空长度→0xFF | PASS ×3 |
| T-L0-02 | 温度 S=1872→−400 / 16855→0 / 26214→250 / 63664→1250；S=0→−450、0xFFFF→1300；有效域 -400/1250 合法、−450/1251/1300 无效；组合换算有效写输出/无效不写 | PASS ×11 |
| T-L0-03 | 湿度 S=0→0 / 2247→0 / 29360→500 / 55575→1000 / 0xFFFF→1000 | PASS ×5 |
| T-L0-04 | 已明 349→LIT、350→DARK；已暗 251→DARK、250→LIT；0→LIT、4095→DARK；带内保持 | PASS ×7 |
| T-L0-05 | 无前值 200→false / 351→true；降 9→false、10→true、11→true；非 DARK→false；cur=350→false；prev==cur→false | PASS ×9 |
| T-L6-01（流程） | 失败周期：不置 report_req、不更新前值、不写 0；恢复后按失败前前值判定 | PASS（阶段 2） |

### 行为要点

1. `build_test.sh` 成为单一宿主入口：**阶段 1** 纯逻辑（38/38）、**阶段 2** 流程（24/24），退出码 0。
2. “失败不更新前值”属 `measure.c` 流程行为，放在阶段 2（`host_measure_flow_check.c`，mock MCU 影子头）验证；文件头注释已交叉引用。
3. 保留的 `host_fw_core_pure_check.c`（ITEM-005 引入）与本套件用例重叠，作为独立 harness 保留。

**本项不包含**：MDK/IAR 源列表一致性（ITEM-011）、网关固件核对（ITEM-012）。

## 验证（本轮实际执行）

- `test/build_test.sh`（本机 MinGW-w64 gcc 12.2.0）→ 阶段 1 **38 passed / 0 failed**、阶段 2 **24 passed / 0 failed**，**退出码 0**。
- 交叉编译：`gcc/build.sh` → **0 错误、28 条告警**；FLASH 32,208 B / RAM 1,712 B（不变，测试不进入固件）。
- 回归：`host_fw_core_pure_check.c` 36/36、`host_measure_flow_check.c` 24/24、`host_gxht40_check.c` 27/27、`host_light_check.c` 17/17、`host_sf_i2c_bus_check.c` 15/15、`host_rf_frame_check.c` 14/14。

## 交接与依赖

- **T-L0-05 用例数值（TD-002 需校正）**：任务文本/FD-002/IC-002 均为 `(prev-cur)>9`（降 9 不触发、降 10 触发）；TD-002 第②行写作 `prev=300,cur=290` 期望 false 与规则不符（“恰好 0.9℃”应为 cur=291）。实现与测试均按 `>9`，并同时固定 9/10/11 三个边界。
- MDK/IAR 源列表一致性（含 fw_core.c/gxht40.c/light.c）属 ITEM-011。
- 板级长时基/功耗/射频验证仍属嵌入式测试（TD-002 T-L4/T-L6/T-L8）。

---

# 任务项 ITEM-011（本轮）

**ITEM-011**：交叉编译验证传感器工程：执行 `gcc/build.sh` 完成编译与链接并生成 elf/hex/bin，RAM 与 Flash 占用不超过 4KB/64KB，且 MDK/IAR 工程源文件列表与新增、移除的源文件保持一致。

设计映射：FD-002 §3/§11.12；TD-002 T-L1-02（交叉编译与产物）、T-L1-07（工程文件一致性）。

## 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `.../MDK/Project.uvprojx` | 修改 | User 组补 `fw_core.c`/`gxht40.c`/`light.c`；Driver 组补 `cw32l010_adc.c`（light.c 依赖 ADC） |
| `.../EWARM/project.ewp` | 修改 | User 组补齐 8 个 `USER/src/*.c`；Driver 组补 `cw32l010_adc/digitalsign/lptim/rtc/uart.c`；新增 `UM2005C` 与 `COMMON` 组；Release/Debug 两套 include 路径补 `..\UM2005C`/`..\COMMON` |
| `.gitignore` | 修改 | 新增 `*.exe`（宿主机测试产物，避免污染 `git status`；ITEM-010 验证提出的卫生项） |

### 一致性结果

- MDK/IAR 的 `USER/src` 列表与磁盘实际 `USER/src/*.c`（8 个）**逐一相等**；已删的 `hall/optcfg/params/history` 在两个工程中均不存在。
- 两个工程均含 `cw32l010_adc.c`、`UM2005C`（含 `app_um2005C.c`）、`COMMON`（`delay.c`）。
- 两个工程文件均为合法 XML（`ElementTree` 解析通过）。

### 本项不包含

网关固件（CH592）构建与核对（ITEM-012）、板级实机验证（嵌入式测试）。

## 验证（本轮实际执行）

- **交叉编译**：`gcc/build.sh` → **0 错误、28 条告警**；生成 `gcc/obj/sensor_fw.elf`、`.hex`、`.bin`。
  - `arm-none-eabi-size`：text 32,208 / data 84 / bss 1,628。
  - **FLASH 32,208 B / 65,536 B（49.15% ≤ 64 KB）**；**RAM 1,712 B / 4,096 B（41.80% ≤ 4 KB）**。
  - `.bin` MD5 `47abc6dfc798c837b2b68916f29db543`；`.hex` MD5 `ddf2e2e7bac47a80ea00177cb104afc9`。
- **生产编译器 AC5**（`armcc --cpu=Cortex-M0+ --c99`，CMSIS 5.9.0）：`USER/src` 8 个源文件均 **0 error / 0 warning**（`encrytogate.c` 3 条既有未用静态量告警）。
- **工程列表一致性**：脚本化比对 MDK/IAR 与 `ls USER/src/*.c` → 完全相等（见 `evidence/driver_test.md`）。
- **宿主机测试**：`test/build_test.sh` → 38/38 + 24/24，退出码 0。

## 交接与依赖

- **MDK/IAR 实机构建未执行**：本环境 Keil 受 CMSIS 6.3.0 与 AC5 不兼容影响（ITEM-001 已登记），IAR 工具链不可用；已用 GNU 交叉编译 + AC5 逐文件编译 + XML 合法性 + 列表一致性代替。建议在具备正确 CMSIS 5.9.0 包的 Keil/IAR 环境复编译。
- 网关固件（CH592 beiwov2）构建与核对属 ITEM-012。
- 板级实机验证（TD-002 T-L2/T-L3/T-L4/T-L6/T-L8）仍属嵌入式测试。

---

# 后续任务项状态

`artifacts/firmware_tasks.yaml` 共 12 项；已完成 ITEM-001…ITEM-011（前十项独立验证 TEST_PASS，ITEM-011 本轮）。其余 1 项（ITEM-012，网关固件兼容性核查）由 Runtime 后续指派。

# 交接与依赖（累计）

- 光照阈值数值（`LIGHT_DARK_ENTER`/`EXIT`）当前为**未标定工程默认值**（`LIGHT_DARK_CALIBRATED=0`）；按 test_design T-L3-03 实板标定后回填配置头（FWR-OPEN-1）。
- 调试钩子 `SENSOR_TEST_TRACE` 已在配置头预留，实际打印点在后续采样/上报任务项中接入（test_design §2 交接项）。
- `sf_i2c.c` 既有函数 `i2c_write_multi_byte`/`i2c_write_multi_byte_16bit` 存在"`length==0` 时 `err` 未初始化"的既有缺陷；本项按「既有函数语义不变」要求未修改，登记为后续可选清理项。
