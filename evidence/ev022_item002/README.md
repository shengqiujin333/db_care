# EV-022 证据目录（ITEM-002 判定前快照）

本目录是 `embedded_tester.embedded_verification` 对 ITEM-002 的独立验证证据。
结论与范围见 `test.md`；下载身份见 `programmed_prototype.md`。

## 文件

| 文件 | 内容 |
|---|---|
| `test.md` | TestEvidence 正文：受测身份、方法、16 周期逐条结果、负向对照、实板观测、静态核对、局限、结论 |
| `programmed_prototype.md` | 本次 `mdk_flash` 实际下载身份与日志原文（仅证明下载） |
| `build_rebuild.log` | 本能力 `mdk_build rebuild` 原始日志（0 Error / 1 既有 warning） |
| `image_md5.txt` | 本轮 axf/hex md5 |
| `source_sha256.txt` | 受测 `measure.c`、harness、旧实现、参考模型 SHA-256 |
| `git_head.txt` | 受测提交 |
| `static_refs.txt` | `rg` 静态引用原始输出（快照/判定/推进/轨迹位置） |
| `compile_current.err` / `compile_old.err` | harness 编译 stderr（无本 harness 告警） |
| `harness_current.out` / `.err` | 当前产品 `measure.c` 的逐周期原始输出 |
| `harness_old.out` / `.err` | 父提交 `17a92ba` `measure.c` 的同一刺激输出（负向对照） |
| `verify_current.out` | 独立参考模型对当前实现：224 passed / 0 failed |
| `verify_old.out` | 独立参考模型对旧实现：11 项 p/H 不符，`NEGATIVE_CONTROL_OK` |
| `verify_measure_snapshot_item002.py` | 独立 Python 参考模型与判据 |
| `old_measure_pre_item002.c` | 从 Git 只读导出的父提交 `measure.c`（证据输入，非产品源码） |
| `com43_capture_a.bin` | 下载后 COM43 原始字节（300 s，115 B，1 个周期） |
| `com43_capture_b.bin` | 下载后 COM43 原始字节（300 s，230 B，2 个周期） |
| `verify_com43_item002.py` | 实板原始字节独立解析/判据 |
| `verify_com43.out` | 实板复核：56 passed / 0 failed（含成功路径未执行的限制声明） |
| `host_tests_regression.log` | 实现侧既有宿主机套件回归（5 组 206/0，仅参考） |

## 边界

- 宿主机 harness 真实编译产品 `measure.c/fw_core.c/sf_i2c.c/encrytogate.c`；光照、GXHT40 驱动、RF 发送为脚本化原语。
- 实板当前 GXHT40 无 ACK（`G a44=0 a45=0`），成功采样路径未出现；成功周期 `p/t` 只在宿主机证明。
- 未做逻辑分析仪、RF/网关/手机/云、功耗与电气测量。
- `harness_*.exe` 为本地构建产物，不入库。
