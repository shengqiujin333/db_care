# 嵌入式验证证据（EV-024，ITEM-004 UART1 诊断输出有界等待）

能力：`embedded_tester.embedded_verification`
受测项：`TL-6c897e7b-38c9-4f2a-9d55-d216ddfaaff2-tasks-ITEM-004`
被测行为：UART1 调试输出的 TXE 与 TXBUSY 等待都使用整行/关闭预算；超时中止本行但**仍**复位并关闭 UART、PA05/PA06 置输入，随后继续睡眠/业务；不使用新定时器；不阻塞采样、判定、发送；正常 BOOT/S/G 行顺序与每行含 CRLF、≤96 B 不变。
结论：**TEST_PASS（ITEM-004 本增量；宿主机独立故障注入 + 实板正常路径观测）**
日期：2026-10-21（本地 11:21–11:33）

> 范围声明：本结论只覆盖 ITEM-004 的有界等待/中止/关闭/引脚回收与正常行不变。真实板无法在不改硬件的条件下构造 TXE/TXBUSY 永久卡死，故卡死语义由宿主机独立 harness（直接编译**当前产品** `debug_trace.c`）证明；实板证明当前镜像已下载、正常运行并按周期输出 `S`/`G` 行。GXHT40 无 ACK（U01）仍存在（每周期 `q=0`），不属于本项，也未被本项掩盖。

---

## 1. 受测件身份

| 项 | 值 | 来源 |
|---|---|---|
| Git HEAD | `7081ed9bd5b28696eea23e1a02026bff89f77dac`（ITEM-004 实现提交，工作区干净） | `git_head.txt` |
| 被测源码 | `USER/src/debug_trace.c` SHA-256 `1c2523e3a410ba82e0eef5ce068b23f5057bd0257f11abde54bd3bfbd1f62f00`；`USER/inc/sensor_config.h` `29123ecc…`；`USER/src/main.c` `c001bd70…` | `source_sha256.txt` |
| 负向对照源码 | 父提交 `fdd1ff1` 的 `debug_trace.c`（暂无预算循环） | `old_debug_trace_pre_item004.c` |
| 构建 | `mdk_build rebuild`：0 Error / 1 既有 warning（`main.c(245) while(k--);`），Code=19372 RO=644 RW=116 ZI=1684 | `build_rebuild.log` |
| 镜像 | `Project.axf` md5 `a0b2b8309435dae29c64e97df8a355b7`；`Project.hex` md5 `cc7fbd8c3919621ecebbab182ad68a0e`（与实现记录一致） | `image_md5.txt` |
| 下载 | `mdk_flash`：`Erase Done. Programming Done. Verify OK. Application running ...`（11:21:53 本地） | `mdk_flash.log`、`programmed_prototype.md` |
| 目标板 | 传感器板 CW32L010F8（GXHT40 + 光敏分压 + UM2005C） | 环境配置 + 板载固件行为 |
| 调试串口 | COM43，Prolific `067B:2303`，9600 8N1 | `serial_ports.txt` |
| 调试器 | `VID_C251/PID_F001`（COM26 为其 CDC 口，非传感器 UART1） | `usb_devices.txt` |
| 宿主机编译器 | MinGW-W64 gcc 12.2.0，`-std=c11 -Wall -Wextra`，两次编译 stderr 均为空 | `compile_current.err` / `compile_old.err` |

被测源码哈希与实现能力记录 `artifacts/firmware_implementation_v6.md` 对应；本轮重新构建得到的 axf/hex md5 与实现证据完全一致（确定性构建）。

---

## 2. 验证方法（独立于实现侧自检）

