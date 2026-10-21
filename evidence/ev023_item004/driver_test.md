# Driver/Logic Self-Test Evidence — ITEM-004

状态：实现侧宿主机自检（**不表示实板通过**；未烧录/未接目标）
能力：`firmware_engineer.firmware_implementation`
项目：dengbei_run10
被测源码：当前工作树 `USER/src/debug_trace.c` + `USER/inc/sensor_config.h`（父基线 `fdd1ff1` + ITEM-004 改动）
编译器：MinGW-w64 gcc 12.2.0，`-std=c11 -Wall -Wextra -DSENSOR_DEBUG_UART=1`

## 1. 实现行为

- 每个 `debug_trace_*` 行开始时 `trace_line_begin()` 重置 `s_tx_guard=DEBUG_UART_LINE_GUARD`、`s_tx_abort=0`。
- `trace_putc()` 在 TXE 未就绪时递减预算；预算为 0 时置 `s_tx_abort=1` 并返回，本行后续字符不再写。
- `debug_trace_flush_close()` 的 TXE 与 TXBUSY 等待各自使用 `DEBUG_UART_CLOSE_GUARD`；超时仍执行：UART1 复位/释放、关 UART1 时钟、PA06/PA05 置输入、`s_uart_open=0`。
- 不使用新定时器/中断；不修改采样、判定、发送或业务状态。
- S 行与紧随的 G 行各自获得独立行预算。

## 2. 卡死场景验证

原始输出：`evidence/ev023_item004/debug_trace_item004.log` `[T14]`/`[T15]`。

| 场景 | 期望 | 实测 |
|---|---|---:|
| TXE 永不置位 | 本行不写任何字节；TXE 轮询不超过 `DEBUG_UART_LINE_GUARD+1` | PASS |
| TXE 恢复后下一行 | 重新获得预算并完整输出 | PASS |
| TXBUSY 永不置位 | 关闭在 `DEBUG_UART_CLOSE_GUARD+1` 内返回 | PASS |
| 关闭超时后 | UART1 复位×2、关时钟×1、PA06/PA05 置输入 | PASS |
| 关闭超时后下一行 | 可重新初始化并完整打印 | PASS |

正常 96 B 行、启动顺序、S/G 格式、行长、BUS/IOTEST/IOSIG/VDD 行均保持既有字节输出（`[T1]`–`[T13]`）。

## 3. 汇总自测

| 测试 | 结果 | 证据 |
|---|---:|---|
| `host_debug_trace_check.c`（含 T14/T15） | **51 passed / 0 failed** | `debug_trace_item004.log` |
| `test/build_test.sh` 全量 5 组 | **233 passed / 0 failed** | `host_tests_full.log` |

五组：53（fw_core）+ 72（measure）+ 51（UART trace）+ 28/29（light）。

## 4. 限制与交接

- 全部为宿主机影子 UART/mock，不是实板、独立验证或验收；未烧录/未采集 COM43。
- 预算为轮询迭代数；实际毫秒数取决于编译循环，需实板/宿主机测量校准；超时只中止本行。
- 本 item 不覆盖启动/UID 零（ITEM-005）或标定配置（ITEM-006）。
