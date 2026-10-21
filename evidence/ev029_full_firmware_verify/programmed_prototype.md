# Programmed Prototype — 本轮实际下载的固件身份

- 能力：`embedded_tester.embedded_verification`
- 项目：dengbei_run10
- 验证范围：n012 固件实现完整交付
- 目标：CW32L010F8 传感器板（当前 Keil 工程 Device 字段）
- 调试/烧录后端：Keil MDK 保存的 CMSIS-DAP（CW-DAPLink）与 CW32L010 Flash 算法
- 下载时间：2026-10-21T05:22:47Z → 05:22:57Z（本地 13:22:47 → 13:22:57）

## 实际下载身份

| 产物 | md5 |
|---|---|
| `MDK/output/exe/Project.axf` | `9cb97918e8832811dfcb00a93646fba1` |
| `MDK/output/exe/Project.hex` | `3af11e7adb21ba43004fc827649adb44` |

源码身份：Git `690d2ee496a1981b5a57d2ca47ccc16204b2096c`（固件交付提交 `c176d02`；
6 个产品源文件 SHA-256 与 `evidence/ev027_item008/source_sha256.txt` 一致，见 `source_sha256.txt`）。

构建：Keil ARMCLANG V6.24，`rebuild` 0 Error / 1 既有 warning（`main.c:252`）；原始日志 `build_rebuild.log`。

## 下载结果（原始日志 `mdk_flash.log`）

```text
Load "...\MDK\output\exe\Project.axf"
Erase Done.Programming Done.Verify OK.Application running ...
Flash Load finished at 13:22:57
```

## 边界

- `Verify OK` 仅证明下载校验通过；**不表示**采样、判定、上报、UART、低功耗或端到端功能合格。
- 下载后的实板观测：4 个采样机会全部 `q=0` + `G s=2 a44=0 a45=0`；详见 `test.md`。
- 本轮未对网关、手机或服务器执行任何烧录/安装。
