# Programmed Prototype — ITEM-004 实际下载身份

能力：`embedded_tester.embedded_verification`
作用：仅记录本次实际成功下载的固件身份与目标；`Verify OK` 只证明下载校验，**不表示业务功能合格**。

| 项 | 值 |
|---|---|
| 下载工具 | `mdk_flash`（使用 Keil 工程保存的 CMSIS-DAP 与 CW32L010 Flash 算法；参数 `{}`） |
| 工程 | `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/Project.uvprojx` |
| 编译 | Keil MDK ARMCLANG V6.24，Build target 'Project'，0 Error / 0 Warning（下载前 build） |
| 下载日志 | `Erase Done. Programming Done. Verify OK. Application running ...`（`Flash Load finished at 11:21:53` 本地） |
| 镜像 | `Project.axf` md5 `a0b2b8309435dae29c64e97df8a355b7`；`Project.hex` md5 `cc7fbd8c3919621ecebbab182ad68a0e` |
| 受测提交 | Git HEAD `7081ed9`（ITEM-004 实现提交，工作区干净） |
| 目标 | 传感器板 CW32L010F8（GXHT40 + 光敏分压 + UM2005C），调试串口 COM43（Prolific `067B:2303`，9600 8N1） |
| 调试器 | 本轮 USB 枚举到 `VID_C251/PID_F001`（COM26 为其 CDC 口，不是传感器调试串口） |

镜像 md5 与实现能力记录 `artifacts/firmware_implementation_v6.md`、`evidence/ev023_item004/image_md5.txt` 一致。
