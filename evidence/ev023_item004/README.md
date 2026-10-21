# EV-023 / ITEM-004 实现证据

本目录是 `firmware_engineer.firmware_implementation` 对 ITEM-004（UART1 诊断输出有界等待/故障隔离）的实现侧原始证据；**不是实板、独立验证或验收通过证据**。

| 文件 | 内容 |
|---|---|
| `build.md` | 构建身份/结果/限制（BuildEvidence 正文） |
| `driver_test.md` | TXE/TXBUSY 卡死、恢复、关闭动作与汇总（DriverTestEvidence 正文） |
| `build_rebuild.log` | `mdk_build {"action":"rebuild"}` 原始日志（0 Error/1 既有 warning，Code/RO/RW/ZI） |
| `image_md5.txt` | axf/hex md5 |
| `source_sha256.txt` | sensor_config.h、debug_trace.c、host_debug_trace_check.c SHA-256 |
| `debug_trace_item004.log` | `host_debug_trace_check` 原始输出（51/0，含 T14/T15） |
| `host_tests_full.log` | `test/build_test.sh` 5 组原始输出（233/0） |

复跑环境：MinGW-w64 gcc 12.2.0；Keil ARMCLANG V6.24。
未烧录、未复位、未连接 COM43/逻辑分析仪/手机/服务器。
