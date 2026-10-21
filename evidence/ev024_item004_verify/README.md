# EV-024 ITEM-004 独立验证证据清单

能力：`embedded_tester.embedded_verification`
受测项：`TL-6c897e7b-38c9-4f2a-9d55-d216ddfaaff2-tasks-ITEM-004`（UART1 诊断输出整行/关闭有界等待；超时中止本行但复位并关闭 UART、PA05/PA06 置输入；随后继续业务/睡眠；不使用新定时器；正常 BOOT/S/G 行顺序、CRLF 与 ≤96 B 不变）
结论：**TEST_PASS（ITEM-004 本增量；宿主机独立故障注入 + 实板正常路径观测）**，范围与限制见 `test.md`

| 文件 | 用途 |
|---|---|
| `test.md` | TestEvidence 正文：受测身份、方法、故障注入逐项、实板观测、静态核对、局限、结论 |
| `programmed_prototype.md` | 本次 `mdk_flash` 实际下载身份（axf/hex md5、`Verify OK`）与目标；仅证明下载 |
| `build_rebuild.log` | 本能力 `mdk_build rebuild` 原始日志（0 Error / 1 既有 warning，Code=19372 RO=644 RW=116 ZI=1684） |
| `mdk_flash.log` | 本能力 `mdk_flash` 原始下载日志（`Erase Done. Programming Done. Verify OK. Application running ...`） |
| `image_md5.txt` | 本次 axf/hex md5（与实现记录 `a0b2b830…` 一致） |
| `git_head.txt` | 受测提交 `7081ed9` |
| `source_sha256.txt` | debug_trace.c / sensor_config.h / main.c / 本 harness / 旧实现源码 SHA-256 |
| `old_debug_trace_pre_item004.c` | 父提交 `fdd1ff1` 的 debug_trace.c（只读导出，负向对照输入，非产品源码） |
| `harness_current.out` / `.err` | 当前产品 `debug_trace.c` 的 14 个用例原始观测（可执行文件为本地构建产物，按复现命令重建，不入库） |
| `harness_old.out` / `.err` | 父提交实现的同刺激原始观测（在 TXE 卡死用例被硬上限捕获为 `UNBOUNDED`） |
| `compile_current.err` / `compile_old.err` | 两次编译 stderr（均为空，无告警） |
| `verify_uart_guard_item004.py` | 独立 Python 判据/参考模型（逐字节格式、CRLF、轮询预算、teardown、业务不变） |
| `verify_current.out` | 当前实现：88 checks passed / 0 failed，`RESULT PASS` |
| `verify_old.out` | 父提交负向对照：`NEGATIVE_CONTROL_OK`（旧实现无界） |
| `static_check_item004.py` / `static_check.out` | 静态核对脚本与输出：29 passed / 0 failed（常量、每行预算、TXE/TXBUSY、无定时器/中断、main.c 顺序） |
| `com43_capture_a.dat` / `com43_capture_b.dat` | 下载后 COM43 9600 8N1 原始字节（300 s ×2，共 3 个周期） |
| `verify_com43_item004.py` / `verify_com43.out` | 实板原始字节独立解析：135 passed / 0 failed |
| `host_tests_regression.log` | 实现侧既有宿主机套件 5 组 233/0（回归参考，非本项独立判据） |
| `serial_ports.txt` / `usb_devices.txt` | 本轮 `serial_list_ports` / `usb_list_devices` 枚举原始结果（COM43、COM26、C251:F001、fx2lafw） |

产品工程新增的测试能力资产（不参与产品构建）：

| 文件 | 用途 |
|---|---|
| `CW32.../test/host_uart_bounded_verify_item004.c` | 独立 harness：真实产品 `debug_trace.c` + 影子 UART，故障注入与原始观测 |
| `CW32.../test/mock_uart_ev/` | 测试能力自建影子 MCU 层（TXE READY/STUCK/SLOW、TXBUSY 卡死、轮询计数、硬上限） |

## 限制

- 真实板无法在不改硬件的条件下构造 UART TXE/TXBUSY 永久卡死；卡死、中止、关闭预算与业务不变由宿主机独立 harness（真实产品源码）证明，实板只证明正常路径。
- 本轮 `mdk_flash` 完成后才开始串口捕获，`BOOT/VDD/IOTEST/IOSIG/BUS` 上电行在捕获窗口之外；上电顺序与逐行字节由真实 `debug_trace.c` 的宿主机 harness + `main.c` 静态顺序证明，实板只观测到 `S→G` 正常行。
- 预算以轮询次数表达；目标板上的实际毫秒数未用示波器/计时器实测。
- GXHT40 对 0x44/0x45 均无 ACK（U01）仍存在，所有实板周期 `q=0`；这不属于 ITEM-004，也不由本项修复或掩盖。
