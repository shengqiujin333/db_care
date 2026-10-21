# EV-026 / ITEM-007 实现证据

本目录是 `firmware_engineer.firmware_implementation` 对 ITEM-007（实现侧自检资产同步）的原始证据；**不是实板、独立验证或验收通过证据**。

| 文件 | 内容 |
|---|---|
| `build.md` | 无产品源码变更的构建证据（BuildEvidence 正文） |
| `driver_test.md` | 7 组 295/0 自检覆盖与负向对照（DriverTestEvidence 正文） |
| `build_incremental.log` | `mdk_build {"action":"build"}` 原始日志（0 Error/0 Warning，无可重建项） |
| `image_md5.txt` | axf/hex md5（与 ITEM-006 相同） |
| `source_sha256.txt` | build_test.sh 与 host_sensor_core_test.c SHA-256 |
| `host_tests_full.log` | `test/build_test.sh` 7 组原始输出（295/0） |

复跑环境：MinGW-w64 gcc 12.2.0；Keil ARMCLANG V6.24。
未烧录、未复位、未连接 COM43/逻辑分析仪/手机/服务器。未修改测试能力 `*_verify_ev.c`。
