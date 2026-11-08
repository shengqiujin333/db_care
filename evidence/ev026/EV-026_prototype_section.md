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
