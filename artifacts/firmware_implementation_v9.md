# 固件实现记录（FWI-009，ITEM-007）

状态：`IMPLEMENTED`（仅 ITEM-007 实现侧自检资产同步与执行；**产品源码未改、未烧录、未实板验证、未产品验收**）
能力：`firmware_engineer.firmware_implementation`
项目：dengbei_run10
父基线：Git `a015804`（ITEM-006 已提交；本 item 开始时工作区干净）
实现范围：`artifacts/firmware_tasks_v3.yaml` 第 7 项
设计依据：FDS-003 §14、FWR-314、`test_design_v3.md` D-07/T-L0-05..20
构建证据：`evidence/ev026_item007/build.md`
自测证据：`evidence/ev026_item007/driver_test.md`

---

## 1. 本次改动

| 文件 | 改动 |
|---|---|
| `test/build_test.sh` | 纳入 `host_fw_core_pure_check` 与 `check_calib_config.sh`；分组编号更新为 5 段 |
| `test/host_sensor_core_test.c` | 14 向量/网格之外新增 as-built 旧方向负向对照（旧实现必须失败 v3/v6/v7） |

未修改产品源码（`USER/**`）、Keil 工程、协议、引脚、硬件、网关。未修改测试能力拥有的 `*_verify_ev.c`。

全部实现侧自检资产当前状态：
- `host_sensor_core_test.c`：14 向量、35.0、400..4095 C_dark 网格、4 参兼容、旧方向负向对照；
- `host_fw_core_pure_check.c`：14 向量、4 参兼容、C_dark 全量边界；
- `host_measure_flow_check.c`：升温门控、判定前 p/H、失败不推进、冻结帧/3 次/不跨周期/S.s/y；
- `host_debug_trace_check.c`：BOOT 构建标识、行长/字段顺序、TXE/TXBUSY 有界；
- `host_light_check.c`：未标定 valid/dark=false、标定 3000 的 1000/999 边界；
- `check_calib_config.sh`：LIGHT_DARK_* 编译门禁。

完整源码/测试 SHA-256：`evidence/ev026_item007/source_sha256.txt`。
产品镜像 md5（与 ITEM-006 相同，未重编）：axf `9cb97918e8832811dfcb00a93646fba1`，hex `3af11e7adb21ba43004fc827649adb44`。

---

## 2. 验证

`test/build_test.sh` 单一入口 **7 组 295 passed / 0 failed**（`host_tests_full.log`）：

| 组 | 结果 |
|---|---:|
| host_sensor_core（14 向量/网格/负向） | 57 / 0 |
| host_fw_core_pure_check | 52 / 0 |
| host_measure_flow（p/H/冻结/3 次） | 72 / 0 |
| host_debug_trace（含 UART 卡死） | 51 / 0 |
| light 未标定 | 28 / 0 |
| light 标定 C_dark=3000 | 29 / 0 |
| 标定配置编译门禁 | 6 / 0 |

- 旧方向 replica 在 13 个可评估向量中恰好失败 v3/v6/v7，证明 14 向量不是恒过空验。
- 非法标定组合（2/0/4096）编译失败；默认/1/4095 通过。
- UART TXE/TXBUSY 卡死在预算内返回且仍关闭/回收引脚。
- 产品构建无源码变更，`mdk_build build` 0 Error/0 Warning，镜像 md5 与 ITEM-006 相同。
- 未烧录、未复位、未连接 COM43；不宣称实板通过。

---

## 3. 未完成/交接

| 项 | 状态 |
|---|---|
| 测试能力 `host_diag_line_verify_ev.c` | 仍硬编码旧 `BOOT fw=FD-002r4`，需测试能力改用 `SENSOR_FW_BUILD_ID`；未修改 |
| 测试能力 `host_measure_flow_verify_ev.c` | 仍为旧降温/跨轮模型，需按 `test_design_v3.md` 重写；未修改 |
| 实板 / 独立验证 | 未执行 |
| 全暗 C_dark 标定（U02） | 未完成；配置仍为未标定 |

## 4. 结论

ITEM-007 的实现侧自检资产已同步并通过单一入口 295/0：14 向量/升温、35.0、p/H、冻结帧/3 次、UART 有界、未标定/标定边界、编译门禁均有覆盖，且旧方向实现会被判失败。结论仅覆盖实现侧自检；实板、独立验证与产品验收未通过。
