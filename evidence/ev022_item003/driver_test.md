# Driver/Logic Self-Test Evidence — ITEM-003

状态：实现侧宿主机自检（**不表示实板通过**；未烧录/未接目标）
能力：`firmware_engineer.firmware_implementation`
项目：dengbei_run10
被测源码：当前工作树 `USER/src/measure.c`（父基线 `d4edf1f` + ITEM-003 改动）
编译器：MinGW-w64 gcc 12.2.0，`-std=c11 -Wall -Wextra`

## 1. 实现行为

- 触发（`sensor_gate_decide` 为真）时立即 `encode_frame10(mcu_uid, tempvalue, huminityvalue, send_data)`，并置 `s_pending_frame_valid=1`、`report_req=1`。
- `send_data_to_gateway()` 在同一次调用内循环最多 `SENSOR_RF_TX_RETRY=3` 次，每次都发送同一个 `send_data[10]`；每次 `app_um2005C_send_data_timeout(..., 200)`。
- 任一成功即停止；全部失败则用尽。成功后与用尽后都清 `report_req` 和 `s_pending_frame_valid`，不跨周期保留；`report_retry` 在 S 读取时为 0（IC-003 现行 y 语义）。
- 无待发时 `s_last_send_result=0`；最终结果 1/2 供 S.s 使用。

## 2. 发送路径验证

原始输出：`evidence/ev022_item003/measure_flow_item003.log` `[9]`。

| 场景 | 期望 | 实测 |
|---|---|---:|
| 未触发调用发送 | 不编码、不发送 | PASS |
| 触发一次 | `encode_frame10` 恰好 1 次 | PASS |
| 第一次成功 | 1 次发送、帧 == 触发冻结帧、不重编码、清 pending | PASS |
| 3 次全失败 | 3 次发送、3 帧逐字节相同、不重编码、用尽清 pending、下一调用不再发送 | PASS |
| fail,fail,success | 3 次尝试、3 帧逐字节相同、第 3 次成功清 pending | PASS |
| S.s / S.y | 成功 `s=1,y=0`；用尽 `s=2,y=0` | PASS |

发送桩逐次保存 `d[0..9]`，编码桩保存 `encode_frame10` 的输出；测试逐字节比较，确认重试期间不重编码、不改值。

## 3. 汇总自测

| 测试 | 结果 | 证据 |
|---|---:|---|
| `host_measure_flow_check.c`（含 [9] 冻结帧/发送） | **72 passed / 0 failed** | `measure_flow_item003.log` |
| `test/build_test.sh` 全量 5 组 | **226 passed / 0 failed** | `host_tests_full.log` |

五组：53（fw_core/14 向量/网格）+ 72（measure 流程/发送）+ 44（UART trace）+ 28/29（light 未标定/标定）。

## 4. 限制与交接

- 全部为宿主机 mock/纯逻辑自检，不是实板、独立验证或验收；未烧录/未采集 COM43。
- 测试能力拥有的 `host_measure_flow_verify_ev.c` 仍按旧“跨轮发送/重编码”模型编写，需由测试能力按 `test_design_v3.md` 重写；本实现未修改该文件。
- 本 item 未实现 UART 有界（ITEM-004）、启动/UID 零（ITEM-005）、标定配置（ITEM-006）；这些不能由本自检宣称完成。
