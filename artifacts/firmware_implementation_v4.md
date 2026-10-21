# 固件实现记录（FWI-004，ITEM-002）

状态：`IMPLEMENTED`（仅 ITEM-002 判定前快照与 S 行 p/H；**未烧录、未实板验证、未产品验收**）
能力：`firmware_engineer.firmware_implementation`
项目：dengbei_run10
父基线：Git `17a92ba`（ITEM-001 已提交；本 item 开始时工作区干净）
实现范围：`artifacts/firmware_tasks_v3.yaml` 第 2 项
设计依据：FDS-003 DD-02/§4/§6、IC-003 IF02/IF08、FWR-306/309、PACC-002 PA-017
构建证据：`evidence/ev021_item002/build.md`
自测证据：`evidence/ev021_item002/driver_test.md`

---

## 1. 本次改动

| 文件 | 改动 |
|---|---|
| `CW32.../USER/src/measure.c` | 每个采样机会在测量前快照 `prev_for_decision/have_prev_for_decision`；成功时用快照判定，判定后才推进 `s_prev_temp_x10/s_have_prev/s_last_hum_x10`；失败不推进；轨迹保存快照 |
| `CW32.../USER/src/measure.c` | `sensor_trace_fetch()` 的 `p/H` 改为读 `s_trace_prev_temp_x10/s_trace_have_prev`，不再读取推进后的 live 值 |
| `CW32.../test/host_measure_flow_check.c` | 新增 `[14]` A（160→170）→失败→B（170→180）的 p/H 与 q/r 检查 |

未修改：门控纯逻辑（ITEM-001）、发送/重试、UART、启动、标定、协议、引脚、硬件、网关。

完整源码 SHA-256：`evidence/ev021_item002/source_sha256.txt`（measure.c 与测试文件）。
构建产物 md5：axf `e8c2ebcdb927930a3366761e4f2999c9`，hex `badfd8142dbf316390e258a8af389486`。

---

## 2. 行为实现

```c
/* 采样机会开始、任何测量之前 */
prev_for_decision      = s_prev_temp_x10;
have_prev_for_decision = s_have_prev;

if (measure_sample()) {
    rep = sensor_gate_decide(true, prev_for_decision, have_prev_for_decision,
                             tempvalue, s_light.valid, s_light.dark);
    /* 判定后才推进 */
    s_prev_temp_x10 = tempvalue;
    s_have_prev = true;
    s_last_hum_x10 = huminityvalue;
} else {
    /* 不判定、不推进、不发送；取 G 诊断 */
}

s_trace_prev_temp_x10 = prev_for_decision;
s_trace_have_prev     = have_prev_for_decision ? 1u : 0u;
```

- 成功周期：`S.p` 为判定前的上一有效温度，`S.t` 为本次值；允许 `p != t`。
- 失败周期：`S.p/H` 保持失败前状态，`q=0`，`t/h` 保留最近有效值；`prev` 不变。
- 发送/上报判定仍使用触发时的 `prev` 快照；本 item 未改冻结帧（ITEM-003）。

---

## 3. 验证

### 3.1 A→B→失败→C

`evidence/ev021_item002/measure_flow_item002.log` `[14]`：

| 周期 | p/H | t/q/r | 结果 |
|---|---|---|---:|
| A（160→170，暗） | 160/1 | 170/1/1 | PASS |
| 失败（CRC） | 170/1 | 170/0/0 | PASS |
| B（170→180，暗） | 170/1 | 180/1/1 | PASS |

C 周期 `p=170` 而非推进后的 180，证明轨迹为判定前快照；失败周期 `p=170` 且 `q=0`，证明失败未推进。

### 3.2 汇总

| 项 | 结果 | 证据 |
|---|---:|---|
| `host_measure_flow_check`（含 [14]） | 52 passed / 0 failed | `measure_flow_item002.log` |
| `host_fw_core_pure_check` | 52 passed / 0 failed | `pure_check_item002.log` |
| `test/build_test.sh` 全量 5 组 | **206 passed / 0 failed** | `host_tests_full.log` |

### 3.3 构建

- `mdk_build build`/`rebuild` 0 Error；唯一告警为既有 `main.c(245) while(k--);`。
- Code=20168，RO=644，RW=116，ZI=1676；相对 ITEM-001 Code +36。
- 未烧录、未复位、未连接 COM43；**不宣称实板通过**。

---

## 4. 未完成/交接

| 项 | 状态 |
|---|---|
| ITEM-003 冻结 10B 帧/单周期 3×200ms/不跨周期 | 未实现；当前仍重新编码/多轮主循环重试 |
| ITEM-004 UART 有界 | 未实现 |
| ITEM-005 主循环/启动/UID 零 | 未实现 |
| ITEM-006 标定配置 | 未实现 |
| 测试能力独立 harness | `host_measure_flow_verify_ev.c` 仍按旧降温/跨轮模型，需测试能力重写；本实现未修改 |
| 实板/端到端 | 未执行 |

## 5. 结论

ITEM-002 的判定前快照、成功后推进、失败不推进、`S.p/H` 轨迹语义已实现并通过实现侧 206 项宿主机自检。结论仅覆盖该 item；实板、RF/网关/App/云与产品验收未通过。
