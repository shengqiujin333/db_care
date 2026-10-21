# Driver/Logic Self-Test Evidence — ITEM-007

状态：实现侧自检资产同步与执行（**不表示实板/独立验收通过**）
能力：`firmware_engineer.firmware_implementation`
项目：dengbei_run10
被测资产：`test/build_test.sh`、`test/host_sensor_core_test.c` 及既有 `host_fw_core_pure_check.c`、`host_measure_flow_check.c`、`host_debug_trace_check.c`、`host_light_check.c`
编译器：MinGW-w64 gcc 12.2.0

## 1. 本次资产同步

| 资产 | 同步内容 |
|---|---|
| `host_sensor_core_test.c` | 14 向量/升温/35.0/4 参兼容/1,600 点网格；新增 as-built 旧方向负向对照，旧实现必须且仅在 v3/v6/v7 失败 |
| `host_fw_core_pure_check.c` | 已含 14 向量/4 参兼容/C_dark 1..4095 边界；本轮纳入 `build_test.sh` 执行 |
| `host_measure_flow_check.c` | 已含升温门控、判定前 p/H、冻结帧逐字节、3 次用尽/不跨周期、S.s/y |
| `host_debug_trace_check.c` | 已含 BOOT 构建标识、TXE/TXBUSY 卡死与恢复、行长/顺序 |
| `host_light_check.c` | 未标定 `valid/dark=false`、标定 3000 时 1000/999 边界 |
| `check_calib_config.sh` | 已含默认/1/4095 通过、2/0/4096 编译失败；本轮纳入 `build_test.sh` |

未修改任何 `*_verify_ev.c` 测试能力资产。

## 2. 单一入口结果

`test/build_test.sh` 原始输出：`evidence/ev026_item007/host_tests_full.log`，**7 组、295 passed / 0 failed**：

| 组 | 结果 |
|---|---:|
| [1/5] host_sensor_core_test（14 向量/网格/负向对照） | 57 / 0 |
| [1b/5] host_fw_core_pure_check | 52 / 0 |
| [2/5] host_measure_flow_check（p/H/冻结/3 次） | 72 / 0 |
| [3/5] host_debug_trace_check（含 UART 卡死） | 51 / 0 |
| [4/5a] light 未标定 | 28 / 0 |
| [4/5b] light 标定（C_dark=3000） | 29 / 0 |
| [5/5] 标定配置编译门禁 | 6 / 0 |

## 3. 验收覆盖对应

| 要求 | 证据 |
|---|---|
| 下降期望全改为 14 向量/升温 | `host_sensor_core_test` 57/0；`host_fw_core_pure_check` 52/0；无下降期望 |
| 35.0 继续评估升温 | 14 向量 v7/v8 + 4 参 `340→350 dark=true/false` |
| 判定前 prev / p/H | `host_measure_flow_check` [14] A→B→失败→C：B `p=A`、C `p=B`、失败 `q=0` |
| 冻结帧、3 次用尽、不跨周期 | `host_measure_flow_check` [9]：1 次成功/3 次失败/fail-fail-success，10B 逐字节相同、不重编码、用尽后再调用不发送 |
| UART 有界 | `host_debug_trace_check` T14/T15：TXE 卡死中止本行、TXBUSY 卡死有界关闭、仍回收引脚 |
| 未标定/标定边界 | light 28/0 + 29/0；`C_dark=1..4095`、ceil/减一 |
| 错误实现会被自检判失败 | 旧方向 replica 在 13 个可评估向量中恰好失败 v3/v6/v7；非法标定组合编译失败 |

## 4. 限制与交接

- 全部为宿主机自检；未烧录、未采集 COM43，不是实板或独立验收。
- 测试能力 `host_diag_line_verify_ev.c`（硬编码旧 BOOT）与 `host_measure_flow_verify_ev.c`（旧降温/跨轮模型）仍待其按 `test_design_v3.md` 更新；本实现未修改。
- 产品源码/镜像未变，构建与 ITEM-006 同一 md5。