| 手段 | 文件 | 说明 |
|---|---|---|
| 测试能力自有 harness | `test/host_uart_bounded_verify_item004.c` | 直接编译**当前产品** `USER/src/debug_trace.c`；影子 MCU 层 `test/mock_uart_ev/` 只替换寄存器/引脚/外设原语，不替换产品逻辑 |
| 独立故障注入 | `mock_uart_ev/mock_uart_hw.h` | TXE READY / STUCK / SLOW（每 N 次轮询就绪一次）；TXBUSY 永久忙；按段统计轮询次数；单 CASE 轮询硬上限 20,000,000，使"无界等待"被捕获为 `UNBOUNDED` 而不是挂死 |
| 独立判据/参考模型 | `verify_uart_guard_item004.py` | 由文档格式独立重算期望字节、行长、CRLF、轮询上限与 teardown；读取 harness 原始观测判定，不调用产品代码 |
| 旧实现负向对照 | `old_debug_trace_pre_item004.c` + `harness_old.out` + `verify_old.out` | 同一 harness/同一注入编译父提交实现，必须在 TXE 卡死用例上被判为无界 |
| 静态核对 | `static_check_item004.py` / `static_check.out` | 常量、每行重置预算、TXE/TXBUSY 预算、关闭动作、无定时器/中断、`main.c` 调用顺序 |
| 实板观测 | `com43_capture_a.dat` / `com43_capture_b.dat` + `verify_com43_item004.py` | 下载后 COM43 原始字节（300 s ×2），独立解析 |
| 回归参考 | `host_tests_regression.log` | 实现侧既有宿主机套件 5 组 233/0（仅参考，不作为本项独立判据） |

复现命令（在 `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/`）：

```sh
EV=D:/mypro/beiwo2/AIP/evidence/ev024_item004_verify
gcc -std=c11 -Wall -Wextra -DSENSOR_DEBUG_UART=1 -Itest/mock_uart_ev -IUSER/inc \
    test/host_uart_bounded_verify_item004.c USER/src/debug_trace.c -o $EV/harness_current.exe
gcc -std=c11 -Wall -Wextra -DSENSOR_DEBUG_UART=1 -Itest/mock_uart_ev -IUSER/inc \
    test/host_uart_bounded_verify_item004.c $EV/old_debug_trace_pre_item004.c -o $EV/harness_old.exe
cd $EV && ./harness_current.exe > harness_current.out && ./harness_old.exe > harness_old.out
python verify_uart_guard_item004.py harness_current.out
python verify_uart_guard_item004.py harness_old.out --expect-unbounded
python static_check_item004.py
python verify_com43_item004.py com43_capture_a.dat com43_capture_b.dat
```

---

## 3. 宿主机故障注入结果（当前产品源码）

原始观测：`harness_current.out`；独立判据：`verify_current.out`（**88 checks passed / 0 failed，RESULT PASS**）。

| 用例 | 刺激 | 观测 | 判定 |
|---|---|---|---|
| `N_BOOT_ORDER` | TXE 正常；按 `main.c` 顺序调用 BOOT→VDD→IOTEST→IOSIG→BUS | 197 B，5 行逐字节与文档格式一致；每行 CRLF；50/39/49/27/33 B 均 ≤96；UART 9600@8 MHz；PA05 上拉输入、PA06 推挽输出 | PASS |
| `N_WORST_S` | 最坏字段（tick=4294967295、p/t=-1250、n/x/a=4095、h=1000） | 92 B（含 CRLF）≤96；字段顺序冻结 | PASS |
| `N_WORST_G` | 失败周期 + 最宽 G（raw 12 HEX） | S 67 B 后紧跟 G 46 B，各含 CRLF 且 ≤96 | PASS |
| `N_WORST_BUS` / `IOTEST` / `IOSIG` / `VDD` | 最宽取值 | 53 / 49 / 27 / 39 B，逐字节与文档格式一致 | PASS |
| `N_SLOW_TXE` | TXE 每 7 次轮询才就绪一次（慢但可用） | 完整 72 B 行、以 CRLF 结束、无截断；504 次轮询 = 7×72，未触及 600000 预算 | PASS |
| `F1_TXE_STUCK_LINE` | TXE **永不**置位 | 本行写 0 B；TXE 轮询 **600001 = LINE_GUARD+1** 后返回；传入快照未修改；关闭时 TXE 等待 **100001 = CLOSE_GUARD+1**，TXBUSY 有界；仍执行 UART1 复位×2、关时钟×1、PA05/PA06 输入；无 GPIOA 复位/时钟操作 | PASS |
| `F2_NEXT_LINE_RECOVERS` | TXE 恢复 | 下一行重新获得整行预算并完整输出 72 B | PASS |
| `F3_S_G_INDEPENDENT_BUDGET` | TXE 卡死 600001 次后恢复，S 带 `diag_valid=1` | S 行 0 B（中止），G 行完整 46 B；总轮询 600047 = 600001 + 46 → S 与 G **各自独立**预算 | PASS |
| `F4_CLOSE_TXE_STUCK` | 正常写行后 TXE 卡死再关闭 | 关闭 TXE 等待 100001（=CLOSE_GUARD+1）；TXBUSY 有界；teardown 完整；第二次 close 幂等（0 复位/0 关时钟/无引脚改写）；关闭超时后重新打开可完整打印 | PASS |
| `F5_CLOSE_TXBUSY_STUCK` | TXBUSY 永不清零 | TXE 1 次轮询；TXBUSY **100001（=CLOSE_GUARD+1）** 后返回；teardown 完整 | PASS |
| `F6_BUSINESS_INVARIANCE` | 5 个业务周期，分别用正常 UART 与 TXE+TXBUSY 全卡死 | 业务序列逐项相同（`BIZ_EQUAL 1`），卡死情形仍完成 5 次睡眠/业务推进（`SLEEP_STUCK 5`） | PASS |

