# 构建证据（BUILD-002）

状态：固件实现证据（firmware_engineer.firmware_implementation）
本轮对象：任务项 ITEM-001（新增 `sensor_config.h` 唯一配置点）

## 1. 环境

- 交叉编译器：`arm-none-eabi-gcc 10.3.1 20210824`（GNU Arm Embedded Toolchain，Cortex-M0+）
- CMSIS-Core：ARM CMSIS 5.9.0（`core_cm0plus.h`，本机 Arm Packs）
- 宿主机编译器：MinGW-w64 GCC 12.2.0（`C:/ProgramData/chocolatey/lib/mingw/tools/install/mingw64/bin/gcc.exe`；PATH 上的 chocolatey shim `bin/gcc.exe` 无法解析 libexec/cc1，需用完整路径）
- 构建脚本：`CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/gcc/build.sh`
- 产物：`gcc/obj/*.o`、`gcc/obj/sensor_fw.elf/.hex/.bin`（被 `.gitignore` 忽略，不入库）

## 2. 交叉编译结果（本轮）

```
== compile ==   USER/src + COMMON + UM2005C + Libraries/src 全部翻译单元
== link ==
Memory region   Used Size   Region Size   %age Used
FLASH:          33,896 B    64 KB         51.72%
RAM:             1,960 B    4 KB          47.85%
== done: obj/sensor_fw.elf/.hex/.bin ==
arm-none-eabi-size: text 33896 / data 84 / bss 1876 / dec 35856
```

- **0 错误**。告警全部来自既有代码与厂商库（`encrytogate.c` 未用函数、`main.c` 返回类型/未用参数、`sf_i2c.c` 可能未初始化、`Libraries/src/*` 库宏等），**无任何告警指向 `sensor_config.h`**。
- 与改动前基线对比：FLASH 33,896 B、RAM 1,960 B **完全一致**（配置头只含宏，不产生代码），证明「加入该头文件后工程仍可交叉编译」。

## 3. 配置头编译期断言检查

命令（不进入固件构建，使用临时宿主机侧 .c）：

```
arm-none-eabi-gcc -mcpu=cortex-m0plus -mthumb -Wall -Wextra \
  -I USER/inc -I COMMON -I UM2005C -I ../../../Libraries/inc \
  -I <CMSIS Core Include> -fsyntax-only /tmp/cfg_check.c
```

断言与结果：

| 断言 | 期望 | 结果 |
|---|---|---|
| `GXHT40_ADDR_WRITE_A/READ_A/WRITE_B/READ_B` | 0x88 / 0x89 / 0x8A / 0x8B | PASS |
| `GXHT40_ADDR_7BIT_A/B` | 0x44 / 0x45 | PASS |
| `GXHT40_CMD_MEASURE_HIGH_REP` / `GXHT40_RESULT_LEN` | 0xFD / 6 | PASS |
| `GXHT40_MEASURE_WAIT_MS` | ≥ 9（覆盖 tMEAS.H max 8.3 ms） | PASS（=10） |
| `LIGHT_ADC_INPUT_CHANNEL` | 11（PB04=AIN11） | PASS |
| `LIGHT_POWER_PIN` / `LIGHT_ADC_PIN` | `GPIO_PIN_5` / `GPIO_PIN_4` | PASS |
| `LIGHT_ADC_FULL_SCALE` | 4095 | PASS |
| `LIGHT_DARK_EXIT < LIGHT_DARK_ENTER` 且差 = `LIGHT_DARK_HYSTERESIS_STEP` | 成立 | PASS |
| `SENSOR_RTC_TICK_PERIOD_MIN × SENSOR_SAMPLE_TICKS` | 3 | PASS |
| `SENSOR_REPORT_DROP_X10` / `SENSOR_REPORT_HIGH_X10` | 9 / 350 | PASS |
| `SENSOR_RF_FRAME_LEN` | 10 | PASS |
| `SENSOR_TEST_TRACE` 默认 | 0 | PASS |

结论：**CONFIG HEADER CHECK OK（全部断言通过）**。

## 4. 宿主机既有 L0 测试（回归）

```
CC=<minGW full path> sh test/build_test.sh
==== result: 56 passed, 0 failed ====
```

本项未改动 `fw_core.c`/`history.c`/`host_sensor_core_test.c`，56/56 与改动前一致，属回归确认（新的纯逻辑用例属后续任务项）。

## 5. 限制与交接

- 本证据为 GNU 交叉编译 + 宿主机回归；量产构建走 Keil MDK / IAR EWARM（既有工程，`USER/inc` 已在两者 include 路径中，故新头文件无需改工程文件列表），未在本环境复编译 MDK/IAR。
- 网关（CH592 beiwov2）需 WCH 工具链，本轮未涉及。
- PATH 上默认 `gcc` shim 损坏导致 `test/build_test.sh` 直接调用失败；需用完整路径或修正 PATH。这是环境问题，非代码问题，已在上面记录调用方式。
