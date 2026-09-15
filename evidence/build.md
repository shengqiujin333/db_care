# 构建证据（BUILD-001）

状态：固件实现证据（firmware_engineer.firmware_implementation）
对象：传感器固件（CW32L010Y8M6）GNU 交叉编译验证 + 宿主机 L0 测试编译。

## 1. 环境

- 编译器：arm-none-eabi-gcc 10.3-2021.10（GNU Arm Embedded Toolchain，Cortex-M0+）
- CMSIS-Core：ARM CMSIS 5.9.0（core_cm0plus.h，来自本机 Arm Packs）
- 构建脚本：`CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/gcc/build.sh`
- 启动/链接：`gcc/startup_cw32l010.S`（GNU as，由 Keil startup 转换）、`gcc/cw32l010.ld`（FLASH 64 KB @0x0，RAM 4 KB @0x20000000；顶层页 0xFF80 保留给参数 NVM）
- 产物：`gcc/obj/*.o`、`gcc/obj/sensor_fw.elf/.hex/.bin`（*.o/*.elf/*.hex/*.bin 被 .gitignore 忽略，不入库）

## 2. 编译结果（40 个翻译单元，全工程）

| 组 | 单元数 | 结果 |
|---|---:|---|
| USER/src（main/measure/interrupts/sf_i2c/encrytogate + 新增 fw_core/hall/history/optcfg/params） | 10 | 0 错误 |
| COMMON（convert/delay） | 2 | 0 错误 |
| UM2005C（app_gtimer/app_um2005C/radio/um2005C/um2005C_hal + 新增 timeout 变体） | 5 | 0 错误 |
| Libraries/src（CW32L010 标准外设库 23 个 .c） | 23 | 0 错误 |
| **合计** | **40** | **0 错误** |

编译器：`-mcpu=cortex-m0plus -mthumb -O1 -Wall -Wextra`。告警均为既有代码风格问题（encrytogate 未用函数、main 返回类型 int32_t、库宏等），无新增代码告警。

## 3. 链接结果

```
Memory region   Used Size     Region Size   %age Used
FLASH:          33,896 B      64 KB          51.72%
RAM:             1,960 B       4 KB          47.85%
```

关键符号（arm-none-eabi-nm）：
`Reset_Handler 0x47a0`、`encode_frame10 0x2e30`、`optcfg_decoder_tick 0x34dc`、`params_commit 0x420c`、`history_push 0x36c8`、`hall_isr 0x3644`、`app_um2005C_send_data_timeout 0x240`。

新增模块静态 RAM 实测（≤300 B 预算）：
s_buf=80 B（20×4B 历史环）、s_dec=43 B（解码器）、s_pending_v/txn=28 B、s_params=20 B、s_hall_event=1 B、窗口标志/计数=8 B → **新增合计 180 B**。

## 4. 宿主机 L0 测试编译

`test/build_test.sh`：`gcc -std=c11 -Wall -Wextra` 编译 fw_core.c + history.c + host_sensor_core_test.c → 运行 56/56 通过（详见 protocol_test/driver_test 证据）。

## 5. 限制与交接

- 本证据为 GNU 交叉编译验证（类型/链接级）；产品量产构建走 Keil MDK / IAR EWARM（既有工程），需在对应工具链复编译确认。
- 中继器（CH592 beiwov2）需 WCH 工具链（MounRiver，config.h 由 IDE 生成），本环境不具备；改动为 app_um2006A.c 中 4 字节索引交换，风险低，交嵌入式测试前在 WCH 环境构建。
