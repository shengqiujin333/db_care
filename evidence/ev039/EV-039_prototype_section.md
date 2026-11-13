# ProgrammedPrototypeArtifact（EV-039 / 当前固件 FD-002r8 独立重建、下载与上电身份回读）

本文件只证明「本轮实际把当前固件镜像下载到真实目标并回读校验成功」，**不表示任何业务功能合格**。
功能判定见 `evidence/test.md` EV-039：当前 TD-002 rev 9.0 适用范围 **TEST_PASS**；
`FW-LIGHT-DEF2-ENABLE` 经人工确认未执行（`SKIP_CONFIRMED`）、`E2E-04` 已批准延期未测，
均在报告中列明。本轮实板已确认：上电五行/`uid`、6 个采样行 `q=1`（k=0/3/9/12/15/18，
其中 k=9/12/15/18 连续）、`p` 链保持、首样本 `H=0 p=0`。

## 本次下载的固件身份

| 项 | 值 |
|---|---|
| 验证时 HEAD | `9ce380cfb7fc5779d85df1f41db5f59a673e622d`；产品固件源 = 实现提交 `82a7e74`（FWI-004 rev 9），`git diff --stat 82a7e74..HEAD -- <sensor>/{USER,MDK,EWARM,UM2005C,COMMON}` 为空 |
| 发布包 | `fw_release/fw_sensor_FD-002r8.zip` md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf`（134,791 B）；三成员 MD5 与 `PACKAGE.txt` 一致；无测试成员 |
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
| 下载结果 | **1 次成功**：2026-11-13T06:28:51.227–06:29:04.466Z，`Erase Done.Programming Done.Verify OK.Application running ...`；下载镜像即上述 axf |
| 运行/身份证明 | 端口先开 + `pyocd 0.36.0` 通用 `cortex_m` HW reset（06:29:23.386Z，rc=0，仅复位）后同一窗口取得 `BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600`、`VDD ok=1 code=1480 bgrmv=1189 mv=3289`、`IOTEST … gc=0`、`IOSIG scl=95555 sda=30300`、`BUS idle=3 scl=1 sda=1 ack=44` 与 `S k=0 … q=1`；随后 k=3/9/12/15/18 采样行 |

`Verify OK` 只证明**下载内容的回读校验一致**。
原始结果：`evidence/ev039/tool_results.txt`、`capture_windows.txt`、`capture_analysis.txt`、`mdk_build_flash.log`；
原始捕获：`evidence/ev039/bot01_hw.cap`、`cap01_k3.cap`、`cap02_k9_k12.cap`、`cap03_k15_k18.cap`。

---

