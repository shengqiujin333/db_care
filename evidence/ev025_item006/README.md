# EV-025 / ITEM-006 实现证据

本目录是 `firmware_engineer.firmware_implementation` 对 ITEM-006（标定配置编译门禁、未标定门禁、BOOT 构建标识）的实现侧原始证据；**不是实板、独立验证或验收通过证据**。

| 文件 | 内容 |
|---|---|
| `build.md` | 构建身份/结果/限制（BuildEvidence 正文） |
| `driver_test.md` | 编译门禁、未标定/边界、BOOT（DriverTestEvidence 正文） |
| `build_rebuild.log` | `mdk_build {"action":"rebuild"}` 原始日志 |
| `image_md5.txt` | axf/hex md5 |
| `source_sha256.txt` | sensor_config.h/debug_trace.c/host_debug_trace_check.c/check_calib_config.sh SHA-256 |
| `calib_cfg.log` | 标定配置编译门禁原始输出（6/0） |
| `debug_trace_item006.log` | host_debug_trace_check 原始输出（51/0，含新 BOOT 标识） |
| `host_tests_full.log` | test/build_test.sh 5 组原始输出（233/0） |

复跑环境：MinGW-w64 gcc 12.2.0；Keil ARMCLANG V6.24。
未烧录、未复位、未连接 COM43/逻辑分析仪/手机/服务器。
