# ProgrammedPrototypeArtifact（EV-037 / 当前固件独立重建、下载与上电身份回读）

本文件只证明「本轮实际把当前固件镜像下载到真实目标并回读校验成功」，**不表示任何业务功能合格**。功能判定见 `evidence/test.md` EV-037：本交付已执行范围 **TEST_PASS**；`FW-LIGHT-DEF2-ENABLE` 经人工确认未执行（`SKIP_CONFIRMED`）、`E2E-04` 已批准延期未测，均在报告中列明。本轮已在实板确认：上电五行/`uid`、7 个采样行 `q=1`（k=0/3/6/9/12/15/21）、位级 I²C 双字 CRC 匹配、`p` 链保持。

## 本次下载的固件身份

| 项 | 值 |
|---|---|
| 验证时 HEAD | `d5701f55b456e5fe07440c8dc4ec4ed206fa626e`；产品固件源 = 实现提交 `82a7e74`（FWI-003 rev 8），`git diff 82a7e74..HEAD -- <sensor>/USER MDK EWARM` 为空 |
| 镜像来源 | 本能力独立 `mdk_build {"action":"rebuild"}`；axf/hex 与发布包 `fw_release/fw_sensor_FD-002r8.zip`（md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf`）成员逐字节一致 |
| 构建方式 | Keil MDK / ARMCLANG **V6.24**（工程保存配置）+ `mdk_flash {}` |
| 构建结果 | `0 Error(s), 1 Warning(s)`（既有 `main.c(254) while(k--);` 空循环体）；`Code=25964 RO-data=656 RW-data=116 ZI-data=1676` |
| 镜像路径 | `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/output/exe/Project.axf` |
| 镜像 md5 | **`b6a15fed2ed5e3d0c4adc51ac87a4830`**（297,380 B） |
| 附带 hex md5 | **`4d728dec9b0d28a352c46436b7586c72`**（75,275 B） |

## 下载目标与下载校验

| 项 | 值 |
|---|---|
| 目标器件 | CW32L010（U7），传感器板（GXHT40 U9 同板） |
| 调试串口 | **COM43** = Prolific USB-to-Serial（VID:PID `067B:2303`），9600 8N1 |
| 下载探针 | CW-DAPLink `USB VID:PID=C251:F001`（SN `87094109484987710672FF50`） |
| 下载结果 | **1 次成功**：2026-11-11T11:27:45.931–11:27:58.287Z，`Erase Done.Programming Done.Verify OK.Application running ...`；下载前后 axf/hex md5 未变 |
| 运行/身份证明 | 端口先开 + `pyocd` HW reset（11:28:12Z，rc=0）后同一窗口取得 `BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600`、`VDD ok=1 code=1481 bgrmv=1189 mv=3287`、`IOTEST … gc=0`、`IOSIG scl=95555 sda=30300`、`BUS idle=3 scl=1 sda=1 ack=44` 与 `S k=0 … q=1`；随后 k=3/6/9/12/15/21 采样行 |

`Verify OK` 只证明**下载内容的回读校验一致**。原始结果：`evidence/ev037/image_identity.txt`、`tool_results.txt`、`capture_analysis.txt`；原始捕获：`evidence/ev037/bot01_hw.bin`、`cap01_k3.bin`、`cap02_k6_k9.bin`、`cap03_k12_k15.bin`、`cap04_k21.bin`；位级 I²C：`raw_i2c_capture.sr`、`decoded_i2c.txt`。

---

# ProgrammedPrototypeArtifact（EV-036 / 发布包 FD-002r8 独立重建、下载与上电身份回读）

本文件只证明「本轮实际把发布候选包对应的镜像下载到真实目标并回读校验成功」，**不表示任何业务功能合格**。功能判定见 `evidence/test.md` EV-036：本交付已执行范围 **HIL_PASS**；`FW-LIGHT-DEF2-ENABLE` 经人工确认未执行（`SKIP_CONFIRMED`）、`E2E-04` 已批准延期未测，均在报告中列明。本轮已在实板确认：上电五行/`uid`、6 个采样行 `q=1`（k=0/3/6/9/15/18）、位级 I²C 双字 CRC 匹配、`p` 链保持。

## 本次下载的固件身份

| 项 | 值 |
|---|---|
| 验证时 HEAD | `5b9b7aa63309c82de1b90e46b533d4559d2f2efa`；产品源码 = 实现提交 `82a7e74`（FWI-003 rev 8），`git diff 82a7e74..HEAD -- <sensor>/USER MDK EWARM` 为空 |
| 发布包 | `fw_release/fw_sensor_FD-002r8.zip` md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf`（134,791 B）；三成员 MD5 与 `PACKAGE.txt` 一致 |
| 构建方式 | `mdk_build {"action":"rebuild"}`（Keil MDK / ARMCLANG **V6.24**，工程保存配置）+ `mdk_flash {}`（先编译再下载） |
| 构建结果 | `0 Error(s), 1 Warning(s)`（既有 `main.c(254) while(k--);` 空循环体）；`Program Size: Code=25964 RO-data=656 RW-data=116 ZI-data=1676` |
| 镜像路径 | `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/output/exe/Project.axf` |
| 镜像 md5 | **`b6a15fed2ed5e3d0c4adc51ac87a4830`**（本能力独立 rebuild，与发布包 axf 成员逐字节一致） |
| 附带 hex md5 | **`4d728dec9b0d28a352c46436b7586c72`**（`Project.hex`，与发布包 hex 成员逐字节一致） |

