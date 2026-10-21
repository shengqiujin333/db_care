# Build Evidence — ITEM-003 冻结帧与单周期发送

状态：ITEM-003 实现侧编译证据（**不表示实板功能通过**）
能力：`firmware_engineer.firmware_implementation`
项目：dengbei_run10
父基线：Git `d4edf1f`（ITEM-002 已提交；本 item 开始时工作区干净）
工程：`CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/Project.uvprojx`
编译器：Keil MDK ARMCLANG `V6.24`

## 1. 构建命令与结果

```text
mdk_build {"action":"build"}     # 2026-10-21T02:45:28Z → 02:45:30Z
mdk_build {"action":"rebuild"}   # 2026-10-21T02:46:35Z → 02:46:37Z
```

Rebuild 原始结果（完整日志：`evidence/ev022_item003/build_rebuild.log`）：

```text
*** Using Compiler 'V6.24', folder: 'C:\Keil_v5\ARM\ARMCLANG\Bin'
Rebuild target 'Project'
...
compiling measure.c...
...
Program Size: Code=20160 RO-data=644 RW-data=116 ZI-data=1676
FromELF: creating hex file...
".\output\exe\Project.axf" - 0 Error(s), 1 Warning(s).
Build Time Elapsed:  00:00:01
```

唯一告警仍为既有模板 `main.c(245) while(k--);` 空循环体，非本次改动引入。

## 2. 产物身份

| 产物 | md5 |
|---|---|
| `MDK/output/exe/Project.axf` | `789a199e720436dff1de2d51f2cbd9cd` |
| `MDK/output/exe/Project.hex` | `afc12baa6cca101578da15cbdec34e58` |

相对 ITEM-002 的 `e8c2ebcd…` / `badfd814…` 变化来自 `measure.c` 冻结帧与单周期发送循环；**未烧录**，不代表板上镜像已更新。

## 3. 本次参与编译的源码改动

| 文件 | 改动 |
|---|---|
| `USER/src/measure.c` | 触发时 `encode_frame10(mcu_uid, tempvalue, huminityvalue, send_data)` 一次并置 `s_pending_frame_valid=1`；`send_data_to_gateway()` 在**同一次调用**内最多 `SENSOR_RF_TX_RETRY` 次发送同一 `send_data[10]`；成功/用尽均清 `report_req` 与 `s_pending_frame_valid`；`report_retry` 在 S 前保持 0（IC-003） |
| `test/host_measure_flow_check.c` | 增强发送/编码桩以捕获每帧 10B；重写 [9]：成功 1 次、3 次失败、fail/fail/success；逐字节比较冻结帧、检查不重编码、清除/不跨周期、S.s/y |

未修改门控、判定前快照、UART、启动、标定、协议、引脚、网关或硬件。

## 4. 限制

- 未烧录/复位/串口采集/实板验证；本文件不是真实目标通过证据。
- 本 item 只完成冻结帧、单周期 3 次发送与清除；UART 有界、启动/UID 零、标定等仍未实现。
- 板上镜像身份需独立测试下载/核对；以本文件 md5 与 Git 提交为准。
