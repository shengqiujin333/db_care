# EV-027 / ITEM-008 最终构建与静态核对证据

本目录是 `firmware_engineer.firmware_implementation` 对 ITEM-008 的最终构建/静态核对证据；**不是实板、独立验证或验收通过证据**。

| 文件 | 内容 |
|---|---|
| `build.md` | 最终构建身份/尺寸/限制（BuildEvidence 正文） |
| `driver_test.md` | 12 项静态核对、S/G 行长、295/0 自检（DriverTestEvidence 正文） |
| `build_rebuild.log` | `mdk_build {"action":"rebuild"}` 原始日志（0 Error/1 既有 warning） |
| `image_md5.txt` | axf/hex md5 |
| `source_sha256.txt` | 6 个改动产品源码的 SHA-256 |
| `static_audit.py` | 退役路径/NVM/心跳/引脚/RF 与 S/G 行长静态核对脚本 |
| `static_audit.txt` | 静态核对原始输出（12/0） |
| `host_tests_full.log` | `test/build_test.sh` 7 组原始输出（295/0） |

复跑环境：MinGW-w64 gcc 12.2.0；Keil ARMCLANG V6.24。
未烧录、未复位、未连接 COM43/逻辑分析仪/手机/服务器。
