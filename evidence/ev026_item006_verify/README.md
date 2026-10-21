# EV-026 证据清单 — ITEM-006 标定配置收口与构建标识

| 文件 | 用途 |
|---|---|
| `test.md` | 验证证据正文（结论 TEST_PASS；范围与限制） |
| `programmed_prototype.md` | 本次实际下载的固件身份与目标（仅证明下载） |
| `build_rebuild.log` | Keil rebuild 原始日志（0 Error / 1 既有 warning） |
| `mdk_flash.log` | 下载原始日志（Verify OK / Application running） |
| `image_md5.txt` | axf/hex md5 |
| `source_sha256.txt` | 被测源码 SHA-256 |
| `git_head.txt` / `run_timestamp.txt` | 受测提交与运行时间 |
| `serial_ports.txt` | serial_list_ports 原始摘要（COM43/COM26） |
| `check_calib_gate_item006.py` / `calib_gate.out` | 独立编译期门禁检查（11/0，含具体 `#error` 文本） |
| `static_check_item006.py` / `static_check.out` | 独立静态核对（23/0） |
| `calib_delivery.out` | 未标定交付配置 harness（48/0） |
| `calib_cal1.out` / `calib_cal3000.out` / `calib_cal4095.out` | 标定变体 harness（各 43/0，全域 8190 断言） |
| `calib_*.exe` / `compile_*.err` | harness 产物与编译 stderr（无错误） |
| `diag_line_verify.out` | 真实 `debug_trace.c` 的 BOOT 行逐字节（36/0） |
| `diag_line_verify_old.out` / `old_debug_trace_pre_item006.c` | 负向对照：旧源码打印 `FD-002r4`，同一检查失败（35/1） |
| `com43_capture_a.bin` / `com43_capture_b.bin` | COM43 9600 8N1 原始字节（90 s + 200 s） |
| `capture_metadata.txt` | 捕获窗口、字节数、md5 |
| `verify_com43_item006.py` / `verify_com43.out` | 实板原始字节独立解析（58/0） |
| `host_tests_regression.log` | 实现侧既有宿主机套件 5 组 233/0（参考） |

测试能力新增/修改的资产（产品源码之外）：

- 新增 `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test/host_item006_calib_verify.c`
- 修改 `.../test/host_diag_line_verify_ev.c`：BOOT 期望改为 `SENSOR_FW_BUILD_ID`

未修改产品源码、Keil 工程、网关、Android、硬件或 `runtime/*.yaml`。
