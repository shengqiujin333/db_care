# Build Evidence — ITEM-007 实现侧自检资产同步

状态：ITEM-007 实现侧编译证据（**不表示实板功能通过**）
能力：`firmware_engineer.firmware_implementation`
项目：dengbei_run10
父基线：Git `a015804`（ITEM-006 已提交；本 item 开始时工作区干净）
工程：`CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/Project.uvprojx`
编译器：Keil MDK ARMCLANG `V6.24`

## 1. 本次改动性质

ITEM-007 只同步实现侧自检资产，**未修改任何产品源码**：
- `test/build_test.sh`：把 `host_fw_core_pure_check` 与 `check_calib_config.sh` 纳入单一入口；更新分组编号。
- `test/host_sensor_core_test.c`：14 向量套件增加 as-built 旧方向负向对照（旧实现必须失败 v3/v6/v7）。

产品源码、Keil 工程、协议、引脚未改，故构建运行不重新编译、镜像字节与 ITEM-006 相同。

## 2. 构建命令与结果

```text
mdk_build {"action":"build"}   # 2026-10-21T04:39:36Z → 04:39:37Z
```

原始结果（`evidence/ev026_item007/build_incremental.log`）：

```text
*** Using Compiler 'V6.24', folder: 'C:\Keil_v5\ARM\ARMCLANG\Bin'
Build target 'Project'
".\output\exe\Project.axf" - 0 Error(s), 0 Warning(s).
Build Time Elapsed:  00:00:00
```

无源码变更 → 0 Error/0 Warning、无可重建项。

## 3. 产物身份（与 ITEM-006 相同）

| 产物 | md5 |
|---|---|
| `MDK/output/exe/Project.axf` | `9cb97918e8832811dfcb00a93646fba1` |
| `MDK/output/exe/Project.hex` | `3af11e7adb21ba43004fc827649adb44` |

**未烧录**；本文件不作为板上镜像更新或实板通过证据。

## 4. 限制

- 测试资产改动不改变产品行为或镜像；不能用本 item 的通过替代实板/独立验收。
- 测试能力拥有的 `*_verify_ev.c` 未修改；其中 `host_diag_line_verify_ev.c`（旧 BOOT 标识）与 `host_measure_flow_verify_ev.c`（旧降温/跨轮模型）仍待测试能力按新语义重写。
- 板上镜像身份需独立测试下载/核对。
