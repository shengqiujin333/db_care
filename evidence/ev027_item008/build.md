# Build Evidence — ITEM-008 最终构建与静态核对

状态：ITEM-008 最终构建/静态证据（**不表示实板功能通过**）
能力：`firmware_engineer.firmware_implementation`
项目：dengbei_run10
父基线：Git `90974f0`（ITEM-007 已提交；本 item 开始时工作区干净）
工程：`CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/Project.uvprojx`
编译器：Keil MDK ARMCLANG `V6.24`

## 1. 构建命令与结果

```text
mdk_build {"action":"rebuild"}   # 2026-10-21T04:52:33Z → 04:52:35Z
```

原始结果（`evidence/ev027_item008/build_rebuild.log`）：

```text
*** Using Compiler 'V6.24', folder: 'C:\Keil_v5\ARM\ARMCLANG\Bin'
Rebuild target 'Project'
...
../USER/src/main.c(252): warning: while loop has empty body [-Wempty-body]
  252 |         while(k--);
...
Program Size: Code=19544 RO-data=644 RW-data=116 ZI-data=1684
FromELF: creating hex file...
".\output\exe\Project.axf" - 0 Error(s), 1 Warning(s).
Build Time Elapsed:  00:00:01
```

- 0 Error；唯一告警是既有 `main.c(252) while(k--);` 空循环体。
- Flash 使用 Code+RO+RW = 20,304 B / 64 KB（约 31.0%）；RAM RW+ZI = 1,800 B / 4 KB（约 43.9%）。
- **未烧录**；构建成功不代表板上镜像或功能通过。

## 2. 产物身份

| 产物 | md5 |
|---|---|
| `MDK/output/exe/Project.axf` | `9cb97918e8832811dfcb00a93646fba1` |
| `MDK/output/exe/Project.hex` | `3af11e7adb21ba43004fc827649adb44` |

与 ITEM-006/007 相同（ITEM-007 仅测试资产、ITEM-008 未改产品源码）；完整产品源码 SHA-256 见 `source_sha256.txt`。

## 3. 本次实现范围（ITEM-001..008 汇总）

产品源码相对 `8ae4a33` 仅以下 6 个文件有改动：

```text
USER/inc/fw_core.h
USER/inc/sensor_config.h
USER/src/debug_trace.c
USER/src/fw_core.c
USER/src/main.c
USER/src/measure.c
```

无 `light.c`、`gxht40.c`、`sf_i2c.c`、`encrytogate.c`、`UM2005C/**` 或网关 `CH592EVT/.../beiwov2` 改动。

## 4. 限制

- 未烧录/复位/串口采集/实板验证；本文件不是真实目标通过证据。
- 全暗 C_dark（U02）仍未标定；真实 GXHT40 ACK（U01）仍依赖硬件/独立测试。
- 板上镜像身份需独立测试下载/核对；不能仅凭本文件声称板上固件已更新。
