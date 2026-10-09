# ProgrammedPrototypeArtifact（EV-014 / run8 ITEM-002）

本文件只证明「本轮实际把当前交付镜像下载到真实目标并回读校验成功」，**不表示任何业务功能合格**。本项的功能判定见 `evidence/test.md` EV-014（结论 **BLOCKED**：用户重焊 U9 后实板仍有 0/11 周期 `q=1`，失败形态与重焊前逐字节相同；剩余路径只有现场物理动作）。

## 本次下载的固件身份

| 项 | 值 |
|---|---|
| 受测提交 | `58978b8`（HEAD；`git status` 干净；本轮**产品源码零改动**） |
| 受测源码版本 | 传感器固件源码自 `ef03c13`（上一轮实现交付提交）以来**零改动**：`git diff --stat ef03c13 HEAD -- <FW>/USER` 为空、`git log -1 -- <FW>/USER` = `ef03c13` |
| 构建方式 | `mdk_flash {}` 内含工程保存配置的编译（Keil MDK / ARMCLANG **V6.24**，未另选芯片/调试器/算法）。两次下载前 build 均为 `0 Error(s), 0 Warning(s)`（无可重建项 ⇒ 所下即下述 axf） |
| 程序大小 | `Code=18888  RO-data=644  RW-data=116  ZI-data=1676`（与实现能力 `evidence/build.md` 声明逐项一致，与 EV-013 受测件相同） |
| 镜像路径 | `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/output/exe/Project.axf` |
| 镜像 md5 | **`5c2fdb58d0fc4ef7dac8a21e343082a3`**（与本项实现交付声明、EV-013 受测件逐字符一致） |
| 附带 hex md5 | `db3dbc72195d346815500b8ef158b90f`（`Project.hex`） |

## 下载目标与下载校验

| 项 | 值 |
|---|---|
| 目标器件 | CW32L010（U7），传感器板（与 GXHT40 U9 同板）；调试口 COM42 = Prolific USB-to-Serial（VID:PID `067B:2303`） |
| 下载探针 | CW-DAPLink `USB\VID_C251&PID_F001`（本机实测 SN `87094109484987710672FF50`，HID MI_02 + Ports MI_00 = COM26），Status OK |
| 下载方式 | `mdk_flash {}`（用工程保存的调试器/Flash 算法先编译再下载） |
| 下载结果 | **2 次全部成功**，每次日志均为 `Erase Done.Programming Done.Verify OK.Application running ...` |

| # | 下载完成时刻（本地，UTC+8） | 日志关键行 | 该次下载后的采集窗口 |
|---|---|---|---|
| 1 | 2026-10-09 09:16:18 | `Erase Done.Programming Done.Verify OK.Application running ...` | `evidence/ev014/f1.cap`（01:16:03–01:21:03 UTC，含该复位横幅与 `BUS`/`IOSIG`） |
| 2 | 2026-10-09 09:35:25 | 同上 | `evidence/ev014/h1.cap`（01:35:11–01:40:11 UTC，含该复位横幅与 `BUS`/`IOSIG`），随后 `h2.cap`/`h3.cap` 无间隙续采 |

原始 flash/build 日志：`evidence/ev014/mdk_flash_logs.txt`；采集窗口清单：`evidence/ev014/capture_windows.txt`。

`Verify OK` 只证明**下载内容的回读校验一致**；下载成功后仍在真实目标上做了 COM42@9600 原始字节观测（`evidence/test.md` EV-014 §2），业务功能判定不取自本文件。第 2 次下载后的 `h1→h2→h3` 两个相邻窗口链把 `k=0,3,6,9,12,15` 接成**无间隙连续 15 分钟**观测。

下载后目标的复位横幅（同一批采集内）：`BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600` 与 `rst=0240`（非零、随复位来源变化），其后为上电自检与总线诊断行 `IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none`、`IOSIG scl=95555 sda=30303`、`BUS idle=3 scl=1 sda=1 ack=none`。2 次成功下载亦再次证明：在目标已连续打印多条轨迹之后 SWD 仍可用。

