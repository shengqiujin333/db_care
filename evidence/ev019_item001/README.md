# EV-019 / ITEM-001 实现证据

本目录是 `firmware_engineer.firmware_implementation` 对 ITEM-001（门控纯逻辑）的实现侧原始证据；
**不是实板、独立验证或验收通过证据**。

| 文件 | 内容 |
|---|---|
| `build.md` | 构建身份/结果/限制（BuildEvidence 正文） |
| `driver_test.md` | 14 向量、1,600 点网格、4 参兼容与汇总自测（DriverTestEvidence 正文） |
| `build_rebuild.log` | `mdk_build {"action":"rebuild"}` 原始日志（0 Error/1 既有 warning，Code/RO/RW/ZI） |
| `image_md5.txt` | 本轮 axf/hex md5 |
| `source_sha256.txt` | 8 个改动源码/Self-check 文件的 SHA-256 |
| `gate_vectors.log` | `host_sensor_core_test` 原始输出（53/0） |
| `pure_check.log` | `host_fw_core_pure_check` 原始输出（52/0） |
| `measure_flow.log` | `host_measure_flow_check` 原始输出（43/0） |
| `host_tests_full.log` | `test/build_test.sh` 5 组原始输出（197/0） |

复跑环境：MinGW-w64 gcc 12.2.0；Keil MDK ARMCLANG V6.24。
未烧录、未复位、未连接 COM43/逻辑分析仪/手机/服务器；未修改产品硬件或网关。
