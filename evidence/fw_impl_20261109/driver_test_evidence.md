# 固件宿主自检证据（FW-DRV-20261109）

负责角色：固件工程
执行时间：2026-11-09 16:14–16:17（Asia/Shanghai）
对象：当前 as-built 传感器源码（HEAD `fdfcd85`；产品源码 tree `09bef65`，`USER/` tree `746a72d`）
性质：**L0/L1 宿主机自检 + 构建**，不证明真实板、串口、I²C 电气或跨链路通过。

---

## 1. 汇总

| 入口 | 命令/方式 | 结果 | 日志 |
|---|---|---|---|
| `test/build_test.sh`（实现侧四阶段） | `sh build_test.sh`（MinGW-w64 gcc 12.2.0） | **218 passed / 0 failed**（46+61+54+28+29），exit 0 | `host_build_test.log` |
| `host_gxht40_check` | gcc，真实 `gxht40.c`/`sf_i2c.c`/`fw_core.c` + mock 总线，`-DSENSOR_CONFIG_NO_MCU` | **60 passed / 0 failed**，exit 0 | `host_gxht40_check.build.log`、`host_gxht40_check.log` |
| `host_sf_i2c_bus_check` | gcc，真实 `sf_i2c.c` | **42 passed / 0 failed**，exit 0 | `host_sf_i2c_check.build.log`、`host_sf_i2c_check.log` |
| `host_rf_frame_check` | gcc，真实 `encrytogate.c` + 网关 `feistel_al.c`/`bleencrypt.c` | **22 passed / 0 failed**，exit 0 | `host_rf_frame_check.build.log`、`host_rf_frame_check.log` |
| `host_fw_core_pure_check` | gcc，真实 `fw_core.c`；因该 harness 注释的编译行未包含 `sensor_config.h`，使用 `-include ../USER/inc/sensor_config.h`，未修改测试文件 | **46 passed / 0 failed**，exit 0 | `host_fw_core_pure_check.build.log`、`host_fw_core_pure_check.log` |

**合计 388 passed / 0 failed**（218 + 60 + 42 + 22 + 46）。

---

## 2. 覆盖范围

- T1：`sensor_decide_report` 九组规范向量、严格 `>35.0℃`、+0.9℃ 不报、降温不报、无前值仅超温、int16 极值/int32 差值；`lt`/边界组合。
- T2：待上报冻结、重试期间帧不变、跨采样不覆盖、用尽放弃、P 行字段/预算、打印无副作用、`SENSOR_DEBUG_UART=0` 空实现。
- T3：正常 mock 输出逐字节、`TXE`/`TXBUSY` 卡死有界返回、关闭后 GPIO/引脚所有权、单点等待常量。
- T4/F-1：GXHT40 地址探测/0xFD/6 B/双 CRC/换算/有效域/有界重读/重测/重探/总线恢复；`S` 行 `p/H` 为判定前快照（首样本 `H=0 p=0`）。
- 光照：PB05 供电/稳定/8 次 ADC/均值/极值/部分成功/全超时不成暗；未标定 `valid=0/dark=0`；标定变体 `3*mean>=C_dark` 边界；VDD BGR 反推与超时不伪造。
- 空口：433 明文 10 B 布局、Feistel 往返、篡改拒绝、网关 8 B 设备块与 Android 大端映射。
- 退役/布局：静态检查无 `hall/optcfg/params/history` 活动引用；PB04=AIN11、PB05=光照供电、PB06 未使用；UART 仅 PA05/PA06。

---

## 3. 实际命令（可复跑）