---

# 历史记录（EV-013 / run8 ITEM-002）

# ProgrammedPrototypeArtifact（EV-013 / run8 ITEM-002）

本文件只证明「本轮实际把当前交付镜像下载到真实目标并回读校验成功」，**不表示任何业务功能合格**。本项的功能判定见 `evidence/test.md` EV-013（结论 **BLOCKED**：实板 0/7 周期 `q=1`，剩余候选原因全在器件/接线侧，须用户现场动作）。

## 本次下载的固件身份

| 项 | 值 |
|---|---|
| 受测提交 | `ef03c13`（HEAD；`firmware_engineer.firmware_implementation` 本轮提交，新增上电 `IOSIG` 逐位回读签名行；接手时 `git status` 干净，产品源码本轮零改动） |
| 构建方式 | `mdk_build rebuild`（Keil MDK / ARMCLANG **V6.24**，使用工程保存的配置，未另选芯片/调试器/算法） |
| 构建结果 | `0 Error(s), 1 Warning(s)`；唯一告警为基线既有的 `USER/src/main.c(238): warning: while loop has empty body`（行号随新增代码位移；实现证据本轮也报 `238`，一致） |
| 程序大小 | `Code=18888  RO-data=644  RW-data=116  ZI-data=1676`（与实现能力 `evidence/build.md` 声明逐项一致） |
| 镜像路径 | `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/output/exe/Project.axf` |
| 镜像 md5 | **`5c2fdb58d0fc4ef7dac8a21e343082a3`**（与实现交付声明逐字符一致） |
| 附带 hex md5 | `db3dbc72195d346815500b8ef158b90f`（`Project.hex`） |

## 下载目标与下载校验

| 项 | 值 |
|---|---|
| 目标器件 | CW32L010（U7），传感器板（与 GXHT40 U9 同板）；调试口 COM42 = Prolific USB-to-Serial（VID:PID `067B:2303`） |
| 下载探针 | CW-DAPLink `USB\VID_C251&PID_F001`（本机实测 SN `87094109484987710672FF50`，HID MI_02 + Ports MI_00 = COM26），Status OK |
| 下载方式 | `mdk_flash {}`（用工程保存的调试器/Flash 算法先编译再下载） |
| 下载结果 | **2 次全部成功**，每次日志均为 `Erase Done.Programming Done.Verify OK.Application running ...` |

| # | 下载完成时刻（本地，UTC+8） | 日志关键行 | 该次下载后的采集窗口 |
|---|---|---|---|
| 1 | 2026-10-08 18:50:21 | `Erase Done.Programming Done.Verify OK.Application running ...` | `evidence/ev013/b1.cap`（10:50:08–10:55:08 UTC） |
| 2 | 2026-10-08 18:55:22 | 同上 | `evidence/ev013/b2.cap`（10:55:09–11:00:09 UTC） |

原始 build/flash 日志：`evidence/ev013/mdk_flash_logs.txt`；采集窗口清单：`evidence/ev013/capture_windows.txt`。

`Verify OK` 只证明**下载内容的回读校验一致**；下载成功后仍在真实目标上做了 COM42@9600 原始字节观测（`evidence/test.md` EV-013 §2），业务功能判定不取自本文件。本轮另做两次**不加复位**的续采（`c1.cap`/`c2.cap`），把下载复位之后的 `k=0,3,6,9,12` 五个周期接成**无间隙连续观测**。

下载后目标的复位横幅（同一批采集内）：`BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600` 与 `rst=0240`（非零、随复位来源变化，前者为窗口起点时的在位镜像所打印），其后为上电自检与总线诊断行 `IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none`、`IOSIG scl=95555 sda=30303`、`BUS idle=3 scl=1 sda=1 ack=none`。2 次成功下载亦再次证明：在目标已连续打印多条轨迹之后 SWD 仍可用。

