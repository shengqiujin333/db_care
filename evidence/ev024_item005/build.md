# Build Evidence — ITEM-005 主循环/启动/UID 零

状态：ITEM-005 实现侧编译证据（**不表示实板功能通过**）
能力：`firmware_engineer.firmware_implementation`
项目：dengbei_run10
父基线：Git `847b230`（ITEM-004 已提交；本 item 开始时工作区干净）
工程：`CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/Project.uvprojx`
编译器：Keil MDK ARMCLANG `V6.24`

## 1. 构建命令与结果

```text
mdk_build {"action":"build"}     # 2026-10-21T03:41:08Z → 03:41:10Z
mdk_build {"action":"rebuild"}   # 2026-10-21T03:41:20Z → 03:41:22Z
```

Rebuild 原始结果（完整日志：`evidence/ev024_item005/build_rebuild.log`）：

```text
*** Using Compiler 'V6.24', folder: 'C:\Keil_v5\ARM\ARMCLANG\Bin'
Rebuild target 'Project'
...
../USER/src/main.c(252): warning: while loop has empty body [-Wempty-body]
  252 |         while(k--);
...
Program Size: Code=19400 RO-data=644 RW-data=116 ZI-data=1684
FromELF: creating hex file...
".\output\exe\Project.axf" - 0 Error(s), 1 Warning(s).
Build Time Elapsed:  00:00:01
```

唯一告警为既有延时 `while(k--);` 空循环体，非本次改动引入。

## 2. 产物身份

| 产物 | md5 |
|---|---|
| `MDK/output/exe/Project.axf` | `e0437fd3405531f107bec844d815acf6` |
| `MDK/output/exe/Project.hex` | `c17f42eba0c1c38461c3768c99e68f22` |

相对 ITEM-004 的 `a0b2b830…` / `cc7fbd8c…` 变化来自 `main.c` 的 UID 致命路径与单次事务主循环；**未烧录**，不代表板上镜像已更新。

## 3. 本次参与编译的源码改动

| 文件 | 改动 |
|---|---|
| `USER/src/main.c` | 删除 UID 零时 `while(1) go_to_sleep()` 空转路径；BOOT/VDD/IOTEST/IOSIG/BUS 保持在 UID 判断之前 |
| `USER/src/main.c` | UID 和为零：清 `report_req/sample_flag`、`light_init()`（PB05 低/关 ADC）、关闭 UART，进入 `while(1) SYSCTRL_GotoDeepSleep()` |
| `USER/src/main.c` | 主循环只在 `sample_flag != 0` 时执行 `temperature_process()` + `send_data_to_gateway()` + 唯一 S（失败 G），随后关闭 UART、`go_to_sleep()`；不再依赖多轮主循环完成 RF 重试 |
| `test/.../host_rtc_cadence_verify_ev.c` | **未修改（测试能力资产）**；本轮用临时外部 stub 运行 RTC 回归，见 driver_test |

未修改：采样/判定/发送函数、UART 实现、标定、协议、引脚、硬件、网关。未新增 GPIOA 复位或引脚操作。

## 4. 限制

- 未烧录/复位/串口采集/实板验证；本文件不是真实目标通过证据。
- UID 零与单次事务的行为级独立验证属于测试设计 T-L0-19/20；本实现提供源码结构自检与 RTC 回归，不替代独立测试。
- 测试能力 `host_rtc_cadence_verify_ev.c` 需为其增加 `report_req` 与 `light_init` 桩（因 main 新增 UID 致命路径引用）；本轮未修改该测试资产。
- 板上镜像身份需独立测试下载/核对；以本文件 md5 与 Git 提交为准。
