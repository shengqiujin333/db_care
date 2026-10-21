# EV-022 ITEM-003 独立验证证据清单

能力：`embedded_tester.embedded_verification`
受测项：`TL-6c897e7b-38c9-4f2a-9d55-d216ddfaaff2-tasks-ITEM-003`（触发时冻结 10 B 帧、单周期最多 3 次发送、成功/用尽清除、不跨周期、`S.s/S.y`）
结论：TEST_PASS（本增量；范围与限制见 `test.md`）

| 文件 | 用途 |
|---|---|
| `test.md` | TestEvidence 正文：身份、方法、16 周期逐条、负向对照、实板、静态、局限、结论 |
| `programmed_prototype.md` | 本次 `mdk_flash` 实际下载身份（axf/hex md5、`Verify OK`）与目标；仅证明下载 |
| `build_rebuild.log` | 本能力 `mdk_build rebuild` 原始日志（0 Error / 1 既有 warning，Code/RO/RW/ZI） |
| `mdk_flash.log` | 本能力 `mdk_flash` 原始下载日志 |
| `image_md5.txt` | 本次 axf/hex md5 |
| `git_head.txt` | 受测提交 `09d4c52` |
| `source_sha256.txt` | measure.c、harness、旧实现的 SHA-256 |
| `old_measure_pre_item003.c` | 父提交 `d4edf1f` 的 measure.c（只读导出，负向对照输入，非产品源码） |
| `harness_current.out` / `harness_current.err` | 当前产品 `measure.c` 的 16 周期原始输出（可执行文件为本地构建产物，按 §复现命令重建，不入库） |
| `harness_old.out` / `harness_old.err` | 旧实现的同刺激原始输出（负向对照） |
| `compile_current.err` / `compile_old.err` | 两次编译 stderr（仅产品 encrytogate.c 既有告警） |
| `verify_measure_freeze_item003.py` | 独立 Python 参考模型（门控/尝试次数/逐字节冻结/s·y/清除） |
| `verify_current.out` | 当前实现：332 passed / 0 failed |
| `verify_old.out` | 旧实现：22 项不符，`NEGATIVE_CONTROL_OK` |
| `com43_capture_a.dat` / `com43_capture_b.dat` | 下载后 COM43 9600 8N1 原始字节（300 s ×2） |
| `verify_com43_item003.py` | 实板原始字节独立解析/判据（非触发周期不变量、节拍、行长） |
| `verify_com43.out` | 实板复核：62 passed / 0 failed |
| `static_refs.py` / `static_refs.txt` | measure.c 冻结/发送/清除/轨迹字段的行号引用原始输出 |
| `host_tests_regression.log` | 实现侧既有宿主机套件 5 组 226/0（回归参考，非本项判据） |

## 限制

- 实板 GXHT40 对 0x44/0x45 均无 ACK（`G a44=0 a45=0`），所有周期 `q=0`，**无法在真实目标上构造触发周期**；冻结帧/3 次尝试/`s=1`/`s=2` 由宿主机 harness（真实产品 measure.c）证明，实板只证明非触发周期 `s=0/y=0`、无残留与节拍。
- 宿主机 `light_sample`/`gxht40_measure`/RF 发送/诊断为脚本化原语；RF 电气与时序、网关、App、云不在本项范围。
- `host_measure_freeze_verify_item003.c` 是测试能力新增资产，位于产品工程 `test/` 目录，不参与产品构建。
