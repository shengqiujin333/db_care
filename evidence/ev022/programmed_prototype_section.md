# ProgrammedPrototypeArtifact（EV-022 / 修复轮复验：时序 A/B 镜像）

本文件只证明「本轮实际把当前交付镜像下载到真实目标并回读校验成功」，**不表示任何业务功能合格**。功能判定见 `evidence/test.md` EV-022（结论 **`TEST_FAIL`**：修复镜像 19 个采样周期中 13 次 `q=0`（68.4%），全部为 `G s=4` 温度 CRC 错、湿度 CRC 全对；两项时序 A/B 未改善失败率；T5 全暗标定与 INT 上报未执行）。

## 本次下载的固件身份

| 项 | 值 |
|---|---|
| 受测提交 | `0f8a8fba`（HEAD，`[role:firmware_engineer]`；接手时工作区干净；本轮**产品源码零改动**） |
| 受测源码增量 | `git diff --stat 7bfd248 HEAD -- <FW>/USER` = **2 文件 +9/−3**：`USER/src/sf_i2c.c`（读字节 SDA 采样点移到 SCL 高电平保持段末尾）、`USER/inc/sensor_config.h`（`GXHT40_MEASURE_WAIT_MS` 10→20 ms） |
| 构建方式 | `mdk_build {"action":"rebuild"}`（Keil MDK / ARMCLANG **V6.24**，工程保存配置，未另选芯片/调试器/算法）+ `mdk_flash {}`（先编译再下载） |
| 构建结果 | `0 Error(s), 1 Warning(s)`（既有 `main.c(245) while(k--);` 空循环体）；`Program Size: Code=25912 RO-data=656 RW-data=116 ZI-data=1676` |
| 镜像路径 | `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/output/exe/Project.axf` |
| 镜像 md5 | **`08a4c266af6a0f058821ec809d38f78d`**（本能力独立 rebuild 与实现 FWI-002 rev 6.1 声明逐字符一致） |
| 附带 hex md5 | `8fcfa1da9b7b204d83d36faf97b267e4`（`Project.hex`） |

## 下载目标与下载校验

| 项 | 值 |
|---|---|
| 目标器件 | CW32L010（U7），传感器板（GXHT40 U9 同板） |
| 调试口 | **COM43** = Prolific USB-to-Serial（VID:PID `067B:2303`），9600 8N1 |
| 下载探针 | CW-DAPLink `USB VID:PID=C251:F001`（SN `87094109484987710672FF50`，同源 COM26） |
| 下载方式 | `mdk_flash {}`（工程保存的调试器/Flash 算法；先编译再下载） |
| 下载结果 | **1 次成功**：`Erase Done.Programming Done.Verify OK.Application running ...`（本地 2026-11-08 13:57:34 = UTC 05:57:34） |

`Verify OK` 只证明**下载内容的回读校验一致**。下载后 COM43 上出现连续的 `S/G` 采样行（`k=3..60`，每周期 `k+3`），证明新镜像已在目标上运行；**本轮未捕获 `BOOT` 横幅**（`serial_capture` 与 `mdk_flash` 在本运行时严格串行），因此板件 `uid` 未重新读取，历史 `uid=6A002C00` 仅作同链路背景，不作本轮身份证据。原始结果：`evidence/ev022/tool_results.txt`；原始捕获：`evidence/ev022/cap01..cap13.cap`。

---