## 下载目标与下载校验

| 项 | 值 |
|---|---|
| 目标器件 | CW32L010（U7），传感器板（GXHT40 U9 同板） |
| 调试串口 | **COM43** = Prolific USB-to-Serial（VID:PID `067B:2303`），9600 8N1 |
| 下载探针 | CW-DAPLink `USB VID:PID=C251:F001`（SN `87094109484987710672FF50`） |
| 下载结果 | **1 次成功**：2026-11-11T05:15:19.665–05:15:32.356Z，`Erase Done.Programming Done.Verify OK.Application running ...`；下载前后 axf/hex md5 未变 |
| 运行/身份证明 | 端口先开 + `pyocd` HW reset 后同一窗口取得 `BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600`、`VDD ok=1 code=1480 bgrmv=1189 mv=3289`、`IOTEST … gc=0`、`IOSIG scl=95555 sda=30300`、`BUS idle=3 scl=1 sda=1 ack=44` 与 `S k=0 … q=1`；随后 k=3/6/9/15/18 采样行 |

`Verify OK` 只证明**下载内容的回读校验一致**。原始结果：`evidence/ev036/release_identity.txt`、`tool_results.txt`、`capture_analysis.txt`；原始捕获：`evidence/ev036/bot01_hw.bin`、`cap01_k3.bin`、`cap02_k6_k9.bin`、`cap03_k15_k18.bin`；位级 I²C：`raw_i2c_capture.sr`、`decoded_i2c.txt`。

---

# ProgrammedPrototypeArtifact（EV-035 / LIGHT-DEF-2 交付镜像下载 + 上电身份回读）

本文件只证明「本轮实际把当前交付镜像下载到真实目标并回读校验成功」，**不表示任何业务功能合格**。功能判定见 `evidence/test.md` EV-035：本交付范围内已执行的必需用例 **TEST_PASS**；`FW-LIGHT-DEF2-ENABLE` 经人工确认未执行（`SKIP_CONFIRMED`）、`E2E-04` 已批准延期未测，均在报告中列明。本轮已在实板确认：上电五行/`uid`（BOT-01/02/03/04）、6 个采样行 `q=1`（k=0/3/6/12/15/18，连续集 k=0/3/6 与 k=12/15/18）、位级 I²C 双字 CRC 匹配、`p` 链保持。

## 本次下载的固件身份

