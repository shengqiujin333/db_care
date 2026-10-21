# EV-021 / ITEM-002 实现证据

本目录是 `firmware_engineer.firmware_implementation` 对 ITEM-002（判定前 prev/H 快照与 S 行 p/H）的实现侧原始证据；**不是实板、独立验证或验收通过证据**。

| 文件 | 内容 |
|---|---|
| `build.md` | 构建身份/结果/限制（BuildEvidence 正文） |
| `driver_test.md` | A→B→失败→C p/H 验证与汇总（DriverTestEvidence 正文） |
| `build_rebuild.log` | `mdk_build {"action":"rebuild"}` 原始日志 |
| `image_md5.txt` | axf/hex md5 |
| `source_sha256.txt` | measure.c 与 self-check 的 SHA-256 |
| `measure_flow_item002.log` | `host_measure_flow_check` 原始输出（52/0，含 [14]） |
| `pure_check_item002.log` | `host_fw_core_pure_check` 原始输出（52/0） |
| `host_tests_full.log` | `test/build_test.sh` 5 组原始输出（206/0） |

复跑环境：MinGW-w64 gcc 12.2.0；Keil ARMCLANG V6.24。
未烧录、未复位、未连接 COM43/逻辑分析仪/手机/服务器。
