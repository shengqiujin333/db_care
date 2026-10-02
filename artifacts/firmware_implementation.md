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

# 任务项 ITEM-004（本轮）

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

# 后续任务项状态

`artifacts/firmware_tasks.yaml` 共 12 项；已完成 ITEM-001、ITEM-002、ITEM-003（均独立验证 TEST_PASS）与 ITEM-004（本轮）。其余 8 项由 Runtime 后续指派，未指派项不在本轮产出。

# 交接与依赖（累计）

- 光照阈值数值（`LIGHT_DARK_ENTER`/`EXIT`）当前为**未标定工程默认值**（`LIGHT_DARK_CALIBRATED=0`）；按 test_design T-L3-03 实板标定后回填配置头（FWR-OPEN-1）。
- 调试钩子 `SENSOR_TEST_TRACE` 已在配置头预留，实际打印点在后续采样/上报任务项中接入（test_design §2 交接项）。
- `sf_i2c.c` 既有函数 `i2c_write_multi_byte`/`i2c_write_multi_byte_16bit` 存在"`length==0` 时 `err` 未初始化"的既有缺陷；本项按「既有函数语义不变」要求未修改，登记为后续可选清理项。
