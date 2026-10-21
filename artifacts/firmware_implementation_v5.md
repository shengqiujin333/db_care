# 固件实现记录（FWI-005，ITEM-003）

状态：`IMPLEMENTED`（仅 ITEM-003 冻结帧、单周期 3 次发送、成功后/用尽清除；**未烧录、未实板验证、未产品验收**）
能力：`firmware_engineer.firmware_implementation`
项目：dengbei_run10
父基线：Git `d4edf1f`（ITEM-002 已提交；本 item 开始时工作区干净）
实现范围：`artifacts/firmware_tasks_v3.yaml` 第 3 项
设计依据：FDS-003 DD-03/04/05/11、IC-003 IF02、FWR-307、PACC-002 PA-010/PA-020
构建证据：`evidence/ev022_item003/build.md`
自测证据：`evidence/ev022_item003/driver_test.md`

---

## 1. 本次改动

| 文件 | 改动 |
|---|---|
| `CW32.../USER/src/measure.c` | 触发时一次性 `encode_frame10(mcu_uid, tempvalue, huminityvalue, send_data)` 并置 `s_pending_frame_valid=1`；`report_req=1`、`s_last_send_result=0` |
| `CW32.../USER/src/measure.c` | `send_data_to_gateway()` 改为单次调用内最多 3 次循环，全部发送同一 `send_data[10]`，每次 200ms；成功或用尽后清 `report_req`/`s_pending_frame_valid`，`report_retry=0` |
| `CW32.../test/host_measure_flow_check.c` | 发送桩保存每次实际 10B；编码桩保存输出；[9] 覆盖成功 1 次、3 次失败、fail/fail/success、逐字节冻结、不重编码、不跨周期、S.s/y |

未修改：`fw_core` 门控、判定前快照、UART、启动、标定、协议、引脚、硬件、网关。

完整源码 SHA-256：`evidence/ev022_item003/source_sha256.txt`（measure.c 与测试文件）。
构建产物 md5：axf `789a199e720436dff1de2d51f2cbd9cd`，hex `afc12baa6cca101578da15cbdec34e58`。

---

## 2. 行为实现

### 2.1 触发冻结

```c
if (rep != 0u) {
    encode_frame10(mcu_uid, tempvalue, huminityvalue, send_data);
    s_pending_frame_valid = 1u;
    report_req = 1u;
    s_last_send_result = 0u;
}
```

- 编码参数是触发本次判定的当前样本（`tempvalue/huminityvalue`），不是后续推进值。
- 只编码一次；后续重试不调用 `encode_frame10`。

### 2.2 单周期发送

```c
for (attempt = 0; attempt < SENSOR_RF_TX_RETRY; attempt++) {
    if (app_um2005C_send_data_timeout(send_data, 10, 200)) {
        result = 1; break;
    }
    result = 2;
}
report_req = 0;
s_pending_frame_valid = 0;
report_retry = 0;
s_last_send_result = result;
```

- 3 次尝试全部使用同一 `send_data[10]`，不重新编码、不重采样、不改值。
- 成功即停止；用尽结束本周期；不跨周期保留。
- `S.s` 填最终 `s_last_send_result`（0/1/2）；`S.y` 为 0（IC-003 现行 y 语义，成功/用尽后清零）。
- 若 `s_pending_frame_valid=0` 而 `report_req=1`（防御），不发送旧数据，按本地失败结束。

---

## 3. 验证

### 3.1 发送场景

`evidence/ev022_item003/measure_flow_item003.log` `[9]`：

| 场景 | 结果 |
|---|---|
| 未触发 | 不编码/不发送 |
| 触发 | 编码 1 次 |
| 第一次成功 | 1 次发送；帧与触发冻结帧逐字节相同；清除 pending |
| 3 次失败 | 3 次发送；3 帧相互逐字节相同；清除 pending；下一调用不再发送 |
| fail,fail,success | 3 次尝试；3 帧相同；第 3 次成功清除 pending |
| S.s/y | 成功 1/0；用尽 2/0 |

### 3.2 汇总

| 项 | 结果 | 证据 |
|---|---:|---|
| `host_measure_flow_check`（含 [9]） | 72 passed / 0 failed | `measure_flow_item003.log` |
| `test/build_test.sh` 全量 5 组 | **226 passed / 0 failed** | `host_tests_full.log` |

### 3.3 构建

- `mdk_build build`/`rebuild` 0 Error；唯一告警为既有 `main.c(245) while(k--);`。
- Code=20160，RO=644，RW=116，ZI=1676。
- 未烧录、未复位、未连接 COM43；**不宣称实板通过**。

---

## 4. 未完成/交接

| 项 | 状态 |
|---|---|
| ITEM-004 UART 有界 | 未实现 |
| ITEM-005 主循环/启动/UID 零 | 未实现 |
| ITEM-006 标定配置 | 未实现 |
| 测试能力独立 harness | `host_measure_flow_verify_ev.c` 仍按旧跨轮/重编码模型，需测试能力重写；本实现未修改 |
| 实板/端到端 | 未执行 |

## 5. 结论

ITEM-003 的冻结帧、单周期最多 3 次发送、成功后/用尽清除、不跨周期、`S.s` 最终结果与 `S.y` 现行语义已实现，并通过实现侧 226 项宿主机自检。结论仅覆盖该 item；实板、RF/网关/App/云与产品验收未通过。
