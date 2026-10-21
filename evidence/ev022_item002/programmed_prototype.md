# 本次实际下载记录（EV-022，ITEM-002 受测镜像）

本文件只记录**本次实际成功下载**的固件身份与目标；`Verify OK` 仅证明下载/回读校验通过，**不表示**业务功能合格。

| 项 | 值 |
|---|---|
| 能力 | `embedded_tester.embedded_verification` |
| 项目 | dengbei_run10 |
| Git HEAD | `31be53b20a23435e03aa2ef3d2f85fae1ab7f6fb` |
| 工程 | `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/Project.uvprojx` |
| 构建 | Keil MDK / ARMCLANG V6.24；`mdk_build rebuild` 0 Error / 1 既有 warning；Code=20168 RO=644 RW=116 ZI=1676 |
| 下载工具 | `mdk_flash`（使用工程保存的调试器与 CW32L010 Flash 算法，参数 `{}`） |
| 镜像 | `MDK/output/exe/Project.axf` md5 `e8c2ebcdb927930a3366761e4f2999c9`；`Project.hex` md5 `badfd8142dbf316390e258a8af389486` |
| 目标 | 传感器板（CW32L010F8 + GXHT40 + 光敏分压 + UM2005C 发射）；调试串口 COM43 @9600 8N1 |
| 下载结果原文 | `Load "D:\\mypro\\beiwo2\\AIP\\CW32L010_StandardPeripheralLib_V1.0.5\\Examples\\sensor\\gpio_input_output\\MDK\\output\\exe\\Project.axf"` / `Erase Done.Programming Done.Verify OK.Application running ...` / `Flash Load finished at 10:26:35` |
| 时间 | 2026-10-21 10:26:35（本地，UTC+08:00） |

## 观测摘要（详见 `test.md`）

- 下载后 COM43 收到 3 个采样周期（`k=3/6/9`，间隔 180 s），全部 `q=0`、`r=0`、`s=0`，`p/H` 保持 `0/0`，`G s=2 a44=0 a45=0`。
- 本记录不把下载成功写成功能通过；成功采样路径因 GXHT40 无 ACK（U01）未在实板出现。
