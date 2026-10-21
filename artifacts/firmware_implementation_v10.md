# 固件实现记录（FWI-010，ITEM-008 / 本轮最终实现交付）

状态：`IMPLEMENTED`（ITEM-001..008 已实现并完成实现侧构建/静态核对；**未烧录、未实板验证、未产品验收**）
能力：`firmware_engineer.firmware_implementation`
项目：dengbei_run10
父基线：Git `90974f0`（ITEM-007 已提交；本 item 开始时工作区干净）
实现范围：`artifacts/firmware_tasks_v3.yaml` 第 8 项（并汇总第 1–7 项）
构建证据：`evidence/ev027_item008/build.md`
自测/静态证据：`evidence/ev027_item008/driver_test.md`

---

## 1. 最终源码身份

相对原始固件 `8ae4a33`，本轮产品源码仅改动 6 个文件：

```text
USER/inc/fw_core.h
USER/inc/sensor_config.h
USER/src/debug_trace.c
USER/src/fw_core.c
USER/src/main.c
USER/src/measure.c
```

- 无 `light.c`、`gxht40.c`、`sf_i2c.c`、`encrytogate.c`、`UM2005C/**` 或网关 `CH592EVT/.../beiwov2` 改动。
- 完整 SHA-256：`evidence/ev027_item008/source_sha256.txt`。
- 构建：Keil ARMCLANG V6.24，`rebuild` **0 Error / 1 既有 warning**（`main.c(252) while(k--);`）。
- axf md5 `9cb97918e8832811dfcb00a93646fba1`，hex `3af11e7adb21ba43004fc827649adb44`。
- Code=19544，RO=644，RW=116，ZI=1684；Flash 20,304 B / 64 KB（31.0%），RAM 1,800 B / 4 KB（43.9%）。

## 2. 已实现增量（ITEM-001..007）

| 增量 | 实现结果 |
|---|---|
| ITEM-001 | `sensor_gate_decide` 完整升温判据；4 参兼容；删除 35.0 短路；`SENSOR_REPORT_RISE_X10` |
| ITEM-002 | 判定前 `prev/have_prev` 快照；判定后推进；失败不推进；S.p/H 为快照 |
| ITEM-003 | 触发时冻结 10B 帧；单周期 1..3 次发送同一字节缓冲；成功/用尽清除；不跨周期 |
| ITEM-004 | UART 整行/关闭有界等待；超时中止本行但照常关闭/回收 PA05/PA06；无新定时器 |
| ITEM-005 | 每机会一次完整事务；BOOT 在 UID 零判断前；UID 零清标志/关 UART/真正深睡 |
| ITEM-006 | `LIGHT_DARK_*` 编译门禁；未标定 valid/dark=false；`SENSOR_FW_BUILD_ID` BOOT |
| ITEM-007 | 实现侧自检资产同步；`build_test.sh` 7 组；旧方向负向对照 |

## 3. 最终验证

### 3.1 构建

- `mdk_build rebuild` 0 Error；唯一告警为既有延时空循环。
- 原始日志：`evidence/ev027_item008/build_rebuild.log`。
- 未烧录；构建成功不代表板上镜像或功能通过。

### 3.2 静态核对（12/12 PASS）

`evidence/ev027_item008/static_audit.py` / `static_audit.txt`：
- 无 hall/OPTCFG/params/history 有效调用；
- 无 NVM/Flash 参数写；
- `sample_flag=1` 仅初值/RTC；temperature/send 各仅主循环 1 处；
- USER 无 GPIOA 复位/关时钟；
- 新增代码行无 GPIO/NVIC/RTC 配置；
- 网关、RF/encrytogate/UM2005C、light/gxht40/sf_i2c 无本轮改动；
- S/G 最坏 67/46 B ≤96 B。

### 3.3 实现侧自检

`test/build_test.sh` 7 组 **295 passed / 0 failed**：
57（14 向量/网格/旧方向负向）+ 52（pure check）+ 72（p/H/冻结/3 次）+ 51（UART/BOOT/卡死）+ 28/29（light 未标定/标定）+ 6（标定编译门禁）。

## 4. 未完成/交接

| 项 | 状态 |
|---|---|
| U01 GXHT40 真实 ACK/连续采集 | 未闭合；依赖硬件/嵌入式测试 |
| U02 全暗 C_dark 实测回填 | 未完成；当前 `LIGHT_DARK_REF_CODE=0/CALIBRATED=0` |
| 真实目标烧录/启动/S 行/UID 零/节拍/深睡 | 未执行；需独立测试 |
| RF→网关→BLE→App→云 | 未验证；由对应测试/运维能力执行 |
| 测试能力旧独立 harness | `host_diag_line_verify_ev.c` 旧 BOOT、`host_measure_flow_verify_ev.c` 旧模型待测试能力更新 |
| 操作部署手册/真实总功能测试 | 后续能力负责 |

## 5. 结论

本轮固件实现（ITEM-001..008）已完成：源码实现、构建 0 Error、静态核对 12/12、实现侧自检 295/0，镜像身份已记录。该结论**仅覆盖实现与实现侧自检**；未烧录、未实板/独立验证，U01/U02 与端到端/产品验收未通过，不得据此宣告交付。
