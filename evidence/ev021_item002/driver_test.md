# Driver/Logic Self-Test Evidence — ITEM-002

状态：实现侧宿主机自检（**不表示实板通过**；未烧录/未接目标）
能力：`firmware_engineer.firmware_implementation`
项目：dengbei_run10
被测源码：当前工作树 `USER/src/measure.c`（父基线 `17a92ba` + ITEM-002 改动）
编译器：MinGW-w64 gcc 12.2.0，`-std=c11 -Wall -Wextra`

## 1. 实现行为

`temperature_process()` 在 `sample_flag` 检查后、任何测量之前：

```text
prev_for_decision = s_prev_temp_x10;
have_prev_for_decision = s_have_prev;
```

- 温湿度有效：用该快照调用 `sensor_gate_decide(...)`；判定后才写 `s_prev_temp_x10=tempvalue`、`s_have_prev=true`、`s_last_hum_x10=huminityvalue`。
- 温湿度失败：不判定、不推进、不发送；`s_trace_diag_valid=1`。
- 两种路径都把 `prev_for_decision/have_prev_for_decision` 存入轨迹；`sensor_trace_fetch` 输出 `p/H` 只读轨迹快照。
- 失败周期 `t/h` 保留最近有效值，但 `q=0`，不得当本次样本。

## 2. A→B→失败→C 轨迹验证

原始输出：`evidence/ev021_item002/measure_flow_item002.log`，`[14]`。

前置：进入 [14] 时 `prev=160`、`have_prev=1`（前序测试已建立）。

| 周期 | 输入 | 期望 p/H | 期望 t/q/r | 实测 |
|---|---|---|---:|---|
| A | 170，rise 10，暗 | p=160，H=1 | t=170，q=1，r=1 | PASS |
| 失败 | GXHT40_ERR_CRC（t=180） | p=170，H=1 | t=170（最近有效），q=0，r=0 | PASS |
| B | 180，rise 10，暗 | p=170，H=1 | t=180，q=1，r=1 | PASS |

关键点：C 周期 `p=170`，**不是**推进后的 180；失败周期 `p=170` 且 `q=0`，证明失败未污染前值。

## 3. 汇总自测

| 测试 | 结果 | 原始输出 |
|---|---:|---|
| `host_measure_flow_check.c`（含 [14] p/H） | **52 passed / 0 failed** | `measure_flow_item002.log` |
| `host_sensor_core_test.c`（14 向量/网格/4 参） | 53 passed / 0 failed | `host_tests_full.log` |
| `host_fw_core_pure_check.c` | 52 passed / 0 failed | `pure_check_item002.log` |
| `host_debug_trace_check.c` | 44 passed / 0 failed | `host_tests_full.log` |
| `host_light_check`（未标定/标定） | 28 + 29 passed / 0 failed | `host_tests_full.log` |
| `test/build_test.sh` 全量 5 组 | **206 passed / 0 failed** | `host_tests_full.log` |

## 4. 限制与交接

- 全部为宿主机 mock/纯逻辑自检，不是实板、独立验证或验收；未烧录/未采集 COM43。
- 测试能力拥有的 `host_measure_flow_verify_ev.c` 仍按旧降温/旧发送跨轮模型编写，需由测试能力按 `test_design_v3.md` 重写；本实现未修改该文件。
- 本 item 不改变发送/重试、UART 有界或标定；这些由 ITEM-003..006 负责。
