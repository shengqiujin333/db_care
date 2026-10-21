# EV-029 — 固件实现完整交付独立验证（n012）

本目录由 `embedded_tester.embedded_verification` 生成，用于对
`artifacts/firmware_implementation_v10.md`（FirmwareImplementation，ITEM-001..008）
做一次**完整交付**的独立验证：当前工作树构建/下载/实板观测 + 宿主独立 harness + 静态核对。

## 结论

**`TEST_FAIL`**：同一启动会话的 4 个采样机会（k=0/3/15/18）全部 `q=0`
（GXHT40 0x44/0x45 均无 ACK），未满足 PA-001 的 ≥3 连续有效周期。
宿主层全部适用判据通过；失败语义/诊断/行长/节拍正确。

## 文件

| 文件 | 内容 |
|---|---|
| `test.md` | **TestEvidence 正文**：身份、方法、逐项结果、验收映射、失败分析、最小区分实验、资产修复、限制 |
| `programmed_prototype.md` | 实际下载的固件身份与目标；仅证明下载（Verify OK） |
| `run_context.txt` | UTC/本地时间、Git HEAD、基线 |
| `source_sha256.txt` | 6 个受测产品源码 SHA-256 |
| `image_md5_build.txt` | 构建后 axf/hex md5 |
| `build_rebuild.log` | 本能力 `mdk_build rebuild` 原始日志（0 Error / 1 既有 warning） |
| `mdk_flash.log` | `mdk_flash` 原始日志（Verify OK / Application running） |
| `com43_capture_window_a.bin` | 下载后 COM43 9600 8N1、300 s 原始字节（257 B；BUS + k=0/3） |
| `com43_capture_window_b.bin` | 续采 300 s 原始字节（226 B；k=15/18） |
| `capture_md5.txt` | 两个窗口 md5 |
| `verify_com43_full.py` / `verify_com43_full.out` | 独立解析/判据：结构 79 项通过；PA-001、PA-017 两项失败 |
| `host_tests_build_test.log` | 实现侧 7 组 295/0（参考，不是独立判据） |
| `gate.out` / `verify_gate.out` | ITEM-001 14 向量 + 1936 网格 + 独立 Python 参考（PASS） |
| `snapshot.out` / `verify_snapshot.out` | ITEM-002 判定前快照（224/0） |
| `freeze.out` / `verify_freeze.out` | ITEM-003 冻结帧/3 次发送（332/0） |
| `uart.out` / `verify_uart.out` | ITEM-004 UART 有界（88/0） |
| `mainflow.out` / `verify_mainflow.out` | ITEM-005 主流程/UID 零（26/0） |
| `diagline.out` | 诊断行逐字节/96 B（36/0） |
| `calib_delivery.out` / `calib_3000.out` / `calib_gate.out` | ITEM-006 未标定/标定/编译门禁（48/0、43/0、11/0） |
| `gxht40.out` / `sfi2c.out` / `diagprobe.out` | GXHT40 驱动 93/0、软 I2C 51/0、BUS/gc/诊断 106/0 |
| `rfframe.out` / `gwcompat.out` | 10 B 帧/网关/Android 归因（10/0、11/0） |
| `vdd.out` / `rtc.out` | 供电轨测量 31/0、RTC 节拍 17/0 |
| `static_audit.out` | 独立静态核对 23/0（含源码身份） |
| `compile_*.err` / `*.err` | 各 harness 编译与运行 stderr（除特别说明为空） |
| `sigrok_capture_attempt1_metadata.json` / `attempt2_metadata.json` / `sigrok_capture_stderr_head.txt` | 逻辑分析仪两次失败的原始元数据与错误首行 |

## 边界

- 宿主/mock 结论已明确标注，不写成实板结论。
- `BOOT/VDD/IOTEST/IOSIG` 未进入实板捕获窗口（上电一次性），由宿主逐字节验证；如实记缺口。
- 逻辑分析仪为可选旁证，本次因工具输出路径缺陷未取得数据，不作为通过前提。
- 未执行遮光标定、安全加温、电流测量、网关/手机/服务器端到端。
- 本目录不修改产品源码；仅修正本能力测试资产的过期期望（见 `test.md` §6）。