| 项 | 值 |
|---|---|
| 验证时 HEAD | `cfe98a18c7932840195f80ed7d587433e0a0701f`；产品源码 = 实现提交 `82a7e74`（FWI-003 rev 8），`git diff 82a7e74..HEAD -- <sensor>/USER MDK EWARM` 为空 |
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
| 下载结果 | **1 次成功**：2026-11-11T04:38:30.078–04:38:42.466Z，`Erase Done.Programming Done.Verify OK.Application running ...`；下载后 axf/hex md5 未变 |
| 运行/身份证明 | 端口先开 + `pyocd` HW reset 后同一窗口取得 `BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600`、`VDD ok=1 code=1481 bgrmv=1189 mv=3287`、`IOTEST … gc=0`、`IOSIG scl=95555 sda=30300`、`BUS idle=3 scl=1 sda=1 ack=44` 与 `S k=0 … q=1`；随后 k=3/6/12/15/18 采样行 |

`Verify OK` 只证明**下载内容的回读校验一致**。原始结果：`evidence/ev035/image_identity.txt`、`capture_windows.txt`、`capture_analysis.txt`、`tool_results.txt`；原始捕获：`evidence/ev035/bot01_hw.bin`、`cap01_k3_k6.bin`、`cap02_k12.bin`、`cap03_k15_k18.bin`；位级 I²C：`raw_i2c_capture.sr`、`decoded_i2c.txt`。

---

# ProgrammedPrototypeArtifact（EV-034 / LIGHT-DEF-2 交付镜像下载 + 上电身份回读）

本文件只证明「本轮实际把当前交付镜像下载到真实目标并回读校验成功」，**不表示任何业务功能合格**。功能判定见 `evidence/test.md` EV-034：整体 `TEST_FAIL`（判据启用、真实升温/跨链路、功耗未完成）；本轮已在实板确认：上电五行/`uid` 取得（BOT-01/02/03）、连续 5 个采样周期 `q=1`（k=0/3/6/9/12）、位级 I²C 双字 CRC 匹配、`p` 链保持。

## 本次下载的固件身份

| 项 | 值 |
|---|---|
| 验证时 HEAD | `d7a645cc9aa732fd752d34933dd672d30d4932b3`；产品源码 = 实现提交 `82a7e74`（FWI-003 rev 8），`git diff 82a7e74..HEAD -- USER/ MDK/ EWARM/` 为空 |
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
| 下载结果 | **1 次成功**：2026-11-11T03:00:06.302–03:00:18.715Z，`Erase Done.Programming Done.Verify OK.Application running ...`；下载后 axf/hex md5 未变 |
| 运行/身份证明 | 端口先开 + `pyocd` HW reset 后同一窗口取得 `BOOT fw=FD-002r4 uid=6A002C00 rst=0040 uart=9600`、`VDD ok=1 code=1481 bgrmv=1189 mv=3287`、`IOTEST … gc=0`、`IOSIG scl=95555 sda=30300`、`BUS idle=3 scl=1 sda=1 ack=44` 与 `S k=0 … q=1`；随后 k=3/6/9/12/18 采样行 |

`Verify OK` 只证明**下载内容的回读校验一致**。原始结果：`evidence/ev034/image_identity.txt`、`capture_windows.txt`、`capture_analysis.txt`、`tool_results.txt`；原始捕获：`evidence/ev034/bot01_hw.bin`、`cap01_k3_k6.bin`、`cap02_k9_k12.bin`、`cap03_k18.bin`；位级 I²C：`raw_i2c_capture.sr`、`decoded_i2c.txt`、`i2c_analysis.txt`。

---

# ProgrammedPrototypeArtifact（EV-033 / LIGHT-DEF-2 交付镜像再次下载）

本文件只证明「本轮实际把当前交付镜像下载到真实目标并回读校验成功」，**不表示任何业务功能合格**。功能判定见 `evidence/test.md` EV-033：**整体 `TEST_FAIL`**（上电五行/`uid`、判据启用、真实升温/跨链路、功耗未完成）；本轮已在实板再次确认：LIGHT-DEF-2 `T=0` 等价性（5 个周期 `V=0 D=0`）、连续 3 个采样周期 `q=1`（k=9/12/15）、位级 I²C 双字 CRC 匹配、`p` 链保持。

