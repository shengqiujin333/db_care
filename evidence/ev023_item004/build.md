# Build Evidence — ITEM-004 UART 有界等待

状态：ITEM-004 实现侧编译证据（**不表示实板功能通过**）
能力：`firmware_engineer.firmware_implementation`
项目：dengbei_run10
父基线：Git `fdd1ff1`（ITEM-003 已提交；本 item 开始时工作区干净）
工程：`CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/Project.uvprojx`
编译器：Keil MDK ARMCLANG `V6.24`

## 1. 构建命令与结果

```text
mdk_build {"action":"build"}     # 2026-10-21T03:11:22Z → 03:11:23Z
mdk_build {"action":"rebuild"}   # 2026-10-21T03:13:19Z → 03:13:21Z
```

Rebuild 原始结果（完整日志：`evidence/ev023_item004/build_rebuild.log`）：

```text
*** Using Compiler 'V6.24', folder: 'C:\Keil_v5\ARM\ARMCLANG\Bin'
Rebuild target 'Project'
...
compiling debug_trace.c...
compiling measure.c...
...
Program Size: Code=19372 RO-data=644 RW-data=116 ZI-data=1684
FromELF: creating hex file...
".\output\exe\Project.axf" - 0 Error(s), 1 Warning(s).
Build Time Elapsed:  00:00:01
```

唯一告警仍为既有模板 `main.c(245) while(k--);` 空循环体，非本次改动引入。`debug_trace_boot/sample/bus/...` 符号仍在最终 axf 中（`fromelf --text -s` 可查），调试通道未被编译掉。

## 2. 产物身份

| 产物 | md5 |
|---|---|
| `MDK/output/exe/Project.axf` | `a0b2b8309435dae29c64e97df8a355b7` |
| `MDK/output/exe/Project.hex` | `cc7fbd8c3919621ecebbab182ad68a0e` |

相对 ITEM-003 的 `789a199e…` / `afc12baa…` 变化来自 `debug_trace.c` 的整行预算与关闭预算；**未烧录**，不代表板上镜像已更新。

## 3. 本次参与编译的源码改动

| 文件 | 改动 |
|---|---|
| `USER/inc/sensor_config.h` | 新增 `DEBUG_UART_LINE_GUARD=600000uL`、`DEBUG_UART_CLOSE_GUARD=100000uL`（8 MHz 轮询预算；行约 300 ms、关闭约 50 ms，按实际编译循环校准） |
| `USER/src/debug_trace.c` | `trace_line_begin()` 每行重置预算；`trace_putc()` 的 TXE 等待有界，超时置 `s_tx_abort` 并停止本行后续字符；`debug_trace_flush_close()` 的 TXE/TXBUSY 等待有界，超时仍复位/关时钟并把 PA05/PA06 置输入 |
| `USER/src/debug_trace.c` | 所有行函数（BOOT/BUS/IOTEST/IOSIG/VDD/S、G）在首个字符前调用 `trace_line_begin()`；S 与 G 各自独立预算 |
| `test/host_debug_trace_check.c` | 增加 TXE 卡死、TXBUSY 卡死、下一行恢复、超时仍关闭/回收引脚的自检 |

未修改采样、判定、发送、标定、协议、引脚或网关。

## 4. 限制

- 未烧录/复位/串口采集/实板验证；本文件不是真实目标通过证据。
- 预算为轮询迭代次数，实际毫秒数取决于编译后循环周期；已按设计目标并需实板/宿主机测量校准。超时只中止本行，不改变业务状态。
- ITEM-005 启动/UID 零、ITEM-006 标定配置仍未实现。
- 板上镜像身份需独立测试下载/核对；以本文件 md5 与 Git 提交为准。
