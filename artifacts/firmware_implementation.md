# 固件实现（FWI-002）

状态：固件实现（firmware_engineer.firmware_implementation），按任务队列逐项推进
依据：FD-002 `artifacts/firmware_design.md`、FWR-002 `artifacts/firmware_requirements.md`、IC-002、`artifacts/test_design.md`、`artifacts/firmware_tasks.yaml`
范围：仅本次 Runtime 指派的当前任务项；未指派的后续项不在本轮实现。

---

## 当前任务项

**ITEM-001**：新增 `sensor_config.h` 作为唯一配置点：定义 PB04/PB05 引脚与 ADC 通道、GXHT40 地址/命令/重试/等待参数、光照明暗阈值与滞回、3 分钟采样计数值、0.9℃ 下降与 35.0℃ 超温上报常量；加入该头文件后工程仍可交叉编译。

设计映射：FD-002 §3.1（新增文件）、§11.1/§11.4（单一配置点、引脚所有权）。

## 实际改动

| 文件 | 动作 | 内容 |
|---|---|---|
| `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/inc/sensor_config.h` | 新增 | 传感器固件唯一配置点（见下表） |
| `.../USER/inc/main.h` | 修改 | 增加 `#include "sensor_config.h"`，使工程翻译单元实际解析该头文件（证明「加入后仍可交叉编译」非空验证） |

### `sensor_config.h` 定义的配置（只定义，不在本项接入业务逻辑）

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

### 预期行为（本项可检验）

1. 上述常量只在 `sensor_config.h` 定义一次；其它文件只引用宏（FD-002 §11.1，「单一配置点」）。
2. 头文件自包含：只依赖 `<stdint.h>`/`<stdbool.h>` 与 CW32L010 标准外设库头，可被后续 `gxht40.c`/`light.c`/`measure.c` 直接包含。
3. 加入头文件后，传感器工程交叉编译与链接**结果与改动前完全一致**（宏不产生代码）。
4. `SENSOR_TEST_TRACE` 默认关闭，量产构建行为不变。

**本项不包含**：GXHT40 驱动、光照采样、判定与节拍接入、旧模块退役、单元测试扩展（属后续任务项，尚未实现）。

## 验证（本轮实际执行）

- 交叉编译：`cw32l010.../gpio_input_output/gcc/build.sh`，`arm-none-eabi-gcc 10.3`，Cortex-M0+，`-O1 -Wall -Wextra` → **0 错误**；`FLASH 33,896 B (51.72%)`、`RAM 1,960 B (47.85%)`，与改动前基线（33,896 B / 1,960 B）逐字节一致，无新增告警指向 `sensor_config.h`。
- 配置头确定性检查（编译期断言，`-fsyntax-only`）：地址 0x88/0x89/0x8A/0x8B、`ADC_InputCH11`=11、`GPIO_PIN_5`/`GPIO_PIN_4`、节拍 1×3=3、9/350、帧长 10、`EXIT<ENTER` 且差 100、`SENSOR_TEST_TRACE==0` → 全部通过。
- 宿主机 L0 既有测试：`test/build_test.sh` → 56 passed / 0 failed（本项未改动被测算例，属回归确认）。

细节见 `evidence/build.md`。

## 后续任务项状态

`artifacts/firmware_tasks.yaml` 共 12 项；本轮只完成第 1 项，其余 11 项由 Runtime 后续指派。未指派项不在本轮产出。

## 交接与依赖

- 光照阈值数值（`LIGHT_DARK_ENTER`/`EXIT`）当前为**未标定工程默认值**（`LIGHT_DARK_CALIBRATED=0`）；按 test_design T-L3-03 实板标定后回填本头文件（FWR-OPEN-1）。
- 调试钩子 `SENSOR_TEST_TRACE` 已在配置头预留，实际打印点在后续采样/上报任务项中接入（test_design §2 交接项）。
