# EV-020 证据清单（ITEM-001 门控纯逻辑独立验证）

| 文件 | 用途 |
|---|---|
| `test.md` | TestEvidence 正文：受测件身份、方法、14 向量、网格/边界、构建与静态核对、局限、结论 |
| `gate_verify_current.out` | 测试能力独立 harness 对当前产品 `fw_core.c` 的原始输出（V/G/B/C/SELF 行） |
| `gate_verify_current.exe` | 上述 harness 的本机构建产物（git 忽略，可复现） |
| `gate_verify_old.out` | 旧实现（Git `8ae4a33`）对同一 14 向量的原始输出（O 行） |
| `gate_verify_old.exe` | 旧实现负向对照构建产物（git 忽略，可复现） |
| `verify_gate_item001.py` | 独立 Python 参考模型与逐点比对脚本 |
| `verify_gate_item001.out` | 上述脚本本次运行原始输出（PASS，14 向量 + 1,936 网格） |
| `compile_current.err` / `gate_verify_current.err` / `compile_old.err` / `gate_verify_old.err` | 编译/运行 stderr（均为空，保留以证明无告警/错误） |
| `host_tests_current.log` | 既有宿主机套件 `test/build_test.sh` 回归输出（197/0，实现侧资产，仅参考） |
| `build_rebuild.log` | `mdk_build rebuild` 原始日志（0 Error / 1 既有 warning，Code=20132 RO=644 RW=116 ZI=1676） |
| `source_and_image_hash.txt` | 受测源码 SHA-256、Git HEAD、axf/hex md5 |
| `old_impl/fw_core.c`、`fw_core.h`、`sensor_config.h` | 从 Git `8ae4a33` 只读导出的旧实现（负向对照输入） |
| `old_impl/old_gate_negative_control.c` | 旧实现负向对照 harness（符号改名，不改旧语义） |

## 边界

- 本目录全部为**宿主机**证据；未烧录、未连接目标、未采集串口/逻辑分析仪。
- 结论仅覆盖 ITEM-001 门控纯逻辑；实板门控方向性受 U01（GXHT40 无 ACK）与 U02（全暗未标定）未闭合限制，见 `test.md` §6。
- `.exe` 为可复现构建产物，按 `.gitignore` 不入库；对应源码与命令见 `test.md` §2。
