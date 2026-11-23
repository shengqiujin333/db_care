# ProgrammedPrototypeArtifact（EV-041 / 当前固件 FD-002r8 下载与上电身份回读）

本文件只证明「本轮实际把当前固件镜像下载到真实目标并回读校验成功」，**不表示任何业务功能合格**。
功能判定见 `evidence/ev041/EV-041_section.md` / `evidence/test.md` EV-041：E2E-JOINT-P0 的**嵌入式段 TEST_PASS**；手机段与平台段为 NOT EXECUTED（非本能力）。

## 本次下载的固件身份

| 项 | 值 |
|---|---|
| 验证时 HEAD | `808107d`；产品固件源 = 实现提交 `82a7e74`，`git diff --stat 82a7e74..HEAD -- <sensor>/{USER,MDK,COMMON,UM2005C}` 为空 |
| 发布包 | `fw_release/fw_sensor_FD-002r8.zip` md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf`（134,791 B）；成员 axf/hex/PACKAGE 与包内自述一致 |
| 构建方式 | Keil MDK / ARMCLANG **V6.24**，工程保存配置；`mdk_build {"action":"rebuild"}` + `mdk_flash {}` |
| 构建结果 | `0 Error(s), 1 Warning(s)`（既有 `USER/src/main.c(254): while(k--);` 空循环体）；`Program Size: Code=25964 RO-data=656 RW-data=116 ZI-data=1676` |
| 镜像路径 | `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/output/exe/Project.axf` |
| 镜像 md5 | **`b6a15fed2ed5e3d0c4adc51ac87a4830`**（297,380 B；与发布包 axf 成员逐字节一致） |
| 附带 hex md5 | **`4d728dec9b0d28a352c46436b7586c72`**（75,275 B；与发布包 hex 成员逐字节一致） |

## 下载目标与下载校验

| 项 | 值 |
|---|---|
| 目标器件 | CW32L010F8（Whxy），传感器板（GXHT40 同板） |
| 调试串口 | **COM43** = Prolific USB-to-Serial（`VID:PID=067B:2303`，LOCATION=1-9.4.2），9600 8N1 |
| 下载探针 | CW-DAPLink `VID:PID=C251:F001`（SN `87094109484987710672FF50`） |
| 下载结果 | 2026-11-23T08:13:11.758Z–08:13:24.274Z：`Erase Done.Programming Done.Verify OK.Application running ...`；下载镜像即上述 axf |
| 运行/身份证明 | 端口先开后由同一次下载复位触发；同一窗口取得 `BOOT fw=FD-002r4 uid=6A002C00 rst=0240 uart=9600`、`VDD ok=1 code=1480 bgrmv=1189 mv=3289`、`IOTEST … gc=0`、`IOSIG scl=95555 sda=30300`、`BUS idle=3 scl=1 sda=1 ack=44`，随后 `S k=0 … q=1 r=0 s=0`；180 s 后另取 `S k=3 … q=1 r=0 s=0` |

`Verify OK` 只证明**下载内容的回读校验一致**；串口有序输出只证明该镜像在板上启动并采样，不证明网关/手机/平台链路。
原始结果：`evidence/ev041/tool_results.txt`、`ev041_boot_capture.cap/.log`、`ev041_serial_capture_k3.cap`、`capture_analysis.txt`。
