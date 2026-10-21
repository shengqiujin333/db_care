# Build Evidence — ITEM-002 判定前快照

状态：ITEM-002 实现侧编译证据（**不表示实板功能通过**）
能力：`firmware_engineer.firmware_implementation`
项目：dengbei_run10
父基线：Git `17a92ba`（ITEM-001 已提交；本 item 开始工作区干净）
工程：`CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/Project.uvprojx`
编译器：Keil MDK ARMCLANG `V6.24`

## 1. 构建命令与结果

```text
mdk_build {"action":"build"}     # 2026-10-21T02:12:56Z → 02:12:58Z
mdk_build {"action":"rebuild"}   # 2026-10-21T02:13:09Z → 02:13:11Z
```

Rebuild 原始结果（完整日志：`evidence/ev021_item002/build_rebuild.log`）：

```text
*** Using Compiler 'V6.24', folder: 'C:\Keil_v5\ARM\ARMCLANG\Bin'
Rebuild target 'Project'
...
compiling measure.c...
...
Program Size: Code=20168 RO-data=644 RW-data=116 ZI-data=1676
FromELF: creating hex file...
".\output\exe\Project.axf" - 0 Error(s), 1 Warning(s).
Build Time Elapsed:  00:00:01
```

唯一告警仍为既有模板 `main.c(245) while(k--);` 空循环体，非本次改动引入。

## 2. 产物身份

| 产物 | md5 |
|---|---|
| `MDK/output/exe/Project.axf` | `e8c2ebcdb927930a3366761e4f2999c9` |
| `MDK/output/exe/Project.hex` | `badfd8142dbf316390e258a8af389486` |

相对 ITEM-001 的 `2e5e8312…` / `83a12749…` 变化来自 `measure.c` 的判定前快照与轨迹取值；**未烧录**，不代表板上镜像已更新。

## 3. 本次参与编译的源码改动

| 文件 | 改动 |
|---|---|
| `USER/src/measure.c` | 每个采样机会先快照 `prev_for_decision/have_prev_for_decision`；判定使用快照；判定后才推进 `prev/have_prev/last_hum`；失败不推进；轨迹保存并使用该快照 |
| `USER/src/measure.c` | `sensor_trace_fetch` 的 `p/H` 改为读取轨迹快照，不再读取推进后的 `s_prev_temp_x10` |
| `test/host_measure_flow_check.c` | 新增 [14] A→B→失败→C 的 p/H 检查 |

未修改门控纯逻辑、发送、UART、启动、标定或协议。

## 4. 限制

- 未烧录/复位/串口采集/实板验证；本文件不是真实目标通过证据。
- 本 item 只完成判定前快照与 p/H 轨迹；ITEM-003 冻结发送、ITEM-004 UART 有界等仍未实现。
- 板上镜像身份需独立测试下载/核对；以本文件 md5 与 Git 提交为准。
