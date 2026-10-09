# ProgrammedPrototypeArtifact（EV-017 / run8 ITEM-002）

本文件只证明「本轮实际把当前交付镜像下载到真实目标并回读校验成功」，**不表示任何业务功能合格**。本项的功能判定见 `evidence/test.md` EV-017（结论 **`TEST_FAIL`**：7/7 采样周期 `q=0`、器件对 112 个合法地址与 general call `0x00` 均不应答（`gc=3`），而**供电轨实测 ≈3.289 V 为正常量级**，失败形态逐字节稳定）。

## 本次下载的固件身份

| 项 | 值 |
|---|---|
| 受测提交 | `8ae4a33`（HEAD；`git status` 干净；本轮**产品源码零改动**） |
| 受测源码版本 | `git log -1 -- <FW>/USER` = `8ae4a33`；本轮交付增量 = `git diff --stat 55ed979 HEAD -- <FW>/USER` = **5 文件 +136/−0**（`light.c`/`light.h`：`light_read_vdd_mv()` 上电供电轨测量；`debug_trace.{h,c}`：`VDD` 行渲染；`main.c`：上电顺序 `BOOT→VDD→IOTEST→IOSIG→BUS`） |
| 构建方式 | `mdk_flash {}` 内含工程保存配置的编译（Keil MDK / ARMCLANG **V6.24**，未另选芯片/调试器/算法）。下载前 build 为 `0 Error(s), 0 Warning(s)`（无可重建项 ⇒ 所下即下述 axf） |
| 镜像路径 | `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/output/exe/Project.axf` |
| 镜像 md5 | **`5fc16235ccd043f4edb48b16ee44da0b`**（与实现交付声明逐字符一致；下载前、下载后、以及本能力**独立 `mdk_build rebuild`** 后三处相同 ⇒ 可由当前工作树确定性重建） |
| 附带 hex md5 | `9a32cf3bcdbca2c1346da9c3bb3581da`（`Project.hex`） |
| 独立重建 | `mdk_build {"action":"rebuild"}`（本能力执行，本地 13:52:58→13:53:00）：`0 Error(s), 1 Warning(s)`（既有模板 `main.c(245)` 空循环体告警）；`Program Size: Code=20100 RO-data=644 RW-data=116 ZI-data=1676`（RAM 1,792 B ≤ 4 KB、Flash 20,744 B ≤ 64 KB）；axf/hex md5 **不变** |

## 下载目标与下载校验

| 项 | 值 |
|---|---|
| 目标器件 | CW32L010（U7），传感器板（与 GXHT40 U9 同板），板上 MCU 唯一编号 `uid=6A002C00`（与 EV-012..EV-016 同一块板） |
| 调试口 | COM42 = Prolific USB-to-Serial（VID:PID `067B:2303`），9600 8N1 |
| 下载探针 | CW-DAPLink `USB VID:PID=C251:F001`（SN `87094109484987710672FF50`，=`COM26` 同源），在位 |
| 下载方式 | `mdk_flash {}`（用工程保存的调试器/Flash 算法先编译再下载） |
| 下载结果 | **1 次成功**：`Erase Done.Programming Done.Verify OK.Application running ...`（本地 2026-10-09 13:29:25 = UTC 05:29:25） |

| # | 下载完成时刻（本地，UTC+8） | 日志关键行 | 该次下载后的采集窗口 |
|---|---|---|---|
| 1 | 2026-10-09 13:29:25 | `Erase Done.Programming Done.Verify OK.Application running ...` | `evidence/ev017/r1.cap`（05:29:11–05:34:11 UTC，含下载复位 `rst=0240` 的横幅与 `VDD ok=1 code=1480 bgrmv=1189 mv=3289` / `IOTEST…gc=3` / `IOSIG` / `BUS`），随后 `s1.cap`/`s2.cap`/`s3.cap` 无间隙续采 |

原始 build/flash 日志：`evidence/ev017/mdk_flash_logs.txt`；镜像身份（三处 md5 + 两次构建日志）：`evidence/ev017/image_identity.txt`；采集窗口清单：`evidence/ev017/capture_windows.txt`。

`Verify OK` 只证明**下载内容的回读校验一致**；下载成功后仍在真实目标上做了 COM42@9600 原始字节观测（`evidence/test.md` EV-017 §2），业务功能判定不取自本文件。`r1→s1→s2→s3` 四个窗口首尾相接（间隙 0.43/0.40/0.41 s），把 `k=0,3,6,9,12,15,18` 接成**无间隙连续 ≈21 分钟**观测。