### 3.1 负向对照（父提交实现必须被拒绝）

`harness_old.out` 用同一 harness、同一刺激编译父提交 `fdd1ff1` 的 `debug_trace.c`：

- 正常路径用例（BOOT 顺序、最坏行、慢 TXE）输出与当前实现一致 → 差异确实来自 ITEM-004；
- 在 `F1_TXE_STUCK_LINE` 上，TXE 永不置位使旧实现的 `while (TXE==RESET);` 超过 **20,000,000** 次轮询硬上限，被捕获为 `UNBOUNDED case=F1_TXE_STUCK_LINE`，进程以退出码 2 结束（未挂死）。

独立判据 `verify_old.out`：**NEGATIVE_CONTROL_OK**。这证明本用例针对的正是 ITEM-004 改变的有界语义，而不是"新旧都全过"。

---

## 4. 静态核对

`static_check.out`：**29 passed / 0 failed**。

| 检查 | 结果 |
|---|---|
| `DEBUG_UART_LINE_GUARD=600000`、`DEBUG_UART_CLOSE_GUARD=100000`、`MAXLINE=96` | 存在且为数值；行预算 ≈ 2×96 B@9600 的设计轮询量，注释明确为设计值而非实测 |
| 6 个行函数（BOOT/BUS/IOTEST/IOSIG/VDD/S） | 均在首个输出原语前调用 `trace_line_begin()` |
| G 行 | 在自身首字符前调用 `trace_line_begin()`（S/G 预算独立） |
| `trace_putc` | TXE 等待中递减 `s_tx_guard`；耗尽置 `s_tx_abort=1` 并停止本行后续字符 |
| `flush_close` | TXE、TXBUSY 各使用一次 `DEBUG_UART_CLOSE_GUARD`；超时后仍 UART1 复位断言+释放、关时钟、PA05/PA06 置输入；不触碰 GPIOA 复位/时钟 |
| 定时器/中断 | `debug_trace.c` 无 TIM/LPTIM/IRQ/NVIC/EXTI/RTOS 引用；UART 发送为轮询，无 UART 中断 |
| `main.c` 顺序 | 采样 → 发送 → S 行 → `flush_close()` → `go_to_sleep()`；UART 输出位于业务之后，关闭位于睡眠之前 |

---

## 5. 实板观测（当前镜像已实际下载）

| 项 | 事实 |
|---|---|
| 下载 | `mdk_flash`：`Erase Done. Programming Done. Verify OK. Application running ...` |
| 捕获 A | COM43 9600 8N1，300 s（03:22:02–03:27:02 UTC），112 B：1 个周期 |
| 捕获 B | COM43 9600 8N1，300 s（03:27:13–03:32:13 UTC），224 B：2 个周期 |
| 周期内容 | 每周期恰一条 `S`，失败周期紧随一条 `G`；S 66 B、G 46 B（含 CRLF，均 ≤96） |
| 节拍 | 观测 `k=3 → 6 → 9`，相邻增量 3（1 min × 3 = 180 s）；捕获窗口内无其它字节 |
| 独立复核 | `verify_com43_item004.py`：**135 checks passed / 0 failed**（`verify_com43.out`） |

```text
S k=3 p=0 H=0 V=0 o=8 n=70 x=73 a=71 D=0 t=0 h=0 q=0 r=0 s=0 y=0
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------
S k=6 p=0 H=0 V=0 o=8 n=66 x=69 a=67 D=0 t=0 h=0 q=0 r=0 s=0 y=0
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------
S k=9 p=0 H=0 V=0 o=8 n=63 x=65 a=64 D=0 t=0 h=0 q=0 r=0 s=0 y=0
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------
```

