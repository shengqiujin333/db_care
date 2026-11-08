# ProgrammedPrototypeArtifact（EV-024 / D2 读时钟速率修复镜像）

本文件只证明「本轮实际把当前交付镜像下载到真实目标并回读校验成功」，**不表示任何业务功能合格**（也不表示其他功能/条件已验收）。功能判定见 `evidence/test.md` EV-024：**整体 `TEST_FAIL`**（T5 全暗标定、INT 上报/跨链路未执行；上电横幅未取得）；但本轮修复目标已在实板上成立：**27 个采样周期 `q=1`、0 次失败**，对照上一镜像 25 周期 16 失败（64.0%）。

## 本次下载的固件身份

| 项 | 值 |
|---|---|
| 受测提交 | `3113a9680985dcaf564839ccbe57bae161a0bcda`（HEAD；接手时工作区干净；本轮**产品源码零改动**） |
| 受测源码增量（相对 `0f8a8fb`） | `USER/inc/sensor_config.h` 新增 `SENSOR_I2C_DELAY_LOOPS 300u`；`USER/src/measure.c` 的 `i2c0_dev.speed`、`i2c0_swap_dev.speed` 与 `i2c0_halfbit_delay()` 改用该常量。仅改软 I²C 读时钟速率 |
| 构建方式 | `mdk_build {"action":"rebuild"}`（Keil MDK / ARMCLANG **V6.24**，工程保存配置）+ `mdk_flash {}`（先编译再下载） |
| 构建结果 | `0 Error(s), 1 Warning(s)`（既有 `main.c(245) while(k--);` 空循环体）；`Program Size: Code=25912 RO-data=656 RW-data=116 ZI-data=1676` |
| 镜像路径 | `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/output/exe/Project.axf` |
| 镜像 md5 | **`e56d8d4b48c14ff29ae376781bbdc915`**（本能力独立 rebuild 与实现 FWI-002 rev 6.2 声明逐字符一致） |
| 附带 hex md5 | `d872d703e0ed8ec95bc3dee88f074b33`（`Project.hex`） |

## 下载目标与下载校验

| 项 | 值 |
|---|---|
| 目标器件 | CW32L010（U7），传感器板（GXHT40 U9 同板） |
| 调试口 | **COM43** = Prolific USB-to-Serial（VID:PID `067B:2303`），9600 8N1 |
| 下载探针 | CW-DAPLink `USB VID:PID=C251:F001`（SN `87094109484987710672FF50`，同源 COM26） |
| 下载方式 | `mdk_flash {}`（工程保存的调试器/Flash 算法；先编译再下载） |
| 下载结果 | **1 次成功**：`Erase Done.Programming Done.Verify OK.Application running ...`（本地 2026-11-08 15:48:23 = UTC 07:48:23） |

`Verify OK` 只证明**下载内容的回读校验一致**。下载后 COM43 上出现 `k=3..81`（每周期 +3、共 27 个周期）的连续 `S` 行，证明新镜像已在目标上运行；**本轮未捕获 `BOOT` 横幅**（`serial_capture` 与 `mdk_flash` 在本运行时严格串行，复位后约 5 s 才开始捕获），因此板件 `uid` 未重新读取，历史 `uid=6A002C00` 仅作同链路背景，不作本轮身份证据。原始结果：`evidence/ev024/tool_results.txt`、`capture_windows.txt`；原始捕获：`evidence/ev024/cap01.cap` … `cap16.cap`。
