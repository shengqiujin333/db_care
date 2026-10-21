# Build Evidence — ITEM-006 标定编译门禁与构建标识

状态：ITEM-006 实现侧编译证据（**不表示实板功能通过**）
能力：`firmware_engineer.firmware_implementation`
项目：dengbei_run10
父基线：Git `8f9f86c`（ITEM-005 已提交；本 item 开始时工作区干净）
工程：`CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/Project.uvprojx`
编译器：Keil MDK ARMCLANG `V6.24`

## 1. 构建命令与结果

```text
mdk_build {"action":"build"}     # 2026-10-21T04:15:45Z → 04:15:47Z
mdk_build {"action":"rebuild"}   # 2026-10-21T04:15:55Z → 04:15:57Z
```

Rebuild 原始结果（完整日志：`evidence/ev025_item006/build_rebuild.log`）：

```text
*** Using Compiler 'V6.24', folder: 'C:\Keil_v5\ARM\ARMCLANG\Bin'
Rebuild target 'Project'
...
compiling light.c...
compiling debug_trace.c...
compiling measure.c...
...
Program Size: Code=19544 RO-data=644 RW-data=116 ZI-data=1684
FromELF: creating hex file...
".\output\exe\Project.axf" - 0 Error(s), 1 Warning(s).
Build Time Elapsed:  00:00:01
```

唯一告警为既有 `main.c(252) while(k--);` 空循环体。

## 2. 产物身份

| 产物 | md5 |
|---|---|
| `MDK/output/exe/Project.axf` | `9cb97918e8832811dfcb00a93646fba1` |
| `MDK/output/exe/Project.hex` | `3af11e7adb21ba43004fc827649adb44` |

相对 ITEM-005 的 `e0437fd3…` / `c17f42eb…` 变化来自标定编译门禁、`SENSOR_FW_BUILD_ID` 与 BOOT 打印；**未烧录**，不代表板上镜像已更新。

## 3. 本次参与编译的源码改动

| 文件 | 改动 |
|---|---|
| `USER/inc/sensor_config.h` | 增加 `LIGHT_DARK_CALIBRATED` 0/1、已标定时 `LIGHT_DARK_REF_CODE` 1..4095 的 `#error` 门禁；新增 `SENSOR_FW_BUILD_ID "RUN10-FDS003"` |
| `USER/src/debug_trace.c` | BOOT 改为 `trace_puts(SENSOR_FW_BUILD_ID)`，不再硬编码 `FD-002r4` |
| `test/host_debug_trace_check.c` | `EXPECT_BOOT` 使用 `SENSOR_FW_BUILD_ID`；新增/保留 T14/T15 UART 有界用例 |
| `test/check_calib_config.sh` | 新增标定配置编译门禁自检（默认/1/4095 通过；2/0/4096 拒绝） |

未修改采样/判定/发送、光照算法（100 ms/8 次/整数均值）、协议、引脚、硬件、网关。

## 4. 限制

- 未烧录/复位/串口采集/实板验证；本文件不是真实目标通过证据。
- `LIGHT_DARK_REF_CODE` 仍为 0、`LIGHT_DARK_CALIBRATED` 仍为 0；本轮只实现编译门禁与构建标识，**不完成全暗标定**（依赖 U02 现场数据）。
- 测试能力 `host_diag_line_verify_ev.c` 仍硬编码旧 `BOOT fw=FD-002r4`，需测试能力按本轮构建标识更新；本实现未修改该测试资产。
- 板上镜像身份需独立测试下载/核对；以本文件 md5 与 Git 提交为准。