## 本次下载的固件身份

| 项 | 值 |
|---|---|
| 验证时 HEAD | **`0008dde`**；产品源码 = 实现提交 `82a7e74`（FWI-003 rev 8），`git diff 82a7e74..HEAD -- USER/ MDK/ EWARM/` 为空 |
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
| 下载结果 | **1 次成功**：2026-11-10 23:14:32.82–23:14:45.41Z，`Erase Done. Programming Done. Verify OK. Application running ...`；下载后 axf/hex md5 未变。日志见 `evidence/ev033/mdk_flash.log`、`tool_results.txt` §3 |
| 运行证明 | 下载后串口窗口取得 `S k=3/9/12/15/21`（5 个周期），全部 `q=1`、`V=0 D=0`、0 条 `G`；位级 I²C `0x44 W FD → 0x44 R 63 DF D3 63 EA E3` 双 CRC 匹配 → 当前镜像已在目标运行 |

`Verify OK` 只证明**下载内容的回读校验一致**。本运行时 `serial_capture` 与 `mdk_flash` 严格串行，串口会话在 `Application running` 后约 **12.9 s** 才建立，**未取得 `BOOT/VDD/IOTEST/IOSIG/BUS` 与横幅 `uid`**；用户已明确回复“无法准备，先跳过”，该项按**人工确认未执行**（`SKIP_CONFIRMED`）记录、不再重测；不得用后续周期行或历史 `uid` 替代。原始结果：`evidence/ev033/tool_results.txt`、`capture_windows.txt`、`capture_analysis.txt`、`image_identity.txt`；原始捕获：`evidence/ev033/cap01_k3.bin`、`cap02_k9.bin`、`cap03_k12_k15.bin`、`cap04_k21.bin`；位级 I²C：`evidence/ev033/raw_i2c_capture.sr`、`decoded_i2c.txt`。

---

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

# ProgrammedPrototypeArtifact（EV-030 / 当前交付镜像完整交付复核）

本文件只证明「本轮实际把当前交付镜像下载到真实目标并回读校验成功」，**不表示任何业务功能合格**。功能判定见 `evidence/test.md` EV-030：**整体 `TEST_FAIL`**（上电五行/`uid`、T5 全暗标定、触发/跨链路/功耗未完成）；本轮已在实板确认：连续 3 个采样周期 `q=1`、位级 I²C 双字 CRC 匹配、首样本 `H=0 p=0`。

## 本次下载的固件身份

| 项 | 值 |
|---|---|
| 受测提交 | **`f978a01`**（`[role:firmware_engineer]`；接手时工作区干净；本轮产品源码零改动） |
| 产品源码差异 | `git diff --stat ef5caa6 HEAD -- .../USER .../test` **为空** |
| 构建方式 | `mdk_build {"action":"rebuild"}`（Keil MDK / ARMCLANG **V6.24**，工程保存配置）+ `mdk_flash {}`（先编译再下载） |
| 构建结果 | `0 Error(s), 1 Warning(s)`（既有 `main.c(254) while(k--);` 空循环体）；`Program Size: Code=25964 RO-data=656 RW-data=116 ZI-data=1676` |
| 镜像路径 | `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/output/exe/Project.axf` |
| 镜像 md5 | **`a3b08774e764f174eac318f6771d0c42`**（本能力独立 rebuild，与 FWI-002 rev 7 / TD-002 rev 7 期望逐字符一致） |
| 附带 hex md5 | `546ce20a718fa69b40177cce0145f4b5`（`Project.hex`） |

## 下载目标与下载校验

