# Build Evidence — ITEM-001 gate logic

状态：IMPLEMENTED 的编译证据（**不表示实板功能通过**；本能力未烧录、未复位、未连接目标）
能力：`firmware_engineer.firmware_implementation`
项目：dengbei_run10
基线：Git `61e99b4`（开始本 item 时工作区干净；产品源码基线 `8ae4a33` 的旧门控语义）
项目工程：`CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/Project.uvprojx`
编译器：Keil MDK ARMCLANG `V6.24`（`C:\Keil_v5\ARM\ARMCLANG\Bin`）

## 1. 构建命令与结果

经受审计 `mdk_build` 入口执行：

```text
mdk_build {"action":"build"}     # 2026-10-21T01:52:52Z → 01:52:54Z
mdk_build {"action":"rebuild"}   # 2026-10-21T01:53:27Z → 01:53:29Z
```

Rebuild 原始结果（完整日志：`evidence/ev019_item001/build_rebuild.log`）：

```text
*** Using Compiler 'V6.24', folder: 'C:\Keil_v5\ARM\ARMCLANG\Bin'
Rebuild target 'Project'
...
compiling fw_core.c...
compiling measure.c...
...
Program Size: Code=20132 RO-data=644 RW-data=116 ZI-data=1676
FromELF: creating hex file...
".\output\exe\Project.axf" - 0 Error(s), 1 Warning(s).
Build Time Elapsed:  00:00:01
```

唯一告警为既有模板 `main.c(245) while(k--);` 空循环体（`-Wempty-body`），非本次改动引入。

## 2. 产物身份

| 产物 | md5 |
|---|---|
| `MDK/output/exe/Project.axf` | `2e5e83120506d9e5c07fb2965f6ad3d6` |
| `MDK/output/exe/Project.hex` | `83a127495f8e968d1f47ba24aeb5b674` |

相对上一实现（`5fc16235…` / `9a32cf3b…`）变化来自本次 `fw_core` 门控与调用点/测试改动；**未执行烧录**，板上镜像身份不由本证据证明。

## 3. 本次参与编译的源码改动

| 文件 | 改动 |
|---|---|
| `USER/inc/fw_core.h` | 新增 `sensor_gate_decide` 声明；4 参 `sensor_decide_report` 保留；注释改为升温/完整判据 |
| `USER/src/fw_core.c` | 升温 `int32 cur-prev>9`；删除 `cur==350` 提前返回；新增包装 |
| `USER/inc/sensor_config.h` | `SENSOR_REPORT_DROP_X10` → `SENSOR_REPORT_RISE_X10`，注释同步 |
| `USER/src/measure.c` | 产品调用改为 `sensor_gate_decide(true, ..., s_light.valid, s_light.dark)`；注释同步 |
| `USER/src/main.c` | 仅注释同步（无业务逻辑改动） |

## 4. 限制

- 未烧录/复位/串口采集/实板验证；`Verify OK` 不存在，本文件不能作为真实目标通过证据。
- 未实现 ITEM-002..008 的判定前快照、冻结发送、UART 有界、主循环/标定等；当前构建仍是 as-built 的 measure 行为，只是门控函数/调用点已修正。
- 镜像身份以本文件 md5 和 Git 提交为准；板上运行镜像是否已更新需独立测试下载/核对。
