# ProgrammedPrototypeArtifact（EV-011 / run8 ITEM-002）

本文件只证明「本轮实际把当前交付镜像下载到真实目标并回读校验成功」，**不表示任何业务功能合格**。本项的功能判定见 `evidence/test.md` EV-011（结论 **TEST_FAIL**）。

## 本次下载的固件身份

| 项 | 值 |
|---|---|
| 受测提交 | `8929438`（HEAD；空提交，源码与上一实现提交 `aa3bcc4` 逐字节相同；`git status` 干净，产品源码本轮零改动） |
| 构建方式 | `mdk_build rebuild`（Keil MDK / ARMCLANG **V6.24**，使用工程保存的配置，未另选芯片/调试器/算法） |
| 构建结果 | `0 Error(s), 1 Warning(s)`；唯一告警为基线既有的 `USER/src/main.c(224): warning: while loop has empty body` |
| 程序大小 | `Code=16572  RO-data=624  RW-data=76  ZI-data=1676`（与实现能力证据声明逐项一致） |
| 镜像路径 | `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/output/exe/Project.axf` |
| 镜像 md5 | `e309bac2a34fd210ee966d6da9001625` |

## 下载目标与下载校验

| 项 | 值 |
|---|---|
| 目标器件 | CW32L010（U7），传感器板；调试口 COM42 = Prolific USB-to-Serial（VID:PID `067B:2303`） |
| 下载探针 | CW-DAPLink `USB\VID_C251&PID_F001`（本机实测 SN `87094109484987710672FF50`，HID MI_02 + Ports MI_00），Status OK |
| 下载方式 | `mdk_flash {}`（先用工程保存的调试器/Flash 算法编译再下载） |
| 下载结果 | **3 次全部成功**，每次日志均为 `Erase Done.Programming Done.Verify OK.Application running ...` |

| # | 下载完成时刻（本地，UTC+8） | 日志关键行 |
|---|---|---|
| 1 | 2026-10-08 16:20:15 | `Erase Done.Programming Done.Verify OK.Application running ...` |
| 2 | 2026-10-08 16:43:40 | 同上 |
| 3 | 2026-10-08 16:45:41 | 同上 |

`Verify OK` 只证明**下载内容的回读校验一致**；下载成功后仍在真实目标上做了 COM42@9600 原始字节观测（见 `evidence/test.md` EV-011 §2），业务功能判定不取自本文件。

下载后目标的复位横幅（同一批采集内）：`BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600` 与 `rst=0240`（非零、随复位来源变化）。3 次成功下载亦再次证明：在目标已连续打印多条轨迹之后 SWD 仍可用。