| 项 | 值 |
|---|---|
| 目标器件 | CW32L010（U7），传感器板（GXHT40 U9 同板） |
| 调试串口 | **COM43** = Prolific USB-to-Serial（VID:PID `067B:2303`），9600 8N1 |
| 下载探针 | CW-DAPLink `USB VID:PID=C251:F001`（SN `87094109484987710672FF50`，同源 COM26） |
| 下载方式 | `mdk_flash {}`（工程保存的调试器/Flash 算法；先编译再下载） |
| 下载结果 | **1 次成功**：2026-11-09 08:22:23.679–08:22:36.147Z，`Erase Done.Programming Done.Verify OK.Application running ...`；日志见 `evidence/ev030/tool_results.txt` §7 |
| 运行证明 | 下载后 0.208 s 建立的串口会话：`BUS idle=3 scl=1 sda=1 ack=44`、`S k=0 … H=0 p=0 q=1`；随后 `k=6/9/12` 连续 `q=1`、`k=18` `q=1`，0 条 `G` → 新镜像已在目标运行 |

`Verify OK` 只证明**下载内容的回读校验一致**。本运行时 `serial_capture` 与 `mdk_flash` 严格串行，串口会话在 `Application running` 后约 **0.208 s** 才建立，**本轮只捕获到 `BUS` 与首条 `S`，未取得 `BOOT/VDD/IOTEST/IOSIG` 四行与横幅 `uid`**；不得用后续周期行或历史 `uid` 替代。原始结果：`evidence/ev030/tool_results.txt`、`capture_windows.txt`、`capture_analysis.txt`；原始捕获：`evidence/ev030/cap01_boot_bus_k0.cap` … `cap04_k18.cap`；位级 I²C：`raw_i2c_capture.sr`、`decoded_i2c.txt`。

---

# ProgrammedPrototypeArtifact（EV-028 / F-1 `S` 行前值语义修复镜像）

本文件只证明「本轮实际把当前交付镜像下载到真实目标并回读校验成功」，**不表示任何业务功能合格**。功能判定见 `evidence/test.md` EV-028：**整体 `TEST_FAIL`**（T4-02 上电五行仍不完整、T5 全暗标定、INT 上报/跨链路未执行）；本轮修复目标已在实板上成立：复位后首条 `S k=0` 为 **`H=0 p=0`**，后续周期 `p` 为上一周期 `t` 且 `p≠t`，7 条有原始行证据的 `S` 全部 `q=1`、0 条 `G`。

## 本次下载的固件身份

| 项 | 值 |
|---|---|
| 受测提交 | **`bee3d8f`**（`[role:firmware_engineer]`；接手时工作区干净） |
| 受测源码增量（相对 `d871957`） | `USER/src/measure.c` **+8/−2**：推进前值前缓存判定所用 `prev/have` 供 `S` 行观测；判定逻辑不变。实现侧 `test/host_measure_flow_check.c` +12 |
| 构建方式 | `mdk_build {"action":"rebuild"}`（Keil MDK / ARMCLANG **V6.24**，工程保存配置）+ `mdk_flash {}`（先编译再下载） |
| 构建结果 | `0 Error(s), 1 Warning(s)`（既有 `main.c(254) while(k--);` 空循环体）；`Program Size: Code=25964 RO-data=656 RW-data=116 ZI-data=1676` |
| 镜像路径 | `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/output/exe/Project.axf` |
| 镜像 md5 | **`a3b08774e764f174eac318f6771d0c42`**（本能力独立 rebuild 与 FWI-002 rev 6.4 / BUILD-002 rev 6.4 声明逐字符一致） |
| 附带 hex md5 | `546ce20a718fa69b40177cce0145f4b5`（`Project.hex`） |

## 下载目标与下载校验

| 项 | 值 |
|---|---|
| 目标器件 | CW32L010（U7），传感器板（GXHT40 U9 同板） |
| 调试串口 | **COM43** = Prolific USB-to-Serial（VID:PID `067B:2303`），9600 8N1 |
| 下载探针 | CW-DAPLink `USB VID:PID=C251:F001`（SN `87094109484987710672FF50`，同源 COM26） |
| 下载方式 | `mdk_flash {}`（工程保存的调试器/Flash 算法；先编译再下载） |
| 下载结果 | **2 次成功**：2026-11-08 11:28:38–11:28:51Z 与 11:39:34–11:39:47Z，均为 `Erase Done.Programming Done.Verify OK.Application running ...`；日志 `evidence/ev028/flash_1.log`、`flash_2.log` |
| 运行证明 | 第二次下载后 0.073 s 建立的串口会话：`BUS idle=3 scl=1 sda=1 ack=44`、`S k=0 … H=0 p=0 q=1`，随后 `S k=3`、`k=9`、`k=18`（全部 `q=1`、0 `G`）→ 新镜像已在目标运行 |

