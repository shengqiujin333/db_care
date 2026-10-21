# EV-025 证据清单 — ITEM-005 主循环单次事务 / BOOT / UID 零

| 文件 | 用途 |
|---|---|
| `test.md` | 验证证据正文（结论 TEST_PASS；范围与限制） |
| `programmed_prototype.md` | 实际下载的固件身份与目标（仅证明下载） |
| `build_rebuild.log` | Keil rebuild 原始日志（0 Error / 1 既有 warning） |
| `image_md5.txt` | axf/hex md5 |
| `source_sha256.txt` | 被测源码 SHA-256 |
| `git_head.txt` / `run_timestamp.txt` | 受测提交与运行时间 |
| `mdk_flash.log` / `mdk_flash_second.log` | 两次下载原始日志（均 Verify OK） |
| `com43_capture_a.bin` / `com43_capture_b.bin` | COM43 9600 8N1 原始字节（300 s×2） |
| `verify_com43_item005.py` / `verify_com43.out` | 实板原始字节独立解析（44/0） |
| `harness_main_flow.exe` / `harness_main_flow.out` | 当前 `main.c` 主流程 harness 原始输出（26/0） |
| `verify_main_flow_item005.py` / `verify_current.out` | 主流程输出的独立判据（26/0） |
| `old_main_pre_item005.c` | 父提交 `847b230` 的 `main.c`（负向对照） |
| `harness_main_flow_old.exe` / `harness_main_flow_old.out` | 父提交在同一 harness 下的观测（10/16 fail） |
| `verify_old.out` | 负向对照独立判据（NEGATIVE_CONTROL_OK） |
| `static_check_item005.py` / `static_check.out` | 静态与 GPIOA/I²C/SWD/UART 边界核对（47/0） |
| `rtc_verify.out` / `harness_rtc_cadence.exe` | 当前 `main.c` 的 RTC 1 min×3 回归（17/0） |
| `host_tests_regression.log` | 既有宿主机套件 5 组 233/0（参考） |
| `serial_ports.txt` / `usb_devices.txt` | 端口与 USB 枚举原始摘要 |
| `compile_main.err` / `compile_harness.err` / `compile_old_main.err` / `compile_rtc_*.err` / `compile_stubs.err` / `link_main_flow.err` | gcc 编译/链接 stderr（仅 `main.c` 一处既有隐式声明 warning，其余为空） |

测试能力新增资产（产品源码之外）：

- `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test/host_main_flow_verify_item005.c`
- `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test/host_item005_link_stubs.c`

未修改产品源码、网关、Android、硬件或 `runtime/*.yaml`。
