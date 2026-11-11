# 人工确认记录（EV-033）

| 项 | 内容 |
|---|---|
| 确认时间（UTC） | 2026-11-11T02:00:31Z |
| 测试项 | `FW-DIAG-BOOT`（BOT-01/02/03）：上电五行 `BOOT→VDD→IOTEST→IOSIG→BUS` 与横幅 `uid` 的同窗捕获编排 |
| 用户回复 | **“无法准备，先跳过”** |
| 处理决定 | 记为**人工确认未执行**（`SKIP_CONFIRMED`，不是通过、不是已批准延期）；同一批准流程内不再重测该项 |
| 缺失条件（原文摘要） | 无已登记/授权的“先打开 COM43 9600 8N1 会话、再在窗口内复位/上电”采集编排入口；`serial_capture` 与 `mdk_flash` 严格串行，端口在 `Application running` 后约 12.9 s 才建立；pyocd 0.36.0 可见 CW-DAPLINK 探针但 `Target=n/a`、无 CW32 目标定义，且未在 `runtime/environment.yaml` 登记 |
| 所需人工操作（未提供） | 提供或授权可在 COM43 会话已打开时执行复位/上电的入口（同镜像 `mdk_flash` 或已登记并带 CW32L010 DFP 的 pyocd reset 等），使上电五行与 `fw/uid/rst/uart=9600` 落入同一采集窗口 |
| 保留证据 | `cap01_k3.bin`…`cap04_k21.bin`（周期 `S` 行）、`tool_results.txt`、`pyocd_check.txt`、`capture_windows.txt`；上电五行与 `uid` 仍未取得 |
| 关联延期项 | `E2E-04`（CR2032 采样峰值/深睡电流）属已批准 `DEFERABLE`，本轮缺少功耗测量仪器，未测并在 `evidence/test.md` EV-033 §5 列明 |

本记录只描述人工对测试条件的确认与未执行状态，不改变任何验收判据；最终结果为 `TEST_FAIL`（`failure_cause=EXTERNAL_CONDITION_UNEXECUTED`）。