`Verify OK` 只证明**下载内容的回读校验一致**。本运行时 `serial_capture` 与 `mdk_flash` 严格串行，串口会话在 `Application running` 后约 **73 ms** 才建立，**本轮只捕获到 `BUS` 与首条 `S`，未取得 `BOOT/VDD/IOTEST/IOSIG` 四行与横幅 `uid`**；不得用后续周期行或历史 `uid` 替代。原始结果：`evidence/ev028/tool_results.txt`、`capture_analysis.txt`；原始捕获：`evidence/ev028/cap01_k3_postflash.bin` … `cap05_k18.bin`。

---

# ProgrammedPrototypeArtifact（EV-026 / E1 `delay_ms()` 修复镜像）

本文件只证明「本轮实际把当前交付镜像下载到真实目标并回读校验成功」，**不表示任何业务功能合格**。功能判定见 `evidence/test.md` EV-026：**整体 `TEST_FAIL`**（T4-02 上电五行/横幅不完整、T5 全暗标定、INT 上报/跨链路未执行）；但本轮修复目标已在实板上成立：`delay()` 不再为空实现，位级 STOP→读 START **32.525 ms**（修前 0.622 ms），15 条串口 `S`（k=0…57）全部 `q=1`、0 条 `G`。

## 本次下载的固件身份

| 项 | 值 |
|---|---|
| 受测提交 | `d871957`（`[role:firmware_engineer]`；接手时工作区干净；本轮**产品源码零改动**） |
| 受测源码增量（相对 `3113a96`） | 仅 `USER/src/main.c`（+15/−6）：`delay()` 计数变量 `volatile`、参数 `uint32_t`；`sensor_config.h` 与其它模块未变 |
| 构建方式 | `mdk_build {"action":"rebuild"}`（Keil MDK / ARMCLANG **V6.24**，工程保存配置）+ `mdk_flash {}`（先编译再下载） |
| 构建结果 | `0 Error(s), 1 Warning(s)`（既有 `main.c(254) while(k--);` 空循环体）；`Program Size: Code=25940 RO-data=656 RW-data=116 ZI-data=1676` |
| 镜像路径 | `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/output/exe/Project.axf` |
| 镜像 md5 | **`917bffe63ba0a4945a20a9a51f4a60df`**（本能力独立 rebuild 与实现 FWI-002 rev 6.3 / BUILD-002 rev 6.3 声明逐字符一致） |
| 附带 hex md5 | `5b017c84d693ce17318fca633b27ad07`（`Project.hex`） |

## 下载目标与下载校验

| 项 | 值 |
|---|---|
| 目标器件 | CW32L010（U7），传感器板（GXHT40 U9 同板） |
| 调试串口 | **COM43** = Prolific USB-to-Serial（VID:PID `067B:2303`），9600 8N1 |
| 下载探针 | CW-DAPLink `USB VID:PID=C251:F001`（SN `87094109484987710672FF50`，同源 COM26） |
| 下载方式 | `mdk_flash {}`（工程保存的调试器/Flash 算法；先编译再下载） |
| 下载结果 | **2 次成功**：2026-11-08 09:46:29–09:46:41Z 与 09:47:13–09:47:25Z，均为 `Erase Done.Programming Done.Verify OK.Application running ...`；日志 `evidence/ev026/flash_1.log`、`flash_2.log` |
| 运行证明 | 第二次下载后同一会话：`BUS idle=3 scl=1 sda=1 ack=44`、`S k=0 … q=1`，随后 15 条 `S`（k=0…57，全部 3 的倍数）→ 新镜像已在目标运行 |

