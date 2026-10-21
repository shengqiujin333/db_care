# 固件实现记录（FWI-003，ITEM-001）

状态：`IMPLEMENTED`（仅 ITEM-001 门控纯逻辑实现与实现侧自测；**未烧录、未实板验证、未产品验收**）
能力：`firmware_engineer.firmware_implementation`
项目：dengbei_run10
父基线：Git `61e99b4`（本 item 开始时工作区干净；产品固件源码原为 `8ae4a33` 的旧门控语义）
实现范围：`artifacts/firmware_tasks_v3.yaml` 第 1 项
设计依据：`artifacts/firmware_solution_design.md`（FDS-003 DD-01/§5）、`artifacts/firmware_requirements_v3.md`（FWR-305/306）、`artifacts/interface_contract_v4.md`（IC-003 IF02）、PDR-001 PR-004、PACC-002 PA-008/009
构建证据：`evidence/ev019_item001/build.md`
自测证据：`evidence/ev019_item001/driver_test.md`

---

## 1. 本次改动

| 文件 | 改动 | 说明 |
|---|---|---|
| `CW32.../USER/inc/fw_core.h` | 新增 `sensor_gate_decide(...)`；保留 `sensor_decide_report(...)`；重写条件上报注释 | 产品路径/兼容入口分离；注释明确升温、严格边界、35.0 继续评估升温 |
| `CW32.../USER/src/fw_core.c` | 4 参函数改为 `(int32)cur-prev > SENSOR_REPORT_RISE_X10`；删除 `cur==350` 提前返回；新增包装：`!sample_valid → false`，否则以 `light_valid && dark` 调用 4 参函数 | 完整 PR-004/IC-003 判据；`int32` 防溢出 |
| `CW32.../USER/inc/sensor_config.h` | `SENSOR_REPORT_DROP_X10` → `SENSOR_REPORT_RISE_X10`；注释改为升温公式 | 单一配置点；无旧 `DROP` 常量参与运行路径 |
| `CW32.../USER/src/measure.c` | 产品调用改为 `sensor_gate_decide(true, s_prev_temp_x10, s_have_prev, tempvalue, s_light.valid, s_light.dark)`；相关注释同步 | 判定输入/行为不变，仅把 light_valid 显式传入包装；未实现后续快照/冻结任务 |
| `CW32.../USER/src/main.c` | 两处注释由 `sensor_decide_report` 改为 `sensor_gate_decide` | 无业务逻辑改动 |
| `test/host_sensor_core_test.c` | 门控段改为 14 向量 + 1,600 点网格 + 4 参兼容；删除下降期望 | 实现侧自检 |
| `test/host_fw_core_pure_check.c` | 门控段改为 14 向量 + 4 参兼容；补 `sensor_config.h`（`SENSOR_CONFIG_NO_MCU`）以编译 `LIGHT_DARK_*` 检查 | 实现侧自检；修复原文件缺少配置头导致的编译失败 |
| `test/host_measure_flow_check.c` | 原有下降温度序列改为升温序列，保持调用点/失败不推进/发送路径断言 | 让 gate 改动后的实现侧流程自检仍有效；未改产品逻辑 |

未修改：`gxht40.c`、`sf_i2c.c`、`light.c`、`encrytogate.c`、`UM2005C/**`、网关、Android、服务器、硬件、协议/密钥/引脚配置。

完整源码 SHA-256：`evidence/ev019_item001/source_sha256.txt`（8 个本次改动文件）。
构建产物 md5：axf `2e5e83120506d9e5c07fb2965f6ad3d6`，hex `83a127495f8e968d1f47ba24aeb5b674`。

---

## 2. 行为实现

### 2.1 产品路径

```c
bool sensor_gate_decide(bool sample_valid,
                        int16_t prev_x10, bool have_prev,
                        int16_t cur_x10,
                        bool light_valid, bool dark)
{
    if (!sample_valid) return false;
    return sensor_decide_report(prev_x10, have_prev, cur_x10,
                                (bool)(light_valid && dark));
}
```