**下载生效的实板自证**：窗口起点那一段（下载前镜像 = rev 5.5）的上电序列为 `BOOT→IOTEST→IOSIG→…`，**没有 `VDD` 行**；下载复位后的段为 `BOOT→VDD→IOTEST→IOSIG→BUS`，含 `VDD` 行（`VDD` 为 rev 5.6 新增，rev 5.5 恒不打印）⇒ 板上镜像确实由 rev 5.5 换成了本次所下的 rev 5.6。本次成功下载亦再次证明：在目标已连续打印多条轨迹之后 SWD 仍可用。

---

# 历史记录（EV-016 / run8 ITEM-002；当轮下载 1 次 `Verify OK`，axf md5 `ff490296…`（rev 5.5）；已提交 e769e56）

# ProgrammedPrototypeArtifact（EV-016 / run8 ITEM-002）

本文件只证明「本轮实际把当前交付镜像下载到真实目标并回读校验成功」，**不表示任何业务功能合格**。本项的功能判定见 `evidence/test.md` EV-016（结论 **`TEST_FAIL`**：7/7 采样周期 `q=0`、器件对 112 个合法地址与 general call `0x00` 均不应答（`gc=3`），失败形态逐字节稳定）。

## 本次下载的固件身份

| 项 | 值 |
|---|---|
| 受测提交 | `55ed979`（HEAD；`git status` 干净；本轮**产品源码零改动**） |
| 受测源码版本 | `git log -1 -- <FW>/USER` = `55ed979`；本轮交付增量 = `git diff --stat 5b07982 HEAD -- <FW>/USER` = **4 文件 +35/−3**（`sensor_config.h`/`measure.c`/`debug_trace.{h,c}`：上电一次性 general call 复位尝试 + `IOTEST` 尾字段 `gc`） |
| 构建方式 | `mdk_flash {}` 内含工程保存配置的编译（Keil MDK / ARMCLANG **V6.24**，未另选芯片/调试器/算法）。下载前 build 为 `0 Error(s), 0 Warning(s)`（无可重建项 ⇒ 所下即下述 axf） |
| 镜像路径 | `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/output/exe/Project.axf` |
| 镜像 md5 | **`ff49029608f3315ee76964c131d06821`**（与实现交付声明逐字符一致） |
| 附带 hex md5 | `d18c0d623812f96351d7b8ebb3d9d3a9`（`Project.hex`） |

## 下载目标与下载校验

| 项 | 值 |
|---|---|
| 目标器件 | CW32L010（U7），传感器板（与 GXHT40 U9 同板），板上 MCU 唯一编号 `uid=6A002C00`（与 EV-012..EV-015 同一块板） |
| 调试口 | COM42 = Prolific USB-to-Serial（VID:PID `067B:2303`），9600 8N1 |
| 下载探针 | CW-DAPLink `USB VID:PID=C251:F001`（SN `87094109484987710672FF50`，=`COM26` 同源），在位 |
| 下载方式 | `mdk_flash {}`（用工程保存的调试器/Flash 算法先编译再下载） |
| 下载结果 | **1 次成功**：`Erase Done.Programming Done.Verify OK.Application running ...`（本地 2026-10-09 12:32:51 = UTC 04:32:51） |

| # | 下载完成时刻（本地，UTC+8） | 日志关键行 | 该次下载后的采集窗口 |
|---|---|---|---|
| 1 | 2026-10-09 12:32:51 | `Erase Done.Programming Done.Verify OK.Application running ...` | `evidence/ev016/r1.cap`（04:32:37–04:37:37 UTC，含下载复位 `rst=0240` 的横幅与 `IOTEST…gc=3`/`IOSIG`/`BUS`），随后 `s1.cap`/`s2.cap`/`s3.cap` 无间隙续采 |

原始 build/flash 日志：`evidence/ev016/mdk_flash_logs.txt`；采集窗口清单：`evidence/ev016/capture_windows.txt`。

`Verify OK` 只证明**下载内容的回读校验一致**；下载成功后仍在真实目标上做了 COM42@9600 原始字节观测（`evidence/test.md` EV-016 §2），业务功能判定不取自本文件。`r1→s1→s2→s3` 四个窗口首尾相接（间隙 0.41/0.43/0.42 s），把 `k=0,3,6,9,12,15,18` 接成**无间隙连续 ≈21 分钟**观测。

