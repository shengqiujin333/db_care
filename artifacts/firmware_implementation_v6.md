# 固件实现记录（FWI-006，ITEM-004）

状态：`IMPLEMENTED`（仅 ITEM-004 UART1 诊断输出有界等待/故障隔离；**未烧录、未实板验证、未产品验收**）
能力：`firmware_engineer.firmware_implementation`
项目：dengbei_run10
父基线：Git `fdd1ff1`（ITEM-003 已提交；本 item 开始时工作区干净）
实现范围：`artifacts/firmware_tasks_v3.yaml` 第 4 项
设计依据：FDS-003 DD-07/§9、FWR-309/310、IC-003 IF08、PACC-002 PA-017
构建证据：`evidence/ev023_item004/build.md`
自测证据：`evidence/ev023_item004/driver_test.md`

---

## 1. 本次改动

| 文件 | 改动 |
|---|---|
| `CW32.../USER/inc/sensor_config.h` | 新增 `DEBUG_UART_LINE_GUARD=600000uL`、`DEBUG_UART_CLOSE_GUARD=100000uL`；注释说明 8 MHz 轮询预算与超时语义 |
| `CW32.../USER/src/debug_trace.c` | 新增 `trace_line_begin()`、`s_tx_guard`、`s_tx_abort`；`trace_putc()` 的 TXE 等待有界，超时中止本行后续字符 |
| `CW32.../USER/src/debug_trace.c` | `debug_trace_flush_close()` 的 TXE/TXBUSY 等待有界；超时仍执行 UART1 复位/关时钟、PA05/PA06 置输入、清状态 |
| `CW32.../USER/src/debug_trace.c` | BOOT/BUS/IOTEST/IOSIG/VDD 行与 S、G 行开头各自 `trace_line_begin()`；S/G 独立预算 |
| `CW32.../test/host_debug_trace_check.c` | 增加 TXE 卡死、TXBUSY 卡死、下一行恢复、超时仍关闭/回收引脚用例 |

未修改：采样、判定、发送、标定、协议、引脚、硬件、网关、Android、服务器。

完整源码 SHA-256：`evidence/ev023_item004/source_sha256.txt`。
构建产物 md5：axf `a0b2b8309435dae29c64e97df8a355b7`，hex `cc7fbd8c3919621ecebbab182ad68a0e`。

---

## 2. 行为实现

- 每行开始：`s_tx_guard=DEBUG_UART_LINE_GUARD; s_tx_abort=0`。
- `trace_putc`：等待 TXE 时递减预算；预算耗尽置 `s_tx_abort` 并返回；后续字符不再写。
- `debug_trace_flush_close`：TXE、TXBUSY 各用 `DEBUG_UART_CLOSE_GUARD` 有界等待；无论是否超时都执行原有 UART1 复位/关时钟/PA05/PA06 输入，清 `s_uart_open/s_tx_abort/s_tx_guard`。
- 不新增定时器/中断；不触碰 PA03/PA04（软 I²C）或 PA07/PA08（SWD），不复位 GPIOA。
- 超时只影响本行输出（可能截断），不改变采样/判定/发送/睡眠业务状态。
- 正常 96 B 行、启动顺序、字段顺序与行长保持不变。

---

## 3. 验证

### 3.1 卡死场景

`evidence/ev023_item004/debug_trace_item004.log`：

| 场景 | 结果 |
|---|---|
| TXE 永不置位 | 本行 0 字节；轮询 ≤ LINE_GUARD+1 |
| TXE 恢复后下一行 | 完整输出 |
| TXBUSY 永不置位 | 关闭 ≤ CLOSE_GUARD+1 |
| 关闭超时 | UART1 复位×2、关时钟×1、PA06/PA05 输入 |
| 关闭超时后 | 可重新初始化并完整打印 |

### 3.2 汇总

| 项 | 结果 | 证据 |
|---|---:|---|
| `host_debug_trace_check`（含 T14/T15） | 51 passed / 0 failed | `debug_trace_item004.log` |
| `test/build_test.sh` 全量 5 组 | **233 passed / 0 failed** | `host_tests_full.log` |

### 3.3 构建

- `mdk_build build`/`rebuild` 0 Error；唯一告警为既有 `main.c(245) while(k--);`。
- Code=19372，RO=644，RW=116，ZI=1684；`debug_trace_*` 符号仍在最终 axf 中。
- 未烧录、未复位、未连接 COM43；**不宣称实板通过**。

---

## 4. 未完成/交接

| 项 | 状态 |
|---|---|
| ITEM-005 主循环/启动/UID 零 | 未实现 |
| ITEM-006 标定配置 | 未实现 |
| 预算实板毫秒校准 | 仅宿主机轮询上限；实际循环周期需目标/基准测量 |
| 测试能力独立 harness | 由测试能力按 test_design_v3 验证；本实现未改其文件 |
| 实板/端到端 | 未执行 |

## 5. 结论

ITEM-004 的 UART1 整行/关闭有界等待、超时中止本行、关闭动作完整、无新增定时器、正常行字节不变已实现，并通过实现侧 233 项宿主机自检。结论仅覆盖该 item；实板串口、业务与产品验收未通过。
