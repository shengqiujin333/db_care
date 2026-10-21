# Programmed Prototype — EV-025（ITEM-005）

状态：**已实际下载并校验**（仅证明下载/目标身份，**不表示业务功能合格**）
日期：2026-10-21（本地 11:53:56 / 12:08:08）

| 项 | 值 |
|---|---|
| 目标 | 传感器板 CW32L010F8（GXHT40 + 光敏分压 + UM2005C） |
| 调试器/后端 | CW-DAPLink（`VID_C251/PID_F001`），Keil 工程保存的调试器与 CW32L010 Flash 算法 |
| 工程 | `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/Project.uvprojx` |
| 受测提交 | `51d8fcc1c459993ef8edefd1949ac276854114e0` |
| 固件 | `MDK/output/exe/Project.axf` md5 `e0437fd3405531f107bec844d815acf6` |
| HEX | `MDK/output/exe/Project.hex` md5 `c17f42eba0c1c38461c3768c99e68f22` |
| 构建 | `mdk_build rebuild`：0 Error / 1 既有 `main.c(252)` 空循环 warning |
| 下载 1 | 11:53:56：`Erase Done. Programming Done. Verify OK. Application running ...` |
| 下载 2 | 12:08:08：再次 `Verify OK`（运行后调试通道仍可用） |
| 原始日志 | `mdk_flash.log`、`mdk_flash_second.log`、`build_rebuild.log` |

`Verify OK` 仅证明写入与回读校验通过；业务结论见 `test.md`（TEST_PASS，范围限定 ITEM-005）。