---

# 历史记录（EV-012 / run8 ITEM-002）

# ProgrammedPrototypeArtifact（EV-012 / run8 ITEM-002）

本文件只证明「本轮实际把当前交付镜像下载到真实目标并回读校验成功」，**不表示任何业务功能合格**。本项的功能判定见 `evidence/test.md` EV-012（结论 **TEST_FAIL**：核心可观察 `q=1` 仍未达成）。

## 本次下载的固件身份

| 项 | 值 |
|---|---|
| 受测提交 | `09408c1`（HEAD；`firmware_engineer.firmware_implementation` 本轮提交，新增上电 `IOTEST` 自检；接手时 `git status` 干净，产品源码本轮零改动） |
| 构建方式 | `mdk_build rebuild`（Keil MDK / ARMCLANG **V6.24**，使用工程保存的配置，未另选芯片/调试器/算法） |
| 构建结果 | `0 Error(s), 1 Warning(s)`；唯一告警为基线既有的 `USER/src/main.c(231): warning: while loop has empty body`（上一轮该告警在 `main.c(224)`，行号随新增代码位移） |
| 程序大小 | `Code=17808  RO-data=636  RW-data=116  ZI-data=1676`（与实现能力 `evidence/build.md` 声明逐项一致） |
| 镜像路径 | `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/output/exe/Project.axf` |
| 镜像 md5 | **`bcfc14dc27ee9afeb02df2e0e7b4b586`**（与实现交付声明逐字符一致） |
| 附带 hex md5 | `7d32b34fe9d4ceddf7811eb44ee76484`（`Project.hex`） |

## 下载目标与下载校验

| 项 | 值 |
|---|---|
| 目标器件 | CW32L010（U7），传感器板（与 GXHT40 U9 同板）；调试口 COM42 = Prolific USB-to-Serial（VID:PID `067B:2303`） |
| 下载探针 | CW-DAPLink `USB\VID_C251&PID_F001`（本机实测 SN `87094109484987710672FF50`，HID MI_02 + Ports MI_00），Status OK |
| 下载方式 | `mdk_flash {}`（用工程保存的调试器/Flash 算法先编译再下载） |
| 下载结果 | **3 次全部成功**，每次日志均为 `Erase Done.Programming Done.Verify OK.Application running ...` |

| # | 下载完成时刻（本地，UTC+8） | 日志关键行 | 该次下载后的采集窗口 |
|---|---|---|---|
| 1 | 2026-10-08 18:09:39 | `Erase Done.Programming Done.Verify OK.Application running ...` | `evidence/ev012/b1.bin`（10:09:26–10:14:27 UTC） |
| 2 | 2026-10-08 18:17:16 | 同上 | `evidence/ev012/b2.bin`（10:17:03–10:22:03 UTC） |
| 3 | 2026-10-08 18:22:17 | 同上 | `evidence/ev012/b3.bin`（10:22:04–10:27:04 UTC） |

原始 build/flash 日志：`evidence/ev012/mdk_flash_logs.txt`。

`Verify OK` 只证明**下载内容的回读校验一致**；下载成功后仍在真实目标上做了 COM42@9600 原始字节观测（`evidence/test.md` EV-012 §2），业务功能判定不取自本文件。本轮还额外做了两次**不加复位**的续采（`c1.bin`/`c2.bin`），用于把 reset#3 之后的 `k=0,3,6,9,12,15` 六个周期接成**无间隙连续观测**。

下载后目标的复位横幅（同一批采集内）：`BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600` 与 `rst=0240`（非零、随复位来源变化），其后为 `IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none` 与 `BUS idle=3 scl=1 sda=1 ack=none`。3 次成功下载亦再次证明：在目标已连续打印多条轨迹之后 SWD 仍可用。
