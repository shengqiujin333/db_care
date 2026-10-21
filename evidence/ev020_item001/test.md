# 嵌入式验证证据（EV-020，ITEM-001 门控纯逻辑）

能力：`embedded_tester.embedded_verification`
被测项：`TL-6c897e7b-...-tasks-ITEM-001` — 将 `fw_core` 条件上报纯逻辑改为 PR-004/IC-003 完整判据
结论：**TEST_PASS（仅限本增量的宿主机功能验证）**
日期：2026-10-21（UTC 02:07–02:10 区间）

> 本结论覆盖 `sensor_gate_decide` / `sensor_decide_report` 的门控判据、调用点与常量更名。
> **不**代表实板门控方向性、温湿度采集、RF/网关/App/云链路或产品验收通过；**未烧录、未连接目标、未做串口/逻辑分析仪采集**。
> 原因见 §6：本增量在真实板上的门控分支不可区分（GXHT40 无 ACK 的 U01、全暗未标定的 U02 均未闭合），且 TD-003 将 ITEM-001 的判据归于 L0 纯逻辑（T-L0-05/06/07/08）。

---

## 1. 受测件身份

| 项 | 值 | 来源 |
|---|---|---|
| Git HEAD（本能力执行时） | `9360e396882caecb2c1845cd69670e3aed36d658` | `git rev-parse HEAD`；工作区干净 |
| 被测前基线 | `61e99b4`（上游固件实现提交 `9360e39` 的父） | `git log` |
| 被测源码 | `USER/src/fw_core.c`、`USER/inc/fw_core.h`、`USER/inc/sensor_config.h`、`USER/src/measure.c`、`USER/src/main.c` | SHA-256 见 `source_and_image_hash.txt` |
| 构建配置 | Keil MDK `MDK/Project.uvprojx`；ARMCLANG V6.24；`mdk_build rebuild` | `build_rebuild.log` |
| 镜像 | `Project.axf` md5 `2e5e83120506d9e5c07fb2965f6ad3d6`；`Project.hex` md5 `83a127495f8e968d1f47ba24aeb5b674` | `source_and_image_hash.txt` |
| 宿主机编译器 | `gcc.exe (x86_64-posix-seh-rev2, Built by MinGW-W64 project) 12.2.0` | 本能力实际调用 |
| 目标板 | **未使用/未烧录**（见 §6） | — |

镜像 md5 与实现能力记录（`artifacts/firmware_implementation_v3.md` §1）一致，说明当前工作树可确定性重建同一镜像。

---

## 2. 验证方法（独立性）

| 手段 | 说明 | 原始产物 |
|---|---|---|
| 测试能力自有 harness | `test/host_fw_core_gate_verify_item001.c`，直接编译**当前产品** `USER/src/fw_core.c` 与真实头文件；不使用实现侧 `host_sensor_core_test.c` 的期望 | `gate_verify_current.out` |
| 独立参考模型 | `verify_gate_item001.py` 按 IC-003 IF02 / FDS-003 DD-01 重新表述判据，不调用产品代码；逐点比对 harness 原始结果 | `verify_gate_item001.out` |
| 旧实现负向对照 | 从 Git `8ae4a33` 只读导出旧 `fw_core.c/fw_core.h/sensor_config.h`，符号改名后独立编译，对同一 14 向量运行 | `gate_verify_old.out`、`old_impl/` |
| 构建复核 | `mdk_build rebuild` 重建当前源码 | `build_rebuild.log` |
| 回归补充 | 既有宿主机套件 `test/build_test.sh`（实现侧资产，仅作回归参考，不作为本项判据的唯一来源） | `host_tests_current.log` |

复现命令（在工作树根目录）：