**下载生效的实板自证**：窗口起点那一段（下载前镜像）的 `IOTEST` 只有 4 个字段，下载复位后的 `IOTEST` 为 5 个字段且带 `gc=3`（`gc` 为 rev 5.5 新增，rev 5.4 恒不打印）⇒ 板上镜像确实由 rev 5.4 换成了本次所下的 rev 5.5。本次成功下载亦再次证明：在目标已连续打印多条轨迹之后 SWD 仍可用。

---

# 历史记录（EV-015 / run8 ITEM-002；当轮下载 1 次 `Verify OK`，axf md5 `5c2fdb58…`（rev 5.4）；已提交 5b07982）

# ProgrammedPrototypeArtifact（EV-015 / run8 ITEM-002）

本文件只证明「本轮实际把当前交付镜像下载到真实目标并回读校验成功」，**不表示任何业务功能合格**。本项的功能判定见 `evidence/test.md` EV-015（结论 **`TEST_FAIL`**：5/5 采样周期 `q=0`、器件地址级不应答，失败形态逐字节稳定）。

## 本次下载的固件身份

| 项 | 值 |
|---|---|
| 受测提交 | `59ca659`（HEAD；`git status` 干净；本轮**产品源码零改动**） |
| 受测源码版本 | 传感器固件源码自 `ef03c13`（实现交付提交）以来**零改动**：`git diff --stat ef03c13 HEAD -- <FW>/USER` 为空、`git log -1 -- <FW>/USER` = `ef03c13` |
| 构建方式 | `mdk_flash {}` 内含工程保存配置的编译（Keil MDK / ARMCLANG **V6.24**，未另选芯片/调试器/算法）。下载前 build 为 `0 Error(s), 0 Warning(s)`（无可重建项 ⇒ 所下即下述 axf） |
| 镜像路径 | `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/output/exe/Project.axf` |
| 镜像 md5 | **`5c2fdb58d0fc4ef7dac8a21e343082a3`**（与实现交付声明、EV-013/EV-014 受测件逐字符一致） |
| 附带 hex md5 | `db3dbc72195d346815500b8ef158b90f`（`Project.hex`） |

## 下载目标与下载校验

| 项 | 值 |
|---|---|
| 目标器件 | CW32L010（U7），传感器板（与 GXHT40 U9 同板）；调试口 COM42 = Prolific USB-to-Serial（VID:PID `067B:2303`） |
| 下载探针 | CW-DAPLink `USB VID:PID=C251:F001`（SN `87094109484987710672FF50`，=`COM26` 同源），在位 |
| 下载方式 | `mdk_flash {}`（用工程保存的调试器/Flash 算法先编译再下载） |
| 下载结果 | **1 次成功**：`Erase Done.Programming Done.Verify OK.Application running ...`（本地 2026-10-09 11:47:24） |

| # | 下载完成时刻（本地，UTC+8） | 日志关键行 | 该次下载后的采集窗口 |
|---|---|---|---|
| 1 | 2026-10-09 11:47:24 | `Erase Done.Programming Done.Verify OK.Application running ...` | `evidence/ev015/r1.cap`（03:47:10–03:52:10 UTC，含该复位横幅与 `IOSIG`/`BUS`），随后 `s1.cap`/`s2.cap` 无间隙续采 |

原始 build/flash 日志：`evidence/ev015/mdk_flash_logs.txt`；采集窗口清单：`evidence/ev015/capture_windows.txt`。

`Verify OK` 只证明**下载内容的回读校验一致**；下载成功后仍在真实目标上做了 COM42@9600 原始字节观测（`evidence/test.md` EV-015 §2），业务功能判定不取自本文件。`r1→s1→s2` 三个窗口首尾相接（间隙 0.42 s / 0.46 s），把 `k=0,3,6,9,12` 接成**无间隙连续 ≈13 分钟**观测。

下载后目标的复位横幅（同一批采集内）：`BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600` 与 `rst=0240`（非零、随复位来源变化），其后为上电自检与总线诊断行 `IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none`、`IOSIG scl=95555 sda=30303`、`BUS idle=3 scl=1 sda=1 ack=none`。本次成功下载亦再次证明：在目标已连续打印多条轨迹之后 SWD 仍可用。

---

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
