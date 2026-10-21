# EV-028 — ITEM-008 最终构建与静态核对（独立验证证据）

本目录由 `embedded_tester.embedded_verification` 生成，用于独立验证当前最终固件
（Git `c176d02` / `8ae4a33` 之后）的构建身份、退役路径/资源边界与 S/G 行 96 B 预算。

| 文件 | 内容 |
|---|---|
| `test.md` | **TestEvidence 正文**：受测身份、独立构建、23 项静态核对、S/G 预算、实板观测、局限、TEST_PASS（本 item 范围） |
| `programmed_prototype.md` | 本次实际下载的固件身份与目标；仅证明下载（Verify OK），不代表功能合格 |
| `build_rebuild.log` | 本能力 `mdk_build rebuild` 原始日志（V6.24，0 Error / 1 既有 warning） |
| `image_md5.txt` | 构建后 axf/hex md5 与大小 |
| `image_md5_after_flash.txt` | 下载后 axf/hex md5（与构建一致） |
| `source_sha256.txt` | 6 个受测产品源码 SHA-256 |
| `run_context.txt` | UTC/本地时间、Git HEAD、对比基线 |
| `static_audit_item008.py` | 本能力自建静态核对脚本（不引用实现侧脚本） |
| `static_audit.out` | 静态核对原始输出（23 passed / 0 failed） |
| `host_tests_regression.log` | 既有实现侧套件在最终树上的回归（7 组 295 passed / 0 failed，仅参考） |
| `compile_diag_line.err` | 诊断行 harness 编译 stderr（空） |
| `diag_line_verify.exe` | 直编当前 `USER/src/debug_trace.c` + 自建影子 MCU 的 harness（构建产物，被 .gitignore 忽略） |
| `diag_line_verify.out` | S/G/BUS/IOTEST/IOSIG/VDD/BOOT 最坏行原始输出（36 passed / 0 failed；worst S=92 B，G=46 B） |
| `verify_com43_item008.py` | COM43 原始字节独立解析/判据 |
| `verify_com43.out` | 实板解析结果（13 passed / 0 failed） |
| `com43_capture_a.dat` | 下载后 COM43 9600 8N1、300 s 原始字节（112 B，md5 `5a6a5731ce57e07d0056a21e991a6a90`） |
| `mdk_flash.log` | `mdk_flash` 原始日志（Erase/Program/Verify OK/Application running） |

## 证据边界

- 全部结论仅覆盖 ITEM-008 的构建、静态边界与诊断行长度；**不是**产品验收。
- 实板窗口只捕获到一个失败周期（`q=0` + `G s=2`），对应已知 U01（GXHT40 无 ACK）；
  BOOT 为上电一次性输出，本次未入窗，标识由宿主机 harness 验证。
- U02（全暗 `C_dark` 标定）未完成；端到端/真实总功能测试不属本 item。
