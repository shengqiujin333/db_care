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

# 任务项 ITEM-002（本轮）

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

# 后续任务项状态

`artifacts/firmware_tasks.yaml` 共 12 项；已完成 ITEM-001（独立验证 TEST_PASS）、ITEM-002（本轮）。其余 10 项由 Runtime 后续指派，未指派项不在本轮产出。

# 交接与依赖（累计）

- 光照阈值数值（`LIGHT_DARK_ENTER`/`EXIT`）当前为**未标定工程默认值**（`LIGHT_DARK_CALIBRATED=0`）；按 test_design T-L3-03 实板标定后回填配置头（FWR-OPEN-1）。
- 调试钩子 `SENSOR_TEST_TRACE` 已在配置头预留，实际打印点在后续采样/上报任务项中接入（test_design §2 交接项）。
- `sf_i2c.c` 既有函数 `i2c_write_multi_byte`/`i2c_write_multi_byte_16bit` 存在"`length==0` 时 `err` 未初始化"的既有缺陷；本项按「既有函数语义不变」要求未修改，登记为后续可选清理项。