实板可证明的部分：

- 当前镜像（axf md5 `a0b2b830…`，`Verify OK`）已下载并运行；
- 正常路径 `S→G` 顺序、字段/键名/顺序、CRLF、每行 ≤96 B 与改动前一致；
- 相邻周期间隔 180 s（k 增量 3）；`s=0`、`y=0`，无发送/重试残留；
- 采样间期无字节输出（两次窗口之间的静默由窗口划分与周期数体现）。

实板**不能**证明的部分（如实登记，不改为通过）：

- **TXE/TXBUSY 永久卡死**：不在不改硬件的条件下构造，实板卡死路径未执行；由宿主机独立 harness（真实产品源码）证明，不能替代为"实板通过"。
- **上电行 `BOOT/VDD/IOTEST/IOSIG/BUS`**：本轮 `mdk_flash` 完成后才开始串口捕获（无授权的复位/电源循环通道），上电行落在捕获窗口之外，未在本次实板观测到；其顺序由 `main.c` 静态核对与真实 `debug_trace.c` 的宿主机 harness 证明（§3、§4）。`BOOT` 行内的 `fw=FD-002r4` 构建标识不属于本项验收。
- **预算的实际毫秒数**：实现以轮询次数表达（600001 / 100001）；8 MHz 下注释估算约 300 ms / 50 ms，本轮未在目标板用计时器或示波器实测循环周期。

---

## 6. 与验收行为的对照

| 验收要求 | 证据 | 结论 |
|---|---|---|
| TXE 与 TXBUSY 都使用整行/关闭预算 | `F1`/`F4`/`F5` 精确等于 GUARD+1；静态核对 | 符合 |
| 超时中止本行，但复位并关闭 UART、PA05/PA06 置输入 | `F1` 0 B + teardown `RRC65`；`F4` 关闭超时仍 teardown | 符合 |
| 随后继续睡眠/业务 | `F6` 睡眠 5/5、业务序列相同；`main.c` 顺序静态核对 | 符合 |
| 不使用新定时器 | 静态：无 TIM/IRQ/NVIC/RTOS 引用 | 符合 |
| 不阻塞采样、判定、发送 | 调试输出位于业务之后；`F6` 业务模型不受 UART 卡死影响 | 符合 |
| 正常 BOOT/S/G 顺序与每行 CRLF ≤96 B 不变 | `N_BOOT_ORDER`、`N_WORST_*`、`F2`；实板 3 个周期 S/G | 符合（实板上电行未在本次窗口内，见 §5 限制） |

---

## 7. 未执行与局限

1. 实板 UART 卡死未构造；见 §5。
2. 本轮未捕获上电诊断行；无授权的复位/电源循环入口，且 `mdk_flash` 的重置发生在捕获开始之前。
3. 预算为轮询迭代数，目标板实际毫秒未测；宿主机轮询硬上限仅用于把"无界"转为可判定的失败。
4. GXHT40 无 ACK（U01）仍存在，所有周期 `q=0`；本项不涉及也不修复。
5. 未做逻辑分析仪/示波器采集；本项结论只基于 UART 原始字节、下载日志、宿主机独立 harness 与静态核对。
6. ITEM-005（启动/UID 零路径）与 ITEM-006（标定配置）仍未实现，不属于本项。

---

## 8. 结论

- 当前受测源码（HEAD `7081ed9`，axf md5 `a0b2b830…`）的 UART1 调试输出在 TXE 卡死时于 `LINE_GUARD+1` 次轮询内中止本行，在 TXBUSY/TXE 关闭卡死时于 `CLOSE_GUARD+1` 内返回；无论是否超时都完成 UART1 复位/关时钟与 PA05/PA06 输入，且不触碰 GPIOA；下一行与 S/G 行各自重新获得预算；无新增定时器/中断。
- 独立 harness 88/88 通过；父提交负向对照在同一注入下被判为 `UNBOUNDED`（NEGATIVE_CONTROL_OK）；静态核对 29/29；实现侧全量宿主机套件 233/0（回归参考）。
- 当前镜像已实际下载（`Verify OK`）；实板 3 个周期 `S→G` 顺序、CRLF、66/46 B、180 s 节拍，独立复核 135/135。
- **ITEM-004 本增量：TEST_PASS**。范围与限制如 §1/§5/§7。
