# EV-024 / ITEM-005 实现证据

本目录是 `firmware_engineer.firmware_implementation` 对 ITEM-005（主循环单次事务、BOOT/UID 零致命路径）的实现侧原始证据；**不是实板、独立验证或验收通过证据**。

| 文件 | 内容 |
|---|---|
| `build.md` | 构建身份/结果/限制（BuildEvidence 正文） |
| `driver_test.md` | 源码结构自检、RTC 回归、汇总（DriverTestEvidence 正文） |
| `build_rebuild.log` | `mdk_build {"action":"rebuild"}` 原始日志 |
| `image_md5.txt` | axf/hex md5 |
| `source_sha256.txt` | main.c 与静态检查脚本 SHA-256 |
| `main_static_check.py` | 16 项源码顺序/结构检查脚本 |
| `main_static_check.txt` | 上述脚本输出（16/0） |
| `rtc_verify.log` | 测试能力 RTC harness 配临时外部 stub 的回归输出（17/0） |
| `host_tests_full.log` | `test/build_test.sh` 5 组原始输出（233/0） |

复跑环境：MinGW-w64 gcc 12.2.0；Keil ARMCLANG V6.24。
未烧录、未复位、未连接 COM43/逻辑分析仪/手机/服务器。
