# 构建证据（BUILD-002）

状态：固件实现证据（firmware_engineer.firmware_implementation）
本轮对象：任务项 ITEM-001（新增 `sensor_config.h` 唯一配置点）、ITEM-002（`sf_i2c` 无寄存器地址总线原语）、ITEM-003（GXHT40 驱动 `gxht40.c/.h`）

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

## 5. ITEM-002 增量（sf_i2c 原语）

改动：`USER/inc/sf_i2c.h`（+8 行声明）、`USER/src/sf_i2c.c`（+62 行实现，**0 行删除**）；新增测试载体 `test/host_sf_i2c_bus_check.c`（不进入固件构建）。

```
== compile ==  全部翻译单元
== link ==
Memory region   Used Size   Region Size   %age Used
FLASH:          34,060 B    64 KB         51.97%
RAM:             1,960 B    4 KB          47.85%
== done: obj/sensor_fw.elf/.hex/.bin ==
arm-none-eabi-size: text 34060 / data 84 / bss 1876 / dec 36020
```

- **0 错误**。相对 ITEM-001（FLASH 33,896 B / RAM 1,960 B）：FLASH **+164 B**（两个原语），RAM 不变。
- 告警：无新增。`sf_i2c.c` 仍只有改动前既有的 2 条 `err may be used uninitialized`（位于未触碰的 `i2c_write_multi_byte` / `i2c_write_multi_byte_16bit`）。
- 链接符号（`arm-none-eabi-nm obj/sensor_fw.elf`）：`i2c_write_cmd 0x46b2 T`、`i2c_read_bytes 0x46e4 T` 已进入镜像；既有 `i2c_write_byte`/`i2c_read_byte`/`i2c_read_multi_byte`/`i2c_write_multi_byte`/`i2c_*_16bit`/`i2c_start`/`i2c_stop`/`i2c_init`/`i2c_obj_find` 全部保留。
- 变更范围：`git diff --numstat` = `62 insertions / 0 deletions`（sf_i2c.c），既有函数未被改动。
- 驱动级自检：`test/host_sf_i2c_bus_check.c` → **15 passed / 0 failed**（详见 `evidence/driver_test.md`）。
- 宿主机 L0 回归：`test/build_test.sh` → 56 passed / 0 failed。

## 6. ITEM-003 增量（GXHT40 驱动）

改动：新增 `USER/inc/gxht40.h`、`USER/src/gxht40.c`；新增测试载体 `test/host_gxht40_check.c`（不进入固件构建）。无既有文件修改。

```
== compile ==  全部翻译单元（含 gxht40.c）
== link ==
Memory region   Used Size   Region Size   %age Used
FLASH:          34,060 B    64 KB         51.97%
RAM:             1,960 B    4 KB          47.85%
== done: obj/sensor_fw.elf/.hex/.bin ==

arm-none-eabi-size obj/gxht40.o : text 460 / data 0 / bss 5
arm-none-eabi-nm   obj/gxht40.o :
  T gxht40_init / gxht40_measure / gxht40_detected_addr7
  t gxht40_crc8 / gxht40_start_and_read
  b s_dev / s_addr7
  U delay_ms / i2c_write_cmd / i2c_read_bytes
```

- **0 错误，无任何告警指向 `gxht40.c`**。
- 工程 FLASH/RAM **与 ITEM-002 相同（34,060 / 1,960 B）**：`gxht40.o` 已编译但尚未被引用，链接器未拉入（接线由 ITEM-006 完成）；因此本项不改变镜像内容，仅新增可编译、可单测的驱动模块。
- 依赖面：`gxht40.c` 仅依赖 `sensor_config.h`（配置）、`sf_i2c`（ITEM-002 原语）与 `delay_ms`；无直接寄存器访问，无浮点。
- 驱动级自检：`test/host_gxht40_check.c` → **27 passed / 0 failed**（详见 `evidence/driver_test.md`）。
- 宿主机 L0 回归：`test/build_test.sh` → 56 passed / 0 failed。
- 工程文件注册（MDK `Project.uvprojx` / IAR `project.ewp` 的源文件列表加入 `gxht40.c`）属 ITEM-011 范围，本项未改工程文件；`gcc/build.sh` 通过通配自动包含。

## 7. 限制与交接

- 本证据为 GNU 交叉编译 + 宿主机回归；量产构建走 Keil MDK / IAR EWARM（既有工程，`USER/inc` 已在两者 include 路径中，故新头文件无需改工程文件列表），未在本环境复编译 MDK/IAR。
- 网关（CH592 beiwov2）需 WCH 工具链，本轮未涉及。
- PATH 上默认 `gcc` shim 损坏导致 `test/build_test.sh` 直接调用失败；需用完整路径或修正 PATH。这是环境问题，非代码问题，已在上面记录调用方式。
