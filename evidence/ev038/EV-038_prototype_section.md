# ProgrammedPrototypeArtifact（EV-038 / 当前发布包 FD-002r8 rev 2 独立重建、下载与上电身份回读）

本文件只证明「本轮实际把当前发布候选包对应的镜像下载到真实目标并回读校验成功」，**不表示任何业务功能合格**。功能判定见 `evidence/test.md` EV-038：本交付已执行范围 **HIL_PASS**；`FW-LIGHT-DEF2-ENABLE` 经人工确认未执行（`SKIP_CONFIRMED`）、`E2E-04` 已批准延期未测，均在报告中列明。本轮已在实板确认：上电五行/`uid`、7 个采样行 `q=1`（k=0/3/6/9/12/15/21）、位级 I²C 双字 CRC 匹配、`p` 链保持。

## 本次下载的固件身份

| 项 | 值 |
|---|---|
| 验证时 HEAD | `15f70a5d4b9570817ce4e71320730d2728be6fdf`；产品固件源 = 实现提交 `82a7e74`（FWI-003 rev 8），`git diff 82a7e74..HEAD -- <sensor>/USER MDK EWARM gcc` 为空 |
| 发布包 | `fw_release/fw_sensor_FD-002r8.zip` md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf`（134,791 B）；三成员 MD5 与 `PACKAGE.txt` 一致 |
| 构建方式 | 本能力独立 `mdk_build {"action":"rebuild"}`（Keil MDK / ARMCLANG **V6.24**，工程保存配置）+ `mdk_flash {}` |
| 构建结果 | `0 Error(s), 1 Warning(s)`（既有 `main.c(254) while(k--);` 空循环体）；`Program Size: Code=25964 RO-data=656 RW-data=116 ZI-data=1676` |
| 镜像路径 | `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/output/exe/Project.axf` |
| 镜像 md5 | **`b6a15fed2ed5e3d0c4adc51ac87a4830`**（297,380 B；与发布包 axf 成员逐字节一致） |
| 附带 hex md5 | **`4d728dec9b0d28a352c46436b7586c72`**（75,275 B；与发布包 hex 成员逐字节一致） |

## 下载目标与下载校验

| 项 | 值 |
|---|---|
| 目标器件 | CW32L010（U7），传感器板（GXHT40 U9 同板） |
| 调试串口 | **COM43** = Prolific USB-to-Serial（VID:PID `067B:2303`），9600 8N1 |
| 下载探针 | CW-DAPLink `USB VID:PID=C251:F001`（SN `87094109484987710672FF50`） |
| 下载结果 | **1 次成功**：2026-11-11T11:59:59.554–12:00:11.910Z，`Erase Done.Programming Done.Verify OK.Application running ...`；下载前后 axf/hex md5 未变 |
| 运行/身份证明 | 端口先开 + `pyocd` HW reset（12:00:22.932–12:00:23.645Z，rc=0）后同一窗口取得 `BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600`、`VDD ok=1 code=1481 bgrmv=1189 mv=3287`、`IOTEST … gc=0`、`IOSIG scl=95555 sda=30300`、`BUS idle=3 scl=1 sda=1 ack=44` 与 `S k=0 … q=1`；随后 k=3/6/9/12/15/21 采样行 |

`Verify OK` 只证明**下载内容的回读校验一致**。原始结果：`evidence/ev038/tool_results.txt`、`capture_windows.txt`、`capture_analysis.txt`；原始捕获：`evidence/ev038/bot01_hw.cap`、`cap01_k3.cap`、`cap02_k6_k9.cap`、`cap03_k12_k15.cap`、`cap04_k21.cap`；位级 I²C：`raw_i2c_capture.sr`、`decoded_i2c.txt`。

---

