# ProgrammedPrototypeArtifact（EV-032 / LIGHT-DEF-2 交付镜像）

本文件只证明「本轮实际把当前交付镜像下载到真实目标并回读校验成功」，**不表示任何业务功能合格**。功能判定见 `evidence/test.md` EV-032：**整体 `TEST_FAIL`**（上电五行/`uid`、判据启用、真实升温/跨链路、功耗未完成）；本轮已在实板确认：LIGHT-DEF-2 `T=0` 等价性（5 个周期 `V=0 D=0`）、连续 3 个采样周期 `q=1`、位级 I²C 双字 CRC 匹配、`p` 链保持。

## 本次下载的固件身份

| 项 | 值 |
|---|---|
| 验证时 HEAD | **`abaa706`**；产品源码 = 实现提交 `82a7e74`（FWI-003 rev 8），`git diff 82a7e74 -- USER/工程文件` 为空 |
| 构建方式 | `mdk_build {"action":"rebuild"}`（Keil MDK / ARMCLANG **V6.24**，工程保存配置）+ `mdk_flash {}`（先编译再下载） |
| 构建结果 | `0 Error(s), 1 Warning(s)`（既有 `main.c(254) while(k--);` 空循环体）；`Program Size: Code=25964 RO-data=656 RW-data=116 ZI-data=1676` |
| 镜像路径 | `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/output/exe/Project.axf` |
| 镜像 md5 | **`b6a15fed2ed5e3d0c4adc51ac87a4830`**（本能力独立 rebuild，与 FWI-003 rev 8 / BUILD-003 声明逐字符一致） |
| 附带 hex md5 | **`4d728dec9b0d28a352c46436b7586c72`**（`Project.hex`） |

## 下载目标与下载校验

| 项 | 值 |
|---|---|
| 目标器件 | CW32L010（U7），传感器板（GXHT40 U9 同板） |
| 调试串口 | **COM43** = Prolific USB-to-Serial（VID:PID `067B:2303`），9600 8N1 |
| 下载探针 | CW-DAPLink `USB VID:PID=C251:F001`（SN `87094109484987710672FF50`） |
| 下载结果 | **1 次成功**：2026-11-10 14:31:18.045–14:31:30.625Z，`Erase Done. Programming Done. Verify OK. Application running ...`；下载后 axf/hex md5 未变。日志见 `evidence/ev032/tool_results.txt` §2 |
| 运行证明 | 下载后串口窗口取得 `S k=3/6/9/15/18`（5 个周期），全部 `q=1`、`V=0 D=0`、0 条 `G`；位级 I²C `0x44 W FD → 0x44 R 69 15 A1 51 8C 95` 双 CRC 匹配 → 当前镜像已在目标运行 |

`Verify OK` 只证明**下载内容的回读校验一致**。本运行时 `serial_capture` 与 `mdk_flash` 严格串行，串口会话在 `Application running` 后约 **3.4 s** 才建立，**未取得 `BOOT/VDD/IOTEST/IOSIG/BUS` 与横幅 `uid`**；不得用后续周期行或历史 `uid` 替代。原始结果：`evidence/ev032/tool_results.txt`、`capture_windows.txt`、`capture_analysis.txt`、`image_identity.txt`；原始捕获：`evidence/ev032/cap01_k3.cap`、`cap02_k6_k9.cap`、`cap03_k15_k18.cap`；位级 I²C：`evidence/ev032/raw_i2c_capture.sr`、`decoded_i2c.txt`。

---