```sh
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output
gcc -std=c11 -Wall -Wextra -I USER/inc \
    test/host_fw_core_gate_verify_item001.c USER/src/fw_core.c \
    -o D:/mypro/beiwo2/AIP/evidence/ev020_item001/gate_verify_current.exe
D:/mypro/beiwo2/AIP/evidence/ev020_item001/gate_verify_current.exe \
    > D:/mypro/beiwo2/AIP/evidence/ev020_item001/gate_verify_current.out

cd D:/mypro/beiwo2/AIP/evidence/ev020_item001/old_impl
gcc -std=c11 -Wall -Wextra -Dsensor_decide_report=old_sensor_decide_report \
    -I . old_gate_negative_control.c fw_core.c -o ../gate_verify_old.exe
../gate_verify_old.exe > ../gate_verify_old.out

cd D:/mypro/beiwo2/AIP
python evidence/ev020_item001/verify_gate_item001.py \
    evidence/ev020_item001/gate_verify_current.out \
    evidence/ev020_item001/gate_verify_old.out
```

harness 编译 `-Wall -Wextra` 无告警；运行 stderr 为空；进程退出码 0。

---

## 3. 14 规范向量（逐条实测）

判定式：`report = sample_valid && (cur>350 || (have_prev && light_valid && dark && int32(cur)-int32(prev)>9))`

| # | sv | hp | prev | cur | lv | dark | 期望 | 实测 | 结论 |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| 1 | 1 | 1 | 300 | 308 | 1 | 1 | 0 | 0 | PASS |
| 2 | 1 | 1 | 300 | 309 | 1 | 1 | 0 | 0 | PASS |
| 3 | 1 | 1 | 300 | 310 | 1 | 1 | 1 | 1 | PASS（旧实现 0） |
| 4 | 1 | 1 | 300 | 310 | 1 | 0 | 0 | 0 | PASS |
| 5 | 1 | 1 | 300 | 310 | 0 | 0 | 0 | 0 | PASS |
| 6 | 1 | 1 | 310 | 300 | 1 | 1 | 0 | 0 | PASS（旧实现 1） |
| 7 | 1 | 1 | 340 | 350 | 1 | 1 | 1 | 1 | PASS（旧实现 0；恰好 35.0 继续升温分支） |
| 8 | 1 | 1 | 340 | 350 | 1 | 0 | 0 | 0 | PASS |
| 9 | 1 | 0 | 0 | 351 | 0 | 0 | 1 | 1 | PASS（超温独立） |
| 10 | 1 | 0 | 0 | 350 | 1 | 1 | 0 | 0 | PASS |
| 11 | 1 | 0 | 0 | 360 | 0 | 0 | 1 | 1 | PASS |
| 12 | 1 | 1 | 352 | 351 | 0 | 0 | 1 | 1 | PASS |
| 13 | 1 | 1 | 300 | 310 | 0 | 0 | 0 | 0 | PASS |
| 14 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | PASS（`sample_valid=false` 不判定） |

- 14/14 与 FDS-003 §5.2 / IC-003 IF02 完全一致。
- 旧实现负向对照的不符集合恰为 **v3、v6、v7**，旧输出 **0/1/0**：证明向量能区分新旧语义，不是“两边都全过”的无效测试。
- `v14` 通过 `sensor_gate_decide(..., sample_valid=false, ...)` 直接验证返回 false；未调用 4 参函数。

---

## 4. 扩展与边界

| 检查 | 结果 | 证据 |
|---|---|---|
| 结构化网格：`sv/hp/lv/dark ∈ {0,1}` × `prev,cur ∈ {-32768,-400,-1,0,9,340,349,350,351,1250,32767}` = **1,936 点** | 逐点与独立参考一致；`sample_valid=0` 恒 false | `verify_gate_item001.out` |
| 4 参兼容 `sensor_decide_report(prev,hp,cur,light_valid&&dark)` 与包装在 `sample_valid=1` 时等价 | 网格内全部一致，0 个不符 | 同上 |
| `int32` 差值边界 | `prev=-32768,cur=32767` → 触发；`prev=32767,cur=-32768` → 不触发；无回绕 | `gate_verify_current.out` B 行 |
| 升温严格边界 | 差 9 → 0；差 10 → 1 | 同上 |
| 35.0 边界 | `cur=350` 且有前值+暗+升温 10 → 1；`light_valid=0` 或 `dark=0` 或无前值 → 0；`cur=351` 无光/无前值 → 1 | 同上 |
| 常量 | `SENSOR_REPORT_RISE_X10=9`、`SENSOR_REPORT_HIGH_X10=350` | `C 9 350` 行 |
| harness 自检汇总 | `SELF 0 0 0` | `gate_verify_current.out` |