- `sample_valid` 只表示温湿度有效；`light_valid && dark` 只作用于升温分支。
- 超温分支 `cur>350` 独立于前值/光照；恰好 350 继续评估升温。
- 升温严格 `cur-prev>9`（0.1℃ 量化下 ≥+1.0℃）；降温不触发。
- 4 参兼容函数保留旧签名，供既有 harness 编译；其 `dark` 参数须由调用方折算 `light_valid && dark`。

### 2.2 调用点

`measure.c` 成功分支：

```c
uint8_t rep = sensor_gate_decide(true, s_prev_temp_x10, s_have_prev,
                                 tempvalue, s_light.valid, s_light.dark) ? 1u : 0u;
```

未改变 `s_prev_temp_x10` 推进时机、采样顺序、失败处理、发送或 UART；这些属于后续 ITEM-002..008。

---

## 3. 验证结果

### 3.1 14 规范向量

`evidence/ev019_item001/gate_vectors.log`：14/14 通过；v3=1、v6=0、v7=1、v14=0。

### 3.2 扩展自检

| 项 | 结果 | 证据 |
|---|---:|---|
| `host_sensor_core_test`（含 1,600 点 gate 网格、4 参兼容、既有 CRC/换算/光照） | 53 passed / 0 failed | `gate_vectors.log` |
| `host_fw_core_pure_check` | 52 passed / 0 failed | `pure_check.log` |
| `host_measure_flow_check` | 43 passed / 0 failed | `measure_flow.log` |
| `test/build_test.sh` 全量 5 组 | 197 passed / 0 failed | `host_tests_full.log` |

### 3.3 构建

- `mdk_build build` 与 `rebuild` 均 0 Error；唯一告警为既有 `main.c(245) while(k--);` 空循环体。
- Code=20132，RO=644，RW=116，ZI=1676；相比旧构建 Code +32，RAM 不变。
- 未烧录、未复位、未连接目标；**不宣称实板通过或下载验证**。

---

## 4. 未完成/交接

| 项 | 状态 | 责任 |
|---|---|---|
| ITEM-002 判定前快照/S 行 p/H | 未实现；当前 measure 仍在判定后推进 prev，S 行 p 仍为推进后值 | 后续固件实现任务 |
| ITEM-003 冻结 10B 帧/单周期 3×200ms/不跨周期 | 未实现；当前仍每次发送重新编码、主循环多轮重试 | 后续固件实现任务 |
| ITEM-004 UART 有界等待 | 未实现 | 后续固件实现任务 |
| ITEM-005 主循环/启动/UID 零路径 | 未实现 | 后续固件实现任务 |
| ITEM-006 标定配置/编译期校验 | 未实现 | 后续固件实现任务 |
| ITEM-007 实现侧测试资产全面同步 | 部分完成：本 item 已同步 gate 相关自检；measure 流程温度序列已按升温修正 | 后续测试任务 |
| ITEM-008 构建/静态核对 | 本 item 已记录构建与 md5；完整静态清单/后续改动核对未做 | 后续任务 |
| 测试能力独立 harness | `host_fw_core_verify_ev.c`、`host_measure_flow_verify_ev.c`、`host_rf_report_verify_ev.c` 仍为旧降温/旧常量；`host_measure_flow_verify_ev.c` 引用已删除的 `SENSOR_REPORT_DROP_X10` 会编译失败 | 测试能力按 `test_design_v3.md` 重写，不由本实现修改 |
| U01 GXHT40 无 ACK | 未触及；仍为硬件/独立测试依赖 | 硬件/嵌入式测试 |
| 实板/端到端 | 未执行 | 独立测试/后续能力 |

---

## 5. 结论

ITEM-001 的源码实现与实现侧自测已完成：门控为升温且严格边界，恰好 35.0℃ 继续评估升温，`sample_valid`/`light_valid` 显式门禁，4 参旧签名保留，常量完成更名；当前构建 0 Error、197 项宿主机自检通过。该结论**仅覆盖门控纯逻辑**，不代表其他任务、实板采集、RF/网关/App/云或产品验收通过。