```text
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test
sh build_test.sh

gcc -std=c11 -Wall -Wextra -Wno-int-to-pointer-cast -DSENSOR_CONFIG_NO_MCU \
    -I../USER/inc -I../COMMON host_gxht40_check.c \
    ../USER/src/gxht40.c ../USER/src/sf_i2c.c ../USER/src/fw_core.c \
    -lm -o _hostbin/gxht40_check.exe && ./_hostbin/gxht40_check.exe

gcc -std=c11 -Wall -Wextra -I../USER/inc host_sf_i2c_bus_check.c \
    ../USER/src/sf_i2c.c -o _hostbin/sf_i2c_check.exe && ./_hostbin/sf_i2c_check.exe

gcc -std=c11 -Wall -Wextra -Wno-unused-function -Wno-unused-const-variable \
    -Wno-misleading-indentation -I../USER/inc \
    -I../../../../../CH592EVT/EVT/EXAM/BLE/beiwov2/APP \
    host_rf_frame_check.c ../USER/src/encrytogate.c \
    ../../../../../CH592EVT/EVT/EXAM/BLE/beiwov2/APP/feistel_al.c \
    ../../../../../CH592EVT/EVT/EXAM/BLE/beiwov2/APP/bleencrypt.c \
    -o _hostbin/rf_frame_check.exe && ./_hostbin/rf_frame_check.exe

gcc -std=c11 -Wall -Wextra -DSENSOR_CONFIG_NO_MCU \
    -include ../USER/inc/sensor_config.h -I../USER/inc \
    host_fw_core_pure_check.c ../USER/src/fw_core.c -lm \
    -o _hostbin/fw_core_pure_check.exe && ./_hostbin/fw_core_pure_check.exe
```

说明：
- `host_rf_frame_check` 首次链接仅漏 `bleencrypt.c`，导致 `build_device_block`/`build_padded_frame_blocks` 未定义；补齐该既有网关源文件后通过。未修改产品源码或测试断言。
- `host_fw_core_pure_check` 的文件头编译注释只写 `-I../USER/inc`，但源码使用 `LIGHT_DARK_*` 宏；本次用 `-include` 提供配置头，未改测试文件。若后续要把该 harness 纳入默认脚本，应由实现侧补一行 include；本轮不改，避免无关文件变化。
- 宿主机 gcc 没有 MCU 寄存器/真实 UART/I²C/ADC；本证据不证明实板时序、电气、功耗、串口输出或跨链路。相关实板项由测试能力按 TD-002 rev 7 执行。

---

## 4. 未执行项

- 未执行 `mdk_flash`/下载/复位/COM43 串口/逻辑分析仪/手机/网关/功耗测量：当前能力只授权 `mdk_build`，无可用的设备工具或外部动作。
- 未执行 T5 受光/全遮光双态标定；`LIGHT_DARK_REF_CODE=0`、`LIGHT_DARK_CALIBRATED=0` 保持，暗态 3914/3915/3932 仅作候选。
- 未执行上电五行/uid、真实触发/433、跨链路、CR2032 峰值/深睡电流；均如实留待测试/硬件责任方。

---

## 5. 静态不变量核对（本轮只读扫描）

| 项 | 结果 |
|---|---|
| 上报常量 | `sensor_config.h` 仅见 `SENSOR_REPORT_RISE_X10=9`、`SENSOR_REPORT_HIGH_X10=350`；全 `USER/` 无 `SENSOR_REPORT_DROP_X10` 命中 |
| 判定实现 | `fw_core.c` 使用 `cur > SENSOR_REPORT_HIGH_X10` 与 `cur-prev > SENSOR_REPORT_RISE_X10`，int32 差值 |
| 退役通路 | `hall/OPTCFG/params/history` 仅在历史注释中出现；无活动源文件/工程引用 |
| 引脚所有权 | `PB06` 仅出现在“不得再配置”的注释；PB04/PB05 归 `light.c`；UART1 归 `debug_trace.c` |
| 未标定门禁 | `LIGHT_DARK_REF_CODE=0`、`LIGHT_DARK_CALIBRATED=0` 保持；暗态读数不触发 `valid/dark` |
| 源码差异 | `git diff --stat ef5caa6 HEAD -- USER/ test/` 为空 |

以上为静态/宿主机结论；真实目标、串口、I²C 电气、功耗与跨链路仍未验证。