---

## 5. 构建、调用点与静态核对

| 检查 | 实测 | 结论 |
|---|---|---|
| `mdk_build rebuild` | `0 Error(s), 1 Warning(s)`；唯一告警为既有 `main.c(245) while(k--);` 空循环 | PASS（与实现记录一致） |
| 资源 | Code=20132，RO=644，RW=116，ZI=1676 | 与实现记录一致；本项无新增 RAM |
| 调用点 `measure.c:492` | `sensor_gate_decide(true, s_prev_temp_x10, s_have_prev, tempvalue, s_light.valid, s_light.dark)`；成功后推进 `s_prev_temp_x10`/`s_have_prev` | 产品路径仅经新包装；`light_valid` 被显式传入 |
| 头文件 | `fw_core.h` 声明 6 参 `sensor_gate_decide` 与保留的 4 参 `sensor_decide_report`；注释写明严格边界与 35.0 继续评估 | PASS |
| 常量更名 | 产品 `USER/**` 中 `SENSOR_REPORT_RISE_X10` 仅定义于 `sensor_config.h:124`；无 `SENSOR_REPORT_DROP_X10`；无 `cur==350` 提前返回；无 `prev-cur` 降温比较 | PASS |
| 其它产品调用者 | 除 `fw_core.c` 内部包装外，产品源码无 `sensor_decide_report` 调用 | PASS |
| 既有宿主机套件回归 | 5 组 53/43/44/28/29，合计 **197 passed / 0 failed** | 回归参考（实现侧资产，非本项独立判据） |

---

## 6. 未执行与局限（不改变本项结论，但限定其范围）

1. **未烧录、未复位、未做串口/逻辑分析仪采集**：本项是纯逻辑增量；TD-003 将 ITEM-001 的行为判据归于 L0（T-L0-05/06/07/08）。实板门控方向性（T-L5）前置条件未闭合：
   - U01：GXHT40 真实 ACK 未恢复（历史/本轮 `q=0`），无法产生有效温湿度样本；
   - U02：全暗 `C_dark` 未标定（`LIGHT_DARK_CALIBRATED=0`），`light.valid` 恒 0，升温分支在实板上恒关闭。
   因此即使烧录当前镜像，`S.r` 也无法区分新旧门控语义；本能力不以实板现象冒充本项通过，也不把未执行写成通过。
2. **测试资产陈旧**：`test/host_measure_flow_verify_ev.c` 仍引用已删除的 `SENSOR_REPORT_DROP_X10` 并按降温/跨轮发送构造期望（TD-003 D-07）。本项未使用该文件；其重写属后续 ITEM-007/测试能力任务，不构成本项产品缺陷。
3. **其它任务未实现**：判定前快照、冻结帧/单周期 3 次发送、UART 有界、主循环/启动、标定配置仍属 ITEM-002..006，未被本项验证；本结论不覆盖它们。
4. 本证据为宿主机层；不得写成实板、RF、网关、App、云或产品验收通过。

---

## 7. 结论

当前受测源码（HEAD `9360e39`，镜像 md5 `2e5e8312…`）的 `sensor_gate_decide` / `sensor_decide_report`：
- 14/14 规范向量正确，`v3=1`、`v6=0`、`v7=1`（旧实现为 `0/1/0`），`v14` 不判定；
- 1,936 点网格与独立参考一致，4 参兼容等价，`int32` 边界无回绕；
- 头文件、`measure.c` 调用点与 `SENSOR_REPORT_RISE_X10` 常量更名同步；
- Keil rebuild 0 Error，镜像可确定性重建。

**ITEM-001 本增量：TEST_PASS（宿主机功能验证范围）。**