`Verify OK` 只证明**下载内容的回读校验一致**。由于本运行时 `serial_capture` 与 `mdk_flash` 严格串行，串口会话在 `Application running` 后约 68 ms 才建立，**本轮只捕获到 `BUS` 与首条 `S`，未取得 `BOOT/VDD/IOTEST/IOSIG` 四行与横幅 `uid`**；不得用后续周期行或历史 `uid` 替代。原始结果：`evidence/ev026/tool_results.txt`、`capture_windows.txt`；原始捕获：`evidence/ev026/cap01_postflash_bus_k0.cap` … `cap10_k54_k57.cap`。

---

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

---

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

# ProgrammedPrototypeArtifact（EV-018 / 当前交付构建 T1–T5 复验）

本文件只证明「本轮实际把当前交付镜像下载到真实目标并回读校验成功」，**不表示任何业务功能合格**。功能判定见 `evidence/test.md` EV-018（结论 **`TEST_FAIL`**：当前构建 15 个采样周期中 10 次 `q=1`、5 次 `q=0`；全部失败为 `G s=4` 温度 CRC 错，6/6 失败帧呈现“温度 CRC 字节 = 温度 MSB、湿度 CRC 正常”的同一签名；T5 全暗标定与 INT 上报未执行）。

## 本次下载的固件身份

| 项 | 值 |
|---|---|
| 受测提交 | `7bfd248`（HEAD；`git status` 干净；本轮**产品源码零改动**） |
| 受测源码版本 | `git log -1 -- <FW>/USER` = `7bfd248`；交付增量 = `USER/{fw_core.c,measure.c,debug_trace.c,debug_trace.h,sensor_config.h}`（T1 判定、T2 冻结+P 行、T3 有界等待） |
| 构建方式 | `mdk_build {action:rebuild}`（Keil MDK / ARMCLANG **V6.24**，工程保存配置）；`mdk_flash {}` 内含增量构建 |
| 构建结果 | `0 Error(s), 1 Warning(s)`（既有 `main.c(245) while(k--);` 空循环体）；`Program Size: Code=25912 RO-data=656 RW-data=116 ZI-data=1676` |
| 镜像路径 | `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/output/exe/Project.axf` |
| 镜像 md5 | **`c7ed03b796f2ae438b325c7b9306d836`**（与实现 `evidence/build.md` 声明逐字符一致） |
| 附带 hex md5 | `79090d64d8e5487ee896983401f828ec`（`Project.hex`） |

## 下载目标与下载校验

| 项 | 值 |
|---|---|
| 目标器件 | CW32L010（U7），传感器板（GXHT40 U9 同板） |
| 调试口 | **COM43** = Prolific USB-to-Serial（VID:PID `067B:2303`），9600 8N1 |
| 下载探针 | CW-DAPLink `USB VID:PID=C251:F001`（SN `87094109484987710672FF50`，同源 COM26） |
| 下载方式 | `mdk_flash {}`（工程保存的调试器/Flash 算法；先编译再下载） |
| 下载结果 | **2 次均成功**：`Erase Done.Programming Done.Verify OK.Application running ...`（本地 2026-11-08 12:24:48、12:53:41）；两次下载的 axf md5 相同 |

`Verify OK` 只证明**下载内容的回读校验一致**。本轮板上行为自证：下载后 COM43 上出现当前构建的 `BUS idle=3 scl=1 sda=1 ack=44` 与 `S/G` 采样行（含 `G s=4 … raw=…`；旧镜像在下载前也可见同类行，但 `ack` 与周期编号不同）。**本轮未捕获 `BOOT` 横幅**，因此未重新读取 MCU `uid`；历史 EV-017 的 `uid=6A002C00` 仅作同板背景，不作本轮身份证据。原始日志：`evidence/ev018/flash_1.log`、`flash_2.log`；采集窗口：`evidence/ev018/capture_windows.txt`。

---

# 历史记录（EV-017 / run8 ITEM-002）

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
